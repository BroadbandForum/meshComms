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
#include "platform_os.h"
#include "platform_os_priv.h"
#include "platform_alme_server_priv.h"
#include "1905_l2.h"

#include <stdlib.h>      // free(), malloc(), ...
#include <string.h>      // memcpy(), memcmp(), ...
#include <pthread.h>     // threads and mutex functions
#include <mqueue.h>      // mq_*() functions
#include <pcap/pcap.h>   // pcap_*() functions
#include <errno.h>       // errno
#include <poll.h>        // poll()
#include <sys/inotify.h> // inotify_*()
#include <unistd.h>      // read(), sleep()
#include <signal.h>      // struct sigevent, SIGEV_*

////////////////////////////////////////////////////////////////////////////////
// Private functions, structures and macros
////////////////////////////////////////////////////////////////////////////////

// *********** IPC stuff *******************************************************

// Queue related function in the PLATFORM API return queue IDs that are INT8U
// elements.
// However, in POSIX all queue related functions deal with a 'mqd_t' type.
// The following global arrays are used to store the association between a
// "PLATFORM INT8U ID" and a "POSIX mqd_t ID"

#define MAX_QUEUE_IDS  256  // Number of values that fit in an INT8U

static mqd_t           queues_id[MAX_QUEUE_IDS] = {[ 0 ... MAX_QUEUE_IDS-1 ] = (mqd_t) -1};
static pthread_mutex_t queues_id_mutex          = PTHREAD_MUTEX_INITIALIZER;


// *********** Packet capture stuff ********************************************

// We use 'libpcap' to capture 1905 packets on all interfaces.
// It works like this:
//
//   - When the PLATFORM API user calls "PLATFORM_REGISTER_QUEUE_EVENT()" with
//     'PLATFORM_QUEUE_EVENT_NEW_1905_PACKET', 'libpcap' is used to set the
//     corresponding interface into monitor mode.
//
//   - In addition, a new thread is created ('_pcapLoopThread()') which runs
//     forever and, everytime a new packet is received on the corresponding
//     interface, that thread calls '_pcapProcessPacket()'
//
//   - '_pcapProcessPacket()' simply post the whole contents of the received
//     packet to a queue so that the user can later obtain it with a call to
//     "PLATFORM_QUEUE_READ()"

static pthread_mutex_t pcap_filters_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  pcap_filters_cond  = PTHREAD_COND_INITIALIZER;
static int             pcap_filters_flag  = 0;

struct _pcapCaptureThreadData
{
    INT8U     queue_id;
    char     *interface_name;
    INT8U     interface_mac_address[6];
    INT8U     al_mac_address[6];
};

static void _pcapProcessPacket(u_char *arg, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
    // This function is executed (on a per-interface dedicated thread) every
    // time a new 1905 packet arrives

    struct _pcapCaptureThreadData *aux;

    INT8U   message[3+MAX_NETWORK_SEGMENT_SIZE];
    INT16U  message_len;
    INT8U   message_len_msb;
    INT8U   message_len_lsb;

    if (NULL == arg)
    {
        // Invalid argument
        //
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Pcap thread* Invalid arguments in _pcapProcessPacket()\n");
        return;
    }

    aux = (struct _pcapCaptureThreadData *)arg;

    if (pkthdr->len > MAX_NETWORK_SEGMENT_SIZE)
    {
        // This should never happen
        //
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Pcap thread* Captured packet too big\n");
        return;
    }

    // In order to build the message that will be inserted into the queue, we
    // need to follow the "message format" defines in the documentation of
    // function 'PLATFORM_REGISTER_QUEUE_EVENT()'
    //
    message_len = (INT16U)pkthdr->len + 6;
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
    message[3] = aux->interface_mac_address[0];
    message[4] = aux->interface_mac_address[1];
    message[5] = aux->interface_mac_address[2];
    message[6] = aux->interface_mac_address[3];
    message[7] = aux->interface_mac_address[4];
    message[8] = aux->interface_mac_address[5];

    memcpy(&message[9], packet, pkthdr->len);

    // Now simply send the message.
    //
    PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *Pcap thread* Sending %d bytes to queue (0x%02x, 0x%02x, 0x%02x, ...)\n", 3+message_len, message[0], message[1], message[2]);

    if (0 == sendMessageToAlQueue(aux->queue_id, message, 3 + message_len))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Pcap thread* Error sending message to queue from _pcapProcessPacket()\n");
        return;
    }

    return;
}

static void *_pcapLoopThread(void *p)
{
    // This function will loop forever in the "pcap_loop()" function, which
    // generates a callback to "_pcapProcessPacket()" every time a new 1905
    // packet arrives

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *pcap_descriptor;

    struct _pcapCaptureThreadData *aux;

    char pcap_filter_expression[255] = "";
    struct bpf_program fcode;

    if (NULL == p)
    {
        // 'p' must point to a valid 'struct _pcapCaptureThreadData'
        //
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Pcap thread* Invalid arguments in _pcapLoopThread()\n");

        pthread_mutex_lock(&pcap_filters_mutex);
        pcap_filters_flag = 1;
        pthread_cond_signal(&pcap_filters_cond);
        pthread_mutex_unlock(&pcap_filters_mutex);

        return NULL;
    }

    aux = (struct _pcapCaptureThreadData *)p;

    // Open the interface in pcap.
    // The third argument of 'pcap_open_live()' is set to '1' so that the
    // interface is configured in 'monitor mode'. This is needed because we are
    // not only interested in receiving packets addressed to the interface
    // MAC address (or broadcast), but also those packets addressed to the
    // "non-existent" (virtual?) AL MAC address of the AL entity (contained in
    // 'aux->al_mac_address')
    //
    pcap_descriptor = pcap_open_live(aux->interface_name, MAX_NETWORK_SEGMENT_SIZE, 1, 512, errbuf);
    if (NULL == pcap_descriptor)
    {
        // Could not configure interface to capture 1905 packets
        //
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Pcap thread* Error opening interface %s\n", aux->interface_name);

        pthread_mutex_lock(&pcap_filters_mutex);
        pcap_filters_flag = 1;
        pthread_cond_signal(&pcap_filters_cond);
        pthread_mutex_unlock(&pcap_filters_mutex);

        return NULL;
    }

    // If we started capturing now, we would receive *all* packets. This means
    // *all* packets (even those that have nothing to do with 1905) would be
    // copied from kernel space into user space (which is a very costly
    // operation).
    //
    // To mitigate this effect (which takes place when enabling 'monitor mode'
    // on an interface), 'pcap' let's us define "filtering rules" that take
    // place in kernel space, thus limiting the amount of copies that need to
    // be done to user space.
    //
    // Here we are going to configure a filter that only lets certain types of
    // packets to get through. In particular those that meet any of these
    // requirements:
    //
    //   1. Have ethertype == ETHERTYPE_1905 *and* are addressed to either the
    //      interface MAC address, the AL MAC address or the broadcast AL MAC
    //      address
    //
    //   2. Have ethertype == ETHERTYPE_LLDP *and* are addressed to the special
    //      LLDP nearest bridge multicast MAC address
    //
    snprintf(
              pcap_filter_expression,
              sizeof(pcap_filter_expression),
              "not ether src %02x:%02x:%02x:%02x:%02x:%02x "
              " and "
              "not ether src %02x:%02x:%02x:%02x:%02x:%02x "
              " and "
              "((ether proto 0x%04x and (ether dst %02x:%02x:%02x:%02x:%02x:%02x or ether dst %02x:%02x:%02x:%02x:%02x:%02x or ether dst %02x:%02x:%02x:%02x:%02x:%02x))"
              " or "
              "(ether proto 0x%04x and ether dst %02x:%02x:%02x:%02x:%02x:%02x))",
              aux->interface_mac_address[0], aux->interface_mac_address[1], aux->interface_mac_address[2], aux->interface_mac_address[3], aux->interface_mac_address[4], aux->interface_mac_address[5],
              aux->al_mac_address[0],        aux->al_mac_address[1],        aux->al_mac_address[2],        aux->al_mac_address[3],        aux->al_mac_address[4],        aux->al_mac_address[5],
              ETHERTYPE_1905,
              aux->interface_mac_address[0], aux->interface_mac_address[1], aux->interface_mac_address[2], aux->interface_mac_address[3], aux->interface_mac_address[4], aux->interface_mac_address[5],
              MCAST_1905_B0,                 MCAST_1905_B1,                 MCAST_1905_B2,                 MCAST_1905_B3,                 MCAST_1905_B4,                 MCAST_1905_B5,
              aux->al_mac_address[0],        aux->al_mac_address[1],        aux->al_mac_address[2],        aux->al_mac_address[3],        aux->al_mac_address[4],        aux->al_mac_address[5],
              ETHERTYPE_LLDP,
              MCAST_LLDP_B0,                 MCAST_LLDP_B1,                 MCAST_LLDP_B2,                 MCAST_LLDP_B3,                 MCAST_LLDP_B4,                 MCAST_LLDP_B5
            );

    if (pcap_compile(pcap_descriptor, &fcode, pcap_filter_expression, 1, 0xffffff) < 0)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Pcap thread* Cannot compile pcap filter (interface %s)\n", aux->interface_name);

        pthread_mutex_lock(&pcap_filters_mutex);
        pcap_filters_flag = 1;
        pthread_cond_signal(&pcap_filters_cond);
        pthread_mutex_unlock(&pcap_filters_mutex);

        return NULL;
    }

    PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM] *Pcap thread* Installing pcap filter on interface %s: %s\n", aux->interface_name, pcap_filter_expression);
    if (pcap_setfilter(pcap_descriptor, &fcode) < 0)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Pcap thread* Cannot attach pcap filter to interface %s\n", aux->interface_name);

        pthread_mutex_lock(&pcap_filters_mutex);
        pcap_filters_flag = 1;
        pthread_cond_signal(&pcap_filters_cond);
        pthread_mutex_unlock(&pcap_filters_mutex);

        return NULL;
    }

    // Signal the main thread so that it can continue its work
    //
    pthread_mutex_lock(&pcap_filters_mutex);
    pcap_filters_flag = 1;
    pthread_cond_signal(&pcap_filters_cond);
    pthread_mutex_unlock(&pcap_filters_mutex);

    // Start the pcap loop. This goes on forever...
    // Everytime a new packet (that meets the filtering rules defined above)
    // arrives, the '_pcapProcessPacket()' callback is executed
    //
    pcap_loop(pcap_descriptor, -1, _pcapProcessPacket, (u_char *)aux);

    // This point should never be reached
    //
    PLATFORM_PRINTF_DEBUG_ERROR("[PLATFORM] *Pcap thread* Exiting thread (interface %s)\n", aux->interface_name);
    free(aux);
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
      // "pcap" event, which is MAX_NETWORK_SEGMENT_SIZE+3 bytes long.
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
            struct _pcapCaptureThreadData    *p2;

            if (NULL == data)
            {
                // 'data' must contain a pointer to a 'struct event1905Packet'
                //
                return 0;
            }

            p1 = (struct event1905Packet *)data;

            p2 = (struct _pcapCaptureThreadData *)malloc(sizeof(struct _pcapCaptureThreadData));
            if (NULL == p2)
            {
                // Out of memory
                //
                return 0;
            }

                   p2->queue_id              = queue_id;
                   p2->interface_name        = strdup(p1->interface_name);
            memcpy(p2->interface_mac_address,         p1->interface_mac_address, 6);
            memcpy(p2->al_mac_address,                p1->al_mac_address,        6);

            pthread_mutex_lock(&pcap_filters_mutex);
            pcap_filters_flag = 0;
            pthread_mutex_unlock(&pcap_filters_mutex);

            pthread_create(&thread, NULL, _pcapLoopThread, (void *)p2);

            // While it is not strictly needed, we will now wait until the PCAP
            // thread registers the needed capture filters.
            //
            pthread_mutex_lock(&pcap_filters_mutex);
            while (0 == pcap_filters_flag)
            {
                pthread_cond_wait(&pcap_filters_cond, &pcap_filters_mutex);
            }
            pthread_mutex_unlock(&pcap_filters_mutex);

            // NOTE:
            //   The memory allocated by "p2" will be lost forever at this
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


