/*++

Copyright (c) 2022 MobSlicer152

Module Name:

    server.h

Abstract:

    This module contains definitions for the server.

Author:

    MobSlicer152 19-Dec-2022

Revision History:

    19-Dec-2022    MobSlicer152

        Created.

--*/

#pragma once

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mongoose.h"

#include "types.h"

//
// Print a message
//

#define LOG(...) fprintf(stderr, "SERVER: " __VA_ARGS__);

//
// Make a string
//

#define STRINGIZE(x) #x
#define STRINGIZE_EXPAND(x) STRINGIZE(x)

//
// Get the number of elements in an array
//

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

//
// Default port
//

#define DEFAULT_PORT 8000

//
// Poll rate (milliseconds)
//

#define POLL_RATE 1000

//
// Root directory of files
//

#define ROOT_DIR "."

//
// Name of file to serve by default
//

#define STATIC_PAGE "index.html"

//
// Subdirectory where API is accessible
//

#define API_DIR "api"

//
// Make an endpoint string
//

#define MAKE_ENDPOINT(End) "/" API_DIR "/" End

//
// Test endpoint
//

#define TEST_ENDPOINT "test"

//
// Send a user's input to the Google Sheet
//

#define SEND_USER_ENDPOINT "send_user"

static
VOID
HandleEvent(
    struct mg_connection* Connection,
    INT Event,
    PVOID EventData,
    PVOID Data
    );

INT
main(
    IN INT argc,
    IN PCHAR argv[]
    );
