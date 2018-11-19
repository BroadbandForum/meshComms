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

#ifndef _AL_SEND_H_
#define _AL_SEND_H_

#include "1905_cmdus.h"
#include "1905_tlvs.h"

#include <datamodel.h> // struct radio
#include "al_wsc.h"   // wscM2List

////////////////////////////////////////////////////////////////////////////////
// Functions to send "raw" 1905 messages
////////////////////////////////////////////////////////////////////////////////

// This function sends a "1905 packet" (the one represented by the provided
// 'cmdu' structure) on the provided interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8"
//
// 'dst_mac_address' is the destination MAC address that will be contained in
// the ETH header.
//
// Return '0' if there was a problem, '1' otherwise.
//
uint8_t send1905RawPacket(const char *interface_name, uint16_t mid, const uint8_t *dst_mac_address, struct CMDU *cmdu);

// This function sends a "1905 ALME reply" (the one represented by the provided
// 'out' pointer, which must point to a "struct *ALME" structure).
//
// 'alme_client_id' is used to identify the ALME entity where the message must
// be send. It must then match the same number originally received in the
// original ALME request message that causes this response.
//
// Return '0' if there was a problem, '1' otherwise.
//
uint8_t send1905RawALME(uint8_t alme_client_id, uint8_t *out);


////////////////////////////////////////////////////////////////////////////////
// Functions to send "specialized" 1905 messages
////////////////////////////////////////////////////////////////////////////////

// This function sends a "1905 topology discovery packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8"
//
// The format of this packet is detailed in "Section 6.3.1"
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905TopologyDiscoveryPacket(const char *interface_name, uint16_t mid);

// This function sends a "1905 topology query packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8"
//
// The format of this packet is detailed in "Section 6.3.2"
//
// 'destination_al_mac_address' is the neighbor AL MAC address where this query
// will be sent to.
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905TopologyQueryPacket(const char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address);

// This function sends a "1905 topology response packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8" (ie. it
// must be the same as the "messsage id" contained in the original "topology
// query" message)
//
// 'destination_al_mac_address' is the neighbor AL MAC address where this query
// will be sent to (ie. the AL MAC address of the node that sent the original
// "topology query" message)
//
// The format of this packet is detailed in "Section 6.3.3"
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905TopologyResponsePacket(const char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address);

// This function sends a "1905 topology notification packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8"
//
// The format of this packet is detailed in "Section 6.3.4"
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905TopologyNotificationPacket(const char *interface_name, uint16_t mid);

// This function sends a "1905 metrics query packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8"
//
// 'destination_al_mac_address' is the neighbor AL MAC address where this query
// will be sent to.
//
// The format of this packet is detailed in "Section 6.3.5"
//
// Note that the the query produced by this functions always asks for both the
// tx and rx metrics for all of the destination's neighbors.
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905MetricsQueryPacket(const char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address);

// This function sends a "1905 metrics response packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8" (ie. it
// must be the same as the "messsage id" contained in the original "topology
// query" message)
//
// 'destination_al_mac_address' is the neighbor AL MAC address where this query
// will be sent to (ie. the AL MAC address of the node that sent the original
// "metrics query" message)
//
// 'destination' is either LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS or
// LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR.
// In the first case, the metrics between 'destination_al_mac_address' and all
// of its neighbors are contained in the response.
// In the later case, only the metrics between 'destination_mac_address' and
// 'specific_neighbor' are contained in the response.
//
// 'specific_neighbor' is the AL MAC of a neighbor of 'destination_mac_address'.
// It is only used when 'destination' is set to
// LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR
//
// 'metrics_type' can be one of these:
//    - LINK_METRIC_QUERY_TLV_TX_LINK_METRICS_ONLY
//    - LINK_METRIC_QUERY_TLV_RX_LINK_METRICS_ONLY
//    - LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS
//
// The format of this packet is detailed in "Section 6.3.6"
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905MetricsResponsePacket(const char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address, uint8_t destination, uint8_t *specific_neighbor, uint8_t metrics_type);

// This function sends a "1905 push button event notification packet" on the
// provided interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8" (ie. it
// must be the same as the "messsage id" contained in the original "topology
// query" message)
//
// 'all_interfaces_names' is a list of interfaces names that will be used to
// build the contents of the "push button event" TLV that the message will
// contain.
// This list contains 'nr' elements and only those whose index ("i") satisfies
// 'push_button_mask[i] == 0' will actually be included in the TLV list.
//
// Example:
//   all_interfaces_names = { "eth0", "eth1" }  |  Only data regarding "eth0"
//   push_button_mask     = [      0,      1 }  |  will be included in the TLVs
//   nr                   = 2                   |
//
// The format of this packet is detailed in "Section 6.3.11"
//
// The destination MAC address of the packet will always be the 1905 multicast
// address ("01:80:C2:00:00:13")
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905PushButtonEventNotificationPacket(const char *interface_name, uint16_t mid, char **all_interfaces_names, uint8_t *push_button_mask, uint8_t nr);

// This function sends a "1905 push join notification packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8" (ie. it
// must be the same as the "messsage id" contained in the original "topology
// query" message)
//
// 'original_al_mac_address' is the AL MAC address contained in the original
// "push button event notification" message that resulted in this "push button
// join notification" message beeing generated.
//
//'original_mid' is the "message ID" of the original "push button event
// notification" message that resulted in this "push button join notification"
// message beeing generated.
//
// 'local_mac_address' is the MAC address of the interface where the "push
// button" process successfully completed.
//
// 'new_mac_address' is the MAC address of the interface at the other end of the
// link which has just been secured.
// For "ethernet" interfaces this must be set to all zeros.
//
// The destination MAC address of the packet will always be the 1905 multicast
// address ("01:80:C2:00:00:13")
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905PushButtonJoinNotificationPacket(const char *interface_name, uint16_t mid, uint8_t *original_al_mac_address, uint16_t original_mid, uint8_t *local_mac_address, uint8_t *new_mac_address);

// This function sends a "AP-autoconfiguration search packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8"
//
// 'freq_band' is the wifi frequency band of the unconfigured AP interface that
// is searching for configuration parameters. It can take any of the following
// values:
//   - IEEE80211_FREQUENCY_BAND_2_4_GHZ
//   - IEEE80211_FREQUENCY_BAND_5_GHZ
//   - IEEE80211_FREQUENCY_BAND_60_GHZ
//
// The destination MAC address of the packet will always be the 1905 multicast
// address ("01:80:C2:00:00:13")
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905APAutoconfigurationSearchPacket(const char *interface_name, uint16_t mid, uint8_t freq_band);

// This function sends a "AP-autoconfiguration response packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8". In other
// words it must match the value of the 'mid' field contained in the original
// "AP autoconfiguration response" message that resulted in this response being
// generated.
//
// 'destination_al_mac_address' is the neighbor AL MAC address where this query
// will be sent to. It must match the 'AL MAC' field contained in the original
// "AP autoconfiguration response" message that resulted in this response being
// generated.
//
// 'freq_band' is the wifi frequency band supported by this AP/registrar. It
// must match the 'freq_band' field contained in the original "AP
// autoconfiguration response" message that resulted in this response being
// generated.
//
// If "include_easymesh" is true, add the easyMesh TLV supportedService.
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905APAutoconfigurationResponsePacket(const char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address,
                                                uint8_t freq_band, bool include_easymesh);

// This function sends a "AP-autoconfiguration WSC packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8". In other
// words it must always be a new MID (when either sending an M1 or an M2
// inside 'wsc_frame').
//
// 'destination_al_mac_address' is the neighbor AL MAC address where this query
// will be sent to.
// When 'wsc_frame' contains a message of type M1, this destination MAC must be
// the one of the node which previously sent the "AP-autoconfiguration
// response".
// When 'wsc_frame' contains a message of type M2, this destination MAC must be
// the one of the node which previously sent the "AP-autoconfiguration
// WSC" containing a message of type M1.
//
// 'wsc_frame' is a pointer to 'wsc_frame_size' bytes, containing either a
// message of type M1 or M2.
// The format of these messages is described in the "Wi-Fi simple configuration"
// document (TODO: actual name of the document)
//
// If 'send_radio_basic_capabilities' is true, the "AP Radio Basic Capabilities"
// TLV will be included in the message.
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905APAutoconfigurationWSCM1Packet(const char *interface_name, uint16_t mid, const uint8_t *destination_al_mac_address,
                                               const uint8_t *wsc_frame, uint16_t wsc_frame_size,
                                               const struct radio *radio, bool send_radio_basic_capabilities);

uint8_t send1905APAutoconfigurationWSCM2Packet(const char *interface_name, uint16_t mid, const uint8_t *destination_al_mac_address,
                                               wscM2List wsc_frames,
                                               const mac_address radio_uid, bool send_radio_identifier);

// This function sends a "1905 generic phy query packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8"
//
// The format of this packet is detailed in "Section 6.3.14"
//
// 'destination_al_mac_address' is the neighbor AL MAC address where this query
// will be sent to.
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905GenericPhyQueryPacket(const char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address);

// This function sends a "1905 generic phy response packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8" (ie. it
// must be the same as the "messsage id" contained in the original "topology
// query" message)
//
// 'destination_al_mac_address' is the neighbor AL MAC address where this query
// will be sent to (ie. the AL MAC address of the node that sent the original
// "topology query" message)
//
// The format of this packet is detailed in "Section 6.3.15"
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905GenericPhyResponsePacket(const char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address);

// This function sends a "1905 high layer query packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8"
//
// The format of this packet is detailed in "Section 6.3.16"
//
// 'destination_al_mac_address' is the neighbor AL MAC address where this query
// will be sent to.
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905HighLayerQueryPacket(const char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address);

// This function sends a "1905 high layer response packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8" (ie. it
// must be the same as the "messsage id" contained in the original "topology
// query" message)
//
// 'destination_al_mac_address' is the neighbor AL MAC address where this query
// will be sent to (ie. the AL MAC address of the node that sent the original
// "topology query" message)
//
// The format of this packet is detailed in "Section 6.3.17"
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905HighLayerResponsePacket(const char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address);

// This function sends an "interface power change information packet" on the
// provided interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8"
//
// 'destination_al_mac_address' is the neighbor AL MAC address where this query
// will be sent to (ie. the AL MAC address of the node that sent the original
// "interface power change request" message)
//
// 'remote_interfaces' is a list of 'nr' remote mac addresses whose status is
// being requested to change.
//
// 'new_states' is a list of 'nr' states to be applied (each one to each of the
// remote interfaces specified in 'remote_interfaces').
// Each state can take any of the following values:
//   - POWER_STATE_REQUEST_OFF
//   - POWER_STATE_REQUEST_ON
//   - POWER_STATE_REQUEST_SAVE
//
// The format of this packet is detailed in "Section 6.3.18"
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905InterfacePowerChangeRequestPacket(const char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address, uint8_t (*remote_interfaces)[6], uint8_t *new_states, uint8_t nr);

// This function sends an "interface power change request packet" on the
// provided interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
// 'mid' is the "Message identifier" value we want this packet to contain. Its
// value must be calculated according to what is said on "Section 7.8" (ie. it
// must be the same as the "messsage id" contained in the original "topology
// query" message)
//
// 'destination_al_mac_address' is the neighbor AL MAC address where this query
// will be sent to (ie. the AL MAC address of the node that sent the original
// "interface power change request" message)
//
// 'local_interfaces' is a list of 'nr' local mac addresses whose state was
// recently tried to be changed (as a result of receiving an "interface power
// change information" message) and whose result code (from trying to apply
// that change) we are going to report back now
//
// 'results' is a list of 'nr' result codes (each one contains the resulting
// code from trying to apply a "power change" operation on the corresponding
// local interface from the 'local_interfaces' list)
// Each result code can take any of the following values:
//   -POWER_STATE_RESULT_COMPLETED
//   -POWER_STATE_RESULT_NO_CHANGE
//   -POWER_STATE_RESULT_ALTERNATIVE_CHANGE
//
// The format of this packet is detailed in "Section 6.3.19"
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t send1905InterfacePowerChangeResponsePacket(const char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address, uint8_t (*local_interfaces)[6], uint8_t *results, uint8_t nr);

////////////////////////////////////////////////////////////////////////////////
// Functions to send LLDP messages
////////////////////////////////////////////////////////////////////////////////

// This function sends a "LLDP bridge discovery packet" on the provided
// interface.
//
// 'interface_name' is one of the values returned by
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()" and must refer to the interface we
// want to use to send the packet.
//
//
// The format of this packet is detailed in "Section 6.1"
//
// Return "0" if a problem was found. "1" otherwise.
//
uint8_t sendLLDPBridgeDiscoveryPacket(const char *interface_name);


////////////////////////////////////////////////////////////////////////////////
// Functions to send ALME reply messages
////////////////////////////////////////////////////////////////////////////////

// Compose and send an "ALME-GET-INTF-LIST.response" message
//
// 'alme_client_id' must be the same one used to receive the original request.
//
uint8_t send1905InterfaceListResponseALME(uint8_t alme_client_id);

// Compose and send an "ALME-GET-METRIC.response" message
//
// 'alme_client_id' must be the same one used to receive the original request.
//
// 'mac_address' is the neighbor AL MAC address at the other end of the link
// whose metrics are going to be reported.
// If 'mac_address' is NULL, all metrics between the current node and all its
// neighbors are reported instead.
//
uint8_t send1905MetricsResponseALME(uint8_t alme_client_id, uint8_t *mac_address);


// Compose and send an "ALME-CUSTOM-COMMAND.response" message
//
// 'alme_client_id' must be the same one used to receive the original request.
//
// 'command' is the particular custom command whose response is going to be
// generated and sent back (ie. the 'command' contained in the original request)
// This 'command' can take any of the "CUSTOM_COMMAND_*" available values.
//
uint8_t send1905CustomCommandResponseALME(uint8_t alme_client_id, uint8_t command);

#endif
