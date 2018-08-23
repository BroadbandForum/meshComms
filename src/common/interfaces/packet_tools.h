/*
 *  Broadband Forum IEEE 1905.1/1a stack
 *
 *  Copyright (c) 2017, Broadband Forum
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _PACKET_TOOLS_H_
#define _PACKET_TOOLS_H_

#include "platform.h"
#include <tlv.h> // mac_address

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>

// Auxiliary functions to:
//
//   A) Extract 1, 2 or 4 bytes from a stream received from the network.
//
//   B) Insert  1, 2 or 4 bytes into a stream which is going to be sent into
//      the network.
//
// These functions do three things:
//
//   1. Avoid unaligned memory accesses (which might cause slowdowns or even
//      exceptions on some architectures)
//
//   2. Convert from network order to host order (and the other way)
//
//   3. Advance the packet pointer as many bytes as those which have just
//      been extracted/inserted.

// Extract/insert 1 byte
//
static inline void _E1B(INT8U **packet_ppointer, INT8U *memory_pointer)
{
    *memory_pointer     = **packet_ppointer;
    (*packet_ppointer) += 1;
}

static inline void _I1B(const INT8U *memory_pointer, INT8U **packet_ppointer)
{
    **packet_ppointer   = *memory_pointer;
    (*packet_ppointer) += 1;
}

static inline bool _E1BL(const uint8_t **packet_ppointer, uint8_t *memory_pointer, size_t *length)
{
    if (*length < 1)
    {
        return false;
    }
    else
    {
        _E1B((INT8U **)packet_ppointer, memory_pointer);
        (*length) -= 1;
        return true;
    }
}

static inline bool _I1BL(const uint8_t *memory_pointer, uint8_t **packet_ppointer, size_t *length)
{
    if (*length < 1)
    {
        return false;
    }
    else
    {
        _I1B(memory_pointer, packet_ppointer);
        (*length) -= 1;
        return true;
    }
}


// Extract/insert 2 bytes
//
static inline void _E2B(uint8_t **packet_ppointer, INT16U *memory_pointer)
{
#if _HOST_IS_BIG_ENDIAN_ == 1
    *(((INT8U *)memory_pointer)+0)  = **packet_ppointer; (*packet_ppointer)++;
    *(((INT8U *)memory_pointer)+1)  = **packet_ppointer; (*packet_ppointer)++;
#elif _HOST_IS_LITTLE_ENDIAN_ == 1
    *(((INT8U *)memory_pointer)+1)  = **packet_ppointer; (*packet_ppointer)++;
    *(((INT8U *)memory_pointer)+0)  = **packet_ppointer; (*packet_ppointer)++;
#else
#error You must specify your architecture endianess
#endif
}

static inline void _I2B(const INT16U *memory_pointer, uint8_t **packet_ppointer)
{
#if _HOST_IS_BIG_ENDIAN_ == 1
    **packet_ppointer = *(((const INT8U *)memory_pointer)+0); (*packet_ppointer)++;
    **packet_ppointer = *(((const INT8U *)memory_pointer)+1); (*packet_ppointer)++;
#elif _HOST_IS_LITTLE_ENDIAN_ == 1
    **packet_ppointer = *(((const INT8U *)memory_pointer)+1); (*packet_ppointer)++;
    **packet_ppointer = *(((const INT8U *)memory_pointer)+0); (*packet_ppointer)++;
#else
#error You must specify your architecture endianess
#endif
}

static inline bool _E2BL(const uint8_t **packet_ppointer, uint16_t *memory_pointer, size_t *length)
{
    if (*length < 2)
    {
        return false;
    }
    else
    {
        _E2B((INT8U **)packet_ppointer, memory_pointer);
        (*length) -= 2;
        return true;
    }
}

static inline bool _I2BL(const uint16_t *memory_pointer, uint8_t **packet_ppointer, size_t *length)
{
    if (*length < 2)
    {
        return false;
    }
    else
    {
        _I2B(memory_pointer, packet_ppointer);
        (*length) -= 2;
        return true;
    }
}


// Extract/insert 4 bytes
//
static inline void _E4B(INT8U **packet_ppointer, INT32U *memory_pointer)
{
#if _HOST_IS_BIG_ENDIAN_ == 1
    *(((INT8U *)memory_pointer)+0)  = **packet_ppointer; (*packet_ppointer)++;
    *(((INT8U *)memory_pointer)+1)  = **packet_ppointer; (*packet_ppointer)++;
    *(((INT8U *)memory_pointer)+2)  = **packet_ppointer; (*packet_ppointer)++;
    *(((INT8U *)memory_pointer)+3)  = **packet_ppointer; (*packet_ppointer)++;
#elif _HOST_IS_LITTLE_ENDIAN_ == 1
    *(((INT8U *)memory_pointer)+3)  = **packet_ppointer; (*packet_ppointer)++;
    *(((INT8U *)memory_pointer)+2)  = **packet_ppointer; (*packet_ppointer)++;
    *(((INT8U *)memory_pointer)+1)  = **packet_ppointer; (*packet_ppointer)++;
    *(((INT8U *)memory_pointer)+0)  = **packet_ppointer; (*packet_ppointer)++;
#else
#error You must specify your architecture endianess
#endif
}

static inline void _I4B(const INT32U *memory_pointer, INT8U **packet_ppointer)
{
#if _HOST_IS_BIG_ENDIAN_ == 1
    **packet_ppointer = *(((const INT8U *)memory_pointer)+0); (*packet_ppointer)++;
    **packet_ppointer = *(((const INT8U *)memory_pointer)+1); (*packet_ppointer)++;
    **packet_ppointer = *(((const INT8U *)memory_pointer)+2); (*packet_ppointer)++;
    **packet_ppointer = *(((const INT8U *)memory_pointer)+3); (*packet_ppointer)++;
#elif _HOST_IS_LITTLE_ENDIAN_ == 1
    **packet_ppointer = *(((const INT8U *)memory_pointer)+3); (*packet_ppointer)++;
    **packet_ppointer = *(((const INT8U *)memory_pointer)+2); (*packet_ppointer)++;
    **packet_ppointer = *(((const INT8U *)memory_pointer)+1); (*packet_ppointer)++;
    **packet_ppointer = *(((const INT8U *)memory_pointer)+0); (*packet_ppointer)++;
#else
#error You must specify your architecture endianess
#endif
}

static inline bool _E4BL(const uint8_t **packet_ppointer, uint32_t *memory_pointer, size_t *length)
{
    if (*length < 4)
    {
        return false;
    }
    else
    {
        _E4B((INT8U **)packet_ppointer, memory_pointer);
        (*length) -= 4;
        return true;
    }
}

static inline bool _I4BL(const uint32_t *memory_pointer, uint8_t **packet_ppointer, size_t *length)
{
    if (*length < 4)
    {
        return false;
    }
    else
    {
        _I4B(memory_pointer, packet_ppointer);
        (*length) -= 4;
        return true;
    }
}



// Extract/insert N bytes (ignore endianess)
//
static inline void _EnB(INT8U **packet_ppointer, void *memory_pointer, INT32U n)
{
    PLATFORM_MEMCPY(memory_pointer, *packet_ppointer, n);
    (*packet_ppointer) += n;
}

static inline void _InB(const void *memory_pointer, INT8U **packet_ppointer, INT32U n)
{
    PLATFORM_MEMCPY(*packet_ppointer, memory_pointer, n);
    (*packet_ppointer) += n;
}

static inline bool _EnBL(const uint8_t **packet_ppointer, void *memory_pointer, size_t n, size_t *length)
{
    if (*length < n)
    {
        return false;
    }
    else
    {
        _EnB((INT8U **)packet_ppointer, memory_pointer, n);
        (*length) -= n;
        return true;
    }
}

static inline bool _InBL(const void *memory_pointer, uint8_t **packet_ppointer, size_t n, size_t *length)
{
    if (*length < n)
    {
        return false;
    }
    else
    {
        _InB(memory_pointer, packet_ppointer, n);
        (*length) -= n;
        return true;
    }
}

// Specific instances of _EnBL/_InBL for mac_addresses.
static inline bool _EmBL(const uint8_t **packet_ppointer, mac_address addr, size_t *length)
{
    return _EnBL(packet_ppointer, addr, 6, length);
}

static inline bool _ImBL(const mac_address addr, uint8_t **packet_ppointer, size_t *length)
{
    return _InBL(addr, packet_ppointer, 6, length);
}

#endif
