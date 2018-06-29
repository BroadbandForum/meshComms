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

#ifndef _LLDP_TLVS_H_
#define _LLDP_TLVS_H_

#include "platform.h"
#include <utils.h>

// In the comments below, every time a reference is made (ex: "See Section 8.5"
// or "See Table 8-2") we are talking about the contents of the following
// document:
//
//   "IEEE Std 802.1AB-2009"
//
// (http://standards.ieee.org/getieee802/download/802.1AB-2009.pdf)

// NOTE: This header file *only* implements those LLDP TLVs needed by the 1905
//       protocol.
//       In particular, from the 10 possible LLDP TLV types, here you will only
//       find 4. And only some "suboptions" for each of these 4 types will be
//       implemented.


////////////////////////////////////////////////////////////////////////////////
// TLV types as detailed in "Section 8.4.1"
////////////////////////////////////////////////////////////////////////////////
#define TLV_TYPE_END_OF_LLDPPDU             (0)
#define TLV_TYPE_CHASSIS_ID                 (1)
#define TLV_TYPE_PORT_ID                    (2)
#define TLV_TYPE_TIME_TO_LIVE               (3)


////////////////////////////////////////////////////////////////////////////////
// End of LLDPPDU TLV associated structures ("Section 8.5.1")
////////////////////////////////////////////////////////////////////////////////
struct endOfLldppduTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_END_OF_LLDPPDU

    // This structure does not contain anything at all
};


////////////////////////////////////////////////////////////////////////////////
// Chassis ID TLV associated structures ("Section 8.5.2")
////////////////////////////////////////////////////////////////////////////////
struct chassisIdTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_CHASSIS_ID

    #define CHASSIS_ID_TLV_SUBTYPE_CHASSIS_COMPONENT   (1)
    #define CHASSIS_ID_TLV_SUBTYPE_INTERFACE_ALIAS     (2)
    #define CHASSIS_ID_TLV_SUBTYPE_PORT_COMPONENT      (3)
    #define CHASSIS_ID_TLV_SUBTYPE_MAC_ADDRESS         (4)
    #define CHASSIS_ID_TLV_SUBTYPE_NETWORK_ADDRESS     (5)
    #define CHASSIS_ID_TLV_SUBTYPE_INTERFACE_NAME      (6)
    #define CHASSIS_ID_TLV_SUBTYPE_LOGICALLY_ASSIGNED  (7)
    INT8U   chassis_id_subtype;   // One of the values from above

    INT8U   chassis_id[256];      // Specific identifier for the particular
                                  // chassis.
                                  // NOTE: In our case (1905 context) we are
                                  //       only interested in generating/
                                  //       consuming chassis subtype "4" (MAC
                                  //       address), thus "chassis_id" will
                                  //       hold a six bytes array representing
                                  //       the MAC address of the transmitting
                                  //       AL entity (as explained in
                                  //       "IEEE Std 1905.1-2013 Section 6.1")
};


////////////////////////////////////////////////////////////////////////////////
// Port ID TLV associated structures ("Section 8.5.3")
////////////////////////////////////////////////////////////////////////////////
struct portIdTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_PORT_ID

    #define PORT_ID_TLV_SUBTYPE_INTERFACE_ALIAS     (1)
    #define PORT_ID_TLV_SUBTYPE_PORT_COMPONENT      (2)
    #define PORT_ID_TLV_SUBTYPE_MAC_ADDRESS         (3)
    #define PORT_ID_TLV_SUBTYPE_NETWORK_ADDRESS     (4)
    #define PORT_ID_TLV_SUBTYPE_INTERFACE_NAME      (5)
    #define PORT_ID_TLV_SUBTYPE_AGENT_CIRCUIT_ID    (6)
    #define PORT_ID_TLV_SUBTYPE_LOGICALLY_ASSIGNED  (7)
    INT8U   port_id_subtype;      // One of the values from above

    INT8U   port_id[256];         // Alpha-numeric string that contains the
                                  // specific identifier for the port from which
                                  // this LLDPPDU was transmitted
                                  // NOTE: In our case (1905 context) we are
                                  //       only interested in generating/
                                  //       consuming port subtype "3" (MAC
                                  //       address), thus "port_id" will hold a
                                  //       six bytes array representing the MAC
                                  //       address of the transmitting interface
                                  //       (as explained in "IEEE Std
                                  //       1905.1-2013 Section 6.1")
                                  // NOTE2: The standard says "alpha-numeric"
                                  //        string... but the implementations I
                                  //        have checked store a 6 bytes MAC
                                  //        address and not its string
                                  //        representation... So we are also
                                  //        storing 6 bytes here.
};


////////////////////////////////////////////////////////////////////////////////
// Time to live TLV associated structures ("Section 8.5.4")
////////////////////////////////////////////////////////////////////////////////
struct timeToLiveTypeTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_TIME_TO_LIVE

    #define TIME_TO_LIVE_TLV_1905_DEFAULT_VALUE  (180)
    INT16U  ttl;                  // Time (in seconds)
                                  // NOTE: In our case (1905 context) we are
                                  //       always setting this parameter to
                                  //       "180" (as explained in "IEEE Std
                                  //       1905.1-2013 Section 6.1")
};



////////////////////////////////////////////////////////////////////////////////
// Main API functions
////////////////////////////////////////////////////////////////////////////////

// This function receives a pointer to a stream of bytes representing an LLPD
// TLV according to "Section 8.5"
//
// It then returns a pointer to a structure whose fields have already been
// filled with the appropiate values extracted from the parsed stream.
//
// The actual type of the returned pointer structure depends on the value of
// the first byte pointed by "packet_stream" (ie. the "Type" field of the TLV):
//
//   TLV_TYPE_END_OF_LLDPPDU   -->  struct endOfLldppduTLV *
//   TLV_TYPE_CHASSIS_ID       -->  struct chassisIdTLV *
//   TLV_TYPE_PORT_ID          -->  struct portIdTLV *
//   TLV_TYPE_TIME_TO_LIVE     -->  struct timeToLiveTypeTLV *
//
// If an error was encountered while parsing the stream, a NULL pointer is
// returned instead.
// Otherwise, the returned structure is dynamically allocated, and once it is
// no longer needed, the user must call the "free_lldp_TLV_structure()" function
//
INT8U *parse_lldp_TLV_from_packet(INT8U *packet_stream);


// This is the opposite of "parse_lldp_TLV_from_packet()": it receives a
// pointer to a TLV structure and then returns a pointer to a buffer which:
//   - is a packet representation of the TLV
//   - has a length equal to the value returned in the "len" output argument
//
// "memory_structure" must point to a structure of one of the types returned by
// "parse_lldp_TLV_from_packet()"
//
// If there is a problem this function returns NULL, otherwise the returned
// buffer must be later freed by the caller (it is a regular, non-nested buffer,
// so you just need to call "free()").
//
// Note that the input structure is *not* freed. You still need to later call
// "free_lldp_TLV_structure()"
//
INT8U *forge_lldp_TLV_from_structure(INT8U *memory_structure, INT16U *len);



////////////////////////////////////////////////////////////////////////////////
// Utility API functions
////////////////////////////////////////////////////////////////////////////////

// This function receives a pointer to a TLV structure and then traverses it
// and all nested structures, calling "free()" on each one of them
//
// "memory_structure" must point to a structure of one of the types returned by
// "parse_lldp_TLV_from_packet()"
//
void free_lldp_TLV_structure(INT8U *memory_structure);


// 'forge_lldp_TLV_from_structure()' returns a regular buffer which can be freed
// using this macro defined to be PLATFORM_FREE
//
#define  free_lldp_TLV_packet  PLATFORM_FREE


// This function returns '0' if the two given pointers represent TLV structures
// of the same type and they contain the same data
//
// "memory_structure_1" and "memory_structure_2" must point (each) to a
// structure of one of the types returned by "parse_lldp_TLV_from_packet()"
//
INT8U compare_lldp_TLV_structures(INT8U *memory_structure_1, INT8U *memory_structure_2);


// The next function is used to call function "callback()" on each element of
// the "memory_structure" structure
//
// "memory_structure" must point to a structure of one of the types returned by
// "parse_lldp_TLV_from_packet()"
//
// It takes four arguments:
//   - The structure whose elements are going to be visited
//   - A callback function that will be executed on each element with the
//     following arguments:
//      * A pointer to the "write()" function that will be used to dump text.
//        This is always the "write_function()" pointer provided as third
//        argument to the "visit_lldp_TLV_structure()" function.
//      * The size of the element to print (1, 2, 4, n bytes)
//      * A prefix string.
//        This is always the "prefix" value provided as fourth argument to the
//        "visit_lldp_TLV_structure()" function/
//      * The name of the element (ex: "mac_address")
//      * A 'fmt' string which must be used to print the contents of the element
//      * A pointer to the element itself
//   - The "write()" function that will be used when the callback is executed
//   - A "prefix" string argument that will be used when the callback is
//     executed (it usually contains "context" information that the callback
//     function prints before anything else to make it easy to follow the
//     structure traversing order)
//
void visit_lldp_TLV_structure(INT8U *memory_structure,  visitor_callback callback, void (*write_function)(const char *fmt, ...), const char *prefix);


// Use this function for debug purposes. It turns a TLV_TYPE_* variable into its
// string representation.
//
// Example: TLV_TYPE_CHASSIS_ID --> "TLV_TYPE_CHASSIS_ID"
//
// Return "Unknown" if the provided type does not exist.
//
char *convert_lldp_TLV_type_to_string(INT8U tlv_type);

#endif

