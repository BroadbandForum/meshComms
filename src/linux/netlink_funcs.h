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

/** @brief  Collect some infos from sysfs (mac, index, ...)
 *
 *  @param  basedir Directory to look for phy's attributes (ex. /sys/class/net/wlan0/phy80211)
 *  @param  name    Output name of the radio interface (phy{0..9})
 *  @param  mac     Output mac address (uid) of the radio
 *  @param  index   Output index of the PHY device
 *
 *  @return >0:success, 0:not found, <0:error
 */
extern int  phy_lookup(const char *basedir, char *name, mac_address *mac, int *index);

/** @brief  Add all the local radios found with their collected datas into global ::local_device
 *  @return 0:success, <0:error
 */
extern int  netlink_collect_local_infos(void);

/** @brief  Open the netlink socket and prepare for commands
 *
 *  @param  out_nlstate Output structure
 *
 *  @return 0=success, <0=error
 */
extern int  netlink_open(struct nl80211_state *out_nlstate);

/** @brief  Prepare a new Netlink message to be sent
 *
 *  @param  nlsock  Netlink socket to use
 *  @param  cmd     Netlink Command to initiate (See nl80211.h)
 *  @param  flags   Optional command flags
 *
 *  @return 0=success, <0=error
 */
extern struct nl_msg*   netlink_prepare(const struct nl80211_state *nlsock,
                            enum nl80211_commands cmd, int flags);

/** @brief  Execute a netlink command
 *
 *  The @a cb callback is called when a valid data is received. Otherwise,
 *  internal handlers assume the error handling.
 *
 *  @param  nlstate Netlink state & socket infos
 *  @param  nlmsg   Netlink message to process
 *  @param  cb      Callback to process the valid datas returned by the nlmsg
 *  @param  cbdatas Callback's datas passed as second parameter to @a cb
 *
 *  @return 0:success, <0:error
 */
extern int  netlink_do(struct nl80211_state *nlstate, struct nl_msg *nlmsg,
                int (*cb)(struct nl_msg *, void *), void *cbdatas);

/** @brief  Close the netlink socket and free allocations
 *
 *  @param  nlstate Netlink socket state structure
 */
extern void netlink_close(struct nl80211_state *nlstate);

/** @brief  Get the frequency of the corresponding channel
 *
 *  @param  chan    Channel ID
 *  @param  band    Band this channel is from
 *
 *  @return Frequency (x100)
 */
extern int  ieee80211_channel_to_frequency(int chan, enum nl80211_band band);

/** @brief  Get the channel corresponding to this frequency
 *
 *  @param  freq    Frequency
 *
 *  @return Channel id
 */
extern int  ieee80211_frequency_to_channel(int freq);

#endif
