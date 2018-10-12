/*
 *  prplMesh Wi-Fi Multi-AP
 *
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

#ifndef DATAMODEL_H
#define DATAMODEL_H

#include "tlv.h" // ssid
#include "ptrarray.h"

#include <stdbool.h> // bool
#include <stddef.h>  // size_t

/** @file
 *
 * Multi-AP/1905.1a Data Model.
 *
 * This file defines the structures that comprise the data model used for Multi-AP / IEEE 1905.1a, and the functions
 * to manipulate it.
 */

/** @brief Definition of a BSS. */
struct bssInfo {
    mac_address bssid;
    struct ssid ssid;
};

enum interfaceType {
    interface_type_unknown = -1, /**< Interface was created without further information. */
    interface_type_ethernet = 0, /**< Wired ethernet interface. */
    interface_type_wifi = 1,     /**< 802.11 wireless interface. */
    interface_type_other = 255,  /**< Other interfaces types, not supported by this data model. */
};

/** @brief Definition of an interface
 *
 * The interface stores some information, but mostly the information is retrieved through callback functions.
 *
 * An interface may be created either because it belongs to an alDevice, or because it is a neighbor of an interface
 * that belongs to an alDevice.
 *
 * When an interface is added as the neighbor of another interface, the inverse relationship is added as well.
 *
 * When an interface is removed as the neighbor of another interface, and the interface does not belong to an alDevice,
 * it is destroyed.
 */
struct interface
{
    dlist_item l; /**< @brief Membership of the owner's alDevice::interfaces */

    /** @brief Interface name, e.g. eth0.
     *
     * Only set for local interfaces. Other device interface has this as NULL.
     */
    const char  *name;
    /** @brief Interface address. */
    mac_address addr;

    /** @brief Interface type. This indicates the subclass of the interface struct. */
    enum interfaceType type;

    /** @brief If the interface belongs to a 1905.1/EasyMesh device, this points to the owning device. */
    struct alDevice *owner;

    /** @brief IEEE 1905.1a Media Type, as per "IEEE Std 1905.1-2013, Table 6-12". */
    uint16_t media_type;

    /** @brief IEEE 1905.1a Media-specific Information, as per "IEEE Std 1905.1-2013, Table 6-12 and 6-13". */
    uint8_t media_specific_info[16];
    uint8_t media_specific_info_length; /**< @brief Valid length of @a media_specific_info. */

    /** @brief Info to control discovery messages sent to this interface.
     *
     * These are only valid for interfaces that are direct neighbors of the local device.
     *
     * @todo these don't belong here, because a single neighbor interface may be linked with multiple local interfaces.
     *
     * @{
     */
    uint32_t              last_topology_discovery_ts;
    uint32_t              last_bridge_discovery_ts;
    /** @} */

    /** @brief Neighbour interfaces. */
    PTRARRAY(struct interface *) neighbors;
};

enum interfaceWifiRole {
    interface_wifi_role_ap = 0, /**< AP role */
    interface_wifi_role_sta = 0b0100, /**< STA role */
    interface_wifi_role_other = 0b1111, /**< Other role, not supported by this data model */
};

/** @brief Wi-Fi interface.
 *
 * Subclass of ::interface for IEEE 802.11 BSSIDs.
 *
 * Wi-Fi interfaces are navigable both through the ::radio and through the ::alDevice structures. The ::alDevice
 * structure is the ::hlist_item parent.
 */
struct interfaceWifi {
    struct interface i;

    enum interfaceWifiRole role;

    /** @brief BSS info for this Wifi interface.
     *
     * Valid for AP and STA interfaces.
     */
    struct bssInfo bssInfo;

    /** @brief Radio on which this interface is active. Must not be NULL. */
    struct radio *radio;

    /** @brief Channel in use
     *
     *  This have to be a valid channel who refers to channel::id
     */
    int channel;

    /** @brief Clients connected to this BSS.
     *
     * Only valid if this is an AP.
     *
     * These are also included in interface::neighbors.
     */
    PTRARRAY(struct interfaceWifi *) clients;
};

/** @brief Wi-Fi radio supported bands.
 *
 *  Typically 2.4 or 5.0 Ghz along with the supported channels.
 */

struct channel {
    uint32_t    id;         /**< Channel id (0..255) */
    uint32_t    freq;       /**< Frequency */
    uint32_t    dbm;        /**< Power (value = (float) dbm * 0.01) */
    bool        radar;      /**< Is radar detection active on this channel ? */
    bool        disabled;   /**< Is this channel disabled ? */
};

struct band {
    int     id;                 /**< Band ID */
    bool    ht40;               /**< HT40 capability ? (True = HT20/40 is supported, else only HT20) */

    enum {
        BAND_SCW_160_OR_80_80 = 0,
        BAND_SCW_160 = 1,
        BAND_SCW_160_AND_80_80 = 2,
        BAND_SCW_RESERVED = 3 }     supChannelWidth;    /**< Supported channel width */
    enum {
        BAND_SGI_NONE = 0,
        BAND_SGI_80 = 1,
        BAND_SGI_160_OR_80_80 = 2 } shortGI;            /**< Short GI */

    PTRARRAY(struct channel) channels;   /**< List of channels allocated for this band */
};

/** @brief Wi-Fi radio.
 *
 * A device may have several radios, and each radio may have several configured interfaces. Each interface is a STA or
 * AP and can join exactly one BSSID.
 */
struct radio {
    dlist_item  l;          /**< Membership of ::alDevice */

    mac_address uid;        /**< Radio Unique Identifier for this radio. */
    char        name[16];   /**< Radio's name (eg phy0) */
    uint32_t    index;      /**< Radio's index (PHY) */

    bool        splitWiphy; /**< Is the protocol has split wiphy ? */

    uint8_t     conf_ant[2];/**< Configured antennas rx/tx */

    /** @brief List of bands and their attributes/channels */
    PTRARRAY(struct band *) bands;

    /** @brief List of BSSes configured for this radio.
     *
     * Elements are of type interfaceWifi. Their interfaceWifi::radio pointer points to this object.
     */
    dlist_head configured_bsses;
};

/** @brief 1905.1 device.
 *
 * Representation of a 1905.1 device in the network, discovered through topology discovery.
 */
struct alDevice {
    dlist_item l; /**< @brief Membership of ::network */

    mac_address al_mac_addr;    /**< @brief 1905.1 AL MAC address for this device. */
    dlist_head interfaces;      /**< @brief The interfaces belonging to this device. */
    dlist_head radios;          /**< @brief The radios belonging to this device. */

    bool is_map_agent; /**< @brief true if this device is a Multi-AP Agent. */
};

/** @brief The local AL device.
 *
 * This must be non-NULL for the AL functionality to work, but it may be NULL when the datamodel is used by an
 * external entity (e.g. a separate HLE).
 */
extern struct alDevice *local_device;

/** @brief WPS constants used in the wscDeviceData fields.
 *
 * These correspond to the definitions in WSC.
 * @{
 */

#define WPS_AUTH_OPEN          (0x0001)
#define WPS_AUTH_WPAPSK        (0x0002)
#define WPS_AUTH_SHARED        (0x0004) /* deprecated */
#define WPS_AUTH_WPA           (0x0008)
#define WPS_AUTH_WPA2          (0x0010)
#define WPS_AUTH_WPA2PSK       (0x0020)

#define WPS_ENCR_NONE          (0x0001)
#define WPS_ENCR_WEP           (0x0002) /* deprecated */
#define WPS_ENCR_TKIP          (0x0004)
#define WPS_ENCR_AES           (0x0008)

#define WPS_RF_24GHZ           (0x01)
#define WPS_RF_50GHZ           (0x02)
#define WPS_RF_60GHZ           (0x04)

/** @} */

/** @brief Device data received from registrar/controller through WSC.
 *
 * If local_device is the registrar/controller, this is the device data that is sent out through WSC.
 *
 * Note that the WSC data can only be mapped to a specific radio through the RF band. Note that WSC allows to apply the
 * same data to multiple bands simultaneously, but 1905.1/Multi-AP does not; still, the WSC frame may specify multiple
 * bands.
 *
 * Only PSK authentication is supported, not entreprise, so we can use a fixed-length key.
 */
struct wscDeviceData {
    mac_address bssid;          /**< BSSID (MAC address) of the BSS configured by this WSC exchange. */
    char device_name      [33]; /**< Device Name (0..32 octets encoded in UTF-8). */
    char manufacturer_name[65]; /**< Manufacturer (0..64 octets encoded in UTF-8). */
    char model_name       [65]; /**< Model Name (0..32 octets encoded in UTF-8). */
    char model_number     [65]; /**< Model Number (0..32 octets encoded in UTF-8). */
    char serial_number    [65]; /**< Serial Number (0..32 octets encoded in UTF-8). */
    uint8_t uuid          [16]; /**< UUID (16 octets). */
    uint8_t rf_bands;           /**< Bitmask of WPS_RF_24GHZ, WPS_RF_50GHZ, WPS_RF_60GHZ. */
    struct ssid ssid;           /**< SSID configured by this WSC. */
    uint16_t auth_types;        /**< Bitmask of WPS_AUTH_NONE, WPS_AUTH_WPA2PSK. */
    uint16_t encr_types;        /**< Bitmask of WPS_ENCR_NONE, WPS_ENCR_TKIP, WPS_ENCR_AES. */
    uint8_t key           [64]; /**< Enryption key. */
    uint8_t key_len;            /**< Length of @a key. */
};


/** @brief The discovered/configured Multi-AP controller or 1905.1 AP-Autoconfiguration Registrar.
 *
 * This points to the alDevice that was discovered to offer the Controller service. It may be the local_device, if it
 * was configured to take the controller role.
 *
 * There can be only one controller OR registrar in the network, so this is a singleton.
 *
 * The local device is the registrar/controller if registrar.d == local_device and local_device is not NULL.
 */
extern struct registrar {
    struct alDevice *d; /**< If non-NULL, a controller/registrar was configured/discovered. */
    bool is_map; /**< If true, it is a Multi-AP Controller. If it is false, it is only a 1905.1 Registrar. */

    /** @brief List of wscDeviceData objects received or configured.
     *
     * Since there can be only one WSC per band, the three bands are included explicitly. If a WSC covers multiple
     * bands, it is duplicated. Note that this makes it redundant with wscDeviceData::rf_band; however, a wscDeviceData
     * structure can also be built up independently (e.g. in WSC exchange).
     *
     * Unconfigured bands have wscDeviceData::bssid and wscDeviceData::rf_band set to 0.
     */
    struct wscDeviceData wsc_data[3];
} registrar;

/** @brief The network, i.e. a list of all discovered devices.
 *
 * Every discovered alDevice is added to this list. local_device (if it exists) is part of the list.
 */
extern dlist_head network;


/** @brief Initialize the data model. Must be called before anything else. */
void datamodelInit(void);

/** @brief Add a @a neighbor as a neighbor of @a interface. */
void interfaceAddNeighbor(struct interface *interface, struct interface *neighbor);

/** @brief Remove a @a neighbor as a neighbor of @a interface.
 *
 * @a neighbor may be free'd by this function. If the @a neighbor is not owned by an alDevice, and the neighbor has no
 * other neighbors, it is deleted.
 */
void interfaceRemoveNeighbor(struct interface *interface, struct interface *neighbor);

/** @brief Allocate a new @a alDevice. */
struct alDevice *alDeviceAlloc(const mac_address al_mac_addr);

/** @brief Delete a device and all its interfaces/radios.
  */
void alDeviceDelete(struct alDevice *alDevice);

/** @brief  Add a radio into ::alDevice
 *  @return 0:success, <0:error
 */
int alDeviceAddRadio(struct alDevice *alDevice, struct radio *radio);

/** @brief  Allocate a new ::radio
 *  @param mac      Unique identifier (mac address)
 *  @param name     Local system name
 *  @param index    Local system index
 */
struct radio *  radioAlloc(const mac_address mac, const char *name, int index);

/** @brief  Delete a ::radio and all its interfaces. */
void radioDelete(struct radio *radio);

/** @brief  Add an interface to ::radio
 *  @return 0:success, <0:error
 */
int radioAddInterfaceWifi(struct radio *radio, struct interfaceWifi *iface);

/** @brief Allocate a new interface, with optional owning device.
 *
 * If the owner is NULL, the interface must be added as a neighbor of another interface, to make sure it is still
 * referenced.
 */
struct interface *interfaceAlloc(const mac_address addr, struct alDevice *owner);

/** @brief Delete an interface and all its neighbors. */
void interfaceDelete(struct interface *interface);

/** @brief Allocate a new interface wifi (BSS) */
struct interfaceWifi *interfaceWifiAlloc(const mac_address addr, struct alDevice *owner);

/** @brief Delete an interface wifi (bss) */
void interfaceWifiDelete(struct interfaceWifi *interfaceWifi);

/** @brief Associate an interface with an alDevice.
 *
 * The interface must not be associated with another device already.
 */
void alDeviceAddInterface(struct alDevice *device, struct interface *interface);

/** @brief Find an alDevice based on its AL-MAC address. */
struct alDevice *alDeviceFind(const mac_address al_mac_addr);

/** @brief Find the interface belonging to a specific device. */
struct interface *alDeviceFindInterface(const struct alDevice *device, const mac_address addr);

/** @brief Find the interface belonging to any device.
 *
 * Only interfaces that are owned by a 1905 device are taken into account, not non-1905 neighbors.
 */
struct interface *findDeviceInterface(const mac_address addr);

/** @brief Find the local interface with a specific interface name. */
struct interface *findLocalInterface(const char *name);


/** @brief true if the local device is a registrar/controller, false if not.
 *
 * If there is no local device, it is always false (even a MultiAP Controller without Agent must have an AL MAC
 * address, so it must have a local device).
 */
static inline bool registrarIsLocal(void)
{
    return local_device != NULL && local_device == registrar.d;
}

#endif // DATAMODEL_H
