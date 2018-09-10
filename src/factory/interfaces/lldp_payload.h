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

#ifndef _LLDP_PAYLOAD_H_
#define _LLDP_PAYLOAD_H_

#include <utils.h>

////////////////////////////////////////////////////////////////////////////////
// PAYLOAD associated structures
////////////////////////////////////////////////////////////////////////////////

struct PAYLOAD
{
    #define MAX_LLDP_TLVS 10
    struct tlv   *list_of_TLVs[MAX_LLDP_TLVS+1];
                                   // NULL-terminated list of pointers to TLV
                                   // structures.
                                   // The "end of lldppdu" TLV is not included
                                   // in this list.
};



////////////////////////////////////////////////////////////////////////////////
// Main API functions
////////////////////////////////////////////////////////////////////////////////

// This function receives a pointer to a streams containing ETH layer packet
// data (ie. offset +14 in a raw network packet, just after the source MAC
// address, destination MAC address and ETH type fields).
//
// The payload of this stream must contain a LLDP bridge discovery message, as
// detailed in "IEEE Std 1905.1-2013, Section 6.1"
//
// The following types of TLVs will be extracted from the contents of the
// packet, parsed, and returned inside a PAYLOAD structure:
//
//   - TLV_TYPE_CHASSIS_ID
//   - TLV_TYPE_PORT_ID
//   - TLV_TYPE_TIME_TO_LIVE
//
// The stream must always finish with a "TLV_TYPE_END_OF_LLDPPDU" TLV, but this
// one won't be contained in the returned PAYLOAD structure.
//
// If any type of error/inconsistency is found, a NULL pointer is returned
// instead, otherwise remember to free the received structure once you don't
// need it anymore (using the "free_lldp_PAYLOAD_structure()" function)
//
struct PAYLOAD *parse_lldp_PAYLOAD_from_packet(const uint8_t *packet_stream);


// This is the opposite of "parse_lldp_PAYLOAD_from_packet_from_packets()": it
// receives a pointer to a PAYLOAD structure and then returns a pointer to a
// packet stream.
//
// 'len' is also an output argument containing the length of the returned
// stream.
//
// The provided 'memory_structure' must have its fields properly filled or else
// this function will return an error (ie. a NULL pointer).
// What does this mean? Easy: only those TLVs detailed in "IEEE Std
// 1905.1-2013, Section 6.1" can appear inside the PAYLOAD structure.
//
//   NOTE: Notice how, in "parse_lldp_PAYLOAD_from_packets()", unexpected TLVs
//         are discarded, but here they produce an error. This is expected!
//
// One more thing: the provided 'memory_structure' is not freed by this
// function, thus, once this function returns, you will be responsible for
// freeing three things:
//
//   - The 'memory_structure' PAYLOAD structure (you were already responsible
//     for), that you can free with 'free_lldp_PAYLOAD_structure()'
//
//   - The returned stream, which can be freed with 'free_lldp_PAYLOAD_packet()'
//
uint8_t *forge_lldp_PAYLOAD_from_structure(struct PAYLOAD *memory_structure, uint16_t *len);



////////////////////////////////////////////////////////////////////////////////
// Utility API functions
////////////////////////////////////////////////////////////////////////////////


// This function receives a pointer to a PAYLOAD structure and then traverses it
// and all nested structures, calling "free()" on each one of them
//
//
void free_lldp_PAYLOAD_structure(struct PAYLOAD *memory_structure);


// This function receives a pointer to a  streams returned by
// "forge_lldp_PAYLOAD_from_structure()"
//
#define  free_lldp_PAYLOAD_packet  free


// This function returns '0' if the two given pointers represent PAYLOAD
// structures that contain the same data
//
uint8_t compare_lldp_PAYLOAD_structures(struct PAYLOAD *memory_structure_1, struct PAYLOAD *memory_structure_2);


// The next function is used to call function "callback()" on each element of
// the "memory_structure" structure
//
// "memory_structure" must point to a structure of one of the types returned by
// "parse_1905_PAYLOAD_from_packets()"
//
// It takes four arguments:
//   - The structure whose elements are going to be visited
//   - A callback function that will be executed on each element with the
//     following arguments:
//      * A pointer to the "write()" function that will be used to dump text.
//        This is always the "write_function()" pointer provided as third
//        argument to the "visit_lldp_PAYLOAD_structure()" function.
//      * The size of the element to print (1, 2, 4, n bytes)
//      * A prefix string.
//        This is always the "prefix" value provided as fourth argument to the
//        "visit_lldp_PAYLOAD_structure()" function/
//      * The name of the element (ex: "mac_address")
//      * A 'fmt' string which must be used to print the contents of the element
//      * A pointer to the element itself
//   - The "write()" function that will be used when the callback is executed
//   - A "prefix" string argument that will be used when the callback is
//     executed (it usually contains "context" information that the callback
//     function prints before anything else to make it easy to follow the
//     structure traversing order)
//
void visit_lldp_PAYLOAD_structure(struct PAYLOAD *memory_structure, visitor_callback callback, void (*write_function)(const char *fmt, ...), const char *prefix);

#endif
