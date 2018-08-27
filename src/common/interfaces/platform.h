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
  // NOTE: This is part of the C standard, thus *all* platforms should have it
  // available... and that's why this include can exist in this "platform
  // independent" file

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
//  INT8U  |                             These types must be adjusted to that
//  INT16U |---------------------------> they represent 1, 2 and 4 bytes
//  INT32U |                             unsigned integers respectively.
//
//  INT8S  |                             These types must be adjusted to that
//  INT16S |---------------------------> they represent 1, 2 and 4 bytes signed
//  INT32S |                             integers respectively.
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

#if !defined(INT8U) || !defined(INT16U) || !defined(INT32U)
#  error  "You must define 'INT8U', 'INT16U' and 'INT32U' to represent 8, 16 and 32 bit unsigned integers respectively"
#endif

#if !defined(INT8S) || !defined(INT16S) || !defined(INT32S)
#  error  "You must define 'INT8S', 'INT16S' and 'INT32S' to represent 8, 16 and 32 bit signed integers respectively"
#endif


////////////////////////////////////////////////////////////////////////////////
// Typical libc stuff
////////////////////////////////////////////////////////////////////////////////

// Allocate a chunk of 'n' bytes and return a pointer to it.
//
// If no memory can be allocated, this function must *not* return (instead of
// returning a NULL pointer), and the program must be exited immediately.
//
void *PLATFORM_MALLOC(INT32U size);

// Free a memory area previously obtained with "PLATFORM_MALLOC()"
//
void PLATFORM_FREE(void *ptr);

// Redimendion a memory area previously obtained  with "PLATFORM_MALLOC()"
//
// If no memory can be allocated, this function must *not* return (instead of
// returning a NULL pointer), and the program must be exited immediately.
//
void *PLATFORM_REALLOC(void *ptr, INT32U size);

// Append up to 'n' characters (in addition to the terminating NULL character)
// from 'src' into 'dest'
//
char *PLATFORM_STRNCAT(char *dest, const char *src, INT32U n);

// Output to a string (see 'man 3 snprintf' on any Linux box)
//
void PLATFORM_SNPRINTF(char *dest, INT32U n, const char *format, ...);

// Output to a string ("va" version, see 'man 3 vsnprintf' on any Linux box)
//
void PLATFORM_VSNPRINTF(char *dest, INT32U n, const char *format, va_list ap);

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
INT32U PLATFORM_GET_TIMESTAMP(void);


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
INT8U PLATFORM_INIT(void);

#endif
