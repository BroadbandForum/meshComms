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
#include <1905_cmdus.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h> /* size_t */

#define ADDR_AL "\x02\xee\xff\x33\x44\x00"
#define ADDR_MAC0 "\x00\xee\xff\x33\x44\x00"
#define ADDR_MAC1 "\x00\xee\xff\x33\x44\x10"
#define ADDR_MAC2 "\x00\xee\xff\x33\x44\x20"
#define ADDR_MAC3 "\x00\xee\xff\x33\x44\x30"

#define ADDR_AL_PEER0 "\x02\xaa\xbb\x33\x44\x00"
#define ADDR_AL_PEER1 "\x02\xaa\xbb\x33\x44\x10"
#define ADDR_AL_PEER2 "\x02\xaa\xbb\x33\x44\x20"
#define ADDR_AL_PEER3 "\x02\xaa\xbb\x33\x44\x30"
#define ADDR_MAC_PEER0 "\x00\xee\xff\x33\x44\x01"
#define ADDR_MAC_PEER1 "\x00\xee\xff\x33\x44\x11"
#define ADDR_MAC_PEER2 "\x00\xee\xff\x33\x44\x21"
#define ADDR_MAC_PEER3 "\x00\xee\xff\x33\x44\x31"

/** Print the contents of @a buf, wrapping at 80 characters, indent every line with @a indent + 1 space */
void dump_bytes(const void *buf, size_t buf_len, const char *indent);

struct CMDU *expect_cmdu(int s, unsigned timeout_ms, const char *testname, uint16_t expected_cmdu_type,
                         mac_address expected_src_addr, mac_address expected_src_al_addr, mac_address expected_dst_address);

int expect_cmdu_match(int s, unsigned timeout_ms, const char *testname, const struct CMDU *expected_cmdu,
                      mac_address expected_src_addr, mac_address expected_src_al_addr, mac_address expected_dst_address);

/** Send a CMDU. If it fails, print an error and return a value >= 1, else return 0. */
int send_cmdu(int s, mac_address dst_addr, mac_address src_addr, const struct CMDU *cmdu);

#endif

