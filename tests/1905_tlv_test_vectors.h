/*
 *  prplMesh Wi-Fi Multi-AP
 *
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

#ifndef _1905_TLV_TEST_VECTORS_H_
#define _1905_TLV_TEST_VECTORS_H_

#include <hlist.h>
#include <stdint.h>

struct x1905_tlv_test_vector {
    hlist_item h;
    const uint8_t *stream;
    uint16_t stream_len;
    const char *description;
    bool parse;
    bool forge;
};

void get_1905_tlv_test_vectors(dlist_head *test_vectors);

#endif

