/*
 *  Broadband Forum IEEE 1905.1/1a stack
 *
 *  Copyright (c) 2017, Broadband Forum
 *  Copyright (c) 2018, prpl Foundation
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

#include "platform.h"
#include "platform_linux.h"

#include <stdlib.h>      // free(), malloc(), ...
#include <string.h>      // memcpy(), memcmp(), ...
#include <stdio.h>       // printf(), ...
#include <stdarg.h>      // va_list
#include <sys/time.h>    // gettimeofday()
#include <errno.h>       // errno

#include <arpa/inet.h>        // htons()
#include <linux/if_packet.h>  // sockaddr_ll
#include <net/if.h>           // struct ifreq, IFNAZSIZE
#include <netinet/ether.h>    // ETH_P_ALL, ETH_A_LEN
#include <sys/socket.h>       // socket()
#include <sys/ioctl.h>        // ioctl(), SIOCGIFINDEX
#include <unistd.h>           // close()

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
#    include <pthread.h> // mutexes, pthread_self()
#endif



////////////////////////////////////////////////////////////////////////////////
// Private functions, structures and macros
////////////////////////////////////////////////////////////////////////////////

// *********** libc stuff ******************************************************

// We will use this variable to save the instant when "PLATFORM_INIT()" was
// called. This way we sill be able to get relative timestamps later when
// someone calls "PLATFORM_GET_TIMESTAMP()"
//
static struct timeval tv_begin;

// The following variable is used to set which "PLATFORM_PRINTF_DEBUG_*()"
// functions should be ignored:
//
//   0 => Only print ERROR messages
//   1 => Print ERROR and WARNING messages
//   2 => Print ERROR, WARNING and INFO messages
//   3 => Print ERROR, WARNING, INFO and DETAIL messages
//
static int verbosity_level = 2;

// Mutex to avoid STDOUT "overlaping" due to different threads writing at the
// same time.
//
#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
pthread_mutex_t printf_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


////////////////////////////////////////////////////////////////////////////////
// Platform API: libc stuff
////////////////////////////////////////////////////////////////////////////////


void PLATFORM_FREE(void *ptr)
{
    return free(ptr);
}


void PLATFORM_PRINTF(const char *format, ...)
{
    va_list arglist;

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
    pthread_mutex_lock(&printf_mutex);
#endif

    va_start( arglist, format );
    vprintf( format, arglist );
    va_end( arglist );

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
    pthread_mutex_unlock(&printf_mutex);
#endif

    return;
}

void PLATFORM_PRINTF_DEBUG_SET_VERBOSITY_LEVEL(int level)
{
    verbosity_level = level;
}

void PLATFORM_PRINTF_DEBUG_ERROR(const char *format, ...)
{
    va_list arglist;
    uint32_t ts;

    if (verbosity_level < 0)
    {
        return;
    }

    ts = PLATFORM_GET_TIMESTAMP();

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
    pthread_mutex_lock(&printf_mutex);
#endif

    printf("[%03d.%03d] ", ts/1000, ts%1000);
    printf("ERROR   : ");
    va_start( arglist, format );
    vprintf( format, arglist );
    va_end( arglist );

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
    pthread_mutex_unlock(&printf_mutex);
#endif

    return;
}

void PLATFORM_PRINTF_DEBUG_WARNING(const char *format, ...)
{
    va_list arglist;
    uint32_t ts;

    if (verbosity_level < 1)
    {
        return;
    }

    ts = PLATFORM_GET_TIMESTAMP();

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
    pthread_mutex_lock(&printf_mutex);
#endif

    printf("[%03d.%03d] ", ts/1000, ts%1000);
    printf("WARNING : ");
    va_start( arglist, format );
    vprintf( format, arglist );
    va_end( arglist );

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
    pthread_mutex_unlock(&printf_mutex);
#endif

    return;
}

void PLATFORM_PRINTF_DEBUG_INFO(const char *format, ...)
{
    va_list arglist;
    uint32_t ts;

    if (verbosity_level < 2)
    {
        return;
    }

    ts = PLATFORM_GET_TIMESTAMP();

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
    pthread_mutex_lock(&printf_mutex);
#endif

    printf("[%03d.%03d] ", ts/1000, ts%1000);
    printf("INFO    : ");
    va_start( arglist, format );
    vprintf( format, arglist );
    va_end( arglist );

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
    pthread_mutex_unlock(&printf_mutex);
#endif

    return;
}

void PLATFORM_PRINTF_DEBUG_DETAIL(const char *format, ...)
{
    va_list arglist;
    uint32_t ts;

    if (verbosity_level < 3)
    {
        return;
    }

    ts = PLATFORM_GET_TIMESTAMP();

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
    pthread_mutex_lock(&printf_mutex);
#endif

    printf("[%03d.%03d] ", ts/1000, ts%1000);
    printf("DETAIL  : ");
    va_start( arglist, format );
    vprintf( format, arglist );
    va_end( arglist );

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
    pthread_mutex_unlock(&printf_mutex);
#endif

    return;
}

uint32_t PLATFORM_GET_TIMESTAMP(void)
{
    struct timeval tv_end;
    uint32_t diff;

    gettimeofday(&tv_end, NULL);

    diff = (tv_end.tv_usec - tv_begin.tv_usec) / 1000 + (tv_end.tv_sec - tv_begin.tv_sec) * 1000;

    return diff;
}


////////////////////////////////////////////////////////////////////////////////
// Platform API: Initialization functions
////////////////////////////////////////////////////////////////////////////////

uint8_t PLATFORM_INIT(void)
{

    // Call "_timeval_print()" for the first time so that the initialization
    // time is saved for future reference.
    //
    gettimeofday(&tv_begin, NULL);

    return 1;
}

int getIfIndex(const char *interface_name)
{
    int                 s;
    struct ifreq        ifr;

    s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (-1 == s)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] socket('%s') returned with errno=%d (%s) while opening a RAW socket\n",
                                    interface_name, errno, strerror(errno));
        return -1;
    }

    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);
    if (ioctl(s, SIOCGIFINDEX, &ifr) == -1)
    {
          PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] ioctl('%s',SIOCGIFINDEX) returned with errno=%d (%s) while opening a RAW socket\n",
                                      interface_name, errno, strerror(errno));
          close(s);
          return -1;
    }
    close(s);
    return ifr.ifr_ifindex;
}

int openPacketSocket(int ifindex, uint16_t eth_type)
{
    int                 s;
    struct sockaddr_ll  socket_address;

    s = socket(AF_PACKET, SOCK_RAW, htons(eth_type));
    if (-1 == s)
    {
        return -1;
    }

    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family   = AF_PACKET;
    socket_address.sll_ifindex  = ifindex;
    socket_address.sll_protocol = htons(eth_type);

    if (-1 == bind(s, (struct sockaddr*)&socket_address, sizeof(socket_address)))
    {
        close(s);
        return -1;
    }

    return s;
}
