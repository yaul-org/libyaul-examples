/*
 * types.h
 *
 * Copyright (C) 2021 celeriyacon - https://github.com/celeriyacon
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */

#ifndef GIGASANTA_TYPES_H
#define GIGASANTA_TYPES_H

#ifdef __m68k__
 typedef signed char int8;
 typedef signed short int16;
 typedef signed long int32;
 typedef signed long long int64;

 typedef unsigned char uint8;
 typedef unsigned short uint16;
 typedef unsigned long uint32;
 typedef unsigned long long uint64;
#elif defined(__sh2__)
 typedef signed char int8;
 typedef signed short int16;
 typedef signed int int32;
 typedef signed long long int64;

 typedef unsigned char uint8;
 typedef unsigned short uint16;
 typedef unsigned int uint32;
 typedef unsigned long long uint64;
#else
 #include <inttypes.h>

 typedef int8_t int8;
 typedef int16_t int16;
 typedef int32_t int32;
 typedef int64_t int64;

 typedef uint8_t uint8;
 typedef uint16_t uint16;
 typedef uint32_t uint32;
 typedef uint64_t uint64;
#endif

#ifndef __cplusplus
 typedef _Bool bool;
#endif

#define INLINE inline __attribute__((always_inline))

#define sign_extend(n, v) ((int32)((uint32)(v) << (32 - (n))) >> (32 - (n)))

#endif
