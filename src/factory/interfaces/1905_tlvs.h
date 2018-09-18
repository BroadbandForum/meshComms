/*
 *  prplMesh Wi-Fi Multi-AP
 *
 *  Copyright (c) 2017, Broadband Forum
 *  Copyright (c) 2018, prpl Foundation
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

#ifndef _1905_TLVS_H_
#define _1905_TLVS_H_

#include "platform.h"
#include <utils.h>
#include <tlv.h>
#include <hlist.h>
#include <string.h> // memcpy()

// In the comments below, every time a reference is made (ex: "See Section 6.4"
// or "See Table 6-11") we are talking about the contents of the following
// document:
//
//   "IEEE Std 1905.1-2013"

////////////////////////////////////////////////////////////////////////////////
// TLV types as detailed in "Section 6.4"
////////////////////////////////////////////////////////////////////////////////
#define TLV_TYPE_END_OF_MESSAGE                      (0)
#define TLV_TYPE_VENDOR_SPECIFIC                     (11)
#define TLV_TYPE_AL_MAC_ADDRESS_TYPE                 (1)
#define TLV_TYPE_MAC_ADDRESS_TYPE                    (2)
#define TLV_TYPE_DEVICE_INFORMATION_TYPE             (3)
#define TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES        (4)
#define TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST       (6)
#define TLV_TYPE_NEIGHBOR_DEVICE_LIST                (7)
#define TLV_TYPE_LINK_METRIC_QUERY                   (8)
#define TLV_TYPE_TRANSMITTER_LINK_METRIC             (9)
#define TLV_TYPE_RECEIVER_LINK_METRIC                (10)
#define TLV_TYPE_LINK_METRIC_RESULT_CODE             (12)
#define TLV_TYPE_SEARCHED_ROLE                       (13)
#define TLV_TYPE_AUTOCONFIG_FREQ_BAND                (14)
#define TLV_TYPE_SUPPORTED_ROLE                      (15)
#define TLV_TYPE_SUPPORTED_FREQ_BAND                 (16)
#define TLV_TYPE_WSC                                 (17)
#define TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION      (18)
#define TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION       (19)
#define TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION      (20)
#define TLV_TYPE_DEVICE_IDENTIFICATION               (21)
#define TLV_TYPE_CONTROL_URL                         (22)
#define TLV_TYPE_IPV4                                (23)
#define TLV_TYPE_IPV6                                (24)
#define TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION      (25)
#define TLV_TYPE_1905_PROFILE_VERSION                (26)
#define TLV_TYPE_POWER_OFF_INTERFACE                 (27)
#define TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION  (28)
#define TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS       (29)
#define TLV_TYPE_L2_NEIGHBOR_DEVICE                  (30)

/** @brief EasyMesh TLV types as detailed in tables 6 to 41 of "Multi-AP Specification Version 1.0".
 *
 * @{
 */

#define TLV_TYPE_SUPPORTED_SERVICE                   (0x80)
#define TLV_TYPE_SEARCHED_SERVICE                    (0x81)
#define TLV_TYPE_AP_OPERATIONAL_BSS                  (0x83)
#define TLV_TYPE_ASSOCIATED_CLIENTS                  (0x84)

/** @} */


////////////////////////////////////////////////////////////////////////////////
// Media types as detailed in "Table 6-12"
////////////////////////////////////////////////////////////////////////////////
#define MEDIA_TYPE_IEEE_802_3U_FAST_ETHERNET       (0x0000)
#define MEDIA_TYPE_IEEE_802_3AB_GIGABIT_ETHERNET   (0x0001)
#define MEDIA_TYPE_IEEE_802_11B_2_4_GHZ            (0x0100)
#define MEDIA_TYPE_IEEE_802_11G_2_4_GHZ            (0x0101)
#define MEDIA_TYPE_IEEE_802_11A_5_GHZ              (0x0102)
#define MEDIA_TYPE_IEEE_802_11N_2_4_GHZ            (0x0103)
#define MEDIA_TYPE_IEEE_802_11N_5_GHZ              (0x0104)
#define MEDIA_TYPE_IEEE_802_11AC_5_GHZ             (0x0105)
#define MEDIA_TYPE_IEEE_802_11AD_60_GHZ            (0x0106)
#define MEDIA_TYPE_IEEE_802_11AF_GHZ               (0x0107)
#define MEDIA_TYPE_IEEE_1901_WAVELET               (0x0200)
#define MEDIA_TYPE_IEEE_1901_FFT                   (0x0201)
#define MEDIA_TYPE_MOCA_V1_1                       (0x0300)
#define MEDIA_TYPE_UNKNOWN                         (0xFFFF)


////////////////////////////////////////////////////////////////////////////////
// IEEE802.11 frequency bands used in "Tables 6-22 and 6-24"
////////////////////////////////////////////////////////////////////////////////
#define IEEE80211_ROLE_REGISTRAR                   (0x00)


////////////////////////////////////////////////////////////////////////////////
// IEEE802.11 frequency bands used in "Tables 6-23 and 6-25"
////////////////////////////////////////////////////////////////////////////////
#define IEEE80211_FREQUENCY_BAND_2_4_GHZ           (0x00)
#define IEEE80211_FREQUENCY_BAND_5_GHZ             (0x01)
#define IEEE80211_FREQUENCY_BAND_60_GHZ            (0x02)


////////////////////////////////////////////////////////////////////////////////
// Media type structures detailed in "Tables 6-12 and 6-13"
////////////////////////////////////////////////////////////////////////////////
struct _ieee80211SpecificInformation
{
    uint8_t  network_membership[6]; // BSSID

    #define IEEE80211_SPECIFIC_INFO_ROLE_AP                   (0x0)
    #define IEEE80211_SPECIFIC_INFO_ROLE_NON_AP_NON_PCP_STA   (0x4)
    #define IEEE80211_SPECIFIC_INFO_ROLE_WIFI_P2P_CLIENT      (0x8)
    #define IEEE80211_SPECIFIC_INFO_ROLE_WIFI_P2P_GROUP_OWNER (0x9)
    #define IEEE80211_SPECIFIC_INFO_ROLE_AD_PCP               (0xa)
    uint8_t  role;                  // One of the values from above

    uint8_t ap_channel_band;        // Hex value of dot11CurrentChannelBandwidth
                                  // (see "IEEE P802.11ac/D3.0" for description)

    uint8_t ap_channel_center_frequency_index_1;
                                  // Hex value of
                                  // dot11CurrentChannelCenterFrequencyIndex1
                                  // (see "IEEE P802.11ac/D3.0" for description)

    uint8_t ap_channel_center_frequency_index_2;
                                  // Hex value of
                                  // dot11CurrentChannelCenterFrequencyIndex2
                                  // (see "IEEE P802.11ac/D3.0" for description)
};
struct _ieee1901SpecificInformation
{
    uint8_t network_identifier[7];  // Network membership
};
union _mediaSpecificData
{
    uint8_t                                dummy;    // Empty placeholder
    struct _ieee80211SpecificInformation ieee80211;
    struct _ieee1901SpecificInformation  ieee1901;

};


////////////////////////////////////////////////////////////////////////////////
// Generic phy common structure used in "Tables 6.29, 6.36 and 6.38"
////////////////////////////////////////////////////////////////////////////////
struct _genericPhyCommonData
{
    uint8_t   oui[3];                   // OUI of the generic phy networking
                                      // technology of the local interface

    uint8_t   variant_index;            // Variant index of the generic phy
                                      // networking technology of the local
                                      // interface

    uint8_t   media_specific_bytes_nr;
    uint8_t  *media_specific_bytes;     // Media specific information of the
                                      // variant.
                                      // This field contains
                                      // "media_specific_bytes_nr" bytes.
};


////////////////////////////////////////////////////////////////////////////////
// Vendor specific TLV associated structures ("Section 6.4.2")
////////////////////////////////////////////////////////////////////////////////
struct vendorSpecificTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_VENDOR_SPECIFIC. */

    uint8_t  vendorOUI[3];          // Vendor specific OUI, the value of the 24
                                  // bit globally unique IEEE-SA assigned number
                                  // to the vendor

    uint16_t  m_nr;                 // Bytes in the following field
    uint8_t  *m;                    // Vendor specific information
};

/** @brief Allocate an empty vendorSpecificTLV.
 *
 * The buffer vendorSpecificTLV::m is not allocated, this must be done by the caller.
 */
struct vendorSpecificTLV *vendorSpecificTLVAlloc(hlist_head *parent);

////////////////////////////////////////////////////////////////////////////////
// AL MAC address type TLV associated structures ("Section 6.4.3")
////////////////////////////////////////////////////////////////////////////////
struct alMacAddressTypeTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_AL_MAC_ADDRESS_TYPE. */

    uint8_t   al_mac_address[6];    // 1905 AL MAC address of the transmitting
                                  // device
};


////////////////////////////////////////////////////////////////////////////////
// MAC address type TLV associated structures ("Section 6.4.4")
////////////////////////////////////////////////////////////////////////////////
struct macAddressTypeTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_MAC_ADDRESS_TYPE. */

    uint8_t   mac_address[6];       // MAC address of the interface on which the
                                  // message is transmitted
};


////////////////////////////////////////////////////////////////////////////////
// Device information type TLV associated structures ("Section 6.4.5")
////////////////////////////////////////////////////////////////////////////////
struct _localInterfaceEntries
{
    uint8_t   mac_address[6];       // MAC address of the local interface

    uint16_t  media_type;           // One of the MEDIA_TYPE_* values

    uint8_t   media_specific_data_size;
                                  // Number of bytes in ensuing field
                                  // Its value is '10' when 'media_type' is one
                                  // of the valid MEDIA_TYPE_IEEE_802_11*
                                  // values.
                                  // Its value is '7' when 'media_type' is one
                                  // of the valid MEDIA_TYPE_IEEE_1901* values.

    union _mediaSpecificData media_specific_data;
                                  // Media specific data
                                  // It will contain a IEEE80211 structure
                                  // when 'media_type' is one of the valid
                                  // MEDIA_TYPE_IEEE_802_11* values
                                  // It will contain a IEE1905 structure
                                  // when 'media_type' is one of the valid
                                  // MEDIA_TYPE_IEEE_1901* values
                                  // It will be empty in the rest of cases

};
struct deviceInformationTypeTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_DEVICE_INFORMATION_TYPE. */

    uint8_t   al_mac_address[6];    // 1905 AL MAC address of the device

    uint8_t   local_interfaces_nr;  // Number of local interfaces

    struct _localInterfaceEntries  *local_interfaces;

};


////////////////////////////////////////////////////////////////////////////////
// Device bridging capability TLV associated structures ("Section 6.4.6")
////////////////////////////////////////////////////////////////////////////////
struct _bridgingTupleMacEntries
{
    uint8_t   mac_address[6];       // MAC address of a 1905 device's network
                                  // interface that belongs to a bridging tuple
};
struct _bridgingTupleEntries
{
    uint8_t   bridging_tuple_macs_nr; // Number of MAC addresses in this bridging
                                    // tuple

    struct  _bridgingTupleMacEntries  *bridging_tuple_macs;
                                   // List of 'mac_nr' elements, each one
                                   // representing a MAC. All these MACs are
                                   // bridged together.
};
struct deviceBridgingCapabilityTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES. */

    uint8_t   bridging_tuples_nr;   // Number of MAC addresses in this bridging
                                  // tuple

    struct _bridgingTupleEntries  *bridging_tuples;

};


////////////////////////////////////////////////////////////////////////////////
// Non-1905 neighbor device list TLV associated structures ("Section 6.4.8")
////////////////////////////////////////////////////////////////////////////////
struct _non1905neighborEntries
{
    uint8_t   mac_address[6];       // MAC address of the non-1905 device
};
struct non1905NeighborDeviceListTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST. */

    uint8_t   local_mac_address[6]; // MAC address of the local interface

    uint8_t                           non_1905_neighbors_nr;
    struct _non1905neighborEntries *non_1905_neighbors;
                                  // One entry for each non-1905 detected
                                  // neighbor
};


////////////////////////////////////////////////////////////////////////////////
// Neighbor device TLV associated structures ("Section 6.4.9")
////////////////////////////////////////////////////////////////////////////////
struct _neighborEntries
{
    uint8_t   mac_address[6];       // AL MAC address of the 1905 neighbor

    uint8_t   bridge_flag;          // "0" --> no IEEE 802.1 bridge exists
                                  // "1" --> at least one IEEE 802.1 bridge
                                  //         exists between this device and the
                                  //         neighbor
};
struct neighborDeviceListTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_NEIGHBOR_DEVICE_LIST. */

    uint8_t   local_mac_address[6]; // MAC address of the local interface

    uint8_t                     neighbors_nr;
    struct _neighborEntries  *neighbors;
                                  // One entry for each 1905 detected neighbor
};


////////////////////////////////////////////////////////////////////////////////
// Link metric query TLV associated structures ("Section 6.4.10")
////////////////////////////////////////////////////////////////////////////////
struct linkMetricQueryTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_LINK_METRIC_QUERY. */

    #define LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS      (0x00)
    #define LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR  (0x01)
    uint8_t  destination;           // One of the values from above

    uint8_t  specific_neighbor[6];  // Only significant when the 'destination'
                                  // field is set to
                                  // 'LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR'

    #define LINK_METRIC_QUERY_TLV_TX_LINK_METRICS_ONLY         (0x00)
    #define LINK_METRIC_QUERY_TLV_RX_LINK_METRICS_ONLY         (0x01)
    #define LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS  (0x02)
    uint8_t  link_metrics_type;     // One of the values from above
};

struct linkMetricQueryTLV *linkMetricQueryTLVAllocAll(hlist_head *parent, uint8_t link_metrics_type);
struct linkMetricQueryTLV *linkMetricQueryTLVAllocSpecific(hlist_head *parent, mac_address neighbour,
                                                           uint8_t link_metrics_type);


////////////////////////////////////////////////////////////////////////////////
// Transmitter link metric TLV associated structures ("Section 6.4.11")
////////////////////////////////////////////////////////////////////////////////
struct _transmitterLinkMetricEntries
{
    uint8_t   local_interface_address[6];      // MAC address of an interface in
                                             // the receiving AL, which connects
                                             // to an interface in the neighbor
                                             // AL

    uint8_t   neighbor_interface_address[6];   // MAC addres of an interface in a
                                             // neighbor AL, which connects to
                                             // an interface in the receiving
                                             // AL

    uint16_t  intf_type;                // Underlaying network technology
                                      // One of the MEDIA_TYPE_* values.

    uint8_t   bridge_flag;              // Indicates whether or not the 1905 link
                                      // includes one or more IEEE 802.11
                                      // bridges

    uint32_t  packet_errors;            // Estimated number of lost packets on the
                                      // transmitting side of the link during
                                      // the measurement period (5 seconds??)

    uint32_t  transmitted_packets;      // Estimated number of packets transmitted
                                      // on the same measurement period used to
                                      // estimate 'packet_errors'

    uint16_t  mac_throughput_capacity;  // The maximum MAC throughput of the link
                                      // estimated at the transmitter and
                                      // expressed in Mb/s

    uint16_t  link_availability;        // The estimated average percentage of
                                      // time that the link is available for
                                      // data transmissions

    uint16_t  phy_rate;                 // This value is the PHY rate estimated at
                                      // the transmitter of the link expressed
                                      // in Mb/s
};
struct transmitterLinkMetricTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_TRANSMITTER_LINK_METRIC. */

    uint8_t  local_al_address[6];    // AL MAC address of the device that
                                   // transmits the response message that
                                   // contains this TLV

    uint8_t  neighbor_al_address[6]; // AL MAC address of the neighbor whose
                                   // link metric is reported in this TLV

    uint8_t                                  transmitter_link_metrics_nr;
    struct _transmitterLinkMetricEntries  *transmitter_link_metrics;
                                   // Link metric information for the above
                                   // interface pair between the receiving AL
                                   // and the neighbor AL
};


////////////////////////////////////////////////////////////////////////////////
// Receiver link metric TLV associated structures ("Section 6.4.12")
////////////////////////////////////////////////////////////////////////////////
struct  _receiverLinkMetricEntries
{
    uint8_t   local_interface_address[6];      // MAC address of an interface in
                                             // the receiving AL, which connects
                                             // to an interface in the neighbor
                                             // AL

    uint8_t   neighbor_interface_address[6];   // MAC addres of an interface in a
                                             // neighbor AL, which connects to
                                             // an interface in the receiving
                                             // AL

    uint16_t  intf_type;                // Underlaying network technology

    uint32_t  packet_errors;            // Estimated number of lost packets on the
                                      // receiving side of the link during
                                      // the measurement period (5 seconds??)

    uint32_t  packets_received;         // Estimated number of packets received on
                                      // the same measurement period used to
                                      // estimate 'packet_errors'

    uint8_t  rssi;                      // This value is the estimated RSSI at the
                                      // receive side of the link expressed in
                                      // dB
};
struct receiverLinkMetricTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_RECEIVER_LINK_METRIC. */

    uint8_t local_al_address[6];     // AL MAC address of the device that
                                   // transmits the response message that
                                   // contains this TLV

    uint8_t neighbor_al_address[6];  // AL MAC address of the neighbor whose
                                   // link metric is reported in this TLV

    uint8_t                               receiver_link_metrics_nr;
    struct _receiverLinkMetricEntries  *receiver_link_metrics;
                                   // Link metric information for the above
                                   // interface pair between the receiving AL
                                   // and the neighbor AL
};


////////////////////////////////////////////////////////////////////////////////
// Link metric result code TLV associated structures ("Section 6.4.13")
////////////////////////////////////////////////////////////////////////////////
struct linkMetricResultCodeTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_LINK_METRIC_RESULT_CODE. */

    #define LINK_METRIC_RESULT_CODE_TLV_INVALID_NEIGHBOR  (0x00)
    uint8_t   result_code;          // One of the values from above
};


////////////////////////////////////////////////////////////////////////////////
// Searched role TLV associated structures ("Section 6.4.14")
////////////////////////////////////////////////////////////////////////////////
struct searchedRoleTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_SEARCHED_ROLE. */

    uint8_t   role;                 // One of the values from IEEE80211_ROLE_*
};


////////////////////////////////////////////////////////////////////////////////
// Autoconfig frequency band TLV associated structures ("Section 6.4.15")
////////////////////////////////////////////////////////////////////////////////
struct autoconfigFreqBandTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_AUTOCONFIG_FREQ_BAND. */

    uint8_t   freq_band;            // Frequency band of the unconfigured
                                  // interface requesting an autoconfiguration.
                                  // Use one of the values in
                                  // IEEE80211_FREQUENCY_BAND_*
};


////////////////////////////////////////////////////////////////////////////////
// Supported role TLV associated structures ("Section 6.4.16")
////////////////////////////////////////////////////////////////////////////////
struct supportedRoleTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_SUPPORTED_ROLE. */

    uint8_t   role;                 // One of the values from IEEE80211_ROLE_*
};



////////////////////////////////////////////////////////////////////////////////
// Supported frequency band TLV associated structures ("Section 6.4.17")
////////////////////////////////////////////////////////////////////////////////
struct supportedFreqBandTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_SUPPORTED_FREQ_BAND. */

    uint8_t   freq_band;            // Frequency band of the unconfigured
                                  // interface requesting an autoconfiguration.
                                  // Use one of the values in
                                  // IEEE80211_FREQUENCY_BAND_*
};


////////////////////////////////////////////////////////////////////////////////
// Supported frequency band TLV associated structures ("Section 6.4.18")
////////////////////////////////////////////////////////////////////////////////
struct wscTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_WSC. */

    uint16_t  wsc_frame_size;
    uint8_t  *wsc_frame;            // Pointer to a buffer containing the M1 or
                                  // M2 message.
};


////////////////////////////////////////////////////////////////////////////////
// Push button event notification TLV associated structures ("Section 6.4.19")
////////////////////////////////////////////////////////////////////////////////
struct _mediaTypeEntries
{
    uint16_t  media_type;           // A media type for which a push button
                                  // configuration method has been activated on
                                  // the device that originates the push button
                                  // event notification
                                  // One of the MEDIA_TYPE_* values

    uint8_t   media_specific_data_size;  // Number of bytes in ensuing field

    union _mediaSpecificData media_specific_data;
                                  // Media specific data
                                  // It will contain a IEEE80211 structure
                                  // when 'media_type' is one of the valid
                                  // MEDIA_TYPE_IEEE_802_11* values
                                  // It will contain a IEE1905 structure
                                  // when 'media_type' is one of the valid
                                  // MEDIA_TYPE_IEEE_1901* values
                                  // It will be empty in the rest of cases
};
struct pushButtonEventNotificationTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION. */

    uint8_t   media_types_nr;       // Number of media types included in this
                                  // message: can be "0" or larger

    struct _mediaTypeEntries *media_types;
};


////////////////////////////////////////////////////////////////////////////////
// Push button join notification TLV associated structures ("Section 6.4.20")
////////////////////////////////////////////////////////////////////////////////
struct pushButtonJoinNotificationTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_AL_MAC_ADDRESS_TYPE. */

    uint8_t   al_mac_address[6];    // 1905 AL MAC address of the device that sent
                                  // the push button event notification message

    uint16_t  message_identifier;   // The message identifier (MID) of the push
                                  // button event notification message

    uint8_t   mac_address[6];       // Interface specific MAC address of the
                                  // interface of the transmitting device
                                  // belonging to the medium on which a new
                                  // device joined

    uint8_t   new_mac_address[6];   // Interface specific MAC address of the
                                  // interface of the new device that was joined
                                  // to the network as a result of the push
                                  // button configuration sequence
};


////////////////////////////////////////////////////////////////////////////////
// Generic PHY device information TLV associated structures ("Section 6.4.21")
////////////////////////////////////////////////////////////////////////////////
struct _genericPhyDeviceEntries
{
    uint8_t   local_interface_address[6];
                                      // MAC address of the local interface

    struct _genericPhyCommonData generic_phy_common_data;
                                      // This structure contains the OUI,
                                      // variant index and media specific
                                      // information of the local interface

    uint8_t   variant_name[32];         // Variant name UTF-8 string (NULL
                                      // terminated)

    uint8_t   generic_phy_description_xml_url_len;
    char   *generic_phy_description_xml_url;
                                      // URL to the "Generic Phy XML Description
                                      // Document" of the variant.
                                      // The string is
                                      // 'generic_phy_description_xml_url_len'
                                      // bytes long including the final NULL
                                      // character.

};
struct genericPhyDeviceInformationTypeTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION. */

    uint8_t   al_mac_address[6];        // 1905 AL MAC address of the device


    uint8_t                            local_interfaces_nr;
    struct _genericPhyDeviceEntries *local_interfaces;
                                      // List of local interfaces that are
                                      // going to be reported as
                                      // MEDIA_TYPE_UNKNOWN
};


////////////////////////////////////////////////////////////////////////////////
// Device identification type TLV associated structures ("Section 6.4.22")
////////////////////////////////////////////////////////////////////////////////
struct deviceIdentificationTypeTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_DEVICE_IDENTIFICATION. */

    char  friendly_name[64];         // Friendly name UTF-8 string (NULL
                                      // terminated)

    char  manufacturer_name[64];     // Manufacturer name UTF-8 string (NULL
                                      // terminated)

    char  manufacturer_model[64];    // Manufacturer modem UTF-8 string (NULL
                                      // terminated)

};


////////////////////////////////////////////////////////////////////////////////
// Control URL type TLV associated structures ("Section 6.4.23")
////////////////////////////////////////////////////////////////////////////////
struct controlUrlTypeTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_CONTROL_URL. */

    char   *url;                      // Pointer to a NULL terminated string
                                      // containing the URL to a control or
                                      // WebUI of the device
};


////////////////////////////////////////////////////////////////////////////////
// IPv4 type TLV associated structures ("Section 6.4.24")
////////////////////////////////////////////////////////////////////////////////
struct _ipv4Entries
{
    #define IPV4_TYPE_UNKNOWN (0)
    #define IPV4_TYPE_DHCP    (1)
    #define IPV4_TYPE_STATIC  (2)
    #define IPV4_TYPE_AUTOIP  (3)
    uint8_t type;                     // One of the values from above

    uint8_t ipv4_address[4];          // IPv4 address associated to the interface

    uint8_t ipv4_dhcp_server[4];      // IPv4 address of the DHCP server (if
                                    // known, otherwise set to all zeros)
};
struct _ipv4InterfaceEntries
{
    uint8_t   mac_address[6];          // MAC address of the interface whose IPv4s
                                     // are going to be reported.
                                     //
                                     //   NOTE: The standard says it can also
                                     //   be an AL MAC address instead of an
                                     //   interface MAC address.
                                     //   In that case I guess *all* IPv4s of
                                     //   the device (no matter the interface
                                     //   they are "binded" to) are reported.

    uint8_t                   ipv4_nr;
    struct  _ipv4Entries   *ipv4;    // List of IPv4s associated to this
                                     // interface
};
struct ipv4TypeTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_IPV4. */

    uint8_t                          ipv4_interfaces_nr;
    struct _ipv4InterfaceEntries  *ipv4_interfaces;
                                      // List of interfaces with at least one
                                      // IPv4 assigned
};


////////////////////////////////////////////////////////////////////////////////
// IPv6 type TLV associated structures ("Section 6.4.25")
////////////////////////////////////////////////////////////////////////////////
struct _ipv6Entries
{
    #define IPV6_TYPE_UNKNOWN (0)
    #define IPV6_TYPE_DHCP    (1)
    #define IPV6_TYPE_STATIC  (2)
    #define IPV6_TYPE_SLAAC   (3)
    uint8_t type;                     // One of the values from above

    uint8_t ipv6_address[16];         // IPv6 address associated to the interface

    uint8_t ipv6_address_origin[16];  // If type == IPV6_TYPE_DHCP, this field
                                    // contains the IPv6 address of the DHCPv6
                                    // server.
                                    // If type == IPV6_TYPE_SLAAC, this field
                                    // contains the IPv6 address of the router
                                    // that provided the SLAAC address.
                                    // In any other case this field is set to
                                    // all zeros.
};
struct _ipv6InterfaceEntries
{
    uint8_t   mac_address[6];          // MAC address of the interface whose IPv4s
                                     // are going to be reported.
                                     //
                                     //   NOTE: The standard says it can also
                                     //   be an AL MAC address instead of an
                                     //   interface MAC address.
                                     //   In that case I guess *all* IPv4s of
                                     //   the device (no matter the interface
                                     //   they are "binded" to) are reported.

    uint8_t  ipv6_link_local_address[16];
                                     // IPv6 link local address corresponding to
                                     // this interface

    uint8_t                   ipv6_nr;
    struct  _ipv6Entries   *ipv6;    // List of IPv4s associated to this
                                     // interface
};
struct ipv6TypeTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_IPV6. */

    uint8_t                          ipv6_interfaces_nr;
    struct _ipv6InterfaceEntries  *ipv6_interfaces;
                                      // List of interfaces with at least one
                                      // IPv6 assigned
};


////////////////////////////////////////////////////////////////////////////////
// Push button generic PHY event notification TLV associated structures
// ("Section 6.4.26")
////////////////////////////////////////////////////////////////////////////////
struct pushButtonGenericPhyEventNotificationTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION. */

    uint8_t                            local_interfaces_nr;
    struct _genericPhyCommonData    *local_interfaces;
                                   // List of local interfaces of type
                                   // MEDIA_TYPE_UNKNOWN for which a push button
                                   // configuration method has been activated on
                                   // the device that originates the push button
                                   // event notification
};


////////////////////////////////////////////////////////////////////////////////
// Profile version TLV associated structures ("Section 6.4.27")
////////////////////////////////////////////////////////////////////////////////
struct x1905ProfileVersionTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_1905_PROFILE_VERSION. */

    #define PROFILE_1905_1   (0x00)
    #define PROFILE_1905_1A  (0x01)
    uint8_t   profile;              // One of the values from above
};


////////////////////////////////////////////////////////////////////////////////
// Power off interface TLV associated structures ("Section 6.4.28")
////////////////////////////////////////////////////////////////////////////////
struct _powerOffInterfaceEntries
{
    uint8_t   interface_address[6];     // MAC address of an interface in the
                                      // "power off" state

    uint16_t  media_type;               // Underlaying network technology
                                      // One of the MEDIA_TYPE_* values

    struct _genericPhyCommonData generic_phy_common_data;
                                      // If 'media_type' is MEDIA_TYPE_UNKNOWN,
                                      // this structure contains the vendor OUI,
                                      // variant index and media specific
                                      // information of the interface
                                      // Otherwise, it is set to all zeros
};
struct powerOffInterfaceTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_POWER_OFF_INTERFACE. */

    uint8_t                              power_off_interfaces_nr;
    struct _powerOffInterfaceEntries  *power_off_interfaces;
                                   // List of local interfaces in the "power
                                   // off" state.
};


////////////////////////////////////////////////////////////////////////////////
// Interface power change information TLV associated structures ("Section
// 6.4.29")
////////////////////////////////////////////////////////////////////////////////
struct _powerChangeInformationEntries
{
    uint8_t   interface_address[6];     // MAC address of an interface in the
                                      // "power off" state

    #define POWER_STATE_REQUEST_OFF  (0x00)
    #define POWER_STATE_REQUEST_ON   (0x01)
    #define POWER_STATE_REQUEST_SAVE (0x02)
    uint8_t   requested_power_state;    // One of the values from above
};
struct interfacePowerChangeInformationTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION. */

    uint8_t                                   power_change_interfaces_nr;
    struct _powerChangeInformationEntries  *power_change_interfaces;
                                 // List of local interfaces for which a power
                                 // status change is requested
};


////////////////////////////////////////////////////////////////////////////////
// Interface power change status TLV associated structures ("Section 6.4.29")
////////////////////////////////////////////////////////////////////////////////
struct _powerChangeStatusEntries
{
    uint8_t   interface_address[6];     // MAC address of an interface in the
                                      // "power off" state

    #define POWER_STATE_RESULT_COMPLETED          (0x00)
    #define POWER_STATE_RESULT_NO_CHANGE          (0x01)
    #define POWER_STATE_RESULT_ALTERNATIVE_CHANGE (0x02)
    uint8_t   result;                   // One of the values from above
};
struct interfacePowerChangeStatusTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS. */

    uint8_t                              power_change_interfaces_nr;
    struct _powerChangeStatusEntries  *power_change_interfaces;
                                 // List of local interfaces whose power status
                                 // change operation result is being reported
};


////////////////////////////////////////////////////////////////////////////////
// L2 neighbor device TLV associated structures ("Section 6.4.31")
////////////////////////////////////////////////////////////////////////////////
struct _l2NeighborsEntries
{
    uint8_t   l2_neighbor_mac_address[6];     // MAC address of remote interface
                                            // sharing the same L2 medium

    uint16_t    behind_mac_addresses_nr;
    uint8_t   (*behind_mac_addresses)[6];     // List of MAC addresses the remote
                                            // device (owner of the remote
                                            // interface) "knows" and that are
                                            // not visible on this interface.
                                            // TODO: Define better !!!
};
struct _l2InterfacesEntries
{
    uint8_t   local_mac_address[6];    // MAC address of the local interface whose
                                     // L2 neighbors are going to be reported

    uint16_t                         l2_neighbors_nr;
    struct  _l2NeighborsEntries   *l2_neighbors;
                                     // List of neighbors that share the same L2
                                     // medium as the local interface
};
struct l2NeighborDeviceTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_L2_NEIGHBOR_DEVICE. */

    uint8_t                         local_interfaces_nr;
    struct _l2InterfacesEntries  *local_interfaces;
                                      // List of interfaces with at least one
                                      // IPv4 assigned
};

/** @brief EasyMesh SupportedService TLV.
 *
 * For the SearchedService TLV, we use the same structure and functions.
 * @{
 */

enum serviceType {
    SERVICE_MULTI_AP_CONTROLLER = 0x00U,
    SERVICE_MULTI_AP_AGENT = 0x01U,
};

struct _supportedService {
    struct tlv_struct s;
    uint8_t service; /* enum serviceType fits in a byte */
};

struct supportedServiceTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_SUPPORTED_SERVICE. */
};

struct supportedServiceTLV *supportedServiceTLVAlloc(hlist_head *parent, bool controller, bool agent);
struct supportedServiceTLV *searchedServiceTLVAlloc(hlist_head *parent, bool controller);

/** @} */

/** @brief EasyMesh AP Operational BSS TLV.
 *
 *  @{
 */
struct _apOperationalBssInfo {
    struct tlv_struct s;
    mac_address bssid; /**< @brief MAC Address of Local Interface (equal to BSSID) operating on the radio. */
    struct ssid ssid;  /**< @brief SSID of this BSS. */
};

struct _apOperationalBssRadio {
    struct tlv_struct s;
    mac_address radio_uid; /**< @brief Radio Unique Identifier of the radio. */
};

struct apOperationalBssTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_AP_OPERATIONAL_BSS. */
};

struct apOperationalBssTLV* apOperationalBssTLVAlloc(hlist_head *parent);
struct _apOperationalBssRadio *apOperationalBssTLVAddRadio(struct apOperationalBssTLV* a, mac_address radio_uid);
struct _apOperationalBssInfo *apOperationalBssRadioAddBss(struct _apOperationalBssRadio* a,
                                                          mac_address bssid, struct ssid ssid);

/** @} */

/** @brief EasyMesh Associated Clients TLV.
 *
 *  @{
 */
struct _associatedClientInfo {
    struct tlv_struct s;
    mac_address addr; /**< @brief The MAC address of the associated 802.11 client. */
#define ASSOCIATED_CLIENT_MAX_AGE 65535 /**< Saturation value of _associatedClientInfo::age. */
    uint16_t    age;  /**< @brief Time since the 802.11 client’s last association to this Multi-AP device, in seconds. */
};

struct _associatedClientsBssInfo {
    struct tlv_struct s;
    mac_address bssid; /**< @brief The BSSID of the BSS operated by the Multi-AP Agent in which the clients are
                        * associated. */
};

struct associatedClientsTLV
{
    struct tlv   tlv; /**< @brief TLV type, must always be set to TLV_TYPE_ASSOCIATED_CLIENTS. */
};

struct associatedClientsTLV* associatedClientsTLVAlloc(hlist_head *parent);
struct _associatedClientsBssInfo *associatedClientsTLVAddBssInfo (struct associatedClientsTLV* a, mac_address bssid);
struct _associatedClientInfo *associatedClientsTLVAddClientInfo (struct _associatedClientsBssInfo* a,
                                                                 mac_address addr, uint16_t age);

/** @} */

////////////////////////////////////////////////////////////////////////////////
// Main API functions
////////////////////////////////////////////////////////////////////////////////

// This function receives a pointer to a stream of bytes representing a 1905
// TLV according to "Section 6.4"
//
// It then returns a pointer to a structure whose fields have already been
// filled with the appropriate values extracted from the parsed stream.
//
// The actual type of the returned pointer structure depends on the value of
// the first byte pointed by "packet_stream" (ie. the "Type" field of the TLV).
//
// If an error was encountered while parsing the stream, a NULL pointer is
// returned instead.
// Otherwise, the returned structure is dynamically allocated, and once it is
// no longer needed, the user must call the "free_1905_TLV_structure()" function
//
struct tlv *parse_1905_TLV_from_packet(const uint8_t *packet_stream);


// This is the opposite of "parse_1905_TLV_from_packet()": it receives a
// pointer to a TLV structure and then returns a pointer to a buffer which:
//   - is a packet representation of the TLV
//   - has a length equal to the value returned in the "len" output argument
//
// "memory_structure" must point to a structure of one of the types returned by
// "parse_1905_TLV_from_packet()"
//
// If there is a problem this function returns NULL, otherwise the returned
// buffer must be later freed by the caller (it is a regular, non-nested buffer,
// so you just need to call "free()").
//
// Note that the input structure is *not* freed. You still need to later call
// "free_1905_TLV_structure()"
//
uint8_t *forge_1905_TLV_from_structure(const struct tlv *memory_structure, uint16_t *len);



////////////////////////////////////////////////////////////////////////////////
// Utility API functions
////////////////////////////////////////////////////////////////////////////////

// This function receives a pointer to a TLV structure and then traverses it
// and all nested structures, calling "free()" on each one of them
//
// "memory_structure" must point to a structure of one of the types returned by
// "parse_1905_TLV_from_packet()"
//
void free_1905_TLV_structure(struct tlv *tlv);


// 'forge_1905_TLV_from_structure()' returns a regular buffer which can be freed
// using this macro defined to be free
//
#define  free_1905_TLV_packet  free


// This function returns '0' if the two given pointers represent TLV structures
// of the same type and they contain the same data
//
// "memory_structure_1" and "memory_structure_2" must point (each) to a
// structure of one of the types returned by "parse_1905_TLV_from_packet()"
//
uint8_t compare_1905_TLV_structures(struct tlv *tlv_1, struct tlv *tlv_2);


// The next function is used to call function "callback()" on each element of
// the "memory_structure" structure
//
// "memory_structure" must point to a structure of one of the types returned by
// "parse_1905_TLV_from_packet()"
//
// It takes four arguments:
//   - The structure whose elements are going to be visited
//   - A callback function that will be executed on each element with the
//     following arguments:
//      * A pointer to the "write()" function that will be used to dump text.
//        This is always the "write_function()" pointer provided as third
//        argument to the "visit_1905_TLV_structure()" function.
//      * The size of the element to print (1, 2, 4, n bytes)
//      * A prefix string.
//        This is always the "prefix" value provided as fourth argument to the
//        "visit_1905_TLV_structure()" function
//      * The name of the element (ex: "mac_address")
//      * A 'fmt' string which must be used to print the contents of the element
//      * A pointer to the element itself
//   - The "write()" function that will be used when the callback is executed
//   - A "prefix" string argument that will be used when the callback is
//     executed (it usually contains "context" information that the callback
//     function prints before anything else to make it easy to follow the
//     structure traversing order)
//
void visit_1905_TLV_structure(struct tlv *tlv, visitor_callback callback, void (*write_function)(const char *fmt, ...), const char *prefix);


// Use this function for debug purposes. It turns a TLV_TYPE_* variable into its
// string representation.
//
// Example: TLV_TYPE_AL_MAC_ADDRESS_TYPE --> "TLV_TYPE_AL_MAC_ADDRESS_TYPE"
//
// Return "Unknown" if the provided type does not exist.
//
const char *convert_1905_TLV_type_to_string(uint8_t tlv_type);

#endif
