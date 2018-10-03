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

#ifndef _LLDP_TLV_TEST_VECTORS_H_
#define _LLDP_TLV_TEST_VECTORS_H_

#include "1905_tlvs.h"

extern struct endOfLldppduTLV     lldp_tlv_structure_001;
extern uint8_t                      lldp_tlv_stream_001[];
extern uint16_t                     lldp_tlv_stream_len_001;

extern struct chassisIdTLV        lldp_tlv_structure_002;
extern uint8_t                      lldp_tlv_stream_002[];
extern uint16_t                     lldp_tlv_stream_len_002;

extern struct portIdTLV           lldp_tlv_structure_003;
extern uint8_t                      lldp_tlv_stream_003[];
extern uint16_t                     lldp_tlv_stream_len_003;

extern struct timeToLiveTypeTLV   lldp_tlv_structure_004;
extern uint8_t                      lldp_tlv_stream_004[];
extern uint16_t                     lldp_tlv_stream_len_004;

#endif

