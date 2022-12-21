/*++

Copyright (c) 2022 MobSlicer152

Module Name:

    server.c

Abstract:

    This module implements the server.

Author:

    MobSlicer152 19-Dec-2022

Revision History:

    21-Dec-2022    MobSlicer152

        Add config file.

    20-Dec-2022    MobSlicer152

        Add API.

    19-Dec-2022    MobSlicer152

        Created.

--*/

#include "server.h"

PCHAR SpreadsheetId;
PCHAR GoogleKey;
UINT16 Port;
INT PollRate;

static
VOID
HandleEvent(
    IN struct mg_connection* Connection,
	IN INT Event,
	IN PVOID EventData,
	IN PVOID Data
    )
/*++

Routine Description:

    This routine handles events from the server.

Arguments:

    Connection - The connection to the server.

    Event - The event.

    EventData - Data for the event.

    Data - Data for the function, not used.

Return Value:

    None.

--*/
{
    struct mg_http_serve_opts StaticOptions = {.root_dir = ROOT_DIR};

    if (Event == MG_EV_HTTP_MSG)
    {
        struct mg_http_message* HttpMessage = EventData;
        struct mg_str* Host = mg_http_get_header(HttpMessage, "Host");

        LOG("Serving host %s\n", Host->ptr);
        if (mg_http_match_uri(
                HttpMessage,
                MAKE_ENDPOINT(TEST_ENDPOINT)
                ))
        {
            mg_http_reply(
                Connection,
                200,
                "Content-Type: text/plain\r\n",
                "yes"
                );
        }
        else if (mg_http_match_uri(
                     HttpMessage,
                     MAKE_ENDPOINT(SEND_USER_ENDPOINT)
                     ))
        {
            char NameRaw[128];
            char Name[128];
            char Number[10];
            int NameLen;
            int NumberLen;

            LOG("Handling send_user\n");

            NameLen = mg_http_get_var(
                &HttpMessage->query,
                "name",
                NameRaw,
                ARRAY_SIZE(NameRaw)
                );
            NumberLen = mg_http_get_var(
                &HttpMessage->query,
                "number",
                Number,
                ARRAY_SIZE(Number)
                );
            if (NameLen > 0 && NumberLen > 0)
            {
                NameLen = mg_url_decode(
                    NameRaw,
                    ARRAY_SIZE(NameRaw),
                    Name,
                    ARRAY_SIZE(Name),
                    true
                    );
                LOG("Received name %s and number %s\n", Name, Number);
                mg_http_reply(
                    Connection,
                    200,
                    "Content-Type: text/plain\r\n",
                    "success\n%s\n%s",
                    Name,
                    Number
                    );
            }
            else if (NameLen <= 0 && NumberLen > 0)
            {
                LOG("Invalid name (query %s)\n", HttpMessage->query.ptr);
                mg_http_reply(
                    Connection,
                    400,
                    "Content-Type: text/plain\r\n",
                    "Invalid name (query %s)\n",
                    HttpMessage->query.ptr
                    );
            }
            else if (NameLen > 0 && NumberLen <= 0)
            {
                LOG("Invalid number (query %s)\n", HttpMessage->query.ptr);
                mg_http_reply(
                    Connection,
                    400,
                    "Content-Type: text/plain\r\n",
                    "Invalid number (query %s)\n",
                    HttpMessage->query.ptr
                    );
            }
            else if (NameLen <= 0 && NumberLen <= 0)
            {
                LOG("Invalid name and number (query %s)\n", HttpMessage->query.ptr);
                mg_http_reply(
                    Connection,
                    400,
                    "Content-Type: text/plain\r\n",
                    "Invalid name and number (query %s)\n",
                    HttpMessage->query.ptr
                    );
            }
        }
        else
        {
            LOG("Serving static content in " ROOT_DIR "/" STATIC_PAGE "\n");
            mg_http_serve_file(
                Connection,
                HttpMessage,
                STATIC_PAGE,
                &StaticOptions
                );
        }
    }
}

static INT LastSignal;
static
VOID
HandleSignal(
    IN INT Signal
    )
/*++

Routine Description:

    Saves signals so the server can exit cleanly.

Arguments:

    Signal

--*/
{
	LastSignal = Signal;
	LOG("Received signal %d\n", LastSignal);
}

INT
main(
    IN INT argc,
    IN PCHAR argv[]
    )
/*++

Routine Description:

    Main function.

Arguments:

    argc - Number of arguments.

    argv - Arguments.

Return Value:

    INT - 0 on success, otherwise an errno code.

--*/
{
    struct mg_mgr Manager;
	FILE* ConfigFile;
	toml_table_t* Config = NULL;
	toml_table_t* Server;
	char TomlErrorBuffer[128];
	toml_datum_t TomlDatum;
	CHAR Url[128];

    LOG("Initializing\n");
    mg_mgr_init(&Manager);

    LOG("Registering signal handlers\n");
	signal(SIGINT, HandleSignal);
	signal(SIGTERM, HandleSignal);

    LOG("Loading configuration " CONFIG_FILE "\n");
	ConfigFile = fopen(
		CONFIG_FILE,
		"r"
        );
	if (!ConfigFile)
	{
		LOG("Failed to open " CONFIG_FILE ": %s (errno %d)\n", ERRNO_STRING());
		goto Cleanup;
    }

    Config = toml_parse_file(
        ConfigFile,
        TomlErrorBuffer,
        ARRAY_SIZE(TomlErrorBuffer)
        );
	fclose(ConfigFile);

    LOG("Parsing config\n");
    Server = toml_table_in(
        Config,
        "server"
        );
	if (!Server)
	{
		LOG("Config missing [server]: %s (errno %d)\n", ERRNO_STRING());
		goto Cleanup;
    }

    TomlDatum = toml_string_in(
        Server,
        "spreadsheet_id"
        );
	if (!TomlDatum.ok)
	{
		LOG("Config missing server.spreadsheet_id: %s (errno %d)\n", ERRNO_STRING());
		goto Cleanup;
    }
	SpreadsheetId = TomlDatum.u.s;

	TomlDatum = toml_string_in(
		Server,
		"google_key"
        );
	if (!TomlDatum.ok)
	{
		LOG("Config missing server.google_key: %s (errno %d)\n", ERRNO_STRING());
		goto Cleanup;
	}
	GoogleKey = TomlDatum.u.s;

	TomlDatum = toml_int_in(
		Server,
		"port"
        );
	if (!TomlDatum.ok)
	{
		LOG("Config missing server.port: %s (errno %d)\n", ERRNO_STRING());
		goto Cleanup;
	}
	Port = TomlDatum.u.i;

	TomlDatum = toml_int_in(
		Server,
		"poll_rate"
        );
	if (!TomlDatum.ok)
	{
		LOG("Config missing server.poll_rate: %s (errno %d)\n", ERRNO_STRING());
		goto Cleanup;
	}
	PollRate = TomlDatum.u.i;

    snprintf(
        Url,
        128,
        "localhost:%hu",
        Port
        );

    LOG("Listening on port :%hu\n", Port);
    mg_http_listen(
        &Manager,
        Url,
        HandleEvent,
        &Manager
        );

    LOG("Polling every %dms\n", PollRate);
    while (LastSignal == 0)
    {
        mg_mgr_poll(
            &Manager,
            PollRate
            );
    }

    errno = 0;
Cleanup:
	LOG("Shutting down\n");

    if (Config)
	{
		LOG("Freeing config file\n");
		toml_free(Config);
	}

    mg_mgr_free(&Manager);
    return errno;
}
