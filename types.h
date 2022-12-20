/*++

Copyright (c) 2022 MobSlicer152

Module Name:

    types.h

Abstract:

    This module contains types and macros.

Author:

    MobSlicer152 19-Dec-2022

Revision History:

    19-Dec-2022    MobSlicer152

        Created.

--*/

#pragma once

#include <inttypes.h>
#include <stddef.h>

//
// Dummy macros
//

#define IN
#define OUT
#define OPTIONAL

//
// Basic types
//

#define VOID void
typedef char CHAR;
typedef unsigned char UCHAR;
typedef UCHAR BYTE;
typedef bool BOOLEAN;
typedef int INT;
typedef unsigned int UINT;
typedef float FLOAT;
typedef double DOUBLE;
typedef int8_t INT8;
typedef int16_t INT16;
typedef INT16 WCHAR;
typedef int32_t INT32;
typedef int64_t INT64;
typedef intptr_t INTPTR;
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef uintptr_t UINTPTR;

//
// Size types
//

typedef size_t SIZE_T;
#if SIZE_WIDTH == 32
typedef INT32 SSIZE_T;
#else
typedef INT64 SSIZE_T;
#endif

//
// Pointer types
//

typedef void* PVOID;
typedef CHAR* PCHAR;
typedef UCHAR* PUCHAR;
typedef BYTE* PBYTE;
typedef BOOLEAN* PBOOLEAN;
typedef INT* PINT;
typedef UINT* PUINT;
typedef FLOAT* PFLOAT;
typedef DOUBLE* PDOUBLE;
typedef INT8* PINT8;
typedef INT16* PINT16;
typedef WCHAR* PWCHAR;
typedef INT32* PINT32;
typedef INT64* PINT64;
typedef INTPTR* PINTPTR;
typedef UINT8* PUINT8;
typedef UINT16* PUINT16;
typedef UINT32* PUINT32;
typedef UINT64* PUINT64;
typedef UINTPTR* PUINTPTR;
typedef SIZE_T* PSIZE_T;
typedef SSIZE_T* PSSIZE_T;

//
// Constant pointer types
//

typedef const void* PCVOID;
typedef const CHAR* PCCHAR;
typedef const UCHAR* PCUCHAR;
typedef const BYTE* PCBYTE;
typedef const BOOLEAN* PCBOOLEAN;
typedef const INT* PCINT;
typedef const UINT* PCUINT;
typedef const FLOAT* PCFLOAT;
typedef const DOUBLE* PCDOUBLE;
typedef const INT8* PCINT8;
typedef const INT16* PCINT16;
typedef const WCHAR* PCWCHAR;
typedef const INT32* PCINT32;
typedef const INT64* PCINT64;
typedef const INTPTR* PCINTPTR;
typedef const UINT8* PCUINT8;
typedef const UINT16* PCUINT16;
typedef const UINT32* PCUINT32;
typedef const UINT64* PCUINT64;
typedef const UINTPTR* PCUINTPTR;
typedef const SIZE_T* PCSIZE_T;
typedef const SSIZE_T* PCSSIZE_T;
