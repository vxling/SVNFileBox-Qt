/* zipconf.h -- libzip configuration for Windows/MSVC
   This file is required because the SDK's libzip was built with
   MSVC and this header provides the platform-specific type definitions
   that zip.h depends on.
*/

#ifndef _HAD_ZIPCONF_H
#define _HAD_ZIPCONF_H

#include <stdint.h>

typedef int8_t  zip_int8_t;
typedef uint8_t zip_uint8_t;
typedef int64_t zip_int64_t;
typedef uint64_t zip_uint64_t;
typedef intptr_t zip_intmax_t;
typedef uintptr_t zip_uintmax_t;
typedef long zip_int32_t;
typedef unsigned long zip_uint32_t;

/* MSVC: ssize_t is defined in stddef.h but we use a signed type */
#if defined(_MSC_VER)
#include <stddef.h>
typedef SSIZE_T zip_ssize_t;
#else
typedef long zip_ssize_t;
#endif

#define ZIP_STATIC

/* These are typically defined in the libzip build */
#ifndef ZIP_EXTERN
#ifdef _WIN32
#define ZIP_EXTERN __declspec(dllimport)
#else
#define ZIP_EXTERN
#endif
#endif

#endif /* _HAD_ZIPCONF_H */
