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
// This file tests the "parse_lldp_PAYLOAD_from_packet()" function by providing
// some test input streams and checking the generated output structure.
//

#include "platform.h"
#include "utils.h"

#include "lldp_payload.h"
#include "lldp_tlvs.h"
#include "lldp_payload_test_vectors.h"

uint8_t _check(const char *test_description, uint8_t *input, struct PAYLOAD *expected_output)
{
    uint8_t  result;
    struct PAYLOAD *real_output;

    real_output = parse_lldp_PAYLOAD_from_packet(input);

    if (0 == compare_lldp_PAYLOAD_structures(real_output, expected_output))
    {
        result = 0;
        PLATFORM_PRINTF("%-100s: OK\n", test_description);
    }
    else
    {
        result = 1;
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  Expected output:\n");
        visit_lldp_PAYLOAD_structure(expected_output, print_callback, PLATFORM_PRINTF, "");
        PLATFORM_PRINTF("  Real output    :\n");
        visit_lldp_PAYLOAD_structure(real_output, print_callback, PLATFORM_PRINTF, "");
    }

    return result;
}


int main(void)
{
    uint8_t result = 0;

    #define LLDPPAYLOADPARSE001 "LLDPPAYLOADPARSE001 - Parse LLDP bridge discovery message (lldp_payload_stream_001)"
    result += _check(LLDPPAYLOADPARSE001, lldp_payload_stream_001, &lldp_payload_structure_001);

    // Return the number of test cases that failed
    //
    return result;
}






