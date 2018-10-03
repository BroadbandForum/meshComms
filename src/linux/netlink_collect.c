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

#include <netlink/attr.h>       // nla_parse()
#include <netlink/genl/genl.h>  // genlmsg_attr*()

#include "netlink_funcs.h"
#include "datamodel.h"

static int collect_radio_datas(struct nl_msg *msg, struct radio *radio)
{
    struct nlattr       *tb_msg[NL80211_ATTR_MAX + 1];
    struct genlmsghdr   *gnlh = nlmsg_data(nlmsg_hdr(msg));

    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

    /** @todo   Extract all the netlink relevant data into ::radio struct */

    return NL_SKIP;
}

int add_local_radios(struct alDevice *alDevice)
{
    int     i, r;
    char    phy[16];

    for ( i=0 ; i < 255 ; i++ ) {
        struct radio    *radio;
        struct _phy      radio_phy;

        sprintf(phy, "phy%d", i);

        if ( (r = phy_lookup(&radio_phy, phy)) <= 0 )
            break;

        /** @todo   Allocate a new ::radio struct */

        if ( netlink_process(radio_phy.index, NL80211_CMD_GET_WIPHY, (void *)collect_radio_datas, &radio) < 0 )
            return -1;

        /** @todo   Connect the new ::radio to ::alDevice */
    }
    return r < 0 ? r : i;
}
