/*
 *  Broadband Forum IEEE 1905.1/1a stack
 *  
 *  Copyright (c) 2017, Broadband Forum
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  
 *  Subject to the terms and conditions of this license, each copyright
 *  holder and contributor hereby grants to those receiving rights under
 *  this license a perpetual, worldwide, non-exclusive, no-charge,
 *  royalty-free, irrevocable (except for failure to satisfy the
 *  conditions of this license) patent license to make, have made, use,
 *  offer to sell, sell, import, and otherwise transfer this software,
 *  where such license applies only to those patent claims, already
 *  acquired or hereafter acquired, licensable by such copyright holder or
 *  contributor that are necessarily infringed by:
 *  
 *  (a) their Contribution(s) (the licensed copyrights of copyright holders
 *      and non-copyrightable additions of contributors, in source or binary
 *      form) alone; or
 *  
 *  (b) combination of their Contribution(s) with the work of authorship to
 *      which such Contribution(s) was added by such copyright holder or
 *      contributor, if, at the time the Contribution is added, such addition
 *      causes such combination to be necessarily infringed. The patent
 *      license shall not apply to any other combinations which include the
 *      Contribution.
 *  
 *  Except as expressly stated above, no rights or licenses from any
 *  copyright holder or contributor is granted under this license, whether
 *  expressly, by implication, estoppel or otherwise.
 *  
 *  DISCLAIMER
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 *  DAMAGE.
 */

#ifndef _AL_H_
#define _AL_H_

#define AL_ERROR_OUT_OF_MEMORY      (1)
#define AL_ERROR_INVALID_ARGUMENTS  (2)
#define AL_ERROR_NO_INTERFACES      (3)
#define AL_ERROR_INTERFACE_ERROR    (4)
#define AL_ERROR_OS                 (5)
#define AL_ERROR_PROTOCOL_EXTENSION (6)

// This is the function that runs the 1905 Abstraction Layer (AL) state machine.
//
// In order to start the AL services this is what you have to do from your
// platform specific code:
//
//   1. Create a thread
//   2. Make that thread execute this function
//
// This function takes the following arguments:
//
//   - 'al_mac_address': a six bytes array containing the AL MAC address of the
//     local host.
//   - 'map_whole_network_flag': If set to '1', the AL entity will map the whole
//     network (instead of just its direct neighbors). This takes more memory
//     and generates much more packets on the network, however the TR069
//     datamodel of this particular node will contain all the information
//     defined in the standard.
//     I would personally *only* set this flag to '1' in *one* of the network
//     nodes.
//   - 'registrar_interface' is the name of the local interface (ex: "wlan0")
//     that will act as a registrar in the 1905 network.
//     Note that *only one registrar* should be present in the 1905 network,
//     thus this parameter should:
//       a) Be set to NULL in all those ALs that are not registrars.
//       b) Be set to the name of a local 802.11 interface (ex: "wlan0") in the
//          AL that will act as a registrar.
//
// It returns when:
//
//   - Something went terribly wrong (maybe at initiallization time or maybe
//     later, while doing its bussiness). It that case it returns an error code
//     which is always bigger than '0':
//
//       AL_ERROR_OUT_OF_MEMORY:
//         A call to "PLATFORM_MALLOC()" failed, meaning there is no more memory
//         available in the system.
//
//       AL_ERROR_INVALID_ARGUMENTS:
//         The provided 'al_mac_address' is not valid.
//
//       AL_ERROR_NO_INTERFACES:
//         A call to "PLATFORM_GET_LIST_OF_1905_INTERFACES()" returned an empty
//         list, meaning there is nothing for the 1905 AL entity to do.
//
//       AL_ERROR_INTERFACE_ERROR:
//         A call to "PLATFORM_GET_LIST_OF_1905_INTERFACES() returned an error
//         or some other interface related problem.
//
//       AL_ERROR_OS:
//         One of the OS-related PLATFORM_* functions returned an error (these
//         are functions use to create queues, start timers, etc...)
//
//       AL_ERROR_PROTOCOL_EXTENSION;
//         Error registering, at least, one protocol extension.
//
//   - The HLE requested the AL service to stop. In this case it will return '0'
//
INT8U start1905AL(INT8U *al_mac_address, INT8U map_whole_network_flag, char *registrar_interface);


#endif

