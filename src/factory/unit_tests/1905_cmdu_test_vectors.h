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

#ifndef _1905_CMDU_TEST_VECTORS_H_
#define _1905_CMDU_TEST_VECTORS_H_

#include "1905_cmdus.h"
#include "1905_tlvs.h"

extern struct CMDU   x1905_cmdu_structure_001;
extern INT8U        *x1905_cmdu_streams_001[];
extern INT16U        x1905_cmdu_streams_len_001[];

extern struct CMDU   x1905_cmdu_structure_002;
extern INT8U        *x1905_cmdu_streams_002[];
extern INT16U        x1905_cmdu_streams_len_002[];

extern struct CMDU   x1905_cmdu_structure_003;
extern INT8U        *x1905_cmdu_streams_003[];
extern INT16U        x1905_cmdu_streams_len_003[];

extern struct CMDU   x1905_cmdu_structure_004;
extern INT8U        *x1905_cmdu_streams_004[];
extern INT16U        x1905_cmdu_streams_len_004[];

extern struct CMDU   x1905_cmdu_structure_005;
extern INT8U        *x1905_cmdu_streams_005[];
extern INT16U        x1905_cmdu_streams_len_005[];

/** @defgroup tv_cmdu_header CMDU header parsing test vectors
 */

/** @defgroup tv_cmdu_header_001 CMDU header with last fragment indicator
 *
 * @ingroup tv_cmdu_header
 * @{
 */
extern struct CMDU_header x1905_cmdu_header_001;
extern uint8_t            x1905_cmdu_packet_001[];
extern size_t             x1905_cmdu_packet_len_001;
/** @} */

/** @defgroup tv_cmdu_header_002 CMDU header without last fragment indicator
 *
 * @ingroup tv_cmdu_header
 * @{
 */
extern struct CMDU_header x1905_cmdu_header_002;
extern uint8_t            x1905_cmdu_packet_002[];
extern size_t             x1905_cmdu_packet_len_002;
/** @} */

/** @defgroup tv_cmdu_header_003 CMDU header with wrong ether type
 *
 * @ingroup tv_cmdu_header
 * @{
 */
extern uint8_t            x1905_cmdu_packet_003[];
extern size_t             x1905_cmdu_packet_len_003;
/** @} */

/** @defgroup tv_cmdu_header_004 CMDU header is too short
 *
 * @ingroup tv_cmdu_header
 * @{
 */
extern uint8_t            x1905_cmdu_packet_004[];
extern size_t             x1905_cmdu_packet_len_004;
/** @} */



#endif
