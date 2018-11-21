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
// This file tests the "forge_1905_CMDU_from_structure()" function by providing
// some test input structures and checking the generated output stream.
//

#include "platform.h"
#include "1905_cmdus.h"
#include "1905_tlvs.h"
#include "1905_cmdu_test_vectors.h"


INT8U _check(const char *test_description, struct CMDU *input, INT8U **expected_output, INT16U *expected_output_lens)
{
    INT8U   result;
    INT8U **real_output;
    INT16U *real_output_lens;

    INT8U expected_elements_nr;
    INT8U real_elements_nr;

    INT8U i;

    if (NULL == expected_output || NULL == expected_output_lens)
    {
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  Invalid arguments!\n");

        return 1;
    }

    // Check that "expected_output" and "expected_output_lens" have the same
    // number of elements
    //
    i = 0;
    while (NULL != expected_output[i])
    {
        if (0 == expected_output_lens[i])
        {
             PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
             PLATFORM_PRINTF("  Invalid arguments: the number of expected streams is LARGER than the lenght of the array containing their lengths\n");

             return 1;
        }
        i++;
    }
    if (0 != expected_output_lens[i])
    {
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  Invalid arguments: the number of expected output streams is SMALLER than the lenght of the array containing their lengths\n");

        return 1;
    }

    // This is the number of elements in both "real_output" and
    // "real_output_lens"
    //
    expected_elements_nr = i;

    // Call the actual function under test
    //
    real_output = forge_1905_CMDU_from_structure(input, &real_output_lens);

    // Check that "real_output" and "real_output_lens" have the same number of
    // elements
    //
    if (NULL == real_output || NULL == real_output_lens)
    {
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  forge_1905_CMDU_from_structure() returned a NULL pointer\n");

        return 1;
    }

    // Check that the number of returned streams matches the length of the
    // returned "real_output_lens" array
    //
    i = 0;
    while (NULL != real_output[i])
    {
        if (0 == real_output_lens[i])
        {
             PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
             PLATFORM_PRINTF("  The number of output streams is LARGER than the lenght of the array containing their lengths\n");

             return 1;
        }
        i++;
    }
    if (0 != real_output_lens[i])
    {
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  The number of output streams is SMALLER than the lenght of the array containing their lengths\n");

        return 1;
    }

    // This is the number of elements in both "real_output" and
    // "real_output_lens"
    //
    real_elements_nr = i;

    // From this point on, if something fails, instead of just returning we
    // will print the contents of both the expected and the forged streams
    //
    result = 0;

    // Check that "expected" and "real" streams have the same length
    //
    if (real_elements_nr != expected_elements_nr)
    {
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  The number of expected streams (%d) does not match the number of forged streams (%d)\n", expected_elements_nr, real_elements_nr);

        result = 1;
    }

    // Next, compare the contents of each stream
    //
    if (0 == result)
    {
        for (i=0; i<real_elements_nr; i++)
        {
            if (
                 (real_output_lens[i] != expected_output_lens[i])                            ||
                 (0 != PLATFORM_MEMCMP(expected_output[i], real_output[i], expected_output_lens[i]))
               )
            {
                result = 1;
                break;
            }
        }
    }

    // Time to print the results
    //
    if (0 == result)
    {
        result = 0;
        PLATFORM_PRINTF("%-100s: OK\n", test_description);
    }
    else
    {
        PLATFORM_PRINTF("%-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  Expected output:\n");
        for (i=0; i<expected_elements_nr; i++)
        {
            INT8U j;

            PLATFORM_PRINTF("    STREAM #%d: ", i);
            for (j=0; j<expected_output_lens[i]; j++)
            {
                PLATFORM_PRINTF("%02x ",*(expected_output[i]+j));
            }
        }
        PLATFORM_PRINTF("\n");
        PLATFORM_PRINTF("  Real output:\n");
        for (i=0; i<real_elements_nr; i++)
        {
            INT8U j;

            PLATFORM_PRINTF("    STREAM #%d: ", i);
            for (j=0; j<real_output_lens[i]; j++)
            {
                PLATFORM_PRINTF("%02x ",*(real_output[i]+j));
            }
        }
        PLATFORM_PRINTF("\n");
    }

    return result;
}


int main(void)
{
    INT8U result = 0;

    #define x1905CMDUFORGE001 "x1905CMDUFORGE001 - Forge link metric query CMDU (x1905_cmdu_001)"
    result += _check(x1905CMDUFORGE001, &x1905_cmdu_structure_001, x1905_cmdu_streams_001, x1905_cmdu_streams_len_001);

    #define x1905CMDUFORGE002 "x1905CMDUFORGE002 - Forge link metric query CMDU (x1905_cmdu_002)"
    result += _check(x1905CMDUFORGE002, &x1905_cmdu_structure_002, x1905_cmdu_streams_002, x1905_cmdu_streams_len_002);

    #define x1905CMDUFORGE003 "x1905CMDUFORGE003 - Forge link metric query CMDU (x1905_cmdu_003)"
    result += _check(x1905CMDUFORGE003, &x1905_cmdu_structure_003, x1905_cmdu_streams_003, x1905_cmdu_streams_len_003);

    #define x1905CMDUFORGE004 "x1905CMDUFORGE004 - Forge topology query CMDU (x1905_cmdu_005)"
    result += _check(x1905CMDUFORGE004, &x1905_cmdu_structure_005, x1905_cmdu_streams_005, x1905_cmdu_streams_len_005);

    // Return the number of test cases that failed
    //
    return result;
}


