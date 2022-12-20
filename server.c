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
		LOG("Serving host %s", mg_http_host);
        if (mg_http_match_uri(
            HttpMessage,
			"/api/test"
            ))
		{
			LOG("Received request:\n%s\n", HttpMessage->message.ptr);
			mg_http_reply(
				Connection,
				200,
				"Content-Type: text/plain\r\n",
				"yes\n"
                );
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
