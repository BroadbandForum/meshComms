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

#ifndef _MEDIA_SPECIFIC_BLOBS_H_
#define _MEDIA_SPECIFIC_BLOBS_H_


////////////////////////////////////////////////////////////////////////////////
// Structure type used for generic interfaces
////////////////////////////////////////////////////////////////////////////////

// The 1905 standard originally only recognized a limited set of interface
// types (IEEE802.11, IEEE802.3, IEEE1901, MOCA, ...) and for each of these
// types some parameters were defined (for example, the "bssid" and other
// things for IEEE802.11 interfaces, the "networ).
//
// Later, the standard was expanded to accept arbitrary parameters from all
// types of interfaces. This is done by using the next structure.
//
struct genericInterfaceType
{
    INT8U oui[3];           // Three bytes containing either the
                            // "Organizationally Unique Identifier"
                            // ("http://standards.ieee.org/develop/regauth/oui")
                            // or the "Company ID"
                            // ("http://standards.ieee.org/develop/regauth/cid")
                            // responsible for this particular interface type.

    char *generic_phy_description_xml_url;
                            // NULL terminated string pointing to the URL of
                            // the "Generic Phy XML Description Document" that
                            // describes the properties of this interface type.
                            // This document must conform to the "Generic Phy
                            // XML schema"
                            // ("http://standards.ieee.org/downloads/1905/GenericPhyInfoV1.xsd")

    INT8U variant_index;    // The "Generic Phy XML Description Document" might
                            // might contain more than one "class" of
                            // interfaces for each OUI. This variable is used
                            // to identify which class/variant is the one that
                            // applies to this particular interface type.

    char *variant_name;     // NULL terminated string containing the "friendly
                            // name" this variant receives in the "Generic Phy
                            // XML Description Document".
                            // Must not be longer than 32 bytes (including the
                            // final NULL character).
                            //   NOTE: The OUI and variant_index are
                            //   enough to identify a variant inside a
                            //   "Generic Phy XML Description Document",
                            //   however we required this field too for
                            //   convinience.

    union _mediaSpecific
    {
        struct _ituGhn
        {
            // This is the structure to fill when:
            //
            //  - 'generic_phy_description_xml_url' is set to
            //    "http://handle.itu.int/11.1002/3000/1706"
            //
            //  - 'oui' is set to 00:19:A7
            //
            //  - ...and the 'variant_index' is set to either 1, 2, 3 or 4

            INT8U dni[2];   // Domain name identifier (see clause
                            // 8.6.8.2.1 of "ITU-T G.9961")
        } ituGhn;

        struct _unsupported
        {
            // This is the structure to fill in all other cases. If you don't
            // want to provide media specific data, just set 'bytes_nr' to '0',
            // otherwise use this array to send arbitrary data to upper layers
            //
            INT16U  bytes_nr;
            INT8U  *bytes;

        } unsupported;

    } media_specific;

};



////////////////////////////////////////////////////////////////////////////////
// Main API functions
////////////////////////////////////////////////////////////////////////////////

// This function receives a pointer to a "genericInterfaceType" structure and
// then allocates and returns a buffer holding the array of bytes that should
// be inserted in the 1905 "media_specific" fields present in several TLVs
// ("generic phy device information TLV", "push button generic phy event
// notification TLV" and "power off interface TLV").
//
// This function "translates" the "genericInterfaceType" structure fields
// specific for each supported interface type into a stream of bytes according
// to the corresponding standard that applies to each particular type of
// interface.
//
// The returned pointer can later be free with "free_media_specific_blob()"
//
// 'len' is an output argument that holds the length of the returned buffer.
//
INT8U *forge_media_specific_blob(struct genericInterfaceType *m, INT16U *len);

// 'forge_media_specific_blob()' returns a regular buffer which can be freed
// using this macro defined to be PLATFORM_FREE
//
#define  free_media_specific_blob  PLATFORM_FREE

#endif
