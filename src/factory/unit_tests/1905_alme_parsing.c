/*
 *  Broadband Forum IEEE 1905.1/1a stack
 *  
 *  Copyright (c) 2017, Broadband Forum
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  
 *  Subject to the terms and conditions of this license, each copyright
 *  holder and contributor hereby grants to those receiving rights under
 *  this license a perpetual, worldwide, non-exclusive, no-charge,
 *  royalty-free, irrevocable (except for failure to satisfy the
 *  conditions of this license) patent license to make, have made, use,
 *  offer to sell, sell, import, and otherwise transfer this software,
 *  where such license applies only to those patent claims, already
 *  acquired or hereafter acquired, licensable by such copyright holder or
 *  contributor that are necessarily infringed by:
 *  
 *  (a) their Contribution(s) (the licensed copyrights of copyright holders
 *      and non-copyrightable additions of contributors, in source or binary
 *      form) alone; or
 *  
 *  (b) combination of their Contribution(s) with the work of authorship to
 *      which such Contribution(s) was added by such copyright holder or
 *      contributor, if, at the time the Contribution is added, such addition
 *      causes such combination to be necessarily infringed. The patent
 *      license shall not apply to any other combinations which include the
 *      Contribution.
 *  
 *  Except as expressly stated above, no rights or licenses from any
 *  copyright holder or contributor is granted under this license, whether
 *  expressly, by implication, estoppel or otherwise.
 *  
 *  DISCLAIMER
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 *  DAMAGE.
 */

//
// This file tests the "parse_1905_TLV_from_packet()" function by providing
// some test input streams and checking the generated output structure.
//

#include "platform.h"
#include "utils.h"

#include "1905_tlvs.h"
#include "1905_alme.h"
#include "1905_alme_test_vectors.h"

INT8U _check(const char *test_description, INT8U *input, INT8U *expected_output)
{
    INT8U  result;
    INT8U *real_output;
    
    real_output = parse_1905_ALME_from_packet(input);

    if (0 == compare_1905_ALME_structures(real_output, expected_output))
    {
        result = 0;
        PLATFORM_PRINTF("%-100s: OK\n", test_description);
    }
    else
    {
        result = 1; 
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  Expected output:\n");
        visit_1905_ALME_structure(expected_output, print_callback, PLATFORM_PRINTF, "");
        PLATFORM_PRINTF("  Real output    :\n");
        visit_1905_ALME_structure(real_output, print_callback, PLATFORM_PRINTF, "");
    }

    return result;
}


int main(void)
{
    INT8U result = 0;

    #define x1905ALMEPARSE001 "x1905ALMEPARSE001 - Parse ALME-GET-INTF-LIST.request (x1905_alme_structure_001)"
    result += _check(x1905ALMEPARSE001, x1905_alme_stream_001, (INT8U *)&x1905_alme_structure_001);

    #define x1905ALMEPARSE002 "x1905ALMEPARSE002 - Parse ALME-GET-INTF-LIST.response (x1905_alme_structure_002)"
    result += _check(x1905ALMEPARSE002, x1905_alme_stream_002, (INT8U *)&x1905_alme_structure_002);

    #define x1905ALMEPARSE003 "x1905ALMEPARSE003 - Parse ALME-GET-INTF-LIST.response (x1905_alme_structure_003)"
    result += _check(x1905ALMEPARSE003, x1905_alme_stream_003, (INT8U *)&x1905_alme_structure_003);

    #define x1905ALMEPARSE004 "x1905ALMEPARSE004 - Parse ALME-GET-INTF-LIST.response (x1905_alme_structure_004)"
    result += _check(x1905ALMEPARSE004, x1905_alme_stream_004, (INT8U *)&x1905_alme_structure_004);

    #define x1905ALMEPARSE005 "x1905ALMEPARSE005 - Parse ALME-SET-INTF-PWR-STATE.request (x1905_alme_structure_005)"
    result += _check(x1905ALMEPARSE005, x1905_alme_stream_005, (INT8U *)&x1905_alme_structure_005);

    #define x1905ALMEPARSE006 "x1905ALMEPARSE006 - Parse ALME-SET-INTF-PWR-STATE.request (x1905_alme_structure_006)"
    result += _check(x1905ALMEPARSE006, x1905_alme_stream_006, (INT8U *)&x1905_alme_structure_006);

    #define x1905ALMEPARSE007 "x1905ALMEPARSE007 - Parse ALME-SET-INTF-PWR-STATE.confirm (x1905_alme_structure_007)"
    result += _check(x1905ALMEPARSE007, x1905_alme_stream_007, (INT8U *)&x1905_alme_structure_007);

    #define x1905ALMEPARSE008 "x1905ALMEPARSE008 - Parse ALME-SET-INTF-PWR-STATE.confirm (x1905_alme_structure_008)"
    result += _check(x1905ALMEPARSE008, x1905_alme_stream_008, (INT8U *)&x1905_alme_structure_008);

    #define x1905ALMEPARSE009 "x1905ALMEPARSE009 - Parse ALME-GET-INTF-PWR-STATE.request (x1905_alme_structure_009)"
    result += _check(x1905ALMEPARSE009, x1905_alme_stream_009, (INT8U *)&x1905_alme_structure_009);

    #define x1905ALMEPARSE010 "x1905ALMEPARSE010 - Parse ALME-GET-INTF-PWR-STATE.response (x1905_alme_structure_010)"
    result += _check(x1905ALMEPARSE010, x1905_alme_stream_010, (INT8U *)&x1905_alme_structure_010);

    #define x1905ALMEPARSE011 "x1905ALMEPARSE011 - Parse ALME-SET-FWD-RULE.request (x1905_alme_structure_011)"
    result += _check(x1905ALMEPARSE011, x1905_alme_stream_011, (INT8U *)&x1905_alme_structure_011);

    #define x1905ALMEPARSE012 "x1905ALMEPARSE012 - Parse ALME-SET-FWD-RULE.request (x1905_alme_structure_012)"
    result += _check(x1905ALMEPARSE012, x1905_alme_stream_012, (INT8U *)&x1905_alme_structure_012);

    #define x1905ALMEPARSE013 "x1905ALMEPARSE013 - Parse ALME-SET-FWD-RULE.confirm (x1905_alme_structure_013)"
    result += _check(x1905ALMEPARSE013, x1905_alme_stream_013, (INT8U *)&x1905_alme_structure_013);

    #define x1905ALMEPARSE014 "x1905ALMEPARSE014 - Parse ALME-GET-FWD-RULES.request (x1905_alme_structure_014)"
    result += _check(x1905ALMEPARSE014, x1905_alme_stream_014, (INT8U *)&x1905_alme_structure_014);

    #define x1905ALMEPARSE015 "x1905ALMEPARSE015 - Parse ALME-GET-FWD-RULES.response (x1905_alme_structure_015)"
    result += _check(x1905ALMEPARSE015, x1905_alme_stream_015, (INT8U *)&x1905_alme_structure_015);

    #define x1905ALMEPARSE016 "x1905ALMEPARSE016 - Parse ALME-GET-FWD-RULES.response (x1905_alme_structure_016)"
    result += _check(x1905ALMEPARSE016, x1905_alme_stream_016, (INT8U *)&x1905_alme_structure_016);

    #define x1905ALMEPARSE017 "x1905ALMEPARSE017 - Parse ALME-GET-FWD-RULES.response (x1905_alme_structure_017)"
    result += _check(x1905ALMEPARSE017, x1905_alme_stream_017, (INT8U *)&x1905_alme_structure_017);

    #define x1905ALMEPARSE018 "x1905ALMEPARSE018 - Parse ALME-MODIFY-FWD-RULE.request (x1905_alme_structure_018)"
    result += _check(x1905ALMEPARSE018, x1905_alme_stream_018, (INT8U *)&x1905_alme_structure_018);

    #define x1905ALMEPARSE019 "x1905ALMEPARSE019 - Parse ALME-MODIFY-FWD-RULE.confirm (x1905_alme_structure_019)"
    result += _check(x1905ALMEPARSE019, x1905_alme_stream_019, (INT8U *)&x1905_alme_structure_019);

    #define x1905ALMEPARSE020 "x1905ALMEPARSE020 - Parse ALME-MODIFY-FWD-RULE.confirm (x1905_alme_structure_020)"
    result += _check(x1905ALMEPARSE020, x1905_alme_stream_020, (INT8U *)&x1905_alme_structure_020);

    #define x1905ALMEPARSE021 "x1905ALMEPARSE021 - Parse ALME-REMOVE-FWD-RULE.request (x1905_alme_structure_021)"
    result += _check(x1905ALMEPARSE021, x1905_alme_stream_021, (INT8U *)&x1905_alme_structure_021);

    #define x1905ALMEPARSE022 "x1905ALMEPARSE022 - Parse ALME-REMOVE-FWD-RULE.confirm (x1905_alme_structure_022)"
    result += _check(x1905ALMEPARSE022, x1905_alme_stream_022, (INT8U *)&x1905_alme_structure_022);

    #define x1905ALMEPARSE023 "x1905ALMEPARSE023 - Parse ALME-GET-METRIC.request (x1905_alme_structure_023)"
    result += _check(x1905ALMEPARSE023, x1905_alme_stream_023, (INT8U *)&x1905_alme_structure_023);

    #define x1905ALMEPARSE024 "x1905ALMEPARSE024 - Parse ALME-GET-METRIC.response (x1905_alme_structure_024)"
    result += _check(x1905ALMEPARSE024, x1905_alme_stream_024, (INT8U *)&x1905_alme_structure_024);

    #define x1905ALMEPARSE025 "x1905ALMEPARSE025 - Parse ALME-GET-METRIC.response (x1905_alme_structure_025)"
    result += _check(x1905ALMEPARSE025, x1905_alme_stream_025, (INT8U *)&x1905_alme_structure_025);


    // Return the number of test cases that failed
    //
    return result;
}






