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

#include "1905_tlvs.h"
#include "bbf_tlvs.h"


// This file contains test vectors than can be used to check the
// "parse_1905_TLV_from_packet()" and "forge_1905_TLV_from_structure()"
// functions
//
// Each test vector is made up of three variables:
//
//   - A TLV structure
//   - An array of bits representing the network packet
//   - An variable holding the length of the packet
//
// Note that some test vectors can be used to test both functions, while others
// can only be used to test "forge_1905_TLV_from_structure()" or
// "parse_1905_TLV_from_packet()":
//
//   - Test vectors marked with "TLV --> packet" can only be used to test the
//     "forge_1905_TLV_from_structure()" function.
//
//   - Test vectors marked with "TLV <-- packet" can only be used to test the
//     "parse_1905_TLV_from_packet()" function.
//
//   - All the other test vectors are marked with "TLC <--> packet", meaning
//     they can be used to test  both functions.
//
// The reason this is happening is that, according to the standard, sometimes
// bits are ignored/changed/forced-to-a-value when forging a packet. Thus, not
// all test vectors are "inversible" (ie. forge(parse(stream)) != x)
//
// This is how you use these test vectors:
//
//   A) stream = forge_non_1905_TLV_from_structure(tlv_xxx, &stream_len);
//
//   B) tlv = parse_non_1905_TLV_from_packet(stream_xxx);
//


////////////////////////////////////////////////////////////////////////////////
//// Test vector 001 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct linkMetricQueryTLV bbf_tlv_structure_001 =
{
    .tlv.type          = BBF_TLV_TYPE_NON_1905_LINK_METRIC_QUERY,
    .destination       = LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR,
    .specific_neighbor = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05},
    .link_metrics_type = LINK_METRIC_QUERY_TLV_RX_LINK_METRICS_ONLY,
};

uint8_t bbf_tlv_stream_001[] =
{
    0x01,
    0x00, 0x08,
    0x01,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x01
};

uint16_t bbf_tlv_stream_len_001 = 11;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 002 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct linkMetricQueryTLV bbf_tlv_structure_002 =
{
    .tlv.type          = BBF_TLV_TYPE_NON_1905_LINK_METRIC_QUERY,
    .destination       = LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS,
    .specific_neighbor = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05},
    .link_metrics_type = LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS,
};

// CheckTrue (TLV --> packet)
uint8_t bbf_tlv_stream_002[] =
{
    0x01,
    0x00, 0x08,
    0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02
};

// CheckFalse (TLV <-- packet)
//   'specific neighbor mac address' should be zero for non-1905 metrics
uint8_t bbf_tlv_stream_002b[] =
{
    0x01,
    0x00, 0x08,
    0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x02
};

uint16_t bbf_tlv_stream_len_002 = 11;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 003 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct linkMetricQueryTLV bbf_tlv_structure_003 =
{
    .tlv.type          = BBF_TLV_TYPE_NON_1905_LINK_METRIC_QUERY,
    .destination       = LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS,
    .specific_neighbor = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .link_metrics_type = LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS,
};

uint8_t bbf_tlv_stream_003[] =
{
    0x01,
    0x00, 0x08,
    0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02
};

uint16_t bbf_tlv_stream_len_003 = 11;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 004 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct transmitterLinkMetricTLV bbf_tlv_structure_004 =
{
    .tlv.type                    = BBF_TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC,
    .local_al_address            = {0x01, 0x02, 0x03, 0x01, 0x02, 0x03},
    .neighbor_al_address         = {0x0a, 0x0b, 0x0c, 0x0a, 0x0b, 0x0c},
    .transmitter_link_metrics_nr = 1,
    .transmitter_link_metrics    =
        (struct _transmitterLinkMetricEntries[]){
            {
                .local_interface_address    = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26},
                .neighbor_interface_address = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36},
                .intf_type                  = MEDIA_TYPE_IEEE_802_11G_2_4_GHZ,
                .bridge_flag                = 0,
                .packet_errors              = 134,
                .transmitted_packets        = 1543209,
                .mac_throughput_capacity    = 400,
                .link_availability          = 50,
                .phy_rate                   = 520,
            },
        },
};

// CheckTrue (TLV --> packet)
uint8_t bbf_tlv_stream_004[] =
{
    0x02,
    0x00, 0x29,
    0x01, 0x02, 0x03, 0x01, 0x02, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x01, 0x01,
    0x00,
    0x00, 0x00, 0x00, 0x86,
    0x00, 0x17, 0x8c, 0x29,
    0x01, 0x90,
    0x00, 0x32,
    0x02, 0x08
};

// CheckFalse (TLV <-- packet)
//   'specific neighbor mac address' should be zero for non-1905 metrics
uint8_t bbf_tlv_stream_004b[] =
{
    0x02,
    0x00, 0x29,
    0x01, 0x02, 0x03, 0x01, 0x02, 0x03,
    0x0a, 0x0b, 0x0c, 0x0a, 0x0b, 0x0c,  // specific neighbor mac address
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x01, 0x01,
    0x00,
    0x00, 0x00, 0x00, 0x86,
    0x00, 0x17, 0x8c, 0x29,
    0x01, 0x90,
    0x00, 0x32,
    0x02, 0x08
};

uint16_t bbf_tlv_stream_len_004 = 44;


////////////////////////////////////////////////////////////////////////////////
////
//// Test vector 005 (TLV <--> packet)
////
////////////////////////////////////////////////////////////////////////////////

struct transmitterLinkMetricTLV bbf_tlv_structure_005 =
{
    .tlv.type                    = BBF_TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC,
    .local_al_address            = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
    .neighbor_al_address         = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .transmitter_link_metrics_nr = 2,
    .transmitter_link_metrics    =
        (struct _transmitterLinkMetricEntries[]){
            {
                .local_interface_address    = {0x03, 0x03, 0x03, 0x03, 0x03, 0x03},
                .neighbor_interface_address = {0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
                .intf_type                  = MEDIA_TYPE_MOCA_V1_1,
                .bridge_flag                = 1,
                .packet_errors              = 0,
                .transmitted_packets        = 1000,
                .mac_throughput_capacity    = 900,
                .link_availability          = 80,
                .phy_rate                   = 1000,
            },
            {
                .local_interface_address    = {0x05, 0x05, 0x05, 0x05, 0x05, 0x05},
                .neighbor_interface_address = {0x06, 0x06, 0x06, 0x06, 0x06, 0x06},
                .intf_type                  = MEDIA_TYPE_IEEE_802_11AC_5_GHZ,
                .bridge_flag                = 0,
                .packet_errors              = 7,
                .transmitted_packets        = 7,
                .mac_throughput_capacity    = 900,
                .link_availability          = 80,
                .phy_rate                   = 1000,
            },
        },
};

uint8_t bbf_tlv_stream_005[] =
{
    0x02,
    0x00, 0x46,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x03, 0x00,
    0x01,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x03, 0xe8,
    0x03, 0x84,
    0x00, 0x50,
    0x03, 0xe8,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x01, 0x05,
    0x00,
    0x00, 0x00, 0x00, 0x07,
    0x00, 0x00, 0x00, 0x07,
    0x03, 0x84,
    0x00, 0x50,
    0x03, 0xe8,
};

uint16_t bbf_tlv_stream_len_005 = 73;


////////////////////////////////////////////////////////////////////////////////
////
//// Test vector 006 (TLV <--> packet)
////
////////////////////////////////////////////////////////////////////////////////

struct receiverLinkMetricTLV bbf_tlv_structure_006 =
{
    .tlv.type                    = BBF_TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC,
    .local_al_address            = {0x01, 0x02, 0xff, 0x01, 0x02, 0x03},
    .neighbor_al_address         = {0x0a, 0x0b, 0x0c, 0x0a, 0x77, 0x0c},
    .receiver_link_metrics_nr    = 1,
    .receiver_link_metrics       =
        (struct _receiverLinkMetricEntries[]){
            {
                .local_interface_address    = {0x21, 0x22, 0x00, 0x24, 0x00, 0x26},
                .neighbor_interface_address = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36},
                .intf_type                  = MEDIA_TYPE_IEEE_802_11AF_GHZ,
                .packet_errors              = 27263110,
                .packets_received           = 27263111,
                .rssi                       = 2,
            },
        },
};

// CheckTrue (TLV --> packet)
uint8_t bbf_tlv_stream_006[] =
{
    0x03,
    0x00, 0x23,
    0x01, 0x02, 0xff, 0x01, 0x02, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x21, 0x22, 0x00, 0x24, 0x00, 0x26,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x01, 0x07,
    0x01, 0xa0, 0x00, 0x86,
    0x01, 0xa0, 0x00, 0x87,
    0x02,
};

// CheckFalse (TLV <-- packet)
//   'specific neighbor mac address' should be zero for non-1905 metrics
uint8_t bbf_tlv_stream_006b[] =
{
    0x03,
    0x00, 0x23,
    0x01, 0x02, 0xff, 0x01, 0x02, 0x03,
    0x0a, 0x0b, 0x0c, 0x0a, 0x77, 0x0c,  // specific neighbor mac address
    0x21, 0x22, 0x00, 0x24, 0x00, 0x26,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x01, 0x07,
    0x01, 0xa0, 0x00, 0x86,
    0x01, 0xa0, 0x00, 0x87,
    0x02,
};

uint16_t bbf_tlv_stream_len_006 = 38;


////////////////////////////////////////////////////////////////////////////////
////
//// Test vector 007 (TLV <--> packet)
////
////////////////////////////////////////////////////packet////////////////////////////

struct receiverLinkMetricTLV bbf_tlv_structure_007 =
{
    .tlv.type                    = BBF_TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC,
    .local_al_address            = {0x01, 0x02, 0xff, 0x01, 0x02, 0x03},
    .neighbor_al_address         = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .receiver_link_metrics_nr    = 2,
    .receiver_link_metrics       =
        (struct _receiverLinkMetricEntries[]){
            {
                .local_interface_address    = {0x21, 0x22, 0x00, 0x24, 0x00, 0x26},
                .neighbor_interface_address = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36},
                .intf_type                  = MEDIA_TYPE_IEEE_802_11AF_GHZ,
                .packet_errors              = 27263110,
                .packets_received           = 27263111,
                .rssi                       = 2,
            },
            {
                .local_interface_address    = {0xff, 0x22, 0x00, 0x24, 0x00, 0x26},
                .neighbor_interface_address = {0xff, 0x32, 0x33, 0x34, 0x35, 0x36},
                .intf_type                  = MEDIA_TYPE_IEEE_802_11AF_GHZ,
                .packet_errors              = 27263110,
                .packets_received           = 27263111,
                .rssi                       = 2,
            },
        },
};

uint8_t bbf_tlv_stream_007[] =
{
    0x03,
    0x00, 0x3a,
    0x01, 0x02, 0xff, 0x01, 0x02, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x21, 0x22, 0x00, 0x24, 0x00, 0x26,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x01, 0x07,
    0x01, 0xa0, 0x00, 0x86,
    0x01, 0xa0, 0x00, 0x87,
    0x02,
    0xff, 0x22, 0x00, 0x24, 0x00, 0x26,
    0xff, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x01, 0x07,
    0x01, 0xa0, 0x00, 0x86,
    0x01, 0xa0, 0x00, 0x87,
    0x02,
};

uint16_t bbf_tlv_stream_len_007 = 61;

