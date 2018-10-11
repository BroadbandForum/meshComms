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

/** @todo   Review all the return codes & error handling in general ... */

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <netlink/attr.h>       // nla_parse()
#include <netlink/genl/genl.h>  // genlmsg_attr*()

#include "netlink_funcs.h"
#include "datamodel.h"
#include "nl80211.h"
#include "platform.h"

static int collect_protocol_features(struct nl_msg *msg, struct radio *radio)
{
    struct nlattr       *tbm[NL80211_ATTR_MAX + 1];
    struct genlmsghdr   *gnlh = nlmsg_data(nlmsg_hdr(msg));

    if ( nla_parse(tbm, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL) < 0 )
        return -1;

    if ( tbm[NL80211_ATTR_PROTOCOL_FEATURES] ) {
        uint32_t feat = nla_get_u32(tbm[NL80211_ATTR_PROTOCOL_FEATURES]);

        PLATFORM_PRINTF_DEBUG_INFO("nl80211 features: 0x%x\n", feat);

        if ( feat & NL80211_PROTOCOL_FEATURE_SPLIT_WIPHY_DUMP ) {
            PLATFORM_PRINTF_DEBUG_INFO("\t* has split wiphy dump\n");
            radio->splitWiphy = true;
        }
    }
    return NL_SKIP;
}

/** @brief  callback to parse & collect radio attributes
 *
 *  This function is called multiple times from the netlink interface
 *  to process the different parts of the radio attributes.
 */
static int collect_radio_datas(struct nl_msg *msg, struct radio *radio)
{
    struct nlattr       *tb_msg[NL80211_ATTR_MAX + 1];
    struct genlmsghdr   *gnlh = nlmsg_data(nlmsg_hdr(msg));

    static struct nla_policy freq_policy[NL80211_FREQUENCY_ATTR_MAX + 1] = {
        [NL80211_FREQUENCY_ATTR_FREQ]         = { .type = NLA_U32 },
        [NL80211_FREQUENCY_ATTR_DISABLED]     = { .type = NLA_FLAG },
        [NL80211_FREQUENCY_ATTR_NO_IR]        = { .type = NLA_FLAG },
        [__NL80211_FREQUENCY_ATTR_NO_IBSS]    = { .type = NLA_FLAG },
        [NL80211_FREQUENCY_ATTR_RADAR]        = { .type = NLA_FLAG },
        [NL80211_FREQUENCY_ATTR_MAX_TX_POWER] = { .type = NLA_U32 },
    };

    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

    /* Configured antennas */
    if ( tb_msg[NL80211_ATTR_WIPHY_ANTENNA_RX] )
        radio->conf_ant[0] = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_ANTENNA_RX]);
    if ( tb_msg[NL80211_ATTR_WIPHY_ANTENNA_TX] )
        radio->conf_ant[1] = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_ANTENNA_TX]);

    /* Valid interface combinations */

    /** @todo Collect the "Valid interface combinations" from msg
     */

    if ( tb_msg[NL80211_ATTR_WIPHY_BANDS] ) {
        static struct band  *band;
        struct nlattr       *tb_band[NL80211_BAND_ATTR_MAX + 1], *nl_band;
        int                  rem_band;

        nla_for_each_nested(nl_band, tb_msg[NL80211_ATTR_WIPHY_BANDS], rem_band) {

            if ( ! band || band->id != nl_band->nla_type ) {
                band = zmemalloc(sizeof(struct band));
                PTRARRAY_ADD(radio->bands, band);
                band->id = nl_band->nla_type;
            }

            nla_parse(tb_band, NL80211_BAND_ATTR_MAX, nla_data(nl_band), nla_len(nl_band), NULL);

            if ( tb_band[NL80211_BAND_ATTR_HT_CAPA] ) {
                /* Band capabilities */
                uint16_t cap = nla_get_u16(tb_band[NL80211_BAND_ATTR_HT_CAPA]);
                band->ht40 = (cap & BIT(1) == BIT(1));
            }

            if ( tb_band[NL80211_BAND_ATTR_FREQS] ) {
                struct nlattr   *tb_freq[NL80211_FREQUENCY_ATTR_MAX + 1], *nl_freq;
                int              rem_freq;

                nla_for_each_nested(nl_freq, tb_band[NL80211_BAND_ATTR_FREQS], rem_freq) {
                    struct channel ch;

                    nla_parse(tb_freq, NL80211_FREQUENCY_ATTR_MAX, nla_data(nl_freq), nla_len(nl_freq), freq_policy);

                    if ( ! tb_freq[NL80211_FREQUENCY_ATTR_FREQ] )
                        continue;

                    ch.freq     = nla_get_u32(tb_freq[NL80211_FREQUENCY_ATTR_FREQ]);
                    ch.id       = ieee80211_frequency_to_channel(ch.freq);
                    ch.disabled = tb_freq[NL80211_FREQUENCY_ATTR_DISABLED];
                    ch.radar    = tb_freq[NL80211_FREQUENCY_ATTR_RADAR];

                    if ( tb_freq[NL80211_FREQUENCY_ATTR_MAX_TX_POWER] )
                        ch.dbm = nla_get_u32(tb_freq[NL80211_FREQUENCY_ATTR_MAX_TX_POWER]);

                    PTRARRAY_ADD(band->channels, ch);
                }
            }
        }
    }
    return NL_SKIP;
}

const char *__sysfs_ieee80211 = "/sys/class/ieee80211";

static int populate_radios_from_sysfs(struct alDevice *alDevice)
{
    DIR             *d;
    struct dirent   *f;
    int              ret = 0;

    if ( ! (d = opendir(__sysfs_ieee80211)) )
        return -1;

    errno = 0;
    while ( (f = readdir(d)) ) {
        mac_address mac;
        int         index;

		if ( f->d_name[0] == '.' )  /* Skip '.', '..' & hidden files */
		    continue;

        if ( phy_lookup(f->d_name, &mac, &index) <= 0 ) {
            ret = -1;
            break;
        }
        alDeviceAddRadio(alDevice, radioAlloc(mac, f->d_name, index));
        errno = 0;
    }
	if ( !f && errno )
		ret = -1;

    closedir(d);
    return ret;
}

int netlink_collect_local_infos(struct alDevice *alDevice)
{
    struct nl80211_state  nlstate;
    struct radio         *radio;
    int                   err = 0;

    PLATFORM_PRINTF_DEBUG_SET_VERBOSITY_LEVEL(3);

    if ( populate_radios_from_sysfs(alDevice) < 0 )
        return -1;
    if ( netlink_open(&nlstate) < 0 )
        return -1;

    dlist_for_each(radio, alDevice->radios, l) {
        struct nl_msg *m;

        /* Detect how the protocol is to be handled */
        if ( ! (m = netlink_prepare(&nlstate, NL80211_CMD_GET_PROTOCOL_FEATURES, 0))
        ||   netlink_do(&nlstate, m, (void *)collect_protocol_features, radio) < 0 ) {
            err = -1;
            break;
        }

        /* Now dump all the infos for this radio */
        if ( ! (m = netlink_prepare(&nlstate, NL80211_CMD_GET_WIPHY, 0)) ) {
            err = -1;
            break;
        }
        if ( radio->splitWiphy ) {
            nla_put_flag(m, NL80211_ATTR_SPLIT_WIPHY_DUMP);
            nlmsg_hdr(m)->nlmsg_flags |= NLM_F_DUMP;
        }
        nla_put(m, NL80211_ATTR_WIPHY, sizeof(radio->index), &radio->index);

        if ( netlink_do(&nlstate, m, (void *)collect_radio_datas, radio) < 0 ) {
            err = -1;
            break;
        }
    }
    netlink_close(&nlstate);
    return 0;
}
