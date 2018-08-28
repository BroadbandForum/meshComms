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
// This file tests the "parse_non_1905_TLV_from_packet()" function by providing
// some test input streams and checking the generated output structure.
//

#include "platform.h"
#include "utils.h"

#include "bbf_tlvs.h"
#include "bbf_tlv_test_vectors.h"

uint8_t _check(const char *test_description, uint8_t mode, uint8_t *input, uint8_t *expected_output)
{
    uint8_t  result;
    uint8_t *real_output;
    uint8_t  comparison;

    // Parse the packet
    real_output = parse_bbf_TLV_from_packet(input);

    // Compare TLVs
    comparison = compare_bbf_TLV_structures(real_output, expected_output);

    if (mode == CHECK_TRUE)
    {
        if (0 == comparison)
        {
            result = 0;
            PLATFORM_PRINTF("%-100s: OK\n", test_description);
        }
        else
        {
            result = 1;
            PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);

            // CheckTrue error needs more debug info
            PLATFORM_PRINTF("  Expected output:\n");
            visit_bbf_TLV_structure(expected_output, print_callback, PLATFORM_PRINTF, "");
            PLATFORM_PRINTF("  Real output    :\n");
            visit_bbf_TLV_structure(real_output, print_callback, PLATFORM_PRINTF, "");
        }
    }
    else
    {
        if (0 == comparison)
        {
            result = 1;
            PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        }
        else
        {
            result = 0;
            PLATFORM_PRINTF("%-100s: OK\n", test_description);
        }
    }

    return result;
}

uint8_t _checkTrue(const char *test_description, uint8_t *input, uint8_t *expected_output)
{
  return _check(test_description, CHECK_TRUE, input, expected_output);
}

uint8_t _checkFalse(const char *test_description, uint8_t *input, uint8_t *expected_output)
{
  return _check(test_description, CHECK_FALSE, input, expected_output);
}


int main(void)
{
    uint8_t result = 0;

    #define BBFTLVPARSE001 "BBFTLVPARSE001 - Parse non-1905 link metric query TLV (bbf_tlv_stream_001)"
    result += _checkTrue(BBFTLVPARSE001, bbf_tlv_stream_001, (uint8_t *)&bbf_tlv_structure_001);

    #define BBFTLVFORGE002 "BBFTLVPARSE002 - Parse non-1905 link metric query TLV (bbf_tlv_stream_003)"
    result += _checkTrue(BBFTLVFORGE002, bbf_tlv_stream_003, (uint8_t *)&bbf_tlv_structure_003);

    #define BBFTLVFORGE003 "BBFTLVPARSE003 - Parse non-1905 transmitter link metric TLV (bbf_tlv_stream_005)"
    result += _checkTrue(BBFTLVFORGE003, bbf_tlv_stream_005, (uint8_t *)&bbf_tlv_structure_005);

    #define BBFTLVFORGE004 "BBFTLVPARSE004 - Parse non-1905 receiver link metric TLV (bbf_tlv_stream_007)"
    result += _checkTrue(BBFTLVFORGE004, bbf_tlv_stream_007, (uint8_t *)&bbf_tlv_structure_007);

    #define BBFTLVFORGE005 "BBFTLVPARSE005 - Parse non-1905 link metric query TLV (bbf_tlv_stream_008)"
    result += _checkFalse(BBFTLVFORGE005, bbf_tlv_stream_002b, (uint8_t *)&bbf_tlv_structure_002);

    #define BBFTLVFORGE006 "BBFTLVPARSE006 - Parse non-1905 transmitter link metric TLV (bbf_tlv_stream_009)"
    result += _checkFalse(BBFTLVFORGE006, bbf_tlv_stream_004b, (uint8_t *)&bbf_tlv_structure_004);

    #define BBFTLVFORGE007 "BBFTLVPARSE007 - Parse non-1905 receiver link metric TLV (bbf_tlv_stream_010)"
    result += _checkFalse(BBFTLVFORGE007, bbf_tlv_stream_006b, (uint8_t *)&bbf_tlv_structure_006);

    // Return the number of test cases that failed
    //
    return result;
}






