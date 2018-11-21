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
// This file tests the "parse_non_1905_TLV_from_packet()" function by providing
// some test input streams and checking the generated output structure.
//

#include "platform.h"
#include "utils.h"

#include "bbf_tlvs.h"
#include "bbf_tlv_test_vectors.h"

INT8U _check(const char *test_description, INT8U mode, INT8U *input, INT8U *expected_output)
{
    INT8U  result;
    INT8U *real_output;
    INT8U  comparison;
    
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

INT8U _checkTrue(const char *test_description, INT8U *input, INT8U *expected_output)
{
  return _check(test_description, CHECK_TRUE, input, expected_output);
}

INT8U _checkFalse(const char *test_description, INT8U *input, INT8U *expected_output)
{
  return _check(test_description, CHECK_FALSE, input, expected_output);
}


int main(void)
{
    INT8U result = 0;

    #define BBFTLVPARSE001 "BBFTLVPARSE001 - Parse non-1905 link metric query TLV (bbf_tlv_stream_001)"
    result += _checkTrue(BBFTLVPARSE001, bbf_tlv_stream_001, (INT8U *)&bbf_tlv_structure_001);

    #define BBFTLVFORGE002 "BBFTLVPARSE002 - Parse non-1905 link metric query TLV (bbf_tlv_stream_003)"
    result += _checkTrue(BBFTLVFORGE002, bbf_tlv_stream_003, (INT8U *)&bbf_tlv_structure_003);

    #define BBFTLVFORGE003 "BBFTLVPARSE003 - Parse non-1905 transmitter link metric TLV (bbf_tlv_stream_005)"
    result += _checkTrue(BBFTLVFORGE003, bbf_tlv_stream_005, (INT8U *)&bbf_tlv_structure_005);

    #define BBFTLVFORGE004 "BBFTLVPARSE004 - Parse non-1905 receiver link metric TLV (bbf_tlv_stream_007)"
    result += _checkTrue(BBFTLVFORGE004, bbf_tlv_stream_007, (INT8U *)&bbf_tlv_structure_007);

    #define BBFTLVFORGE005 "BBFTLVPARSE005 - Parse non-1905 link metric query TLV (bbf_tlv_stream_008)"
    result += _checkFalse(BBFTLVFORGE005, bbf_tlv_stream_002b, (INT8U *)&bbf_tlv_structure_002);

    #define BBFTLVFORGE006 "BBFTLVPARSE006 - Parse non-1905 transmitter link metric TLV (bbf_tlv_stream_009)"
    result += _checkFalse(BBFTLVFORGE006, bbf_tlv_stream_004b, (INT8U *)&bbf_tlv_structure_004);

    #define BBFTLVFORGE007 "BBFTLVPARSE007 - Parse non-1905 receiver link metric TLV (bbf_tlv_stream_010)"
    result += _checkFalse(BBFTLVFORGE007, bbf_tlv_stream_006b, (INT8U *)&bbf_tlv_structure_006);

    // Return the number of test cases that failed
    //
    return result;
}






