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

#include <string.h> // memcmp(), memcpy(), ...

static int _check(const char *test_description, const struct tlv *input, const uint8_t *expected_output,
               uint16_t expected_output_len)
{
    int      result;
    uint8_t *real_output;
    uint16_t real_output_len;

    real_output = forge_1905_TLV_from_structure(input, &real_output_len);

    if (NULL == real_output)
    {
        PLATFORM_PRINTF("Parse %-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  forge_1905_TLV_from_structure() returned a NULL pointer\n");

        return 1;
    }

    if ((expected_output_len == real_output_len) && (0 == memcmp(expected_output, real_output, real_output_len)))
    {
        result = 0;
        PLATFORM_PRINTF("Parse %-100s: OK\n", test_description);
    }
    else
    {
        uint16_t i;

        result = 1;
        PLATFORM_PRINTF("Parse %-100s: KO !!!\n", test_description);
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
    int result = 0;
    struct x1905_test_vector *t;
    hlist_head test_vectors;

    hlist_head_init(&test_vectors);
    get_1905_test_vectors(&test_vectors);

    hlist_for_each(t, test_vectors, struct x1905_test_vector, h)
    {
        if (t->forge)
            result += _check(t->description, container_of(t->h.children[0].next, struct tlv, h.l), t->stream, t->stream_len);
    }
    // @todo currently the test vectors still point to statically allocated TLVs
    // hlist_delete(&test_vectors);

    // Return the number of test cases that failed
    //
    return result;
}
