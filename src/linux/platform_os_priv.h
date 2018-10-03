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

#ifndef _PLATFORM_OS_PRIV_H_
#define _PLATFORM_OS_PRIV_H_

#include <platform.h>

// Send a message to the AL queue whose id is 'queue_id'
//
// Return "0" if there was a problem, "1" otherwise
//
uint8_t sendMessageToAlQueue(uint8_t queue_id, uint8_t *message, uint16_t message_len);

#endif


