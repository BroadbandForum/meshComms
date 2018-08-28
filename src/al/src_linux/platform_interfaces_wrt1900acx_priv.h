/*
 *  Broadband Forum IEEE 1905.1/1a stack
 *
 *  Copyright (c) 2017, Broadband Forum
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

#ifndef _PLATFORM_INTERFACES_WRT1900ACX_PRIV_H_
#define _PLATFORM_INTERFACES_WRT1900ACX_PRIV_H_


// Fill the "interfaceInfo" structure (associated to the provided
// "interface_name") by obtaining information from the OpenWRT UCI subsystem.
//
//
uint8_t linksys_wrt1900acx_get_interface_info(char *interface_name, struct interfaceInfo *m);

// Modify the current Wifi configuration according to the values passed as
// parameters. Modifications take effect immediately.
//
uint8_t linksys_wrt1900acx_apply_80211_configuration(char *interface_name, uint8_t *ssid, uint8_t *network_key);

#endif
