/*
 *  Broadband Forum IEEE 1905.1/1a stack
 *  
 *  Copyright (c) 2017, Broadband Forum
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  
 *  Subject to the terms and conditions of this license, each copyright
 *  holder and contributor hereby grants to those receiving rights under
 *  this license a perpetual, worldwide, non-exclusive, no-charge,
 *  royalty-free, irrevocable (except for failure to satisfy the
 *  conditions of this license) patent license to make, have made, use,
 *  offer to sell, sell, import, and otherwise transfer this software,
 *  where such license applies only to those patent claims, already
 *  acquired or hereafter acquired, licensable by such copyright holder or
 *  contributor that are necessarily infringed by:
 *  
 *  (a) their Contribution(s) (the licensed copyrights of copyright holders
 *      and non-copyrightable additions of contributors, in source or binary
 *      form) alone; or
 *  
 *  (b) combination of their Contribution(s) with the work of authorship to
 *      which such Contribution(s) was added by such copyright holder or
 *      contributor, if, at the time the Contribution is added, such addition
 *      causes such combination to be necessarily infringed. The patent
 *      license shall not apply to any other combinations which include the
 *      Contribution.
 *  
 *  Except as expressly stated above, no rights or licenses from any
 *  copyright holder or contributor is granted under this license, whether
 *  expressly, by implication, estoppel or otherwise.
 *  
 *  DISCLAIMER
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 *  DAMAGE.
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
            (INT8U *)(struct chassisIdTLV[]){
                {
                    .tlv_type           = TLV_TYPE_CHASSIS_ID,
                    .chassis_id_subtype = CHASSIS_ID_TLV_SUBTYPE_MAC_ADDRESS,
                    .chassis_id         = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
                },
            },
            (INT8U *)(struct portIdTLV[]){
                {
                    .tlv_type           = TLV_TYPE_PORT_ID,
                    .port_id_subtype    = PORT_ID_TLV_SUBTYPE_MAC_ADDRESS,
                    .port_id            = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66},
                },
            },
            (INT8U *)(struct timeToLiveTypeTLV[]){
                {
                    .tlv_type           = TLV_TYPE_TIME_TO_LIVE,
                    .ttl                = TIME_TO_LIVE_TLV_1905_DEFAULT_VALUE,
                },
            },
            NULL,
        },
};

INT8U lldp_payload_stream_001[] =
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

INT16U lldp_payload_stream_len_001 = 24;


