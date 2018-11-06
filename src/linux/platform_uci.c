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

#define _GNU_SOURCE // asprintf
#include <stdio.h>      // printf(), popen()
#include <stdlib.h>     // malloc(), ssize_t
#include <string.h>     // strdup()
#include <errno.h>      // errno

static bool uci_create_ap(struct radio *radio, struct ssid ssid, mac_address bssid,
                          uint16_t auth_type, uint16_t encryption_type, const uint8_t *key, size_t key_len)
{
    char cmd[500];
    /* @todo set encryption */
    snprintf(cmd, sizeof(cmd), "ubus call uci add "
                    "'{\"config\":\"wireless\",\"type\":\"wifi-iface\",\"values\":"
                        "{\"device\":\"radio%u\",\"mode\":\"ap\","
                        "\"network\":\"lan\"," /* @todo set appropriate network */
                        "\"bssid\":\"" MACSTR "\","
                        "\"ssid\":\"%.*s\"},"
                        "\"encryption\":\"none\"}'",
                   radio->index, MAC2STR(bssid), ssid.length, ssid.ssid);

    system(cmd);
    system("ubus call uci commit '{\"config\":\"wireless\"}'");

    /* @todo The presence of the new AP should be detected through netlink. For the time being, however, we update the data model
     * straight away. */
    struct interfaceWifi *iface = interfaceWifiAlloc(bssid, local_device);
    radioAddInterfaceWifi(radio, iface);
    iface->role = interface_wifi_role_ap;
    memcpy(iface->bssInfo.bssid, bssid, 6);
    memcpy(&iface->bssInfo.ssid, &ssid, sizeof(ssid));
    return true;
}

void uci_register_handlers(void)
{
    struct radio *radio;
    dlist_for_each(radio, local_device->radios, l)
    {
        /* @todo check if the radio exists in UCI */
        radio->addAP = uci_create_ap;
    }
}
