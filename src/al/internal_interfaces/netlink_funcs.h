#ifndef _NETLINK_FUNCS_H
#define _NETLINK_FUNCS_H
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

#include <stdbool.h>
#include <netlink/netlink.h> // struct nl_sock
#include <netlink/msg.h> // nl_msg

#include "datamodel.h" // struct alDevice / mac_address
#include "nl80211.h" // nl80211_commands

#define BIT(x) (1ULL<<(x))

/** @file
 *
 *  This file defines all the prototypes needed by the netlink functions
 */

struct nl80211_state {
    struct nl_sock *nl_sock;    /**< Netlink socket */
    int             nl80211_id; /**< Generic netlink family identifer */
};

struct _phy {
    mac_address     mac;
    int             index;
};

/** @brief  Collect some infos from sysfs (mac, index, ...)
 *  @return >0:success, 0:not found, <0:error
 */
extern int  phy_lookup(
                struct _phy *phy,   /**< Phy's informations found */
                const char *name    /**< Name of the radio interface (phy? in /sys/class/ieee80211) */
            );

/** @brief  Add all the local radios found with their collected datas into the datamodel
 *  @return 0:success, <0:error
 */
extern int  add_local_radios(
                struct alDevice *aldev  /**< ::alDevice on which this radio belongs */
            );

/** @brief  Open a netlink socket and issue a netlink command
 *
 *  The callback is called when a valid data is received. Otherwise,
 *  internal handlers assume the error handling.
 *
 *  @return 0:success, <0:error
 */
extern int  netlink_process(
                enum nl80211_commands cmd,          /**< Netlink request command */
                int device_index,                   /**< Radio index (_phy::index) */
                int (*cb)(struct nl_msg *, void *), /**< Callback to process the datas returned by the command */
                void *db_datas                      /**< Callback's datas */
            );

#endif
