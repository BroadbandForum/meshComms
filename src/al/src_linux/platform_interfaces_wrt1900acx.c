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

#include "platform_interfaces.h"
#include "platform_interfaces_priv.h"
#include "platform_os_priv.h"

#include <stdio.h>      // printf(), popen()
#include <stdlib.h>     // malloc(), ssize_t
#include <string.h>     // strdup()
#include <errno.h>      // errno
#include <pthread.h>    // mutex functions


////////////////////////////////////////////////////////////////////////////////
// Private data and functions
////////////////////////////////////////////////////////////////////////////////

// OpenWRT includes the "UCI" configuration system to centralize in a single
// place all the configurability "needs" the user might have.
//
// It works like this:
//
//   - There is a special folder ("/etc/config") containing configuration files
//     for most of the OpenWRT components.
//
//   - These files are "mapped" to the real configuration files of each
//     component. So, for example, if you change "/etc/config/system" to update
//     the "hostname" paramete, UCI knows which other file really needs to be
//     modified (in this case "/etc/hostname") for the change to be effective.
//
//   - In addition, there is a command ("uci") than can be invoked to update
//     these file in "/etc/config" and reload the corresponding subsystems.
//
// The UCI subsystem is explained in great detail in the official OpenWRT wiki:
//
//   https://wiki.openwrt.org/doc/uci
//
// In order to obtain information of the UCI subsytem of apply a desired
// configuration setting we will use the following functions than simply
// execute the "uci" command and wait for a response.

// Mutex to avoid concurrent UCI access
//
pthread_mutex_t uci_mutex = PTHREAD_MUTEX_INITIALIZER;

static char * _read_uci_parameter_value(char * parameter)
{
    FILE    *pipe ;
    char    *line;
    size_t   len;
    char command[200] = "";

    strcat(command,"uci get ");
    strcat(command, parameter);

    // Execute the UCI query command.
    //
    pthread_mutex_lock(&uci_mutex);
    pipe = popen(command, "r");

    if (!pipe)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] popen() returned with errno=%d (%s)\n", errno, strerror(errno));
        pthread_mutex_unlock(&uci_mutex);
        return NULL;
    }

    // Next read/fill the rest of parameters
    //
    line = NULL;
    if (-1 != getline(&line, &len, pipe))
    {
        // Remove the last "\n"
        //
        line[strlen(line)-1] = 0x00;
    }

    pclose(pipe);
    pthread_mutex_unlock(&uci_mutex);

    return line;
}

static void _set_uci_parameter_value(char * parameter, INT8U *value)
{
    FILE *pipe ;
    char command[200] = "";

    strcat(command,"uci set ");
    strcat(command,parameter);
    strcat(command, (char *)value);

    // Execute the UCI query command.
    //
    pthread_mutex_lock(&uci_mutex);
    pipe = popen(command, "r");

    if (!pipe)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] popen() returned with errno=%d (%s)\n", errno, strerror(errno));
        pthread_mutex_unlock(&uci_mutex);
        return;
    }

    pclose(pipe);
    pthread_mutex_unlock(&uci_mutex);

    return;
}

static void _get_wifi_connected_devices(char *interface_name, struct interfaceInfo *m)
{
    FILE    *pipe;
    char    *line;
    size_t   len;
    ssize_t  read;
    INT8U    mac_addr[6];
    char     command[200];

    sprintf(command, "iw dev %s station dump | grep Station | cut -f2 -d' '",interface_name);

    // Execute the UCI query command.
    //
    pthread_mutex_lock(&uci_mutex);
    pipe = popen(command, "r");

    if (!pipe)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] popen() returned with errno=%d (%s)\n", errno, strerror(errno));
        pthread_mutex_unlock(&uci_mutex);
        return;
    }

    // Next read/fill the rest of parameters
    //
    line = NULL;
    m->neighbor_mac_addresses_nr = 0;
    m->neighbor_mac_addresses    = NULL;
    while (-1 != (read = getline(&line, &len, pipe)))
    {
        // Remove the last "\n"
        //
        line[strlen(line)-1] = 0x00;

        if (6 == sscanf(line, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]))
        {
             PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] %02x:%02x:%02x:%02x:%02x:%02x wifi device connected to %s\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], interface_name);
             m->neighbor_mac_addresses = (INT8U (*)[6])realloc(m->neighbor_mac_addresses, sizeof(INT8U[6]) * m->neighbor_mac_addresses_nr + 1);
             memcpy(m->neighbor_mac_addresses[m->neighbor_mac_addresses_nr], mac_addr, 6);
             m->neighbor_mac_addresses_nr++;
        }
        else
        {
             PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] Invalid MAC address \n", line);
        }
    }

    if (m->neighbor_mac_addresses_nr == 0 )
    {
        PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] No Wifi device connected \n");
    }

    pclose(pipe);
    pthread_mutex_unlock(&uci_mutex);

    return;
}


////////////////////////////////////////////////////////////////////////////////
// Internal API: to be used by other platform-specific files (functions
// declaration is found in "./platform_interfaces_wrt1900acx_priv.h")
////////////////////////////////////////////////////////////////////////////////

INT8U linksys_wrt1900acx_get_interface_info(char *interface_name, struct interfaceInfo *m)
{
    // Check interface name
    //
    if (strstr(interface_name, "wlan") != NULL)
    {
        char  *line = NULL;
        char command[200];
        char *interface_id = interface_name + 4;

        // Find out if device is configured as AP or EP
        //
        sprintf(command, "wireless.@wifi-iface[%c].mode",*interface_id);

        line = _read_uci_parameter_value(command);
        if (line != NULL)
        {
            PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM]   > UCI mode: %s\n", line);

            if (strstr(line, "ap") != NULL)
            {
                m->interface_type_data.ieee80211.role = IEEE80211_ROLE_AP;
            }
            else
            {
                m->interface_type_data.ieee80211.role = IEEE80211_ROLE_NON_AP_NON_PCP_STA;
            }
        }

        // Retrieve SSID information
        //
        sprintf(command, "wireless.@wifi-iface[%c].ssid",*interface_id);

        line = _read_uci_parameter_value(command);
        if (line != NULL)
        {
            PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM]   > UCI SSID: %s\n", line);
            memcpy(m->interface_type_data.ieee80211.ssid, line, strlen(line)+1);
        }

        // Retrieve Network key information
        //
        sprintf(command, "wireless.@wifi-iface[%c].key",*interface_id);

        line = _read_uci_parameter_value(command);
        if (line != NULL)
        {
            PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM]   > UCI key: %s\n", line);
            memcpy(m->interface_type_data.ieee80211.network_key, line, strlen(line)+1);
        }

        // Relases 'getline' resources
        free(line);

        // TODO: Add full support of WIFI parameters. For now, use static
        // values.
        //
        m->interface_type                                    = INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ;
        m->interface_type_data.ieee80211.authentication_mode = IEEE80211_AUTH_MODE_WPAPSK     | IEEE80211_AUTH_MODE_WPA2PSK;
        m->interface_type_data.ieee80211.encryption_mode     = IEEE80211_ENCRYPTION_MODE_TKIP | IEEE80211_ENCRYPTION_MODE_AES;
        m->is_secured                                        = 1;

        m->interface_type_data.ieee80211.bssid[0] = 0x00;
        m->interface_type_data.ieee80211.bssid[1] = 0x00;
        m->interface_type_data.ieee80211.bssid[2] = 0x00;
        m->interface_type_data.ieee80211.bssid[3] = 0x00;
        m->interface_type_data.ieee80211.bssid[4] = 0x00;
        m->interface_type_data.ieee80211.bssid[5] = 0x00;


        //Retrieve list of connected devices
        //
        _get_wifi_connected_devices(interface_name,m);
    }
    else
    {
        m->interface_type = INTERFACE_TYPE_IEEE_802_3AB_GIGABIT_ETHERNET;
        m->is_secured     = 1;
    }

    // TODO: Obtain the actual value for the following parameters
    //
    m->push_button_on_going        = 2; // "2" means "not supported"
    m->power_state                 = INTERFACE_POWER_STATE_ON;
    m->ipv4_nr                     = 0;
    m->ipv4                        = NULL;
    m->ipv6_nr                     = 0;
    m->ipv6                        = NULL;
    m->vendor_specific_elements_nr = 0;
    m->vendor_specific_elements    = NULL;

    return 1;
}

INT8U linksys_wrt1900acx_apply_80211_configuration(char *interface_name, INT8U *ssid, INT8U *network_key)
{
    _set_uci_parameter_value("wireless.@wifi-iface[1].ssid=",ssid);
    _set_uci_parameter_value("wireless.@wifi-iface[1].key=",network_key);
    _set_uci_parameter_value("wireless.@wifi-iface[1].network_key=",network_key);
    _set_uci_parameter_value("wireless.@wifi-iface[1].encryption=",(INT8U *)"psk2");

    system("wifi reload");

    return 1;
}

