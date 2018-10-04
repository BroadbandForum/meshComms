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

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <netlink/attr.h>       // nla_parse()
#include <netlink/genl/genl.h>  // genlmsg_attr*()

#include "netlink_funcs.h"
#include "datamodel.h"
#include "nl80211.h"

static int collect_radio_datas(struct nl_msg *msg, struct radio *radio)
{
    struct nlattr       *tb_msg[NL80211_ATTR_MAX + 1];
    struct genlmsghdr   *gnlh = nlmsg_data(nlmsg_hdr(msg));

    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

    /** @todo   Extract all the netlink relevant data into ::radio */

    return NL_SKIP;
}

const char *__sysfs_ieee80211 = "/sys/class/ieee80211";

int netlink_collect_local_infos(struct alDevice *alDevice)
{
    DIR                 *d;
    struct dirent       *f;
    int                  i = 0, r;
    struct radio        *radio;
    struct _phy          phy;

	if ( ! (d = opendir(__sysfs_ieee80211)) )
		return -1;

    errno = 0;
    while ( (f = readdir(d)) )
    {
		if ( f->d_name[0] == '.' )  /* Skip '.', '..' & hidden files */
		    continue;

        if ( (r = phy_lookup(&phy, f->d_name)) < 0 )
            break;

        radio = radioAlloc(phy.mac, f->d_name, phy.index);

        if ( netlink_process(phy.index, NL80211_CMD_GET_WIPHY, (void *)collect_radio_datas, radio) < 0 ) {
            radioDelete(radio);
            return -1;
        }
        alDeviceAddRadio(local_device, radio);
        i++;
        errno = 0;
	}
	if ( !f && errno )
		return -1;

	closedir(d);

    return r < 0 ? r : i;
}
