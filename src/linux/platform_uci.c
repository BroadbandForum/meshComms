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
    blobmsg_close_table(&b, match);
    if (ubus_lookup_id(ctx, "uci", &id) ||
        ubus_invoke(ctx, id, "add", b.head, NULL, NULL, 3000)) {
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
    char radioname[16];
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
    // assume that radio->index is equal to the enumeration in OpenWrt,
    // which is not necessarily always true in the way radios are
    // currently enumerated by prplmesh
    snprintf(radioname, sizeof(radioname), "radio%u", radio->index);
    blobmsg_add_string(&b, "device", radioname);
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

    /* @todo The presence of the new AP should be detected through netlink. For the time being, however, we update the data model
     * straight away. */
    struct interfaceWifi *iface = interfaceWifiAlloc(bssInfo.bssid, local_device);
    radioAddInterfaceWifi(radio, iface);
    iface->role = interface_wifi_role_ap;
    memcpy(&iface->bssInfo, &bssInfo, sizeof(bssInfo));
    iface->i.tearDown = uci_teardown_iface;

out:
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


void uci_register_handlers(void)
{
    struct radio *radio;
    dlist_for_each(radio, local_device->radios, l)
    {
        /* @todo check if the radio exists in UCI */
        radio->addAP = uci_create_ap;
        radio->addSTA = uci_create_sta;
    }
}
