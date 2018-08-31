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
// This file tests the "forge_lldp_TLV_from_structure()" function by providing
// some test input structures and checking the generated output stream.
//

#include "platform.h"
#include "lldp_tlvs.h"
#include "lldp_tlv_test_vectors.h"

#include <string.h> // memcmp(), memcpy(), ...

uint8_t _check(const char *test_description, struct tlv *input, uint8_t *expected_output, uint16_t expected_output_len)
{
    uint8_t  result;
    uint8_t *real_output;
    uint16_t real_output_len;

    real_output = forge_lldp_TLV_from_structure(input, &real_output_len);

    if (NULL == real_output)
    {
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  forge_lldp_TLV_from_structure() returned a NULL pointer\n");

        return 1;
    }

    if ((expected_output_len == real_output_len) && (0 == memcmp(expected_output, real_output, real_output_len)))
    {
        result = 0;
        PLATFORM_PRINTF("%-100s: OK\n", test_description);
    }
    else
    {
        uint16_t i;

        result = 1;
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
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

    return result;
}


int main(void)
{
    uint8_t result = 0;

    #define LLDPTLVFORGE001 "LLDPTLVFORGE001 - Forge end of LLDP TLV (lldp_tlv_structure_001)"
    result += _check(LLDPTLVFORGE001, &lldp_tlv_structure_001.tlv, lldp_tlv_stream_001, lldp_tlv_stream_len_001);

    #define LLDPTLVFORGE002 "LLDPTLVFORGE002 - Forge chassis ID TLV (lldp_tlv_structure_002)"
    result += _check(LLDPTLVFORGE002, &lldp_tlv_structure_002.tlv, lldp_tlv_stream_002, lldp_tlv_stream_len_002);

    #define LLDPTLVFORGE003 "LLDPTLVFORGE003 - Forge port ID TLV (lldp_tlv_structure_003)"
    result += _check(LLDPTLVFORGE003, &lldp_tlv_structure_003.tlv, lldp_tlv_stream_003, lldp_tlv_stream_len_003);

    #define LLDPTLVFORGE004 "LLDPTLVFORGE004 - Forge time to live TLV (lldp_tlv_structure_004)"
    result += _check(LLDPTLVFORGE004, &lldp_tlv_structure_004.tlv, lldp_tlv_stream_004, lldp_tlv_stream_len_004);

    // Return the number of test cases that failed
    //
    return result;
}






