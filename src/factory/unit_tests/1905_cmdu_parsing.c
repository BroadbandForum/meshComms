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

//
// This file tests the "parse_1905_CMDU_from_packets()" function by providing
// some test input streams and checking the generated output structure.
//

#include "platform.h"
#include "utils.h"

#include "1905_tlvs.h"
#include "1905_cmdus.h"
#include "1905_cmdu_test_vectors.h"

#include <string.h> // memcmp

static int check_parse_1905_cmdu(const char *test_description, uint8_t **input, struct CMDU *expected_output)
{
    int result;
    struct CMDU *real_output;

    real_output = parse_1905_CMDU_from_packets(input);

    if (0 == compare_1905_CMDU_structures(real_output, expected_output))
    {
        result = 0;
        PLATFORM_PRINTF("%-100s: OK\n", test_description);
    }
    else
    {
        result = 1;
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  Expected output:\n");
        visit_1905_CMDU_structure(expected_output, print_callback, PLATFORM_PRINTF, "");
        PLATFORM_PRINTF("  Real output    :\n");
        visit_1905_CMDU_structure(real_output, print_callback, PLATFORM_PRINTF, "");
    }

    return result;
}

static int check_parse_1905_cmdu_header(const char *test_description, uint8_t *input, size_t input_len,
                                        struct CMDU_header *expected_output)
{
    int result = 1;
    struct CMDU_header real_output;

    memset(&real_output, 0x42, sizeof(real_output));

    if (parse_1905_CMDU_header_from_packet(input, input_len, &real_output))
    {
        if (NULL != expected_output)
        {
            if (0 == memcmp(expected_output, &real_output, sizeof(real_output)))
            {
                result = 0;
            }
        }
        // Else failed because we expected parse to fail
    }
    else
    {
        if (NULL == expected_output)
        {
            result = 0;
        }
    }

    if (0 == result)
    {
        PLATFORM_PRINTF("%-100s: OK\n", test_description);
    }
    else
    {
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        if (NULL != expected_output)
        {
            PLATFORM_PRINTF("  Expected output:\n    dst_addr: " MACSTR "\n    src_addr: " MACSTR "\n"
                            "    MID: 0x%04x FID: 0x%02x Last fragment: %d\n",
                            MAC2STR(expected_output->dst_addr), MAC2STR(expected_output->src_addr),
                            expected_output->mid, expected_output->fragment_id, expected_output->last_fragment_indicator);
        }
        PLATFORM_PRINTF("  Real output:\n    dst_addr: " MACSTR "\n    src_addr: " MACSTR "\n"
                        "    MID: 0x%04x FID: 0x%02x Last fragment: %d\n",
                        MAC2STR(real_output.dst_addr), MAC2STR(real_output.src_addr),
                        real_output.mid, real_output.fragment_id, real_output.last_fragment_indicator);
    }

    return result;
}

int main(void)
{
    int result = 0;

    #define x1905CMDUPARSE001 "x1905CMDUPARSE001 - Parse link metric query CMDU (x1905_cmdu_streams_001)"
    result += check_parse_1905_cmdu(x1905CMDUPARSE001, x1905_cmdu_streams_001, &x1905_cmdu_structure_001);

    #define x1905CMDUPARSE002 "x1905CMDUPARSE002 - Parse link metric query CMDU (x1905_cmdu_streams_002)"
    result += check_parse_1905_cmdu(x1905CMDUPARSE002, x1905_cmdu_streams_002, &x1905_cmdu_structure_002);

    #define x1905CMDUPARSE003 "x1905CMDUPARSE003 - Parse link metric query CMDU (x1905_cmdu_streams_004)"
    result += check_parse_1905_cmdu(x1905CMDUPARSE003, x1905_cmdu_streams_004, &x1905_cmdu_structure_004);

    #define x1905CMDUPARSE004 "x1905CMDUPARSE004 - Parse topology query CMDU (x1905_cmdu_streams_005)"
    result += check_parse_1905_cmdu(x1905CMDUPARSE004, x1905_cmdu_streams_005, &x1905_cmdu_structure_005);

    result += check_parse_1905_cmdu_header("x1905CMDUPARSEHDR001 - Parse CMDU packet last fragment",
                                           x1905_cmdu_packet_001, x1905_cmdu_packet_len_001, &x1905_cmdu_header_001);

    result += check_parse_1905_cmdu_header("x1905CMDUPARSEHDR002 - Parse CMDU packet not last fragment",
                                           x1905_cmdu_packet_002, x1905_cmdu_packet_len_002, &x1905_cmdu_header_002);

    result += check_parse_1905_cmdu_header("x1905CMDUPARSEHDR003 - Parse CMDU packet wrong ethertype",
                                           x1905_cmdu_packet_003, x1905_cmdu_packet_len_003, NULL);

    result += check_parse_1905_cmdu_header("x1905CMDUPARSEHDR004 - Parse CMDU packet too short",
                                           x1905_cmdu_packet_004, x1905_cmdu_packet_len_004, NULL);

    // Return the number of test cases that failed
    //
    return result;
}







