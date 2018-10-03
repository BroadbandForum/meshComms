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

#ifndef _PLATFORM_ALME_SERVER_PRIV_H_
#define _PLATFORM_ALME_SERVER_PRIV_H_

#include <platform.h>

// When the AL calls "PLATFORM_REGISTER_QUEUE_EVENT()" with 'event_type' set to
// "PLATFORM_QUEUE_EVENT_NEW_ALME_MESSAGE", a thread that runs the following
// function must be started.
//
// This will take care of receiving (in a platform-specific way) ALME messages
// and then forward them to the queue whose ID is contained in the "struct
// _almeServerThreadData" structure that is passed in the "void *p" argument.
//
struct almeServerThreadData
{
    uint8_t  queue_id;
};

void *almeServerThread(void *p);


// This function is used to set the port number where the ALME server will
// listen to, waiting for ALME requests.
// It must be called *before* starting the 'almeServerThread()' thread.
//
void almeServerPortSet(int port_number);

#endif

