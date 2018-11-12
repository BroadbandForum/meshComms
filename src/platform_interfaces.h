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

#ifndef _PLATFORM_INTERFACES_H_
#define _PLATFORM_INTERFACES_H_

#include "media_specific_blobs.h"  // struct genericInterfaceType


////////////////////////////////////////////////////////////////////////////////
// Interfaces info
////////////////////////////////////////////////////////////////////////////////

struct interfaceInfo
{
    char  *name;           // Example: "eth0"

    uint8_t mac_address[6];  // 6  bytes long MAC address of the interface.


    char manufacturer_name[64]; // Marvell
    char model_name       [64]; // HiEndWifi
    char model_number     [64]; // 00000001
    char serial_number    [64]; // 0123456
    char device_name      [64]; // "HiEnd wifi"
    char uuid             [64];
                           // ID information (NULL terminated strings)

    #define INTERFACE_TYPE_IEEE_802_3U_FAST_ETHERNET       (0x0000)
    #define INTERFACE_TYPE_IEEE_802_3AB_GIGABIT_ETHERNET   (0x0001)
    #define INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ            (0x0100)
    #define INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ            (0x0101)
    #define INTERFACE_TYPE_IEEE_802_11A_5_GHZ              (0x0102)
    #define INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ            (0x0103)
    #define INTERFACE_TYPE_IEEE_802_11N_5_GHZ              (0x0104)
    #define INTERFACE_TYPE_IEEE_802_11AC_5_GHZ             (0x0105)
    #define INTERFACE_TYPE_IEEE_802_11AD_60_GHZ            (0x0106)
    #define INTERFACE_TYPE_IEEE_802_11AF_GHZ               (0x0107)
    #define INTERFACE_TYPE_IEEE_1901_WAVELET               (0x0200)
    #define INTERFACE_TYPE_IEEE_1901_FFT                   (0x0201)
    #define INTERFACE_TYPE_MOCA_V1_1                       (0x0300)
    #define INTERFACE_TYPE_UNKNOWN                         (0xFFFF)
    uint16_t interface_type;  // Indicates the MAC/PHY type of the underlying
                            // network technology.
                            // Valid values: any "INTERFACE_TYPE_*" value.
                            // If your interface is of a type not listed here,
                            // set it to "INTERFACE_TYPE_UNKNOWN" and then
                            // use the "interface_type_data.other" field to
                            // further identify it.

    union _interfaceTypeData
    {
        // Only to be filled when interface_type = INTERFACE_TYPE_IEEE_802_11*
        //
        struct _ieee80211Data
        {
            uint8_t  bssid[6];        // This is the BSSID (MAC address of the
                                    // registrar AP on a wifi network).
                                    // On unconfigured nodes (ie. STAs which
                                    // have not yet joined a network or non-
                                    // registrar APs which have not yet cloned
                                    // the credentiales from the registrar) this
                                    // parameter must be set to all zeros.

            char   ssid[50];        // This is the "friendly" name of the wifi
                                    // network created by the registrar AP
                                    // identified by 'bssid'

            #define IEEE80211_ROLE_AP                   (0x0)
            #define IEEE80211_ROLE_NON_AP_NON_PCP_STA   (0x4)
            #define IEEE80211_ROLE_WIFI_P2P_CLIENT      (0x8)
            #define IEEE80211_ROLE_WIFI_P2P_GROUP_OWNER (0x9)
            #define IEEE80211_ROLE_AD_PCP               (0xa)
            uint8_t  role;            // One of the values from above

            uint8_t ap_channel_band;  // Hex value of dot11CurrentChannelBandwidth
                                    // (see "IEEE P802.11ac/D3.0" for
                                    // description)

            uint8_t ap_channel_center_frequency_index_1;
                                    // Hex value of
                                    // dot11CurrentChannelCenterFrequencyIndex1
                                    // (see "IEEE P802.11ac/D3.0" for
                                    // description)

            uint8_t ap_channel_center_frequency_index_2;
                                    // Hex value of
                                    // dot11CurrentChannelCenterFrequencyIndex2
                                    // (see "IEEE P802.11ac/D3.0" for
                                    // description)

            #define IEEE80211_AUTH_MODE_OPEN    (0x0001)
            #define IEEE80211_AUTH_MODE_WPA     (0x0002)
            #define IEEE80211_AUTH_MODE_WPAPSK  (0x0004)
            #define IEEE80211_AUTH_MODE_WPA2    (0x0008)
            #define IEEE80211_AUTH_MODE_WPA2PSK (0x0010)
            uint16_t authentication_mode;
                                    // For APs: list of supported modes that
                                    // clients can use (OR'ed list of flags)
                                    // For STAs: current mode being used with
                                    // its AP (a single flag)

            #define IEEE80211_ENCRYPTION_MODE_NONE (0x0001)
            #define IEEE80211_ENCRYPTION_MODE_TKIP (0x0002)
            #define IEEE80211_ENCRYPTION_MODE_AES  (0x0004)
            uint16_t encryption_mode;
                                    // For APs: list of supported modes that
                                    // clients can use (OR'ed list of flags)
                                    // For STAs: current mode being used with
                                    // its AP (a single flag)

            char  network_key[64];  // Key that grants access to the AP network

        } ieee80211;

        // Only to be filled when interface_type = INTERFACE_TYPE_IEEE_1901*
        //
        struct _ieee1901Data
        {
            char network_identifier[7];  // Network membership

        } ieee1901;

        // Only to be filled when interface_type = INTERFACE_TYPE_UNKNOWN
        //
        struct genericInterfaceType other;

    } interface_type_data; // Depending on the value of "interface_type", one
                           // (and only one!) of the structures of this union
                           // must be filled

    uint8_t is_secured;      // Contains "1" if the interface is secure, "0"
                           // otherwise.
                           //
                           // Note that "secure" in this context means that the
                           // interface can be trusted to send private (in a
                           // "local network" way) messages.
                           //
                           // For example:
                           //
                           //   1. A "wifi" interface can only be considered
                           //      "secure" if encryption is on (WPA, WPA2,
                           //      etc...)
                           //
                           //   2. A G.hn/1901 interface can only be considered
                           //      "secure" if some one else's untrusted device
                           //      can not "sniff" your traffic.  This typically
                           //      means either encryption or some other
                           //      technology dependent "trick" (ex: "network
                           //      id") is enabled.
                           //
                           //   3. An ethernet interface can probably be always
                           //      be considered "secure" (but this is let for
                           //      the implementer to decide)
                           //
                           // One interface becomes "secured" when it contains
                           // at least one link which is "secured".
                           // For example, a wifi AP interface is considered
                           // "secured" if there is at least one STA connected
                           // to it by means of an encrypted channel.

    uint8_t push_button_on_going;
                           // Some types of interfaces support a technology-
                           // specific "push button" configuration mechanism
                           // (ex: "802.11", "G.hn"). Others don't (ex: "eth").
                           //
                           // This value is set to any of these possible values:
                           //
                           //   - "0" if the interface type supports this "push
                           //     button" configuration mechanism but, right
                           //     now, this process is not running.
                           //
                           //   - "1" if the interface type supports this "push
                           //     button" configuration mechanism and, right
                           //     now, we are in the middle of such process.
                           //
                           //   - "2" if the interface does not support the
                           //     "push button" configuration mechanism.

    uint8_t push_button_new_mac_address[6];
                           // 6  bytes long MAC address of the device that has
                           // just joined the network as a result of a "push
                           // button configuration" process (ie., just after
                           // "push_button_on_going" changes from "1" to "0")
                           // This field is set to all zeros when either:
                           //
                           //   A) WE are the device joining the network
                           //
                           //   B) No new device entered the network
                           //
                           //   C) The underlying technology does not offer this
                           //      information

    #define INTERFACE_POWER_STATE_ON     (0x00)
    #define INTERFACE_POWER_STATE_SAVE   (0x01)
    #define INTERFACE_POWER_STATE_OFF    (0x02)
    uint8_t power_state;     // Contains one of the INTERFACE_POWER_STATE_* values
                           // from above

    #define INTERFACE_NEIGHBORS_UNKNOWN (0xFF)
    uint8_t  neighbor_mac_addresses_nr;
                             // Number of other MAC addresses (pertaining -or
                             // not- to 1905 devices) this interface has
                             // received packets from in the past (not
                             // necessarily from the time the interface was
                             // brought up, but a reasonable amount of time)
                             // A special value of "INTERFACE_NEIGHBORS_UNKNOWN"
                             // is used to indicate that this interface has no
                             // way of obtaining this information (note that
                             // this is different from "0" which means "I know I
                             // have zero neighbors")

    uint8_t  (*neighbor_mac_addresses)[6];
                             // List containing those MAC addreses just
                             // described in the comment above.

    uint8_t  ipv4_nr;          // Number of IPv4 this device responds to
    struct _ipv4
    {
        #define IPV4_UNKNOWN (0)
        #define IPV4_DHCP    (1)
        #define IPV4_STATIC  (2)
        #define IPV4_AUTOIP  (3)
        uint8_t type;          // One of the values from above

        uint8_t address[4];    // IPv4 address

        uint8_t dhcp_server[4];// If the ip was obtained by DHCP, this
                             // variable holds the IPv4 of the server
                             // (if known). Set to all zeros otherwise

    } *ipv4;                 // Array of 'ipv4_nr' elements. Each
                             // element represents one of the IPv4 of
                             // this device.

    uint8_t  ipv6_nr;          // Number of IPv6 this device responds to
    struct _ipv6
    {
        #define IPV6_UNKNOWN (0)
        #define IPV6_DHCP    (1)
        #define IPV6_STATIC  (2)
        #define IPV6_SLAAC   (3)
        uint8_t type;          // One of the values from above

        uint8_t address[16];   // IPv6 address

        uint8_t origin[16];    // If type == IPV6_TYPE_DHCP, this field
                             // contains the IPv6 address of the DHCPv6 server.
                             // If type == IPV6_TYPE_SLAAC, this field contains
                             // the IPv6 address of the router that provided
                             // the SLAAC address.
                             // In any other case this field is set to all
                             // zeros.

    } *ipv6;                 // Array of 'ipv6_nr' elements. Each
                             // element represents one of the IPv6 of
                             // this device.

    uint8_t  vendor_specific_elements_nr;
                           // Number of items in the "vendor_specific_elements"
                           // array

    struct _vendorSpecificInfoElement
    {
        uint8_t    oui[3];    // 24 bits globally unique IEEE-RA assigned
                            // number to the vendor

        uint16_t   vendor_data_len;  // Number of bytes in "vendor_data"
        uint8_t   *vendor_data;      // Vendor specific data
    } *vendor_specific_elements;
};

// Return a list of strings (each one representing an "interface name", such
// as "eth0", "eth1", etc...).
//
// The length of the list is returned in the 'nr' output argument.
//
// If something goes wrong, return NULL and set the contents of 'nr' to '0'
//
// Each element of the list represents an interface on the localhost that will
// participate in the 1905 network.
//
// The 'name' field is a platform-specific NULL terminated string that will
// later be used in other functions to refer to this particular interface.
//
// The returned list must not be modified by the caller.
//
// When the returned list is no longer needed, it can be freed by calling
// "free_LIST_OF_1905_INTERFACES()"
//
// [PLATFORM PORTING NOTE]
//   Typically you want to return as many entries as physical interfaces there
//   are in the platform. However, if for some reason you want to make one or
//   more interfaces "invisible" to 1905 (maybe because they are "debug"
//   interfaces, such as a "management" ethernet port) you can return a reduced
//   list of interfaces.
//
char **PLATFORM_GET_LIST_OF_1905_INTERFACES(uint8_t *nr);

// Used to free the pointer returned by a previous call to
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()"
//
// 'nr' is the same one returned by "PLATFORM_GET_LIST_OF_1905_INTERFACES()"
//
void free_LIST_OF_1905_INTERFACES(char **x, uint8_t nr);

// Return a "struct interfaceInfo" structure containing all kinds of information
// associated to the provided 'interface_name'
//
// If something goes wrong, return NULL.
//
// 'interface_name' is one of the names previously returned in a call to
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()"
//
// The documentation of the "struct interfaceInfo" structure explain what each
// field of this structure should contain.
//
// Once the caller is done with the returned structure, hw must call
// "free_1905_STRUCTURE()" to dispose it
//
struct interfaceInfo *PLATFORM_GET_1905_INTERFACE_INFO(char *interface_name);

// Free the memory used by a "struct interfaceInfo" structure previously
// obtained by calling "PLATFORM_GET_1905_INTERFACE_INFO()"
//
void free_1905_INTERFACE_INFO(struct interfaceInfo *i);

/** @brief Populate the data model with the local interfaces. */
void createLocalInterfaces(void);

////////////////////////////////////////////////////////////////////////////////
// Link metrics
////////////////////////////////////////////////////////////////////////////////

struct linkMetrics
{
    uint8_t   local_interface_address[6];     // A MAC address belonging to one of
                                            // the local interfaces.
                                            // Let's call this MAC "A"

    uint8_t   neighbor_interface_address[6];  // A MAC address belonging to a
                                            // neighbor interface that is
                                            // directly reachable from "A".
                                            // Let's call this MAC "B".

    uint32_t  measures_window;   // Time in seconds representing how far back in
                               // time statistics have been being recorded for
                               // this interface.
                               // For example, if this value is set to "5" and
                               // 'tx_packet_ok' is set to "7", it means that
                               // in the last 5 seconds 7 packets have been
                               // transmitted OK between "A" and "B".
                               //
                               // [PLATFORM PORTING NOTE]
                               //   This is typically the amount of time
                               //   ellapsed since the interface was brought
                               //   up.

    uint32_t  tx_packet_ok;      // Estimated number of transmitted packets from
                               // "A" to "B" in the last 'measures_window'
                               // seconds.

    uint32_t  tx_packet_errors;  // Estimated number of packets with errors
                               // transmitted from "A" to "B" in the last
                               // 'measures_window' seconds.

    uint16_t  tx_max_xput;       // Extimated maximum MAC throughput from "A" to
                               // "B" in Mbits/s.

    uint16_t  tx_phy_rate;       // Extimated PHY rate from "A" to "B" in Mbits/s.

    uint16_t  tx_link_availability;
                               // Estimated average percentage of time that the
                               // link is available to transmit data from "A"
                               // to "B" in the last 'measures_window' seconds.

    uint32_t  rx_packet_ok;      // Estimated number of transmitted packets from
                               // "B" to "A" in the last 'measures_window'
                               // seconds.

    uint32_t  rx_packet_errors;  // Estimated number of packets with errors
                               // transmitted from "B" to "A" i nthe last
                               // 'measures_window' seconds.

    uint8_t   rx_rssi;           // Estimated RSSI when receiving data from "B" to
                               // "A" in dB.
};

// Return a "struct linkMetrics" structure containing all kinds of information
// associated to the link that exists between the provided local interface and
// neighbor's interface whose MAC address is 'neighbor_interface_address'.
//
// If something goes wrong, return NULL.
//
// 'local_interface_name' is one of the names previously returned in a call to
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()"
//
// 'neighbor_interface_address' is the MAC address at the other end of the link.
// (This MAC address belong to a neighbor's interface)
//
// The documentation of the "struct linkMetrics" structure explain what each
// field of this structure should contain.
//
// Once the caller is done with the returned structure, hw must call
// "free_LINK_METRICS()" to dispose it
//
// [PLATFORM PORTING NOTE]
//   You will notice how each 'struct linkMetrics' is associated to a LINK and
//   not to an interface.
//   In some cases, the platform might not be able to keep PER LINK stats.
//   For example, in Linux is easy to check how many packets were received by
//   "eth0" *in total*, but it is not trivial to find out how many packets were
//   received by "eth0" *from each neighbor*.
//   In these cases there are two solutions:
//     1. Add new platform code to make this PER LINK reporting possible (for
//        example, in Linux you would have to create iptables rules among other
//        things)
//     2. Just report the overwall PER INTERFACE stats (thus ignoring the
//        'neighbor_interface_address' parameter).
//        This is better than reporting nothing at all.
//
struct linkMetrics *PLATFORM_GET_LINK_METRICS(char *local_interface_name, uint8_t *neighbor_interface_address);

// Free the memory used by a "struct linkMetrics" structure previously
// obtained by calling "PLATFORM_GET_LINK_METRICS()"
//
void free_LINK_METRICS(struct linkMetrics *l);

////////////////////////////////////////////////////////////////////////////////
// Bridges info
////////////////////////////////////////////////////////////////////////////////

struct bridge
{
    char   *name;           // Example: "br0"

    uint8_t   bridged_interfaces_nr;
    char   *bridged_interfaces[10];
                            // Names of the interfaces (such as "eth0") that
                            // belong to this bridge

    uint8_t   forwarding_rules_nr;
    struct _forwardingRules
    {
        // To be defined...
    } forwarding_rules;
};

// Return a list of "bridge" structures. Each of them represents a set of
// local interfaces that have been "bridged" together.
//
// The length of the list is returned in the 'nr' output argument.
//
// When the returned list is no longer needed, it can be freed by calling
// "free_LIST_OF_BRIDGES()"
//
struct bridge *PLATFORM_GET_LIST_OF_BRIDGES(uint8_t *nr);

// Used to free the pointer returned by a previous call to
// "PLATFORM_GET_LIST_OF_BRIDGES()"
//
// 'nr' is the same one returned by "PLATFORM_GET_LIST_OF_BRIDGES()"
//
void free_LIST_OF_BRIDGES(struct bridge *x, uint8_t nr);


////////////////////////////////////////////////////////////////////////////////
// RAW packet generation
////////////////////////////////////////////////////////////////////////////////

// Send a RAW ethernet frame on interface 'name_interface' with:
//
//   - The "destination MAC address" field set to 'dst_mac'
//   - The "source MAC address" field set to 'src_mac'
//   - The "ethernet type" field set to 'eth_type'
//   - The payload os the ethernet frame set to the first 'payload_len' bytes
//     pointed by 'payload'
//
// If there is a problem and the packet cannot be sent, this function returns
// "0", otherwise it returns "1"
//
uint8_t PLATFORM_SEND_RAW_PACKET(const char *interface_name, const uint8_t *dst_mac, const uint8_t *src_mac,
                                 uint16_t eth_type, const uint8_t *payload, uint16_t payload_len);


////////////////////////////////////////////////////////////////////////////////
/// Push button configuration
////////////////////////////////////////////////////////////////////////////////

// Start the technology-specific "push button" configuration process on the
// provided interface.
//
// 'queue_id' is a value previously returned by a call to
// "PLATFORM_CREATE_QUEUE()"
//
// 'al_mac_address' is the AL MAC address contained in the "push button event
// notification" message that caused this function to be called. This value
// will later be reported back to the AL entity in the
// "PLATFORM_QUEUE_EVENT_AUTHENTICATED_LINK" message.
//
// 'mid' is the "message id" of the "push button event notification" message
// that caused this function to be called. This value will later be reported
// back to the AL entity in the "PLATFORM_QUEUE_EVENT_AUTHENTICATED_LINK"
// message.
//
//   NOTE:
//     When this function is called as a result of the user pressing a button
//     in the local device (versus receiving a remote "push button event
//     notification message" from another node) then 'al_mac_address' and 'mid'
//     contain the values that go inside the "push button event notification"
//     message that this local node is going to send to the others.
//
// Before calling this function, the "PLATFORM_QUEUE_EVENT_AUTHENTICATED_LINK"
// event must have been registered with "PLATFORM_REGISTER_QUEUE_EVENT()"
//
// This "push button" configuration process is used to add new devices to the
// network:
//
//   - For 802.11 interface this is usually the WPS mechanism.
//   - For G.hn interfaces we use the "pairing" mechanism.
//
// The function does not wait for the process to complete, instead it returns
// immediately and the configuration process is ran in background. Eventually,
// either:
//
//   A) The "push button" configuration process is stopped (because no one
//      answered at the other end of the link or because something failed)
//      after some technology-specific time.
//
//   B) The "push button" configuration is stopped because a new device has been
//      added. When this happens, a new message of type
//      "PLATFORM_QUEUE_EVENT_AUTHENTICATED_LINK" is posted to the system
//      queue.
//
// If there is a problem and the process cannot be started, this function
// returns "0", otherwise it returns "1"
//
// [PLATFORM PORTING NOTE]
//   If "interface_name" does not support the "push button" configuration
//   mechanism, this function should immediatley return "1".
//   Ie. a "PLATFORM_QUEUE_EVENT_AUTHENTICATED_LINK" message must *not* be
//   posted to the AL queue.
//
// [PLATFORM PORTING NOTE]
//   Note that once the process is started and until it finishes, if someone
//   calls "PLATFORM_GET_1905_INTERFACE_INFO()" on this interface, the field
//   'push_button_on_going' must return a value of "1".
//
uint8_t PLATFORM_START_PUSH_BUTTON_CONFIGURATION(char *interface_name, uint8_t queue_id, uint8_t *al_mac, uint16_t mid);


////////////////////////////////////////////////////////////////////////////////
/// Power control
////////////////////////////////////////////////////////////////////////////////

// Change the power mode of the provided interface.
//
// 'power_mode' can take any of the "INTERFACE_POWER_STATE_*" values
//
// The returned value can take any of the following values:
//   INTERFACE_POWER_RESULT_EXPECTED
//     The power mode has been applied as expected (ie. the new "power mode" is
//     the specified in the call)
//   INTERFACE_POWER_RESULT_NO_CHANGE
//     There was no need to apply anything, because the interface *already* was
//     in the requested mode
//   INTERFACE_POWER_RESULT_ALTERNATIVE
//     The interface power mode has changed as a result for this call, however
//     the new state is *not* the given one.  Example: You said
//     "INTERFACE_POWER_STATE_OFF", but the interface, due to maybe platform
//     limitations, ends up in "INTERFACE_POWER_STATE_SAVE"
//   INTERFACE_POWER_RESULT_KO
//     There was some problem trying to apply the given power mode
//
#define INTERFACE_POWER_RESULT_EXPECTED     (0x00)
#define INTERFACE_POWER_RESULT_NO_CHANGE    (0x01)
#define INTERFACE_POWER_RESULT_ALTERNATIVE  (0x02)
#define INTERFACE_POWER_RESULT_KO           (0x03)
uint8_t PLATFORM_SET_INTERFACE_POWER_MODE(char *interface_name, uint8_t power_mode);

#endif
