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

#ifndef _BBF_TLVS_H_
#define _BBF_TLVS_H_

#include "platform.h"


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


static const INT8U BBF_OUI[3] = {0x00, 0x25, 0x6d};

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
INT8U *parse_bbf_TLV_from_packet(INT8U *packet_stream);


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
INT8U *forge_bbf_TLV_from_structure(INT8U *memory_structure, INT16U *len);



////////////////////////////////////////////////////////////////////////////////
// Utility API functions
////////////////////////////////////////////////////////////////////////////////

// This function receives a pointer to a TLV structure and then traverses it
// and all nested structures, calling "free()" on each one of them
//
// "memory_structure" must point to a structure of one of the types returned by
// "parse_bbf_TLV_from_packet()"
//
void free_bbf_TLV_structure(INT8U *memory_structure);


// 'forge_bbf_TLV_from_structure()' returns a regular buffer which can be freed
// using this macro defined to be PLATFORM_FREE
//
#define  free_bbf_TLV_packet  PLATFORM_FREE


// This function returns '0' if the two given pointers represent TLV structures
// of the same type and they contain the same data
//
// "memory_structure_1" and "memory_structure_2" must point (each) to a
// structure of one of the types returned by "parse_bbf_TLV_from_packet()"
//
INT8U compare_bbf_TLV_structures(INT8U *memory_structure_1, INT8U *memory_structure_2);


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
void visit_bbf_TLV_structure(INT8U *memory_structure, void (*callback)(void (*write_function)(const char *fmt, ...), const char *prefix, INT8U size, const char *name, const char *fmt, void *p), void (*write_function)(const char *fmt, ...), const char *prefix);


// Use this function for debug purposes. It turns a TLV_TYPE_* variable into its
// string representation.
//
// Example: TLV_TYPE_AL_MAC_ADDRESS_TYPE --> "TLV_TYPE_AL_MAC_ADDRESS_TYPE"
//
// Return "Unknown" if the provided type does not exist.
//
char *convert_bbf_TLV_type_to_string(INT8U tlv_type);

#endif // END _BBF_TLVS_H_
