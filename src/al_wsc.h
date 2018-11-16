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

#ifndef _AL_WSC_H_
#define _AL_WSC_H_

#include <datamodel.h>

// One type of 1905 CMDUs embeds "M1" and "M2" messages from the "Wi-Fi simple
// configuration standard".
//
// Because building these "M1" and "M2" messages in completely independent from
// the 1905 standard, I have created this simple interface to isolate the
// process as much as possible.
//
// It works like this:
//
//   * ENROLLEE:
//
//     1. When a 1905 node has an unconfigured interface, it needs to send an
//        "M1" message. It does this by calling "wscBuildM1()", which takes the
//        WSC device information as argument and returns a WSC data buffer. That
//        WSC data buffer must be stored for use when M2 is received.
//
//     2. When the response ("M2") is received, the enrollee calls
//       "wscProcessM2()" with the received M2 and the stored WSC data buffer.
//
//   * REGISTRAR:
//
//     1. When a 1905 node recieves an "M1" message, it calls "wscBuildM2()",
//        which takes the contents of the "M1" message and returns a data buffer
//        that can be directly embedded inside the WSC response TLV/CMDU.
//
//     2. After sending the TLV/CMDU, the "M2" buffer must be freed with a call
//        to "wscFreeM2()"
//
// Note that, in the enrolle, "M1" is automatically freed by "wscProcessM2()",
// while, in the registrar, "M2" needs to be freed with "wscFreeM2()".
//
// When receiving a WSC TLV, because its contents are opaque to the 1905 node,
// function "wscGetType()" can be used do distinguish between "M1" and "M2".
//
// By the way, all the next functions return "0" if there a problem an "1"
// otherwise (except for "wscGetType()", which returns the message type)

/** @brief Build a WSC M1 message.
 *
 * @param radio The radio for which we build the WSC M1 message.
 * @param wsc_device_data The device information to be stored in the M1 message. @todo should be part of radio.
 * @return true on success, false on failure.
 *
 * The M1 message to send is stored in the radio::wsc_info::m1 member of radio.
 *
 * If the radio already has a radio::wsc_info structure, it will be cleared.
 */
bool wscBuildM1(struct radio *radio, const struct wscDeviceData *wsc_device_data);

/** @brief Process a WSC M2 message.
 *
 * @param radio The radio for which this WSC is received.
 * @param m2 The received M2 WSC message.
 * @param m2_size The length of @a m2.
 * @return true on success, false on failure.
 */
bool wscProcessM2(struct radio *radio, const uint8_t *m2, uint16_t m2_size);

/** @brief Free the radio's radio::wsc_info structure. */
void wscInfoFree(struct radio *radio);

/** @brief Parsed M1 information.
 *
 * This structure collects the information parsed out of a M1 message. The pointers point into the @a m1 buffer, so this structure must no
 * longer be used when that buffer is freed. If some attribute is missing, the corresponding pointer is NULL.
 */
struct wscM1Info {
    const uint8_t *m1;           /**< M1 buffer parsed in this structure. */
    uint16_t       m1_size;      /**< Length of @a m1. */
    const uint8_t  *mac_address; /**< MAC Address (6 bytes). */
    const uint8_t  *nonce;       /**< Enrollee Nonce (16 bytes). */
    const uint8_t  *pubkey;      /**< Public Key. */
    uint16_t        pubkey_len;  /**< Length of @a pubkey. */
    uint16_t        auth_types;  /**< Authentication Type bitmask, 0 if the attribute is missing. */
    uint16_t        encr_types;  /**< Encryption Type bitmask, 0 if the attribute is missing. */
    uint8_t         rf_bands;    /**< RF Bands bitmask, 0 if the attribute is missing. */
};

/** @brief Parse M1.
 *
 * Find the interesting attributes in the M1 message @a m1, and write them to @a m1_info. The returned structure
 * must be freed. @a m1 is still used by it, so @a m1 may only be freed when @a m1_info is no longer used.
 *
 * No check is done if the required attributes are present; missing attributes remain NULL or 0 in @a m1_info.
 *
 * @return true on success, false on failure. @a m1_info may be inconsistent on failure.
 *
 * @todo Also parse device info, which can be stored in the datamodel.
 */
bool wscParseM1(const uint8_t *m1, uint16_t m1_size, struct wscM1Info *m1_info);

uint8_t wscBuildM2(struct wscM1Info *m1_info, const struct wscRegistrarInfo *wsc_info, uint8_t **m2, uint16_t *m2_size);
uint8_t wscFreeM2(uint8_t *m, uint16_t m_size);


#define WSC_TYPE_M1      (0x00)
#define WSC_TYPE_M2      (0x01)
#define WSC_TYPE_UNKNOWN (0xFF)

uint8_t wscGetType(const uint8_t *m, uint16_t m_size);


#endif
