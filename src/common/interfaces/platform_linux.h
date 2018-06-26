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

#ifndef PLATFORM_LINUX_H
#define PLATFORM_LINUX_H

/** @file
 *
 * Platform-specific functions and data structures that are only available on Linux platforms.
 *
 * Only linux-specific source files (i.e., files in the src_linux directory) may include this header.
 */

/** @brief Get the interface index by name.
 *
 * @param[in] interface_name The name of the interface on which to listen.
 * @return The interface index on success, or -1 on error (errno will be set).
 *
 * The interface index can be used to set options on the interface and to bind a socket to the interface.
 */
int getIfIndex(const char *interface_name);

/** @brief Open a socket suitable for raw ethernet packets.
 *
 * @param[in] ifindex The interface index on which to listen (as returned by getIfIndex()).
 * @param[in] eth_type The protocol number (in host byte order)
 * @return The socket file descriptor on success, or -1 on error (errno will be set).
 *
 * The socket is created as SOCK_RAW, so the ethernet header must be added to / removed from the payload.
 *
 * The created socket will be bound to the given interface and protocol.
 *
 * Close the socket with close() when done.
 *
 * No messages are printed in case of error, but errno will be set upon return.
 *
 * @todo factor with the AL server itself.
 */
int openPacketSocket(int ifindex, INT16U eth_type);



#endif // PLATFORM_LINUX_H
