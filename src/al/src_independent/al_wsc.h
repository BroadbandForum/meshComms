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

#ifndef _AL_WSC_H_
#define _AL_WSC_H_

#include "platform.h"

// One type of 1905 CMDUs embeds "M1" and "M2" messages from the "Wi-Fi simple
// configuration standard".
//
// Because building these "M1" and "M2" messages in completely independent from
// the 1905 standard, I have created this simple interface to isolate the
// process as much as possible.
//
// It works like this:
//
//   * ENROLLEE:
//
//     1. When a 1905 node has an unconfigured interface, it needs to send an
//        "M1" message. It does this by calling "wscBuildM1()", which takes the
//        name of the unconfigured interface and returns a data buffer that can
//        be directly embedded inside the WSC TLV/CMDU.
//        It also returns a "key" structure that will later be need to process
//        the response.
//
//     2. When the response ("M2") is received, the enrollee calls
//       "wscProcessM2()", which configures the interface and frees the "M1"
//       and "key" buffers previously allocated in "wscBuildM1()"
//
//   * REGISTRAR:
//
//     1. When a 1905 node recieves an "M1" message, it calls "wscBuildM2()",
//        which takes the contents of the "M1" message and returns a data buffer
//        that can be directly embedded inside the WSC response TLV/CMDU.
//
//     2. After sending the TLV/CMDU, the "M2" buffer must be freed with a call
//        to "wscFreeM2()"
//       
// Note that, in the enrolle, "M1" is automatically freed by "wscProcessM2()",
// while, in the registrar, "M2" needs to be freed with "wscFreeM2()".
//
// When receiving a WSC TLV, because its contents are opaque to the 1905 node,
// function "wscGetType()" can be used do distinguish between "M1" and "M2".
//
// With all of this said, this is how you would use this API in the 1905 code:
//
//   - When receiving a AP search response (ie. when we want to configure an
//     unconfigured AP interface and a registrar has been found):
//
//       INT8U  *m1;
//       INT16U  m1_size;
//       void   *key;
//
//       wscBuildM1("wlan0", &m1, &m1_size, &key);
//       <send TVL/CMDU containing "m1">
//       <save "m1"/"m1_size"/"key" somehow, associated to "wlan0">
//
//
//   - When receiving a WSC CMDU:
//
//       if (WSC_TYPE_M1 == wscGetType(m, m_size))
//       {
//         // Registrar
//
//         INT8U  *m2;
//         INT16U  m2_size;
//
//         wscBuildM2(m, m_size, &m2, &m2_size);
//         <send TLV/CMDU containing "m2">
//         wscFreeM2(m2, m2_size);
//       }
//       else
//       {
//         // Enrollee
//         
//         <retrieve "m1"/"m1_size"/"key", associated to "wlan0">
//         wscProcessM2(key, m1, m1_size, m, m_size);
//       }
//
// Note that the references to "M1" and "key" must be "saved" for later use,
// once the response is received.
// For convenience, "wscProcessM2()" also accepts "NULL" as the 'M1'
// arguments meaning "the last built M and its key"
// However, if you use this shortcut, make sure you never call "wscBuildM1()"
// more than once in a row (without calling "wscProcessM2()" in between), or
// else the first "M1" will be lost forever.
//
// By the way, all the next functions return "0" if there a problem an "1"
// otherwise (except for "wscGetType()", which returns the message type)

INT8U  wscBuildM1(char *interface_name, INT8U **m1, INT16U *m1_size, void **key);
INT8U  wscProcessM2(void *key, INT8U *m1, INT16U m1_size, INT8U *m2, INT16U m2_size);


INT8U wscBuildM2(INT8U *m1, INT16U m1_size, INT8U **m2, INT16U *m2_size);
INT8U wscFreeM2(INT8U *m, INT16U m_size);


#define WSC_TYPE_M1      (0x00)
#define WSC_TYPE_M2      (0x01)
#define WSC_TYPE_UNKNOWN (0xFF)

INT8U wscGetType(INT8U *m, INT16U m_size);


#endif
