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
// This file tests the "parse_1905_TLV_from_packet()" function by providing
// some test input streams and checking the generated output structure.
//

#include "platform.h"
#include "utils.h"

#include "1905_tlvs.h"
#include "1905_tlv_test_vectors.h"

INT8U _check(const char *test_description, INT8U *input, INT8U *expected_output)
{
    INT8U  result;
    INT8U *real_output;

    real_output = parse_1905_TLV_from_packet(input);

    if (0 == compare_1905_TLV_structures(real_output, expected_output))
    {
        result = 0;
        PLATFORM_PRINTF("%-100s: OK\n", test_description);
    }
    else
    {
        result = 1;
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  Expected output:\n");
        visit_1905_TLV_structure(expected_output, print_callback, PLATFORM_PRINTF, "");
        PLATFORM_PRINTF("  Real output    :\n");
        visit_1905_TLV_structure(real_output, print_callback, PLATFORM_PRINTF, "");
    }

    return result;
}


int main(void)
{
    INT8U result = 0;

    #define x1905TLVPARSE001 "x1905TLVPARSE001 - Parse link metric query TLV (x1905_tlv_stream_001)"
    result += _check(x1905TLVPARSE001, x1905_tlv_stream_001, (INT8U *)&x1905_tlv_structure_001);

    #define x1905TLVPARSE002 "x1905TLVPARSE002 - Parse link metric query TLV (x1905_tlv_stream_003)"
    result += _check(x1905TLVPARSE002, x1905_tlv_stream_003, (INT8U *)&x1905_tlv_structure_003);

    #define x1905TLVPARSE003 "x1905TLVPARSE003 - Parse transmitter link metric TLV (x1905_tlv_stream_004)"
    result += _check(x1905TLVPARSE003, x1905_tlv_stream_004, (INT8U *)&x1905_tlv_structure_004);

    #define x1905TLVPARSE004 "x1905TLVPARSE004 - Parse transmitter link metric TLV (x1905_tlv_stream_005)"
    result += _check(x1905TLVPARSE004, x1905_tlv_stream_005, (INT8U *)&x1905_tlv_structure_005);

    #define x1905TLVPARSE005 "x1905TLVPARSE005 - Parse receiver link metric TLV (x1905_tlv_stream_006)"
    result += _check(x1905TLVPARSE005, x1905_tlv_stream_006, (INT8U *)&x1905_tlv_structure_006);

    #define x1905TLVPARSE006 "x1905TLVPARSE006 - Parse receiver link metric TLV (x1905_tlv_stream_007)"
    result += _check(x1905TLVPARSE006, x1905_tlv_stream_007, (INT8U *)&x1905_tlv_structure_007);

    #define x1905TLVPARSE007 "x1905TLVPARSE007 - Parse AL MAC address type TLV (x1905_tlv_stream_008)"
    result += _check(x1905TLVPARSE007, x1905_tlv_stream_008, (INT8U *)&x1905_tlv_structure_008);

    #define x1905TLVPARSE008 "x1905TLVPARSE008 - Parse MAC address type TLV (x1905_tlv_stream_009)"
    result += _check(x1905TLVPARSE008, x1905_tlv_stream_009, (INT8U *)&x1905_tlv_structure_009);

    #define x1905TLVPARSE009 "x1905TLVPARSE009 - Parse device information type TLV (x1905_tlv_stream_010)"
    result += _check(x1905TLVPARSE009, x1905_tlv_stream_010, (INT8U *)&x1905_tlv_structure_010);

    #define x1905TLVPARSE010 "x1905TLVPARSE010 - Parse device bridging capability TLV (x1905_tlv_stream_011)"
    result += _check(x1905TLVPARSE010, x1905_tlv_stream_011, (INT8U *)&x1905_tlv_structure_011);

    #define x1905TLVPARSE011 "x1905TLVPARSE011 - Parse device bridging capability TLV (x1905_tlv_stream_012)"
    result += _check(x1905TLVPARSE011, x1905_tlv_stream_012, (INT8U *)&x1905_tlv_structure_012);

    #define x1905TLVPARSE012 "x1905TLVPARSE012 - Parse device bridging capability TLV (x1905_tlv_stream_013)"
    result += _check(x1905TLVPARSE012, x1905_tlv_stream_013, (INT8U *)&x1905_tlv_structure_013);

    #define x1905TLVPARSE013 "x1905TLVPARSE013 - Parse non 1905 neighbor device list TLV (x1905_tlv_stream_014)"
    result += _check(x1905TLVPARSE013, x1905_tlv_stream_014, (INT8U *)&x1905_tlv_structure_014);

    #define x1905TLVPARSE014 "x1905TLVPARSE014 - Parse non 1905 neighbor device list TLV (x1905_tlv_stream_015)"
    result += _check(x1905TLVPARSE014, x1905_tlv_stream_015, (INT8U *)&x1905_tlv_structure_015);

    #define x1905TLVPARSE015 "x1905TLVPARSE015 - Parse neighbor device list TLV (x1905_tlv_stream_016)"
    result += _check(x1905TLVPARSE015, x1905_tlv_stream_016, (INT8U *)&x1905_tlv_structure_016);

    #define x1905TLVPARSE016 "x1905TLVPARSE016 - Parse neighbor device list TLV (x1905_tlv_stream_017)"
    result += _check(x1905TLVPARSE016, x1905_tlv_stream_017, (INT8U *)&x1905_tlv_structure_017);

    #define x1905TLVPARSE017 "x1905TLVPARSE017 - Parse link metric result code TLV (x1905_tlv_stream_017)"
    result += _check(x1905TLVPARSE017, x1905_tlv_stream_018, (INT8U *)&x1905_tlv_structure_018);

    #define x1905TLVPARSE018 "x1905TLVPARSE018 - Parse link metric result code TLV (x1905_tlv_stream_019)"
    result += _check(x1905TLVPARSE018, x1905_tlv_stream_019, (INT8U *)&x1905_tlv_structure_019);

    #define x1905TLVPARSE019 "x1905TLVPARSE019 - Parse searched role TLV (x1905_tlv_stream_020)"
    result += _check(x1905TLVPARSE019, x1905_tlv_stream_020, (INT8U *)&x1905_tlv_structure_020);

    #define x1905TLVPARSE020 "x1905TLVPARSE020 - Parse searched role TLV (x1905_tlv_stream_021)"
    result += _check(x1905TLVPARSE020, x1905_tlv_stream_021, (INT8U *)&x1905_tlv_structure_021);

    #define x1905TLVPARSE021 "x1905TLVPARSE021 - Parse autoconfig freq band TLV (x1905_tlv_stream_022)"
    result += _check(x1905TLVPARSE021, x1905_tlv_stream_022, (INT8U *)&x1905_tlv_structure_022);

    #define x1905TLVPARSE022 "x1905TLVPARSE022 - Parse autoconfig freq band TLV (x1905_tlv_stream_023)"
    result += _check(x1905TLVPARSE022, x1905_tlv_stream_023, (INT8U *)&x1905_tlv_structure_023);

    #define x1905TLVPARSE023 "x1905TLVPARSE023 - Parse supported role TLV (x1905_tlv_stream_024)"
    result += _check(x1905TLVPARSE023, x1905_tlv_stream_024, (INT8U *)&x1905_tlv_structure_024);

    #define x1905TLVPARSE024 "x1905TLVPARSE024 - Parse supported role TLV (x1905_tlv_stream_025)"
    result += _check(x1905TLVPARSE024, x1905_tlv_stream_025, (INT8U *)&x1905_tlv_structure_025);

    #define x1905TLVPARSE025 "x1905TLVPARSE025 - Parse supported freq band TLV (x1905_tlv_stream_026)"
    result += _check(x1905TLVPARSE025, x1905_tlv_stream_026, (INT8U *)&x1905_tlv_structure_026);

    #define x1905TLVPARSE026 "x1905TLVPARSE026 - Parse supported freq band TLV (x1905_tlv_stream_027)"
    result += _check(x1905TLVPARSE026, x1905_tlv_stream_027, (INT8U *)&x1905_tlv_structure_027);

    #define x1905TLVPARSE027 "x1905TLVPARSE027 - Parse push button event notification TLV (x1905_tlv_stream_028)"
    result += _check(x1905TLVPARSE027, x1905_tlv_stream_028, (INT8U *)&x1905_tlv_structure_028);

    #define x1905TLVPARSE028 "x1905TLVPARSE028 - Parse power off interface TLV (x1905_tlv_stream_029)"
    result += _check(x1905TLVPARSE028, x1905_tlv_stream_029, (INT8U *)&x1905_tlv_structure_029);

    #define x1905TLVPARSE029 "x1905TLVPARSE029 - Parse power off interface TLV (x1905_tlv_stream_030)"
    result += _check(x1905TLVPARSE029, x1905_tlv_stream_030, (INT8U *)&x1905_tlv_structure_030);

    #define x1905TLVPARSE030 "x1905TLVPARSE030 - Parse generic PHY device information type TLV (x1905_tlv_stream_031)"
    result += _check(x1905TLVPARSE030, x1905_tlv_stream_031, (INT8U *)&x1905_tlv_structure_031);

    #define x1905TLVPARSE031 "x1905TLVPARSE031 - Parse push button generic PHY event notification TLV (x1905_tlv_stream_032)"
    result += _check(x1905TLVPARSE031, x1905_tlv_stream_032, (INT8U *)&x1905_tlv_structure_032);

    #define x1905TLVPARSE032 "x1905TLVPARSE032 - Parse device identification type TLV (x1905_tlv_stream_033)"
    result += _check(x1905TLVPARSE032, x1905_tlv_stream_033, (INT8U *)&x1905_tlv_structure_033);

    #define x1905TLVPARSE033 "x1905TLVPARSE033 - Parse control URL type TLV (x1905_tlv_stream_034)"
    result += _check(x1905TLVPARSE033, x1905_tlv_stream_034, (INT8U *)&x1905_tlv_structure_034);

    #define x1905TLVPARSE034 "x1905TLVPARSE034 - Parse IPv4 type TLV (x1905_tlv_stream_035)"
    result += _check(x1905TLVPARSE034, x1905_tlv_stream_035, (INT8U *)&x1905_tlv_structure_035);

    #define x1905TLVPARSE035 "x1905TLVPARSE035 - Parse IPv6 type TLV (x1905_tlv_stream_036)"
    result += _check(x1905TLVPARSE035, x1905_tlv_stream_036, (INT8U *)&x1905_tlv_structure_036);

    #define x1905TLVPARSE036 "x1905TLVPARSE036 - Parse 1905 profile version TLV (x1905_tlv_stream_037)"
    result += _check(x1905TLVPARSE036, x1905_tlv_stream_037, (INT8U *)&x1905_tlv_structure_037);

    #define x1905TLVPARSE037 "x1905TLVPARSE037 - Parse interface power change information TLV (x1905_tlv_stream_038)"
    result += _check(x1905TLVPARSE037, x1905_tlv_stream_038, (INT8U *)&x1905_tlv_structure_038);

    #define x1905TLVPARSE038 "x1905TLVPARSE038 - Parse interface power change status TLV (x1905_tlv_stream_039)"
    result += _check(x1905TLVPARSE038, x1905_tlv_stream_039, (INT8U *)&x1905_tlv_structure_039);

    #define x1905TLVPARSE039 "x1905TLVPARSE039 - Parse L2 neighbor device TLV (x1905_tlv_stream_040)"
    result += _check(x1905TLVPARSE039, x1905_tlv_stream_040, (INT8U *)&x1905_tlv_structure_040);

    #define x1905TLVPARSE040 "x1905TLVPARSE040 - Parse vendor specific TLV (x1905_tlv_stream_041)"
    result += _check(x1905TLVPARSE040, x1905_tlv_stream_041, (INT8U *)&x1905_tlv_structure_041);


    // Return the number of test cases that failed
    //
    return result;
}






