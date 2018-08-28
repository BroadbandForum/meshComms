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

#ifndef _PLATFORM_OS_H_
#define _PLATFORM_OS_H_


////////////////////////////////////////////////////////////////////////////////
// Device information functions
////////////////////////////////////////////////////////////////////////////////
struct deviceInfo
{
    char *friendly_name;               // NULL-terminated string containing the
                                       // name that identifies this device on
                                       // the network.
                                       // This is the same name devices display
                                       // on their web interface or when queried
                                       // by uPnP.
                                       // Max length = 64 bytes (including the
                                       // NULL char)

    char *manufacturer_name;           // NULL-terminated string containing the
                                       // manufacturer name.
                                       // Max length = 64 bytes (including the
                                       // NULL char)

    char *manufacturer_model;          // NULL-terminated string containing the
                                       // manufacturer model.
                                       // Max length = 64 bytes (including the
                                       // NULL char)

    char *control_url;                 // NULL-terminated string containing the
                                       // a control URL (typically a WebUI that
                                       // can be used to further configure the
                                       // device).  Example:
                                       // "http://192.168.1.10/index.html"

};

// Return a "struct deviceInfo" or NULL if there wa san error
//
// The returned pointer must not be freed.
//
// [PLATFORM PORTING NOTE]
//   Check the documentation for each field of the "struct deviceInfo" structure
//   (see above) to understand how this structure must be filled
//
struct deviceInfo *PLATFORM_GET_DEVICE_INFO(void);

////////////////////////////////////////////////////////////////////////////////
// IPC related functions
////////////////////////////////////////////////////////////////////////////////

// Create a queue object that will later be used to receive platform
// notifications.
//
// It works like this:
//
//   1. The platform API user calls this function, which returns a "queue id":
//
//        queue_id = PLATFORM_CREATE_QUEUE();
//
//   2. Then it tells the platform which events he is interested in receiving
//      notifications from on this queue:
//
//         PLATFORM_REGISTER_QUEUE_EVENT(queue_id, <EVENT A>, NULL);
//         PLATFORM_REGISTER_QUEUE_EVENT(queue_id, <EVENT B>, NULL);
//         ...
//
//      (Note: see the documentation of "PLATFORM_REGISTER_QUEUE_EVENT()" for
//      more details about the second and third arguments)
//
//    3. Finally, the user waits for one of the registered events to happen:
//
//         while (1)
//         {
//             PLATFORM_READ_QUEUE(queue_id, &message_buffer[0]);
//
//             <process event>
//         }
//
// The only argument that this function takes is a 'name' string, that is not
// used for anything.
//
// [PLATFORM PORTING NOTE]
//   However, you can use this 'name' argument for debug purposes.
//
// If something goes wrong, this function returns "0", otherwise it returns
// a number greater than "0" representing a "queue id"
//
uint8_t PLATFORM_CREATE_QUEUE(const char *name);

// This function takes:
//
//   1. a 'queue_id' (previously obtained with 'PLATFORM_CREATE_QUEUE()")
//   2. an 'event_type' (valid values are explained below)
//   3. a pointer to (optional) aditional data associated to the type of event
//
// ...and then, from that point on, platform events that match 'event_type'
// will be sent to the queue so that later the API user can receive them with
// a call to "PLATFORM_READ_QUEUE()"
//
// These are the possible values that 'event_type' can take and what they mean:
//
//   - PLATFORM_QUEUE_EVENT_NEW_1905_PACKET:
//
//       A new event is generated every time a 1905 packet arrives on the
//       interface provided (see below).
//
//       'data' is interpreted as a pointer to a "struct event1905Packet" which
//       contains the following fields:
//
//         - 'interface_name' ---------> pointer to a string containing the name
//                                       of the interface (ex: "eth0")
//
//         - 'interface_mac_address' --> MAC addres of 'interface_name'
//
//         - 'al_mac_address' ---------> MAC address of the 1905 AL entity
//
//       When the event takes place, the message that is inserted in the queue
//       has the following format:
//
//         byte 0x00 - PLATFORM_QUEUE_EVENT_NEW_1905_PACKET
//         byte 0x01 - Message length MSB
//         byte 0x02 - Message length LSB
//         byte 0x03 - Byte 1 of the MAC addr of the interface where the packet was received
//         byte 0x04 - Byte 2 of the MAC addr of the interface where the packet was received
//         byte 0x05 - Byte 3 of the MAC addr of the interface where the packet was received
//         byte 0x06 - Byte 4 of the MAC addr of the interface where the packet was received
//         byte 0x07 - Byte 5 of the MAC addr of the interface where the packet was received
//         byte 0x08 - Byte 6 of the MAC addr of the interface where the packet was received
//         byte 0x09... Packet payload
//
//       "Message length" (bytes 0x01 and 0x02) makes reference to how many bytes
//       come after the third one. Ie. 6 + length of "Packet payload".
//
//       "Packet payload" is the whole ethernet frame (ie. "src MAC addr" +
//       "destination MAC addr" + "ether type" + "payload")
//
//       Note that this event will only be triggered when 1905 packets arrive
//       to the interface. This includes:
//
//         * All ethernet packets with ethernet type = 0x893a (ETHERTYPE_1905)
//           and addressed to the local AL MAC address or to the local
//           interface MAC address or to the braodcast AL MAC address
//           (01:80:C2:00:00:13)
//         * All ethernet packets with ethernet type = 0x88cc (ETHERTYPE_LLDP)
//           and addressed to the LLDP nearest bridge multicast address
//           (01:80:C2:00:00:0E)
//
//       [PLATFORM PORTING NOTE]
//         When implementing this function you will probably have to set the
//         interface to "promiscuous" mode or else these 1905 packets won't be
//         captured.
//
//   - PLATFORM_QUEUE_EVENT_TIMEOUT:
//
//       A new event is generated after "x" milliseconds
//
//       'data' is interpreted as a pointer to a "struct eventTimeOut" which
//       contains the following fields:
//
//         - 'timeout_ms' ---------> Time (in milliseconds) after which the
//                                   event message will be inserted into the
//                                   queue
//
//         - 'token' --------------> This same token will be contained in the
//                                   event message and can be used to identify
//                                   the timeout instance (this way you can find
//                                   out *which* timer timed out)
//                                   It can be any value greater than "0" and
//                                   smaller than "MAX_TIMER_TOKEN"
//
//       When the event takes place, the message that is inserted in the queue
//       has the following format:
//
//         byte 0x00 - PLATFORM_QUEUE_EVENT_TIMEOUT
//         byte 0x01 - Message length MSB (always "0x00")
//         byte 0x02 - Message length LSB (always "0x04")
//         byte 0x03 - 'token' MSB
//         byte 0x03 - 'token' 2nd MSB
//         byte 0x03 - 'token' 3rd MSB
//         byte 0x03 - 'token' LSB
//
//       "Message length" (bytes 0x01 and 0x02) makes reference to the size of
//       the rest of the message, which is always "4".
//
//       'id_token' is the same 'id_token' used when calling this function.
//
//   - PLATFORM_QUEUE_EVENT_TIMEOUT_PERIODIC:
//
//       Works in the same way as "PLATFORM_QUEUE_EVENT_TIMEOUT", except that
//       it expires periodically, instead of just once.
//
//   - PLATFORM_QUEUE_EVENT_NEW_ALME_MESSAGE:
//
//       A new event is generated everytime a new ALME message is received.
//
//       'data' can be set to NULL (it is not used for anything).
//
//       The ALME message bit stream format is not specied in the standard.
//       However, in order to be able to understand ALME messages received on
//       this queue, we are going to use a custom (invented by me, and not
//       standarized in any way!) bit stream format: the one resulting from
//       calling function "forge_1905_ALME_from_structure()" function.
//
//       Taking this into consideration, the message inserted in the queue must
//       have the following format:
//
//         byte 0x00 - PLATFORM_QUEUE_EVENT_NEW_ALME_MESSAGE
//         byte 0x01 - Message length MSB
//         byte 0x02 - Message length LSB
//         byte 0x03 - ALME client ID
//         byte 0x04... ALME payload
//
//       "Message length" (bytes 0x01 and 0x02) makes reference to how many
//       bytes come after the third one. Ie. 1 + length of "ALME payload".
//
//       "ALME client ID" is some number that will be used when the AL entity
//       calls "PLATFORM_SEND_ALME_REPLY()". It works like this:
//
//         1. Using a platform-specific mechanism, an ALME REQUEST arrives.
//
//         2. Platform-specific code then has to send an ALME REQUEST to the AL,
//            so it inserts the ALME message in the AL queue using some ID (for
//            example, "7").
//
//         3. The AL processes the queue, takes action, an readies either an
//            ALME RESPONSE or an ALME CONFIRM message, and then calls
//            "PLATFORM_SEND_ALME_REPLY()" with both a pointer to the message
//            and the same ID used for the request (ie. "7")
//
//         4. Platform-specific code looks at that ID and knows exactly which
//            platform-dependent means of communication with the HLE it has to
//            use to send back the ALME RESPONSE/CONFIRM message.
//
//       "ALME payload" is the bit stream generated by a call to
//       "forge_1905_ALME_from_structure()"
//
//       [PLATFORM PORTING NOTE]
//         The way ALME messages are received is not specified in the standard.
//         It could be anything: a TCP socket, a proprietary L2 protocol, a
//         UNIX socket receiving data from another process running on the same
//         machine, etc...
//
//   - PLATFORM_QUEUE_EVENT_PUSH_BUTTON:
//
//       A new event is generated when the platform detects that the user
//       has started the "push button" configuration mechanism, which is
//       typically done by pressing a physical button on the device.
//
//       'data' can be set to NULL (it is not used for anything).
//
//       The "push button" event is triggered...
//         - ...in "new" devices when they want to join a secured network
//         - ...in already secure devices when they want to allow new devices
//           to join the secured network.
//
//       In other words: if you have a secured network with "N" devices and
//       bring a new one, in order to add this new device to the network, you
//       would have to trigger a "push button" configuration event on the new
//       the device and on (at least) one of the already secured "N" devices.
//
//       When the event takes place, the message that is inserted in the queue
//       has the following format:
//
//         byte 0x00 - PLATFORM_QUEUE_EVENT_PUSH_BUTTON
//         byte 0x01 - 0x00
//         byte 0x02 - 0x00
//
//       [PLATFORM PORTING NOTE]
//         The way to trigger this event could be anything: a software flag, a
//         physical button, a message received through a socket, etc... but the
//         most common way is to use a physical button.
//
//   - PLATFORM_QUEUE_EVENT_AUTHENTICATED_LINK:
//
//       A new event is generated when a new link (where one of the ends is a
//       local interface) becomes "authenticated".
//
//       'data' can be set to NULL (it is not used for anything).
//
//       The message inserted in the queue must have the following format:
//
//         byte 0x00 - PLATFORM_QUEUE_EVENT_AUTHENTICATED_LINK
//         byte 0x01 - Message length MSB (0x00)
//         byte 0x02 - Message length LSB (0x14)
//         byte 0x03 - Byte 1 of the MAC addr of the local  interface of the new authenticated link
//         byte 0x04 - Byte 2 of the MAC addr of the local  interface of the new authenticated link
//         byte 0x05 - Byte 3 of the MAC addr of the local  interface of the new authenticated link
//         byte 0x06 - Byte 4 of the MAC addr of the local  interface of the new authenticated link
//         byte 0x07 - Byte 5 of the MAC addr of the local  interface of the new authenticated link
//         byte 0x08 - Byte 6 of the MAC addr of the local  interface of the new authenticated link
//         byte 0x09 - Byte 1 of the MAC addr of the remote interface of the new authenticated link
//         byte 0x0a - Byte 2 of the MAC addr of the remote interface of the new authenticated link
//         byte 0x0b - Byte 3 of the MAC addr of the remote interface of the new authenticated link
//         byte 0x0c - Byte 4 of the MAC addr of the remote interface of the new authenticated link
//         byte 0x0d - Byte 5 of the MAC addr of the remote interface of the new authenticated link
//         byte 0x0e - Byte 6 of the MAC addr of the remote interface of the new authenticated link
//         byte 0x0f - Byte 1 of the MAC addr of the AL MAC address contained in the "push button event notification" messages that started everything
//         byte 0x10 - Byte 2 of the MAC addr of the AL MAC address contained in the "push button event notification" messages that started everything
//         byte 0x11 - Byte 3 of the MAC addr of the AL MAC address contained in the "push button event notification" messages that started everything
//         byte 0x12 - Byte 4 of the MAC addr of the AL MAC address contained in the "push button event notification" messages that started everything
//         byte 0x13 - Byte 5 of the MAC addr of the AL MAC address contained in the "push button event notification" messages that started everything
//         byte 0x14 - Byte 6 of the MAC addr of the AL MAC address contained in the "push button event notification" messages that started everything
//         byte 0x15 - MSB bytes of the "message id" contained in the "push button event notification" messages that started everything
//         byte 0x16 - MSB bytes of the "message id" contained in the "push button event notification" messages that started everything
//
//       "Message length" (bytes 0x01 and 0x02) makes reference to how many
//       bytes come after the third one. Ie. for this messages its value is
//       always "20" ("0x14")
//
//       The typical sequence of events that makes this happen is this one:
//
//         1) The user starts a "push button" configuration process by pressing
//            a button and sending the "PLATFORM_QUEUE_EVENT_PUSH_BUTTON" to
//            the AL entity.
//
//         2) The AL entity receives this message from either its local platform
//            or a remote one (by means of a "push button" CMDU) and calls
//            "PLATFORM_START_PUSH_BUTTON_CONFIGURATION()" which starts a
//            technology-specific "pairing" process.
//
//         3) If this "pairing" process finishes as expected, the link is
//            condidered "authenticated" from that point on and a message of
//            type "PLATFORM_QUEUE_EVENT_AUTHENTICATED_LINK" is sent back to
//            the AL.
//
//       [PLATFORM PORTING NOTE]
//         Interfaces that do not support the "push button" configuration
//         mechanism (like, for example, ethernet) should *not* generate this
//         event (they are internally handled in a different way).
//
//   - PLATFORM_QUEUE_EVENT_TOPOLOGY_CHANGE_NOTIFICATION:
//
//       A new event is generated when the platform detects that the topology
//       has changed in "any" way (maybe one interface has been added or
//       removed, a new bridge has been set up, a forwarding rule has been
//       modified, etc...)
//
//       'data' can be set to NULL (it is not used for anything).
//
//       When the event takes place, the message that is inserted in the queue
//       has the following format:
//
//         byte 0x00 - PLATFORM_QUEUE_EVENT_TOPOLOGY_CHANGE_NOTIFICATION
//         byte 0x01 - 0x00
//         byte 0x02 - 0x00
//
//
// In all cases, if there is a problem registering the event, this function
// returns "0", otherwise it returns "1"
//
#define PLATFORM_QUEUE_EVENT_NEW_1905_PACKET              (0x00)
#define PLATFORM_QUEUE_EVENT_NEW_ALME_MESSAGE             (0x01)
#define PLATFORM_QUEUE_EVENT_TIMEOUT                      (0x02)
#define PLATFORM_QUEUE_EVENT_TIMEOUT_PERIODIC             (0x03)
#define PLATFORM_QUEUE_EVENT_PUSH_BUTTON                  (0x04)
#define PLATFORM_QUEUE_EVENT_AUTHENTICATED_LINK           (0x05)
#define PLATFORM_QUEUE_EVENT_TOPOLOGY_CHANGE_NOTIFICATION (0x06)

#define MAX_TIMER_TOKEN (1000)

struct event1905Packet
{
    char     *interface_name;
    uint8_t     interface_mac_address[6];
    uint8_t     al_mac_address[6];
};
struct eventTimeOut
{
    uint32_t    timeout_ms;
    uint32_t    token;
};
uint8_t PLATFORM_REGISTER_QUEUE_EVENT(uint8_t queue_id, uint8_t event_type, void *data);

// Wait until a new message is available in the queue represented by 'queue_id'
// (which is the value obtained when calling "PLATFORM_CREATE_QUEUE()"), and
// then copy it into the provided buffer 'message_buffer'
//
// 'message_buffer' must at least MAX_NETWORK_SEGMENT_SIZE+3 long
//
// If there is a problem this function returns "0", otherwise it returns "1"
//
uint8_t PLATFORM_READ_QUEUE(uint8_t queue_id, uint8_t *message_buffer);

#endif
