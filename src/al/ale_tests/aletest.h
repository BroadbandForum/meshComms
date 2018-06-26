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

#ifndef _ALETEST_H_
#define _ALETEST_H_

#include <platform.h> /* PLATFORM_PRINTF_* */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h> /* size_t */

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*(a)))

/** Print the contents of @a buf, wrapping at 80 characters, indent every line with @a indent + 1 space */
void dump_bytes(const uint8_t *buf, size_t buf_len, const char *indent);

/** @brief Byte + mask combination.
 *
 * The 8 most significant bits of this type are the inverse of a bitmask, the 8 LSB are the bits to mask against.
 *
 * This representation is very compact for the common case where we want to check all bits, because then the mask is 0
 * and we can just put the byte we want to check.
 */
typedef uint16_t maskedbyte_t;

/** @brief Compare masked bytes.
 *
 * @param buf The buffer to check.
 * @param buf_len Length of @a buf.
 * @param expected The expected bytes (including their mask).
 * @param expected_len The expected length. @a buf_len may be larger than @a expected_len, the rest must be 0 bytes.
 * @return false if @a buf differs from @a expected, taking intou account the mask.
 */
bool compare_masked(const uint8_t *buf, size_t buf_len, const maskedbyte_t *expected, size_t expected_len);

/** @brief Verify that received bytes are what is expected.
 *
 * @param buf The buffer to check.
 * @param buf_len Length of @a buf.
 * @param expected The expected bytes (including their mask).
 * @param expected_len The expected length. @a buf_len may be larger than @a expected_len, the rest must be 0 bytes.
 * @param message The message to be printed in case of failure, with additional printf arguments.
 * @return false in case of failure.
 *
 * In case of failure, the @a message is printed and @a buf is dumped.
 */
bool check_expected_bytes(const uint8_t *buf, size_t buf_len, const maskedbyte_t *expected, size_t expected_len,
                          const char *message, ...) __attribute__((format(printf, 5, 6)));

/** @brief Expect a packet on socket @a s, with timeout.
 *
 * This function receives and discards packets from socket @a s, until either a packet is received that matches @a
 * expected with length @a expected_len, or the timeout is reached.
 *
 * @return true if the expected packet was received, @false if not.
 *
 * @note This function has no way to report that packets were discarded, or if there is an error on the socket.
 */
bool expect_packet(int s, const maskedbyte_t *expected, size_t expected_len, unsigned timeout_ms);

/** Wrapper around expect_packet() that covers the common case */
#define CHECK_EXPECT_PACKET(s, expected, timeout_ms, result) \
    do { \
        if (expect_packet(s, expected, ARRAY_SIZE(expected), timeout_ms)) { \
            PLATFORM_PRINTF_DEBUG_INFO("Received expected " #expected "\n"); \
        } else { \
            PLATFORM_PRINTF_DEBUG_ERROR("<- Did not receive " #expected " within " #timeout_ms " ms\n"); \
            (result)++; \
        } \
    } while (0)

#endif

