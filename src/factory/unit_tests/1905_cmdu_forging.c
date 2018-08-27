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
// This file tests the "forge_1905_CMDU_from_structure()" function by providing
// some test input structures and checking the generated output stream.
//

#include "platform.h"
#include "1905_cmdus.h"
#include "1905_tlvs.h"
#include "1905_cmdu_test_vectors.h"

#include <string.h> // strcmp, memcmp, ...
#include <stdio.h>  // vsnprintf
#include <stdarg.h> // va_start etc.

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
                 (0 != memcmp(expected_output[i], real_output[i], expected_output_lens[i]))
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

static const char *x1905_cmdu_print_expected_001 =
    "->message_version: 0\n"
    "->message_type: 5\n"
    "->message_id: 7\n"
    "->relay_indicator: 0\n"
    "->TLV(linkMetricQuery)->destination: 0\n"
    "->TLV(linkMetricQuery)->specific_neighbor: 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \n"
    "->TLV(linkMetricQuery)->link_metrics_type: 2\n";

static char x1905_cmdu_print_real[4000];

static void check_print(const char *format, ...)
{
    va_list arglist;
    size_t offset = strlen(x1905_cmdu_print_real);

    va_start(arglist, format);
    vsnprintf(x1905_cmdu_print_real + offset, sizeof(x1905_cmdu_print_real) - offset - 1,
              format, arglist);
    va_end(arglist);
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

    x1905_cmdu_print_real[0] = '\0';
    visit_1905_CMDU_structure(&x1905_cmdu_structure_001, print_callback, check_print, "->");
    if (strcmp(x1905_cmdu_print_expected_001, x1905_cmdu_print_real) != 0)
    {
        PLATFORM_PRINTF("%-100s: KO !!!\n", "x1905CMDUPRINT001");
        PLATFORM_PRINTF("  Expected output:\n%s\n  Real output:\n%s\n", x1905_cmdu_print_expected_001, x1905_cmdu_print_real);
        result++;
    }
    else
    {
        PLATFORM_PRINTF("%-100s: OK\n", "x1905CMDUPRINT001");
    }

    // Return the number of test cases that failed
    //
    return result;
}


