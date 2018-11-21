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

#ifndef _AL_EXTENSION_H_
#define _AL_EXTENSION_H_

#  include "1905_cmdus.h"
#  include "1905_tlvs.h"


////////////////////////////////////////////////////////////////////////////////
// Private definitions
////////////////////////////////////////////////////////////////////////////////


// Insert, process, free third-party extensions in a CMDU
typedef INT8U (*CMDU_EXTENSION_CBK)(struct CMDU *);

// Obtain third-party local node informatiom
typedef void  (*DM_OBTAIN_LOCAL_INFO_CBK)(struct vendorSpecificTLV ***extensions,
                                          INT8U                      *nr);

// Update obtained info in the datamodel
typedef void  (*DM_UPDATE_LOCAL_INFO_CBK)(struct vendorSpecificTLV  **extensions,
                                          INT8U                       nr,
                                          INT8U                      *al_mac_address);

// Dump third-party info
typedef void  (*DM_EXTENSION_CBK)(INT8U **memory_structure,
                                  INT8U   structure_nr,
                                  void  (*callback)(void (*write_function)(const char *fmt, ...), const char *prefix, INT8U size, const char *name, const char *fmt, void *p),
                                  void  (*write_function)(const char *fmt, ...),
                                  const char *prefix);


#define MAX_EXTENSION_NAME_LEN        (20)

#define IEEE1905_EXTENSION_TYPE_RECV  (0)
#define IEEE1905_EXTENSION_TYPE_SEND  (1)
#define IEEE1905_EXTENSION_TYPE_DUMP  (2)
#define IEEE1905_EXTENSION_TYPE_FREE  (3)
#define IEEE1905_EXTENSION_MAX        (3)


////////////////////////////////////////////////////////////////////////////////
// Public functions (CMDU Rx/Tx callback processing).
//
// These functions are meant to be used by the ieee1905 stack core
////////////////////////////////////////////////////////////////////////////////

// This function runs through all the registered 'process' callbacks.
// Each registered 'process' callback is responsible for processing its own
// non-standard TLVs.
//
// 'c' is the CMDU structure which contains a list of TLVs. This 'c' pointer
// will be passed as argument to all the registered 'process' callbacks.
//
// Return '0' if there was a problem, '1' otherwise.
//
INT8U process1905CmduExtensions(struct CMDU *c);

// This funtion runs through all the registered 'send' callbacks.
// Each registered 'send' callback is responsible for adding its own
// non-standaard TLVs in the CMDU, using the provided API
// (embedExtensionInVendorSpecificTLV and insertVendorSpecificTLV)
//
// 'c' is the CMDU structure which contains a list of TLVs.
//
// Each registered 'send' function may add extra TLVs at the end of the current
// list. The caller of this function is responsible for releasing the allocated
// resources in this function (see send1905RawPacket())
//
// Return '0' if there was a problem, '1' otherwise.
//
INT8U send1905CmduExtensions(struct CMDU *c);

// This funtion releases all the Vendor Specific TLVs found in the CMDU's TLV
// list, It is used to free the resources previously allocated by
// send1905CmduExtensions().
//
// Unlike with the process/sendxxx functions, there is no need to define a free
// callback function for each registered group (like BBF). The stack knows how
// to release the resources allocated by send1905CmduExtensions(), which all
// are Vendor Specific TLVs.
//
// 'c' is the CMDU structure which contains a list of TLVs.
//
// Return '0' if there was a problem, '1' otherwise.
//
INT8U free1905CmduExtensions(struct CMDU *c);


////////////////////////////////////////////////////////////////////////////////
// Public functions (data model callback processing).
//
// These functions are meant to be used by the ieee1905 stack core
////////////////////////////////////////////////////////////////////////////////

// The next functions are required to extend the ALME 'dnd' response with
// extended information.
//
// The ALME 'dnd' response runs through the whole datamodel generating a text
// report in an organized manner. When this happens, the datamodel is expected
// to be updated.
// However the datamodel section of the device itself is not updated
// periodically, but only when it is required. So before generating the ALME
// 'dnd' report the datamodel section of the device itself must be updated (see
// '_updateLocalDeviceData()').
//
// Our ieee1905 stack implementation follows the same flow for the standard and
// the non-standard local (current device) datamodel update:
// - First, obtain the local info modeled as an array of TLVs.
// - Then, update the datamodel with these TLVs
// - Finally, release no longer used resources (like the arrays holding a set
//   of TLVs).
//
// Note: extended info is always embedded inside Vendor Specific TLVs
//
// At this point, the whole datamodel is updated, so we can call
// 'dumpExtendedInfo()' which is responsible for running through the whole
// datamodel (device by device) generating a text report included as the
// response in the ALME 'dnd' message.
//
// In summary, these are the steps to generate the ALME 'dnd' response:
// - (prerequisite: update the local extended info in the datamodel)
//   1. call 'obtainExtendedLocalInfo()' to obtain an array of Vendor Specific
//      TLVs containing the extended (non-standard) local info.
//   2. call 'updateExtendedInfo()' to update the datamodel extensions section
//      with this array.
//   3. call 'freeExtendedLocalInfo()' to release the array holding the
//      previously allocated Vendor Specific TLVs by 'obtainExtendedLocalInfo()'
//
// - Finally, call 'dumpExtendedInfo()' once for each device to get the
//   non-standard report data
//
INT8U obtainExtendedLocalInfo(struct vendorSpecificTLV ***extensions, INT8U *nr);
INT8U updateExtendedInfo(struct vendorSpecificTLV  **extensions, INT8U  nr, INT8U *al_mac_address);
INT8U dumpExtendedInfo(INT8U **memory_structure,
                       INT8U   structure_nr,
                       void  (*callback)(void (*write_function)(const char *fmt, ...), const char *prefix, INT8U size, const char *name, const char *fmt, void *p),
                       void  (*write_function)(const char *fmt, ...),
                       const char *prefix);
void freeExtendedLocalInfo(struct vendorSpecificTLV ***extensions, INT8U *nr);


////////////////////////////////////////////////////////////////////////////////
// Public functions (to insert non-standard TLVs in a CMDU).
//
// These functions are meant to be used by files from the "extensions"
// subfolder **only**
////////////////////////////////////////////////////////////////////////////////

// This function embeds a non-standard TLV inside a standard Vendor Specific
// TLV.
//
// Each registered actor is responsible for forging its own non-standard TLVs.
// The stream resulting from the 'forge' operation will be pointed by the Vendor
// Specific 'm' field.
//
// 'memory_structure' is a pointer to the non-standard TLV
//
// 'forge' is a function used to convert the non-standard TLV to an stream
//
// 'oui' is the identifier which defines the registered actor. Each vendor must
// use its designated OUI when adding new custom TLVs.
//
// Return a pointer to the new allocated Vendor Specific TLV or NULL in case of
// error
//
struct vendorSpecificTLV *vendorSpecificTLVEmbedExtension(void *memory_structure, INT8U *forge(INT8U *memory_structure, INT16U *len), INT8U oui[3]);

// This function inserts a Vendor Specific TLV in the CMDU's TLV list
//
// 'memory_structure' is a pointer to the CMDU structure
//
// 'vendor_specific' is a pointer to a Vendor Specific TLV
//
// Return '0' if there was a problem, '1' otherwise.
//
INT8U vendorSpecificTLVInsertInCDMU(struct CMDU *memory_structure, struct vendorSpecificTLV *vendor_specific);

// This function duplicates a Vendor Specific TLV
//
// The datamodel is based on pointers to received standard TLVs.
// On the other hand, the stack core releases the whole TLV list from a CMDU
// once it is processed. So, how can the datamodel retain this info then?
// When one or more TLVs from a CMDU must be stored in the datamodel, the stack
// takes care of releasing (only) the others.
// But... the stack can ***not*** understand the non-standard TLVs (embedded
// inside a Vendor Specific TLV). This is why all Vendor Specific TLVs included
// in a CMDU will be released once they are processed.
//
// In this regard, it is the third-party developer responsibility to clone the
// original Vendor Specific TLV and update the datamodel extension section with
// it.
//
// 'tlv' is the original TLV
//
// Return a copy of the original TLV
//
struct vendorSpecificTLV *vendorSpecificTLVDuplicate(struct vendorSpecificTLV *tlv);

////////////////////////////////////////////////////////////////////////////////
// Public function (extensions entry point).
//
// This function is meant to be used by the ieee1905 stack core
////////////////////////////////////////////////////////////////////////////////

// This function registers all the extensions
//
// This is the only function which must be modified each time a new extension
// group is created (ex. BBF).
// If an already registered extension group (ex. BBF) is later extended (more
// non-standard TLVs), no action here is required (the group is already
// registered)
//
INT8U start1905ALExtensions(void);


////////////////////////////////////////////////////////////////////////////////
// Al entity extension user API
//
// These functions are meant to be used by the ieee1905 stack core
////////////////////////////////////////////////////////////////////////////////

// This function registers the callbacks required to extend the CMDU
// functinality
//
// 'name' is the name assigned by the extension group (ex. BBF).
//
// 'process' is a callback used to process non-standard TLVs inside the incoming
// CMDU
//
// 'send' is a callback used to insert non-standard TLVs in the outgoig CMDU
//
// Return '0' if there was a problem, '1' otherwise.
//
INT8U register1905CmduExtension(char *name,
                                CMDU_EXTENSION_CBK process,
                                CMDU_EXTENSION_CBK send);

// This function registers the callbacks required to extend the ALME 'dnd'
// report.
//
// 'name' is the name assigned by the extension group (ex. BBF).
//
// 'obtain' is a callback used to obtain local non-standard info (shaped like
// an array of Vendor Specific TLVs)
//
// 'update' is a callback used to update the datamodel with the obtained
// non-standard info
//
// 'dump' is a callback used to extend the datamodel report with non-standard
// info
//
// Return '0' if there was a problem, '1' otherwise.
//
INT8U register1905AlmeDumpExtension(char *name,
                                    DM_OBTAIN_LOCAL_INFO_CBK obtain,
                                    DM_UPDATE_LOCAL_INFO_CBK update,
                                    DM_EXTENSION_CBK         dump);



// This function registers all protocol extensions
//
// This function must be called from 'start1905AL()' before entering the
// read-process loop.
//
// Return '0' if there was a problem, '1' otherwise.
//
INT8U start1905ALExtensions(void);

#endif
