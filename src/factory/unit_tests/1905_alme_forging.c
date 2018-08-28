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
// This file tests the "forge_1905_ALME_from_structure()" function by providing
// some test input structures and checking the generated output stream.
//

#include "platform.h"
#include "1905_tlvs.h"
#include "1905_alme.h"
#include "1905_alme_test_vectors.h"

#include <string.h> // memcmp(), memcpy(), ...

uint8_t _check(const char *test_description, uint8_t *input, uint8_t *expected_output, uint16_t expected_output_len)
{
    uint8_t  result;
    uint8_t *real_output;
    uint16_t real_output_len;

    real_output = forge_1905_ALME_from_structure((uint8_t *)input, &real_output_len);

    if (NULL == real_output)
    {
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  forge_1905_ALME_from_structure() returned a NULL pointer\n");

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

    #define x1905ALMEFORGE001 "x1905ALMEFORGE001 - Forge ALME-GET-INTF-LIST.request (x1905_alme_structure_001)"
    result += _check(x1905ALMEFORGE001, (uint8_t *)&x1905_alme_structure_001, x1905_alme_stream_001, x1905_alme_stream_len_001);

    #define x1905ALMEFORGE002 "x1905ALMEFORGE002 - Forge ALME-GET-INTF-LIST.response (x1905_alme_structure_002)"
    result += _check(x1905ALMEFORGE002, (uint8_t *)&x1905_alme_structure_002, x1905_alme_stream_002, x1905_alme_stream_len_002);

    #define x1905ALMEFORGE003 "x1905ALMEFORGE003 - Forge ALME-GET-INTF-LIST.response (x1905_alme_structure_003)"
    result += _check(x1905ALMEFORGE003, (uint8_t *)&x1905_alme_structure_003, x1905_alme_stream_003, x1905_alme_stream_len_003);

    #define x1905ALMEFORGE004 "x1905ALMEFORGE004 - Forge ALME-GET-INTF-LIST.response (x1905_alme_structure_004)"
    result += _check(x1905ALMEFORGE004, (uint8_t *)&x1905_alme_structure_004, x1905_alme_stream_004, x1905_alme_stream_len_004);

    #define x1905ALMEFORGE005 "x1905ALMEFORGE005 - Forge ALME-SET-INTF-PWR-STATE.request (x1905_alme_structure_005)"
    result += _check(x1905ALMEFORGE005, (uint8_t *)&x1905_alme_structure_005, x1905_alme_stream_005, x1905_alme_stream_len_005);

    #define x1905ALMEFORGE006 "x1905ALMEFORGE006 - Forge ALME-SET-INTF-PWR-STATE.request (x1905_alme_structure_006)"
    result += _check(x1905ALMEFORGE006, (uint8_t *)&x1905_alme_structure_006, x1905_alme_stream_006, x1905_alme_stream_len_006);

    #define x1905ALMEFORGE007 "x1905ALMEFORGE007 - Forge ALME-SET-INTF-PWR-STATE.confirm (x1905_alme_structure_007)"
    result += _check(x1905ALMEFORGE007, (uint8_t *)&x1905_alme_structure_007, x1905_alme_stream_007, x1905_alme_stream_len_007);

    #define x1905ALMEFORGE008 "x1905ALMEFORGE008 - Forge ALME-SET-INTF-PWR-STATE.confirm (x1905_alme_structure_008)"
    result += _check(x1905ALMEFORGE008, (uint8_t *)&x1905_alme_structure_008, x1905_alme_stream_008, x1905_alme_stream_len_008);

    #define x1905ALMEFORGE009 "x1905ALMEFORGE009 - Forge ALME-GET-INTF-PWR-STATE.request (x1905_alme_structure_009)"
    result += _check(x1905ALMEFORGE009, (uint8_t *)&x1905_alme_structure_009, x1905_alme_stream_009, x1905_alme_stream_len_009);

    #define x1905ALMEFORGE010 "x1905ALMEFORGE010 - Forge ALME-GET-INTF-PWR-STATE.response (x1905_alme_structure_010)"
    result += _check(x1905ALMEFORGE010, (uint8_t *)&x1905_alme_structure_010, x1905_alme_stream_010, x1905_alme_stream_len_010);

    #define x1905ALMEFORGE011 "x1905ALMEFORGE011 - Forge ALME-SET-FWD-RULE.request (x1905_alme_structure_011)"
    result += _check(x1905ALMEFORGE011, (uint8_t *)&x1905_alme_structure_011, x1905_alme_stream_011, x1905_alme_stream_len_011);

    #define x1905ALMEFORGE012 "x1905ALMEFORGE012 - Forge ALME-SET-FWD-RULE.request (x1905_alme_structure_012)"
    result += _check(x1905ALMEFORGE012, (uint8_t *)&x1905_alme_structure_012, x1905_alme_stream_012, x1905_alme_stream_len_012);

    #define x1905ALMEFORGE013 "x1905ALMEFORGE013 - Forge ALME-SET-FWD-RULE.confirm (x1905_alme_structure_013)"
    result += _check(x1905ALMEFORGE013, (uint8_t *)&x1905_alme_structure_013, x1905_alme_stream_013, x1905_alme_stream_len_013);

    #define x1905ALMEFORGE014 "x1905ALMEFORGE014 - Forge ALME-GET-FWD-RULES.request (x1905_alme_structure_014)"
    result += _check(x1905ALMEFORGE014, (uint8_t *)&x1905_alme_structure_014, x1905_alme_stream_014, x1905_alme_stream_len_014);

    #define x1905ALMEFORGE015 "x1905ALMEFORGE015 - Forge ALME-GET-FWD-RULES.response (x1905_alme_structure_015)"
    result += _check(x1905ALMEFORGE015, (uint8_t *)&x1905_alme_structure_015, x1905_alme_stream_015, x1905_alme_stream_len_015);

    #define x1905ALMEFORGE016 "x1905ALMEFORGE016 - Forge ALME-GET-FWD-RULES.response (x1905_alme_structure_016)"
    result += _check(x1905ALMEFORGE016, (uint8_t *)&x1905_alme_structure_016, x1905_alme_stream_016, x1905_alme_stream_len_016);

    #define x1905ALMEFORGE017 "x1905ALMEFORGE017 - Forge ALME-GET-FWD-RULES.response (x1905_alme_structure_017)"
    result += _check(x1905ALMEFORGE017, (uint8_t *)&x1905_alme_structure_017, x1905_alme_stream_017, x1905_alme_stream_len_017);

    #define x1905ALMEFORGE018 "x1905ALMEFORGE018 - Forge ALME-MODIFY-FWD-RULE.request (x1905_alme_structure_018)"
    result += _check(x1905ALMEFORGE018, (uint8_t *)&x1905_alme_structure_018, x1905_alme_stream_018, x1905_alme_stream_len_018);

    #define x1905ALMEFORGE019 "x1905ALMEFORGE019 - Forge ALME-MODIFY-FWD-RULE.confirm (x1905_alme_structure_019)"
    result += _check(x1905ALMEFORGE019, (uint8_t *)&x1905_alme_structure_019, x1905_alme_stream_019, x1905_alme_stream_len_019);

    #define x1905ALMEFORGE020 "x1905ALMEFORGE020 - Forge ALME-MODIFY-FWD-RULE.confirm (x1905_alme_structure_020)"
    result += _check(x1905ALMEFORGE020, (uint8_t *)&x1905_alme_structure_020, x1905_alme_stream_020, x1905_alme_stream_len_020);

    #define x1905ALMEFORGE021 "x1905ALMEFORGE021 - Forge ALME-REMOVE-FWD-RULE.request (x1905_alme_structure_021)"
    result += _check(x1905ALMEFORGE021, (uint8_t *)&x1905_alme_structure_021, x1905_alme_stream_021, x1905_alme_stream_len_021);

    #define x1905ALMEFORGE022 "x1905ALMEFORGE022 - Forge ALME-REMOVE-FWD-RULE.confirm (x1905_alme_structure_022)"
    result += _check(x1905ALMEFORGE022, (uint8_t *)&x1905_alme_structure_022, x1905_alme_stream_022, x1905_alme_stream_len_022);

    #define x1905ALMEFORGE023 "x1905ALMEFORGE023 - Forge ALME-GET-METRIC.request (x1905_alme_structure_023)"
    result += _check(x1905ALMEFORGE023, (uint8_t *)&x1905_alme_structure_023, x1905_alme_stream_023, x1905_alme_stream_len_023);

    #define x1905ALMEFORGE024 "x1905ALMEFORGE024 - Forge ALME-GET-METRIC.response (x1905_alme_structure_024)"
    result += _check(x1905ALMEFORGE024, (uint8_t *)&x1905_alme_structure_024, x1905_alme_stream_024, x1905_alme_stream_len_024);

    #define x1905ALMEFORGE025 "x1905ALMEFORGE025 - Forge ALME-GET-METRIC.response (x1905_alme_structure_025)"
    result += _check(x1905ALMEFORGE025, (uint8_t *)&x1905_alme_structure_025, x1905_alme_stream_025, x1905_alme_stream_len_025);

    // Return the number of test cases that failed
    //
    return result;
}






