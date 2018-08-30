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

#include "media_specific_blobs.h"


////////////////////////////////////////////////////////////////////////////////
// Actual API functions
////////////////////////////////////////////////////////////////////////////////

INT8U *forge_media_specific_blob(struct genericInterfaceType *m, INT16U *len)
{
    #define ITU_T_GHN_XML "http://handle.itu.int/11.1002/3000/1706"

    INT8U *ret;

    ret = NULL;

    if (NULL == m->generic_phy_description_xml_url)
    {
        return NULL;
    }

    if (0 == PLATFORM_MEMCMP(m->generic_phy_description_xml_url, ITU_T_GHN_XML, PLATFORM_STRLEN(ITU_T_GHN_XML)+1))
    {
        // This XML file defines the *same* media specific data format for all
        // interfaces that meet the following requirements:
        //
        //   - OUI     = 00:19:A7
        //   - Variant = 0, 1, 2, 3 or 4 (it also defines 10 and 11, but we will
        //               ignore these)

        if (
             m->oui[0] != 0x00 ||
             m->oui[1] != 0x19 ||
             m->oui[2] != 0xa7
           )
        {
            // Unknown OUI
        }
        else if (
             m->variant_index != 0 &&
             m->variant_index != 1 &&
             m->variant_index != 2 &&
             m->variant_index != 3 &&
             m->variant_index != 4
           )
        {
            // Unknown variant
        }
        else
        {
           // The 1905 media specific field is made up of FIVE bytes:
           //
           //   0x01, 0x00, 0x02, dni[0], dni[1]
           //
           // (see ITU-T G.9979 Tables 8.2 and 8.3)
           //
           *len   = 5;
           ret    = (INT8U *)PLATFORM_MALLOC(*len);
           ret[0] = 0x01;
           ret[1] = 0x00;
           ret[2] = 0x02;
           ret[3] = m->media_specific.ituGhn.dni[0];
           ret[4] = m->media_specific.ituGhn.dni[1];
        }
    }

    if (NULL == ret)
    {
        // If we get to this point and "ret" is still "NULL", that means that the
        // "XML"/"OUI"/"variant_index" combination has not been recognized, and thus
        // we will simply return the contents of the "unsupported" structure.
        //
        *len = m->media_specific.unsupported.bytes_nr;
        ret  = (INT8U *)PLATFORM_MALLOC(*len);

        PLATFORM_MEMCPY(ret, m->media_specific.unsupported.bytes, *len);
    }

    return ret;
}

