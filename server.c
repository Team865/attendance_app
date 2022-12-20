/*++

Copyright (c) 2022 MobSlicer152

Module Name:

    server.c

Abstract:

    This module implements the server.

Author:

    MobSlicer152 19-Dec-2022

Revision History:

    19-Dec-2022    MobSlicer152

        Created.

--*/

#include "server.h"

static
VOID
HandleEvent(
    struct mg_connection* Connection,
    INT Event,
    PVOID EventData,
    PVOID Data
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
			char Number[9];
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
				mg_url_decode(
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
			else if (NameLen > 0 && NumberLen <= 0)
			{
				LOG("Invalid number (query %s)\n", HttpMessage->query.ptr);
				mg_http_reply(
                    Connection,
                    400,
					"Content-Type: text/plain\r\n",
					"Invalid number"
                    );
            }
			else if (NameLen <= 0 && NumberLen > 0)
			{
				LOG("Invalid name (query %s)\n", HttpMessage->query.ptr);
				mg_http_reply(
					Connection,
					400,
					"Content-Type: text/plain\r\n",
					"Invalid name"
                    );
			}
			else if (NameLen <= 0 && NumberLen <= 0)
			{
				LOG("Invalid name and number (query %s)\n", HttpMessage->query.ptr);
				mg_http_reply(
					Connection,
					400,
					"Content-Type: text/plain\r\n",
					"Invalid name and number"
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
	else
	{
		//LOG("Ignoring event %d\n", Event);
    }
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

    LOG("Initializing\n");
    mg_mgr_init(&Manager);

    LOG("Listening on port :" STRINGIZE_EXPAND(DEFAULT_PORT) "\n");
    mg_http_listen(
        &Manager,
		"http://localhost:" STRINGIZE_EXPAND(DEFAULT_PORT),
        HandleEvent,
        &Manager
        );

    LOG("Polling every " STRINGIZE_EXPAND(POLL_RATE) "ms\n");
    while (1)
	{
        mg_mgr_poll(
            &Manager,
            POLL_RATE
            );
    }

    LOG("Shutting down\n");
    mg_mgr_free(&Manager);
	return 0;
}
