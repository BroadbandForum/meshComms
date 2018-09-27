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

#include <datamodel.h>
#include <string.h> // memcpy

#define EMPTY_MAC_ADDRESS {0, 0, 0, 0, 0, 0}

struct alDevice *local_device = NULL;

struct registrar registrar = {
    .d = NULL,
    .is_map = false,
    .wsc_data = {
        {
            .bssid = EMPTY_MAC_ADDRESS,
            .rf_bands = 0,
        },
        {
            .bssid = EMPTY_MAC_ADDRESS,
            .rf_bands = 0,
        },
        {
            .bssid = EMPTY_MAC_ADDRESS,
            .rf_bands = 0,
        },
    }
};

DEFINE_DLIST_HEAD(network);

void datamodelInit(void)
{
}

struct alDevice *alDeviceAlloc(mac_address al_mac_addr)
{
    struct alDevice *ret = HLIST_ALLOC(struct alDevice, h, &network);
    memcpy(ret->al_mac_addr, al_mac_addr, 6);
    ret->is_map_agent = false;
    return ret;
}
