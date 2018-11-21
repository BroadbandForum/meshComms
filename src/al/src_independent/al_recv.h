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


