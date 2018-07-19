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

#ifndef _BBF_RECV_H_
#define _BBF_RECV_H_

#include "1905_cmdus.h"


// Process BBF TLVs included in the incoming CMDU structure
//
// This function allows to parse the new TLVs defined in the BBF community.
// According to the standard, any CMDU is subject to be extended with extra
// Vendor Specific TLVs, so each inserted BBF TLV will be embedded inside a
// Vendor Specific TLV.
// This implementation will only process defined BBF TLVs embedded inside a
// Vendor Specific TLV whose OUI is the BBF one (0x00256d)
//
// 'memory_structure' is the CMDU structure
//
// Return '0' if there was a problem, '1' otherwise
//
INT8U CBKprocess1905BBFExtensions(struct CMDU *memory_structure);

#endif


