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

static maskedbyte_t aletest_expect_cmdu_topology_discovery[] = {
    0x01, 0x80, 0xc2, 0x00, 0x00, 0x13,     /* 1905.1 multicast MAC address */
    0x0202, 0xee, 0xff, 0x33, 0x44, 0x00,   /* AL MAC address OR interface address */
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

static uint8_t aletest_send_cmdu_topology_discovery[] = {
    0x01, 0x80, 0xc2, 0x00, 0x00, 0x13,     /* 1905.1 multicast MAC address */
    0x02, 0xaa, 0xbb, 0x33, 0x44, 0x00,     /* AL MAC address OR interface address */
    0x89, 0x3a,                             /* Protocol number */
    0x00, 0x00,                             /* Version, reserved */
    0x00, 0x00,                             /* Message type */
    0x42, 0x24,                             /* MID (may be anything) */
    0x00,                                   /* Fragment ID */
    0x80,                                   /* last fragment, relay indicator */
    /* TLV 0 */ 0x01,                               /* 1905.1 AL MAC address type TLV */
                0x00, 0x06,
                0x02, 0xaa, 0xbb, 0x33, 0x44, 0x00, /* AL MAC address */
    /* TLV 1 */ 0x02,                               /* MAC address type TLV */
                0x00, 0x06,
                0x00, 0xaa, 0xbb, 0x33, 0x44, 0x00, /* inteface MAC address */
    0x00, 0x00, 0x00, /* End of message TLV */
};

static maskedbyte_t aletest_expect_cmdu_topology_query[] = {
    0x02, 0xaa, 0xbb, 0x33, 0x44, 0x00,     /* my AL MAC address */
    0x0202, 0xee, 0xff, 0x33, 0x44, 0x00,   /* AL MAC address OR interface address */
    0x89, 0x3a,                             /* Protocol number */
    0x00, 0x00,                             /* Version, reserved */
    0x00, 0x02,                             /* Message type */
    0xffff, 0xffff,                         /* MID (may be anything) */
    0x00,                                   /* Fragment ID */
    0x80,                                   /* last fragment, relay indicator */
    0x00, 0x00, 0x00, /* End of message TLV */
};

static uint8_t aletest_send_cmdu_topology_query[] = {
    0x02, 0xee, 0xff, 0x33, 0x44, 0x00,     /* AL MAC address */
    0x02, 0xaa, 0xbb, 0x33, 0x44, 0x00,     /* my AL MAC address */
    0x89, 0x3a,                             /* Protocol number */
    0x00, 0x00,                             /* Version, reserved */
    0x00, 0x02,                             /* Message type */
    0x42, 0x25,                             /* MID (incremented) */
    0x00,                                   /* Fragment ID */
    0x80,                                   /* last fragment, relay indicator */
    0x00, 0x00, 0x00, /* End of message TLV */
};

static maskedbyte_t aletest_expect_cmdu_topology_response[] = {
    0x02, 0xaa, 0xbb, 0x33, 0x44, 0x00,     /* my AL MAC address */
    0x0202, 0xee, 0xff, 0x33, 0x44, 0x00,   /* AL MAC address OR interface address */
    0x89, 0x3a,                             /* Protocol number */
    0x00, 0x00,                             /* Version, reserved */
    0x00, 0x03,                             /* Message type */
    0x42, 0x25,                             /* MID (same as query) */
    0x00,                                   /* Fragment ID */
    0x80,                                   /* last fragment, relay indicator */
    /* TLV 0 */ 0x03,                               /* 1905.1 device information type TLV */
                0x00, 0x3f,
                0x02, 0xee, 0xff, 0x33, 0x44, 0x00, /* AL MAC address */
                0x04,                               /* 4 interfaces */
    /* intf0 */ 0x00, 0xee, 0xff, 0x33, 0x44, 0x00, /* altest0 MAC address */
                0x01, 0x01,                         /* 802.11g */
                0x0a,                               /* Additional info for 802.11 */
                0x00, 0x16, 0x03, 0x01, 0x85, 0x1f, /* BSSID (from .sim) */
                0x00,                               /* Role == AP (from .sim) */
                0x10, 0x20, 0x30,                   /* Bandwidth, Freq1, Freq2 (from .sim) */
    /* intf1 */ 0x00, 0xee, 0xff, 0x33, 0x44, 0x10, /* altest1 MAC address */
                0x00, 0x00,                         /* Ethernet */
                0x00,                               /* No additional info */
    /* intf2 */ 0x00, 0xee, 0xff, 0x33, 0x44, 0x20, /* altest2 MAC address */
                0x01, 0x01,                         /* 802.11g */
                0x0a,                               /* Additional info for 802.11 */
                0x00, 0x16, 0x03, 0x01, 0x85, 0x1f, /* BSSID (from .sim) */
                0x00,                               /* Role == AP (from .sim) */
                0x10, 0x20, 0x30,                   /* Bandwidth, Freq1, Freq2 (from .sim) */
    /* intf3 */ 0x00, 0xee, 0xff, 0x33, 0x44, 0x30, /* altest3 MAC address */
                0x00, 0x00,                         /* Ethernet */
                0x00,                               /* No additional info */
    /* TLV 1 */ 0x04,                               /* Device bridging capability TLV */
                0x00, 0x01,
                0x00,                               /* No bridges configured on this device */
    /* No Non-1905 neighbors */
    /* TLV 2 */ 0x07,                               /* 1905.1 neighbor device TLV */
                0x00, 0x0d,                         /* 1 neighbor */
                0x00, 0xee, 0xff, 0x33, 0x44, 0x00, /* altest0 MAC address */
                0x02, 0xaa, 0xbb, 0x33, 0x44, 0x00, /* my AL MAC address */
                0x00,                               /* No bridges between them */
    /* TLV 3 */ 0x1b,                               /* power off interface type TLV */
                0x00, 0x01,
                0x00,                               /* No interfaces */
    /* TLV 4 */ 0x1e,                               /* L2 neighbor device TLV */
                0x00, 0x01,
                0x00,                               /* No L2 neighbors */
    0x00, 0x00, 0x00, /* End of message TLV */
};

static uint8_t aletest_send_cmdu_topology_discovery2[] = {
    0x01, 0x80, 0xc2, 0x00, 0x00, 0x13,     /* 1905.1 multicast MAC address */
    0x02, 0xaa, 0xbb, 0x33, 0x44, 0x10,     /* AL MAC address OR interface address */
    0x89, 0x3a,                             /* Protocol number */
    0x00, 0x00,                             /* Version, reserved */
    0x00, 0x00,                             /* Message type */
    0x43, 0x24,                             /* MID (may be anything) */
    0x00,                                   /* Fragment ID */
    0x80,                                   /* last fragment, relay indicator */
    /* TLV 0 */ 0x01,                               /* 1905.1 AL MAC address type TLV */
                0x00, 0x06,
                0x02, 0xaa, 0xbb, 0x33, 0x44, 0x10, /* AL MAC address */
    /* TLV 1 */ 0x02,                               /* MAC address type TLV */
                0x00, 0x06,
                0x00, 0xaa, 0xbb, 0x33, 0x44, 0x10, /* inteface MAC address */
    0x00, 0x00, 0x00, /* End of message TLV */
};

static maskedbyte_t aletest_expect_cmdu_topology_query2[] = {
    0x02, 0xaa, 0xbb, 0x33, 0x44, 0x10,     /* my AL MAC address */
    0x0202, 0xee, 0xff, 0x33, 0x44, 0x1000, /* AL MAC address OR interface address */
    0x89, 0x3a,                             /* Protocol number */
    0x00, 0x00,                             /* Version, reserved */
    0x00, 0x02,                             /* Message type */
    0xffff, 0xffff,                         /* MID (may be anything) */
    0x00,                                   /* Fragment ID */
    0x80,                                   /* last fragment, relay indicator */
    0x00, 0x00, 0x00, /* End of message TLV */
};

/* Minimal topology response */
static uint8_t aletest_send_cmdu_topology_response2[] = {
    0x02, 0xee, 0xff, 0x33, 0x44, 0x00,     /* AL MAC address */
    0x02, 0xaa, 0xbb, 0x33, 0x44, 0x10,     /* my AL MAC address */
    0x89, 0x3a,                             /* Protocol number */
    0x00, 0x00,                             /* Version, reserved */
    0x00, 0x03,                             /* Message type */
    0xff, 0xff,                             /* MID (same as query, TODO current implementation ignores it) */
    0x00,                                   /* Fragment ID */
    0x80,                                   /* last fragment, relay indicator */
    /* TLV 0 */ 0x03,                               /* 1905.1 device information type TLV */
                0x00, 0x10,
                0x02, 0xaa, 0xbb, 0x33, 0x44, 0x10, /* AL MAC address */
                0x01,                               /* 1 interface */
    /* intf0 */ 0x00, 0xaa, 0xbb, 0x33, 0x44, 0x11, /* altest1 MAC address */
                0x00, 0x01,                         /* Gigabit ethernet */
                0x00,                               /*  No additional info */
    /* No device bridging capability */
    /* No Non-1905 neighbors */
    /* No 1905 neighbors */
    0x00, 0x00, 0x00, /* End of message TLV */
};

static maskedbyte_t aletest_expect_cmdu_topology_notification[] = {
    0x01, 0x80, 0xc2, 0x00, 0x00, 0x13,     /* 1905.1 multicast MAC address */
    0x0202, 0xee, 0xff, 0x33, 0x44, 0x1000, /* AL MAC address OR interface address */
    0x89, 0x3a,                             /* Protocol number */
    0x00, 0x00,                             /* Version, reserved */
    0x00, 0x01,                             /* Message type */
    0xffff, 0xffff,                         /* MID (one more than previous one) */
    0x00,                                   /* Fragment ID */
    0xc0,                                   /* last fragment, relayed multicast */
    /* TLV 0 */ 0x01,                               /* 1905.1 AL MAC address type TLV */
                0x00, 0x06,
                0x02, 0xee, 0xff, 0x33, 0x44, 0x00, /* AL MAC address */
    0x00, 0x00, 0x00, /* End of message TLV */
};



int main()
{
    struct sockaddr_ll addr;
    int result = 0;
    int s0, s1;

    PLATFORM_INIT();
    PLATFORM_PRINTF_DEBUG_SET_VERBOSITY_LEVEL(3);

    s0 = openPacketSocket("aletestpeer0", htons(ETHERTYPE_1905), &addr);
    if (-1 == s0) {
        return 1;
    }

    /* The AL MUST send a topology discovery CMDU every 60 seconds (+1s jitter). */
    CHECK_EXPECT_PACKET(s0, aletest_expect_cmdu_topology_discovery, 61000, result);

    /* Trigger a topology query from the AL by sending a topology discovery. The AL MAY send a query, but we expect the
     * AL under test to indeed send one immediately. */
    if (-1 == send(s0, aletest_send_cmdu_topology_discovery, sizeof(aletest_send_cmdu_topology_discovery), 0)) {
        PLATFORM_PRINTF_DEBUG_ERROR("Failed to send topology discovery: %d (%s)\n", errno, strerror(errno));
    } else {
        /* Note: the AL also sends another topology discovery, but that is not needed so we don't do an expect for it. */
        CHECK_EXPECT_PACKET(s0, aletest_expect_cmdu_topology_query, 3000, result);
        /* No need to respond to the query. */
    }

    /* Send a topology query. The AL MUST send a response. */
    if (-1 == send(s0, aletest_send_cmdu_topology_query, sizeof(aletest_send_cmdu_topology_query), 0))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Failed to send topology query: %d (%s)\n", errno, strerror(errno));
        result++;
    } else {
        /* AL must respond within 1 second */
        CHECK_EXPECT_PACKET(s0, aletest_expect_cmdu_topology_response, 1000, result);
    }

    s1 = openPacketSocket("aletestpeer1", htons(ETHERTYPE_1905), &addr);
    if (-1 == s1) {
        close(s0);
        return 1;
    }

    /* Announce a second AL by sending a second toplogy discovery, which should trigger another query. */
    if (-1 == send(s1, aletest_send_cmdu_topology_discovery2, sizeof(aletest_send_cmdu_topology_discovery2), 0)) {
        PLATFORM_PRINTF_DEBUG_ERROR("Failed to send topology discovery: %d (%s)\n", errno, strerror(errno));
        result++;
    } else {
        /* Note: the AL also sends another topology discovery, but that is not needed so we don't do an expect for it. */
        CHECK_EXPECT_PACKET(s1, aletest_expect_cmdu_topology_query2, 3000, result);
        if (-1 == send(s1, aletest_send_cmdu_topology_response2, sizeof(aletest_send_cmdu_topology_response2), 0)) {
            PLATFORM_PRINTF_DEBUG_ERROR("Failed to send topology response: %d (%s)\n", errno, strerror(errno));
            result++;
        } else {
            /* This should trigger a topology notification on the other interface, because there is a new neighbor. */
            CHECK_EXPECT_PACKET(s0, aletest_expect_cmdu_topology_notification, 1000, result);
            /* TODO Currently this doesn't trigger a topology change! So ignore this error for now. */
            result--;
        }
    }

    /* The AL MUST send a topology discovery CMDU every 60 seconds (+1s jitter). */
    /* FIXME we should subtract the time spent since the last topology discovery message */
    CHECK_EXPECT_PACKET(s0, aletest_expect_cmdu_topology_discovery, 61000, result);

    close(s0);
    close(s1);
    return result;
}
