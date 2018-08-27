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
// This file tests the "forge_non_1905_TLV_from_structure()" function by
// providing some test input structures and checking the generated output
// stream.
//

#include "platform.h"
#include "bbf_tlvs.h"
#include "bbf_tlv_test_vectors.h"

#include <string.h> // memcmp(), memcpy(), ...

INT8U _check(const char *test_description, INT8U mode, INT8U *input, INT8U *expected_output, INT16U expected_output_len)
{
    INT8U  result;
    INT8U *real_output;
    INT16U real_output_len;

    // Build the packet
    real_output = forge_bbf_TLV_from_structure((INT8U *)input, &real_output_len);

    if (NULL == real_output)
    {
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  forge_non_1905_TLV_from_structure() returned a NULL pointer\n");

        return 1;
    }

    // Compare packets
    if (mode == CHECK_TRUE)
    {
        // Compare the packets
        if ((expected_output_len == real_output_len) && (0 == memcmp(expected_output, real_output, real_output_len)))
        {
            result = 0;
            PLATFORM_PRINTF("%-100s: OK\n", test_description);
        }
        else
        {
            INT16U i;

            result = 1;
            PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);

            // CheckTrue error needs more debug info
            //
            PLATFORM_PRINTF("  Expected output: ");
            for (i=0; i<expected_output_len; i++)
            {
                PLATFORM_PRINTF("%02x ",expected_output[i]);
            }
            PLATFORM_PRINTF("\n");
            PLATFORM_PRINTF("  Real output    : ");
            for (i=0; i<real_output_len; i++)
            {
                PLATFORM_PRINTF("%02x ",real_output[i]);
            }
            PLATFORM_PRINTF("\n");
        }
    }
    else
    {
        if ((expected_output_len == real_output_len) && (0 == memcmp(expected_output, real_output, real_output_len)))
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

INT8U _checkTrue(const char *test_description, INT8U *input, INT8U *expected_output, INT16U expected_output_len)
{
  return _check(test_description, CHECK_TRUE, input, expected_output, expected_output_len);
}

INT8U _checkFalse(const char *test_description, INT8U *input, INT8U *expected_output, INT16U expected_output_len)
{
  return _check(test_description, CHECK_FALSE, input, expected_output, expected_output_len);
}


int main(void)
{
    INT8U result = 0;

    #define BBFTLVFORGE001 "BBFTLVFORGE001 - Forge non-1905 link metric query TLV (bbf_tlv_structure_001)"
    result += _checkTrue(BBFTLVFORGE001, (INT8U *)&bbf_tlv_structure_001, bbf_tlv_stream_001, bbf_tlv_stream_len_001);

    #define BBFTLVFORGE002 "BBFTLVFORGE002 - Forge non-1905 link metric query TLV (bbf_tlv_structure_002)"
    result += _checkTrue(BBFTLVFORGE002, (INT8U *)&bbf_tlv_structure_002, bbf_tlv_stream_002, bbf_tlv_stream_len_002);

    #define BBFTLVFORGE003 "BBFTLVFORGE003 - Forge non-1905 link metric query TLV (bbf_tlv_structure_003)"
    result += _checkTrue(BBFTLVFORGE003, (INT8U *)&bbf_tlv_structure_003, bbf_tlv_stream_003, bbf_tlv_stream_len_003);

    #define BBFTLVFORGE004 "BBFTLVFORGE004 - Forge non-1905 transmitter link metric TLV (bbf_tlv_structure_004)"
    result += _checkTrue(BBFTLVFORGE004, (INT8U *)&bbf_tlv_structure_004, bbf_tlv_stream_004, bbf_tlv_stream_len_004);

    #define BBFTLVFORGE005 "BBFTLVFORGE005 - Forge non-1905 transmitter link metric TLV (bbf_tlv_structure_005)"
    result += _checkTrue(BBFTLVFORGE005, (INT8U *)&bbf_tlv_structure_005, bbf_tlv_stream_005, bbf_tlv_stream_len_005);

    #define BBFTLVFORGE006 "BBFTLVFORGE006 - Forge non-1905 receiver link metric TLV (bbf_tlv_structure_006)"
    result += _checkTrue(BBFTLVFORGE006, (INT8U *)&bbf_tlv_structure_006, bbf_tlv_stream_006, bbf_tlv_stream_len_006);

    #define BBFTLVFORGE007 "BBFTLVFORGE007 - Forge non-1905 receiver link metric TLV (bbf_tlv_structure_007)"
    result += _checkTrue(BBFTLVFORGE007, (INT8U *)&bbf_tlv_structure_007, bbf_tlv_stream_007, bbf_tlv_stream_len_007);

    // Return the number of test cases that failed
    //
    return result;
}

