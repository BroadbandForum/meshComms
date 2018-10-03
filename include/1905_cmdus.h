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

#ifndef _1905_CDMUS_H_
#define _1905_CDMUS_H_

#include "platform.h"
#include "utils.h"
#include "tlv.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h> // size_t

////////////////////////////////////////////////////////////////////////////////
// CMDU message type as detailed in "IEEE Std 1905.1-2013, Table 6-4"
////////////////////////////////////////////////////////////////////////////////
#define CMDU_TYPE_TOPOLOGY_DISCOVERY               0x0000
#define CMDU_TYPE_TOPOLOGY_NOTIFICATION            0x0001
#define CMDU_TYPE_TOPOLOGY_QUERY                   0x0002
#define CMDU_TYPE_TOPOLOGY_RESPONSE                0x0003
#define CMDU_TYPE_VENDOR_SPECIFIC                  0x0004
#define CMDU_TYPE_LINK_METRIC_QUERY                0x0005
#define CMDU_TYPE_LINK_METRIC_RESPONSE             0x0006
#define CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH      0x0007
#define CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE    0x0008
#define CMDU_TYPE_AP_AUTOCONFIGURATION_WSC         0x0009
#define CMDU_TYPE_AP_AUTOCONFIGURATION_RENEW       0x000a
#define CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION   0x000b
#define CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION    0x000c
#define CMDU_TYPE_HIGHER_LAYER_QUERY               0x000d
#define CMDU_TYPE_HIGHER_LAYER_RESPONSE            0x000e
#define CMDU_TYPE_INTERFACE_POWER_CHANGE_REQUEST   0x000f
#define CMDU_TYPE_INTERFACE_POWER_CHANGE_RESPONSE  0x0010
#define CMDU_TYPE_GENERIC_PHY_QUERY                0x0011
#define CMDU_TYPE_GENERIC_PHY_RESPONSE             0x0012


////////////////////////////////////////////////////////////////////////////////
// CMDU message version
////////////////////////////////////////////////////////////////////////////////
#define CMDU_MESSAGE_VERSION_1905_1_2013  (0x00)


////////////////////////////////////////////////////////////////////////////////
// CMDU associated structures
////////////////////////////////////////////////////////////////////////////////


/** @brief Header information of a single CMDU packet.
 *
 * This structure contains the information parsed out of the CMDU header that is relevant to be able to handle
 * relaying, duplicates, and fragmentation/reassembly.
 */
struct CMDU_header
{
    mac_address dst_addr;                /**< @brief Destination MAC address of this packet. */
    mac_address src_addr;                /**< @brief Source MAC address of this packet. */
    uint16_t    message_type;            /**< @brief Message type vlaue of this packet (in host byte order). */
    uint16_t    mid;                     /**< @brief Message-ID (MID) value of this packet (in host byte order). */
    uint8_t     fragment_id;             /**< @brief Fragment-ID valud of this packet. */
    bool        last_fragment_indicator; /**< @brief If true, this is the last (or only) fragment of this CMDU. */
};

struct CMDU
{
    uint8_t   message_version;       // One of "CMDU_MESSAGE_VERSION_*" values

    uint16_t  message_type;          // Any of the CMDU_TYPE_* types

    uint16_t  message_id;            // Identifies the message

    uint8_t   relay_indicator;       // Set to '1' to indicate that his packet
                                   // must be relayed by 1905 AL to its
                                   // neighbors.

    struct tlv **list_of_TLVs;        // NULL-terminated list of pointers to TLV
                                      // structures.
                                      // The "end of message" TLV is not included
                                      // in this list.
};



////////////////////////////////////////////////////////////////////////////////
// Main API functions
////////////////////////////////////////////////////////////////////////////////

// This function receives a NULL-terminated list of pointers to streams
// containing ETH layer packet data (ie. offset +14 in a raw network packet,
// just after the source MAC address, destination MAC address and ETH type
// fields).
//
// The payload of each stream must contain a 1905 CMDU as detailed in ""IEEE
// Std 1905.1-2013, Section 6.2 (Table 6-3 and beyond)"
//
// The list of received streams must "belong" to one same "message":
//   * The 'message_id' must be the same one in all of them
//   * The 'last_fragment_indicator' flag must be set to '1' in *exactly* one
//     (and only one!) of them.
//
// Note that the streams do not need to be provided in order. The function will
// take care of that.
//
// In the typical case where there is no fragmentation, this function will
// receive a single stream (actually, a list of two pointers: one pointing to
// the stream and another one set to NULL to mark the end of the list) with the
// 'last_fragment_indicator' set to '1', otherwise it will receive a list of
// streams where only the last one has 'last_fragment_indicator' set to '1' and
// all of them share the same 'message_id'.
//
// For each possible CMDU message type, the following list of TLVs is returned:
//
//   CMDU_TYPE_TOPOLOGY_DISCOVERY
//     - 1 TLV_TYPE_AL_MAC_ADDRESS_TYPE
//     - 1 TLV_TYPE_MAC_ADDRESS_TYPE
//
//   CMDU_TYPE_TOPOLOGY_NOTIFICATION
//     - 1 TLV_TYPE_AL_MAC_ADDRESS_TYPE
//
//   CMDU_TYPE_TOPOLOGY_QUERY
//     - <no requirements>
//
//   CMDU_TYPE_TOPOLOGY_RESPONSE
//     - 1 TLV_TYPE_DEVICE_INFORMATION_TYPE
//     - 0 or more TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES
//     - 0 or more TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST
//     - 0 or more TLV_TYPE_NEIGHBOR_DEVICE_LIST
//
//   CMDU_TYPE_VENDOR_SPECIFIC
//     - 1 TLV_TYPE_VENDOR_SPECIFIC (must be the first one received!)
//     - 0 or more TLVs of any type (always after the previous one)
//
//   CMDU_TYPE_LINK_METRIC_QUERY
//     - 1 TLV_TYPE_LINK_METRIC_QUERY
//
//   CMDU_TYPE_LINK_METRIC_RESPONSE
//     - For each valid 1905 neighbor one of the followings options
//       - 1 TLV_TYPE_TRANSMITTER_LINK_METRIC
//       - 1 TLV_TYPE_RECEIVER_LINK_METRIC
//       - 1 TLV_TYPE_TRANSMITTER_LINK_METRIC *and* 1 TLV_TYPE_RECEIVER_LINK_METRIC
//
//   CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH
//     - 1 TLV_TYPE_AL_MAC_ADDRESS_TYPE
//     - 1 TLV_TYPE_SEARCHED_ROLE
//     - 1 TLV_TYPE_AUTOCONFIG_FREQ_BAND
//
//   CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE
//     - 1 TLV_TYPE_SUPPORTED_ROLE
//     - 1 TLV_TYPE_SUPPORTED_FREQ_BAND
//
//   CMDU_TYPE_AP_AUTOCONFIGURATION_WSC
//     - 1 TLV_TYPE_WSC
//
//   CMDU_TYPE_AP_AUTOCONFIGURATION_RENEW
//     - 1 TLV_TYPE_AL_MAC_ADDRESS_TYPE
//     - 1 TLV_TYPE_SUPPORTED_ROLE
//     - 1 TLV_TYPE_SUPPORTED_FREQ_BAND
//
//   CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION
//     - 1 TLV_TYPE_AL_MAC_ADDRESS_TYPE
//     - 1 TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION
//
//   CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION
//     - 1 TLV_TYPE_AL_MAC_ADDRESS_TYPE
//     - 1 TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION
//
// In addition, *all* of them must *always* end with one TLV_TYPE_END_OF_MESSAGE
// TLV.
//
// If the aggregated payload of all the streams does not contain one of the TLVs
// required for its 'message_type', this function returns an error (a NULL
// pointer).
//
// If the aggregated payload of all the streams contains more TLVs than those
// required for its 'message_type', they are ignored (ie. not contained in the
// returned structure)
//
// The returned structure will then contain:
//   - 'message_version' set to the same value contained in all streams
//   - 'message_type' set to the same value contained in all streams
//   - 'message_id' set to the same value contained in all streams
//   - 'relay_indicator' set to the same value contained in all streams
//   - 'list_of_TLVs' is a NULL-terminated list of pointers to the TLVs as
//     explained before (note that TLV_TYPE_END_OF_MESSAGE is *not* included in
//     this list)
//
// If any type of error/inconsistency is found, a NULL pointer is returned
// instead, otherwise remember to free the received structure once you don't
// need it anymore (using the "free_1905_CMDU_structure()" function)
//
//   NOTE: An 'inconsistency' happens when one of the fragment, for example,
//         contains a different 'message_version' from another fragment or if
//         two streams contain fragments with the same 'fragment_id', etc...
//
struct CMDU *parse_1905_CMDU_from_packets(uint8_t **packet_streams);


// This is the opposite of "parse_1905_CMDU_from_packets()": it receives a
// pointer to a TLV structure and then returns a list of pointers to fragmented
// streams.
//
// Note that if everything fits under MAX_NETWORK_SEGMENT_SIZE size, only one
// stream will be returned (actually, what will be returned is a list of two
// elements, containing the pointer to the stream and a NULL entry)
//
// Fragments will be returned in order so that only the last one will contain
// the 'last_fragment_indicator' flag set.
//
// 'lens' is also an output argument: it is dynamically allocated by this
// function and contains the number of bytes of each returned stream and a '0'
// element at the end.
//
// The provided 'memory_structure' must have its fields properly filled or else
// this function will return an error (ie. a NULL pointer).
// What does this mean? Easy: for each type of message only the expected TLVs
// can be included in the 'list_of_TLVs' list.
// The list of acceptable values for each message type is detailed in the
// "parse_1905_CMDU_from_packets()" function documentation.
//
//   NOTE: Notice how, in "parse_1905_CMDU_from_packets()", unexpected TLVs are
//         discarded, but here they produce an error. This is expected!
//
// In addition, the 'relay_indicator' field will only be respected when
// 'message_type' == CMDU_TYPE_VENDOR_SPECIFIC. In all other cases the value
// that is defined in the standard will be used instead. So don't worry about
// setting it to the correct value (it will be done automatically for you).
//
// One more thing: the provided 'memory_structure' is not freed by this
// function, thus, once this function returns, you will be responsible for
// freeing three things:
//
//   - The 'memory_structure' CMDU structure (you were already responsible for),
//     that you can free with 'free_1905_CMDU_structure()'
//
//   - The 'lens' array, which you can later free with "free()"
//
//   - The returned streams and the array where they are contained. Both can
//     be freed with 'free_1905_CMDU_packets()'
//
//
uint8_t **forge_1905_CMDU_from_structure(const struct CMDU *memory_structure, uint16_t **lens);



////////////////////////////////////////////////////////////////////////////////
// Utility API functions
////////////////////////////////////////////////////////////////////////////////


/** @brief Parse the Ethernet and CMDU headers from a packet.
 *
 * Parse the ::CMDU_header information out of @a packet_buffer.
 *
 * @param[in] packet_buffer The packet to parse, including Ethernet header.
 * @param[in] len The length of @a packet_buffer.
 * @param[out] cmdu_header The parsed CMDU header information. Must not be NULL.
 * @return @a true if a valid CMDU header was found (@a cmdu_header is filled with the header information), @a false
 *         if not (@a cmdu_header may be partially overwritten).
 */
bool parse_1905_CMDU_header_from_packet(const uint8_t *packet_buffer, size_t len, struct CMDU_header *cmdu_header);


// This function receives a pointer to a CMDU structure and then traverses it
// and all nested structures, calling "free()" on each one of them
//
//
void free_1905_CMDU_structure(struct CMDU *memory_structure);


// This function receives a pointer to a list of streams (such as the one
// returned by 'forge_1905_CMDU_from_structure()' and frees all the associated
// structures
//
//
void free_1905_CMDU_packets(uint8_t **packet_streams);


// This function returns '0' if the two given pointers represent CMDU structures
// that contain the same data
//
uint8_t compare_1905_CMDU_structures(const struct CMDU *memory_structure_1, const struct CMDU *memory_structure_2);


// The next function is used to call function "callback()" on each element of
// the "memory_structure" structure
//
// "memory_structure" must point to a structure of one of the types returned by
// "parse_1905_CMDU_from_packets()"
//
// It takes four arguments:
//   - The structure whose elements are going to be visited
//   - A callback function that will be executed on each element with the
//     following arguments:
//      * A pointer to the "write()" function that will be used to dump text.
//        This is always the "write_function()" pointer provided as third
//        argument to the "visit_1905_CMDU_structure()" function.
//      * The size of the element to print (1, 2, 4, n bytes)
//      * A prefix string.
//        This is always the "prefix" value provided as fourth argument to the
//        "visit_1905_CMDU_structure()" function
//      * The name of the element (ex: "mac_address")
//      * A 'fmt' string which must be used to print the contents of the element
//      * A pointer to the element itself
//   - The "write()" function that will be used when the callback is executed
//   - A "prefix" string argument that will be used when the callback is
//     executed (it usually contains "context" information that the callback
//     function prints before anything else to make it easy to follow the
//     structure traversing order)
//
void visit_1905_CMDU_structure(const struct CMDU *memory_structure,
                               visitor_callback callback,
                               void (*write_function)(const char *fmt, ...),
                               const char *prefix);

// Use this function for debug purposes. It turns a CMDU_TYPE_* variable into its
// string representation.
//
// Example: CMDU_TYPE_TOPOLOGY_NOTIFICATION --> "CMDU_TYPE_TOPOLOGY_NOTIFICATION"
//
// Return "Unknown" if the provided type does not exist.
//
char *convert_1905_CMDU_type_to_string(uint8_t cmdu_type);
#endif
