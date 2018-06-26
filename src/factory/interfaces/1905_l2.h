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

#ifndef _1905_L2_H_
#define _1905_L2_H_

// Ethernet types por 1905 and LLDP packets
//
#define ETHERTYPE_1905  (0x893a)
#define ETHERTYPE_LLDP  (0x88cc)


// 1905 multicast address ("01:80:C2:00:00:13")
//
#define MCAST_1905 "\x01\x80\xC2\x00\x00\x13"


// LLDP nearest bridge multicast address ("01:80:C2:00:00:0E")
//
#define MCAST_LLDP  "\x01\x80\xC2\x00\x00\x0E"

#endif

