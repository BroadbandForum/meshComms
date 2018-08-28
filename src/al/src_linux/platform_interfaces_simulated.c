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

#include "platform.h"
#include "platform_interfaces.h"                // struct interfaceInfo
#include "platform_interfaces_priv.h"           // registerInterfaceStub
#include "platform_interfaces_simulated_priv.h"

#include <stdio.h>    // fopen()
#include <stdlib.h>   // malloc()
#include <string.h>   // index()
#include <errno.h>    // errno


////////////////////////////////////////////////////////////////////////////////
// Private data and functions
////////////////////////////////////////////////////////////////////////////////

// Obtain information from the simulated device associated to interface
// 'interface_name' and fill the 'm' structure.
//
// The 'simulated_extended_params' is a string with the following format:
//
//   simulated:<filename>
//
// Example:
//
//   ghn_spirit:interface_parameters.txt
//
// This function will then open the given file, parse it, and fill the 'm'
// structure with the data contained in that file.
//
// Some sample files (to understand the expected syntax) are given next:
//
// REGULAR ETHERNET INTERFACE:
//
//   # This is a comment. The "2" in "push_button_on_going" means "push
//   # button" configuration is not supported.
//
//   mac_address                 = 00:16:03:01:85:1f
//   manufacturer_name           = Marvell
//   model_name                  = ETH PHY x200
//   model_number                = 00001
//   serial_number               = 0982946599817632
//   device_name                 = Marvell eth phy x200
//   uuid                        = 0982946599817632
//   interface_type              = INTERFACE_TYPE_IEEE_802_3U_FAST_ETHERNET
//   is_secured                  = 1
//   push_button_on_going        = 2
//   push_button_new_mac_address = 00:00:00:00:00:00
//   power_state                 = INTERFACE_POWER_STATE_ON
//   neighbor_mac_address        = 00:10:1a:b3:e4:01
//   neighbor_mac_address        = 00:10:1a:b3:e4:02
//   neighbor_mac_address        = 00:10:1a:b3:e4:03
//   ipv4                        = 10.10.1.4 dhcp 10.10.1.10
//   ipv4                        = 192.168.1.7 static 0.0.0.0
//   ipv6                        = fe80:0000:0000:0000:0221:9bff:fefd:1520 static 0000:0000:0000:0000:0000:0000:0000:0000
//   oui                         = 0a:b5:08
//   vendor_data                 = 00:00:00:0a:09:1a:9b:ed:e3
//   oui                         = 0a:b5:f4
//   vendor_data                 = 00:00:01:01:02
//
// STA wifi client (initially unconfigured):
//
//   mac_address                                   = 00:16:03:01:85:1f
//   manufacturer_name                             = Marvell
//   model_name                                    = WIFI PHY RT5200
//   model_number                                  = 00001
//   serial_number                                 = 8778291200910012
//   device_name                                   = Marvell eth phy x200
//   uuid                                          = 1111000020100204
//   interface_type                                = INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ
//   ieee80211.bssid                               = 00:00:00:00:00:00
//   ieee80211.ssid                                =
//   ieee80211.role                                = IEEE80211_ROLE_NON_AP_NON_PCP_STA
//   ieee80211.ap_channel_band                     = 0
//   ieee80211.ap_channel_center_frequency_index_1 = 0
//   ieee80211.ap_channel_center_frequency_index_2 = 0
//   ieee80211.authentication_mode                 = IEEE80211_AUTH_MODE_OPEN | IEEE80211_AUTH_MODE_WPAPSK
//   ieee80211.encryption_mode                     = IEEE80211_ENCRYPTION_MODE_AES
//   ieee80211.network_key                         =
//   is_secured                                    = 0
//   push_button_on_going                          = 0
//   push_button_new_mac_address                   = 00:00:00:00:00:00
//   power_state                                   = INTERFACE_POWER_STATE_ON
//
// AP registrar:
//
//   mac_address                                   = 00:16:03:01:85:1f
//   manufacturer_name                             = Marvell
//   model_name                                    = WIFI PHY RT5200
//   model_number                                  = 00001
//   serial_number                                 = 8778291200910013
//   device_name                                   = Marvell eth phy x200
//   uuid                                          = 1111000020100203
//   interface_type                                = INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ
//   ieee80211.bssid                               = 0a:30:4b:00:00:07
//   ieee80211.ssid                                = My wifi Network
//   ieee80211.role                                = IEEE80211_ROLE_AP
//   ieee80211.ap_channel_band                     = 10
//   ieee80211.ap_channel_center_frequency_index_1 = 20
//   ieee80211.ap_channel_center_frequency_index_2 = 30
//   ieee80211.authentication_mode                 = IEEE80211_AUTH_MODE_WPAPSK
//   ieee80211.encryption_mode                     = IEEE80211_ENCRYPTION_MODE_AES
//   ieee80211.network_key                         = my secret password
//   is_secured                                    = 1
//   push_button_on_going                          = 0
//   push_button_new_mac_address                   = 00:00:00:00:00:00
//   power_state                                   = INTERFACE_POWER_STATE_ON
//
void _getInterfaceInfoFromSimulatedDevice(__attribute__((unused)) char *interface_name, char *simulated_extended_params, struct interfaceInfo *m)
{
    FILE  *fp;
    char  *simulation_filename;

    char   aux1[200];
    char   aux2[200];

    char  *save_ptr1;
    char  *save_ptr2;

    if (NULL == (simulation_filename = index(simulated_extended_params, ':')))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Missing simulation file name in extended params string (%s)\n", simulated_extended_params);
        return;
    }
    simulation_filename++;

    if(NULL == (fp = fopen(simulation_filename, "r")))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] fopen('%s') failed with errno=%d (%s)\n", simulation_filename, errno, strerror(errno));
        return;
    }

    PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] Using simulated parameters from file %s\n", simulation_filename);

    while (NULL != fgets(aux1, sizeof(aux1), fp))
    {
        char *param, *value;
        size_t i, j;
        int equal_found, first_char_after_equal_found;

        // Remove spaces (up to the point where the parameter value starts,
        // after the first '=') and '\n'
        //
        equal_found                  = 0;
        first_char_after_equal_found = 0;
        for (i=0, j=0; i<strlen(aux1); i++)
        {
            if ('\n' == aux1[i])
            {
                // Remove '\n'
                //
                continue;
            }
            else if ('=' == aux1[i])
            {
                equal_found = 1;
            }
            else if (' ' == aux1[i])
            {
                if (!first_char_after_equal_found)
                {
                    // Remove this space
                    //
                    continue;
                }
            }
            else
            {
                // This is a regular character
                //
                if (equal_found && !first_char_after_equal_found)
                {
                    first_char_after_equal_found = 1;
                }
            }

            aux2[j++] = aux1[i];
        }
        aux2[j] = 0x0;

        // Skip comments
        //
        if (aux2[0] == '#')
        {
            continue;
        }

        // Parse parameters
        //
        if      (( NULL != (param = strtok_r(aux2, "=", &save_ptr1))) && (NULL != (value = strtok_r(NULL, "=", &save_ptr1))))
        {
            if     (0 == strcmp(param, "mac_address"))
            {
                sscanf(value, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                        &(m->mac_address[0]),
                        &(m->mac_address[1]),
                        &(m->mac_address[2]),
                        &(m->mac_address[3]),
                        &(m->mac_address[4]),
                        &(m->mac_address[5]));
            }
            else if (0 == strcmp(param, "manufacturer_name"))
            {
                strcpy(m->manufacturer_name, value);
            }
            else if (0 == strcmp(param, "model_name"))
            {
                strcpy(m->model_name, value);
            }
            else if (0 == strcmp(param, "model_number"))
            {
                strcpy(m->model_number, value);
            }
            else if (0 == strcmp(param, "serial_number"))
            {
                strcpy(m->serial_number, value);
            }
            else if (0 == strcmp(param, "device_name"))
            {
                strcpy(m->device_name, value);
            }
            else if (0 == strcmp(param, "uuid"))
            {
                strcpy(m->uuid, value);
            }
            else if (0 == strcmp(param, "interface_type"))
            {
                if      (0 == strcmp(value, "INTERFACE_TYPE_IEEE_802_3U_FAST_ETHERNET"))
                {
                    m->interface_type = INTERFACE_TYPE_IEEE_802_3U_FAST_ETHERNET;
                }
                else if (0 == strcmp(value, "INTERFACE_TYPE_IEEE_802_3AB_GIGABIT_ETHERNET"))
                {
                    m->interface_type = INTERFACE_TYPE_IEEE_802_3AB_GIGABIT_ETHERNET;
                }
                else if (0 == strcmp(value, "INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ"))
                {
                    m->interface_type = INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ;
                }
                else if (0 == strcmp(value, "INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ"))
                {
                    m->interface_type = INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ;
                }
                else if (0 == strcmp(value, "INTERFACE_TYPE_IEEE_802_11A_5_GHZ"))
                {
                    m->interface_type = INTERFACE_TYPE_IEEE_802_11A_5_GHZ;
                }
                else if (0 == strcmp(value, "INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ"))
                {
                    m->interface_type = INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ;
                }
                else if (0 == strcmp(value, "INTERFACE_TYPE_IEEE_802_11N_5_GHZ"))
                {
                    m->interface_type = INTERFACE_TYPE_IEEE_802_11N_5_GHZ;
                }
                else if (0 == strcmp(value, "INTERFACE_TYPE_IEEE_802_11AC_5_GHZ"))
                {
                    m->interface_type = INTERFACE_TYPE_IEEE_802_11AC_5_GHZ;
                }
                else if (0 == strcmp(value, "INTERFACE_TYPE_IEEE_802_11AD_60_GHZ"))
                {
                    m->interface_type = INTERFACE_TYPE_IEEE_802_11AD_60_GHZ;
                }
                else if (0 == strcmp(value, "INTERFACE_TYPE_IEEE_802_11AF_GHZ"))
                {
                    m->interface_type = INTERFACE_TYPE_IEEE_802_11AF_GHZ;
                }
                else if (0 == strcmp(value, "INTERFACE_TYPE_IEEE_1901_WAVELET"))
                {
                    m->interface_type = INTERFACE_TYPE_IEEE_1901_WAVELET;
                }
                else if (0 == strcmp(value, "INTERFACE_TYPE_IEEE_1901_FFT"))
                {
                    m->interface_type = INTERFACE_TYPE_IEEE_1901_FFT;
                }
                else if (0 == strcmp(value, "INTERFACE_TYPE_MOCA_V1_1"))
                {
                    m->interface_type = INTERFACE_TYPE_MOCA_V1_1;
                }
                else if (0 == strcmp(value, "INTERFACE_TYPE_UNKNOWN"))
                {
                    m->interface_type = INTERFACE_TYPE_UNKNOWN;
                }
            }
            else if (0 == strcmp(param, "ieee80211.bssid"))
            {
                sscanf(value, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                        &(m->interface_type_data.ieee80211.bssid[0]),
                        &(m->interface_type_data.ieee80211.bssid[1]),
                        &(m->interface_type_data.ieee80211.bssid[2]),
                        &(m->interface_type_data.ieee80211.bssid[3]),
                        &(m->interface_type_data.ieee80211.bssid[4]),
                        &(m->interface_type_data.ieee80211.bssid[5]));
            }
            else if (0 == strcmp(param, "ieee80211.ssid"))
            {
                strcpy(m->interface_type_data.ieee80211.ssid, value);
            }
            else if (0 == strcmp(param, "ieee80211.role"))
            {
                if      (0 == strcmp(value, "IEEE80211_ROLE_AP"))
                {
                    m->interface_type_data.ieee80211.role = IEEE80211_ROLE_AP;
                }
                else if (0 == strcmp(value, "IEEE80211_ROLE_NON_AP_NON_PCP_STA"))
                {
                    m->interface_type_data.ieee80211.role = IEEE80211_ROLE_NON_AP_NON_PCP_STA;
                }
                else if (0 == strcmp(value, "IEEE80211_ROLE_WIFI_P2P_CLIENT"))
                {
                    m->interface_type_data.ieee80211.role = IEEE80211_ROLE_WIFI_P2P_CLIENT;
                }
                else if (0 == strcmp(value, "IEEE80211_ROLE_WIFI_P2P_GROUP_OWNER"))
                {
                    m->interface_type_data.ieee80211.role = IEEE80211_ROLE_WIFI_P2P_GROUP_OWNER;
                }
                else if (0 == strcmp(value, "IEEE80211_ROLE_AD_PCP"))
                {
                    m->interface_type_data.ieee80211.role = IEEE80211_ROLE_AD_PCP;
                }
            }
            else if (0 == strcmp(param, "ieee80211.ap_channel_band"))
            {
                sscanf(value, "%02hhx", &(m->interface_type_data.ieee80211.ap_channel_band));
            }
            else if (0 == strcmp(param, "ieee80211.ap_channel_center_frequency_index_1"))
            {
                sscanf(value, "%02hhx", &(m->interface_type_data.ieee80211.ap_channel_center_frequency_index_1));
            }
            else if (0 == strcmp(param, "ieee80211.ap_channel_center_frequency_index_2"))
            {
                sscanf(value, "%02hhx", &(m->interface_type_data.ieee80211.ap_channel_center_frequency_index_2));
            }
            else if (0 == strcmp(param, "ieee80211.authentication_mode"))
            {
                uint16_t am = 0;

                char *token;

                token = strtok_r(value, " |", &save_ptr2);

                while (NULL != token)
                {
                    if      (0 == strcmp(token, "IEEE80211_AUTH_MODE_OPEN"))
                    {
                        am |= IEEE80211_AUTH_MODE_OPEN;
                    }
                    else if (0 == strcmp(token, "IEEE80211_AUTH_MODE_WPA"))
                    {
                        am |= IEEE80211_AUTH_MODE_WPA;
                    }
                    else if (0 == strcmp(token, "IEEE80211_AUTH_MODE_WPAPSK"))
                    {
                        am |= IEEE80211_AUTH_MODE_WPAPSK;
                    }
                    else if (0 == strcmp(token, "IEEE80211_AUTH_MODE_WPA2"))
                    {
                        am |= IEEE80211_AUTH_MODE_WPA2;
                    }
                    else if (0 == strcmp(token, "IEEE80211_AUTH_MODE_WPA2PSK"))
                    {
                        am |= IEEE80211_AUTH_MODE_WPA2PSK;
                    }

                    token = strtok_r(NULL, " |", &save_ptr2);
                }

                m->interface_type_data.ieee80211.authentication_mode = am;
            }
            else if (0 == strcmp(param, "ieee80211.encryption_mode"))
            {
                uint16_t em = 0;

                char *token;

                token = strtok_r(value, " |", &save_ptr2);

                while (NULL != token)
                {
                    if      (0 == strcmp(token, "IEEE80211_ENCRYPTION_MODE_NONE"))
                    {
                        em |= IEEE80211_ENCRYPTION_MODE_NONE;
                    }
                    else if (0 == strcmp(token, "IEEE80211_ENCRYPTION_MODE_TKIP"))
                    {
                        em |= IEEE80211_ENCRYPTION_MODE_TKIP;
                    }
                    else if (0 == strcmp(token, "IEEE80211_ENCRYPTION_MODE_AES"))
                    {
                        em |= IEEE80211_ENCRYPTION_MODE_AES;
                    }

                    token = strtok_r(NULL, " |", &save_ptr2);
                }

                m->interface_type_data.ieee80211.encryption_mode = em;
            }
            else if (0 == strcmp(param, "ieee80211.network_key"))
            {
                strcpy(m->interface_type_data.ieee80211.network_key, value);
            }
            else if (0 == strcmp(param, "ieee1901.network_identifier"))
            {
                sscanf(value, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                        &(m->interface_type_data.ieee1901.network_identifier[0]),
                        &(m->interface_type_data.ieee1901.network_identifier[1]),
                        &(m->interface_type_data.ieee1901.network_identifier[2]),
                        &(m->interface_type_data.ieee1901.network_identifier[3]),
                        &(m->interface_type_data.ieee1901.network_identifier[4]),
                        &(m->interface_type_data.ieee1901.network_identifier[5]),
                        &(m->interface_type_data.ieee1901.network_identifier[6]));
            }
            else if (0 == strcmp(param, "other.oui"))
            {
                sscanf(value, "%02hhx:%02hhx:%02hhx",
                        &(m->interface_type_data.other.oui[0]),
                        &(m->interface_type_data.other.oui[1]),
                        &(m->interface_type_data.other.oui[2]));
            }
            else if (0 == strcmp(param, "other.xml_url"))
            {
                m->interface_type_data.other.generic_phy_description_xml_url = strdup(value);
            }
            else if (0 == strcmp(param, "other.variant_index"))
            {
                sscanf(value, "%hhd", &m->interface_type_data.other.variant_index);
            }
            else if (0 == strcmp(param, "other.variant_name"))
            {
                m->interface_type_data.other.variant_name = strdup(value);
            }
            else if (0 == strcmp(param, "other.ituGhn.dni"))
            {
                sscanf(value, "%02hhx:%02hhx",
                        &(m->interface_type_data.other.media_specific.ituGhn.dni[0]),
                        &(m->interface_type_data.other.media_specific.ituGhn.dni[1]));
            }
            else if (0 == strcmp(param, "other.unsupported.data"))
            {
                char *p;

                p = strtok_r(value, ":", &save_ptr2);

                while (p)
                {
                   if (NULL == m->interface_type_data.other.media_specific.unsupported.bytes)
                   {
                       m->interface_type_data.other.media_specific.unsupported.bytes_nr = 0;
                       m->interface_type_data.other.media_specific.unsupported.bytes    = (uint8_t *)malloc(sizeof(uint8_t) * 1);
                   }
                   else
                   {
                       m->interface_type_data.other.media_specific.unsupported.bytes     = (uint8_t *)realloc(m->interface_type_data.other.media_specific.unsupported.bytes, sizeof(uint8_t) * (m->interface_type_data.other.media_specific.unsupported.bytes_nr + 1));
                   }

                   sscanf(p, "%02hhx", &(m->interface_type_data.other.media_specific.unsupported.bytes[m->interface_type_data.other.media_specific.unsupported.bytes_nr]));

                   m->interface_type_data.other.media_specific.unsupported.bytes_nr++;

                    p = strtok_r(NULL, ":", &save_ptr2);
                }
            }
            else if (0 == strcmp(param, "is_secured"))
            {
                sscanf(value, "%hhd",  &m->is_secured);
            }
            else if (0 == strcmp(param, "push_button_on_going"))
            {
                sscanf(value, "%hhd",  &m->push_button_on_going);
            }
            else if (0 == strcmp(param, "push_button_new_mac_address"))
            {
                sscanf(value, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                        &(m->push_button_new_mac_address[0]),
                        &(m->push_button_new_mac_address[1]),
                        &(m->push_button_new_mac_address[2]),
                        &(m->push_button_new_mac_address[3]),
                        &(m->push_button_new_mac_address[4]),
                        &(m->push_button_new_mac_address[5]));
            }
            else if (0 == strcmp(param, "power_state"))
            {
                if      (0 == strcmp(value, "INTERFACE_POWER_STATE_ON"))
                {
                    m->power_state = INTERFACE_POWER_STATE_ON;
                }
                else if (0 == strcmp(value, "INTERFACE_POWER_STATE_SAVE"))
                {
                    m->power_state = INTERFACE_POWER_STATE_SAVE;
                }
                else if (0 == strcmp(value, "INTERFACE_POWER_STATE_OFF"))
                {
                    m->power_state = INTERFACE_POWER_STATE_OFF;
                }
            }
            else if (0 == strcmp(param, "neighbor_mac_address"))
            {
                if (0 == strcmp(value, "INTERFACE_NEIGHBORS_UNKNOWN"))
                {
                    if (NULL == m->neighbor_mac_addresses)
                    {
                        m->neighbor_mac_addresses_nr = INTERFACE_NEIGHBORS_UNKNOWN;
                    }
                    else
                    {
                        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] _getInterfaceInfoFromSimulatedDevice(): Invalid format (neighbor_mac_addresses)");
                        fclose(fp);
                        return;
                    }
                }
                else
                {
                    if (NULL == m->neighbor_mac_addresses)
                    {
                        m->neighbor_mac_addresses_nr = 0;
                        m->neighbor_mac_addresses    = (uint8_t (*)[6])malloc(sizeof(uint8_t[6]) * 1);
                    }
                    else
                    {
                        m->neighbor_mac_addresses = (uint8_t (*)[6])realloc(m->neighbor_mac_addresses, sizeof(uint8_t[6]) * (m->neighbor_mac_addresses_nr + 1));
                    }

                    sscanf(value, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                            &(m->neighbor_mac_addresses[m->neighbor_mac_addresses_nr][0]),
                            &(m->neighbor_mac_addresses[m->neighbor_mac_addresses_nr][1]),
                            &(m->neighbor_mac_addresses[m->neighbor_mac_addresses_nr][2]),
                            &(m->neighbor_mac_addresses[m->neighbor_mac_addresses_nr][3]),
                            &(m->neighbor_mac_addresses[m->neighbor_mac_addresses_nr][4]),
                            &(m->neighbor_mac_addresses[m->neighbor_mac_addresses_nr][5]));

                    m->neighbor_mac_addresses_nr++;
                }
            }
            else if (0 == strcmp(param, "ipv4"))
            {
                char type[10];

                if (NULL == m->ipv4)
                {
                    m->ipv4_nr = 0;
                    m->ipv4    = (struct _ipv4 *)malloc(sizeof(struct _ipv4));
                }
                else
                {
                    m->ipv4    = (struct _ipv4 *)realloc(m->ipv4, sizeof(struct _ipv4) * (m->ipv4_nr + 1));
                }

                sscanf(value, "%hhd.%hhd.%hhd.%hhd %9s %hhd.%hhd.%hhd.%hhd",
                        &(m->ipv4[m->ipv4_nr].address[0]),
                        &(m->ipv4[m->ipv4_nr].address[1]),
                        &(m->ipv4[m->ipv4_nr].address[2]),
                        &(m->ipv4[m->ipv4_nr].address[3]),
                        type,
                        &(m->ipv4[m->ipv4_nr].dhcp_server[0]),
                        &(m->ipv4[m->ipv4_nr].dhcp_server[1]),
                        &(m->ipv4[m->ipv4_nr].dhcp_server[2]),
                        &(m->ipv4[m->ipv4_nr].dhcp_server[3]));

                if      (0 == strcmp(type, "unknown"))
                {
                    m->ipv4[m->ipv4_nr].type = IPV4_UNKNOWN;
                }
                else if (0 == strcmp(type, "dhcp"))
                {
                    m->ipv4[m->ipv4_nr].type = IPV4_DHCP;
                }
                else if (0 == strcmp(type, "static"))
                {
                    m->ipv4[m->ipv4_nr].type = IPV4_STATIC;
                }
                else if (0 == strcmp(type, "auto"))
                {
                    m->ipv4[m->ipv4_nr].type = IPV4_AUTOIP;
                }

                m->ipv4_nr++;
            }
            else if (0 == strcmp(param, "ipv6"))
            {
                char type[10];

                if (NULL == m->ipv6)
                {
                    m->ipv6_nr = 0;
                    m->ipv6    = (struct _ipv6 *)malloc(sizeof(struct _ipv6));
                }
                else
                {
                    m->ipv6    = (struct _ipv6 *)realloc(m->ipv6, sizeof(struct _ipv6) * (m->ipv6_nr + 1));
                }

                sscanf(value, "%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx %9s %02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx",
                        &(m->ipv6[m->ipv6_nr].address[0]),
                        &(m->ipv6[m->ipv6_nr].address[1]),
                        &(m->ipv6[m->ipv6_nr].address[2]),
                        &(m->ipv6[m->ipv6_nr].address[3]),
                        &(m->ipv6[m->ipv6_nr].address[4]),
                        &(m->ipv6[m->ipv6_nr].address[5]),
                        &(m->ipv6[m->ipv6_nr].address[6]),
                        &(m->ipv6[m->ipv6_nr].address[7]),
                        &(m->ipv6[m->ipv6_nr].address[8]),
                        &(m->ipv6[m->ipv6_nr].address[9]),
                        &(m->ipv6[m->ipv6_nr].address[10]),
                        &(m->ipv6[m->ipv6_nr].address[11]),
                        &(m->ipv6[m->ipv6_nr].address[12]),
                        &(m->ipv6[m->ipv6_nr].address[13]),
                        &(m->ipv6[m->ipv6_nr].address[14]),
                        &(m->ipv6[m->ipv6_nr].address[15]),
                        type,
                        &(m->ipv6[m->ipv6_nr].origin[0]),
                        &(m->ipv6[m->ipv6_nr].origin[1]),
                        &(m->ipv6[m->ipv6_nr].origin[2]),
                        &(m->ipv6[m->ipv6_nr].origin[3]),
                        &(m->ipv6[m->ipv6_nr].origin[4]),
                        &(m->ipv6[m->ipv6_nr].origin[5]),
                        &(m->ipv6[m->ipv6_nr].origin[6]),
                        &(m->ipv6[m->ipv6_nr].origin[7]),
                        &(m->ipv6[m->ipv6_nr].origin[8]),
                        &(m->ipv6[m->ipv6_nr].origin[9]),
                        &(m->ipv6[m->ipv6_nr].origin[10]),
                        &(m->ipv6[m->ipv6_nr].origin[11]),
                        &(m->ipv6[m->ipv6_nr].origin[12]),
                        &(m->ipv6[m->ipv6_nr].origin[13]),
                        &(m->ipv6[m->ipv6_nr].origin[14]),
                        &(m->ipv6[m->ipv6_nr].origin[15]));

                if      (0 == strcmp(type, "unknown"))
                {
                    m->ipv6[m->ipv6_nr].type = IPV6_UNKNOWN;
                }
                else if (0 == strcmp(type, "dhcp"))
                {
                    m->ipv6[m->ipv6_nr].type = IPV6_DHCP;
                }
                else if (0 == strcmp(type, "static"))
                {
                    m->ipv6[m->ipv6_nr].type = IPV6_STATIC;
                }
                else if (0 == strcmp(type, "slaac"))
                {
                    m->ipv6[m->ipv6_nr].type = IPV6_SLAAC;
                }

                m->ipv6_nr++;
            }
            else if (0 == strcmp(param, "oui"))
            {
                if (NULL == m->vendor_specific_elements)
                {
                    m->vendor_specific_elements_nr = 0;
                    m->vendor_specific_elements    = (struct _vendorSpecificInfoElement *)malloc(sizeof(struct _vendorSpecificInfoElement) * 1);
                }
                else
                {
                    m->vendor_specific_elements = (struct _vendorSpecificInfoElement *)realloc(m->vendor_specific_elements, sizeof(struct _vendorSpecificInfoElement) * (m->vendor_specific_elements_nr + 1));
                }

                sscanf(value, "%02hhx %02hhx %02hhx",
                        &(m->vendor_specific_elements[m->vendor_specific_elements_nr].oui[0]),
                        &(m->vendor_specific_elements[m->vendor_specific_elements_nr].oui[1]),
                        &(m->vendor_specific_elements[m->vendor_specific_elements_nr].oui[2]));

                m->vendor_specific_elements[m->vendor_specific_elements_nr].vendor_data_len = 0;
                m->vendor_specific_elements[m->vendor_specific_elements_nr].vendor_data     = NULL;

                m->vendor_specific_elements_nr++;
            }
            else if (0 == strcmp(param, "vendor_data"))
            {
                char *p;

                p = strtok_r(value, ":", &save_ptr2);

                while (p)
                {
                   if (NULL == m->vendor_specific_elements[m->vendor_specific_elements_nr-1].vendor_data)
                   {
                       m->vendor_specific_elements[m->vendor_specific_elements_nr-1].vendor_data_len = 0;
                       m->vendor_specific_elements[m->vendor_specific_elements_nr-1].vendor_data     = (uint8_t *)malloc(sizeof(uint8_t) * 1);
                   }
                   else
                   {
                       m->vendor_specific_elements[m->vendor_specific_elements_nr-1].vendor_data = (uint8_t *)realloc(m->vendor_specific_elements[m->vendor_specific_elements_nr-1].vendor_data, sizeof(uint8_t) * (m->vendor_specific_elements[m->vendor_specific_elements_nr-1].vendor_data_len + 1));
                   }

                   sscanf(p, "%02hhx", &(m->vendor_specific_elements[m->vendor_specific_elements_nr-1].vendor_data[m->vendor_specific_elements[m->vendor_specific_elements_nr-1].vendor_data_len]));

                   m->vendor_specific_elements[m->vendor_specific_elements_nr-1].vendor_data_len++;

                    p = strtok_r(NULL, ":", &save_ptr2);
                }
            }
        }
    }

    fclose(fp);

    return;
}

// Fill the metrics structure with simulation data
//
void _getMetricsFromSimulatedDevice(__attribute__((unused)) char *interface_name, char *simulated_extended_params, struct linkMetrics *m)
{
    FILE  *fp;
    char  *simulation_filename;

    if (NULL == (simulation_filename = index(simulated_extended_params, ':')))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Missing simulation file name in extended params string (%s)\n", simulated_extended_params);
        return;
    }
    simulation_filename++;

    if(NULL == (fp = fopen(simulation_filename, "r")))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] fopen('%s') failed with errno=%d (%s)\n", simulation_filename, errno, strerror(errno));
        return;
    }

    // TODO: Read these values from the "simulation file" too (instead of just
    // assigning them some arbitrary value). This way we open the gates to
    // complex testing scenarios.

    m->measures_window      = 120;

    m->tx_packet_ok         = 10;
    m->tx_packet_errors     = 1;
    m->tx_max_xput          = 120;
    m->tx_phy_rate          = 140;
    m->tx_link_availability = 80;

    m->rx_packet_ok         = 1350;
    m->rx_packet_errors     = 9;
    m->rx_rssi              = 7;


    fclose(fp);

    return;
}

void _startPushButtonOnSimulatedDevice(__attribute__((unused)) char *interface_name, char *simulated_extended_params)
{
    // This function will simply open the simulation file and change the
    // "push_button_on_going" parameter from "0" to "1".
    //
    // It is then the user who *manually* (from the terminal) has to:
    //
    //   1. Fill the "push_button_new_mac_address"
    //   2. Set "push_button_on_going" back to "0"
    //
    // ...to stop the "push button process"

    FILE  *fp1, *fp2;
    char  *simulation_filename;
    char  simulation_filename_tmp[50];

    char   aux[200];

    if (NULL == (simulation_filename = index(simulated_extended_params, ':')))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Missing simulation file name in extended params string (%s)\n", simulated_extended_params);
        return;
    }
    simulation_filename++;

    if(NULL == (fp1 = fopen(simulation_filename, "r")))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] fopen('%s') failed with errno=%d (%s)\n", simulation_filename, errno, strerror(errno));
        return;
    }

    snprintf(simulation_filename_tmp, 50, "%s.tmp", simulation_filename);

    if(NULL == (fp2 = fopen(simulation_filename_tmp, "w+")))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] fopen('%s') failed with errno=%d (%s)\n", simulation_filename_tmp, errno, strerror(errno));
        fclose(fp1);
        return;
    }

    PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] Updating simulated parameters file %s\n", simulation_filename);

    while (NULL != fgets(aux, sizeof(aux), fp1))
    {
        if (NULL == strstr(aux, "push_button_on_going"))
        {
            // Write the line unmodified
            //
            fwrite(aux, strlen(aux), 1, fp2);
        }
        else
        {
            // Change the line so that "push_button_on_going" is now set to "1"
            //
            fwrite("push_button_on_going = 1\n", strlen("push_button_on_going = 1\n"), 1, fp2);
        }
    }

    fclose(fp1);
    fclose(fp2);

    rename(simulation_filename_tmp, simulation_filename);

    return;
}

////////////////////////////////////////////////////////////////////////////////
// Internal API: to be used by other platform-specific files (functions
// declaration is found in "./platform_interfaces_ghnspirit_priv.h")
////////////////////////////////////////////////////////////////////////////////

void registerSimulatedInterfaceType(void)
{
    registerInterfaceStub("simulated", STUB_TYPE_GET_INFO,          _getInterfaceInfoFromSimulatedDevice);
    registerInterfaceStub("simulated", STUB_TYPE_GET_METRICS,       _getMetricsFromSimulatedDevice);
    registerInterfaceStub("simulated", STUB_TYPE_PUSH_BUTTON_START, _startPushButtonOnSimulatedDevice);
}


