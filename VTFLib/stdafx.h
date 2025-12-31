/*
 * VTFLib
 * Copyright (C) 2005-2011 Neil Jedrzejewski & Ryan Gregg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

// ============================================================
// NOTE: This file is commented for compatibility with Doxygen.
// ============================================================
/*!
    \file StdAfx.h
    \brief Application framework header plus VTFLib custom data types.
*/

#ifndef STDAFX_H
#define STDAFX_H

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#   if _MSC_VER >= 1400
#       define _CRT_SECURE_NO_WARNINGS
#       define _CRT_NONSTDC_NO_DEPRECATE
#   endif

#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>

#   if defined(VTFLIB_STATIC)
#       define VTFLIB_API
#   elif defined(VTFLIB_EXPORTS)
#       define VTFLIB_API __declspec(dllexport)
#   else
#       define VTFLIB_API __declspec(dllimport)
#   endif
#else
#   ifndef __cdecl
#       define __cdecl
#   endif
#   ifndef __stdcall
#       define __stdcall
#   endif
#   if defined(VTFLIB_STATIC)
#       define VTFLIB_API
#   else
#       define VTFLIB_API __attribute__((visibility("default")))
#   endif
#endif

// Custom data types
typedef uint8_t     vlBool;   //!< Boolean value 0/1.
typedef char        vlChar;   //!< Single signed character.
typedef uint8_t     vlByte;   //!< Single unsigned byte.
typedef int16_t     vlShort;  //!< Signed short floating point value.
typedef uint16_t    vlUShort; //!< Unsigned short floating point value.
typedef int32_t     vlInt;    //!< Signed integer value.
typedef uint32_t    vlUInt;   //!< Unsigned integer value.
typedef int64_t     vlLong;   //!< Signed long number.
typedef uint64_t    vlULong;  //!< Unsigned long number.
typedef float       vlSingle; //!< Floating point number
typedef double      vlDouble; //!< Double number
typedef void        vlVoid;   //!< Void value.

typedef uint8_t     vlUInt8;
typedef uint16_t    vlUInt16;
typedef uint32_t    vlUInt32;
typedef uint64_t    vlUInt64;

typedef vlSingle    vlFloat; //!< Floating point number (same as vlSingle).

#define vlFalse 0
#define vlTrue  1

#ifndef FILE_BEGIN
#   define FILE_BEGIN 0
#endif
#ifndef FILE_CURRENT
#   define FILE_CURRENT 1
#endif
#ifndef FILE_END
#   define FILE_END 2
#endif

#ifndef _WIN32
#   include <strings.h>
#   define _stricmp strcasecmp
#   define stricmp  strcasecmp
#endif

#if _MSC_VER >= 1600 // Visual Studio 2010
#   define STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#   define STATIC_ASSERT(condition, message) typedef char __C_ASSERT__[(condition) ? 1 : -1]
#endif

#endif
