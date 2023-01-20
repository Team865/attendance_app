/*++

Copyright (c) 2022 MobSlicer152

Module Name:

    server.c

Abstract:

    This module implements the server.

--*/

#include "server.h"
#include "curl/easy.h"

CHAR Url[128];
PCHAR SpreadsheetId;
PCHAR GoogleOauth2ClientJson;
PCHAR GoogleOauth2AuthUri;
PCHAR GoogleOauth2TokenUri;
PCHAR GoogleOauth2ClientId;
PCHAR GoogleOauth2ClientSecret;
BOOLEAN HaveGoogleOauth2Token;
PCHAR GoogleOauth2Token;
PCHAR TlsCertPath;
PCHAR TlsKeyPath;
UINT16 Port;
INT PollRate;
PCHAR Email;

CHAR GoogleAuthCode[256];
CHAR GoogleAuthState[256];
CHAR GoogleOauth2AccessToken[128];
UINT64 TimeOfLastRefresh;
UINT16 TimeUntilRefresh;
BOOLEAN HaveGoogleAuthCode;

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
        CHAR Query[1024];
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
			{
			    NameLen = strlen(Name);
            }

            NumberLen = mg_http_get_var(
                &QueryMgStr,
                "number",
                Number,
                ARRAY_SIZE(Number)
                );
            if ( NumberLen == -3 )
			{
			    NumberLen = strlen(Number);
            }

            if ( NameLen > 0 && NumberLen > 0 )
            {
                LOG("Received name %s and number %s\n", Name, Number);

                // Warnings must start with a newline for frontend
                if ( atoi(Number) < 100000000 )
				{
				    Warning = "\nNumber is invalid or less than 9 digits";
                }
                else
				{
                    Warning = "";
                }

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
        else if ( mg_http_match_uri(
                    HttpMessage,
                    MAKE_ENDPOINT(OAUTH_ENDPOINT)
                    ) )
        {
            LOG("Received authentication response from Google\n");

            if ( !HaveGoogleAuthCode )
			{
                mg_http_get_var(
                    &QueryMgStr,
                    "code",
                    GoogleAuthCode,
                    ARRAY_SIZE(GoogleAuthCode)
                    );
                mg_http_get_var(
                    &QueryMgStr,
                    "state",
                    GoogleAuthState,
                    ARRAY_SIZE(GoogleAuthState)
                    );
                HaveGoogleAuthCode = TRUE;
            }
			else
			{

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

SIZE_T
CurlWrite(
    IN PVOID Pointer,
    IN SIZE_T Size,
    IN SIZE_T Count,
    IN PVOID Buffer
    )
/*++

Routine Description:

    Writes curl'd data to a buffer (must be 1024 bytes).

Arguments:

    Pointer - Input data to write.

    Size - Size of elements.

    Count - Number of elements.

    Buffer - Buffer to write to. Must be at least 1024 bytes.

Return Value:

    Returns the number of bytes written.

--*/
{
    SIZE_T AmountCopied;

    LOG("Writing data for cURL\n");
    AmountCopied = MAX(Size * Count, 1024);
    memcpy(
        Buffer,
        Pointer,
        AmountCopied
        );
    if ( Buffer && Size == sizeof(CHAR) && strnlen(Buffer, Size * Count) > 0 )
	{
	    LOG("Data:\n%s\n", (PCHAR)Buffer);
    }
    HaveGoogleOauth2Token = TRUE;
    return AmountCopied;
}

BOOLEAN
AuthenticateGoogle(
    IN PVOID Parameter
    )
/*++

Routine Description:

    This routine gets an OAuth token for the Google Sheets API.

    TODO: Much of the RNG code should be put in a more reusable function.

Arguments:

    None.

Return Value:

    TRUE - Obtaining the token was successful.

    FALSE - Obtaining the token failed.

--*/
{
    /*
    BYTE RandomBytes[64];
    CHAR State[33];
    CHAR CodeVerifier[129];
    CHAR CodeSha256[65];
    CHAR CodeChallenge[172];
    */
    CHAR RequestUrl[1024];
    UINT Error;
    CURL* Curl;
    CHAR Response[1024];
    INT i;
    struct curl_slist* HttpHeader;
    cJSON* JsonResponseRoot;
    cJSON* JsonObject;

    (Parameter);

    /*LOG("Creating challenge code\n");

    CodeVerifier[0] = 0;
    Error = psa_generate_random(
        RandomBytes,
        sizeof(RandomBytes)
        );
    while ( Error != PSA_SUCCESS )
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
            CodeVerifier + i * 2,
            ARRAY_SIZE(CodeVerifier) - i * 2,
            "%02X",
            RandomBytes[i]
            );
        if ( !CodeVerifier[i * 2] || !CodeVerifier[i * 2 + 1] )
    	{
            CodeVerifier[i * 2] = '0';
            CodeVerifier[i * 2 + 1] = '0';
    	}
    }
    CodeVerifier[ARRAY_SIZE(CodeVerifier) - 1] = 0;

    LOG("Encoding challenge verifier \"%s\"\n", CodeVerifier);

    if ( mbedtls_sha256(
        CodeVerifier,
        ARRAY_SIZE(CodeVerifier),
        CodeSha256,
        FALSE
        ) )
    {
        LOG("Failed to compute SHA256 of %s\n", CodeVerifier);
        return FALSE;
    }
    mg_base64_encode(
        CodeSha256,
        ARRAY_SIZE(CodeSha256),
        CodeChallenge
        );
    memset(
        CodeSha256,
        0,
        sizeof(CodeSha256)
        );
    if ( strchr(
            CodeChallenge,
            '='
            ) )
    {
        *strchr(
            CodeChallenge,
            '='
            ) = 0;
    }

    for ( i = 0; i < ARRAY_SIZE(CodeChallenge); i++ )
    {
        if ( CodeChallenge[i] == '+' )
        {
            CodeChallenge[i] = '-';
        }
        else if ( CodeChallenge[i] == '/' )
        {
            CodeChallenge[i] = '_';
        }
    }*/

    snprintf(
        RequestUrl,
        ARRAY_SIZE(RequestUrl),
        "%s?"
        "response_type=code&"
        "scope=openid%%20profile&"
        "redirect_uri=https://localhost:%d" MAKE_ENDPOINT(OAUTH_ENDPOINT) "&"
        "client_id=%s&"
		"state=asdf&"
        //"code_challenge=%s&"
        //"code_challenge_method=S256&"
        "login_hint=%s",
        GoogleOauth2AuthUri,
        Port,
        GoogleOauth2ClientId,
        //CodeChallenge,
        Email
        );

    LOG("Visit this URL to authenticate: %s\n", RequestUrl);
	HaveGoogleAuthCode = FALSE;
    while ( !HaveGoogleAuthCode )
	{
        ;
    }

    HttpHeader = NULL;
    HttpHeader = curl_slist_append(
        HttpHeader,
        "Content-Type: application/x-www-form-urlencoded"
        );
    snprintf(
        RequestUrl,
        ARRAY_SIZE(RequestUrl),
        "client_id=%s&"
        "client_secret=%s&"
        "code=%s&"
        //"code_verifier=%s&"
		"redirect_uri=https%%3A//localhost%%3A%hu" MAKE_ENDPOINT(OAUTH_ENDPOINT) "&"
		"grant_type=authorization_code",
        GoogleOauth2ClientId,
		GoogleOauth2ClientSecret,
	    GoogleAuthCode,
        Port
		//CodeVerifier
        );
    LOG("Requesting token:\n%s\n", RequestUrl);
    HaveGoogleOauth2Token = FALSE;
    Curl = curl_easy_init();
    curl_easy_setopt(
        Curl,
        CURLOPT_PROTOCOLS,
        CURLPROTO_HTTPS
        );
    curl_easy_setopt(
        Curl,
        CURLOPT_URL,
        "https://oauth2.googleapis.com/token"
        );
    curl_easy_setopt(
        Curl,
        CURLOPT_HTTPHEADER,
        HttpHeader
        );
    curl_easy_setopt(
        Curl,
        CURLOPT_POSTFIELDS,
        RequestUrl
        );
    curl_easy_setopt(
        Curl,
        CURLOPT_POSTFIELDSIZE,
        strlen(RequestUrl) + 1
        );
    curl_easy_setopt(
        Curl,
        CURLOPT_SSL_VERIFYPEER,
        FALSE
        );
    curl_easy_setopt(
        Curl,
        CURLOPT_WRITEDATA,
        Response
        );
    curl_easy_setopt(
        Curl,
        CURLOPT_WRITEFUNCTION,
        CurlWrite
        );
    curl_easy_perform(Curl);
    curl_easy_cleanup(Curl);
    
    JsonResponseRoot = cJSON_Parse(Response);
    JsonObject = cJSON_GetObjectItem(
        JsonResponseRoot,
        "access_token"
        );
    if ( JsonObject )
	{
        strncpy(
            GoogleOauth2AccessToken,
            cJSON_GetStringValue(JsonObject),
            ARRAY_SIZE(GoogleOauth2AccessToken)
            );
		JsonObject = cJSON_GetObjectItem(
			JsonResponseRoot,
			"refresh_token"
            );
		GoogleOauth2Token = strdup(
            cJSON_GetStringValue(JsonObject)
            );
        TimeUntilRefresh = cJSON_GetNumberValue(JsonObject);
        TimeOfLastRefresh = time(NULL);

        LOG("Set server.google_oauth2_token to \"%s\"", GoogleOauth2Token);
        exit(0);
	}
	else
	{
		LOG("Failed to get tokens:\n%s\n", Response);
		exit(1);
    }
}

BOOLEAN
ParseConfiguration(
    VOID
    )
/*++

Routine Description:

    This routine parses the server configuration.

Arguments:

    None.

Return Value:

    TRUE - The configuration file was successfully parsed.

    FALSE - The configuration file could not be parsed.

--*/
{
    BOOLEAN Error;
	FILE* ConfigFile;
	toml_table_t* Config = NULL;
	toml_table_t* Server;
	char TomlErrorBuffer[128];
	toml_datum_t TomlDatum;

	Error = FALSE;

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
		Error = TRUE;
		goto Cleanup;
	}

	TomlDatum = toml_string_in(
		Server,
		"spreadsheet_id"
        );
	if ( !TomlDatum.ok )
	{
		LOG("Config missing server.spreadsheet_id: %s\n", TomlErrorBuffer);
		Error = TRUE;
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
		Error = TRUE;
		goto Cleanup;
	}
	GoogleOauth2ClientJson = TomlDatum.u.s;

	TomlDatum = toml_string_in(
		Server,
		"google_oauth2_token"
        );
	if ( !TomlDatum.ok )
	{
		LOG("Config missing server.google_oauth2_token: %s\n", TomlErrorBuffer);
		Error = TRUE;
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
		Error = TRUE;
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
		Error = TRUE;
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
		Error = TRUE;
		goto Cleanup;
	}
	Port = TomlDatum.u.i;

	TomlDatum = toml_int_in(
		Server,
		"poll_rate");
	if ( !TomlDatum.ok )
	{
		LOG("Config missing server.poll_rate: %s\n", TomlErrorBuffer);
		Error = TRUE;
		goto Cleanup;
	}
	PollRate = TomlDatum.u.i;

	TomlDatum = toml_string_in(
		Server,
		"email"
        );
	if ( !TomlDatum.ok )
	{
		LOG("Config missing server.email: %s\n", TomlErrorBuffer);
		Error = TRUE;
		goto Cleanup;
	}
	Email = TomlDatum.u.s;

Cleanup:
	if ( Config )
	{
		LOG("Freeing config file\n");
		toml_free(Config);
	}

    return !Error;
}

BOOLEAN
ParseOauth2ClientJson(
    VOID
    )
/*++

Routine Description:

    This routine parses the Google OAuth2 client JSON.

    TODO: It does not have enough error checking and
    assumes the file has the required fields.

Arguments:

    None.

Return Value:

    TRUE - Parsing the JSON was successful.

    FALSE - Parsing the JSON failed.

--*/
{
    BOOLEAN Error;
	FILE* ClientJsonFile = NULL;
	PCHAR ClientJsonBuffer = NULL;
	SIZE_T ClientJsonLength;
	cJSON* ClientJson = NULL;
	cJSON* ClientJsonInstalled = NULL;
	cJSON* JsonObject = NULL;

	Error = FALSE;

	LOG("Parsing OAuth2 client data in %s\n", GoogleOauth2ClientJson);
	ClientJsonFile = fopen(
		GoogleOauth2ClientJson,
		"rb"
        );
	if ( !ClientJsonFile )
	{
		LOG("Failed to open file \"%s\" in read mode: %s (errno %d)\n", GoogleOauth2ClientJson, ERRNO_STRING());
        Error = TRUE;
		goto Cleanup;
	}

	fseek(
		ClientJsonFile,
		0,
		SEEK_END
        );
	ClientJsonLength = ftell(ClientJsonFile) + 1;
	fseek(
		ClientJsonFile,
		0,
		SEEK_SET
        );

	ClientJsonBuffer = calloc(
		ClientJsonLength,
		1
        );
	if ( !ClientJsonBuffer )
	{
		LOG("Failed to allocate %zu-byte buffer for client JSON: %s (errno %d)\n", ClientJsonLength, ERRNO_STRING());
		Error = TRUE;
		goto Cleanup;
	}

	fread(
		ClientJsonBuffer,
		ClientJsonLength,
		1,
		ClientJsonFile
        );

	ClientJson = cJSON_Parse(ClientJsonBuffer);
	if ( !ClientJson )
	{
		LOG("Failed to parse client JSON: %s", cJSON_GetErrorPtr());
		Error = TRUE;
		goto Cleanup;
	}

	ClientJsonInstalled = cJSON_GetObjectItem(
		ClientJson,
		"installed"
        );

	JsonObject = cJSON_GetObjectItem(
		ClientJsonInstalled,
		"auth_uri"
        );
	GoogleOauth2AuthUri = strdup(cJSON_GetStringValue(JsonObject));

	JsonObject = cJSON_GetObjectItem(
		ClientJsonInstalled,
		"client_id"
        );
	GoogleOauth2ClientId = strdup(cJSON_GetStringValue(JsonObject));

	JsonObject = cJSON_GetObjectItem(
		ClientJsonInstalled,
		"client_secret"
        );
	GoogleOauth2ClientSecret = strdup(cJSON_GetStringValue(JsonObject));

	JsonObject = cJSON_GetObjectItem(
		ClientJsonInstalled,
		"token_uri"
        );
	GoogleOauth2TokenUri = strdup(cJSON_GetStringValue(JsonObject));

Cleanup:
	if ( ClientJson )
	{
		cJSON_Delete(ClientJson);
	}

	if ( ClientJsonBuffer )
	{
		free(ClientJsonBuffer);
	}

	if ( ClientJsonFile )
	{
		fclose(ClientJsonFile);
	}

    return !Error;
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

    0 - Success.

    errno code - An appropriate code for the error that occured (though not always).

--*/
{
    struct mg_mgr Manager;
    PVOID AuthenticationThread;

    LOG("Initializing\n");
    mg_mgr_init(&Manager);
    psa_crypto_init();

    LOG("Registering signal handlers\n");
    signal(SIGINT, HandleSignal);
    signal(SIGTERM, HandleSignal);

    if ( !ParseConfiguration() )
	{
        goto Cleanup;
    }

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
		if ( !ParseOauth2ClientJson() )
		{
			goto Cleanup;
		}
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

    if ( !strlen(GoogleOauth2Token) )
    {
        LOG("Starting OAuth thread\n");
        AuthenticationThread = NULL;
#ifdef _WIN32
        AuthenticationThread = CreateThread(
            0,
            0,
            (LPTHREAD_START_ROUTINE)AuthenticateGoogle,
            NULL,
            0,
            NULL
            );
#else
        pthread_create(
            &AuthenticationThread,
            NULL,
            AuthenticateGoogle,
            NULL
            );
#endif
        if ( !AuthenticationThread )
        {
            LOG("Failed to create thread\n");
            goto Cleanup;
        }
    }

    LOG("Polling every %dms\n", PollRate);
    while (LastSignal == 0)
    {
        mg_mgr_poll(
            &Manager,
            PollRate
            );
        if ( time(NULL) - TimeOfLastRefresh > TimeUntilRefresh )
		{
            RefreshGoogleToken();
        }
    }

    errno = 0;
Cleanup:
    LOG("Shutting down\n");

    mg_mgr_free(&Manager);
    return errno;
}
