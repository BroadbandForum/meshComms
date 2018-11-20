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

/** @file
 * @brief Driver interface for UCI
 *
 * This file provides driver functionality using UCI. It uses UCI calls to create access points.
 *
 * The register function must be called when the radios have already been discovered (e.g. with nl80211).
 */

#include <datamodel.h>
#include "platform_uci.h"

#include <stdio.h>      // printf(), popen()
#include <stdlib.h>     // malloc(), ssize_t
#include <string.h>     // strdup()
#include <libubox/blobmsg.h>
#include <libubus.h>

static bool uci_teardown_iface(struct interface *interface)
{
    struct interfaceWifi *interface_wifi = container_of(interface, struct interfaceWifi, i);
    struct ubus_context *ctx = ubus_connect(NULL);
    char macstr[18];
    uint32_t id;
    struct blob_buf b;
    void *match;
    bool ret = true;

    if (!ctx) {
        fprintf(stderr, "failed to connect to ubus.\n");
        return false;
    }

    if (interface->type != interface_type_wifi)
    {
        ret = false;
    }

    blob_buf_init(&b, 0);
    blobmsg_add_string(&b, "config", "wireless");
    blobmsg_add_string(&b, "type", "wifi-iface");
    match = blobmsg_open_table(&b, "match");
    snprintf(macstr, sizeof(macstr), MACSTR, MAC2STR(interface_wifi->bssInfo.bssid));
    blobmsg_add_string(&b, "bssid", macstr);
    blobmsg_add_string(&b, "device", (char *)interface_wifi->radio->priv);
    blobmsg_close_table(&b, match);
    if (ubus_lookup_id(ctx, "uci", &id) ||
        ubus_invoke(ctx, id, "delete", b.head, NULL, NULL, 3000)) {
        ret = false;
        goto out;
    }

    blob_buf_free(&b);
    blob_buf_init(&b, 0);
    blobmsg_add_string(&b, "config", "wireless");

    if (ubus_lookup_id(ctx, "uci", &id) ||
        ubus_invoke(ctx, id, "commit", b.head, NULL, NULL, 3000)) {
        ret = false;
        goto out;
    }

    out:
    /* @todo The removal of the interface should be detected through netlink. For the time being, however, we update the data model
     * straight away. */
    interfaceWifiRemove(interface_wifi);
    return ret;
}

static bool uci_create_iface(struct radio *radio, struct bssInfo bssInfo, bool ap)
{
    char macstr[18];
    struct ubus_context *ctx = ubus_connect(NULL);
    uint32_t id;
    struct blob_buf b;
    void *values;
    bool ret = true;

    if (!ctx) {
        fprintf(stderr, "failed to connect to ubus.\n");
        return false;
    }

    blob_buf_init(&b, 0);
    blobmsg_add_string(&b, "config", "wireless");
    blobmsg_add_string(&b, "type", "wifi-iface");
    values = blobmsg_open_table(&b, "values");
    blobmsg_add_string(&b, "device", (char *)radio->priv);
    blobmsg_add_string(&b, "mode", ap ? "ap" : "sta");
    blobmsg_add_string(&b, "network", "lan"); /* @todo set appropriate network */
    snprintf(macstr, sizeof(macstr), MACSTR, MAC2STR(bssInfo.bssid));
    blobmsg_add_string(&b, "bssid", macstr);
    blobmsg_add_field(&b, BLOBMSG_TYPE_STRING, "ssid", bssInfo.ssid.ssid, bssInfo.ssid.length);
    blobmsg_add_string(&b, "encryption", "none"); /* @todo set encryption */
    /* @todo handle backhaul flags */
    blobmsg_close_table(&b, values);
    if (ubus_lookup_id(ctx, "uci", &id) ||
        ubus_invoke(ctx, id, "add", b.head, NULL, NULL, 3000)) {
        ret = false;
        goto create_ap_out;
    }

    blob_buf_free(&b);
    blob_buf_init(&b, 0);
    blobmsg_add_string(&b, "config", "wireless");

    if (ubus_lookup_id(ctx, "uci", &id) ||
        ubus_invoke(ctx, id, "commit", b.head, NULL, NULL, 3000)) {
        ret = false;
        goto create_ap_out;
    }

    /* @todo The presence of the new AP should be detected through netlink. For the time being, however, we update the data model
     * straight away. */
    struct interfaceWifi *iface = interfaceWifiAlloc(bssInfo.bssid, local_device);
    radioAddInterfaceWifi(radio, iface);
    iface->role = interface_wifi_role_ap;
    memcpy(&iface->bssInfo, &bssInfo, sizeof(bssInfo));
    iface->i.tearDown = uci_teardown_iface;

create_ap_out:
    blob_buf_free(&b);
    ubus_free(ctx);
    return ret;
}

static bool uci_create_ap(struct radio *radio, struct bssInfo bssInfo)
{
    return uci_create_iface(radio, bssInfo, true);
}

static bool uci_create_sta(struct radio *radio, struct bssInfo bssInfo)
{
    return uci_create_iface(radio, bssInfo, false);
}

/*
 * policy for UCI get
 */
enum {
    UCI_GET_VALUES,
    __UCI_GET_MAX,
};

static const struct blobmsg_policy uciget_policy[__UCI_GET_MAX] = {
    [UCI_GET_VALUES] = { .name = "values", .type = BLOBMSG_TYPE_TABLE },
};

/* dlist to store uci wifi-device section names */
struct uciradiolist {
    dlist_item l;
    char *section;
    char *phyname;
};

/*
 * called by UCI get ubus call with all wifi-device sections
 * populates dlist with uci config names of wifi radios
 */
static void radiolist_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
    dlist_head *list = (dlist_head *)req->priv;
    struct blob_attr *tb[__UCI_GET_MAX];
    struct blob_attr *cur;
    struct uciradiolist *item;
    int rem;

    blobmsg_parse(uciget_policy, __UCI_GET_MAX, tb, blob_data(msg), blob_len(msg));

    if (!tb[UCI_GET_VALUES]) {
        fprintf(stderr, "No radios found\n");
        return;
    }

    blobmsg_for_each_attr(cur, tb[UCI_GET_VALUES], rem) {
        item = zmemalloc(sizeof(struct uciradiolist));
        item->section = strdup(blobmsg_name(cur));
        dlist_add_tail(list, &item->l);
    }
}

/*
 * policy for iwinfo phyname
 */
enum {
    IWINFO_PHYNAME_PHYNAME,
    __IWINFO_PHYNAME_MAX,
};

static const struct blobmsg_policy phyname_policy[__IWINFO_PHYNAME_MAX] = {
    [IWINFO_PHYNAME_PHYNAME] = { .name = "phyname", .type = BLOBMSG_TYPE_STRING },
};

/*
 * called by iwinfo phyname ubus call with resolved phyname
 * copies phyname to result pointer
 */
static void phyname_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
    char **phyname = (char **)req->priv;
    struct blob_attr *tb[__IWINFO_PHYNAME_MAX];
    blobmsg_parse(phyname_policy, __IWINFO_PHYNAME_MAX, tb, blob_data(msg), blob_len(msg));

    if (!tb[IWINFO_PHYNAME_PHYNAME]) {
        fprintf(stderr, "No phyname returned\n");
        return;
    }

    *phyname = strdup(blobmsg_get_string(tb[IWINFO_PHYNAME_PHYNAME]));
}

void uci_register_handlers(void)
{
    struct ubus_context *ctx = ubus_connect(NULL);
    uint32_t id;
    static struct blob_buf req;
    static dlist_head uciradios;
    struct radio *radio;
    struct blob_attr *cur;
    int rem;
    struct uciradiolist *uciphymatch;
    char *phyname;

    if (!ctx) {
        fprintf(stderr, "failed to connect to ubus.\n");
        return;
    }

    blob_buf_init(&req, 0);
    blobmsg_add_string(&req, "config", "wireless");
    blobmsg_add_string(&req, "type", "wifi-device");

    dlist_head_init(&uciradios);

    /* get radios from UCI */
    if (ubus_lookup_id(ctx, "uci", &id) ||
        ubus_invoke(ctx, id, "get", req.head, radiolist_cb, &uciradios, 3000))
        goto reghandlers_out;

    blob_buf_free(&req);

    /* populate phyname for each UCI wifi-device section */
    dlist_for_each(uciphymatch, uciradios, l)
    {
        blob_buf_init(&req, 0);
        blobmsg_add_string(&req, "section", uciphymatch->section);
        /* get phyname from iwinfo */
        if (ubus_lookup_id(ctx, "iwinfo", &id) ||
            ubus_invoke(ctx, id, "phyname", req.head, phyname_cb, &phyname, 3000))
            goto reghandlers_out;

        blob_buf_free(&req);
        uciphymatch->phyname = phyname;
    }

    /* register handlers for phy matching wifi-device UCI section */
    dlist_for_each(radio, local_device->radios, l)
    {
        dlist_for_each(uciphymatch, uciradios, l)
        {
            /*
             * register handlers if there is an UCI config section for
             * a discovered phy.
             * work-around new-line character at the end of radio->name
             */
            if(!strncmp(radio->name, uciphymatch->phyname, strlen(radio->name)-1)) {
                radio->addAP = uci_create_ap;
                radio->addSTA = uci_create_sta;
                radio->priv = (void *)strdup(uciphymatch->section);
                PLATFORM_PRINTF_DEBUG_DETAIL("registered UCI wifi-device %s (%s)\n",
                                             uciphymatch->section, uciphymatch->phyname);
                break;
            }
        }
    }

reghandlers_out:
    dlist_for_each(uciphymatch, uciradios, l)
    {
            free(uciphymatch->section);
            free(uciphymatch->phyname);
    }
    /* TODO: free dlist */
    ubus_free(ctx);
}
