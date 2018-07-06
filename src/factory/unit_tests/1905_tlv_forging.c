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
// This file tests the "forge_1905_TLV_from_structure()" function by providing
// some test input structures and checking the generated output stream.
//

#include "platform.h"
#include "1905_tlvs.h"
#include "1905_tlv_test_vectors.h"

INT8U _check(const char *test_description, INT8U *input, INT8U *expected_output, INT16U expected_output_len)
{
    INT8U  result;
    INT8U *real_output;
    INT16U real_output_len;

    real_output = forge_1905_TLV_from_structure((INT8U *)input, &real_output_len);

    if (NULL == real_output)
    {
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  forge_1905_TLV_from_structure() returned a NULL pointer\n");

        return 1;
    }

    if ((expected_output_len == real_output_len) && (0 == PLATFORM_MEMCMP(expected_output, real_output, real_output_len)))
    {
        result = 0;
        PLATFORM_PRINTF("%-100s: OK\n", test_description);
    }
    else
    {
        INT16U i;

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
    free_1905_TLV_packet(real_output);

    return result;
}


int main(void)
{
    INT8U result = 0;

    #define x1905TLVFORGE001 "x1905TLVFORGE001 - Forge link metric query TLV (x1905_tlv_structure_001)"
    result += _check(x1905TLVFORGE001, (INT8U *)&x1905_tlv_structure_001, x1905_tlv_stream_001, x1905_tlv_stream_len_001);

    #define x1905TLVFORGE002 "x1905TLVFORGE002 - Forge link metric query TLV (x1905_tlv_structure_002)"
    result += _check(x1905TLVFORGE002, (INT8U *)&x1905_tlv_structure_002, x1905_tlv_stream_002, x1905_tlv_stream_len_002);

    #define x1905TLVFORGE003 "x1905TLVFORGE003 - Forge transmitter link metric TLV (x1905_tlv_structure_004)"
    result += _check(x1905TLVFORGE003, (INT8U *)&x1905_tlv_structure_004, x1905_tlv_stream_004, x1905_tlv_stream_len_004);

    #define x1905TLVFORGE004 "x1905TLVFORGE004 - Forge transmitter link metric TLV (x1905_tlv_structure_005)"
    result += _check(x1905TLVFORGE004, (INT8U *)&x1905_tlv_structure_005, x1905_tlv_stream_005, x1905_tlv_stream_len_005);

    #define x1905TLVFORGE005 "x1905TLVFORGE005 - Forge receiver link metric TLV (x1905_tlv_structure_006)"
    result += _check(x1905TLVFORGE005, (INT8U *)&x1905_tlv_structure_006, x1905_tlv_stream_006, x1905_tlv_stream_len_006);

    #define x1905TLVFORGE006 "x1905TLVFORGE006 - Forge receiver link metric TLV (x1905_tlv_structure_007)"
    result += _check(x1905TLVFORGE006, (INT8U *)&x1905_tlv_structure_007, x1905_tlv_stream_007, x1905_tlv_stream_len_007);

    #define x1905TLVFORGE007 "x1905TLVFORGE007 - Forge AL MAC address type TLV (x1905_tlv_structure_008)"
    result += _check(x1905TLVFORGE007, (INT8U *)&x1905_tlv_structure_008, x1905_tlv_stream_008, x1905_tlv_stream_len_008);

    #define x1905TLVFORGE008 "x1905TLVFORGE008 - Forge MAC address type TLV (x1905_tlv_structure_009)"
    result += _check(x1905TLVFORGE008, (INT8U *)&x1905_tlv_structure_009, x1905_tlv_stream_009, x1905_tlv_stream_len_009);

    #define x1905TLVFORGE009 "x1905TLVFORGE009 - Forge device information type TLV (x1905_tlv_structure_010)"
    result += _check(x1905TLVFORGE009, (INT8U *)&x1905_tlv_structure_010, x1905_tlv_stream_010, x1905_tlv_stream_len_010);

    #define x1905TLVFORGE010 "x1905TLVFORGE010 - Forge device bridging capability TLV (x1905_tlv_structure_011)"
    result += _check(x1905TLVFORGE010, (INT8U *)&x1905_tlv_structure_011, x1905_tlv_stream_011, x1905_tlv_stream_len_011);

    #define x1905TLVFORGE011 "x1905TLVFORGE011 - Forge device bridging capability TLV (x1905_tlv_structure_012)"
    result += _check(x1905TLVFORGE011, (INT8U *)&x1905_tlv_structure_012, x1905_tlv_stream_012, x1905_tlv_stream_len_012);

    #define x1905TLVFORGE012 "x1905TLVFORGE012 - Forge device bridging capability TLV (x1905_tlv_structure_013)"
    result += _check(x1905TLVFORGE012, (INT8U *)&x1905_tlv_structure_013, x1905_tlv_stream_013, x1905_tlv_stream_len_013);

    #define x1905TLVFORGE013 "x1905TLVFORGE013 - Forge non 1905 neighbor device list TLV (x1905_tlv_structure_014)"
    result += _check(x1905TLVFORGE013, (INT8U *)&x1905_tlv_structure_014, x1905_tlv_stream_014, x1905_tlv_stream_len_014);

    #define x1905TLVFORGE014 "x1905TLVFORGE014 - Forge non 1905 neighbor device list TLV (x1905_tlv_structure_015)"
    result += _check(x1905TLVFORGE014, (INT8U *)&x1905_tlv_structure_015, x1905_tlv_stream_015, x1905_tlv_stream_len_015);

    #define x1905TLVFORGE015 "x1905TLVFORGE015 - Forge neighbor device list TLV (x1905_tlv_structure_016)"
    result += _check(x1905TLVFORGE015, (INT8U *)&x1905_tlv_structure_016, x1905_tlv_stream_016, x1905_tlv_stream_len_016);

    #define x1905TLVFORGE016 "x1905TLVFORGE016 - Forge neighbor device list TLV (x1905_tlv_structure_017)"
    result += _check(x1905TLVFORGE016, (INT8U *)&x1905_tlv_structure_017, x1905_tlv_stream_017, x1905_tlv_stream_len_017);

    #define x1905TLVFORGE017 "x1905TLVFORGE017 - Forge link metric result code TLV (x1905_tlv_structure_018)"
    result += _check(x1905TLVFORGE017, (INT8U *)&x1905_tlv_structure_018, x1905_tlv_stream_018, x1905_tlv_stream_len_018);

    #define x1905TLVFORGE018 "x1905TLVFORGE018 - Forge link metric result code TLV (x1905_tlv_structure_020)"
    result += _check(x1905TLVFORGE018, (INT8U *)&x1905_tlv_structure_020, x1905_tlv_stream_020, x1905_tlv_stream_len_020);

    #define x1905TLVFORGE019 "x1905TLVFORGE019 - Forge autoconfig freq band TLV (x1905_tlv_structure_022)"
    result += _check(x1905TLVFORGE019, (INT8U *)&x1905_tlv_structure_022, x1905_tlv_stream_022, x1905_tlv_stream_len_022);

    #define x1905TLVFORGE020 "x1905TLVFORGE020 - Forge supported role TLV (x1905_tlv_structure_024)"
    result += _check(x1905TLVFORGE020, (INT8U *)&x1905_tlv_structure_024, x1905_tlv_stream_024, x1905_tlv_stream_len_024);

    #define x1905TLVFORGE021 "x1905TLVFORGE021 - Forge supported freq band TLV (x1905_tlv_structure_026)"
    result += _check(x1905TLVFORGE021, (INT8U *)&x1905_tlv_structure_026, x1905_tlv_stream_026, x1905_tlv_stream_len_026);

    #define x1905TLVFORGE022 "x1905TLVFORGE022 - Forge push button event notification TLV (x1905_tlv_structure_028)"
    result += _check(x1905TLVFORGE022, (INT8U *)&x1905_tlv_structure_028, x1905_tlv_stream_028, x1905_tlv_stream_len_028);

    #define x1905TLVFORGE023 "x1905TLVFORGE023 - Forge power off interface TLV (x1905_tlv_structure_029)"
    result += _check(x1905TLVFORGE023, (INT8U *)&x1905_tlv_structure_029, x1905_tlv_stream_029, x1905_tlv_stream_len_029);

    #define x1905TLVFORGE024 "x1905TLVFORGE024 - Forge power off interface TLV (x1905_tlv_structure_030)"
    result += _check(x1905TLVFORGE024, (INT8U *)&x1905_tlv_structure_030, x1905_tlv_stream_030, x1905_tlv_stream_len_030);

    #define x1905TLVFORGE025 "x1905TLVFORGE025 - Forge generic PHY device information type TLV (x1905_tlv_structure_031)"
    result += _check(x1905TLVFORGE025, (INT8U *)&x1905_tlv_structure_031, x1905_tlv_stream_031, x1905_tlv_stream_len_031);

    #define x1905TLVFORGE026 "x1905TLVFORGE026 - Forge push button generic PHY event notification TLV (x1905_tlv_structure_032)"
    result += _check(x1905TLVFORGE026, (INT8U *)&x1905_tlv_structure_032, x1905_tlv_stream_032, x1905_tlv_stream_len_032);

    #define x1905TLVFORGE027 "x1905TLVFORGE027 - Forge device identification type TLV (x1905_tlv_structure_033)"
    result += _check(x1905TLVFORGE027, (INT8U *)&x1905_tlv_structure_033, x1905_tlv_stream_033, x1905_tlv_stream_len_033);

    #define x1905TLVFORGE028 "x1905TLVFORGE028 - Forge control URL type TLV (x1905_tlv_structure_034)"
    result += _check(x1905TLVFORGE028, (INT8U *)&x1905_tlv_structure_034, x1905_tlv_stream_034, x1905_tlv_stream_len_034);

    #define x1905TLVFORGE029 "x1905TLVFORGE029 - Forge IPv4 type TLV (x1905_tlv_structure_035)"
    result += _check(x1905TLVFORGE029, (INT8U *)&x1905_tlv_structure_035, x1905_tlv_stream_035, x1905_tlv_stream_len_035);

    #define x1905TLVFORGE030 "x1905TLVFORGE030 - Forge IPv6 type TLV (x1905_tlv_structure_036)"
    result += _check(x1905TLVFORGE030, (INT8U *)&x1905_tlv_structure_036, x1905_tlv_stream_036, x1905_tlv_stream_len_036);

    #define x1905TLVFORGE031 "x1905TLVFORGE031 - Forge 1905 profile version TLV (x1905_tlv_structure_037)"
    result += _check(x1905TLVFORGE031, (INT8U *)&x1905_tlv_structure_037, x1905_tlv_stream_037, x1905_tlv_stream_len_037);

    #define x1905TLVFORGE032 "x1905TLVFORGE032 - Forge interface power change information TLV (x1905_tlv_structure_038)"
    result += _check(x1905TLVFORGE032, (INT8U *)&x1905_tlv_structure_038, x1905_tlv_stream_038, x1905_tlv_stream_len_038);

    #define x1905TLVFORGE033 "x1905TLVFORGE033 - Forge interface power change status TLV (x1905_tlv_structure_039)"
    result += _check(x1905TLVFORGE033, (INT8U *)&x1905_tlv_structure_039, x1905_tlv_stream_039, x1905_tlv_stream_len_039);

    #define x1905TLVFORGE034 "x1905TLVFORGE034 - Forge L2 neighbor device TLV (x1905_tlv_structure_040)"
    result += _check(x1905TLVFORGE034, (INT8U *)&x1905_tlv_structure_040, x1905_tlv_stream_040, x1905_tlv_stream_len_040);

    #define x1905TLVFORGE035 "x1905TLVFORGE035 - Forge vendor specific TLV (x1905_tlv_structure_041)"
    result += _check(x1905TLVFORGE035, (INT8U *)&x1905_tlv_structure_041, x1905_tlv_stream_041, x1905_tlv_stream_len_041);

    #define x1905TLVFORGE036 "x1905TLVFORGE036 - Forge supported service TLV (x1905_tlv_structure_050)"
    result += _check(x1905TLVFORGE036, (INT8U *)&x1905_tlv_structure_050, x1905_tlv_stream_050, x1905_tlv_stream_len_050);


    // Return the number of test cases that failed
    //
    return result;
}






