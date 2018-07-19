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

#ifndef _AL_RECV_H_
#define _AL_RECV_H_

#include "1905_cmdus.h"
#include "lldp_payload.h"

// This function does "whatever needs to be done" as a result of receiving a
// CMDU: for example, some CMDU trigger a response, others are used to update
// the topology data base, etc...
//
// This function does *not* deal with "discarding" or "forwarding" the packet
// (that should have already been taken care of before this function is called)
//
// 'c' is the just received CMDU structure.
//
// 'receiving_interface_addr' is the MAC address of the local interface where
// the CMDU packet was received
//
// 'src_addr' is the MAC address contained in the 'src' field of the ethernet
// frame which contained the just received CMDU payload.
//
// 'queue_id' is the ID of the queue where messages to the AL entity should be
// posted in case new actions are derived from the processing of the current
// message.
//
// Return values:
//   PROCESS_CMDU_KO:
//     There was problem processing the CMDU
//   PROCESS_CMDU_OK:
//     The CMDU was correctly processed. No further action required.
//   PROCESS_CMDU_OK_TRIGGER_AP_SEARCH:
//     The CMDU was correctly processed. The caller should now trigger an "AP
//     search" process if there is still an unconfigured AP local interface.
//
#define PROCESS_CMDU_KO                     (0)
#define PROCESS_CMDU_OK                     (1)
#define PROCESS_CMDU_OK_TRIGGER_AP_SEARCH   (2)
INT8U process1905Cmdu(struct CMDU *c, INT8U *receiving_interface_addr, INT8U *src_addr, INT8U queue_id);

// Call this function when receiving an LLPD "bridge discovery" message so that
// the topology database is properly updated.
//
INT8U processLlpdPayload(struct PAYLOAD *payload, INT8U *receiving_interface_addr);

// Call this function when receiving an ALME REQUEST message. It will take
// action depending on the actual contents of this message (ie. "shut down an
// interface", "add a new bridge configuration", etc...)
//
INT8U process1905Alme(INT8U *alme_tlv, INT8U alme_client_id);

#endif


