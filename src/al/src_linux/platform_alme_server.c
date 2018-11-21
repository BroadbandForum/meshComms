/*
 *  Broadband Forum IEEE 1905.1/1a stack
 *  
 *  Copyright (c) 2017, Broadband Forum
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  
 *  Subject to the terms and conditions of this license, each copyright
 *  holder and contributor hereby grants to those receiving rights under
 *  this license a perpetual, worldwide, non-exclusive, no-charge,
 *  royalty-free, irrevocable (except for failure to satisfy the
 *  conditions of this license) patent license to make, have made, use,
 *  offer to sell, sell, import, and otherwise transfer this software,
 *  where such license applies only to those patent claims, already
 *  acquired or hereafter acquired, licensable by such copyright holder or
 *  contributor that are necessarily infringed by:
 *  
 *  (a) their Contribution(s) (the licensed copyrights of copyright holders
 *      and non-copyrightable additions of contributors, in source or binary
 *      form) alone; or
 *  
 *  (b) combination of their Contribution(s) with the work of authorship to
 *      which such Contribution(s) was added by such copyright holder or
 *      contributor, if, at the time the Contribution is added, such addition
 *      causes such combination to be necessarily infringed. The patent
 *      license shall not apply to any other combinations which include the
 *      Contribution.
 *  
 *  Except as expressly stated above, no rights or licenses from any
 *  copyright holder or contributor is granted under this license, whether
 *  expressly, by implication, estoppel or otherwise.
 *  
 *  DISCLAIMER
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 *  DAMAGE.
 */

#include "platform.h"
#include "platform_alme_server.h"
#include "platform_alme_server_priv.h"
#include "platform_os.h"
#include "platform_os_priv.h"

#include <arpa/inet.h>  // socket(), AF_INET, htons(), ...
#include <errno.h>      // errno
#include <pthread.h>    // threads and mutex functions
#include <mqueue.h>     // mq_*() functions
#include <string.h>     // strerror()
#include <stdio.h>      // snprintf(), ...
#include <stdlib.h>     // free(), malloc(), ...
#include <unistd.h>     // close(), ...

// Each platform/implementation decides how ALME messages are received by the AL
// (ie. the standard does not specify how this is done).
//
// In this implementation we have decided that the AL entity will listen on a
// TCP socket waiting for ALME messages.
//
// Whenever an HLE wants to communicate with this AL, it needs to follow these
// steps:
//
//   1. Prepare an ALME bit stream compatible with the output of
//      "forge_1905_ALME_from_structure()".
//
//   2. Open a TCP connection to the AL entity TCP server.
//
//   3. Send the ALME bit stream and nothing else.
//
//   4. Close the socket.
//
// The ALME TCP server will then forward the data to the system queue that the
// main 1905 thread uses to receive events.
//
// The following structure is passed to the thread in charge of running the TCP
// server. It contains the "queue id" needed to be able to later forward all the
// ALME messages from the TCP socket to the 1905 main thread.


////////////////////////////////////////////////////////////////////////////////
// Private functions, structures and macros
////////////////////////////////////////////////////////////////////////////////

#define ALME_CLIENT_ID_TCP_SOCKET                   0x1
#define ALME_CLIENT_ID_1905_VENDOR_SPECIFIC_TUNNEL  0x2

// These global variables and mutex are used to send information from the AL
// main thread (the one running "start1905AL()") to the ALME TCP server thread
//
static pthread_mutex_t tcp_server_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  tcp_server_cond  = PTHREAD_COND_INITIALIZER;
static int             tcp_server_flag  = 0;

static INT8U  *alme_response;
static INT16U  alme_response_len;

// This variable holds the number of the port number the server will use
//
static int alme_server_port = 0;


////////////////////////////////////////////////////////////////////////////////
// Internal API: to be used by other platform-specific files (functions
// declaration is found in "./platform_alme_server_priv.h")
////////////////////////////////////////////////////////////////////////////////

void *almeServerThread(void *p)
{
    int socketfd;

    struct sockaddr_in server_addr;

    #define ALME_TCP_SERVER_MAX_MESSAGE_SIZE (3*MAX_NETWORK_SEGMENT_SIZE)
    INT8U  queue_message[4+ALME_TCP_SERVER_MAX_MESSAGE_SIZE];
    INT8U *alme_message;

    // The first three bytes of the message that this thread is going to insert
    // into the AL queue every time a new ALME message arrives looks like this:
    //
    //    byte 0x00 - PLATFORM_QUEUE_EVENT_NEW_ALME_MESSAGE
    //    byte 0x01 - Message length MSB
    //    byte 0x02 - Message length LSB
    //    byte 0x03 - ALME client ID
    //    byte 0x04... ALME payload
    //
    // Thus, the actual ALME payload starts at byte #5
    //
    alme_message = &queue_message[4];

    // Create socket and configure it with "SO_REUSEADDR" (this is needed so
    // that every time we exit the program we don't have to wait for the OS to
    // "destroy" server sockets -up to 2 minutes- before restarting it again)
    //
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == socketfd)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *ALME server thread* socket() failed with errno=%d (%s)\n", errno, strerror(errno));
        return NULL;
    }
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *ALME server thread* setsockopt() failed with errno=%d (%s)\n", errno, strerror(errno));
        return NULL;
    }
     
    // Prepare the sockaddr_in structure
    //
    if (0 == alme_server_port)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *ALME server thread* server port has not been set!\n");
        return NULL;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(alme_server_port);
     
    // Bind
    //
    if(bind(socketfd,(struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *ALME server thread* bind() failed\n");
        return NULL;
    }
     
    // Listen
    //
    if (-1 == listen(socketfd, 3))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *ALME server thread* listen() failed with errno=%d (%s)\n", errno, strerror(errno));
        return NULL;
    }
     
    // Accept connections from an incoming clients
    //
    while (1)
    {
        struct sockaddr_in client_addr;
        int addrlen;

        int new_socketfd;
        int read_size;
        int total_size;

        memset(&client_addr, 0, sizeof(client_addr));
        addrlen = sizeof(client_addr);
        
        // Accept an incoming connection
        //
        PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *ALME server thread* Waiting for incoming connections...\n");
     
        new_socketfd = accept(socketfd, (struct sockaddr *)&client_addr, (socklen_t*)&addrlen);
        if (new_socketfd < 0)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("[PLATFORM] *ALME server thread* accept() failed with errno=%d (%s)\n", errno, strerror(errno));
            continue;
        }
        PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *ALME server thread* New connection established from HLE.\n");
         
        // Receive a message from client
        //
        total_size = 0;
        while( (read_size = recv(new_socketfd, alme_message + total_size, ALME_TCP_SERVER_MAX_MESSAGE_SIZE - total_size, 0)) > 0 )
        {
            // Keep reading until the client closes the connection
            //
            total_size += read_size;

            if (total_size >= ALME_TCP_SERVER_MAX_MESSAGE_SIZE)
            {
                // This message is too big. If this is not an error from the
                // client, then "ALME_TCP_SERVER_MAX_MESSAGE_SIZE" needs to be
                // increased.
                //
                PLATFORM_PRINTF_DEBUG_WARNING("[PLATFORM] *ALME server thread* Received message is too big.\n");

                read_size = -1;
                break;
            }
        }
         
        if(0 == read_size)
        {
            // Connection closed, forward ALME message to the AL entity
            //
            INT16U  message_len = total_size + 1;
            INT8U   message_len_msb;
            INT8U   message_len_lsb;

#if _HOST_IS_LITTLE_ENDIAN_ == 1
            message_len_msb = *(((INT8U *)&message_len)+1);
            message_len_lsb = *(((INT8U *)&message_len)+0);
#else
            message_len_msb = *(((INT8U *)&message_len)+0);
            message_len_lsb = *(((INT8U *)&message_len)+1);
#endif

            queue_message[0] = PLATFORM_QUEUE_EVENT_NEW_ALME_MESSAGE;
            queue_message[1] = message_len_msb;
            queue_message[2] = message_len_lsb;
            queue_message[3] = ALME_CLIENT_ID_TCP_SOCKET;

            PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *ALME server thread* Sending %d bytes to queue (%02x, %02x, %02x, ...)\n", 3+message_len, queue_message[0], queue_message[1], queue_message[2]);

            pthread_mutex_lock(&tcp_server_mutex);
            tcp_server_flag = 0;
            pthread_mutex_unlock(&tcp_server_mutex);

            if (0 == sendMessageToAlQueue(((struct almeServerThreadData *)p)->queue_id, queue_message, 3+message_len))
            {
                PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *ALME server thread* Error sending message to queue from _alme_server_thread()\n");
            }
            else
            {
                INT32U total_sent;

                // Wait for response
                //
                PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *ALME server thread* Waiting for the AL response...\n");

                pthread_mutex_lock(&tcp_server_mutex);
                while (0 == tcp_server_flag)
                {
                    pthread_cond_wait(&tcp_server_cond, &tcp_server_mutex);
                }
                pthread_mutex_unlock(&tcp_server_mutex);

                // Once the mutex is unlocked it means the ALME response is
                // contained in global var "alme_response" (which is
                // "alme_response_len" bytes long)
                //
                PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *ALME server thread* Sending ALME reply to HLE...\n");

                total_sent = 0;
                if (NULL != alme_response && 0 != alme_response_len)
                {
                    do
                    {
                        ssize_t sent;

                        sent = send(new_socketfd, alme_response, alme_response_len, MSG_NOSIGNAL);

                        if (-1 == sent)
                        {
                            PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *ALME server thread* send() failed with errno=%d (%s)\n", errno, strerror(errno));
                            break;
                        }

                        total_sent += sent;
                    } while (total_sent < alme_response_len);

                    free(alme_response);
                    alme_response     = NULL;
                    alme_response_len = 0;
                }
                PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *ALME server thread* ALME reply sent (total %d bytes)\n", total_sent);
            }
            close(new_socketfd);
        }
        else if(-1 == read_size)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("[PLATFORM] *ALME server thread* recv() failed.\n");
        }
    }
     
    return NULL;
}

void almeServerPortSet(int port_number)
{
    alme_server_port = port_number;
}


////////////////////////////////////////////////////////////////////////////////
// Platform API: Interface related functions to be used by platform-independent
// files (functions declarations are  found in "../interfaces/platform.h)
////////////////////////////////////////////////////////////////////////////////

INT8U PLATFORM_SEND_ALME_REPLY(INT8U alme_client_id, INT8U *alme_message, INT16U alme_message_len)
{
    int i, first_time;
    char aux1[200];
    char aux2[10];

    PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] Payload of ALME bit stream to send:\n");
    aux1[0]    = 0x0;
    aux2[0]    = 0x0;
    first_time = 1;
    for (i=0; i<alme_message_len; i++)
    {
        snprintf(aux2, 6, "0x%02x ", alme_message[i]);
        strncat(aux1, aux2, 200-strlen(aux1)-1);

        if (0 != i && 0 == (i+1)%8)
        {
            if (1 == first_time)
            {
                PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM]   - Payload        = %s\n", aux1);
                first_time = 0;
            }
            else
            {
                PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM]                      %s\n", aux1);
            }
            aux1[0] = 0x0;
        }
    }
    if (1 == first_time)
    {
        PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM]   - Payload        = %s\n", aux1);
    }
    else
    {
        PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM]                      %s\n", aux1);
    }

    switch (alme_client_id)
    {
        case ALME_CLIENT_ID_TCP_SOCKET:
        {
            // Send the ALME RESPONSE/CONFIRMATION through the same socket where
            // the REQUEST was originally received
            //
            if (0 == alme_message_len || NULL == alme_message)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Refuse to send an *invalid* ALME reply\n");
                alme_response     = NULL;
                alme_response_len = 0;
            }
            else
            {
                alme_response = (INT8U *)malloc(alme_message_len);
                if (NULL == alme_response)
                {
                    PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Cannot allocate memory for the ALME RESPONSE/CONFIRMATION message\n");

                    alme_response     = NULL;
                    alme_response_len = 0;
                }
                else
                {
                    PLATFORM_MEMCPY(alme_response, alme_message, alme_message_len);
                    alme_response_len = alme_message_len;
                }
            }

            pthread_mutex_lock(&tcp_server_mutex);
            tcp_server_flag = 1;
            pthread_cond_signal(&tcp_server_cond);
            pthread_mutex_unlock(&tcp_server_mutex);

            break;
        }

        case ALME_CLIENT_ID_1905_VENDOR_SPECIFIC_TUNNEL:
        {
            // Tunnel the response in a ALME vendor specific message
            //
            break;
        }

        default:
        {
            break;
        }
    }

    return 1;
}
