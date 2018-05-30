/*
 *  prplMesh Wi-Fi Multi-AP
 *
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

#include "aletest.h"

#include <1905_l2.h>
#include <platform.h>
#include <utils.h>

#include <arpa/inet.h>        // socket, AF_INTER, htons(), ...
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

static maskedbyte_t aletest_cmdu_topology_discovery[] = {
    0x01, 0x80, 0xc2, 0x00, 0x00, 0x13,     /* 1905.1 multicast MAC address */
    0x0202, 0xee, 0xff, 0x33, 0x44, 0x00,   /* AL MAC address OR interface address*/
    0x89, 0x3a,                             /* Protocol number */
    0x00, 0x00,                             /* Version, reserved */
    0x00, 0x00,                             /* Message type */
    0xffff, 0xffff,                         /* MID (may be anything) */
    0x00,                                   /* Fragment ID */
    0x80,                                   /* last fragment, relay indicator */
    /* TLV 0 */ 0x01,                               /* 1905.1 AL MAC address type TLV */
                0x00, 0x06,
                0x02, 0xee, 0xff, 0x33, 0x44, 0x00, /* AL MAC address */
    /* TLV 1 */ 0x02,                               /* MAC address type TLV */
                0x00, 0x06,
                0x00, 0xee, 0xff, 0x33, 0x44, 0x00, /* inteface MAC address (from .sim file) */
    0x00, 0x00, 0x00, /* End of message TLV */
};


int main()
{
    struct sockaddr_ll addr;
    const char *interface_name = "aletestpeer0";
    int result = 0;

    PLATFORM_INIT();
    PLATFORM_PRINTF_DEBUG_SET_VERBOSITY_LEVEL(3);

    int s = openPacketSocket(interface_name, htons(ETHERTYPE_1905), &addr);
    if (-1 == s)
    {
        return 1;
    }

    /* The AL MUST send a topology discovery CMDU every 60 seconds (+1s jitter). */
    CHECK_EXPECT_PACKET(s, aletest_expect_cmdu_topology_discovery, 61000, result);

    close(s);
    return result;
}
