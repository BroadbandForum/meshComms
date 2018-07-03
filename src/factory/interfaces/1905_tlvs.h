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

#ifndef _1905_TLVS_H_
#define _1905_TLVS_H_

#include "platform.h"
#include <utils.h>

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

#define TLV_TYPE_LAST                                (30)
                                                     // NOTE: If new types are
                                                     // introduced in future
                                                     // revisions of the
                                                     // standard, update this
                                                     // value so that it always
                                                     // points to the last one.
                                                     // HOWEVER, it is used as
                                                     // a 32-bit bitmask so we
                                                     // can't actually add more
                                                     // types...

#define TLV_TYPE_SUPPORTED_SERVICE                   (0x80)



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
    INT8U  network_membership[6]; // BSSID

    #define IEEE80211_SPECIFIC_INFO_ROLE_AP                   (0x0)
    #define IEEE80211_SPECIFIC_INFO_ROLE_NON_AP_NON_PCP_STA   (0x4)
    #define IEEE80211_SPECIFIC_INFO_ROLE_WIFI_P2P_CLIENT      (0x8)
    #define IEEE80211_SPECIFIC_INFO_ROLE_WIFI_P2P_GROUP_OWNER (0x9)
    #define IEEE80211_SPECIFIC_INFO_ROLE_AD_PCP               (0xa)
    INT8U  role;                  // One of the values from above

    INT8U ap_channel_band;        // Hex value of dot11CurrentChannelBandwidth
                                  // (see "IEEE P802.11ac/D3.0" for description)

    INT8U ap_channel_center_frequency_index_1;
                                  // Hex value of
                                  // dot11CurrentChannelCenterFrequencyIndex1
                                  // (see "IEEE P802.11ac/D3.0" for description)

    INT8U ap_channel_center_frequency_index_2;
                                  // Hex value of
                                  // dot11CurrentChannelCenterFrequencyIndex2
                                  // (see "IEEE P802.11ac/D3.0" for description)
};
struct _ieee1901SpecificInformation
{
    INT8U network_identifier[7];  // Network membership
};
union _mediaSpecificData
{
    INT8U                                dummy;    // Empty placeholder
    struct _ieee80211SpecificInformation ieee80211;
    struct _ieee1901SpecificInformation  ieee1901;

};


////////////////////////////////////////////////////////////////////////////////
// Generic phy common structure used in "Tables 6.29, 6.36 and 6.38"
////////////////////////////////////////////////////////////////////////////////
struct _genericPhyCommonData
{
    INT8U   oui[3];                   // OUI of the generic phy networking
                                      // technology of the local interface

    INT8U   variant_index;            // Variant index of the generic phy
                                      // networking technology of the local
                                      // interface

    INT8U   media_specific_bytes_nr;
    INT8U  *media_specific_bytes;     // Media specific information of the
                                      // variant.
                                      // This field contains
                                      // "media_specific_bytes_nr" bytes.
};


////////////////////////////////////////////////////////////////////////////////
// End of message TLV associated structures ("Section 6.4.1")
////////////////////////////////////////////////////////////////////////////////
struct endOfMessageTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_END_OF_MESSAGE

    // This structure does not contain anything at all
};


////////////////////////////////////////////////////////////////////////////////
// Vendor specific TLV associated structures ("Section 6.4.2")
////////////////////////////////////////////////////////////////////////////////
struct vendorSpecificTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_VENDOR_SPECIFIC

    INT8U  vendorOUI[3];          // Vendor specific OUI, the value of the 24
                                  // bit globally unique IEEE-SA assigned number
                                  // to the vendor

    INT16U  m_nr;                 // Bytes in the following field
    INT8U  *m;                    // Vendor specific information
};


////////////////////////////////////////////////////////////////////////////////
// AL MAC address type TLV associated structures ("Section 6.4.3")
////////////////////////////////////////////////////////////////////////////////
struct alMacAddressTypeTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_AL_MAC_ADDRESS_TYPE

    INT8U   al_mac_address[6];    // 1905 AL MAC address of the transmitting
                                  // device
};


////////////////////////////////////////////////////////////////////////////////
// MAC address type TLV associated structures ("Section 6.4.4")
////////////////////////////////////////////////////////////////////////////////
struct macAddressTypeTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_MAC_ADDRESS_TYPE

    INT8U   mac_address[6];       // MAC address of the interface on which the
                                  // message is transmitted
};


////////////////////////////////////////////////////////////////////////////////
// Device information type TLV associated structures ("Section 6.4.5")
////////////////////////////////////////////////////////////////////////////////
struct _localInterfaceEntries
{
    INT8U   mac_address[6];       // MAC address of the local interface

    INT16U  media_type;           // One of the MEDIA_TYPE_* values

    INT8U   media_specific_data_size;
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
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_DEVICE_INFORMATION_TYPE

    INT8U   al_mac_address[6];    // 1905 AL MAC address of the device

    INT8U   local_interfaces_nr;  // Number of local interfaces

    struct _localInterfaceEntries  *local_interfaces;

};


////////////////////////////////////////////////////////////////////////////////
// Device bridging capability TLV associated structures ("Section 6.4.6")
////////////////////////////////////////////////////////////////////////////////
struct _bridgingTupleMacEntries
{
    INT8U   mac_address[6];       // MAC address of a 1905 device's network
                                  // interface that belongs to a bridging tuple
};
struct _bridgingTupleEntries
{
    INT8U   bridging_tuple_macs_nr; // Number of MAC addresses in this bridging
                                    // tuple

    struct  _bridgingTupleMacEntries  *bridging_tuple_macs;
                                   // List of 'mac_nr' elements, each one
                                   // representing a MAC. All these MACs are
                                   // bridged together.
};
struct deviceBridgingCapabilityTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES

    INT8U   bridging_tuples_nr;   // Number of MAC addresses in this bridging
                                  // tuple

    struct _bridgingTupleEntries  *bridging_tuples;

};


////////////////////////////////////////////////////////////////////////////////
// Non-1905 neighbor device list TLV associated structures ("Section 6.4.8")
////////////////////////////////////////////////////////////////////////////////
struct _non1905neighborEntries
{
    INT8U   mac_address[6];       // MAC address of the non-1905 device
};
struct non1905NeighborDeviceListTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST

    INT8U   local_mac_address[6]; // MAC address of the local interface

    INT8U                           non_1905_neighbors_nr;
    struct _non1905neighborEntries *non_1905_neighbors;
                                  // One entry for each non-1905 detected
                                  // neighbor
};


////////////////////////////////////////////////////////////////////////////////
// Neighbor device TLV associated structures ("Section 6.4.9")
////////////////////////////////////////////////////////////////////////////////
struct _neighborEntries
{
    INT8U   mac_address[6];       // AL MAC address of the 1905 neighbor

    INT8U   bridge_flag;          // "0" --> no IEEE 802.1 bridge exists
                                  // "1" --> at least one IEEE 802.1 bridge
                                  //         exists between this device and the
                                  //         neighbor
};
struct neighborDeviceListTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_NEIGHBOR_DEVICE_LIST

    INT8U   local_mac_address[6]; // MAC address of the local interface

    INT8U                     neighbors_nr;
    struct _neighborEntries  *neighbors;
                                  // One entry for each 1905 detected neighbor
};


////////////////////////////////////////////////////////////////////////////////
// Link metric query TLV associated structures ("Section 6.4.10")
////////////////////////////////////////////////////////////////////////////////
struct linkMetricQueryTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_LINK_METRIC_QUERY

    #define LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS      (0x00)
    #define LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR  (0x01)
    INT8U  destination;           // One of the values from above

    INT8U  specific_neighbor[6];  // Only significant when the 'destination'
                                  // field is set to
                                  // 'LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR'

    #define LINK_METRIC_QUERY_TLV_TX_LINK_METRICS_ONLY         (0x00)
    #define LINK_METRIC_QUERY_TLV_RX_LINK_METRICS_ONLY         (0x01)
    #define LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS  (0x02)
    INT8U  link_metrics_type;     // One of the values from above
};


////////////////////////////////////////////////////////////////////////////////
// Transmitter link metric TLV associated structures ("Section 6.4.11")
////////////////////////////////////////////////////////////////////////////////
struct _transmitterLinkMetricEntries
{
    INT8U   local_interface_address[6];      // MAC address of an interface in
                                             // the receiving AL, which connects
                                             // to an interface in the neighbor
                                             // AL

    INT8U   neighbor_interface_address[6];   // MAC addres of an interface in a
                                             // neighbor AL, which connects to
                                             // an interface in the receiving
                                             // AL

    INT16U  intf_type;                // Underlaying network technology
                                      // One of the MEDIA_TYPE_* values.

    INT8U   bridge_flag;              // Indicates whether or not the 1905 link
                                      // includes one or more IEEE 802.11
                                      // bridges

    INT32U  packet_errors;            // Estimated number of lost packets on the
                                      // transmitting side of the link during
                                      // the measurement period (5 seconds??)

    INT32U  transmitted_packets;      // Estimated number of packets transmitted
                                      // on the same measurement period used to
                                      // estimate 'packet_errors'

    INT16U  mac_throughput_capacity;  // The maximum MAC throughput of the link
                                      // estimated at the transmitter and
                                      // expressed in Mb/s

    INT16U  link_availability;        // The estimated average percentage of
                                      // time that the link is available for
                                      // data transmissions

    INT16U  phy_rate;                 // This value is the PHY rate estimated at
                                      // the transmitter of the link expressed
                                      // in Mb/s
};
struct transmitterLinkMetricTLV
{
    INT8U  tlv_type;               // Must always be set to
                                   // TLV_TYPE_TRANSMITTER_LINK_METRIC

    INT8U  local_al_address[6];    // AL MAC address of the device that
                                   // transmits the response message that
                                   // contains this TLV

    INT8U  neighbor_al_address[6]; // AL MAC address of the neighbor whose
                                   // link metric is reported in this TLV

    INT8U                                  transmitter_link_metrics_nr;
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
    INT8U   local_interface_address[6];      // MAC address of an interface in
                                             // the receiving AL, which connects
                                             // to an interface in the neighbor
                                             // AL

    INT8U   neighbor_interface_address[6];   // MAC addres of an interface in a
                                             // neighbor AL, which connects to
                                             // an interface in the receiving
                                             // AL

    INT16U  intf_type;                // Underlaying network technology

    INT32U  packet_errors;            // Estimated number of lost packets on the
                                      // receiving side of the link during
                                      // the measurement period (5 seconds??)

    INT32U  packets_received;         // Estimated number of packets received on
                                      // the same measurement period used to
                                      // estimate 'packet_errors'

    INT8U  rssi;                      // This value is the estimated RSSI at the
                                      // receive side of the link expressed in
                                      // dB
};
struct receiverLinkMetricTLV
{
    INT8U   tlv_type;              // Must always be set to
                                   // TLV_TYPE_RECEIVER_LINK_METRIC

    INT8U local_al_address[6];     // AL MAC address of the device that
                                   // transmits the response message that
                                   // contains this TLV

    INT8U neighbor_al_address[6];  // AL MAC address of the neighbor whose
                                   // link metric is reported in this TLV

    INT8U                               receiver_link_metrics_nr;
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
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_LINK_METRIC_RESULT_CODE

    #define LINK_METRIC_RESULT_CODE_TLV_INVALID_NEIGHBOR  (0x00)
    INT8U   result_code;          // One of the values from above
};


////////////////////////////////////////////////////////////////////////////////
// Searched role TLV associated structures ("Section 6.4.14")
////////////////////////////////////////////////////////////////////////////////
struct searchedRoleTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_SEARCHED_ROLE

    INT8U   role;                 // One of the values from IEEE80211_ROLE_*
};


////////////////////////////////////////////////////////////////////////////////
// Autoconfig frequency band TLV associated structures ("Section 6.4.15")
////////////////////////////////////////////////////////////////////////////////
struct autoconfigFreqBandTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_AUTOCONFIG_FREQ_BAND

    INT8U   freq_band;            // Frequency band of the unconfigured
                                  // interface requesting an autoconfiguration.
                                  // Use one of the values in
                                  // IEEE80211_FREQUENCY_BAND_*
};


////////////////////////////////////////////////////////////////////////////////
// Supported role TLV associated structures ("Section 6.4.16")
////////////////////////////////////////////////////////////////////////////////
struct supportedRoleTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_SUPPORTED_ROLE

    INT8U   role;                 // One of the values from IEEE80211_ROLE_*
};



////////////////////////////////////////////////////////////////////////////////
// Supported frequency band TLV associated structures ("Section 6.4.17")
////////////////////////////////////////////////////////////////////////////////
struct supportedFreqBandTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_SUPPORTED_FREQ_BAND

    INT8U   freq_band;            // Frequency band of the unconfigured
                                  // interface requesting an autoconfiguration.
                                  // Use one of the values in
                                  // IEEE80211_FREQUENCY_BAND_*
};


////////////////////////////////////////////////////////////////////////////////
// Supported frequency band TLV associated structures ("Section 6.4.18")
////////////////////////////////////////////////////////////////////////////////
struct wscTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_WSC

    INT16U  wsc_frame_size;
    INT8U  *wsc_frame;            // Pointer to a buffer containing the M1 or
                                  // M2 message.
};


////////////////////////////////////////////////////////////////////////////////
// Push button event notification TLV associated structures ("Section 6.4.19")
////////////////////////////////////////////////////////////////////////////////
struct _mediaTypeEntries
{
    INT16U  media_type;           // A media type for which a push button
                                  // configuration method has been activated on
                                  // the device that originates the push button
                                  // event notification
                                  // One of the MEDIA_TYPE_* values

    INT8U   media_specific_data_size;  // Number of bytes in ensuing field

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
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION

    INT8U   media_types_nr;       // Number of media types included in this
                                  // message: can be "0" or larger

    struct _mediaTypeEntries *media_types;
};


////////////////////////////////////////////////////////////////////////////////
// Push button join notification TLV associated structures ("Section 6.4.20")
////////////////////////////////////////////////////////////////////////////////
struct pushButtonJoinNotificationTLV
{
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_AL_MAC_ADDRESS_TYPE

    INT8U   al_mac_address[6];    // 1905 AL MAC address of the device that sent
                                  // the push button event notification message

    INT16U  message_identifier;   // The message identifier (MID) of the push
                                  // button event notification message

    INT8U   mac_address[6];       // Interface specific MAC address of the
                                  // interface of the transmitting device
                                  // belonging to the medium on which a new
                                  // device joined

    INT8U   new_mac_address[6];   // Interface specific MAC address of the
                                  // interface of the new device that was joined
                                  // to the network as a result of the push
                                  // button configuration sequence
};


////////////////////////////////////////////////////////////////////////////////
// Generic PHY device information TLV associated structures ("Section 6.4.21")
////////////////////////////////////////////////////////////////////////////////
struct _genericPhyDeviceEntries
{
    INT8U   local_interface_address[6];
                                      // MAC address of the local interface

    struct _genericPhyCommonData generic_phy_common_data;
                                      // This structure contains the OUI,
                                      // variant index and media specific
                                      // information of the local interface

    INT8U   variant_name[32];         // Variant name UTF-8 string (NULL
                                      // terminated)

    INT8U   generic_phy_description_xml_url_len;
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
    INT8U  tlv_type;                  // Must always be set to
                                      // TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION

    INT8U   al_mac_address[6];        // 1905 AL MAC address of the device


    INT8U                            local_interfaces_nr;
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
    INT8U  tlv_type;                  // Must always be set to
                                      // TLV_TYPE_DEVICE_IDENTIFICATION

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
    INT8U  tlv_type;                  // Must always be set to
                                      // TLV_TYPE_CONTROL_URL

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
    INT8U type;                     // One of the values from above

    INT8U ipv4_address[4];          // IPv4 address associated to the interface

    INT8U ipv4_dhcp_server[4];      // IPv4 address of the DHCP server (if
                                    // known, otherwise set to all zeros)
};
struct _ipv4InterfaceEntries
{
    INT8U   mac_address[6];          // MAC address of the interface whose IPv4s
                                     // are going to be reported.
                                     //
                                     //   NOTE: The standard says it can also
                                     //   be an AL MAC address instead of an
                                     //   interface MAC address.
                                     //   In that case I guess *all* IPv4s of
                                     //   the device (no matter the interface
                                     //   they are "binded" to) are reported.

    INT8U                   ipv4_nr;
    struct  _ipv4Entries   *ipv4;    // List of IPv4s associated to this
                                     // interface
};
struct ipv4TypeTLV
{
    INT8U  tlv_type;                  // Must always be set to
                                      // TLV_TYPE_IPV4

    INT8U                          ipv4_interfaces_nr;
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
    INT8U type;                     // One of the values from above

    INT8U ipv6_address[16];         // IPv6 address associated to the interface

    INT8U ipv6_address_origin[16];  // If type == IPV6_TYPE_DHCP, this field
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
    INT8U   mac_address[6];          // MAC address of the interface whose IPv4s
                                     // are going to be reported.
                                     //
                                     //   NOTE: The standard says it can also
                                     //   be an AL MAC address instead of an
                                     //   interface MAC address.
                                     //   In that case I guess *all* IPv4s of
                                     //   the device (no matter the interface
                                     //   they are "binded" to) are reported.

    INT8U  ipv6_link_local_address[16];
                                     // IPv6 link local address corresponding to
                                     // this interface

    INT8U                   ipv6_nr;
    struct  _ipv6Entries   *ipv6;    // List of IPv4s associated to this
                                     // interface
};
struct ipv6TypeTLV
{
    INT8U  tlv_type;                  // Must always be set to
                                      // TLV_TYPE_IPV6

    INT8U                          ipv6_interfaces_nr;
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
    INT8U  tlv_type;               // Must always be set to
                                   // TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION

    INT8U                            local_interfaces_nr;
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
    INT8U   tlv_type;             // Must always be set to
                                  // TLV_TYPE_1905_PROFILE_VERSION

    #define PROFILE_1905_1   (0x00)
    #define PROFILE_1905_1A  (0x01)
    INT8U   profile;              // One of the values from above
};


////////////////////////////////////////////////////////////////////////////////
// Power off interface TLV associated structures ("Section 6.4.28")
////////////////////////////////////////////////////////////////////////////////
struct _powerOffInterfaceEntries
{
    INT8U   interface_address[6];     // MAC address of an interface in the
                                      // "power off" state

    INT16U  media_type;               // Underlaying network technology
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
    INT8U  tlv_type;               // Must always be set to
                                   // TLV_TYPE_POWER_OFF_INTERFACE

    INT8U                              power_off_interfaces_nr;
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
    INT8U   interface_address[6];     // MAC address of an interface in the
                                      // "power off" state

    #define POWER_STATE_REQUEST_OFF  (0x00)
    #define POWER_STATE_REQUEST_ON   (0x01)
    #define POWER_STATE_REQUEST_SAVE (0x02)
    INT8U   requested_power_state;    // One of the values from above
};
struct interfacePowerChangeInformationTLV
{
    INT8U  tlv_type;             // Must always be set to
                                 // TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION

    INT8U                                   power_change_interfaces_nr;
    struct _powerChangeInformationEntries  *power_change_interfaces;
                                 // List of local interfaces for which a power
                                 // status change is requested
};


////////////////////////////////////////////////////////////////////////////////
// Interface power change status TLV associated structures ("Section 6.4.29")
////////////////////////////////////////////////////////////////////////////////
struct _powerChangeStatusEntries
{
    INT8U   interface_address[6];     // MAC address of an interface in the
                                      // "power off" state

    #define POWER_STATE_RESULT_COMPLETED          (0x00)
    #define POWER_STATE_RESULT_NO_CHANGE          (0x01)
    #define POWER_STATE_RESULT_ALTERNATIVE_CHANGE (0x02)
    INT8U   result;                   // One of the values from above
};
struct interfacePowerChangeStatusTLV
{
    INT8U  tlv_type;             // Must always be set to
                                 // TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS

    INT8U                              power_change_interfaces_nr;
    struct _powerChangeStatusEntries  *power_change_interfaces;
                                 // List of local interfaces whose power status
                                 // change operation result is being reported
};


////////////////////////////////////////////////////////////////////////////////
// L2 neighbor device TLV associated structures ("Section 6.4.31")
////////////////////////////////////////////////////////////////////////////////
struct _l2NeighborsEntries
{
    INT8U   l2_neighbor_mac_address[6];     // MAC address of remote interface
                                            // sharing the same L2 medium

    INT16U    behind_mac_addresses_nr;
    INT8U   (*behind_mac_addresses)[6];     // List of MAC addresses the remote
                                            // device (owner of the remote
                                            // interface) "knows" and that are
                                            // not visible on this interface.
                                            // TODO: Define better !!!
};
struct _l2InterfacesEntries
{
    INT8U   local_mac_address[6];    // MAC address of the local interface whose
                                     // L2 neighbors are going to be reported

    INT16U                         l2_neighbors_nr;
    struct  _l2NeighborsEntries   *l2_neighbors;
                                     // List of neighbors that share the same L2
                                     // medium as the local interface
};
struct l2NeighborDeviceTLV
{
    INT8U  tlv_type;                  // Must always be set to
                                      // TLV_TYPE_L2_NEIGHBOR_DEVICE

    INT8U                         local_interfaces_nr;
    struct _l2InterfacesEntries  *local_interfaces;
                                      // List of interfaces with at least one
                                      // IPv4 assigned
};

/** @brief EasyMesh SupportedService TLV.
 * @{
 */

enum serviceType {
    SERVICE_MULTI_AP_CONTROLLER = 0x00U,
    SERVICE_MULTI_AP_AGENT = 0x01U,
};

struct supportedServiceTLV
{
    INT8U  tlv_type; /**< @brief TLV type, must always be set to TLV_TYPE_SUPPORTED_SERVICE. */
    INT8U  supported_service_nr; /**< @brief Number of supported_service. */
    enum serviceType supported_service[]; /**< @brief List of supported services. */
};

/** @} */

////////////////////////////////////////////////////////////////////////////////
// Main API functions
////////////////////////////////////////////////////////////////////////////////

// This function receives a pointer to a stream of bytes representing a 1905
// TLV according to "Section 6.4"
//
// It then returns a pointer to a structure whose fields have already been
// filled with the appropiate values extracted from the parsed stream.
//
// The actual type of the returned pointer structure depends on the value of
// the first byte pointed by "packet_stream" (ie. the "Type" field of the TLV):
//
//   TLV_TYPE_END_OF_MESSAGE                  -->  struct endOfMessageTLV *
//   TLV_TYPE_VENDOR_SPECIFIC                 -->  struct vendorSpecificTLV *
//   TLV_TYPE_AL_MAC_ADDRESS_TYPE             -->  struct alMacAddressTypeTLV *
//   TLV_TYPE_MAC_ADDRESS_TYPE                -->  struct macAddressTypeTLV *
//   TLV_TYPE_DEVICE_INFORMATION_TYPE         -->  struct deviceInformationTypeTLV *
//   TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES    -->  struct deviceBridgingCapabilityTLV *
//   TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST   -->  struct non1905NeighborDeviceListTLV *
//   TLV_TYPE_NEIGHBOR_DEVICE_LIST            -->  struct neighborDeviceListTLV *
//   TLV_TYPE_LINK_METRIC_QUERY               -->  struct linkMetricQueryTLV *
//   TLV_TYPE_TRANSMITTER_LINK_METRIC         -->  struct transmitterLinkMetricTLV *
//   TLV_TYPE_RECEIVER_LINK_METRIC            -->  struct receiverLinkMetricTLV *
//   TLV_TYPE_LINK_METRIC_RESULT_CODE         -->  struct linkMetricResultCodeTLV *
//   TLV_TYPE_SEARCHED_ROLE                   -->  struct searchedRoleTLV *
//   TLV_TYPE_AUTOCONFIG_FREQ_BAND            -->  struct autoconfigFreqBandTLV *
//   TLV_TYPE_SUPPORTED_ROLE                  -->  struct supportedRoleTLV *
//   TLV_TYPE_SUPPORTED_FREQ_BAND             -->  struct supportedFreqBandTLV *
//   TLV_TYPE_WSC                             -->  struct wscTLV *
//   TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION  -->  struct pushButtonEventNotificationTLV *
//   TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION   -->  struct pushButtonJoinNotificationTLV *
//
// If an error was encountered while parsing the stream, a NULL pointer is
// returned instead.
// Otherwise, the returned structure is dynamically allocated, and once it is
// no longer needed, the user must call the "free_1905_TLV_structure()" function
//
INT8U *parse_1905_TLV_from_packet(INT8U *packet_stream);


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
INT8U *forge_1905_TLV_from_structure(INT8U *memory_structure, INT16U *len);



////////////////////////////////////////////////////////////////////////////////
// Utility API functions
////////////////////////////////////////////////////////////////////////////////

// This function receives a pointer to a TLV structure and then traverses it
// and all nested structures, calling "free()" on each one of them
//
// "memory_structure" must point to a structure of one of the types returned by
// "parse_1905_TLV_from_packet()"
//
void free_1905_TLV_structure(INT8U *memory_structure);


// 'forge_1905_TLV_from_structure()' returns a regular buffer which can be freed
// using this macro defined to be PLATFORM_FREE
//
#define  free_1905_TLV_packet  PLATFORM_FREE


// This function returns '0' if the two given pointers represent TLV structures
// of the same type and they contain the same data
//
// "memory_structure_1" and "memory_structure_2" must point (each) to a
// structure of one of the types returned by "parse_1905_TLV_from_packet()"
//
INT8U compare_1905_TLV_structures(INT8U *memory_structure_1, INT8U *memory_structure_2);


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
void visit_1905_TLV_structure(INT8U *memory_structure, visitor_callback callback, void (*write_function)(const char *fmt, ...), const char *prefix);


// Use this function for debug purposes. It turns a TLV_TYPE_* variable into its
// string representation.
//
// Example: TLV_TYPE_AL_MAC_ADDRESS_TYPE --> "TLV_TYPE_AL_MAC_ADDRESS_TYPE"
//
// Return "Unknown" if the provided type does not exist.
//
char *convert_1905_TLV_type_to_string(INT8U tlv_type);

#endif
