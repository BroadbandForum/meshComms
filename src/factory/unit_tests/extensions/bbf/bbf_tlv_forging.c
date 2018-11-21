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
// This file tests the "forge_non_1905_TLV_from_structure()" function by
// providing some test input structures and checking the generated output
// stream.
//

#include "platform.h"
#include "bbf_tlvs.h"
#include "bbf_tlv_test_vectors.h"

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
        if ((expected_output_len == real_output_len) && (0 == PLATFORM_MEMCMP(expected_output, real_output, real_output_len)))
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

