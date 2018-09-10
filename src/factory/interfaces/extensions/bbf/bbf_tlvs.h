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

#ifndef _BBF_TLVS_H_
#define _BBF_TLVS_H_

#include "platform.h"
#include <utils.h>
#include <tlv.h>

////////////////////////////////////////////////////////////////////////////////
// BBF TLV types
////////////////////////////////////////////////////////////////////////////////
#define BBF_TLV_TYPE_NON_1905_LINK_METRIC_QUERY                   (1)
#define BBF_TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC             (2)
#define BBF_TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC                (3)
#define BBF_TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE             (4)

#define BBF_TLV_TYPE_NON_1905_LAST                                (5)
                                                     // NOTE: If new types are
                                                     // introduced in future
                                                     // revisions of the
                                                     // standard, update this
                                                     // value so that it always
                                                     // points to the last one.


static const uint8_t BBF_OUI[3] = {0x00, 0x25, 0x6d};

////////////////////////////////////////////////////////////////////////////////
// Main API functions
////////////////////////////////////////////////////////////////////////////////

// This function receives a pointer to a stream of bytes representing a BBF TLV.
//
// It then returns a pointer to a structure whose fields have already been
// filled with the appropriate values extracted from the parsed stream.
//
// The actual type of the returned pointer structure depends on the value of
// the first byte pointed by "packet_stream" (ie. the "Type" field of the TLV):
//
//   TLV_TYPE_NON_1905_LINK_METRIC_QUERY        -->  struct linkMetricQueryTLV *
//   TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC  -->  struct transmitterLinkMetricTLV *
//   TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC     -->  struct receiverLinkMetricTLV *
//   TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE  -->  struct linkMetricResultCodeTLV *
//
//
// If an error was encountered while parsing the stream, a NULL pointer is
// returned instead.
// Otherwise, the returned structure is dynamically allocated, and once it is
// no longer needed, the user must call the "free_bbf_TLV_structure()"
//
struct tlv *parse_bbf_TLV_from_packet(uint8_t *packet_stream);


 // This is the opposite of "parse_bbf_TLV_from_packet()": it receives a
 // pointer to a TLV structure and then returns a pointer to a buffer which:
 //   - is a packet representation of the TLV
 //   - has a length equal to the value returned in the "len" output argument
 //
 // "memory_structure" must point to a structure of one of the types returned by
 // "parse_bbf_TLV_from_packet()"
 //
 // If there is a problem this function returns NULL, otherwise the returned
 // buffer must be later freed by the caller (it is a regular, non-nested
 // buffer, so you just need to call "free()").
 //
 // Note that the input structure is *not* freed. You still need to later call
 // "free_bbf_TLV_structure()"
 //
uint8_t *forge_bbf_TLV_from_structure(struct tlv *memory_structure, uint16_t *len);



////////////////////////////////////////////////////////////////////////////////
// Utility API functions
////////////////////////////////////////////////////////////////////////////////

// This function receives a pointer to a TLV structure and then traverses it
// and all nested structures, calling "free()" on each one of them
//
// "memory_structure" must point to a structure of one of the types returned by
// "parse_bbf_TLV_from_packet()"
//
void free_bbf_TLV_structure(struct tlv *memory_structure);


// 'forge_bbf_TLV_from_structure()' returns a regular buffer which can be freed
// using this macro defined to be free
//
#define  free_bbf_TLV_packet  free


// This function returns '0' if the two given pointers represent TLV structures
// of the same type and they contain the same data
//
// "memory_structure_1" and "memory_structure_2" must point (each) to a
// structure of one of the types returned by "parse_bbf_TLV_from_packet()"
//
uint8_t compare_bbf_TLV_structures(struct tlv *memory_structure_1, struct tlv *memory_structure_2);


// The next function is used to call function "callback()" on each element of
// the "memory_structure" structure
//
// "memory_structure" must point to a structure of one of the types returned by
// "parse_bbf_TLV_from_packet()"
//
// It takes four arguments:
//   - The structure whose elements are going to be visited
//   - A callback function that will be executed on each element with the
//     following arguments:
//      * A pointer to the "write()" function that will be used to dump text.
//        This is always the "write_function()" pointer provided as third
//        argument to the "visit_bbf_TLV_structure()" function.
//      * The size of the element to print (1, 2, 4, n bytes)
//      * A prefix string.
//        This is always the "prefix" value provided as fourth argument to the
//        "visit_bbf_TLV_structure()" function
//      * The name of the element (ex: "mac_address")
//      * A 'fmt' string which must be used to print the contents of the element
//      * A pointer to the element itself
//   - The "write()" function that will be used when the callback is executed
//   - A "prefix" string argument that will be used when the callback is
//     executed (it usually contains "context" information that the callback
//     function prints before anything else to make it easy to follow the
//     structure traversing order)
//
void visit_bbf_TLV_structure(struct tlv *memory_structure, visitor_callback callback, void (*write_function)(const char *fmt, ...), const char *prefix);


// Use this function for debug purposes. It turns a TLV_TYPE_* variable into its
// string representation.
//
// Example: TLV_TYPE_AL_MAC_ADDRESS_TYPE --> "TLV_TYPE_AL_MAC_ADDRESS_TYPE"
//
// Return "Unknown" if the provided type does not exist.
//
char *convert_bbf_TLV_type_to_string(uint8_t tlv_type);

#endif // END _BBF_TLVS_H_
