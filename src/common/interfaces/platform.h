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

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <stdarg.h>   // va_list
#include <stddef.h>
#include <stdint.h>

////////////////////////////////////////////////////////////////////////////////
// Hardware stuff
////////////////////////////////////////////////////////////////////////////////

// The following preprocessor macros must be defined to a platform-dependent
// value:
//
//   _HOST_IS_LITTLE_ENDIAN_        |--> Set one (and only one!) of these macros
//   _HOST_IS_BIG_ENDIAN_ENDIAN_    |    to "1" to indicate your CPU endianness
//
//
//   MAX_NETWORK_SEGMENT_SIZE  --------> This is the maximum packet size that
//                                       is allowed in your platform. It is
//                                       used to 'fragment' CMDUs.  Note that
//                                       even if your platform supports packets
//                                       bigger than 1500 bytes, this macro
//                                       must never be bigger than that.  This
//                                       macro is only present in this file for
//                                       those special cases where, for some
//                                       platform related reason, packets must
//                                       be smaller than 1500.
//
//
// In the next few lines we are just going to check that these are defined,
// nothing else.
// In order to actually define them use the "root" Makefile where these MACROS
// are sent to the compiler using the "-D flag" (open the "root" Makefile and
// search for "CCFLAGS" to understand how to do this)

#if !defined(_HOST_IS_LITTLE_ENDIAN_) && !defined(_HOST_IS_BIG_ENDIAN_ENDIAN_)
#  error  "You must define either '_HOST_IS_LITTLE_ENDIAN_' or '_HOST_IS_BIG_ENDIAN_'"
#elif defined(_HOST_IS_LITTLE_ENDIAN_) && defined(_HOST_IS_BIG_ENDIAN_ENDIAN_)
#  error  "You cannot define both '_HOST_IS_LITTLE_ENDIAN_' and '_HOST_IS_BIG_ENDIAN_' at the same time"
#endif

#ifndef  MAX_NETWORK_SEGMENT_SIZE
#  error  "You must define 'MAX_NETWORK_SEGMENT_SIZE' to some value (for example, '1500')"
#endif


////////////////////////////////////////////////////////////////////////////////
// Typical libc stuff
////////////////////////////////////////////////////////////////////////////////

// Free a memory area previously obtained with "memalloc()"
//
void PLATFORM_FREE(void *ptr);

// Output the provided format string (see 'man 3 printf' on any Linux box)
//
void PLATFORM_PRINTF(const char *format, ...);

// Same as 'PLATFORM_PRINTF', but the message will only be processed if the
// platform has the pertaining debug level enabled
//
void PLATFORM_PRINTF_DEBUG_ERROR(const char *format, ...);
void PLATFORM_PRINTF_DEBUG_WARNING(const char *format, ...);
void PLATFORM_PRINTF_DEBUG_INFO(const char *format, ...);
void PLATFORM_PRINTF_DEBUG_DETAIL(const char *format, ...);

// Used to set the verbosity of the previous functions:
//
//   0 => Only print ERROR messages
//   1 => Print ERROR and WARNING messages
//   2 => Print ERROR, WARNING and INFO messages
//   3 => Print ERROR, WARNING, INFO and DETAIL messages
//
void PLATFORM_PRINTF_DEBUG_SET_VERBOSITY_LEVEL(int level);

// Return the number of milliseconds ellapsed since the program started
//
uint32_t PLATFORM_GET_TIMESTAMP(void);


////////////////////////////////////////////////////////////////////////////////
// Misc stuff
////////////////////////////////////////////////////////////////////////////////

// [PLATFORM PORTING NOTE]
//   Depending on what other platform headers you have included up until this
//   point, 'NULL' might or might not be defined. If so, define it here
//
#ifndef NULL
#  define NULL (0x0)
#endif


////////////////////////////////////////////////////////////////////////////////
// Initialization functions
////////////////////////////////////////////////////////////////////////////////

// This function *must* be called before any other "PLATFORM_*()" API function
//
// Returns "0" if there was a problem. "1" otherwise.
//
// [PLATFORM PORTING NOTE]
//   Use this function to reserve memory, initialize semaphores, etc...
//
uint8_t PLATFORM_INIT(void);

#endif
