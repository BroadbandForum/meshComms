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

#include "aletest.h"

#include <platform.h>
#include <utils.h>

#include <poll.h>             // poll()
#include <time.h>             // clock_gettime()
#include <unistd.h>           // close()
#include <sys/types.h>        // recv()
#include <sys/socket.h>       // recv()

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void dump_bytes(const void *buf, size_t buf_len, const char *indent)
{
    size_t i;
    int bytes_per_line = (80 - 1 - (int)strlen(indent)) / 3;
    int bytecount;

    /* If indent is too long, just print 8 bytes per line */
    if (bytes_per_line < 8)
        bytes_per_line = 8;

    for (i = 0; i < buf_len; /* i is incremented in inner loop */)
    {
        PLATFORM_PRINTF("%s", indent);
        for (bytecount = 0; bytecount < bytes_per_line && i < buf_len; bytecount++, i++)
        {
            PLATFORM_PRINTF(" %02x", ((const uint8_t*)buf)[i]);
        }
        PLATFORM_PRINTF("\n");
    }
}

static int64_t get_time_ns()
{
    struct timespec t;
    /* We want real hardware time, but timer should be stopped while suspended (simulation) */
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return (int64_t)t.tv_sec * 1000000000 + (int64_t)t.tv_nsec;
}

struct CMDU *expect_cmdu(int s, unsigned timeout_ms, const char *testname, uint16_t expected_cmdu_type,
                         mac_address expected_src_addr, mac_address expected_src_al_addr, mac_address expected_dst_address)
{
    int64_t deadline = get_time_ns() + (int64_t)timeout_ms * 1000000;
    int64_t remaining_ns;
    int remaining_ms;
    struct pollfd p = { .fd = s, .events = POLLIN, .revents = 0, };
    int poll_result;
    char buf[1500];
    ssize_t received;
    struct CMDU_header cmdu_header;

    while (true) {
        remaining_ns = (deadline - get_time_ns());
        if (timeout_ms > 0) {
            if (remaining_ns <= 0)
            {
                PLATFORM_PRINTF_DEBUG_INFO("Timed out while expecting %s\n", testname);
                return NULL;
            }
            remaining_ms = (int)(remaining_ns / 1000000);
            if (remaining_ms <= 0)
                remaining_ms = 1;
        } else {
            remaining_ms = 0;
        }
        poll_result = poll(&p, 1, remaining_ms);
        if (poll_result == 1)
        {
            received = recv(s, buf, sizeof(buf), 0);
            if (-1 == received)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Receive failed while expecting %s\n", testname);
                return NULL;
            }
            if (!parse_1905_CMDU_header_from_packet((uint8_t*)buf, (size_t) received, &cmdu_header))
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Failed to parse CMDU header while expecting %s\n", testname);
                PLATFORM_PRINTF_DEBUG_DETAIL("  Received:\n");
                dump_bytes(buf, (size_t)received, "   ");
            }
            else if (expected_cmdu_type != cmdu_header.message_type)
            {
                PLATFORM_PRINTF_DEBUG_INFO("Received CMDU of type 0x%04x while expecting %s\n",
                                           cmdu_header.message_type, testname);
            }
            else if (0 != memcmp(expected_dst_address, cmdu_header.dst_addr, 6))
            {
                PLATFORM_PRINTF_DEBUG_INFO("Received CMDU with destination " MACSTR " while expecting %s\n",
                                           MAC2STR(cmdu_header.dst_addr), testname);
            }
            else if (0 != memcmp(expected_src_addr, cmdu_header.src_addr, 6) &&
                     0 != memcmp(expected_src_al_addr, cmdu_header.src_addr, 6))
            {
                PLATFORM_PRINTF_DEBUG_INFO("Received CMDU with source " MACSTR " while expecting %s\n",
                                           MAC2STR(cmdu_header.src_addr), testname);
            }
            else
            {
                INT8U *packets[] = {(INT8U *)buf + (6+6+2), NULL};
                struct CMDU *cmdu = parse_1905_CMDU_from_packets(packets);
                if (NULL == cmdu)
                {
                    PLATFORM_PRINTF_DEBUG_ERROR("Failed to parse CMDU %s\n", testname);
                    return NULL;
                }
                else
                {
                    return cmdu;
                }
            }
        }
        else if (poll_result < 0)
        {
            PLATFORM_PRINTF_DEBUG_ERROR("Poll error while expecting packet\n");
            return NULL;
        }
        // else check timeout again, poll may not be accurate enough.
    }
    // Unreachable
}

int expect_cmdu_match(int s, unsigned timeout_ms, const char *testname, const struct CMDU *expected_cmdu,
                      mac_address expected_src_addr, mac_address expected_src_al_addr, mac_address expected_dst_address)
{
    int ret = 1;
    struct CMDU *cmdu = expect_cmdu(s, timeout_ms, testname, expected_cmdu->message_type,
                                    expected_src_addr, expected_src_al_addr, expected_dst_address);
    if (NULL != cmdu)
    {
        cmdu->message_id = expected_cmdu->message_id;
        if (0 != compare_1905_CMDU_structures(cmdu, expected_cmdu))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("Received something else than expected %s\n", testname);
            PLATFORM_PRINTF_DEBUG_INFO("  Expected CMDU:\n");
            visit_1905_CMDU_structure(expected_cmdu, print_callback, PLATFORM_PRINTF_DEBUG_INFO, "");
            PLATFORM_PRINTF_DEBUG_INFO("  Received CMDU:\n");
            visit_1905_CMDU_structure(cmdu, print_callback, PLATFORM_PRINTF_DEBUG_INFO, "");
        }
        else
        {
            PLATFORM_PRINTF_DEBUG_DETAIL("Received expected %s\n", testname);
            ret = 0;
        }
        free_1905_CMDU_structure(cmdu);
    }
    return ret;
}
