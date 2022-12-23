/*++

Copyright (c) 2022 MobSlicer152

Module Name:

    server.h

Abstract:

    This module contains definitions for the server.

Author:

    MobSlicer152 19-Dec-2022

Revision History:

    21-Dec-2022    MobSlicer152

        Switch some preprocessor macros to being in a config file.

    20-Dec-2022    MobSlicer152

        Add more definitions.

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
#include "toml.h"

#include "types.h"

//
// Print a message
//

#define LOG(...) fprintf(stderr, "SERVER: " __VA_ARGS__);

//
// Format string arguments for errno errors
//

#define ERRNO_STRING() strerror(errno), errno

//
// Make a string
//

#define STRINGIZE(x) #x
#define STRINGIZE_EXPAND(x) STRINGIZE(x)

//
// Smaller of two values
//

#define MIN(a, b) ((a) < (b) ? (a) : (b))

//
// Larger of two values
//

#define MAX(a, b) ((a) > (b) ? (a) : (b))

//
// Clamp to range
//

#define CLAMP(x, min, max) ((x) < (max) && (x) > (min) ? (x) : (x) > (max) ? (x) < (min) ? (min) : (x) : (max))

//
// Get the number of elements in an array
//

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

//
// Config file
//

#define CONFIG_FILE "config.toml"

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

//
// Google Sheets spreadsheet ID to send user input to
//

extern PCHAR SpreadsheetId;

//
// Google API key
//

extern PCHAR GoogleKey;

//
// TLS certificate path
//

extern PCHAR TlsCertPath;

//
// TLS private key file path
//

extern PCHAR TlsKeyPath;

//
// Port
//

extern UINT16 Port;

//
// Polling rate
//

extern INT PollRate;

//
// Handle server events
//

static
VOID
HandleEvent(
	IN struct mg_connection* Connection,
	IN INT Event,
	IN PVOID EventData,
	IN PVOID Data
    );

//
// Signal handler
//

static
VOID
HandleSignal(
	IN INT Signal
    );

//
// Send user input to a spreadsheet
//

VOID
SendUser(
	IN PCCHAR Name,
	IN INT NameLen,
	IN PCCHAR Number,
	IN INT NumberLen
    );

//
// Main
//

INT
main(
    IN INT argc,
    IN PCHAR argv[]
    );
