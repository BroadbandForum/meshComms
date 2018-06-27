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

#ifndef INTERFACE_H
#define INTERFACE_H

#include <1905_cmdus.h>

#include <stdbool.h> // bool
#include <stddef.h>  // size_t

/** @file
 *
 * Definition of interfaces.
 *
 * Interface control can be implemented in various ways. The ::interface struct collects the functions needed to
 * get statistics from an interface and to control it.
 */

/** @brief Defintion of a BSS. */
struct bss_info {
    mac_address bssid;
    const char *ssid;
};

/** @brief Definition of an interface
 *
 * The interface stores some information, but mostly the information is retrieved through callback functions.
 */
struct interface
{
    /** @brief Interface name, e.g. eth0. */
    char  *name;
    /** @brief Interface address or Radio Unique Identifier. */
    mac_address addr;
    /** @brief List of BSS for which this interface is an AP.
     *
     * The number of elements in this list is given by num_bss.
     *
     * If the interface is not an AP or no BSS are configured on it, this is NULL and num_bss is 0.
     */
    struct bss_info *bss_info;
    /** @brief Number of elements in bss_info. */
    size_t num_bss;

    /** @brief Implementation callback to fill bss_info.
     *
     * The implementation of this function must set or update the bss_info and num_bss fields of @a interface.
     *
     * This function is called at startup.
     *
     * Interfaces that are not access points can leave this as NULL, in which case bss_info will stay NULL as well.
     *
     * @param interface The interface for which to get BSS info.
     * @return true If bss_info was updated, false if not.
     *
     * @todo Foresee a way to update BSS info dynamically.
     */
    bool (*get_bss_info) (struct interface *interface);

    /** @brief Implementation callback to block a station from a specific BSS
     *
     * This function is called when an authenticated controller sends a Client steering request to block clients.
     *
     * @param interface The interface to which the steering request applies.
     * @param bss The BSS to which the steering request applies.
     * @param clients The list of clients to block.
     * @param num_clients The number of elements in @a clients.
     *
     * @return true if the block request succeeded, false if it failed.
     */
    bool (*block_client) (
            struct interface *interface,
            const struct bss_info *bss,
            mac_address *clients,
            size_t num_clients);
};


#endif // INTERFACE_H
