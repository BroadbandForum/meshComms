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
#define MCAST_1905_B0 (0x01)
#define MCAST_1905_B1 (0x80)
#define MCAST_1905_B2 (0xC2)
#define MCAST_1905_B3 (0x00)
#define MCAST_1905_B4 (0x00)
#define MCAST_1905_B5 (0x13)
#define MCAST_1905  {MCAST_1905_B0, MCAST_1905_B1, MCAST_1905_B2, MCAST_1905_B3, MCAST_1905_B4, MCAST_1905_B5}


// LLDP nearest bridge multicast address ("01:80:C2:00:00:0E")
//
#define MCAST_LLDP_B0  (0x01)
#define MCAST_LLDP_B1  (0x80)
#define MCAST_LLDP_B2  (0xC2)
#define MCAST_LLDP_B3  (0x00)
#define MCAST_LLDP_B4  (0x00)
#define MCAST_LLDP_B5  (0x0E)
#define MCAST_LLDP  {MCAST_LLDP_B0, MCAST_LLDP_B1, MCAST_LLDP_B2, MCAST_LLDP_B3, MCAST_LLDP_B4, MCAST_LLDP_B5}

#endif

