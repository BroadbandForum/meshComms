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

#ifndef _1905_ALME_TEST_VECTORS_H_
#define _1905_ALME_TEST_VECTORS_H_

#include "1905_alme.h"

extern struct getIntfListRequestALME         x1905_alme_structure_001;
extern INT8U                                 x1905_alme_stream_001[];
extern INT16U                                x1905_alme_stream_len_001;

extern struct getIntfListResponseALME        x1905_alme_structure_002;
extern INT8U                                 x1905_alme_stream_002[];
extern INT16U                                x1905_alme_stream_len_002;

extern struct getIntfListResponseALME        x1905_alme_structure_003;
extern INT8U                                 x1905_alme_stream_003[];
extern INT16U                                x1905_alme_stream_len_003;

extern struct getIntfListResponseALME        x1905_alme_structure_004;
extern INT8U                                 x1905_alme_stream_004[];
extern INT16U                                x1905_alme_stream_len_004;

extern struct setIntfPwrStateRequestALME     x1905_alme_structure_005;
extern INT8U                                 x1905_alme_stream_005[];
extern INT16U                                x1905_alme_stream_len_005;

extern struct setIntfPwrStateRequestALME     x1905_alme_structure_006;
extern INT8U                                 x1905_alme_stream_006[];
extern INT16U                                x1905_alme_stream_len_006;

extern struct setIntfPwrStateConfirmALME     x1905_alme_structure_007;
extern INT8U                                 x1905_alme_stream_007[];
extern INT16U                                x1905_alme_stream_len_007;

extern struct setIntfPwrStateConfirmALME     x1905_alme_structure_008;
extern INT8U                                 x1905_alme_stream_008[];
extern INT16U                                x1905_alme_stream_len_008;

extern struct getIntfPwrStateRequestALME     x1905_alme_structure_009;
extern INT8U                                 x1905_alme_stream_009[];
extern INT16U                                x1905_alme_stream_len_009;

extern struct getIntfPwrStateResponseALME    x1905_alme_structure_010;
extern INT8U                                 x1905_alme_stream_010[];
extern INT16U                                x1905_alme_stream_len_010;

extern struct setFwdRuleRequestALME          x1905_alme_structure_011;
extern INT8U                                 x1905_alme_stream_011[];
extern INT16U                                x1905_alme_stream_len_011;

extern struct setFwdRuleRequestALME          x1905_alme_structure_012;
extern INT8U                                 x1905_alme_stream_012[];
extern INT16U                                x1905_alme_stream_len_012;

extern struct setFwdRuleConfirmALME          x1905_alme_structure_013;
extern INT8U                                 x1905_alme_stream_013[];
extern INT16U                                x1905_alme_stream_len_013;

extern struct getFwdRulesRequestALME         x1905_alme_structure_014;
extern INT8U                                 x1905_alme_stream_014[];
extern INT16U                                x1905_alme_stream_len_014;

extern struct getFwdRulesResponseALME        x1905_alme_structure_015;
extern INT8U                                 x1905_alme_stream_015[];
extern INT16U                                x1905_alme_stream_len_015;

extern struct getFwdRulesResponseALME        x1905_alme_structure_016;
extern INT8U                                 x1905_alme_stream_016[];
extern INT16U                                x1905_alme_stream_len_016;

extern struct getFwdRulesResponseALME        x1905_alme_structure_017;
extern INT8U                                 x1905_alme_stream_017[];
extern INT16U                                x1905_alme_stream_len_017;

extern struct modifyFwdRuleRequestALME       x1905_alme_structure_018;
extern INT8U                                 x1905_alme_stream_018[];
extern INT16U                                x1905_alme_stream_len_018;

extern struct modifyFwdRuleConfirmALME       x1905_alme_structure_019;
extern INT8U                                 x1905_alme_stream_019[];
extern INT16U                                x1905_alme_stream_len_019;

extern struct modifyFwdRuleConfirmALME       x1905_alme_structure_020;
extern INT8U                                 x1905_alme_stream_020[];
extern INT16U                                x1905_alme_stream_len_020;

extern struct removeFwdRuleRequestALME       x1905_alme_structure_021;
extern INT8U                                 x1905_alme_stream_021[];
extern INT16U                                x1905_alme_stream_len_021;

extern struct removeFwdRuleConfirmALME       x1905_alme_structure_022;
extern INT8U                                 x1905_alme_stream_022[];
extern INT16U                                x1905_alme_stream_len_022;

extern struct getMetricRequestALME           x1905_alme_structure_023;
extern INT8U                                 x1905_alme_stream_023[];
extern INT16U                                x1905_alme_stream_len_023;

extern struct getMetricResponseALME          x1905_alme_structure_024;
extern INT8U                                 x1905_alme_stream_024[];
extern INT16U                                x1905_alme_stream_len_024;

extern struct getMetricResponseALME          x1905_alme_structure_025;
extern INT8U                                 x1905_alme_stream_025[];
extern INT16U                                x1905_alme_stream_len_025;

#endif

