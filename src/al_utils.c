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

#include "platform.h"
#include "platform_crypto.h"


////////////////////////////////////////////////////////////////////////////////
// Public functions (exported only to files in this same folder)
////////////////////////////////////////////////////////////////////////////////

uint16_t getNextMid(void)
{
    static uint16_t mid       = 0;
    static uint8_t first_time = 1;

    if (1 == first_time)
    {
        // Start with a random MID. The standard is not clear about this, but
        // I think a random number is better than simply choosing zero, to
        // avoid start up problems (ex: one node boots and after a short time
        // it is reset and starts making use of the same MIDs all over again,
        // which will probably be ignored by other nodes, thinking they have
        // already processed these messages in the past)
        //
        first_time = 0;
        PLATFORM_GET_RANDOM_BYTES((uint8_t*)&mid, sizeof(uint16_t));
    }
    else
    {
        mid++;
    }

    return mid;
}

