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
#include "utils.h"
#include "1905_alme.h"

#include <stdio.h>   // printf
#include <unistd.h>  // getopt
#include <stdlib.h>  // exit
#include <string.h>  // strtok

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
#    include <arpa/inet.h>  // socket(), AF_INET, htons(), ...
#else
#    include <winsock2.h>
#endif

#include <errno.h>      // errno

////////////////////////////////////////////////////////////////////////////////
// Static (auxiliary) private functions, structures and macros
////////////////////////////////////////////////////////////////////////////////

// Convert a character to lower case
//
static char _asciiToLowCase (char c)
{
    if (c >= 'a' && c <= 'z')
    {
        return c;
    }
    else if (c >= 'A' && c <= 'Z')
    {
        return c + ('a' - 'A');
    }
    else
    {
        return c;
    }
}

// Convert a MAC string representation (example: "0a:fa:41:a3:ff:40") into a
// six bytes array (example: {0x0a, 0xfa, 0x41, 0xa3, 0xff, 0x40})
//
static void _asciiToMac (const char *str, uint8_t *addr)
{
    int i = 0;

    if (NULL == str)
    {
        addr[0] = 0x00;
        addr[1] = 0x00;
        addr[2] = 0x00;
        addr[3] = 0x00;
        addr[4] = 0x00;
        addr[5] = 0x00;

        return;
    }

    while (0x00 != *str && i < 6)
    {
        uint8_t byte = 0;

        while (0x00 != *str && ':' != *str)
        {
            char low;

            byte <<= 4;
            low    = _asciiToLowCase (*str);

            if (low >= 'a')
            {
                byte |= low - 'a' + 10;
            }
            else
            {
                byte |= low - '0';
            }
            str++;
        }

        addr[i] = byte;
        i++;

        if (*str == 0)
        {
            break;
        }

        str++;
      }
}


// Return a properly filled structure representing the desired ALME REQUEST
// Some types of ALME requests require arguments. These are taken from the
// arguments the executable was called with.
//
uint8_t *_build_alme_request(char *alme_request_type, int argc, char **argv)
{
    uint8_t *ret = NULL;

    if      (0 == strcmp(alme_request_type, "ALME-GET-INTF-LIST.request"))
    {
        struct getIntfListRequestALME *p;

        p = (struct getIntfListRequestALME *)malloc(sizeof(struct getIntfListRequestALME));
        if (NULL == p)
        {
            PLATFORM_PRINTF_DEBUG_ERROR("Could not allocate getIntfListRequestALME structure!\n");
            return NULL;
        }
        p->alme_type = ALME_TYPE_GET_INTF_LIST_REQUEST;

        ret = (uint8_t *)p;
    }
    else if (0 == strcmp(alme_request_type, "ALME-GET-METRIC.request"))
    {
        struct getMetricRequestALME *p;

        uint8_t mac_address[8];

        // When an ALME-GET-METRIC.request is solicited, the user can either
        // ask for a specific neighbor (in that case an extra argument is
        // provided) or for all neighbors (in that case no extra argument is
        // provided).
        //
        if (optind == argc)
        {
            // No extra argument was provided
            //
            mac_address[0] = 0x0;
            mac_address[1] = 0x0;
            mac_address[2] = 0x0;
            mac_address[3] = 0x0;
            mac_address[4] = 0x0;
            mac_address[5] = 0x0;
        }
        else
        {
            _asciiToMac(argv[optind], mac_address);
        }

        p = (struct getMetricRequestALME *)malloc(sizeof(struct getMetricRequestALME));
        if (NULL == p)
        {
            PLATFORM_PRINTF_DEBUG_ERROR("Could not allocate getMetricRequestALME structure!\n");
            return NULL;
        }
        p->alme_type = ALME_TYPE_GET_METRIC_REQUEST;
        memcpy(p->interface_address, mac_address, 6);

        ret = (uint8_t *)p;
    }
    else if (0 == strcmp(alme_request_type, "ALME-CUSTOM-COMMAND.request"))
    {
        struct customCommandRequestALME *p;

        if (optind == argc)
        {
            // No extra argument was provided
            //
            PLATFORM_PRINTF_DEBUG_ERROR("Invalid arguments for 'ALME-CUSTOM-COMMAND' message\n");
            return NULL;
        }

        p = (struct customCommandRequestALME *)malloc(sizeof(struct customCommandRequestALME));
        if (NULL == p)
        {
            PLATFORM_PRINTF_DEBUG_ERROR("Could not allocate customCommandRequestALME structure!\n");
            return NULL;
        }
        p->alme_type = ALME_TYPE_CUSTOM_COMMAND_REQUEST;

        if (0 == strcmp(argv[optind], "dnd"))
        {
            p->command = CUSTOM_COMMAND_DUMP_NETWORK_DEVICES;
        }
        else
        {
            PLATFORM_PRINTF_DEBUG_ERROR("Invalid arguments for 'ALME-CUSTOM-COMMAND' message\n");
            free(p);
            return NULL;
        }

        ret = (uint8_t *)p;
    }
    else
    {
        PLATFORM_PRINTF_DEBUG_ERROR("ERROR: Unknown ALME message type: %s\n", alme_request_type);
    }

    return ret;
}

// Sends an ALME REQUEST message to an AL entity:
//
//   - 'server_ip_and_port' is a string containing the "IP:port" where the AL
//     TCP server is listening (ex: "10.32.1.44:8888")
//
//   - 'alme_request' is a pointer to the ALME REQUEST payload (as generated by
//     "forge_1905_ALME_from_structure()")
//
//   - 'alme_request_len' is the number of bytes of 'alme_request'
//
//   - 'alme_reply' is a pointer to a buffer which is 'alme_reply_len' bytes
//      long where the response from the AL entity (either an ALME RESPONSE or
//      an ALME CONFIRMATION message) will be placed.
//
//   - 'alme_request_len' is the capacity of the 'alme_reply' buffer. It is also
//     used as an output argument that will contain the actual length of the
//     reply.
//
// Note that the caller is responsible for freeing both 'alme_request' and
// 'alme_response' after they are no longer needed (ie. this function does not
// allocate/free memory at all).
//
int _sendAlmeRequestAndWaitForReply(char *server_ip_and_port, uint8_t *alme_request, int alme_request_len, uint8_t *alme_reply, int *alme_reply_len)
{
    int sock;

    struct sockaddr_in server;

    ssize_t total_sent;

    ssize_t received;
    ssize_t total_received;

    char *aux;
    char *ip;
    char *port;

    aux  = strdup(server_ip_and_port);
    ip   = strtok(aux,  ":");
    port = strtok(NULL, ":");

    if (NULL == ip || NULL == port)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Invalid address format. Must follow this template: '<ip_address>:<port_number>'\n");
        return 0;
    }

    //Create socket
    //
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sock)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("socket() failed with errno=%d (%s)\n", errno, strerror(errno));
        return 0;
    }

    //Connect to remote server
    //
    memset(&server, 0, sizeof(server));
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family      = AF_INET;
    server.sin_port        = htons(atoi(port));

    free(aux);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("connect() failed with errno=%d (%s)\n", errno, strerror(errno));
        return 0;
    }

    // Send the ALME REQUEST message
    //
    PLATFORM_PRINTF_DEBUG_INFO("Sending ALME request message (%d byte(s) long)...\n", alme_request_len);
    total_sent = 0;
    do
    {
        ssize_t sent;

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
        sent = send(sock, alme_request, alme_request_len, 0);
#else
        sent = send(sock, (const char *)alme_request, alme_request_len, 0);
#endif

        if (-1 == sent)
        {
            PLATFORM_PRINTF_DEBUG_ERROR("send() failed with errno=%d (%s)\n", errno, strerror(errno));
            return 0;
        }
#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
        PLATFORM_PRINTF_DEBUG_INFO("%zd byte(s) sent\n", sent);
#else
        PLATFORM_PRINTF_DEBUG_INFO("%d byte(s) sent\n", sent);
#endif

        total_sent += sent;
    } while (total_sent < alme_request_len);

    // Close the socket for writing. This will inform the other end that we
    // are done (ie. send a "FIN" TCP packet)
    //
    PLATFORM_PRINTF_DEBUG_INFO("ALME request sent. Closing writing end of the socket descriptor...\n");
#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
    if (-1 == shutdown(sock, SHUT_WR))
#else
    if (-1 == shutdown(sock, SD_SEND))
#endif
    {
        PLATFORM_PRINTF_DEBUG_ERROR("shutdown(\"SHUT_WR\") failed with errno=%d (%s)\n", errno, strerror(errno));
        return 0;
    }

    // Receive a reply from the server
    //
    PLATFORM_PRINTF_DEBUG_INFO("Waiting for the ALME reply...\n");
    total_received = 0;
#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
    while( (received = recv(sock, alme_reply + total_received, *alme_reply_len - total_received, 0)) > 0 )
#else
    while( (received = recv(sock, (char *)(alme_reply + total_received), *alme_reply_len - total_received, 0)) > 0 )
#endif
    {
        // Keep reading until the server closes the connection
        //
        if (-1 == received)
        {
            PLATFORM_PRINTF_DEBUG_ERROR("recv() failed with errno=%d (%s)\n", errno, strerror(errno));
            return 0;
        }
#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
        PLATFORM_PRINTF_DEBUG_INFO("%zd byte(s) received\n", received);
#else
        PLATFORM_PRINTF_DEBUG_INFO("%d byte(s) received\n", received);
#endif

        total_received += received;

        if (total_received >= *alme_reply_len)
        {
            // This message is too big.
            //
            PLATFORM_PRINTF_DEBUG_ERROR("Error! Received message is too big\n");
            return 0;
        }
    }

#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
    PLATFORM_PRINTF_DEBUG_INFO("ALME reply received (%zd bytes in total). Closing socket...\n", total_received);
    if (-1 == close(sock))
#else
    PLATFORM_PRINTF_DEBUG_INFO("ALME reply received (%d bytes in total). Closing socket...\n", total_received);
    if (-1 == closesocket(sock))
#endif
    {
#ifndef _FLAVOUR_X86_WINDOWS_MINGW_
        PLATFORM_PRINTF_DEBUG_ERROR("close() failed with errno=%d (%s)\n", errno, strerror(errno));
#else
        PLATFORM_PRINTF_DEBUG_ERROR("closesocket() failed with errno=%d (%s)\n", errno, strerror(errno));
#endif
        return 0;
    }

    *alme_reply_len = total_received;

    return 1;
}



////////////////////////////////////////////////////////////////////////////////
// External public functions
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    int   c;
    char *al_ip_address_and_tcp_port = NULL;
    char *alme_request_type          = NULL;

    uint8_t  *alme_request_structure    = NULL;
    uint8_t  *alme_request_payload      = NULL;
    uint16_t  alme_request_payload_len  = 0;

    int verbosity_counter = 1; // Only ERROR and WARNING messages

    #define MAX_REPLY_SIZE (100*MAX_NETWORK_SEGMENT_SIZE)

    uint8_t  *alme_reply_structure;
    uint8_t   alme_reply_payload[MAX_REPLY_SIZE];
    int     alme_reply_payload_len = MAX_REPLY_SIZE;

    char aux[300*1024];

    int i;

#ifdef _FLAVOUR_X86_WINDOWS_MINGW_
   // This is needed for network functions (from "winsock2.h") to work
   //
    WORD versionWanted = MAKEWORD(1, 1);
    WSADATA wsaData;
    WSAStartup(versionWanted, &wsaData);
#endif

    while ((c = getopt (argc, argv, "va:m:h")) != -1)
    {
        switch (c)
        {
            case 'v':
            {
                // Each time this flag appears, the verbosity counter is
                // incremented.
                //
                verbosity_counter++;
                break;
            }
            case 'a':
            {
                // <ip address>:<tcp port> where the AL is waiting for HLE
                // messages (ex: "10.8.34.3:8970")
                //
                al_ip_address_and_tcp_port = optarg;
                break;
            }

            case 'm':
            {
                // ALME REQUEST message that we want to send the AL entity
                // (ex: "ALME-GET-INTF-LIST.request")
                //
                alme_request_type = optarg;
                break;
            }
            case 'h':
            {
                // Help
                //
                PLATFORM_PRINTF("HLE entity (build %s)\n", _BUILD_NUMBER_);
                PLATFORM_PRINTF("\n");
                PLATFORM_PRINTF("Usage:  %s  [-v] -a <ip address>:<tcp port> -m <ALME request type> [ALME arguments]\n", argv[0]);
                PLATFORM_PRINTF("\n");
                PLATFORM_PRINTF("  where...\n");
                PLATFORM_PRINTF("\n");
                PLATFORM_PRINTF("    *  '-v', if present, will increase the verbosity level. Can be present more than once,\n");
                PLATFORM_PRINTF("       making the HLE entity even more verbose each time.\n");
                PLATFORM_PRINTF("\n");
                PLATFORM_PRINTF("    * <ip address>:<tcp port> are used to identify the ALME listening socket used by the AL we want to query/control\n");
                PLATFORM_PRINTF("\n");
                PLATFORM_PRINTF("    * <ALME request type> can be any of the following (some of them use extra arguments):\n");
                PLATFORM_PRINTF("        - ALME-GET-INTF-LIST.request                 <--- Get information regarding the queried AL interfaces\n");
                PLATFORM_PRINTF("        - ALME-GET-METRIC.request                    <--- Get metrics between the queried AL and *all* of its neighbors\n");
                PLATFORM_PRINTF("        - ALME-GET-METRIC.request xx:xx:xx:xx:xx:xx  <--- Get metrics between the queried AL and the neighbor whose AL MAC address matches the provided one\n");
                PLATFORM_PRINTF("        - ALME-CUSTOM-COMMAND.request <command>      <--- Custom (non-standard) commands. Possible values and their effect:\n");
                PLATFORM_PRINTF("                                                            - dnd : dump network devices. Returns a text dump of the AL internal devices database\n");
                PLATFORM_PRINTF("\n");
                exit(0);
            }
        }
    }

    if (NULL == al_ip_address_and_tcp_port)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("ERROR: You *must* provide an AL address (example: '-a 10.9.123.1:9077')\n");
        exit(1);
    }
    if (NULL == alme_request_type)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("ERROR: You *must* provide the type of ALME REQUEST that you want to send to the AL entity (ex: '-m ALME-GET-INTF-LIST.request')\n");
        exit(1);
    }

    PLATFORM_PRINTF_DEBUG_SET_VERBOSITY_LEVEL(verbosity_counter);

    // Build the ALME structure and print it to stdout
    //
    alme_request_structure = _build_alme_request(alme_request_type, argc, argv);
    if (NULL == alme_request_structure)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("ERROR: The ALME REQUEST structure could not be build.\n");
        exit(1);
    }
    PLATFORM_PRINTF_DEBUG_INFO("Displaying contents of the ALME REQUEST that is going to be sent:\n");
    visit_1905_ALME_structure(alme_request_structure, print_callback, PLATFORM_PRINTF_DEBUG_INFO, "");

    // From the structure, generate a bit stream
    //
    alme_request_payload = forge_1905_ALME_from_structure(alme_request_structure, &alme_request_payload_len);
    if (NULL == alme_request_payload)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("ERROR: The ALME REQUEST payload could not be build.\n");
        exit(1);
    }
    PLATFORM_PRINTF_DEBUG_INFO("Displaying the bit stream associated to this ALME REQUEST structure (%d byte(s) long):\n", alme_request_payload_len);
    aux[0] = 0;
    for (i=0; i<alme_request_payload_len; i++)
    {
        char item[10];

        sprintf(item, "0x%02x ", alme_request_payload[i]);
        strcat(aux, item);
    }
    PLATFORM_PRINTF_DEBUG_INFO("%s\n", aux);
    free_1905_ALME_structure(alme_request_structure);

    // Send that bit stream to the AL entity and wait for a response
    //
    PLATFORM_PRINTF_DEBUG_INFO("Sending bit stream to %s (len = %d)...\n", al_ip_address_and_tcp_port, alme_request_payload_len);
    if (0 == _sendAlmeRequestAndWaitForReply(al_ip_address_and_tcp_port, alme_request_payload, alme_request_payload_len, alme_reply_payload, &alme_reply_payload_len))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("ERROR: AL communication problem\n");
        exit(1);
    }
    free_1905_ALME_packet(alme_request_payload);

    PLATFORM_PRINTF_DEBUG_INFO("Displaying bit stream associated to the ALME RESPONSE/CONFIRMATION structure (%d byte(s) long):\n", alme_reply_payload_len);
    aux[0] = 0;
    for (i=0; i<alme_reply_payload_len; i++)
    {
        char item[10];

        sprintf(item, "0x%02x ", alme_reply_payload[i]);
        strcat(aux, item);
    }
    PLATFORM_PRINTF_DEBUG_INFO("%s\n", aux);

    // Convert the response back into a structure and print it to stdout
    //
    alme_reply_structure = parse_1905_ALME_from_packet(alme_reply_payload);
    if (NULL == alme_reply_structure)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("ERROR: Cannot parse ALME RESPONSE/CONFIRMATION\n");
    }
    visit_1905_ALME_structure(alme_reply_structure, print_callback, PLATFORM_PRINTF, "");

    return 0;
}


