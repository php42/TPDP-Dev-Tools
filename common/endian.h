/*
    Copyright 2018 php42

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <cstring>
#else
#include <stdint.h>
#include <string.h>
#endif

/* endian independent binary functions. obeys alignment and aliasing rules.
 * this is intended for serialization and deserialization of integral types
 * to/from buffers such as file and network buffers. */

/* define ENDIAN_LE_OPTIMIZATION on little endian cpu architectures for performance optimizations
 *
 * strict aliasing rules permit memcpy to be used to convert types, and most
 * compilers will optimize away the temporary variable and the result is a single
 * wide load/store equivalent to a type-pun such as *(uint32_t*)buf */

#ifndef ENDIAN_LE_OPTIMIZATION
#if (defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)) && !defined(ENDIAN_NO_OPTIMIZATION)
#define ENDIAN_LE_OPTIMIZATION
#endif
#endif // ENDIAN_LE_OPTIMIZATION

static inline void write_le64(void *dest, uint64_t val)
{
#ifndef ENDIAN_LE_OPTIMIZATION
    unsigned char *d = (unsigned char*)dest;
    for(size_t i = 0; i < sizeof(val); ++i)
    {
        d[i] = (unsigned char)val;
        val >>= 8;
    }
#else
    memcpy(dest, &val, sizeof(val));
#endif // ENDIAN_LE_OPTIMIZATION
}

static inline uint64_t read_le64(const void *src)
{
#ifndef ENDIAN_LE_OPTIMIZATION
    const unsigned char *s = (const unsigned char*)src;
    return (((uint64_t)s[0]) | (((uint64_t)s[1]) << 8) | (((uint64_t)s[2]) << 16) | (((uint64_t)s[3]) << 24)
            | (((uint64_t)s[4]) << 32) | (((uint64_t)s[5]) << 40) | (((uint64_t)s[6]) << 48) | (((uint64_t)s[7]) << 56));
#else
    uint64_t temp;
    memcpy(&temp, src, sizeof(temp));
    return temp;
#endif // ENDIAN_LE_OPTIMIZATION
}

static inline void write_le32(void *dest, uint32_t val)
{
#ifndef ENDIAN_LE_OPTIMIZATION
    unsigned char *d = (unsigned char*)dest;
    for(size_t i = 0; i < sizeof(val); ++i)
    {
        d[i] = (unsigned char)val;
        val >>= 8;
    }
#else
    memcpy(dest, &val, sizeof(val));
#endif // ENDIAN_LE_OPTIMIZATION
}

static inline uint32_t read_le32(const void *src)
{
#ifndef ENDIAN_LE_OPTIMIZATION
    const unsigned char *s = (const unsigned char*)src;
    return (((uint32_t)s[0]) | (((uint32_t)s[1]) << 8) | (((uint32_t)s[2]) << 16) | (((uint32_t)s[3]) << 24));
#else
    uint32_t temp;
    memcpy(&temp, src, sizeof(temp));
    return temp;
#endif // ENDIAN_LE_OPTIMIZATION
}

static inline void write_le16(void *dest, uint16_t val)
{
#ifndef ENDIAN_LE_OPTIMIZATION
    unsigned char *d = (unsigned char*)dest;
    for(size_t i = 0; i < sizeof(val); ++i)
    {
        d[i] = (unsigned char)val;
        val >>= 8;
    }
#else
    memcpy(dest, &val, sizeof(val));
#endif // ENDIAN_LE_OPTIMIZATION
}

static inline uint16_t read_le16(const void *src)
{
#ifndef ENDIAN_LE_OPTIMIZATION
    const unsigned char *s = (const unsigned char*)src;
    return (((uint16_t)s[0]) | (((uint16_t)s[1]) << 8));
#else
    uint16_t temp;
    memcpy(&temp, src, sizeof(temp));
    return temp;
#endif // ENDIAN_LE_OPTIMIZATION
}
