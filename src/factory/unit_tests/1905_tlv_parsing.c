/*
 *  prplMesh Wi-Fi Multi-AP
 *
 *  Copyright (c) 2017, Broadband Forum
 *  Copyright (c) 2018, prpl Foundation
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

static uint8_t _check(const char *test_description, const uint8_t *input, struct tlv *expected_output)
{
    uint8_t  result;
    struct tlv *real_output;

    real_output = parse_1905_TLV_from_packet(input);

    if (real_output == NULL)
    {
        result = 1;
        PLATFORM_PRINTF("Parse %-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  Parse failure\n");
    }
    else if (0 == compare_1905_TLV_structures(real_output, expected_output))
    {
        result = 0;
        PLATFORM_PRINTF("Parse %-100s: OK\n", test_description);
    }
    else
    {
        result = 1;
        PLATFORM_PRINTF("Parse %-100s: KO !!!\n", test_description);
        PLATFORM_PRINTF("  Expected output:\n");
        visit_1905_TLV_structure(expected_output, print_callback, PLATFORM_PRINTF, "");
        PLATFORM_PRINTF("  Real output    :\n");
        visit_1905_TLV_structure(real_output, print_callback, PLATFORM_PRINTF, "");
    }
    free_1905_TLV_structure(real_output);

    return result;
}

int main(void)
{
    int result = 0;
    struct x1905_tlv_test_vector *t;
    hlist_head test_vectors;

    hlist_head_init(&test_vectors);
    get_1905_tlv_test_vectors(&test_vectors);

    hlist_for_each(t, test_vectors, struct x1905_tlv_test_vector, h)
    {
        if (t->parse)
            result += _check(t->description, t->stream, container_of(t->h.children[0].next, struct tlv, s.h.l));
    }
    // @todo currently the test vectors still point to statically allocated TLVs
    // hlist_delete(&test_vectors);

    // Return the number of test cases that failed
    //
    return result;
}
