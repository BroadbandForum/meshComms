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

#ifndef _PLATFORM_INTERFACES_PRIV_H_
#define _PLATFORM_INTERFACES_PRIV_H_

// This function must be called at the very beginning of your program to
// register new types of "special" interfaces.
//
// Special interfaces are those that, in the command line, contain one or more
// ":". Examples:
//
//   eth0:ghnspirit:00ab443a600f:bluemoon
//   eth1:simulated:eth1_params.txt
//
// 'interface_type' is the first token after the first ":" ("ghnspirit" or
// "simulated" in the examples above).
// It can be anything you want (as long as you are willing to provide handlers
// for each context to that particular type)
//
// 'sub_type' is one of the available contexts ("STUB_TYPE_*").
//
// 'f' is a pointer to a function with a specific prototype depending on the
// context:
//
//   - STUB_TYPE_GET_INFO          --> void (*f)(char *interface_name, char *extended_params, struct interfaceInfo *m)
//   - STUB_TYPE_GET_METRICS       --> void (*f)(char *interface_name, char *extended_params, struct linkMetrics   *m)
//   - STUB_TYPE_PUSH_BUTTON_START --> void (*f)(char *interface_name, char *extended_params)
//
// Once registered, the 'f' function will be called from the associated context
// and this is its expected behaviour:
//
//   - STUB_TYPE_GET_INFO:
//       It receives the 'interface_name' (ex: "eth0") and the "extended_params"
//       (everything after the first ":", ex: "ghnspirit:00ab443a600f:bluemoon")
//       as arguments, and with that information 'f' is expected to fill the
//       'm' structure with the appropiate information.
//
//   - STUB_TYPE_GET_METRICS:
//       Same as "STUB_TYPE_GET_INFO", but this time the structure has to be
//       filled with metrics information of the link connecting the local
//       interface and 'm->neighbor_interface_address'.
//       'm->local_interface_address' and 'm->neighbor_interface_address' are
//       the only two fields already filled when the handler is called. All the
//       others must be filled by the handler.
//
//    - STUB_TYPE_PUSH_BUTTON_START:
//        'f' is expected to start the "push button configuration process" on
//        the given local interface.
//
// Note that for each "interface_type" you must call this function STUB_TYPE_MAX
// times (one for each stub type) with (obviously) different handlers (one for
// each context)
//
// This function returns '1' if there was a problem, '0' otherwise.
//
#define STUB_TYPE_GET_INFO            (0)
#define STUB_TYPE_GET_METRICS         (1)
#define STUB_TYPE_PUSH_BUTTON_START   (2)
#define STUB_TYPE_MAX                 (2)
INT8U registerInterfaceStub(char *interface_type, INT8U stub_type, void *f);

// This function is used to initialize the "interfaces list database" from the
// arguments obtained from the command line.
//
// In other words: when the initialization function obtains the list of
// interfaces the user is interested in making visible to the 1905 AL entity,
// it should call this function (once per interface) *before* starting the AL
// entity.
//
// Note that 'long_interface_name' must include the "whole" interface name as
// given in the command line. Examples:
//
//   Regular interface: "eth0" 
//   Special interface: "eth0:simualted:eth1_params.txt"
//
// For "special interfaces" to work, you need to call "registerInterfaceStub()"
// before calling this function.
//
void addInterface(char *long_interface_name);

#endif

