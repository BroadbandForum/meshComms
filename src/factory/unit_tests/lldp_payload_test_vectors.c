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
#include "lldp_payload.h"

// This file contains test vectors than can be used to check the
// "parse_lldp_PAYLOAD_from_packet()" and "forge_lldp_PAYLOAD_from_structure()"
// functions
//
// Each test vector is made up of three variables:
//
//   - A PAYLOAD structure
//   - An array of bits representing the network packet
//   - An variable holding the length of the packet
//
// Note that some test vectors can be used to test both functions, while others
// can only be used to test "forge_lldp_PAYLOAD_from_structure()" or
// "parse_lldp_PAYLOAD_from_packet()":
//
//   - Test vectors marked with "PAYLOAD --> packet" can only be used to test the
//     "forge_lldp_PAYLOAD_from_structure()" function.
//
//   - Test vectors marked with "PAYLOD <-- packet" can only be used to test the
//     "parse_lldp_PAYLOAD_from_packets()" function.
//
//   - All the other test vectors are marked with "PAYLOAD <--> packet", meaning
//     they can be used to test  both functions.
//
// The reason this is happening is that, according to the standard, sometimes
// bits are ignored/changed/forced-to-a-value when forging a packet. Thus, not
// all test vectors are "inversible" (ie. forge(parse(stream)) != x)
//
// This is how you use these test vectors:
//
//   A) stream = forge_lldp_PAYLOAD_from_structure(payload_xxx, &stream_len);
//
//   B) tlv = parse_lldp_PAYLOAD_from_packets(stream_xxx);
//


////////////////////////////////////////////////////////////////////////////////
//// Test vector 001 (PAYLOAD <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct PAYLOAD lldp_payload_structure_001 =
{
    .list_of_TLVs    =
        {
            (uint8_t *)(struct chassisIdTLV[]){
                {
                    .tlv.type           = TLV_TYPE_CHASSIS_ID,
                    .chassis_id_subtype = CHASSIS_ID_TLV_SUBTYPE_MAC_ADDRESS,
                    .chassis_id         = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
                },
            },
            (uint8_t *)(struct portIdTLV[]){
                {
                    .tlv.type           = TLV_TYPE_PORT_ID,
                    .port_id_subtype    = PORT_ID_TLV_SUBTYPE_MAC_ADDRESS,
                    .port_id            = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66},
                },
            },
            (uint8_t *)(struct timeToLiveTypeTLV[]){
                {
                    .tlv.type           = TLV_TYPE_TIME_TO_LIVE,
                    .ttl                = TIME_TO_LIVE_TLV_1905_DEFAULT_VALUE,
                },
            },
            NULL,
        },
};

uint8_t lldp_payload_stream_001[] =
{
    0x02, 0x07,
    0x04,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x04, 0x07,
    0x03,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
    0x06, 0x02,
    0x00, 0xb4,
    0x00, 0x00,
};

uint16_t lldp_payload_stream_len_001 = 24;


