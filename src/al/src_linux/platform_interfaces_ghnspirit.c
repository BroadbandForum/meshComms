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
#include "platform_interfaces_priv.h"           // registerInterfaceStub
#include "platform_interfaces_ghnspirit_priv.h"

#include <stdio.h>   // popen()
#include <stdlib.h>  // ssize_t
#include <string.h>  // strdup()
#include <errno.h>   // errno
#include <pthread.h> // mutex functions
#include <unistd.h>  // sleep()


////////////////////////////////////////////////////////////////////////////////
// Private data and functions
////////////////////////////////////////////////////////////////////////////////

// Mutex to avoid concurrent modem LCMP access
//
pthread_mutex_t lcmp_mutex = PTHREAD_MUTEX_INITIALIZER;

// These are static values to fill the "interface_type_data" field
//
static uint8_t itu_ghn_oui[]                 = {0x00, 0x19, 0xa7};
static char  itu_ghn_generic_phy_xml_url[] = "http://handle.itu.int/11.1002/3000/1706";

#define VARIANT_POWERLINE (0x00)
#define VARIANT_PHONELINE (0x01)
#define VARIANT_COAX_BASE (0x02)
#define VARIANT_COAX_RF   (0x03)
#define VARIANT_POF       (0x04)

static char *variant_names[] = {
  "ITU-T G.996x Powerline",
  "ITU-T G.996x Phoneline",
  "ITU-T G.996x Coax Baseband",
  "ITU-T G.996x Coax RF",
  "ITU-T G.996x Plastic Optical Fiber (POF)",
};

// Given a string such as this one ('ghnspirit_extended_params'):
//
//   ghnspirit:001122334455:bluemoon
//
// ...returns two pointers to the second and third token:
//
//   ghn_mac_address -> 001122334455
//   lcmp_password   -> bluemon
//
// If there is a problem this function returns '0' and the caller does not
// have to do anything.
//
// If everything works ok this function returns '1' and the caller is responsible
// for freeing ("free()") 'ghn_mac_address' and 'lcmp_password'
//
int _extractMacAndPassword(char *ghnspirit_extended_params, char **ghn_mac_address, char **lcmp_password)
{
    char *aux;
    char *saveptr;

    // Extract the "MAC adddress" and LCMP password from the
    // "ghnspirit_extended_params" string
    //
    aux              = strdup(ghnspirit_extended_params);
    *ghn_mac_address = strtok_r(aux,  ":", &saveptr);
    *ghn_mac_address = strtok_r(NULL, ":", &saveptr);

    if (NULL == *ghn_mac_address)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] MAC address of G.hn/Spirit device not specified (%s)\n", ghnspirit_extended_params);
        free(aux);
        return 0;
    }

    *lcmp_password = strtok_r(NULL, ":", &saveptr);

    if (NULL == *lcmp_password)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] LCMP password of G.hn/Spirit device not specified (%s)\n", ghnspirit_extended_params);
        free(aux);
        return 0;
    }

    *ghn_mac_address = strdup(*ghn_mac_address);
    *lcmp_password   = strdup(*lcmp_password);
    free(aux);

    return 1;
}

// Obtain information from the G.hn/Spirit device connected to interface
// 'interface_name' and fill the 'm' structure.
//
// The 'ghnspirit_extended_params' is a string with the following format:
//
//   ghnspirit:<ghn_mac_address>:<lcmp_password>
//
// Example:
//
//   ghnspirit:00139d04ff54:bluemoon
//
// This function will then use an external tool to query the G.hn device (which
// is has the given MAC address) using the LCMP protocol (with the given
// password) and fill the 'm' structure with the results.
//
void _getInterfaceInfoFromGhnSpiritDevice(char *interface_name, char *ghnspirit_extended_params, struct interfaceInfo *m)
{
    #define MAX_COMMAND_SIZE 1024
    #define LCMP_CONFIGLAYER_GETINFO_COMMAND "configlayer -i %s -m %s -o GET "          \
                                             "-p SYSTEM.PRODUCTION.MAC_ADDR "           \
                                             "-p SYSTEM.PRODUCTION.DEVICE_MANUFACTURER "\
                                             "-p SYSTEM.PRODUCTION.HW_PRODUCT "         \
                                             "-p SYSTEM.PRODUCTION.HW_REVISION "        \
                                             "-p SYSTEM.PRODUCTION.SERIAL_NUMBER "      \
                                             "-p SYSTEM.PRODUCTION.DEVICE_NAME "        \
                                             "-p NODE.GENERAL.DNI "                     \
                                             "-p PAIRING.GENERAL.SECURED "              \
                                             "-p PAIRING.GENERAL.PROCESS_START "        \
                                             "-p POWERSAVING.GENERAL.STATUS "           \
                                             "-p DHCP.GENERAL.ENABLED_IPV4 "            \
                                             "-p DHCP.GENERAL.SERVER_IPV4 "             \
                                             "-p DHCP.GENERAL.ENABLED_IPV6 "            \
                                             "-p TCPIP.IPV4.IP_ADDRESS "                \
                                             "-p TCPIP.IPV6.IP_ADDRESS "                \
                                             "-p DIDMNG.GENERAL.MACS "                  \
                                             "-w %s"

    FILE* pipe ;

    char command[MAX_COMMAND_SIZE];

    char *ghn_mac_address;
    char *lcmp_password;

    char    *line;
    size_t   len;
    ssize_t  read;

    int   dhcp_enabled_parsed = 0;
    int   dhcp_server_parsed  = 0;
    char *dhcp_server         = NULL;
    int   dhcp_enabled        = 0;

    int   dhcpv6_enabled_parsed = 0;
    int   dhcpv6_enabled        = 0;

    m->interface_type = INTERFACE_TYPE_UNKNOWN; // This is the type assigned to
                                                // "generic PHYs" (ie. anything
                                                // that is not 802.3, 802.11,
                                                // 1901 or MoCA.

    m->interface_type_data.other.oui[0]                          = itu_ghn_oui[0];
    m->interface_type_data.other.oui[1]                          = itu_ghn_oui[1];
    m->interface_type_data.other.oui[2]                          = itu_ghn_oui[2];
    m->interface_type_data.other.generic_phy_description_xml_url = strdup(itu_ghn_generic_phy_xml_url);

    // TODO: Obtain the actual variant from CFL parameters (maybe
    // "PHYMNG.GENERAL.RUNNING_PHYMODE_ID" and "PHYMNG.GENERAL.PHYMODE_ID_INFO"
    //
    {
        uint8_t variant = VARIANT_POWERLINE;

        m->interface_type_data.other.variant_index = variant;
        m->interface_type_data.other.variant_name  = strdup(variant_names[variant]);
    }

    // Obtain G.hn/Spirit MAC address and LCMP password
    //
    if (0 == _extractMacAndPassword(ghnspirit_extended_params, &ghn_mac_address, &lcmp_password))
    {
        return;
    }

    // "ghn_mac_address" contains the G.hn device MAC address and
    // "lcmp_password" contains the password to query for G.hn parameters using
    // the LCMP tool.
    // Let's build the "command" that inccorporates this information:
    //
    snprintf(command, MAX_COMMAND_SIZE, LCMP_CONFIGLAYER_GETINFO_COMMAND, interface_name, ghn_mac_address, lcmp_password);
    free(ghn_mac_address);
    free(lcmp_password);

    PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] Querying G.hn device using the LCMP tool:\n");
    PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM]   > %s\n", command);

    // Execute the LCMP query tool.
    //
    pthread_mutex_lock(&lcmp_mutex);
    pipe = popen(command, "r");

    if (!pipe)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] popen() returned with errno=%d (%s)\n", errno, strerror(errno));
        pthread_mutex_unlock(&lcmp_mutex);
        return;
    }

    // Next read/fill the rest of parameters
    //
    line = NULL;
    while (-1 != (read = getline(&line, &len, pipe)))
    {
        char *value;

        // Remove the last "\n"
        //
        line[strlen(line)-1] = 0x00;

        // Each line returned by the LCMP tools has the following format:
        //
        //   <PARAM_NAME>=<PARAM_VALUE>

        // Find the "=" sign
        //
        if (NULL == (value = index(line, '=')))
        {
            continue;
        }

        // ...and advance to the next character (this is where the parameter
        // argument starts
        //
        value++;

        if      (0 == strncmp(line, "SYSTEM.PRODUCTION.MAC_ADDR=", strlen("SYSTEM.PRODUCTION.MAC_ADDR=")))
        {
            sscanf(value, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                   &(m->mac_address[0]),
                   &(m->mac_address[1]),
                   &(m->mac_address[2]),
                   &(m->mac_address[3]),
                   &(m->mac_address[4]),
                   &(m->mac_address[5]));
        }
        else if (0 == strncmp(line, "SYSTEM.PRODUCTION.DEVICE_MANUFACTURER=", strlen("SYSTEM.PRODUCTION.DEVICE_MANUFACTURER=")))
        {
            memcpy(m->manufacturer_name, value,  strlen(value)+1);
        }
        else if (0 == strncmp(line, "SYSTEM.PRODUCTION.HW_PRODUCT=", strlen("SYSTEM.PRODUCTION.HW_PRODUCT=")))
        {
            memcpy(m->model_name, value,  strlen(value)+1);
        }
        else if (0 == strncmp(line, "SYSTEM.PRODUCTION.HW_REVISION=", strlen("SYSTEM.PRODUCTION.HW_REVISION=")))
        {
            memcpy(m->model_number, value,  strlen(value)+1);
        }
        else if (0 == strncmp(line, "SYSTEM.PRODUCTION.SERIAL_NUMBER=", strlen("SYSTEM.PRODUCTION.SERIAL_NUMBER=")))
        {
            memcpy(m->serial_number, value,  strlen(value)+1);
        }
        else if (0 == strncmp(line, "SYSTEM.PRODUCTION.DEVICE_NAME=", strlen("SYSTEM.PRODUCTION.DEVICE_NAME=")))
        {
            memcpy(m->device_name, value,  strlen(value)+1);
        }
        //else if (0 == strncmp(line, "???", strlen("???")))
        //{
        //    TODO: There is no G.hn parameter that represents the UUID
        //    memcpy(m->uuid, value,  strlen(value)+1);
        //}
        else if (0 == strncmp(line, "NODE.GENERAL.DNI=", strlen("NODE.GENERAL.DNI=")))
        {
            int dni;

            sscanf(value, "%d", &dni);

            m->interface_type_data.other.media_specific.ituGhn.dni[0] = dni/256; // MSB
            m->interface_type_data.other.media_specific.ituGhn.dni[1] = dni%256; // LSB
        }
        else if (0 == strncmp(line, "PAIRING.GENERAL.SECURED=", strlen("PAIRING.GENERAL.SECURED=")))
        {
            if (value[0] == 'N' && value[1] == 'O')
            {
               m->is_secured = 0;
            }
            else
            {
               m->is_secured = 1;
            }
        }
        else if (0 == strncmp(line, "PAIRING.GENERAL.PROCESS_START=", strlen("PAIRING.GENERAL.PROCESS_START=")))
        {
            if (value[0] == '1')
            {
               m->push_button_on_going = 1;
            }
            else
            {
               m->push_button_on_going = 0;
            }
        }
        //else if (0 == strncmp(line, "???", strlen("???")))
        //{
        //    TODO: There is no G.hn parameter that represents the MAC address
        //    of the last device that joined the network
        //    sscanf(value, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
        //            &(m->push_button_new_mac_address[0]),
        //            &(m->push_button_new_mac_address[1]),
        //            &(m->push_button_new_mac_address[2]),
        //            &(m->push_button_new_mac_address[3]),
        //            &(m->push_button_new_mac_address[4]),
        //            &(m->push_button_new_mac_address[5]));
        //}
        else if (0 == strncmp(line, "POWERSAVING.GENERAL.STATUS=", strlen("POWERSAVING.GENERAL.STATUS=")))
        {
            // G.hn values:
            //   - 0 : Full-power mode (L0). It can transmit and receive
            //         network traffic at its maximum capacity.
            //   - 1 : Efficient-power mode (L1). It can transmit and receive
            //         network traffic at its maximum capacity.
            //   - 2 : Low-power mode (L2). It can transmit and receive network
            //         traffic at reduced capacity.
            //   - 3 : Idle mode (L3). It cannot transmit and receive network
            //         traffic, but may transmit and receive management traffic.
            //   - 4 : The node is down.
            //   - 5 : The node is turned on, but not ready to transmit and
            //         receive network traffic.
            //   - 6 : The node is in a fault/error condition.
            //
            // 1905 values:
            //
            //   - ON
            //   - SAVE
            //   - OFF
            //
            // We will use the following mappings:
            //
            //   ON   <--> 0
            //   SAVE <--> 1, 2
            //   OFF  <--> 3, 4, 5, 6
            //
            if ('0' == value[0])
            {
                m->power_state = INTERFACE_POWER_STATE_ON;
            }
            else if ('1' == value[0] || '2' == value[0])
            {
                m->power_state = INTERFACE_POWER_STATE_SAVE;
            }
            else
            {
                m->power_state = INTERFACE_POWER_STATE_OFF;
            }
        }
        else if (0 == strncmp(line, "DHCP.GENERAL.ENABLED_IPV4=", strlen("DHCP.GENERAL.ENABLED_IPV4=")))
        {
            dhcp_enabled_parsed = 1;

            if (value[0] == 'N')
            {
                dhcp_enabled = 0;
            }
            else
            {
                dhcp_enabled = 1;
            }
        }
        else if (0 == strncmp(line, "DHCP.GENERAL.SERVER_IPV4=", strlen("DHCP.GENERAL.SERVER_IPV4=")))
        {
            dhcp_server_parsed = 1;

            dhcp_server = strdup(value);
        }
        else if (0 == strncmp(line, "DHCP.GENERAL.ENABLED_IPV6=", strlen("DHCP.GENERAL.ENABLED_IPV6=")))
        {
            dhcpv6_enabled_parsed = 1;

            if (value[0] == 'N')
            {
                dhcpv6_enabled = 0;
            }
            else
            {
                dhcpv6_enabled = 1;
            }
        }
        else if (0 == strncmp(line, "TCPIP.IPV4.IP_ADDRESS=", strlen("TCPIP.IPV4.IP_ADDRESS=")))
        {
            // This parameter *must* be parsed *after* these other two:
            //
            //   - DHCP.GENERAL.ENABLED_IPV4
            //   - DHCP.GENERAL.SERVER_IPV4
            //
            // We can achieve this by modifying the LCMP command line so that
            // parameter these parameters appear *before* this other one.
            //
            if (0 == dhcp_enabled_parsed || 0 == dhcp_server_parsed)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Problems obtaining the IPv4 type. Check the order of parameters in the LCMP command!\n");
            }

            m->ipv4_nr = 1;
            m->ipv4    = (struct _ipv4 *)malloc(sizeof(struct _ipv4));

            sscanf(value, "%hhd.%hhd.%hhd.%hhd",
                    &(m->ipv4[0].address[0]),
                    &(m->ipv4[0].address[1]),
                    &(m->ipv4[0].address[2]),
                    &(m->ipv4[0].address[3]));

            if (1 == dhcp_enabled)
            {
                m->ipv4[0].type = IPV4_DHCP;
            }
            else
            {
                m->ipv4[0].type = IPV4_STATIC;
            }

            if (0 != dhcp_server_parsed)
            {
                sscanf(dhcp_server, "%hhd.%hhd.%hhd.%hhd",
                        &(m->ipv4[0].dhcp_server[0]),
                        &(m->ipv4[0].dhcp_server[1]),
                        &(m->ipv4[0].dhcp_server[2]),
                        &(m->ipv4[0].dhcp_server[3]));
            }

            // TODO: Consider additional IPv4 addresses
            // ("TCPIP.IPV4.ADDITIONAL_IP_ADDRESS")
        }
        else if (0 == strncmp(line, "TCPIP.IPV6.IP_ADDRESS=", strlen("TCPIP.IPV6.IP_ADDRESS=")))
        {
            // This parameter *must* be parsed *after* this other one:
            //
            //   - DHCP.GENERAL.ENABLED_IPV6
            //
            // We can achieve this by modifying the LCMP command line so that
            // parameter these parameters appear *before* this other one.
            //
            if (0 == dhcpv6_enabled_parsed)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Problems obtaining the IPv6 type. Check the order of parameters in the LCMP command!\n");
            }

            m->ipv6_nr = 1;
            m->ipv6    = (struct _ipv6 *)malloc(sizeof(struct _ipv6));

            sscanf(value, "%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx",
                    &(m->ipv6[0].address[0]),
                    &(m->ipv6[0].address[1]),
                    &(m->ipv6[0].address[2]),
                    &(m->ipv6[0].address[3]),
                    &(m->ipv6[0].address[4]),
                    &(m->ipv6[0].address[5]),
                    &(m->ipv6[0].address[6]),
                    &(m->ipv6[0].address[7]),
                    &(m->ipv6[0].address[8]),
                    &(m->ipv6[0].address[9]),
                    &(m->ipv6[0].address[10]),
                    &(m->ipv6[0].address[11]),
                    &(m->ipv6[0].address[12]),
                    &(m->ipv6[0].address[13]),
                    &(m->ipv6[0].address[14]),
                    &(m->ipv6[0].address[15]));

            if (1 == dhcpv6_enabled)
            {
                m->ipv6[0].type = IPV6_DHCP;
            }
            else
            {
                m->ipv6[0].type = IPV4_STATIC;
            }

            // TODO: How do we fill this?
            //
            m->ipv6[0].origin[0]  = 0x00;
            m->ipv6[0].origin[1]  = 0x00;
            m->ipv6[0].origin[2]  = 0x00;
            m->ipv6[0].origin[3]  = 0x00;
            m->ipv6[0].origin[4]  = 0x00;
            m->ipv6[0].origin[5]  = 0x00;
            m->ipv6[0].origin[6]  = 0x00;
            m->ipv6[0].origin[7]  = 0x00;
            m->ipv6[0].origin[8]  = 0x00;
            m->ipv6[0].origin[9]  = 0x00;
            m->ipv6[0].origin[10] = 0x00;
            m->ipv6[0].origin[11] = 0x00;
            m->ipv6[0].origin[12] = 0x00;
            m->ipv6[0].origin[13] = 0x00;
            m->ipv6[0].origin[14] = 0x00;
            m->ipv6[0].origin[15] = 0x00;

            // TODO: Consider additional IPv4 addresses
            // ("TCPIP.IPV6.ADDITIONAL_IP_ADDRESS")
        }
        else if (0 == strncmp(line, "DIDMNG.GENERAL.MACS=", strlen("DIDMNG.GENERAL.MACS=")))
        {
            char *saveptr;
            char *aux;

            uint8_t mac_addr[6];

            // The G.hn modem reports *all* G.hn neighbor MACs (including our
            // own MAC address). That's why we must be aware of which is our own
            // MAC and remove it from the list.
            // In order for this to work, parameter "SYSTEM.PRODUCTION.MAC_ADDR"
            // must have been parsed *before* "DIDMNG.GENERAL.MACS" or else the
            // "m->mac_address" field will be empty.
            // We can achieve this by modifying the LCMP command line so that
            // parameter "SYSTEM.PRODUCTION.MAC_ADDR" is reported first.
            //
            if (
                 m->mac_address[0] == 0x0 &&
                 m->mac_address[1] == 0x0 &&
                 m->mac_address[2] == 0x0 &&
                 m->mac_address[3] == 0x0 &&
                 m->mac_address[4] == 0x0 &&
                 m->mac_address[5] == 0x0
               )
            {
                PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Local MAC address will not be removed from the list of neighbors. Check the order of parameters in the LCMP command!\n");
            }

            // This parameter's value is reported as a comma separated list
            // of MAC addresses
            //
            for (aux=value; ;aux=NULL)
            {
                char *mac_str;

                mac_str = strtok_r(aux, ",", &saveptr);

                if (NULL == mac_str)
                {
                    break;
                }

                sscanf(mac_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                       &(mac_addr[0]),
                       &(mac_addr[1]),
                       &(mac_addr[2]),
                       &(mac_addr[3]),
                       &(mac_addr[4]),
                       &(mac_addr[5]));

                if (0 != memcmp(m->mac_address, mac_addr, 6))
                {
                    // This is not our MAC address. Add it.
                    //
                    if (NULL == m->neighbor_mac_addresses)
                    {
                        m->neighbor_mac_addresses_nr = 0;
                        m->neighbor_mac_addresses    = (uint8_t (*)[6])malloc(sizeof(uint8_t[6]) * 1);
                    }
                    else
                    {
                        m->neighbor_mac_addresses = (uint8_t (*)[6])realloc(m->neighbor_mac_addresses, sizeof(uint8_t[6]) * (m->neighbor_mac_addresses_nr + 1));
                    }
                    memcpy(m->neighbor_mac_addresses[m->neighbor_mac_addresses_nr], mac_addr, 6);

                    m->neighbor_mac_addresses_nr++;
                }
            }
        }
    }


    if (line)
    {
        free(line);
    }
    if (0 != dhcp_server_parsed)
    {
        free(dhcp_server);
    }

    pclose(pipe);
    pthread_mutex_unlock(&lcmp_mutex);

    return;
}

// This function works in the same way as the previous function
// ("_getInterfaceInfoFromGhnSpiritDevice()"), but this time we keep those
// parameters containing metrics information.
//
void _getMetricsFromGhnSpiritDevice(char *interface_name, char *ghnspirit_extended_params, struct linkMetrics *m)
{
    #define LCMP_CONFIGLAYER_GETMETRICS_COMMAND "configlayer -i %s -m %s -o GET -p QOS.STATS.G9962 -p BFT.GENERAL.MACS_INFO_DESC -p BFT.GENERAL.MACS_INFO -p DIDMNG.GENERAL.DIDS -p DIDMNG.GENERAL.TX_BPS -w %s"

    FILE* pipe ;

    char command[200];

    char *ghn_mac_address;
    char *lcmp_password;

    char    *line;
    size_t   len;
    ssize_t  read;
    ssize_t  dest_id       = -1; // not found
    ssize_t  didmng_index  = -1; // not found
    ssize_t  bft_row_size  = -1; // not found
    ssize_t  bft_mac_index = -1; // not found
    ssize_t  bft_did_index = -1; // not found


    // Obtain G.hn/Spirit MAC address and LCMP password
    //
    if (0 == _extractMacAndPassword(ghnspirit_extended_params, &ghn_mac_address, &lcmp_password))
    {
        return;
    }

    // "ghn_mac_address" contains the G.hn device MAC address and
    // "lcmp_password" contains the password to query for G.hn parameters using
    // the LCMP tool.
    // Let's build the "command" that incorporates this information:
    //
    sprintf(command, LCMP_CONFIGLAYER_GETMETRICS_COMMAND, interface_name, ghn_mac_address, lcmp_password);
    free(ghn_mac_address);
    free(lcmp_password);

    PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] Querying G.hn device using the LCMP tool:\n");
    PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM]   > %s\n", command);

    // Execute the LCMP query tool.
    //
    pthread_mutex_lock(&lcmp_mutex);
    pipe = popen(command, "r");

    if (!pipe)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] popen() returned with errno=%d (%s)\n", errno, strerror(errno));
        pthread_mutex_unlock(&lcmp_mutex);
        return;
    }

    // Next read/fill the rest of parameters
    //
    line = NULL;
    while (-1 != (read = getline(&line, &len, pipe)))
    {
        char *value;

        // Remove the last "\n"
        //
        line[strlen(line)-1] = 0x00;

        // Each line returned by the LCMP tools has the following format:
        //
        //   <PARAM_NAME>=<PARAM_VALUE>

        // Find the "=" sign
        //
        if (NULL == (value = index(line, '=')))
        {
            continue;
        }

        // ...and advance to the next character (this is where the parameter
        // argument starts
        //
        value++;

        if      (0 == strncmp(line, "QOS.STATS.G9962=", strlen("QOS.STATS.G9962=")))
        {
            int bytes_tx, bytes_rx, pkts_tx, pkts_rx, errors_tx, errors_rx;

            sscanf(value, "%d,%d,%d,%d,%d,%d,", &bytes_tx, &bytes_rx, &pkts_tx, &pkts_rx, &errors_tx, &errors_rx);

            m->tx_packet_ok         = pkts_tx;
            m->tx_packet_errors     = errors_tx;

            m->rx_packet_ok         = pkts_rx;
            m->rx_packet_errors     = errors_rx;
        }
        // The 'Phyrate' metric is computed from the DIDMNG.GENERAL.TX_BPS CFL
        // parameter
        //
        // We need to find the DIDMNG.GENERAL.* index corresponding to the
        // requested mac. Example:
        //
        //   Requested mac           = 00:13:9d:00:11:1b -->
        //   DIDMNG.GENERAL.MACS.2   = 00:13:9d:00:11:1b --> index = 2
        //   phyrate = f(DIDMNG.GENERAL.TX_BPS.2)
        //   ... where f(x) = (x * 32000) / 1000000;
        //
        //   The problem is that the 'configlayer' tool output is not indexed
        //   and DIDMNG.GENERAL.MACS.* elements with zero values are filtered.
        //   This breaks any kind of correlation between DIDMNG.GENERAL.MAC
        //   and the other DIDMNG.GENERAL.* parameters.
        //
        //   Current solution:
        //   - get 'BFT.GENERAL.MACS_INFO' to find the DID corresponding to the
        //     requested mac
        //   - get 'DIDMNG.GENERAL.DIDS' to find the DIDMNG.GENERAL index corresponding
        //     to this DID value
        //   - get 'DIDMNG.GENERAL.TX_BPS.index' to compute the 'phyrate' value
        //
        //   Another possible solution would be to iterate over DIDMNG.GENERAL.MACS
        //   to find the DIDMNG.GENERAL index corresponding to the neighbor
        //   interface mac. The drawback is that it needs more LCMP queries.
        //   configlayer ..... -p DIDMNG.GENERAL.MACS.1
        //   configlayer ..... -p DIDMNG.GENERAL.MACS.2
        //   ...
        //

        // Get BFT.GENERAL.MACS_INFO field description
        //
        else if (0 == strncmp(line, "BFT.GENERAL.MACS_INFO_DESC=", strlen("BFT.GENERAL.MACS_INFO_DESC=")))
        {
            char   *aux;
            char   *str;
            uint8_t   i;

            // BFT.GENERAL.MACS_INFO parameter is a table where each
            // row brings together a group of different data:
            //       MAC address   port   destination id   ...   port type
            // Bytes:   0-5         6          7           ...      10
            //
            i = 0;
            aux = value;
            do
            {
                char   *saveptr;

                str = strtok_r(aux, ",", &saveptr);
                aux = NULL;

                if (NULL != str)
                {
                    if (NULL != strstr(str, "MAC byte5"))
                    {
                       bft_mac_index = i;
                    }
                    else if (NULL != strstr(str, "Destination ID"))
                    {
                       bft_did_index = i;
                    }

                    i++;
                }

            }
            while (str != NULL);

            bft_row_size = i;
        }
        else if (0 == strncmp(line, "BFT.GENERAL.MACS_INFO=", strlen("BFT.GENERAL.MACS_INFO=")))
        {
            uint8_t  *elements;

            if ((bft_row_size >= 0) && (bft_mac_index >= 0) && (bft_did_index >= 0))
            {
                char   *aux;

                elements = (uint8_t *)malloc(bft_row_size);

                // Find the DID value corresponding to the neighbor interface mac address
                //
                // This parameter's value is reported as a comma separated list
                // of several parameters: MAC addresses, destination id, ...
                //
                dest_id = -1;
                for (aux=value; dest_id<0 ;aux=NULL)
                {
                    // BFT.GENERAL.MACS_INFO is a table of 1152x11 bytes. Each row
                    // (11 elements) stores a mixture of data: mac address, port,
                    // destination id, ...
                    // The first 6 bytes correspond to the mac, then the port (1 byte),
                    // the did (1 byte), etc...
                    //
                    uint8_t   i;
                    uint8_t   did;
                    int     value;
                    uint8_t  *mac_addr;
                    char   *str;
                    char   *saveptr;

                    for (i=0; i < bft_row_size; i++)
                    {
                        str = strtok_r(aux, ",", &saveptr);
                        aux=NULL;

                        if (NULL == str)
                        {
                            // No more available data
                            //
                            break;
                        }

                        sscanf(str, "%d", &value);
                        elements[i] = value;
                    }

                    if (NULL == str)
                    {
                        // No more available data
                        //
                        break;
                    }

                    mac_addr = &elements[bft_mac_index];
                    did      =  elements[bft_did_index];
                    if (0 == memcmp(m->neighbor_interface_address, mac_addr, 6))
                    {
                        dest_id = did;
                    }
                }

                if (elements)
                {
                    free(elements);
                }
            }
            else
            {
                PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Unknown BFT.GENERAL.MACS_INFO dimensions!\n");
            }
        }
        else if (0 == strncmp(line, "DIDMNG.GENERAL.DIDS=", strlen("DIDMNG.GENERAL.DIDS=")))
        {
            // Find the DIDMNG.GENERAL index corresponding to the destination id
            // associated with the requested neighbor interface MAC address
            // Note: 'destination id' was obtained from BFT.GENERAL.MACS_INFO
            //
            // This parameter's value is reported as a comma separated list
            // of destination ids
            //
            if (dest_id >= 0)
            {
                char *aux;
                uint8_t index;

                didmng_index = -1;
                index        =  1; // CFL index starts at 1 (not 0)
                for (aux=value; didmng_index<0 ;aux=NULL, index++)
                {
                    char *saveptr;
                    char *str;
                    int   did;

                    str = strtok_r(aux, ",", &saveptr);

                    if (NULL == str)
                    {
                        // No more available data
                        //
                        break;
                    }

                    sscanf(str, "%d", &did);

                    if (dest_id == did)
                    {
                        didmng_index = index;
                    }
                }
            }
            else
            {
                PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Unknown destination id. Check the order of parameters in the LCMP command!\n");
            }
        }
        else if (0 == strncmp(line, "DIDMNG.GENERAL.TX_BPS=", strlen("DIDMNG.GENERAL.TX_BPS=")))
        {
            uint32_t  tx_bps;

            // Compute Phyrate based on DIDMNG.GENERAL.TX_BPS.index value
            // Note: 'DIDMNG.GENERAL index' was obtained from DIDMNG.GENERAL.DIDS
            //
            // This parameter's value is reported as a comma separated list
            // of XPUT values
            //
            tx_bps = 0;
            if (didmng_index >= 0)
            {
                char   *aux;
                char   *str;
                uint8_t   index;

                aux = value;
                for (index=1; index<=didmng_index ;index++)
                {
                    char   *saveptr;

                    str = strtok_r(aux, ",", &saveptr);
                    if (NULL == str)
                    {
                        // No more available data
                        //
                        break;
                    }
                    aux = NULL;
                }

                if (NULL != str)
                {
                    sscanf(str, "%u", &tx_bps);
                }
            }
            else
            {
                PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Unknown destination id. Check the order of parameters in the LCMP command!\n");
            }

            m->tx_phy_rate = (tx_bps * 32000) / 1000000;
            m->tx_max_xput = (70*m->tx_phy_rate)/100;
        }
    }

    // TODO: Hardcoded value for now (9% is used for signalization).
    //
    m->tx_link_availability = 91;

    // According to the standard (Table 6-20), this field must be set to 0xff
    // for non IEEE 802.11 links
    //
    m->rx_rssi = 0xff;

    if (line)
    {
        free(line);
    }

    pclose(pipe);
    pthread_mutex_unlock(&lcmp_mutex);

    return;
}

void _startPushButtonOnGhnSpiritDevice(char *interface_name, char *ghnspirit_extended_params)
{
    #define LCMP_CONFIGLAYER_START_PAIRING_COMMAND "configlayer -i %s -m %s -o SET -p PAIRING.GENERAL.PROCESS_START=1 -w %s"

    FILE* pipe ;

    char command[100];

    char *ghn_mac_address;
    char *lcmp_password;

    // Obtain G.hn/Spirit MAC address and LCMP password
    //
    if (0 == _extractMacAndPassword(ghnspirit_extended_params, &ghn_mac_address, &lcmp_password))
    {
        return;
    }

    // "ghn_mac_address" contains the G.hn device MAC address and
    // "lcmp_password" contains the password to query for G.hn parameters using
    // the LCMP tool.
    // Let's build the "command" that inccorporates this information:
    //
    sprintf(command, LCMP_CONFIGLAYER_START_PAIRING_COMMAND, interface_name, ghn_mac_address, lcmp_password);
    free(ghn_mac_address);
    free(lcmp_password);

    PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] Using the LCMP tool to instruct the G.hn device to start its pairing process\n");
    PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM]   > %s\n", command);

    // Execute the LCMP tool.
    //
    pthread_mutex_lock(&lcmp_mutex);
    pipe = popen(command, "r");

    if (!pipe)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] popen() returned with errno=%d (%s)\n", errno, strerror(errno));
        pthread_mutex_unlock(&lcmp_mutex);
        return;
    }

    // TODO: Maybe we should parse the tool output here to find out if the
    // command executed OK

    pclose(pipe);
    pthread_mutex_unlock(&lcmp_mutex);

    // The G.hn modem might need a few seconds to actually start the pairing
    // process. In order to be sure that the process has started before this
    // function returns we need to pause for a while.
    //
    //   TODO: A possible improvement for this is to query the modem inside a
    //   while loop until the response indicates that the pairing process has
    //   actually started.
    //
    sleep(5);

    return;
}


////////////////////////////////////////////////////////////////////////////////
// Internal API: to be used by other platform-specific files (functions
// declaration is found in "./platform_interfaces_ghnspirit_priv.h")
////////////////////////////////////////////////////////////////////////////////

void registerGhnSpiritInterfaceType(void)
{
    registerInterfaceStub("ghnspirit", STUB_TYPE_GET_INFO,          _getInterfaceInfoFromGhnSpiritDevice);
    registerInterfaceStub("ghnspirit", STUB_TYPE_GET_METRICS,       _getMetricsFromGhnSpiritDevice);
    registerInterfaceStub("ghnspirit", STUB_TYPE_PUSH_BUTTON_START, _startPushButtonOnGhnSpiritDevice);
}
