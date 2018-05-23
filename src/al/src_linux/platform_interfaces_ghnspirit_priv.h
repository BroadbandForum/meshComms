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

#ifndef _PLATFORM_INTERFACES_GHNSPIRIT_H_
#define _PLATFORM_INTERFACES_GHNSPIRIT_H_

#include "platform_interfaces.h" // struct interfaceInfo


// Call this function at the very beginning of your program so that interfaces
// of type "ghnspirit" can be processed with the corresponding callbacks in the
// future.
//
void registerGhnSpiritInterfaceType(void);

#endif
