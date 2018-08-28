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
#include "1905_cmdus.h"
#include "1905_cmdu_test_vectors.h"

#include <utils.h> // ARRAY_SIZE

// This file contains test vectors than can be used to check the
// "parse_1905_CMDU_from_packets()" and "forge_1905_CMDU_from_structure()"
// functions
//
// Each test vector is made up of three variables:
//
//   - A CMDU structure
//   - An arrray of streams
//   - An array with the lengths of each stream
//
// Note that some test vectors can be used to test both functions, while others
// can only be used to test "forge_1905_CMDU_from_structure()" or
// "parse_1905_CMDU_from_packets()":
//
//   - Test vectors marked with "CMDU --> packet" can only be used to test the
//     "forge_1905_TLV_from_structure()" function.
//
//   - Test vectors marked with "CMDU <-- packet" can only be used to test the
//     "parse_1905_CMDU_from_packets()" function.
//
//   - All the other test vectors are marked with "CMDU <--> packet", meaning
//     they can be used to test  both functions.
//
// The reason this is happening is that, according to the standard, sometimes
// bits are ignored/changed/forced-to-a-value when forging a packet. Thus, not
// all test vectors are "inversible" (ie. forge(parse(stream)) != x)
//
// This is how you use these test vectors:
//
//   A) stream = forge_1905_CMDU_from_structure(tlv_xxx, &stream_len);
//
//   B) tlv = parse_1905_CMDU_from_packets(stream_xxx);
//


////////////////////////////////////////////////////////////////////////////////
//// Test vector 001 (CMDU <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct CMDU x1905_cmdu_structure_001 =
{
    .message_version = CMDU_MESSAGE_VERSION_1905_1_2013,
    .message_type    = CMDU_TYPE_LINK_METRIC_QUERY,
    .message_id      = 7,
    .relay_indicator = 0,
    .list_of_TLVs    =
        (uint8_t* []){
            (uint8_t *)(struct linkMetricQueryTLV[]){
                {
                    .tlv_type          = TLV_TYPE_LINK_METRIC_QUERY,
                    .destination       = LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS,
                    .specific_neighbor = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                    .link_metrics_type = LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS,
                },
            },
            NULL
        },
};

uint8_t *x1905_cmdu_streams_001[] =
{
    (uint8_t []){
        0x00,
        0x00,
        0x00, 0x05,
        0x00, 0x07,
        0x00,
        0x80,

        0x08,
        0x00, 0x08,
        0x00,
        0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x02,

        0x00,
        0x00, 0x00,
    },
    NULL
};

uint16_t x1905_cmdu_streams_len_001[] = {22, 0};


////////////////////////////////////////////////////////////////////////////////
//// Test vector 002 (CMDU <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct CMDU x1905_cmdu_structure_002 =
{
    .message_version = CMDU_MESSAGE_VERSION_1905_1_2013,
    .message_type   = CMDU_TYPE_LINK_METRIC_QUERY,
    .message_id      = 2348,
    .relay_indicator = 0,
    .list_of_TLVs    =
        (uint8_t* []){
            (uint8_t *)(struct linkMetricQueryTLV[]){
                {
                    .tlv_type          = TLV_TYPE_LINK_METRIC_QUERY,
                    .destination       = LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR,
                    .specific_neighbor = {0x01, 0x02, 0x02, 0x03, 0x04, 0x05},
                    .link_metrics_type = LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS,
                },
            },
            NULL
        },
};

uint8_t *x1905_cmdu_streams_002[] =
{
    (uint8_t []){
        0x00,
        0x00,
        0x00, 0x05,
        0x09, 0x2c,
        0x00,
        0x80,

        0x08,
        0x00, 0x08,
        0x01,
        0x01, 0x02, 0x02, 0x03, 0x04, 0x05,
        0x02,

        0x00,
        0x00, 0x00,
    },
    NULL
};

uint16_t x1905_cmdu_streams_len_002[] = {22, 0};


////////////////////////////////////////////////////////////////////////////////
//// Test vector 003 (CMDU --> packet)
////////////////////////////////////////////////////////////////////////////////

struct CMDU x1905_cmdu_structure_003 =
{
    .message_version = CMDU_MESSAGE_VERSION_1905_1_2013,
    .message_type    = CMDU_TYPE_LINK_METRIC_QUERY,
    .message_id      = 2348,
    .relay_indicator = 1,
    .list_of_TLVs    =
        (uint8_t* []){
            (uint8_t *)(struct linkMetricQueryTLV[]){
                {
                    .tlv_type          = TLV_TYPE_LINK_METRIC_QUERY,
                    .destination       = LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS,
                    .specific_neighbor = {0x01, 0x02, 0x02, 0x03, 0x04, 0x05},
                    .link_metrics_type = LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS,
                },
            },
            NULL
        },
};

uint8_t *x1905_cmdu_streams_003[] =
{
    (uint8_t []){
        0x00,
        0x00,
        0x00, 0x05,
        0x09, 0x2c,
        0x00,
        0x80, // Note that 'relay_indicator' is *not* set because for
              // this type of message ("CMDU_TYPE_LINK_METRIC_QUERY")
              // it must always be set to '0'

        0x08,
        0x00, 0x08,
        0x00,
        0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x02,

        0x00,
        0x00, 0x00,
    },
    NULL
};

uint16_t x1905_cmdu_streams_len_003[] = {22, 0};


////////////////////////////////////////////////////////////////////////////////
//// Test vector 004 (CMDU <-- packet)
////////////////////////////////////////////////////////////////////////////////

struct CMDU x1905_cmdu_structure_004 =
{
    .message_version = CMDU_MESSAGE_VERSION_1905_1_2013,
    .message_type    = CMDU_TYPE_LINK_METRIC_QUERY,
    .message_id      = 2348,
    .relay_indicator = 0,
    .list_of_TLVs    =
        (uint8_t* []){
            (uint8_t *)(struct linkMetricQueryTLV[]){
                {
                    .tlv_type          = TLV_TYPE_LINK_METRIC_QUERY,
                    .destination       = LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS,
                    .specific_neighbor = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                    .link_metrics_type = LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS,
                },
            },
            NULL
        },
};

uint8_t *x1905_cmdu_streams_004[] =
{
    (uint8_t []){
        0x00,
        0x00,
        0x00, 0x05,
        0x09, 0x2c,
        0x00,
        0x80,

        0x08,
        0x00, 0x08,
        0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x02,

        0x00,
        0x00, 0x00,
    },
    NULL
};

uint16_t x1905_cmdu_streams_len_004[] = {22, 0};


////////////////////////////////////////////////////////////////////////////////
//// Test vector 005 (CMDU <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct CMDU x1905_cmdu_structure_005 =
{
    .message_version = CMDU_MESSAGE_VERSION_1905_1_2013,
    .message_type    = CMDU_TYPE_TOPOLOGY_QUERY,
    .message_id      = 9,
    .relay_indicator = 0,
    .list_of_TLVs    =
        (uint8_t* []){
            NULL
        },
};

uint8_t *x1905_cmdu_streams_005[] =
{
    (uint8_t []){
        0x00,
        0x00,
        0x00, 0x02,
        0x00, 0x09,
        0x00,
        0x80,

        0x00,
        0x00, 0x00,
    },
    NULL
};

uint16_t x1905_cmdu_streams_len_005[] = {11, 0};


// TODO: More tests for all types of CMDUs


struct CMDU_header x1905_cmdu_header_001 =
{
    .dst_addr                = "\x00\xb2\xc3\xd4\xe5\xf6",
    .src_addr                = "\x02\x22\x33\x44\x55\x66",
    .message_type            = 0x0002,
    .mid                     = 0x4321,
    .fragment_id             = 0x00,
    .last_fragment_indicator = true,
};

uint8_t x1905_cmdu_packet_001[] =
{
    0x00, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6,
    0x02, 0x22, 0x33, 0x44, 0x55, 0x66,
    0x89, 0x3a,
    0x00,
    0x00,
    0x00, 0x02,
    0x43, 0x21,
    0x00,
    0x80,

    0x00,
    0x00, 0x00,
};
size_t  x1905_cmdu_packet_len_001 = ARRAY_SIZE(x1905_cmdu_packet_001);

struct CMDU_header x1905_cmdu_header_002 =
{
    .dst_addr                = "\x00\xb2\xc3\xd4\xe5\xf6",
    .src_addr                = "\x02\x22\x33\x44\x55\x66",
    .message_type            = 0x0002,
    .mid                     = 0x4321,
    .fragment_id             = 0x01,
    .last_fragment_indicator = false,
};

uint8_t x1905_cmdu_packet_002[] =
{
    0x00, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6,
    0x02, 0x22, 0x33, 0x44, 0x55, 0x66,
    0x89, 0x3a,
    0x00,
    0x00,
    0x00, 0x02,
    0x43, 0x21,
    0x01,
    0x40,

    0x00,
    0x00, 0x00,
};
size_t  x1905_cmdu_packet_len_002 = ARRAY_SIZE(x1905_cmdu_packet_002);

uint8_t x1905_cmdu_packet_003[] =
{
    0x00, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6,
    0x02, 0x22, 0x33, 0x44, 0x55, 0x66,
    0x89, 0x3b,
    0x00,
    0x00,
    0x00, 0x02,
    0x43, 0x21,
    0x02,
    0x80,

    0x00,
    0x00, 0x00,
};
size_t  x1905_cmdu_packet_len_003 = ARRAY_SIZE(x1905_cmdu_packet_003);

uint8_t x1905_cmdu_packet_004[] =
{
    0x00, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6,
    0x02, 0x22, 0x33, 0x44, 0x55, 0x66,
    0x89, 0x3a,
    0x00,
    0x00,
    0x00, 0x02,
    0x43, 0x21,
    0x01,
};
size_t  x1905_cmdu_packet_len_004 = ARRAY_SIZE(x1905_cmdu_packet_004);
