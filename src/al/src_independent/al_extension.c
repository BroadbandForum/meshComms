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
#include "1905_cmdus.h"
#include "1905_tlvs.h"
#include "al_extension.h"

#include <string.h> // memcmp(), memcpy(), ...

////////////////////////////////////////////////////////////////////////////////
// Private definitions and data
////////////////////////////////////////////////////////////////////////////////

// Structure for CMDU extensions management
//
struct _ieee1905CmduExtension
{
    INT8U  entries_nr;

    struct _cmduExtension
    {
        char name[MAX_EXTENSION_NAME_LEN];
        CMDU_EXTENSION_CBK process;
        CMDU_EXTENSION_CBK send;

    } *entries;

} ieee1905_cmdu_extension = {0, NULL};


// Structure for datamodel extensions management
//
struct _ieee1905DmExtension
{
    INT8U  entries_nr;

    struct _dmExtension
    {
        char name[MAX_EXTENSION_NAME_LEN];
        DM_OBTAIN_LOCAL_INFO_CBK obtain;
        DM_UPDATE_LOCAL_INFO_CBK update;
        DM_EXTENSION_CBK         dump;

    } *entries;

} ieee1905_dm_extension = {0, NULL};


////////////////////////////////////////////////////////////////////////////////
// Public functions (CMDU Rx/Tx callback processing).
////////////////////////////////////////////////////////////////////////////////

// - process1905CmduExtensions(): Run through all the registered entities to
//                                process the non-standard data embedded in the
//                                incoming CMDU
// - send1905CmduExtensions()   : Run through all the registered entities to
//                                extend the outgoing CMDU with the
//                                non-standard data
// - free1905CmduExtensions()   : Free no longer used resources allocated by
//                                send1905CmduExtensions().
//
INT8U process1905CmduExtensions(struct CMDU *c)
{
    INT32U                          i;
    struct _ieee1905CmduExtension  *t;

    if (NULL == c)
    {
        return 0;
    }

    t = &ieee1905_cmdu_extension;

    if (NULL != t->entries)
    {
        for (i=0; i<t->entries_nr; i++)
        {
            if (NULL != t->entries[i].process)
            {
                t->entries[i].process(c);
            }
        }
    }

    return 1;
}

INT8U send1905CmduExtensions(struct CMDU *c)
{
    INT32U                          i;
    struct _ieee1905CmduExtension  *t;

    if (NULL == c)
    {
        return 0;
    }

    t = &ieee1905_cmdu_extension;

    if (NULL != t->entries)
    {
        for (i=0; i<t->entries_nr; i++)
        {
            if (NULL != t->entries[i].send)
            {
                t->entries[i].send(c);
            }
        }
    }

    return 1;
}

// Free allocated resources in 'send1905CmduExtensions' which are a list of
// Vendor Specific TLVs. There is no need for a registered 'free' callback for
// each actor (a Vendor Specific TLV is always released the same way)
//
INT8U free1905CmduExtensions(struct CMDU *c)
{
    INT32U                      i;
    INT8U                      *p;
    struct vendorSpecificTLV   *vs_tlv;

    if ((NULL == c) || (NULL == c->list_of_TLVs))
    {
        return 0;
    }

    i = 0;
    while (NULL != (p = c->list_of_TLVs[i]))
    {
        // Protocol extension is always embedded inside a Vendor Specific
        // TLV. Ignore other TLVs
        //
        if (*p == TLV_TYPE_VENDOR_SPECIFIC)
        {
            vs_tlv = (struct vendorSpecificTLV *)p;

            if (vs_tlv->m)
            {
                PLATFORM_FREE(vs_tlv->m);
            }
            PLATFORM_FREE(vs_tlv);
        }
        i++;
    }

    return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Public functions (data model callback processing).
////////////////////////////////////////////////////////////////////////////////

// - obtainExtendedLocalInfo(): Run through all the registered entities to
//                              obtain the local non-standard data (embedded in
//                              Vendor Specific TLVs)
// - updateExtendedInfo()     : Run through all the registered entities to
//                              update the non-standard data in the datamodel.
// - dumpExtendedInfo()       : Run through all the registered entities to
//                              extend the 'dnd' ALME response (non-standard
//                              data from datamodel)
// - freeExtendedLocalInfo()  : Free no longer used resources allocated by
//                              obtainExtendedLocalInfo()
//
INT8U obtainExtendedLocalInfo(struct vendorSpecificTLV ***extensions, INT8U *nr)
{
    INT32U                        i;
    INT32U                        j;
    struct _ieee1905DmExtension  *t;

    struct vendorSpecificTLV **total_tlvs;      // The returned TLVs from all
                                                // the registered actors
    INT8U                      total_tlvs_nr;

    struct vendorSpecificTLV **tlvs;            // TLVs returned by some
                                                // registered actor
    INT8U                      tlvs_nr;

    if ((NULL == extensions) || (NULL == nr))
    {
        return 0;
    }

    // For each registered actor, obtain the extended local info data
    //
    total_tlvs_nr = 0;
    total_tlvs = NULL;
    tlvs_nr = 0;
    tlvs = NULL;

    t = &ieee1905_dm_extension;

    if (NULL != t->entries)
    {
        for (i=0; i<t->entries_nr; i++)
        {
            if (NULL != t->entries[i].obtain)
            {
                // Obtain particular extensions from this actor
                //
                t->entries[i].obtain(&tlvs, &tlvs_nr);

                // Keep all the obtained extensions stacked together
                //
                if (NULL == total_tlvs)
                {
                    total_tlvs = (struct vendorSpecificTLV **)PLATFORM_MALLOC(sizeof(struct vendorSpecificTLV*) * (total_tlvs_nr + tlvs_nr) );
                }
                else
                {
                    total_tlvs = (struct vendorSpecificTLV **)PLATFORM_REALLOC(total_tlvs, sizeof(struct vendorSpecificTLV*) * (total_tlvs_nr + tlvs_nr) );
                }

                for (j=0; j<tlvs_nr; j++)
                {
                    total_tlvs[total_tlvs_nr] = tlvs[j];
                    total_tlvs_nr++;
                }

                // Free no longer used resources
                //
                PLATFORM_FREE(tlvs);
                tlvs = NULL;
            }
        }
    }

    *extensions = total_tlvs;
    *nr = total_tlvs_nr;

    return 1;
}

// Free the contents of the pointers filled by a previous call to
// "obtainExtendedLocalInfo()".
// This function is called with the same three last arguments as
// "obtainExtendedLocalInfo()"
// There is no need for a registered 'free' callback for each actor (a Vendor
// Specific TLV is always released the same way)
//
void freeExtendedLocalInfo(struct vendorSpecificTLV ***extensions, INT8U *nr)
{
    INT8U  i;

    if ((NULL != extensions) && (NULL != *extensions) && (NULL != nr))
    {
        for (i=0; i<*nr; i++)
        {
            if (NULL != (*extensions)[i])
            {
                if ((*extensions)[i]->m)
                {
                    PLATFORM_FREE((*extensions)[i]->m);
                }
                PLATFORM_FREE((*extensions)[i]);
            }
        }
        PLATFORM_FREE(*extensions);
    }
}

INT8U updateExtendedInfo(struct vendorSpecificTLV **extensions, INT8U nr, INT8U *al_mac_address)
{
    INT32U                        i;
    struct _ieee1905DmExtension  *t;

    if ((NULL == extensions) || (NULL == al_mac_address))
    {
        return 0;
    }

    t = &ieee1905_dm_extension;

    if (NULL != t->entries)
    {
        for (i=0; i<t->entries_nr; i++)
        {
            if (NULL != t->entries[i].update)
            {
                t->entries[i].update(extensions, nr, al_mac_address);
            }
        }
    }

    return 1;
}


INT8U dumpExtendedInfo(INT8U **memory_structure,
                       INT8U   structure_nr,
                       visitor_callback callback,
                       void  (*write_function)(const char *fmt, ...),
                       const char *prefix)
{
    INT32U                        i;
    struct _ieee1905DmExtension  *t;

    if ((NULL == memory_structure) || (NULL == callback) || (NULL == write_function) || (NULL == prefix))
    {
        return 0;
    }

    t = &ieee1905_dm_extension;

    if (NULL != t->entries)
    {
        for (i=0; i<t->entries_nr; i++)
        {
            if (NULL != t->entries[i].dump)
            {
                t->entries[i].dump(memory_structure, structure_nr, callback, write_function, prefix);
            }
        }
    }

    return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Public functions (to insert non-standard TLVs in a CMDU).
////////////////////////////////////////////////////////////////////////////////

// - VendorSpecificTLVEmbedExtension():   Embed a non-standard TLV inside a
//                                        Vendor Specific TLV
//
// - VendorSpecificTLVInsertInCDMU():     Add a Vendor Specific TLV to the CMDU
//
// - VendorSpecificTLVDuplicate():        Clone a Vendor Specific TLV
//
struct vendorSpecificTLV *vendorSpecificTLVEmbedExtension(void *memory_structure, INT8U *forge(INT8U *memory_structure, INT16U *len), INT8U oui[3])
{
    struct vendorSpecificTLV   *vendor_specific;
    INT8U                      *stream;
    INT16U                      stream_len;

    if ((NULL == memory_structure) || (NULL == forge) || (NULL == oui))
    {
        return NULL;
    }

    stream = forge((INT8U *)memory_structure, &stream_len);
    if (NULL == stream)
    {
        // Could not forge the packet. Error?
        //
        PLATFORM_PRINTF_DEBUG_WARNING("forge extended TLV failed!\n");
        return NULL;
    }

    vendor_specific                = (struct vendorSpecificTLV *)PLATFORM_MALLOC(sizeof(struct vendorSpecificTLV));
    memcpy(vendor_specific->vendorOUI, oui, 3);
    vendor_specific->tlv_type      = TLV_TYPE_VENDOR_SPECIFIC;
    vendor_specific->m_nr          = stream_len;
    vendor_specific->m             = stream;

    return vendor_specific;
}

INT8U vendorSpecificTLVInsertInCDMU(struct CMDU *memory_structure, struct vendorSpecificTLV *vendor_specific)
{
    INT8U      tlv_stop;

    if ((NULL == memory_structure) || (NULL == memory_structure->list_of_TLVs) ||
        (NULL == vendor_specific)  || (*(INT8U *)vendor_specific != TLV_TYPE_VENDOR_SPECIFIC))
    {
        // Invalid params
        //
        return 0;
    }

    // Point at the end of the TLV list (NULL pointer)
    //
    tlv_stop = 0;
    while(memory_structure->list_of_TLVs[tlv_stop++]);
    tlv_stop--;

    // Insert TLV
    //
    memory_structure->list_of_TLVs             = (INT8U **)PLATFORM_REALLOC(memory_structure->list_of_TLVs, sizeof(INT8U *) * (tlv_stop+2));
    memory_structure->list_of_TLVs[tlv_stop++] = (INT8U *)vendor_specific;
    memory_structure->list_of_TLVs[tlv_stop]   = NULL;

    return 1;
}

struct vendorSpecificTLV *vendorSpecificTLVDuplicate(struct vendorSpecificTLV *tlv)
{
  struct vendorSpecificTLV *vs_tlv;

  if (NULL == tlv)
  {
      return NULL;
  }

  // Clone the Vendor Specific TLV
  //
  vs_tlv = (struct vendorSpecificTLV *)PLATFORM_MALLOC(sizeof(struct vendorSpecificTLV));
  vs_tlv->tlv_type = tlv->tlv_type;
  vs_tlv->vendorOUI[0] = tlv->vendorOUI[0];
  vs_tlv->vendorOUI[1] = tlv->vendorOUI[1];
  vs_tlv->vendorOUI[2] = tlv->vendorOUI[2];
  vs_tlv->m_nr = tlv->m_nr;

  vs_tlv->m = (INT8U *)PLATFORM_MALLOC(vs_tlv->m_nr);
  memcpy(vs_tlv->m, tlv->m, vs_tlv->m_nr);

  return vs_tlv;
}


////////////////////////////////////////////////////////////////////////////////
// Public functions (callbacks registration).
////////////////////////////////////////////////////////////////////////////////

// - register1905CmduExtension()    : Register callbacks to manage the CMDU
//                                    extensions
//
// - register1905AlmeDumpExtension(): Register callbacks to manage the ALME
//                                    'dnd' extended info response
//
INT8U register1905CmduExtension(char *name,
                                CMDU_EXTENSION_CBK process,
                                CMDU_EXTENSION_CBK send)
{
    INT32U                          i;
    struct _ieee1905CmduExtension  *t;

    if ((NULL == name) || (NULL == process) || (NULL == send))
    {
        return 0;
    }

    t = &ieee1905_cmdu_extension;

    // Check if this extension group is already registered
    //
    if (NULL != t->entries)
    {
        for (i=0; i<t->entries_nr; i++)
        {
            if ((NULL != t->entries[i].name) &&
                (0 == memcmp(t->entries[i].name, name, strlen(name) + 1)))
            {
                // Already exists!
                //
                PLATFORM_PRINTF_DEBUG_WARNING("[PLATFORM] A protocol extension with the name %s already exists. Ignoring...\n", name);
                return 0;
            }
        }
    }

    if (0 == t->entries_nr)
    {
        t->entries = (struct _cmduExtension *)PLATFORM_MALLOC(sizeof(struct _cmduExtension) * 1);
    }
    else
    {
        t->entries = (struct _cmduExtension *)PLATFORM_REALLOC(t->entries, sizeof(struct _cmduExtension) * (t->entries_nr + 1));
    }

    memcpy(t->entries[t->entries_nr].name, name, MAX_EXTENSION_NAME_LEN-1);
    t->entries[t->entries_nr].name[MAX_EXTENSION_NAME_LEN-1] = 0x0;
    t->entries[t->entries_nr].process = process;
    t->entries[t->entries_nr].send    = send;

    t->entries_nr++;

    return 1;
}

INT8U register1905AlmeDumpExtension(char *name,
                                    DM_OBTAIN_LOCAL_INFO_CBK obtain,
                                    DM_UPDATE_LOCAL_INFO_CBK update,
                                    DM_EXTENSION_CBK         dump)
{
    INT32U                              i;
    struct _ieee1905DmExtension  *t;

    if ((NULL == name) || (NULL == obtain) || (NULL == update) || (NULL == dump))
    {
        return 0;
    }

    t = &ieee1905_dm_extension;

    for (i=0; i<t->entries_nr; i++)
    {
        if ((NULL != t->entries[i].name) &&
            (0 == memcmp(t->entries[i].name, name, strlen(name) + 1)))
        {
            // Already exists!
            //
            PLATFORM_PRINTF_DEBUG_WARNING("[PLATFORM] A dump protocol extension with the name %s already exists. Ignoring...\n", name);
            return 0;
        }
    }

    if (0 == t->entries_nr)
    {
        t->entries = (struct _dmExtension *)PLATFORM_MALLOC(sizeof(struct _dmExtension) * 1);
    }
    else
    {
        t->entries = (struct _dmExtension *)PLATFORM_REALLOC(t->entries, sizeof(struct _dmExtension) * (t->entries_nr + 1));
    }

    memcpy(t->entries[t->entries_nr].name, name, MAX_EXTENSION_NAME_LEN-1);
    t->entries[t->entries_nr].name[MAX_EXTENSION_NAME_LEN-1] = 0x0;
    t->entries[t->entries_nr].obtain = obtain;
    t->entries[t->entries_nr].update = update;
    t->entries[t->entries_nr].dump   = dump;

    t->entries_nr++;

    return 1;
}
