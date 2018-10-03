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
// This file tests the "forge_lldp_PAYLOAD_from_structure()" function by
// providing some test input structures and checking the generated output
// stream.
//

#include "platform.h"
#include "lldp_payload.h"
#include "lldp_tlvs.h"
#include "lldp_payload_test_vectors.h"

#include <string.h> // memcmp(), memcpy(), ...

uint8_t _check(const char *test_description, struct PAYLOAD *input, uint8_t *expected_output, uint16_t expected_output_len)
{
    uint8_t  result;
    uint8_t *real_output;
    uint16_t real_output_len;

    real_output = forge_lldp_PAYLOAD_from_structure(input, &real_output_len);

    if (NULL == real_output)
    {
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  forge_lldp_PAYLOAD_from_structure() returned a NULL pointer\n");

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

    #define LLDPPAYLOADFORGE001 "LLDPPAYLOADFORGE001 - Forge LLDP bridge discovery message (lldp_payload_structure_001)"
    result += _check(LLDPPAYLOADFORGE001, &lldp_payload_structure_001, lldp_payload_stream_001, lldp_payload_stream_len_001);

    // Return the number of test cases that failed
    //
    return result;
}


