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
#include <1905_cmdus.h>
#include <1905_tlvs.h>
#include <platform.h>
#include <platform_linux.h>
#include <utils.h>

#include <arpa/inet.h>        // socket, AF_INTER, htons(), ...
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utime.h>             // utime()

static struct alMacAddressTypeTLV expect_al_mac_tlv =
{
    .tlv_type          = TLV_TYPE_AL_MAC_ADDRESS_TYPE,
    .al_mac_address    = ADDR_AL,
};

static struct CMDU aletest_expect_cmdu_topology_discovery =
{
    .message_version = CMDU_MESSAGE_VERSION_1905_1_2013,
    .message_type    = CMDU_TYPE_TOPOLOGY_DISCOVERY,
    .relay_indicator = 0,
    .list_of_TLVs    =
        (INT8U* []){
            (INT8U *)&expect_al_mac_tlv,
            (INT8U *)(struct macAddressTypeTLV[]){
                {
                    .tlv_type          = TLV_TYPE_MAC_ADDRESS_TYPE,
                    .mac_address       = ADDR_MAC0,
                }
            },
            NULL,
        },
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

static struct CMDU aletest_expect_cmdu_topology_query =
{
    .message_version = CMDU_MESSAGE_VERSION_1905_1_2013,
    .message_type    = CMDU_TYPE_TOPOLOGY_QUERY,
    .relay_indicator = 0,
    .list_of_TLVs    =
        (INT8U* []){
            NULL,
        },
};

static uint8_t aletest_send_cmdu_topology_query[] = {
    0x02, 0xee, 0xff, 0x33, 0x44, 0x00,     /* AL MAC address */
    0x02, 0xaa, 0xbb, 0x33, 0x44, 0x00,     /* my AL MAC address */
    0x89, 0x3a,                             /* Protocol number */
    0x00, 0x00,                             /* Version, reserved */
    0x00, 0x02,                             /* Message type */
    0x42, 0x26,                             /* MID (incremented) */
    0x00,                                   /* Fragment ID */
    0x80,                                   /* last fragment, relay indicator */
    0x00, 0x00, 0x00, /* End of message TLV */
};

static struct _localInterfaceEntries aletest_local_interfaces[] = {
    {
        .mac_address = ADDR_MAC0,
        .media_type  = MEDIA_TYPE_IEEE_802_11G_2_4_GHZ,
        .media_specific_data_size = 10,
        .media_specific_data.ieee80211 = {
            .network_membership = { 0x00, 0x16, 0x03, 0x01, 0x85, 0x1f, },
            .role = IEEE80211_SPECIFIC_INFO_ROLE_AP,
            .ap_channel_band = 0x10,
            .ap_channel_center_frequency_index_1 = 0x20,
            .ap_channel_center_frequency_index_2 = 0x30,
        },
    },
    {
        .mac_address = ADDR_MAC1,
        .media_type  = MEDIA_TYPE_IEEE_802_3U_FAST_ETHERNET,
        .media_specific_data_size = 0,
    },
    {
        .mac_address = ADDR_MAC2,
        .media_type  = MEDIA_TYPE_IEEE_802_11G_2_4_GHZ,
        .media_specific_data_size = 10,
        .media_specific_data.ieee80211 = {
            .network_membership = { 0x00, 0x16, 0x03, 0x01, 0x85, 0x1f, },
            .role = IEEE80211_SPECIFIC_INFO_ROLE_AP,
            .ap_channel_band = 0x10,
            .ap_channel_center_frequency_index_1 = 0x20,
            .ap_channel_center_frequency_index_2 = 0x30,
        },
    },
    {
        .mac_address = ADDR_MAC3,
        .media_type  = MEDIA_TYPE_IEEE_802_3U_FAST_ETHERNET,
        .media_specific_data_size = 0,
    },
};

static struct CMDU aletest_expect_cmdu_topology_response =
{
    .message_version = CMDU_MESSAGE_VERSION_1905_1_2013,
    .message_type    = CMDU_TYPE_TOPOLOGY_RESPONSE,
    .relay_indicator = 0,
    .list_of_TLVs    =
        (INT8U* []){
            (INT8U *)(struct deviceInformationTypeTLV[]){
                {
                    .tlv_type            = TLV_TYPE_DEVICE_INFORMATION_TYPE,
                    .al_mac_address      = ADDR_AL,
                    .local_interfaces_nr = 4,
                    .local_interfaces    = aletest_local_interfaces,
                }
            },
            (INT8U *)(struct deviceBridgingCapabilityTLV[]){
                {
                    .tlv_type            = TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES,
                    .bridging_tuples_nr  = 0,
                },
            },
            (INT8U *)(struct neighborDeviceListTLV[]){
                {
                    .tlv_type            = TLV_TYPE_NEIGHBOR_DEVICE_LIST,
                    .local_mac_address   = ADDR_MAC0,
                    .neighbors_nr        = 1,
                    .neighbors           = (struct _neighborEntries[]) {
                        {
                            .mac_address = ADDR_AL_PEER0,
                            .bridge_flag = 0,
                        },
                    },
                }
            },
            (INT8U *)(struct powerOffInterfaceTLV[]){
                {
                    .tlv_type                 = TLV_TYPE_POWER_OFF_INTERFACE,
                    .power_off_interfaces_nr  = 0,
                },
            },
            (INT8U *)(struct l2NeighborDeviceTLV[]){
                {
                    .tlv_type            = TLV_TYPE_L2_NEIGHBOR_DEVICE,
                    .local_interfaces_nr  = 0,
                },
            },
            NULL,
        },
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

static struct CMDU aletest_expect_cmdu_topology_discovery2 =
{
    .message_version = CMDU_MESSAGE_VERSION_1905_1_2013,
    .message_type    = CMDU_TYPE_TOPOLOGY_DISCOVERY,
    .relay_indicator = 0,
    .list_of_TLVs    =
        (INT8U* []){
            (INT8U *)&expect_al_mac_tlv,
            (INT8U *)(struct macAddressTypeTLV[]){
                {
                    .tlv_type          = TLV_TYPE_MAC_ADDRESS_TYPE,
                    .mac_address       = ADDR_MAC1,
                }
            },
            NULL,
        },
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

static struct CMDU aletest_expect_cmdu_topology_notification =
{
    .message_version = CMDU_MESSAGE_VERSION_1905_1_2013,
    .message_type    = CMDU_TYPE_TOPOLOGY_NOTIFICATION,
    .relay_indicator = 1,
    .list_of_TLVs    =
        (INT8U* []){
            (INT8U *)&expect_al_mac_tlv,
            NULL,
        },
};


int main()
{
    int result = 0;
    int s0, s1;
    uint8_t buf[10];

    PLATFORM_INIT();
    PLATFORM_PRINTF_DEBUG_SET_VERBOSITY_LEVEL(3);

    s0 = openPacketSocket(getIfIndex("aletestpeer0"), ETHERTYPE_1905);
    if (-1 == s0) {
        PLATFORM_PRINTF_DEBUG_ERROR("Failed to open aletestpeer0");
        return 1;
    }

    /* The AL MUST send a topology discovery CMDU every 60 seconds (+1s jitter). */
    result += expect_cmdu_match(s0, 61000, "topology discovery", &aletest_expect_cmdu_topology_discovery,
                                (uint8_t *)ADDR_MAC0, (uint8_t *)ADDR_AL, (uint8_t *)MCAST_1905);

    /* Trigger a topology query from the AL by sending a topology discovery. The AL MAY send a query, but we expect the
     * AL under test to indeed send one immediately. */
    if (-1 == send(s0, aletest_send_cmdu_topology_discovery, sizeof(aletest_send_cmdu_topology_discovery), 0)) {
        PLATFORM_PRINTF_DEBUG_ERROR("Failed to send topology discovery: %d (%s)\n", errno, strerror(errno));
    } else {
#ifdef SPEED_UP_DISCOVERY
        /* The AL also sends another topology discovery. */
        result += expect_cmdu_match(s0, 3000, "topology discovery repeat", &aletest_expect_cmdu_topology_discovery,
                                    (uint8_t *)ADDR_MAC0, (uint8_t *)ADDR_AL, (uint8_t *)MCAST_1905);
#endif
        result += expect_cmdu_match(s0, 3000, "topology query", &aletest_expect_cmdu_topology_query,
                                    (uint8_t *)ADDR_MAC0, (uint8_t *)ADDR_AL, (uint8_t *)ADDR_AL_PEER0);
        /* No need to respond to the query. */
#ifdef SPEED_UP_DISCOVERY
        /* A second topology discovery (with a new MID) must not re-trigger discovery. */
        aletest_send_cmdu_topology_discovery[19]++;
        if (-1 == send(s0, aletest_send_cmdu_topology_discovery, sizeof(aletest_send_cmdu_topology_discovery), 0)) {
            PLATFORM_PRINTF_DEBUG_ERROR("Failed to send topology discovery: %d (%s)\n", errno, strerror(errno));
            result++;
        } else {
            /* Don't expect anything on that interface. */
            sleep(1);
            if (-1 != recv(s0, buf, sizeof(buf), MSG_DONTWAIT) || EAGAIN != errno) {
                PLATFORM_PRINTF_DEBUG_ERROR("Got a response on second topology discovery\n");
                result++;
            }
        }
#endif
    }

    /* Send a topology query. The AL MUST send a response. */
    if (-1 == send(s0, aletest_send_cmdu_topology_query, sizeof(aletest_send_cmdu_topology_query), 0))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Failed to send topology query: %d (%s)\n", errno, strerror(errno));
        result++;
    } else {
        /* AL must respond within 1 second */
        result += expect_cmdu_match(s0, 1000, "topology response", &aletest_expect_cmdu_topology_response,
                                    (uint8_t *)ADDR_MAC0, (uint8_t *)ADDR_AL, (uint8_t *)ADDR_AL_PEER0);
    }

    s1 = openPacketSocket(getIfIndex("aletestpeer1"), ETHERTYPE_1905);
    if (-1 == s1) {
        PLATFORM_PRINTF_DEBUG_ERROR("Failed to open aletestpeer1");
        close(s0);
        return 1;
    }

    /* Announce a second AL by sending a second toplogy discovery, which should trigger another query. */
    if (-1 == send(s1, aletest_send_cmdu_topology_discovery2, sizeof(aletest_send_cmdu_topology_discovery2), 0)) {
        PLATFORM_PRINTF_DEBUG_ERROR("Failed to send topology discovery: %d (%s)\n", errno, strerror(errno));
        result++;
    } else {
#ifdef SPEED_UP_DISCOVERY
        /* The AL also sends another topology discovery. */
        result += expect_cmdu_match(s1, 3000, "topology discovery aletest1", &aletest_expect_cmdu_topology_discovery2,
                                    (uint8_t *)ADDR_MAC1, (uint8_t *)ADDR_AL, (uint8_t *)MCAST_1905);
#endif
        result += expect_cmdu_match(s1, 3000, "topology query aletest1", &aletest_expect_cmdu_topology_query,
                                    (uint8_t *)ADDR_MAC1, (uint8_t *)ADDR_AL, (uint8_t *)ADDR_AL_PEER1);
        if (-1 == send(s1, aletest_send_cmdu_topology_response2, sizeof(aletest_send_cmdu_topology_response2), 0)) {
            PLATFORM_PRINTF_DEBUG_ERROR("Failed to send topology response: %d (%s)\n", errno, strerror(errno));
            result++;
        } else {
            /* This should trigger a topology notification on the other interface, because there is a new neighbor. */
            /* TODO Currently this doesn't trigger a topology change! So ignore this error for now. */
            (void) expect_cmdu_match(s0, 1000, "topology notification 0", &aletest_expect_cmdu_topology_notification,
                                        (uint8_t *)ADDR_MAC0, (uint8_t *)ADDR_AL, (uint8_t *)MCAST_1905);
        }
    }

    /* Force a topology notification with the virtual file. */
    if (-1 == utime("/tmp/topology_change", NULL)) {
        PLATFORM_PRINTF_DEBUG_ERROR("Failed to trigger topology change: %d (%s)\n", errno, strerror(errno));
        result++;
    } else {
        /* Notification should appear on both interfaces. */
        result += expect_cmdu_match(s0, 1000, "topology notification triggered 0", &aletest_expect_cmdu_topology_notification,
                                    (uint8_t *)ADDR_MAC0, (uint8_t *)ADDR_AL, (uint8_t *)MCAST_1905);
        result += expect_cmdu_match(s1, 1000, "topology notification triggered 1", &aletest_expect_cmdu_topology_notification,
                                    (uint8_t *)ADDR_MAC1, (uint8_t *)ADDR_AL, (uint8_t *)MCAST_1905);
    }

    /* The AL MUST send a topology discovery CMDU every 60 seconds (+1s jitter). */
    /* FIXME we should subtract the time spent since the last topology discovery message */
    result += expect_cmdu_match(s0, 61000, "topology discovery", &aletest_expect_cmdu_topology_discovery,
                                (uint8_t *)ADDR_MAC0, (uint8_t *)ADDR_AL, (uint8_t *)MCAST_1905);
    result += expect_cmdu_match(s1, 61000, "topology discovery aletest1", &aletest_expect_cmdu_topology_discovery2,
                                (uint8_t *)ADDR_MAC1, (uint8_t *)ADDR_AL, (uint8_t *)MCAST_1905);

    close(s0);
    close(s1);
    return result;
}
