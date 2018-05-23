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
// This file tests the "parse_lldp_TLV_from_packet()" function by providing
// some test input streams and checking the generated output structure.
//

#include "platform.h"
#include "utils.h"

#include "lldp_tlvs.h"
#include "lldp_tlv_test_vectors.h"

INT8U _check(const char *test_description, INT8U *input, INT8U *expected_output)
{
    INT8U  result;
    INT8U *real_output;

    real_output = parse_lldp_TLV_from_packet(input);

    if (0 == compare_lldp_TLV_structures(real_output, expected_output))
    {
        result = 0;
        PLATFORM_PRINTF("%-100s: OK\n", test_description);
    }
    else
    {
        result = 1;
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  Expected output:\n");
        visit_lldp_TLV_structure(expected_output, print_callback, PLATFORM_PRINTF, "");
        PLATFORM_PRINTF("  Real output    :\n");
        visit_lldp_TLV_structure(real_output, print_callback, PLATFORM_PRINTF, "");
    }

    return result;
}


int main(void)
{
    INT8U result = 0;

    #define LLDPTLVPARSE001 "LLDPTLVPARSE001 - Parse end of LLDP TLV (lldp_tlv_stream_001)"
    result += _check(LLDPTLVPARSE001, lldp_tlv_stream_001, (INT8U *)&lldp_tlv_structure_001);

    #define LLDPTLVPARSE002 "LLDPTLVPARSE002 - Parse chassis ID TLV (lldp_tlv_stream_002)"
    result += _check(LLDPTLVPARSE002, lldp_tlv_stream_002, (INT8U *)&lldp_tlv_structure_002);

    #define LLDPTLVPARSE003 "LLDPTLVPARSE003 - Parse port ID TLV (lldp_tlv_stream_003)"
    result += _check(LLDPTLVPARSE003, lldp_tlv_stream_003, (INT8U *)&lldp_tlv_structure_003);

    #define LLDPTLVPARSE004 "LLDPTLVPARSE004 - Parse time to live TLV (lldp_tlv_stream_004)"
    result += _check(LLDPTLVPARSE004, lldp_tlv_stream_004, (INT8U *)&lldp_tlv_structure_004);

    // Return the number of test cases that failed
    //
    return result;
}






