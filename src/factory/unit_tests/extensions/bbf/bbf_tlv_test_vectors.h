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

#ifndef _BBF_TLV_TEST_VECTORS_H_
#define _BBF_TLV_TEST_VECTORS_H_

#include "bbf_tlvs.h"

#define CHECK_TRUE     0 // Check a successful parse/forge operation
#define CHECK_FALSE    1 // Check a wrong parse operation (malformed frame)

extern struct linkMetricQueryTLV                       bbf_tlv_structure_001;
extern uint8_t                                           bbf_tlv_stream_001[];
extern uint16_t                                          bbf_tlv_stream_len_001;

extern struct linkMetricQueryTLV                       bbf_tlv_structure_002;
extern uint8_t                                           bbf_tlv_stream_002[];
extern uint8_t                                           bbf_tlv_stream_002b[];
extern uint16_t                                          bbf_tlv_stream_len_002;

extern struct linkMetricQueryTLV                       bbf_tlv_structure_003;
extern uint8_t                                           bbf_tlv_stream_003[];
extern uint16_t                                          bbf_tlv_stream_len_003;

extern struct linkMetricQueryTLV                       bbf_tlv_structure_004;
extern uint8_t                                           bbf_tlv_stream_004[];
extern uint8_t                                           bbf_tlv_stream_004b[];
extern uint16_t                                          bbf_tlv_stream_len_004;

extern struct linkMetricQueryTLV                       bbf_tlv_structure_005;
extern uint8_t                                           bbf_tlv_stream_005[];
extern uint16_t                                          bbf_tlv_stream_len_005;

extern struct linkMetricQueryTLV                       bbf_tlv_structure_006;
extern uint8_t                                           bbf_tlv_stream_006[];
extern uint8_t                                           bbf_tlv_stream_006b[];
extern uint16_t                                          bbf_tlv_stream_len_006;

extern struct linkMetricQueryTLV                       bbf_tlv_structure_007;
extern uint8_t                                           bbf_tlv_stream_007[];
extern uint16_t                                          bbf_tlv_stream_len_007;

#endif

