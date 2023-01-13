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
PCHAR GoogleOauth2Client;
PCHAR GoogleOauth2Token;
PCHAR TlsCertPath;
PCHAR TlsKeyPath;
UINT16 Port;
INT PollRate;

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

    if ( Event == MG_EV_ACCEPT )
    {
        struct mg_tls_opts TlsOptions = {
            .cert = TlsCertPath,
            .certkey = TlsKeyPath
        };

        mg_tls_init(
            Connection,
            &TlsOptions
            );
    }
    else if ( Event == MG_EV_HTTP_MSG )
    {
        struct mg_http_message* HttpMessage = EventData;
        struct mg_str* Host = mg_http_get_header(HttpMessage, "Host");
        CHAR Query[128];
        struct mg_str QueryMgStr;
        PCHAR p;

        p = NULL;
        if ( HttpMessage->query.ptr )
        {
            p = strstr(HttpMessage->query.ptr, " HTTP");
            if ( p )
            {
                size_t Count = MIN(
                        p - HttpMessage->query.ptr,
                        ARRAY_SIZE(Query) - 1
                        );

                strncpy(
                    Query,
                    HttpMessage->query.ptr,
                    Count
                    );
                Query[Count] = 0;

                QueryMgStr.ptr = Query;
                QueryMgStr.len = Count;
            }
        }

        LOG("Serving host %s\n", Host->ptr);
        if ( mg_http_match_uri(
                 HttpMessage,
                 MAKE_ENDPOINT(TEST_ENDPOINT)
                 ) )
        {
            mg_http_reply(
                Connection,
                200,
                "Content-Type: text/plain\r\n",
                "yes"
                );
        }
        else if ( mg_http_match_uri(
                      HttpMessage,
                      MAKE_ENDPOINT(SEND_USER_ENDPOINT)
                      ) )
        {
            CHAR Name[128];
            CHAR Number[10];
            PCCHAR Warning;
            INT NameLen;
            INT NumberLen;

            LOG("Handling send_user\n");

            NameLen = mg_http_get_var(
                &QueryMgStr,
                "name",
                Name,
                ARRAY_SIZE(Name)
                );
            if ( NameLen == -3 )
                NameLen = strlen(Name);
            NumberLen = mg_http_get_var(
                &QueryMgStr,
                "number",
                Number,
                ARRAY_SIZE(Number)
                );
            if ( NumberLen == -3 )
                NumberLen = strlen(Number);
            if ( NameLen > 0 && NumberLen > 0 )
            {
                LOG("Received name %s and number %s\n", Name, Number);

                // Warnings must start with a newline for frontend
                if ( atoi(Number) < 100000000 )
                    Warning = "\nNumber is invalid or less than 9 digits";
                else
                    Warning = "";

                mg_http_reply(
                    Connection,
                    200,
                    "Content-Type: text/plain\r\n",
                    "success\n%s\n%s%s",
                    Name,
                    Number,
                    Warning
                    );
            }
            else if ( NameLen <= 0 && NumberLen > 0 )
            {
                LOG("Invalid name (query %s)\n", p ? Query : "(none)");
                mg_http_reply(
                    Connection,
                    400,
                    "Content-Type: text/plain\r\n",
                    "Invalid name (query %s)\n",
                    p ? Query : "(none)"
                    );
            }
            else if ( NameLen > 0 && NumberLen <= 0 )
            {
                LOG("Invalid number (query %s)\n", p ? Query : "(none)");
                mg_http_reply(
                    Connection,
                    400,
                    "Content-Type: text/plain\r\n",
                    "Invalid number (query %s)\n",
                    p ? Query : "(none)"
                    );
            }
            else if ( NameLen <= 0 && NumberLen <= 0 )
            {
                LOG("Invalid name and number (query %s)\n", p ? Query : "(none)");
                mg_http_reply(
                    Connection,
                    400,
                    "Content-Type: text/plain\r\n",
                    "Invalid name and number (query %s)\n",
                    p ? Query : "(none)"
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

BOOLEAN
AuthenticateGoogle(
    VOID
    )
/*++

Routine Description:

    This routine gets an OAuth token for the Google Sheets API.

Arguments:

    None.

Return Value:

    TRUE - Obtaining the token was successful.

    FALSE - Obtaining the token failed.

--*/
{
    BYTE RandomBytes[64];
    CHAR ChallengeCode[129];
    CHAR ChallengeSha256[65];
    CHAR ChallengeVerifier[172];
    CHAR RequestUrl[1024];
    UINT Error;
    INT i;

    LOG("Creating challenge code\n");

    Error = psa_generate_random(
        RandomBytes,
        sizeof(RandomBytes)
        );
    while (Error != PSA_SUCCESS)
    {
        LOG("Random byte generation failed: PSA status %d\n", Error);
        Error = psa_generate_random(
            RandomBytes,
            sizeof(RandomBytes)
            );
    }
    for ( i = 0; i < ARRAY_SIZE(RandomBytes); i++ )
    {
        snprintf(
            ChallengeCode + i * 2,
            ARRAY_SIZE(ChallengeCode) - i * 2,
            "%X",
            RandomBytes[i]
            );
    }
    ChallengeCode[ARRAY_SIZE(ChallengeCode) - 1] = 0;

    LOG("Encoding challenge verifier \"%s\"\n", ChallengeCode);

    if ( mbedtls_sha256(
        ChallengeCode,
        ARRAY_SIZE(ChallengeCode),
        ChallengeSha256,
        FALSE
        ) )
    {
        LOG("Failed to compute SHA256 of %s\n", ChallengeCode);
        return FALSE;
    }
    mg_base64_encode(
        ChallengeSha256,
        ARRAY_SIZE(ChallengeSha256),
        ChallengeVerifier
        );
    memset(
        ChallengeSha256,
        0,
        sizeof(ChallengeSha256)
        );
    if ( strchr(
            ChallengeVerifier,
            '='
            ) )
    {
        *strchr(
            ChallengeVerifier,
            '='
            ) = 0;
    }

    for ( i = 0; i < ARRAY_SIZE(ChallengeVerifier); i++ )
    {
        if ( ChallengeVerifier[i] == '+' )
        {
            ChallengeVerifier[i] = '-';
        }
        else if ( ChallengeVerifier[i] == '/' )
        {
            ChallengeVerifier[i] = '_';
        }
    }

    snprintf(
        RequestUrl,
        ARRAY_SIZE(RequestUrl),
        "https://www.googleapis.com/oauth2/v4/token?"

    return TRUE;
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
    psa_crypto_init();

    LOG("Registering signal handlers\n");
    signal(SIGINT, HandleSignal);
    signal(SIGTERM, HandleSignal);

    LOG("Loading configuration " CONFIG_FILE "\n");
    ConfigFile = fopen(
        CONFIG_FILE,
        "r"
        );
    if ( !ConfigFile )
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
    if ( !Server )
    {
        LOG("Config missing [server]: %s\n", TomlErrorBuffer);
        goto Cleanup;
    }

    TomlDatum = toml_string_in(
        Server,
        "spreadsheet_id"
        );
    if ( !TomlDatum.ok )
    {
        LOG("Config missing server.spreadsheet_id: %s\n", TomlErrorBuffer);
        goto Cleanup;
    }
    SpreadsheetId = TomlDatum.u.s;

    TomlDatum = toml_string_in(
        Server,
        "google_oauth2_client"
        );
    if ( !TomlDatum.ok )
    {
        LOG("Config missing server.google_oauth2_client: %s\n", TomlErrorBuffer);
        goto Cleanup;
    }
    GoogleOauth2Client = TomlDatum.u.s;

    TomlDatum = toml_string_in(
        Server,
        "google_oauth2_token"
        );
    if ( !TomlDatum.ok )
    {
        LOG("Config missing server.google_oauth2_token: %s\n", TomlErrorBuffer);
        goto Cleanup;
    }
    GoogleOauth2Token = TomlDatum.u.s;

    TomlDatum = toml_string_in(
        Server,
        "tls_cert_path"
        );
    if ( !TomlDatum.ok )
    {
        LOG("Config missing server.tls_cert_path: %s\n", TomlErrorBuffer);
        goto Cleanup;
    }
    TlsCertPath = TomlDatum.u.s;

    TomlDatum = toml_string_in(
        Server,
        "tls_key_path"
        );
    if ( !TomlDatum.ok )
    {
        LOG("Config missing server.tls_key_path: %s\n", TomlErrorBuffer);
        goto Cleanup;
    }
    TlsKeyPath = TomlDatum.u.s;

    TomlDatum = toml_int_in(
        Server,
        "port"
        );
    if ( !TomlDatum.ok )
    {
        LOG("Config missing server.port: %s\n", TomlErrorBuffer);
        goto Cleanup;
    }
    Port = TomlDatum.u.i;

    TomlDatum = toml_int_in(
        Server,
        "poll_rate"
        );
    if ( !TomlDatum.ok )
    {
        LOG("Config missing server.poll_rate: %s\n", TomlErrorBuffer);
        goto Cleanup;
    }
    PollRate = TomlDatum.u.i;

    snprintf(
        Url,
        128,
        "localhost:%hu",
        Port
        );

    LOG("Using spreadsheet ID %s\n", SpreadsheetId);
    if ( strlen(GoogleOauth2Token) )
    {
        LOG("Using OAuth2 token %s\n", GoogleOauth2Token);
    }
    else
    {
        LOG("Using OAuth2 client data in %s\n", GoogleOauth2Client);
    }
    LOG("Using TLS certificate in %s\n", TlsCertPath);
    LOG("Using TLS private key in %s\n", TlsKeyPath);

    LOG("Listening on port :%hu\n", Port);
    mg_http_listen(
        &Manager,
        Url,
        HandleEvent,
        &Manager
        );

    if ( !strlen(GoogleOauth2Token) && !AuthenticateGoogle() )
    {
        goto Cleanup;
    }

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

    if ( Config )
    {
        LOG("Freeing config file\n");
        toml_free(Config);
    }

    mg_mgr_free(&Manager);
    return errno;
}
