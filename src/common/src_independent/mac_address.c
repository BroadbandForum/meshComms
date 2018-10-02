/*
 *  prplMesh Wi-Fi Multi-AP
 *
 *  Copyright (c) 2018, prpl Foundation
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

#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "hlist.h"

mac_address * asciiToMac(const char *str, mac_address *addr)
{
    int i = 0;

    if ( ! str )
        return memset(addr, 0, sizeof(mac_address));

    while ( 0x00 != *str && i < 6 ) {
        uint8_t byte = 0;
        while ( 0x00 != *str && ':' != *str ) {
            char low;
            byte <<= 4;
            low    = tolower(*str);

            if ( low >= 'a' )   byte |= low - 'a' + 10;
            else                byte |= low - '0';

            str++;
        }
        (*addr)[i] = byte;
        i++;
        if ( ! *str )
            break;
        str++;
    }
    return addr;
}
