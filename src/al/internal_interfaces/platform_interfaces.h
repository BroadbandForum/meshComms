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

#ifndef _PLATFORM_INTERFACES_H_
#define _PLATFORM_INTERFACES_H_

#include "media_specific_blobs.h"  // struct genericInterfaceType


////////////////////////////////////////////////////////////////////////////////
// Interfaces info
////////////////////////////////////////////////////////////////////////////////

struct interfaceInfo
{
    char  *name;           // Example: "eth0"

    INT8U mac_address[6];  // 6  bytes long MAC address of the interface.


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
    INT16U interface_type;  // Indicates the MAC/PHY type of the underlying
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
            INT8U  bssid[6];        // This is the BSSID (MAC address of the
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
            INT8U  role;            // One of the values from above

            INT8U ap_channel_band;  // Hex value of dot11CurrentChannelBandwidth
                                    // (see "IEEE P802.11ac/D3.0" for
                                    // description)

            INT8U ap_channel_center_frequency_index_1;
                                    // Hex value of
                                    // dot11CurrentChannelCenterFrequencyIndex1
                                    // (see "IEEE P802.11ac/D3.0" for
                                    // description)

            INT8U ap_channel_center_frequency_index_2;
                                    // Hex value of
                                    // dot11CurrentChannelCenterFrequencyIndex2
                                    // (see "IEEE P802.11ac/D3.0" for
                                    // description)

            #define IEEE80211_AUTH_MODE_OPEN    (0x0001)
            #define IEEE80211_AUTH_MODE_WPA     (0x0002)
            #define IEEE80211_AUTH_MODE_WPAPSK  (0x0004)
            #define IEEE80211_AUTH_MODE_WPA2    (0x0008)
            #define IEEE80211_AUTH_MODE_WPA2PSK (0x0010)
            INT16U authentication_mode;
                                    // For APs: list of supported modes that
                                    // clients can use (OR'ed list of flags)
                                    // For STAs: current mode being used with
                                    // its AP (a single flag)

            #define IEEE80211_ENCRYPTION_MODE_NONE (0x0001)
            #define IEEE80211_ENCRYPTION_MODE_TKIP (0x0002)
            #define IEEE80211_ENCRYPTION_MODE_AES  (0x0004)
            INT16U encryption_mode;
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

    INT8U is_secured;      // Contains "1" if the interface is secure, "0"
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
    
    INT8U push_button_on_going;
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

    INT8U push_button_new_mac_address[6];
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
    INT8U power_state;     // Contains one of the INTERFACE_POWER_STATE_* values
                           // from above

    #define INTERFACE_NEIGHBORS_UNKNOWN (0xFF)
    INT8U  neighbor_mac_addresses_nr;
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

    INT8U  (*neighbor_mac_addresses)[6];
                             // List containing those MAC addreses just
                             // described in the comment above.

    INT8U  ipv4_nr;          // Number of IPv4 this device responds to
    struct _ipv4
    {
        #define IPV4_UNKNOWN (0)
        #define IPV4_DHCP    (1)
        #define IPV4_STATIC  (2)
        #define IPV4_AUTOIP  (3)
        INT8U type;          // One of the values from above

        INT8U address[4];    // IPv4 address

        INT8U dhcp_server[4];// If the ip was obtained by DHCP, this
                             // variable holds the IPv4 of the server
                             // (if known). Set to all zeros otherwise

    } *ipv4;                 // Array of 'ipv4_nr' elements. Each 
                             // element represents one of the IPv4 of
                             // this device.

    INT8U  ipv6_nr;          // Number of IPv6 this device responds to
    struct _ipv6
    {
        #define IPV6_UNKNOWN (0)
        #define IPV6_DHCP    (1)
        #define IPV6_STATIC  (2)
        #define IPV6_SLAAC   (3)
        INT8U type;          // One of the values from above

        INT8U address[16];   // IPv6 address

        INT8U origin[16];    // If type == IPV6_TYPE_DHCP, this field
                             // contains the IPv6 address of the DHCPv6 server.
                             // If type == IPV6_TYPE_SLAAC, this field contains
                             // the IPv6 address of the router that provided
                             // the SLAAC address.
                             // In any other case this field is set to all
                             // zeros.

    } *ipv6;                 // Array of 'ipv6_nr' elements. Each 
                             // element represents one of the IPv6 of
                             // this device.

    INT8U  vendor_specific_elements_nr;
                           // Number of items in the "vendor_specific_elements"
                           // array

    struct _vendorSpecificInfoElement
    {
        INT8U    oui[3];    // 24 bits globally unique IEEE-RA assigned 
                            // number to the vendor

        INT16U   vendor_data_len;  // Number of bytes in "vendor_data"
        INT8U   *vendor_data;      // Vendor specific data
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
// "PLATFORM_FREE_LIST_OF_1905_INTERFACES()"
//
// [PLATFORM PORTING NOTE]
//   Typically you want to return as many entries as physical interfaces there
//   are in the platform. However, if for some reason you want to make one or
//   more interfaces "invisible" to 1905 (maybe because they are "debug"
//   interfaces, such as a "management" ethernet port) you can return a reduced
//   list of interfaces.
//
char **PLATFORM_GET_LIST_OF_1905_INTERFACES(INT8U *nr);

// Used to free the pointer returned by a previous call to
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()"
//
// 'nr' is the same one returned by "PLATFORM_GET_LIST_OF_1905_INTERFACES()"
//
void PLATFORM_FREE_LIST_OF_1905_INTERFACES(char **x, INT8U nr);

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
// "PLATFORM_FREE_1905_STRUCTURE()" to dispose it
//
struct interfaceInfo *PLATFORM_GET_1905_INTERFACE_INFO(char *interface_name);

// Free the memory used by a "struct interfaceInfo" structure previously
// obtained by calling "PLATFORM_GET_1905_INTERFACE_INFO()"
//
void PLATFORM_FREE_1905_INTERFACE_INFO(struct interfaceInfo *i);


////////////////////////////////////////////////////////////////////////////////
// Link metrics
////////////////////////////////////////////////////////////////////////////////

struct linkMetrics
{
    INT8U   local_interface_address[6];     // A MAC address belonging to one of
                                            // the local interfaces.
                                            // Let's call this MAC "A"
                                             
    INT8U   neighbor_interface_address[6];  // A MAC address belonging to a
                                            // neighbor interface that is
                                            // directly reachable from "A".
                                            // Let's call this MAC "B".

    INT32U  measures_window;   // Time in seconds representing how far back in
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

    INT32U  tx_packet_ok;      // Estimated number of transmitted packets from
                               // "A" to "B" in the last 'measures_window'
                               // seconds.
    
    INT32U  tx_packet_errors;  // Estimated number of packets with errors
                               // transmitted from "A" to "B" in the last
                               // 'measures_window' seconds.

    INT16U  tx_max_xput;       // Extimated maximum MAC throughput from "A" to
                               // "B" in Mbits/s.

    INT16U  tx_phy_rate;       // Extimated PHY rate from "A" to "B" in Mbits/s.

    INT16U  tx_link_availability;
                               // Estimated average percentage of time that the
                               // link is available to transmit data from "A"
                               // to "B" in the last 'measures_window' seconds.

    INT32U  rx_packet_ok;      // Estimated number of transmitted packets from
                               // "B" to "A" in the last 'measures_window'
                               // seconds.
    
    INT32U  rx_packet_errors;  // Estimated number of packets with errors
                               // transmitted from "B" to "A" i nthe last
                               // 'measures_window' seconds.

    INT8U   rx_rssi;           // Estimated RSSI when receiving data from "B" to
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
// "PLATFORM_FREE_LINK_METRICS()" to dispose it
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
struct linkMetrics *PLATFORM_GET_LINK_METRICS(char *local_interface_name, INT8U *neighbor_interface_address);

// Free the memory used by a "struct linkMetrics" structure previously
// obtained by calling "PLATFORM_GET_LINK_METRICS()"
//
void PLATFORM_FREE_LINK_METRICS(struct linkMetrics *l);

////////////////////////////////////////////////////////////////////////////////
// Bridges info
////////////////////////////////////////////////////////////////////////////////

struct bridge
{
    char   *name;           // Example: "br0"

    INT8U   bridged_interfaces_nr;
    char   *bridged_interfaces[10];
                            // Names of the interfaces (such as "eth0") that
                            // belong to this bridge

    INT8U   forwarding_rules_nr;
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
// "PLATFORM_FREE_LIST_OF_BRIDGES()"
//
struct bridge *PLATFORM_GET_LIST_OF_BRIDGES(INT8U *nr);

// Used to free the pointer returned by a previous call to
// "PLATFORM_GET_LIST_OF_BRIDGES()"
//
// 'nr' is the same one returned by "PLATFORM_GET_LIST_OF_BRIDGES()"
//
void PLATFORM_FREE_LIST_OF_BRIDGES(struct bridge *x, INT8U nr);


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
INT8U PLATFORM_SEND_RAW_PACKET(char *interface_name, INT8U *dst_mac, INT8U *src_mac, INT16U eth_type, INT8U *payload, INT16U payload_len);


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
INT8U PLATFORM_START_PUSH_BUTTON_CONFIGURATION(char *interface_name, INT8U queue_id, INT8U *al_mac, INT16U mid);


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
INT8U PLATFORM_SET_INTERFACE_POWER_MODE(char *interface_name, INT8U power_mode);

////////////////////////////////////////////////////////////////////////////////
/// Security configuration
////////////////////////////////////////////////////////////////////////////////

// Configure an 80211 AP interface.
//
// 'interface_name' is one of the names previously returned in a call to
// "PLATFORM_GET_LIST_OF_1905_INTERFACES()".
// It must be an 802.11 interface with the role of "AP".
//
// 'ssid' is a NULL terminated string containing the "friendly" name of the
// network that the AP is going to create.
//
// 'bssid' is a 6 bytes long ID containing the MAC address of the "main" AP
// (typically the registrar) on "extended" networks (where several APs share the
// same security settings to make it easier for devices to "roam" between them).
//
// 'auth_mode' is the "authentication mode" the AP is going to use. It must take
// one of the values from "IEEE80211_AUTH_MODE_*"
//
// 'encryption_mode' is "encryption mode" the AP is going to use. It must take
// one of the values from "IEEE80211_ENCRYPTION_MODE_*"
//
// 'network_key' is a NULL terminated string representing the "network key" the
// AP is going to use.
// 
INT8U PLATFORM_CONFIGURE_80211_AP(char *interface_name, INT8U *ssid, INT8U *bssid, INT16U auth_mode, INT16U encryption_mode, INT8U *network_key);

#endif
