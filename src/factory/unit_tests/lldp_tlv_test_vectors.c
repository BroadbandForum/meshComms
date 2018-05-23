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

#include "lldp_tlvs.h"


// This file contains test vectors than can be used to check the
// "parse_lldp_TLV_from_packet()" and "forge_lldp_TLV_from_structure()"
// functions
//
// Each test vector is made up of three variables:
//
//   - A TLV structure
//   - An array of bits representing the network packet
//   - An variable holding the length of the packet
//
// Note that some test vectors can be used to test both functions, while others
// can only be used to test "forge_lldp_TLV_from_structure()" or
// "parse_lldp_TLV_from_packet()":
//
//   - Test vectors marked with "TLV --> packet" can only be used to test the
//     "forge_lldp_TLV_from_structure()" function.
//
//   - Test vectors marked with "TLV <-- packet" can only be used to test the
//     "parse_lldp_TLV_from_packet()" function.
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
//   A) stream = forge_lldp_TLV_from_structure(tlv_xxx, &stream_len);
//
//   B) tlv = parse_lldp_TLV_from_packet(stream_xxx);
//


////////////////////////////////////////////////////////////////////////////////
//// Test vector 001 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct endOfLldppduTLV lldp_tlv_structure_001 =
{
    .tlv_type          = TLV_TYPE_END_OF_LLDPPDU,
};

INT8U lldp_tlv_stream_001[] =
{
    0x00, 0x00,
};

INT16U lldp_tlv_stream_len_001 = 2;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 002 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct chassisIdTLV lldp_tlv_structure_002 =
{
    .tlv_type            = TLV_TYPE_CHASSIS_ID,
    .chassis_id_subtype  = CHASSIS_ID_TLV_SUBTYPE_MAC_ADDRESS,
    .chassis_id          = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
};

INT8U lldp_tlv_stream_002[] =
{
    0x02, 0x07,
    0x04,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
};

INT16U lldp_tlv_stream_len_002 = 9;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 003 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct portIdTLV lldp_tlv_structure_003 =
{
    .tlv_type            = TLV_TYPE_PORT_ID,
    .port_id_subtype     = PORT_ID_TLV_SUBTYPE_MAC_ADDRESS,
    .port_id             = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
};

INT8U lldp_tlv_stream_003[] =
{
    0x04, 0x07,
    0x03,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
};

INT16U lldp_tlv_stream_len_003 = 9;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 004 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct timeToLiveTypeTLV lldp_tlv_structure_004 =
{
    .tlv_type            = TLV_TYPE_TIME_TO_LIVE,
    .ttl                 = TIME_TO_LIVE_TLV_1905_DEFAULT_VALUE,
};

INT8U lldp_tlv_stream_004[] =
{
    0x06, 0x02,
    0x00, 0xb4,
};

INT16U lldp_tlv_stream_len_004 = 4;
