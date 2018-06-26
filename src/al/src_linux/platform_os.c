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

#include <interface.h>
#include "platform.h"
#include "platform_os.h"
#include "platform_os_priv.h"
#include "platform_alme_server_priv.h"
#include <platform_linux.h>
#include <utils.h>
#include "1905_l2.h"

#include <stdlib.h>      // free(), malloc(), ...
#include <stdio.h>       // fopen(), FILE, sprintf(), fwrite()
#include <string.h>      // memcpy(), memcmp(), ...
#include <pthread.h>     // threads and mutex functions
#include <mqueue.h>      // mq_*() functions
#include <errno.h>       // errno
#include <poll.h>        // poll()
#include <sys/inotify.h> // inotify_*()
#include <unistd.h>      // read(), sleep()
#include <signal.h>      // struct sigevent, SIGEV_*
#include <sys/types.h>   // recv(), setsockopt()
#include <sys/socket.h>  // recv(), setsockopt()
#include <linux/if_packet.h> // packet_mreq

////////////////////////////////////////////////////////////////////////////////
// Private functions, structures and macros
////////////////////////////////////////////////////////////////////////////////

/** @brief Linux-specific per-interface data. */
struct linux_interface_info {
    struct interface interface;

    /** @brief Index of the interface, to be used for sockaddr_ll::sll_ifindex. */
    int ifindex;

    /** @brief File descriptor of the packet socket bound to the IEEE1905 protocol. */
    int sock_1905_fd;

    /** @brief File descriptor of the packet socket bound to the LLDP protocol. */
    int sock_lldp_fd;

    INT8U     al_mac_address[6];
    INT8U     queue_id;
};

// *********** IPC stuff *******************************************************

// Queue related function in the PLATFORM API return queue IDs that are INT8U
// elements.
// However, in POSIX all queue related functions deal with a 'mqd_t' type.
// The following global arrays are used to store the association between a
// "PLATFORM INT8U ID" and a "POSIX mqd_t ID"

#define MAX_QUEUE_IDS  256  // Number of values that fit in an INT8U

static mqd_t           queues_id[MAX_QUEUE_IDS] = {[ 0 ... MAX_QUEUE_IDS-1 ] = (mqd_t) -1};
static pthread_mutex_t queues_id_mutex          = PTHREAD_MUTEX_INITIALIZER;


// *********** Receiving packets ********************************************

static void handlePacket(INT8U queue_id, const uint8_t *packet, size_t packet_len, mac_address interface_mac_address)
{
    INT8U   message[9+MAX_NETWORK_SEGMENT_SIZE];
    INT16U  message_len;
    INT8U   message_len_msb;
    INT8U   message_len_lsb;

    if (packet_len > MAX_NETWORK_SEGMENT_SIZE)
    {
        // This should never happen
        //
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Recv thread* Captured packet too big\n");
        return;
    }

    // In order to build the message that will be inserted into the queue, we
    // need to follow the "message format" defines in the documentation of
    // function 'PLATFORM_REGISTER_QUEUE_EVENT()'
    //
    message_len = packet_len + 6;
#if _HOST_IS_LITTLE_ENDIAN_ == 1
    message_len_msb = *(((INT8U *)&message_len)+1);
    message_len_lsb = *(((INT8U *)&message_len)+0);
#else
    message_len_msb = *(((INT8U *)&message_len)+0);
    message_len_lsb = *(((INT8U *)&message_len)+1);
#endif

    message[0] = PLATFORM_QUEUE_EVENT_NEW_1905_PACKET;
    message[1] = message_len_msb;
    message[2] = message_len_lsb;
    memcpy(&message[3], interface_mac_address, 6);
    memcpy(&message[9], packet, packet_len);

    // Now simply send the message.
    //
    PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *Recv thread* Sending %d bytes to queue (0x%02x, 0x%02x, 0x%02x, ...)\n", 3+message_len, message[0], message[1], message[2]);

    if (0 == sendMessageToAlQueue(queue_id, message, 3 + message_len))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Receive thread* Error sending message to queue\n");
        return;
    }

    return;
}

static void *recvLoopThread(void *p)
{
    struct linux_interface_info *interface = (struct linux_interface_info *)p;

    struct packet_mreq multicast_request;

    if (NULL == p)
    {
        // 'p' must point to a valid 'struct linux_interface_info'
        //
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Recv thread* Invalid arguments in recvLoopThread()\n");

        return NULL;
    }

    interface->ifindex = getIfIndex(interface->interface.name);
    if (-1 == interface->ifindex)
    {
        free(interface);
        return NULL;
    }

    memset(&multicast_request, 0, sizeof(multicast_request));
    multicast_request.mr_ifindex = interface->ifindex;
    multicast_request.mr_alen = 6;

    interface->sock_1905_fd = openPacketSocket(interface->ifindex, ETHERTYPE_1905);
    if (-1 == interface->sock_1905_fd)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] socket('%s' protocol 1905) returned with errno=%d (%s) while opening a RAW socket\n",
                                    interface->interface.name, errno, strerror(errno));
        free(interface);
        return NULL;
    }

    /* Add the AL address to this interface */
    multicast_request.mr_type = PACKET_MR_UNICAST;
    memcpy(multicast_request.mr_address, interface->al_mac_address, 6);
    if (-1 == setsockopt(interface->sock_1905_fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &multicast_request, sizeof(multicast_request)))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Failed to add AL MAC address to interface '%s' with errno=%d (%s)\n",
                                    interface->interface.name, errno, strerror(errno));
    }

    /* Add the 1905 multicast address to this interface */
    multicast_request.mr_type = PACKET_MR_MULTICAST;
    memcpy(multicast_request.mr_address, MCAST_1905, 6);
    if (-1 == setsockopt(interface->sock_1905_fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &multicast_request, sizeof(multicast_request)))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Failed to add 1905 multicast address to interface '%s' with errno=%d (%s)\n",
                                    interface->interface.name, errno, strerror(errno));
    }

    /** @todo Make LLDP optional, for when lldpd is also running on the same device. */
    interface->sock_lldp_fd = openPacketSocket(interface->ifindex, ETHERTYPE_LLDP);
    if (-1 == interface->sock_lldp_fd)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] socket('%s' protocol 1905) returned with errno=%d (%s) while opening a RAW socket\n",
                                    interface->interface.name, errno, strerror(errno));
        close(interface->sock_1905_fd);
        free(interface);
        return NULL;
    }

    /* Add the LLDP multicast address to this interface */
    multicast_request.mr_type = PACKET_MR_MULTICAST;
    memcpy(multicast_request.mr_address, MCAST_LLDP, 6);
    if (-1 == setsockopt(interface->sock_lldp_fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &multicast_request, sizeof(multicast_request)))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Failed to add LLDP multicast address to interface '%s' with errno=%d (%s)\n",
                                    interface->interface.name, errno, strerror(errno));
    }


    PLATFORM_PRINTF_DEBUG_DETAIL("Starting recv on %s\n", interface->interface.name);
    /** @todo move to libevent instead of threads + poll */
    while(1)
    {
        struct pollfd fdset[2];
        size_t i;

        memset((void*)fdset, 0, sizeof(fdset));

        fdset[0].fd = interface->sock_1905_fd;
        fdset[0].events = POLLIN;
        fdset[1].fd = interface->sock_lldp_fd;
        fdset[1].events = POLLIN;
        if (0 > poll(fdset, 2, -1))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Interface %s receive thread* poll() returned with errno=%d (%s)\n",
                                        interface->interface.name, errno, strerror(errno));
            break;
        }

        for (i = 0; i < ARRAY_SIZE(fdset); i++)
        {
            if (fdset[i].revents & (POLLIN|POLLERR))
            {
                uint8_t packet[MAX_NETWORK_SEGMENT_SIZE];
                ssize_t recv_length;

                recv_length = recv(fdset[i].fd, packet, sizeof(packet), MSG_DONTWAIT);
                if (recv_length < 0)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
                    {
                        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Interface %s receive thread* recv failed with errno=%d (%s) \n",
                                                    interface->interface.name, errno, strerror(errno));
                        /* Probably not recoverable. */
                        close(interface->sock_1905_fd);
                        close(interface->sock_lldp_fd);
                        free(interface);
                        return NULL;
                    }
                }
                else
                {
                    handlePacket(interface->queue_id, packet, (size_t)recv_length, interface->interface.addr);
                }
            }
        }
    }

    // Unreachable
    PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Recv thread* Exiting thread (interface %s)\n", interface->interface.name);
    close(interface->sock_1905_fd);
    close(interface->sock_lldp_fd);
    free(interface);
    return NULL;
}

// *********** Timers stuff ****************************************************

// We use the POSIX timers API to implement PLATFORM timers
// It works like this:
//
//   - When the PLATFORM API user calls "PLATFORM_REGISTER_QUEUE_EVENT()" with
//     'PLATFORM_QUEUE_EVENT_TIMEOUT*', a new POSIX timer is created.
//
//   - When the timer expires, the POSIX API creates a thread for us and makes
//     it run function '_timerHandler()'
//
//   - '_timerHandler()' simply deletes (or reprograms, depending on the type
//     of timer) the timer and sends a message to a queue so that the user can
//     later be aware of the timer expiration with a call to
//     "PLATFORM_QUEUE_READ()"

struct _timerHandlerThreadData
{
    INT8U    queue_id;
    INT32U   token;
    INT8U    periodic;
    timer_t  timer_id;
};

static void _timerHandler(union sigval s)
{
    struct _timerHandlerThreadData *aux;

    INT8U   message[3+4];
    INT16U  packet_len;
    INT8U   packet_len_msb;
    INT8U   packet_len_lsb;
    INT8U   token_msb;
    INT8U   token_2nd_msb;
    INT8U   token_3rd_msb;
    INT8U   token_lsb;

    aux = (struct _timerHandlerThreadData *)s.sival_ptr;

    // In order to build the message that will be inserted into the queue, we
    // need to follow the "message format" defines in the documentation of
    // function 'PLATFORM_REGISTER_QUEUE_EVENT()'
    //
    packet_len = 4;

#if _HOST_IS_LITTLE_ENDIAN_ == 1
    packet_len_msb = *(((INT8U *)&packet_len)+1);
    packet_len_lsb = *(((INT8U *)&packet_len)+0);

    token_msb      = *(((INT8U *)&aux->token)+3);
    token_2nd_msb  = *(((INT8U *)&aux->token)+2);
    token_3rd_msb  = *(((INT8U *)&aux->token)+1);
    token_lsb      = *(((INT8U *)&aux->token)+0);
#else
    packet_len_msb = *(((INT8U *)&packet_len)+0);
    packet_len_lsb = *(((INT8U *)&packet_len)+1);

    token_msb     = *(((INT8U *)&aux->token)+0);
    token_2nd_msb = *(((INT8U *)&aux->token)+1);
    token_3rd_msb = *(((INT8U *)&aux->token)+2);
    token_lsb     = *(((INT8U *)&aux->token)+3);
#endif

    message[0] = 1 == aux->periodic ? PLATFORM_QUEUE_EVENT_TIMEOUT_PERIODIC : PLATFORM_QUEUE_EVENT_TIMEOUT;
    message[1] = packet_len_msb;
    message[2] = packet_len_lsb;
    message[3] = token_msb;
    message[4] = token_2nd_msb;
    message[5] = token_3rd_msb;
    message[6] = token_lsb;

    PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *Timer handler* Sending %d bytes to queue (%02x, %02x, %02x, ...)\n", 3+packet_len, message[0], message[1], message[2]);

    if (0 == sendMessageToAlQueue(aux->queue_id, message, 3+packet_len))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Timer handler* Error sending message to queue from _timerHandler()\n");
    }

    if (1 == aux->periodic)
    {
        // Periodic timer are automatically re-armed. We don't need to do
        // anything
    }
    else
    {
        // Delete the asociater timer
        //
        timer_delete(aux->timer_id);

        // Free 'struct _timerHandlerThreadData', as we don't need it any more
        //
        free(aux);
    }

    return;
}

// *********** Push button stuff ***********************************************

// Pressing the button can be simulated by "touching" (ie. updating the
// timestamp) the following tmp file
//
#define PUSH_BUTTON_VIRTUAL_FILENAME  "/tmp/virtual_push_button"

// For those platforms with a physical buttons attached to a GPIO, we need to
// know the actual GPIO number (as seen by the Linux kernel) to use.
//
//     NOTE: "PUSH_BUTTON_GPIO_NUMBER" is a string, not a number. It will later
//     be used in a string context, thus the "" are needed.
//     It can take the string representation of a number (ex: "26") or the
//     special value "disable", meaning we don't have GPIO support.
//
#define PUSH_BUTTON_GPIO_NUMBER              "disable" //"26"

#define PUSH_BUTTON_GPIO_EXPORT_FILENAME     "/sys/class/gpio/export"
#define PUSH_BUTTON_GPIO_DIRECTION_FILENAME  "/sys/class/gpio/gpio"PUSH_BUTTON_GPIO_NUMBER"/direction"
#define PUSH_BUTTON_GPIO_VALUE_FILENAME      "/sys/class/gpio/gpio"PUSH_BUTTON_GPIO_NUMBER"/direction"

// The only information that needs to be sent to the new thread is the "queue
// id" to later post messages to the queue.
//
struct _pushButtonThreadData
{
    INT8U     queue_id;
};

static void *_pushButtonThread(void *p)
{
    // In this implementation we will send the "push button" configuration
    // event message to the queue when either:
    //
    //   a) The user presses a physical button associated to a GPIO whose number
    //      is "PUSH_BUTTON_GPIO_NUMBER" (ie. it is exported by the linux kernel
    //      in "/sys/class/gpio/gpioXXX", where "XXX" is
    //      "PUSH_BUTTON_GPIO_NUMBER")
    //
    //   b) The user updates the timestamp of a tmp file called
    //      "PUSH_BUTTON_VIRTUAL_FILENAME".
    //      This is useful for debugging and for supporting the "push button"
    //      mechanism in those platforms without a physical button.
    //
    // This thread will simply wait for activity on any of those two file
    // descriptors and then send the "push button" configuration event to the
    // AL queue.
    // How is this done?
    //
    //   1. Configure the GPIO as input.
    //   2. Create an "inotify" watch on the tmp file.
    //   3. Use "poll()" to wait for either changes in the value of the GPIO or
    //      timestamp updates in the tmp file.

    int    gpio_enabled;

    FILE  *fd_gpio;
    FILE  *fd_tmp;

    int  fdraw_gpio;
    int  fdraw_tmp;

    struct pollfd fdset[2];

    INT8U queue_id;

    queue_id = ((struct _pushButtonThreadData *)p)->queue_id;;

    if (0 != strcmp(PUSH_BUTTON_GPIO_NUMBER, "disable"))
    {
        gpio_enabled = 1;
    }
    else
    {
        gpio_enabled = 0;
    }

    // First of all, prepare the GPIO kernel descriptor for "reading"...
    //
    if (gpio_enabled)
    {

        // 1. Write the number of the GPIO where the physical button is
        //    connected to file "/sys/class/gpio/export".
        //    This will instruct the Linux kernel to create a folder named
        //    "/sys/class/gpio/gpioXXX" that we can later use to read the GPIO
        //    level.
        //
        if (NULL == (fd_gpio = fopen(PUSH_BUTTON_GPIO_EXPORT_FILENAME, "w")))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Push button thread* Error opening GPIO fd %s\n", PUSH_BUTTON_GPIO_EXPORT_FILENAME);
            return NULL;
        }
        if (0 == fwrite(PUSH_BUTTON_GPIO_NUMBER, 1, strlen(PUSH_BUTTON_GPIO_NUMBER), fd_gpio))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Push button thread* Error writing '"PUSH_BUTTON_GPIO_NUMBER"' to %s\n", PUSH_BUTTON_GPIO_EXPORT_FILENAME);
            fclose(fd_gpio);
            return NULL;
        }
        fclose(fd_gpio);

        // 2. Write "in" to file "/sys/class/gpio/gpioXXX/direction" to tell the
        //    kernel that this is an "input" GPIO (ie. we are only going to
        //    read -and not write- its value).

        if (NULL == (fd_gpio = fopen(PUSH_BUTTON_GPIO_DIRECTION_FILENAME, "w")))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Push button thread* Error opening GPIO fd %s\n", PUSH_BUTTON_GPIO_DIRECTION_FILENAME);
            return NULL;
        }
        if (0 == fwrite("in", 1, strlen("in"), fd_gpio))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Push button thread* Error writing 'in' to %s\n", PUSH_BUTTON_GPIO_DIRECTION_FILENAME);
            fclose(fd_gpio);
            return NULL;
        }
        fclose(fd_gpio);
    }

    // ... and then re-open the GPIO file descriptors for reading in "raw"
    // (ie "open" instead of "fopen") mode.
    //
    if (gpio_enabled)
    {
        if (-1  == (fdraw_gpio = open(PUSH_BUTTON_GPIO_VALUE_FILENAME, O_RDONLY | O_NONBLOCK)))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Push button thread* Error opening GPIO fd %s\n", PUSH_BUTTON_GPIO_VALUE_FILENAME);
        }
    }

    // Next, regarding the "virtual" button, first create the "tmp" file in
    // case it does not already exist...
    //
    if (NULL == (fd_tmp = fopen(PUSH_BUTTON_VIRTUAL_FILENAME, "w+")))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Push button thread* Could not create tmp file %s\n", PUSH_BUTTON_VIRTUAL_FILENAME);
        return NULL;
    }
    fclose(fd_tmp);

    // ...and then add a "watch" that triggers when its timestamp changes (ie.
    // when someone does a "touch" of the file or writes to it, for example).
    //
    if (-1 == (fdraw_tmp = inotify_init()))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Push button thread* inotify_init() returned with errno=%d (%s)\n", errno, strerror(errno));
        return NULL;
    }
    if (-1 == inotify_add_watch(fdraw_tmp, PUSH_BUTTON_VIRTUAL_FILENAME, IN_ATTRIB))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Push button thread* inotify_add_watch() returned with errno=%d (%s)\n", errno, strerror(errno));
        return NULL;
    }

    // At this point we have two file descriptors ("fdraw_gpio" and "fdraw_tmp")
    // that we can monitor with a call to "poll()"
    //
    while(1)
    {
        int   nfds;
        INT8U button_pressed;

        memset((void*)fdset, 0, sizeof(fdset));

        fdset[0].fd     = fdraw_tmp;
        fdset[0].events = POLLIN;
        nfds            = 1;

        if (gpio_enabled)
        {
            fdset[1].fd     = fdraw_gpio;
            fdset[1].events = POLLPRI;
            nfds            = 2;
        }

        // The thread will block here (forever, timeout = -1), until there is
        // a change in one of the two file descriptors ("changes" in the "tmp"
        // file fd are cause by "attribute" changes -such as the timestamp-,
        // while "changes" in the GPIO fd are caused by a value change in the
        // GPIO value).
        //
        if (0 > poll(fdset, nfds, -1))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Push button thread* poll() returned with errno=%d (%s)\n", errno, strerror(errno));
            break;
        }

        button_pressed = 0;

        if (fdset[0].revents & POLLIN)
        {
            struct inotify_event event;

            PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *Push button thread* Virtual button has been pressed!\n");
            button_pressed = 1;

            // We must "read()" from the "tmp" fd to "consume" the event, or
            // else the next call to "poll() won't block.
            //
            read(fdraw_tmp, &event, sizeof(event));
        }
        else if (gpio_enabled && (fdset[1].revents & POLLPRI))
        {
            char buf[3];

            if (-1 == read(fdset[1].fd, buf, 3))
            {
                PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Push button thread* read() returned with errno=%d (%s)\n", errno, strerror(errno));
                continue;
            }

            if (buf[0] == '1')
            {
                PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *Push button thread* Physical button has been pressed!\n");
                button_pressed = 1;
            }
        }

        if (1 == button_pressed)
        {
            INT8U   message[3];

            message[0] = PLATFORM_QUEUE_EVENT_PUSH_BUTTON;
            message[1] = 0x0;
            message[2] = 0x0;

            PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *Push button thread* Sending 3 bytes to queue (0x%02x, 0x%02x, 0x%02x)\n", message[0], message[1], message[2]);

            if (0 == sendMessageToAlQueue(queue_id, message, 3))
            {
                PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Push button thread* Error sending message to queue from _pushButtonThread()\n");
            }
        }
    }

    // Close file descriptors and exit
    //
    if (gpio_enabled)
    {
        fclose(fd_gpio);
    }

    PLATFORM_PRINTF_DEBUG_INFO("[PLATFORM] *Push button thread* Exiting...\n");

    free(p);
    return NULL;
}

// *********** Topology change notification stuff ******************************

// The platform notifies the 1905 that a topology change has just took place
// by "touching" the following tmp file
//
#define TOPOLOGY_CHANGE_NOTIFICATION_FILENAME  "/tmp/topology_change"

// The only information that needs to be sent to the new thread is the "queue
// id" to later post messages to the queue.
//
struct _topologyMonitorThreadData
{
    INT8U     queue_id;
};

static void *_topologyMonitorThread(void *p)
{
    FILE  *fd_tmp;

    int  fdraw_tmp;

    struct pollfd fdset[2];

    INT8U  queue_id;

    queue_id = ((struct _topologyMonitorThreadData *)p)->queue_id;

    // Regarding the "virtual" notification system, first create the "tmp" file
    // in case it does not already exist...
    //
    if (NULL == (fd_tmp = fopen(TOPOLOGY_CHANGE_NOTIFICATION_FILENAME, "w+")))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Topology change monitor thread* Could not create tmp file %s\n", TOPOLOGY_CHANGE_NOTIFICATION_FILENAME);
        return NULL;
    }
    fclose(fd_tmp);

    // ...and then add a "watch" that triggers when its timestamp changes (ie.
    // when someone does a "touch" of the file or writes to it, for example).
    //
    if (-1 == (fdraw_tmp = inotify_init()))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Push button thread* inotify_init() returned with errno=%d (%s)\n", errno, strerror(errno));
        return NULL;
    }
    if (-1 == inotify_add_watch(fdraw_tmp, TOPOLOGY_CHANGE_NOTIFICATION_FILENAME, IN_ATTRIB))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Push button thread* inotify_add_watch() returned with errno=%d (%s)\n", errno, strerror(errno));
        return NULL;
    }

    while (1)
    {
        int   nfds;
        INT8U notification_activated;

        memset((void*)fdset, 0, sizeof(fdset));

        fdset[0].fd     = fdraw_tmp;
        fdset[0].events = POLLIN;
        nfds            = 1;

        // TODO: Other fd's to detect topoly changes would be initialized here.
        // One good idea would be to use a NETLINK socket that is notified by
        // the Linux kernel when network "stuff" (routes, IPs, ...) change.
        //
        //fdset[0].fd     = ...;
        //fdset[0].events = POLLIN;
        //nfds            = 2;

        // The thread will block here (forever, timeout = -1), until there is
        // a change in one of the previous file descriptors .
        //
        if (0 > poll(fdset, nfds, -1))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Topology change monitor thread* poll() returned with errno=%d (%s)\n", errno, strerror(errno));
            break;
        }

        notification_activated = 0;

        if (fdset[0].revents & POLLIN)
        {
            struct inotify_event event;

            PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *Topology change monitor thread* Virtual notification has been activated!\n");
            notification_activated = 1;

            // We must "read()" from the "tmp" fd to "consume" the event, or
            // else the next call to "poll() won't block.
            //
            read(fdraw_tmp, &event, sizeof(event));
        }

        if (1 == notification_activated)
        {
            INT8U  message[3];

            message[0] = PLATFORM_QUEUE_EVENT_TOPOLOGY_CHANGE_NOTIFICATION;
            message[1] = 0x0;
            message[2] = 0x0;

            PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *Topology change monitor thread* Sending 3 bytes to queue (0x%02x, 0x%02x, 0x%02x)\n", message[0], message[1], message[2]);

            if (0 == sendMessageToAlQueue(queue_id, message, 3))
            {
                PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Topology change monitor thread* Error sending message to queue from _pushButtonThread()\n");
            }
        }
    }

    PLATFORM_PRINTF_DEBUG_INFO("[PLATFORM] *Topology change monitor thread* Exiting...\n");

    free(p);
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////
// Internal API: to be used by other platform-specific files (functions
// declaration is found in "./platform_os_priv.h")
////////////////////////////////////////////////////////////////////////////////

INT8U sendMessageToAlQueue(INT8U queue_id, INT8U *message, INT16U message_len)
{
    mqd_t   mqdes;

    mqdes = queues_id[queue_id];
    if ((mqd_t) -1 == mqdes)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Invalid queue ID\n");
        return 0;
    }

    if (NULL == message)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] Invalid message\n");
        return 0;
    }

    if (0 !=  mq_send(mqdes, (const char *)message, message_len, 0))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] mq_send('%d') returned with errno=%d (%s)\n", queue_id, errno, strerror(errno));
        return 0;
    }

    return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Platform API: Device information functions to be used by platform-independent
// files (functions declarations are  found in "../interfaces/platform_os.h)
////////////////////////////////////////////////////////////////////////////////

struct deviceInfo *PLATFORM_GET_DEVICE_INFO(void)
{
    // TODO: Retrieve real data from OS

    static struct deviceInfo x =
    {
        .friendly_name      = "Kitchen ice cream dispatcher",
        .manufacturer_name  = "Megacorp S.A.",
        .manufacturer_model = "Ice cream dispatcher X-2000",

        .control_url        = "http://192.168.10.44",
    };

    return &x;
}


////////////////////////////////////////////////////////////////////////////////
// Platform API: IPC related functions to be used by platform-independent
// files (functions declarations are  found in "../interfaces/platform_os.h)
////////////////////////////////////////////////////////////////////////////////

INT8U PLATFORM_CREATE_QUEUE(const char *name)
{
    mqd_t          mqdes;
    struct mq_attr attr;
    int            i;
    char           name_tmp[20];

    pthread_mutex_lock(&queues_id_mutex);

    for (i=1; i<MAX_QUEUE_IDS; i++)  // Note: "0" is not a valid "queue_id"
    {                                // according to the documentation of
        if (-1 == queues_id[i])      // "PLATFORM_CREATE_QUEUE()". That's why we
        {                            // skip it
            // Empty slot found.
            //
            break;
        }
    }
    if (MAX_QUEUE_IDS == i)
    {
        // No more queue id slots available
        //
        pthread_mutex_unlock(&queues_id_mutex);
        return 0;
    }

    if (!name)
    {
        name_tmp[0] = 0x0;
        sprintf(name_tmp, "/queue_%03d", i);
        name = name_tmp;
    }
    else if (name[0] != '/')
    {
        snprintf(name_tmp, 20, "/%s", name);
        name = name_tmp;
    }

    // If a queue with this name already existed (maybe from a previous
    // session), destroy and re-create it
    //
    mq_unlink(name);

    attr.mq_flags   = 0;
    attr.mq_maxmsg  = 100;
    attr.mq_curmsgs = 0;
    attr.mq_msgsize = MAX_NETWORK_SEGMENT_SIZE+3;
      //
      // NOTE: The biggest value in the queue is going to be a message from the
      // "new packet" event, which is MAX_NETWORK_SEGMENT_SIZE+3 bytes long.
      // The "PLATFORM_CREATE_QUEUE()" documentation mentions

    if ((mqd_t) -1 == (mqdes = mq_open(name, O_RDWR | O_CREAT, 0666, &attr)))
    {
        // Could not create queue
        //
        pthread_mutex_unlock(&queues_id_mutex);
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] mq_open('%s') returned with errno=%d (%s)\n", name, errno, strerror(errno));
        return 0;
    }

    queues_id[i] = mqdes;

    pthread_mutex_unlock(&queues_id_mutex);
    return i;
}

INT8U PLATFORM_REGISTER_QUEUE_EVENT(INT8U queue_id, INT8U event_type, void *data)
{
    switch (event_type)
    {
        case PLATFORM_QUEUE_EVENT_NEW_1905_PACKET:
        {
            pthread_t                         thread;
            struct event1905Packet           *p1;
            struct linux_interface_info      *interface;

            if (NULL == data)
            {
                // 'data' must contain a pointer to a 'struct event1905Packet'
                //
                return 0;
            }

            p1 = (struct event1905Packet *)data;

            interface = (struct linux_interface_info *)malloc(sizeof(struct linux_interface_info));
            if (NULL == interface)
            {
                // Out of memory
                //
                return 0;
            }

            interface->queue_id              = queue_id;
            interface->interface.name        = strdup(p1->interface_name);
            memcpy(interface->interface.addr,         p1->interface_mac_address, 6);
            memcpy(interface->al_mac_address,         p1->al_mac_address,        6);

            pthread_create(&thread, NULL, recvLoopThread, (void *)interface);

            /** @todo This is a horrible hack to make sure the addresses are configured on the interfaces before we
             * start sending raw packets. */
            usleep(30000);

            // NOTE:
            //   The memory allocated by "interface" will be lost forever at this
            //   point (well... until the application exits, that is).
            //   This is considered acceptable.

            break;
        }

        case PLATFORM_QUEUE_EVENT_NEW_ALME_MESSAGE:
        {
            // The AL entity is telling us that it is capable of processing ALME
            // messages and that it wants to receive ALME messages on the
            // provided queue.
            //
            // In our platform-dependent implementation, we have decided that
            // ALME messages are going to be received on a dedicated thread
            // that runs a TCP server.
            //
            // What we are going to do now is:
            //
            //   1) Create that thread
            //
            //   2) Tell it that everytime a new packet containing ALME
            //      commands arrives on its socket it should forward the
            //      payload to this queue.
            //
            pthread_t                thread;
            struct almeServerThreadData  *p;

            p = (struct almeServerThreadData *)malloc(sizeof(struct almeServerThreadData));
            if (NULL == p)
            {
                // Out of memory
                //
                return 0;
            }
            p->queue_id = queue_id;

            pthread_create(&thread, NULL, almeServerThread, (void *)p);

            break;
        }

        case PLATFORM_QUEUE_EVENT_TIMEOUT:
        case PLATFORM_QUEUE_EVENT_TIMEOUT_PERIODIC:
        {
            struct eventTimeOut             *p1;
            struct _timerHandlerThreadData  *p2;

            struct sigevent      se;
            struct itimerspec    its;
            timer_t              timer_id;

            p1 = (struct eventTimeOut *)data;

            if (p1->token > MAX_TIMER_TOKEN)
            {
                // Invalid arguments
                //
                return 0;
            }

            p2 = (struct _timerHandlerThreadData *)malloc(sizeof(struct _timerHandlerThreadData));
            if (NULL == p2)
            {
                // Out of memory
                //
                return 0;
            }

            p2->queue_id = queue_id;
            p2->token    = p1->token;
            p2->periodic = PLATFORM_QUEUE_EVENT_TIMEOUT_PERIODIC == event_type ? 1 : 0;

            // Next, create the timer. Note that it will be automatically
            // destroyed (by us) in the callback function
            //
            memset(&se, 0, sizeof(se));
            se.sigev_notify          = SIGEV_THREAD;
            se.sigev_notify_function = _timerHandler;
            se.sigev_value.sival_ptr = (void *)p2;

            if (-1 == timer_create(CLOCK_REALTIME, &se, &timer_id))
            {
                // Failed to create a new timer
                //
                free(p2);
                return 0;
            }
            p2->timer_id = timer_id;

            // Finally, arm/start the timer
            //
            its.it_value.tv_sec     = p1->timeout_ms / 1000;
            its.it_value.tv_nsec    = (p1->timeout_ms % 1000) * 1000000;
            its.it_interval.tv_sec  = PLATFORM_QUEUE_EVENT_TIMEOUT_PERIODIC == event_type ? its.it_value.tv_sec  : 0;
            its.it_interval.tv_nsec = PLATFORM_QUEUE_EVENT_TIMEOUT_PERIODIC == event_type ? its.it_value.tv_nsec : 0;

            if (0 != timer_settime(timer_id, 0, &its, NULL))
            {
                // Problems arming the timer
                //
                free(p2);
                timer_delete(timer_id);
                return 0;
            }

            break;
        }

        case PLATFORM_QUEUE_EVENT_PUSH_BUTTON:
        {
            // The AL entity is telling us that it is capable of processing
            // "push button" configuration events.
            //
            // Create the thread in charge of generating these events.
            //
            pthread_t                      thread;
            struct _pushButtonThreadData  *p;

            p = (struct _pushButtonThreadData *)malloc(sizeof(struct _pushButtonThreadData));
            if (NULL == p)
            {
                // Out of memory
                //
                return 0;
            }

            p->queue_id = queue_id;
            pthread_create(&thread, NULL, _pushButtonThread, (void *)p);

            break;
        }

        case PLATFORM_QUEUE_EVENT_AUTHENTICATED_LINK:
        {
            // The AL entity is telling us that it is capable of processing
            // "authenticated link" events.
            //
            // We don't really need to do anything here. The interface specific
            // thread will be created when the AL entity calls the
            // "PLATFORM_START_PUSH_BUTTON_CONFIGURATION()" function.

            break;
        }

        case PLATFORM_QUEUE_EVENT_TOPOLOGY_CHANGE_NOTIFICATION:
        {
            // The AL entity is telling us that it is capable of processing
            // "topology change" events.
            //
            // We will create a new thread in charge of monitoring the local
            // topology to generate these events.
            //
            pthread_t                           thread;
            struct _topologyMonitorThreadData  *p;

            p = (struct _topologyMonitorThreadData *)malloc(sizeof(struct _topologyMonitorThreadData));
            if (NULL == p)
            {
                // Out of memory
                //
                return 0;
            }

            p->queue_id = queue_id;

            pthread_create(&thread, NULL, _topologyMonitorThread, (void *)p);

            break;
        }

        default:
        {
            // Unknown event type!!
            //
            return 0;
        }
    }

    return 1;
}

INT8U PLATFORM_READ_QUEUE(INT8U queue_id, INT8U *message_buffer)
{
    mqd_t    mqdes;
    ssize_t  len;

    mqdes = queues_id[queue_id];
    if ((mqd_t) -1 == mqdes)
    {
        // Invalid ID
        return 1;
    }

    len = mq_receive(mqdes, (char *)message_buffer, MAX_NETWORK_SEGMENT_SIZE+3, NULL);

    if (-1 == len)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] mq_receive() returned with errno=%d (%s)\n", errno, strerror(errno));
        return 0;
    }

    // All messages are TLVs where the second and third bytes indicate the
    // total length of the payload. This value *must* match "len-3"
    //
    if ( len < 0 )
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] mq_receive() returned than 3 bytes (minimum TLV size)\n");
        return 0;
    }
    else
    {
        INT16U payload_len;

        PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] Receiving %d bytes from queue (%02x, %02x, %02x, ...)\n", len, message_buffer[0], message_buffer[1], message_buffer[2]);

        payload_len = *(((INT8U *)message_buffer)+1) * 256 + *(((INT8U *)message_buffer)+2);

        if (payload_len != len-3)
        {
            PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] mq_receive() returned %d bytes, but the TLV is %d bytes\n", len, payload_len+3);
            return 0;
        }
    }

    return 1;
}


