/*
 *  prplMesh Wi-Fi Multi-AP
 *
 *  Copyright (c) 2017, Broadband Forum
 *  Copyright (c) 2018, prpl Foundation
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

#include "tlv.h"
#include "1905_tlvs.h"
#include "packet_tools.h"

#include <stddef.h>
#include <string.h> // memcmp(), memcpy(), ...
#include <stdio.h>  // snprintf
#include <ctype.h>  // isprint(), isascii()


// Buffer size to store a prefix string that will be used to show each
// element of a structure on screen
//
#define MAX_PREFIX  100


/** @brief Support functions for supportedService TLV.
 *
 * See "Multi-AP Specification Version 1.0" Section 17.2.1
 *
 * @{
 */
#define TLV_NAME          supportedService
#define TLV_FIELD1_NAME   supported_service_nr
#define TLV_FIELD1_LENGTH 1
#define TLV_FIELD2_NAME   supported_service
#define TLV_PARSE         2
#define TLV_FORGE         2
#define TLV_PRINT         2
#define TLV_COMPARE       2
#define TLV_LENGTH_BODY
#define TLV_FREE_BODY

static bool tlv_parse_field2_supportedService(const struct tlv_def *def __attribute__((unused)),
                                              struct supportedServiceTLV *self,
                                              const uint8_t **buf,
                                              size_t *length)
{
    uint8_t i;
    if (self->supported_service_nr != *length)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Malformed %s TLV: supported_service_nr %u but length %u\n",
                                      def->desc.name, self->supported_service_nr, *length);
        return false;
    }
    self->supported_service = memalloc(self->supported_service_nr * sizeof(self->supported_service));

    for (i = 0; i < self->supported_service_nr; i++)
    {
        uint8_t service_type;
        _E1BL(buf, &service_type, length);
        self->supported_service[i] = (enum serviceType)service_type;
    }
    return true;
}

static uint16_t tlv_length_body_supportedService(const struct supportedServiceTLV *self)
{
    return 1 + self->supported_service_nr;
}

static bool tlv_forge_field2_supportedService(const struct supportedServiceTLV *self,
                                              uint8_t **buf,
                                              size_t *length)
{
    uint8_t i;
    for (i = 0; i < self->supported_service_nr; i++)
    {
        uint8_t service_type = (uint8_t) self->supported_service[i];
        if (!_I1BL(&service_type, buf, length))
            return false;
    }
    return true;
}

static void tlv_print_field2_supportedService(const struct supportedServiceTLV *self,
                                              void (*write_function)(const char *fmt, ...),
                                              const char *prefix)
{
    uint8_t i;
    char supported_services_list[80];
    size_t supported_services_list_len = 0;
    for (i = 0; i < self->supported_service_nr; i++)
    {
        snprintf(supported_services_list + supported_services_list_len,
                          sizeof (supported_services_list) - supported_services_list_len,
                          "0x%02x ", self->supported_service[i]);
        supported_services_list_len += 5;
        if (supported_services_list_len >= sizeof (supported_services_list) - 5 ||
            i == self->supported_service_nr - 1)
        {
            supported_services_list[supported_services_list_len] = '\0';
            print_callback(write_function, prefix, sizeof(self->supported_service[i]),
                           "supported_services", "%s", supported_services_list);
        }
    }
}

static void tlv_free_body_supportedService(const struct supportedServiceTLV *self)
{
    free(self->supported_service);
}

static bool tlv_compare_field2_supportedService(const struct supportedServiceTLV *self1,
                                                const struct supportedServiceTLV *self2)
{
    uint8_t i, j;
    /* Already checked before that they have the same nr */
    for (i = 0; i < self1->supported_service_nr; i++)
    {
        for (j = 0; j < self2->supported_service_nr; j++)
        {
            if (self1->supported_service[i] == self2->supported_service[j])
            {
                break;
            }
        }
        if (j == self2->supported_service_nr)
        {
            /* Not found in p2 */
            return false;
        }
    }
    /** All services of p1 were also found in p2, and they have the same number, so they are equal.
        @todo this does not check against duplicates */
    return true;
}

#include <tlv_template.h>

/** @} */

/** @brief Support functions for linkMetricQuery TLV.
 *
 * See "IEEE Std 1905.1-2013" Section 6.4.10
 *
 * @{
 */

#define TLV_NAME          linkMetricQuery
#define TLV_FIELD1_NAME   destination
#define TLV_FIELD1_LENGTH 1
#define TLV_FIELD2_NAME   specific_neighbor
#define TLV_FIELD3_NAME   link_metrics_type
#define TLV_FIELD3_LENGTH 1
#define TLV_PARSE         0b100
#define TLV_FORGE         0b010

static bool tlv_parse_field3_linkMetricQuery(const struct tlv_def *def __attribute__((unused)),
                                             struct linkMetricQueryTLV *self,
                                             const uint8_t **buf,
                                             size_t *length)
{
    uint8_t link_metrics_type;

    /* This is really a check of field1 and field2, but it's easier to include it here. */
    if (0 == self->destination)
    {
        uint8_t dummy_address[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

        self->destination = LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS;
        memcpy(self->specific_neighbor, dummy_address, 6);
    }
    else if (1 == self->destination)
    {
        self->destination = LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR;
    }
    else
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Malformed %s TLV: invalid destination %u\n", def->desc.name, self->destination);
        return false;
    }

    if (!_E1BL(buf, &link_metrics_type, length))
        return false;

    if (0 == link_metrics_type)
    {
        self->link_metrics_type = LINK_METRIC_QUERY_TLV_TX_LINK_METRICS_ONLY;
    }
    else if (1 == link_metrics_type)
    {
        self->link_metrics_type = LINK_METRIC_QUERY_TLV_RX_LINK_METRICS_ONLY;
    }
    else if (2 == link_metrics_type)
    {
        self->link_metrics_type = LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS;
    }
    else
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Malformed %s TLV: invalid link_metrics_type %u\n", def->desc.name, self->link_metrics_type);
        return false;
    }

    return true;
}

static bool tlv_forge_field2_linkMetricQuery(const struct linkMetricQueryTLV *self,
                                             uint8_t **buf,
                                             size_t *length)
{
    if (LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR == self->destination)
    {
        if (!_ImBL(self->specific_neighbor, buf, length))
            return false;
    }
    else
    {
        /*
         * Ugh? Why is the first value set to "self->link_metrics_type" instead of "0x00"? What kind of black magic is
         * this?
         *
         * Well... it turns out there is a reason for this. Take a chair and let me explain.
         *
         * The original 1905 standard document (and also its later "1a" update) describe the "metric query TLV" fields
         * like this:
         *
         *   - Field #1: 1 octet set to "8" (tlv.type)
         *   - Field #2: 1 octet set to "8" (tlv_length)
         *   - Field #3: 1 octet set to "0" or "1" (destination)
         *   - Field #4: 6 octets set to the MAC address of a neighbour when field #3 is set "1"
         *   - Field #5: 1 octet set to "0", "1", "2" or "3" (link_metrics_type)
         *
         * The problem is that we don't know what to put inside field #4 when Field #3 is set to "0" ("all neighbors")
         * instead of "1" ("specific neighbor").
         *
         * A "reasonable" solution would be to set all bytes from field #4 to "0x00". *However*, one could also think
         * that the correct thing to do is to not include the field at all (ie. skip from field #3 to field #5).
         *
         * Now... this is actually insane. Typically protocols have a fixed number of fields (whenever possible) to
         * make it easier for parsers (in fact, this would be the only exception to this rule in the whole 1905
         * standard). Then... why would someone think that not including field #4 is a good idea?
         *
         * Well... because this is what the "description" of field #3 reads on the standard:
         *
         *   "If the value is 0, then the EUI-48 field is not present;
         *    if the value is 1, then the EUI-48 field shall be present"
         *
         * ...and "not present" seems to imply not to include it (although one could argue that it could also mean "set
         * all bytes to zero).
         *
         * I really think the standard means "set to zero" instead of "not including it" (even if the wording seems to
         * imply otherwise). Why? For two reasons:
         *
         *   1. The standard says field #2 must *always* be "8" (and if field #4 could not be included, this value
         *      should be allowed to also take the value of 6)
         *
         *   2. There is no other place in the whole standard where a field can be present or not.
         *
         * Despite what I have just said, *some implementations* seem to have taken the other route, and expect field #4
         * *not* to be present (even if field #2 is set to "8"!!).
         *
         * When we send one "all neighbors" topology query to one of these implementations they will interpret the first
         * byte of field #4 as the contents of field #5.
         *
         * And that's why when querying for all neighbors, because the contents of field #4 don't really matter, we are
         * going to set its first byte to the same value as field #5. This way all implementations, no matter how they
         * decided to interpret the standard, will work :)
         */
        uint8_t empty_address[] = {self->link_metrics_type, 0x00, 0x00, 0x00, 0x00, 0x00};
        if (!_ImBL(empty_address, buf, length))
            return false;
    }
    return true;
}

#include <tlv_template.h>

/** @} */

/** @brief Support functions for vendorSpecific TLV.
 *
 * See "IEEE Std 1905.1-2013" Section 6.4.2
 *
 * @{
 */

#define TLV_NAME          vendorSpecific
#define TLV_FIELD1_NAME   vendorOUI
#define TLV_FIELD2_NAME   m
#define TLV_PARSE         2
#define TLV_FORGE         2
#define TLV_PRINT         2
#define TLV_COMPARE       2
#define TLV_LENGTH_BODY
#define TLV_FREE_BODY

static bool tlv_parse_field2_vendorSpecific(const struct tlv_def *def __attribute__((unused)),
                                            struct vendorSpecificTLV *self,
                                            const uint8_t **buf,
                                            size_t *length)
{
    self->m = memalloc(*length);
    self->m_nr = *length;
    return _EnBL(buf, self->m, *length, length);
}

static uint16_t tlv_length_body_vendorSpecific(const struct vendorSpecificTLV *self)
{
    return 3 + self->m_nr;
}

static bool tlv_forge_field2_vendorSpecific(const struct vendorSpecificTLV *self,
                                            uint8_t **buf,
                                            size_t *length)
{
    return _InBL(self->m, buf, self->m_nr, length);
}

static void tlv_free_body_vendorSpecific(const struct vendorSpecificTLV *self)
{
    free(self->m);
}

static void tlv_print_field2_vendorSpecific(const struct vendorSpecificTLV *self,
                                            void (*write_function)(const char *fmt, ...),
                                            const char *prefix)
{
    print_callback(write_function, prefix, sizeof(self->m_nr), "m_nr", "%d ", &self->m_nr);
    print_callback(write_function, prefix, self->m_nr, "m", "0x%02x", self->m);
}

static bool tlv_compare_field2_vendorSpecific(const struct vendorSpecificTLV *self1,
                                              const struct vendorSpecificTLV *self2)
{
    if (self1->m_nr != self2->m_nr)
        return false;
    else if (memcmp(self1->m, self2->m, self1->m_nr) != 0)
        return false;
    else
        return true;
}

#include <tlv_template.h>

/** @} */

/** @brief Support functions for alMacAddressType TLV.
 *
 * See "IEEE Std 1905.1-2013" Section 6.4.3
 *
 * @{
 */

#define TLV_NAME          alMacAddressType
#define TLV_FIELD1_NAME   al_mac_address

#include <tlv_template.h>

/** @} */

/** @brief Support functions for macAddressType TLV.
 *
 * See "IEEE Std 1905.1-2013" Section 6.4.4
 *
 * @{
 */

#define TLV_NAME          macAddressType
#define TLV_FIELD1_NAME   mac_address

#include <tlv_template.h>

/** @} */

/** @brief Support functions for apOperationalBss TLV.
 *
 * See "Multi-AP Specification Version 1.0" Section 17.2.1
 *
 * @{
 */

static const struct tlv_struct_description _apOperationalBssInfoDesc;

static struct tlv_struct *_apOperationalBssInfoParse(const struct tlv_struct_description *desc, hlist_head *parent,
                                                     const uint8_t **buffer, size_t *length)
{
    struct _apOperationalBssInfo *bss_info =
            TLV_STRUCT_ALLOC(&_apOperationalBssInfoDesc, struct _apOperationalBssInfo, s, parent);
    if (!_EmBL(buffer, bss_info->bssid, length))
        goto error_out;
    if (!_E1BL(buffer, &bss_info->ssid.length, length))
        goto error_out;
    if (bss_info->ssid.length > SSID_MAX_LEN)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Malformed %s TLV: SSID too large %u > %u\n", desc->name,
                                      bss_info->ssid.length, SSID_MAX_LEN);
        goto error_out;
    }
    if (!_EnBL(buffer, bss_info->ssid.ssid, bss_info->ssid.length, length))
        goto error_out;

    return &bss_info->s;

error_out:
    hlist_delete_item(&bss_info->s.h);
    return NULL;

}

static size_t _apOperationalBssInfoLength(const struct tlv_struct *item)
{
    const struct _apOperationalBssInfo *bss_info = container_of(item, const struct _apOperationalBssInfo, s);
    return 6 + 1 + bss_info->ssid.length;
}

static bool _apOperationalBssInfoForge(const struct tlv_struct *item, uint8_t **buffer, size_t *length)
{
    const struct _apOperationalBssInfo *bss_info = container_of(item, const struct _apOperationalBssInfo, s);
    if (!_ImBL(bss_info->bssid, buffer, length))
        return false;
    if (!_I1BL(&bss_info->ssid.length, buffer, length))
        return false;
    if (!_InBL(bss_info->ssid.ssid, buffer, bss_info->ssid.length, length))
        return false;

    return true;
}

static void _apOperationalBssInfoPrint(const struct tlv_struct *item,
                                       void (*write_function)(const char *fmt, ...),
                                       const char *prefix)

{
    const struct _apOperationalBssInfo *bss_info = container_of(item, const struct _apOperationalBssInfo, s);
    uint8_t i;

    tlv_struct_print_field(item, &item->desc->fields[0], write_function, prefix);

    write_function("%s->ssid: \"", prefix);
    for (i = 0; i < bss_info->ssid.length; i++)
    {
        if (isprint(bss_info->ssid.ssid[i]) && isascii(bss_info->ssid.ssid[i]))
        {
            write_function("%c", bss_info->ssid.ssid[i]);
        }
        else
        {
            write_function("\\x%02x", bss_info->ssid.ssid[i]);
        }
    }
    write_function("\"\n");
}

static const struct tlv_struct_description _apOperationalBssInfoDesc = {
    .name = "bss",
    .size = sizeof(struct _apOperationalBssInfo),
    .fields = {
        TLV_STRUCT_FIELD_DESCRIPTION(struct _apOperationalBssInfo, bssid, tlv_struct_print_format_mac),
        /* This is not actually used since we override all functions, but for reference we define it */
        TLV_STRUCT_FIELD_DESCRIPTION(struct _apOperationalBssInfo, ssid, tlv_struct_print_format_hex),
        TLV_STRUCT_FIELD_SENTINEL,
    },
    .children = {NULL,},
    .parse = _apOperationalBssInfoParse,
    .length = _apOperationalBssInfoLength,
    .forge = _apOperationalBssInfoForge,
    .print = _apOperationalBssInfoPrint,
};

static const struct tlv_struct_description _apOperationalBssRadioDesc = {
    .name = "radio",
    .size = sizeof(struct _apOperationalBssRadio),
    .fields = {
        TLV_STRUCT_FIELD_DESCRIPTION(struct _apOperationalBssRadio, radio_uid, tlv_struct_print_format_mac),
        TLV_STRUCT_FIELD_SENTINEL,
    },
    .children = { &_apOperationalBssInfoDesc, NULL, },
};


/** @} */

/** @brief Support functions for associatedClients TLV.
 *
 * See "Multi-AP Specification Version 1.0" Section 17.2.1
 *
 * @{
 */

static const struct tlv_struct_description _associatedClientInfoDesc = {
    .name = "client",
    .size = sizeof(struct _associatedClientInfo),
    .fields = {
        TLV_STRUCT_FIELD_DESCRIPTION(struct _associatedClientInfo, addr, tlv_struct_print_format_mac),
        TLV_STRUCT_FIELD_DESCRIPTION(struct _associatedClientInfo, age, tlv_struct_print_format_unsigned),
        TLV_STRUCT_FIELD_SENTINEL,
    },
    .children = {NULL,},
};

static const struct tlv_struct_description _associatedClientsBssInfoDesc = {
    .name = "bss",
    .size = sizeof(struct _associatedClientsBssInfo),
    .fields = {
        TLV_STRUCT_FIELD_DESCRIPTION(struct _associatedClientsBssInfo, bssid, tlv_struct_print_format_mac),
        TLV_STRUCT_FIELD_SENTINEL,
    },
    .children = {
        &_associatedClientInfoDesc,
        NULL
    }
};

/** @} */


static tlv_defs_t tlv_1905_defs = {
    [TLV_TYPE_END_OF_MESSAGE] = {
        .type = TLV_TYPE_END_OF_MESSAGE,
        .desc = {
            .name = "endOfMessage",
            .size = sizeof(struct tlv),
        },
    },
    TLV_DEF_ENTRY(vendorSpecific,TLV_TYPE_VENDOR_SPECIFIC),
    TLV_DEF_ENTRY(alMacAddressType,TLV_TYPE_AL_MAC_ADDRESS_TYPE),
    TLV_DEF_ENTRY(macAddressType,TLV_TYPE_MAC_ADDRESS_TYPE),
    TLV_DEF_ENTRY(linkMetricQuery,TLV_TYPE_LINK_METRIC_QUERY),
    TLV_DEF_ENTRY(supportedService,TLV_TYPE_SUPPORTED_SERVICE),
    /* Searched service is exactly the same as supported service, so reuse the functions. */
    [TLV_TYPE_SEARCHED_SERVICE] = {
        .type = TLV_TYPE_SEARCHED_SERVICE,
        .desc = {
            .name = "searchedService",
            .size = sizeof(struct supportedServiceTLV),
            .fields = {
                TLV_STRUCT_FIELD_SENTINEL,
            },
            .children = {
                NULL
            }
        },
        .parse = tlv_parse_supportedService,
        .length = tlv_length_supportedService,
        .forge = tlv_forge_supportedService,
        .print = tlv_print_supportedService,
        .free = tlv_free_supportedService,
        .compare = tlv_compare_supportedService,
    },
    TLV_DEF_ENTRY_NEW(apOperationalBss,TLV_TYPE_AP_OPERATIONAL_BSS,
        .fields = {
            TLV_STRUCT_FIELD_SENTINEL,
        },
        .children = {
            &_apOperationalBssRadioDesc,
            NULL,
        }
    ),
    TLV_DEF_ENTRY_NEW(associatedClients,TLV_TYPE_ASSOCIATED_CLIENTS,
        .fields = {
            TLV_STRUCT_FIELD_SENTINEL,
        },
        .children = {
            &_associatedClientsBssInfoDesc,
            NULL
        }
    ),
};

struct apOperationalBssTLV* apOperationalBssTLVAlloc(hlist_head *parent)
{
    struct apOperationalBssTLV *ret = TLV_STRUCT_ALLOC(&tlv_1905_defs[TLV_TYPE_AP_OPERATIONAL_BSS].desc,
                                                       struct apOperationalBssTLV, tlv.s, parent);
    ret->tlv.type = TLV_TYPE_AP_OPERATIONAL_BSS;
    return ret;
}

struct _apOperationalBssRadio *apOperationalBssTLVAddRadio(struct apOperationalBssTLV* a, mac_address radio_uid)
{
    struct _apOperationalBssRadio *ret = TLV_STRUCT_ALLOC(&_apOperationalBssRadioDesc, struct _apOperationalBssRadio, s,
                                                          &a->tlv.s.h.children[0]);
    memcpy(ret->radio_uid, radio_uid, 6);
    return ret;
}

struct _apOperationalBssInfo *apOperationalBssRadioAddBss(struct _apOperationalBssRadio* a,
                                                          mac_address bssid, struct ssid ssid)
{
    struct _apOperationalBssInfo *ret = TLV_STRUCT_ALLOC(&_apOperationalBssInfoDesc, struct _apOperationalBssInfo, s,
                                                          &a->s.h.children[0]);
    memcpy(ret->bssid, bssid, 6);
    memset(&ret->ssid.ssid, 0, sizeof(ret->ssid.ssid));
    ret->ssid.length = ssid.length;
    memcpy(ret->ssid.ssid, ssid.ssid, ssid.length);
    return ret;

}

struct _associatedClientsBssInfo *associatedClientsTLVAddBssInfo (struct associatedClientsTLV* a, mac_address bssid)
{
    struct _associatedClientsBssInfo *ret =
            TLV_STRUCT_ALLOC(&_associatedClientsBssInfoDesc, struct _associatedClientsBssInfo, s, &a->tlv.s.h.children[0]);
    memcpy(ret->bssid, bssid, sizeof(mac_address));
    return ret;
}

struct _associatedClientInfo *associatedClientsTLVAddClientInfo (struct _associatedClientsBssInfo* a,
                                                                 mac_address addr, uint16_t age)
{
    struct _associatedClientInfo *ret = TLV_STRUCT_ALLOC(&_associatedClientInfoDesc, struct _associatedClientInfo, s, &a->s.h.children[0]);
    memcpy(ret->addr, addr, sizeof(mac_address));
    ret->age = age;
    return ret;
}

struct associatedClientsTLV* associatedClientsTLVAlloc(hlist_head *parent)
{
    struct associatedClientsTLV *ret = TLV_STRUCT_ALLOC(&tlv_1905_defs[TLV_TYPE_ASSOCIATED_CLIENTS].desc,
                                                   struct associatedClientsTLV, tlv.s, parent);
    ret->tlv.type = TLV_TYPE_ASSOCIATED_CLIENTS;
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Actual API functions
////////////////////////////////////////////////////////////////////////////////

struct tlv *parse_1905_TLV_from_packet(const uint8_t *packet_stream)
{
    const uint8_t *p;
    if (NULL == packet_stream)
    {
        return NULL;
    }

    // The first byte of the stream is the "Type" field from the TLV structure.
    // Valid values for this byte are the following ones...
    //
    switch (*packet_stream)
    {
        case TLV_TYPE_DEVICE_INFORMATION_TYPE:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.5"

            struct deviceInformationTypeTLV  *ret;

            uint16_t len;
            uint8_t  i;

            ret = (struct deviceInformationTypeTLV *)memalloc(sizeof(struct deviceInformationTypeTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            ret->tlv.type = TLV_TYPE_DEVICE_INFORMATION_TYPE;

            _EnB(&p,  ret->al_mac_address, 6);
            _E1B(&p, &ret->local_interfaces_nr);

            ret->local_interfaces = (struct _localInterfaceEntries *)memalloc(sizeof(struct _localInterfaceEntries) * ret->local_interfaces_nr);

            for (i=0; i < ret->local_interfaces_nr; i++)
            {
                _EnB(&p,  ret->local_interfaces[i].mac_address, 6);
                _E2B(&p, &ret->local_interfaces[i].media_type);
                _E1B(&p, &ret->local_interfaces[i].media_specific_data_size);

                if (
                     (MEDIA_TYPE_IEEE_802_11B_2_4_GHZ == ret->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11G_2_4_GHZ == ret->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11A_5_GHZ   == ret->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_2_4_GHZ == ret->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_5_GHZ   == ret->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AC_5_GHZ  == ret->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AD_60_GHZ == ret->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AF_GHZ    == ret->local_interfaces[i].media_type)
                   )
                {
                    uint8_t aux;

                    if (10 != ret->local_interfaces[i].media_specific_data_size)
                    {
                        // Malformed packet
                        //
                        free(ret->local_interfaces);
                        free(ret);
                        return NULL;
                    }

                    _EnB(&p, ret->local_interfaces[i].media_specific_data.ieee80211.network_membership, 6);
                    _E1B(&p, &aux);
                    ret->local_interfaces[i].media_specific_data.ieee80211.role = aux >> 4;
                    _E1B(&p, &ret->local_interfaces[i].media_specific_data.ieee80211.ap_channel_band);
                    _E1B(&p, &ret->local_interfaces[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_1);
                    _E1B(&p, &ret->local_interfaces[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_2);

                }
                else if (
                          (MEDIA_TYPE_IEEE_1901_WAVELET == ret->local_interfaces[i].media_type) ||
                          (MEDIA_TYPE_IEEE_1901_FFT     == ret->local_interfaces[i].media_type)
                        )
                {
                    if (7 != ret->local_interfaces[i].media_specific_data_size)
                    {
                        // Malformed packet
                        //
                        free(ret->local_interfaces);
                        free(ret);
                        return NULL;
                    }
                    _EnB(&p, ret->local_interfaces[i].media_specific_data.ieee1901.network_identifier, 7);
                }
                else
                {
                    if (0 != ret->local_interfaces[i].media_specific_data_size)
                    {
                        // Malformed packet
                        //
                        free(ret->local_interfaces);
                        free(ret);
                        return NULL;
                    }
                }
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                free(ret->local_interfaces);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        case TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.6"

            struct deviceBridgingCapabilityTLV  *ret;

            uint16_t len;
            uint8_t  i, j;

            ret = (struct deviceBridgingCapabilityTLV *)memalloc(sizeof(struct deviceBridgingCapabilityTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            ret->tlv.type = TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES;

            if (0 == len)
            {
#ifdef FIX_BROKEN_TLVS
                // Malformed packet. Even if there are NO bridging tuples, the
                // length should be "1" (which is the length of the next field,
                // that would containing a "zero", indicating the number of
                // bridging tuples).
                // *However*, because at least one other implementation sets
                // the 'length' to zero to indicate "no bridging tuples", we
                // will also accept this type of "malformed" packet.
                //
                ret->bridging_tuples_nr = 0;
                return &ret->tlv;
#else
                free(ret);
                return NULL;
#endif
            }

            _E1B(&p, &ret->bridging_tuples_nr);

            if (ret->bridging_tuples_nr > 0)
            {
                ret->bridging_tuples = (struct _bridgingTupleEntries *)memalloc(sizeof(struct _bridgingTupleEntries) * ret->bridging_tuples_nr);

                for (i=0; i < ret->bridging_tuples_nr; i++)
                {
                    _E1B(&p, &ret->bridging_tuples[i].bridging_tuple_macs_nr);

                    if (ret->bridging_tuples[i].bridging_tuple_macs_nr > 0)
                    {
                        ret->bridging_tuples[i].bridging_tuple_macs = (struct _bridgingTupleMacEntries *)memalloc(sizeof(struct _bridgingTupleMacEntries) * ret->bridging_tuples[i].bridging_tuple_macs_nr);

                        for (j=0; j < ret->bridging_tuples[i].bridging_tuple_macs_nr; j++)
                        {
                            _EnB(&p, ret->bridging_tuples[i].bridging_tuple_macs[j].mac_address, 6);
                        }
                    }
                }
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                for (i=0; i < ret->bridging_tuples_nr; i++)
                {
                    free(ret->bridging_tuples[i].bridging_tuple_macs);
                }
                free(ret->bridging_tuples);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        case TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.8"

            struct non1905NeighborDeviceListTLV  *ret;

            uint16_t len;
            uint8_t  i;

            ret = (struct non1905NeighborDeviceListTLV *)memalloc(sizeof(struct non1905NeighborDeviceListTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // According to the standard, the length *must* be "6 + 6*n"
            //
            if (0 != ((len-6)%6))
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }
            ret->tlv.type = TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST;

            _EnB(&p,  ret->local_mac_address, 6);

            ret->non_1905_neighbors_nr = (len-6)/6;

            ret->non_1905_neighbors = (struct _non1905neighborEntries *)memalloc(sizeof(struct _non1905neighborEntries) * ret->non_1905_neighbors_nr);

            for (i=0; i < ret->non_1905_neighbors_nr; i++)
            {
                _EnB(&p,  ret->non_1905_neighbors[i].mac_address, 6);
            }

            return &ret->tlv;
        }

        case TLV_TYPE_NEIGHBOR_DEVICE_LIST:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.9"

            struct neighborDeviceListTLV  *ret;

            uint16_t len;
            uint8_t  i;

            ret = (struct neighborDeviceListTLV *)memalloc(sizeof(struct neighborDeviceListTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // According to the standard, the length *must* be "6 + 7*n"
            // "6+1"
            //
            if (0 != ((len-6)%7))
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }
            ret->tlv.type = TLV_TYPE_NEIGHBOR_DEVICE_LIST;

            _EnB(&p,  ret->local_mac_address, 6);

            ret->neighbors_nr = (len-6)/7;

            ret->neighbors = (struct _neighborEntries *)memalloc(sizeof(struct _neighborEntries) * ret->neighbors_nr);

            for (i=0; i < ret->neighbors_nr; i++)
            {
                uint8_t aux;

                _EnB(&p,  ret->neighbors[i].mac_address, 6);
                _E1B(&p, &aux);

                if (aux & 0x80)
                {
                    ret->neighbors[i].bridge_flag = 1;
                }
                else
                {
                    ret->neighbors[i].bridge_flag = 0;
                }
            }

            return &ret->tlv;
        }

        case TLV_TYPE_TRANSMITTER_LINK_METRIC:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.11"

            struct transmitterLinkMetricTLV  *ret;

            uint16_t len;
            uint8_t  i;

            ret = (struct transmitterLinkMetricTLV *)memalloc(sizeof(struct transmitterLinkMetricTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // According to the standard, the length *must* be "12+29*n" where
            // "n" is "1" or greater
            //
            if ((12+29*1) > len)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }
            if (0 != (len-12)%29)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->tlv.type = TLV_TYPE_TRANSMITTER_LINK_METRIC;

            _EnB(&p, ret->local_al_address,    6);
            _EnB(&p, ret->neighbor_al_address, 6);

            ret->transmitter_link_metrics_nr = (len-12)/29;

            ret->transmitter_link_metrics = (struct _transmitterLinkMetricEntries *)memalloc(sizeof(struct _transmitterLinkMetricEntries) * ret->transmitter_link_metrics_nr);

            for (i=0; i < ret->transmitter_link_metrics_nr; i++)
            {
                _EnB(&p,  ret->transmitter_link_metrics[i].local_interface_address,    6);
                _EnB(&p,  ret->transmitter_link_metrics[i].neighbor_interface_address, 6);

                _E2B(&p, &ret->transmitter_link_metrics[i].intf_type);
                _E1B(&p, &ret->transmitter_link_metrics[i].bridge_flag);
                _E4B(&p, &ret->transmitter_link_metrics[i].packet_errors);
                _E4B(&p, &ret->transmitter_link_metrics[i].transmitted_packets);
                _E2B(&p, &ret->transmitter_link_metrics[i].mac_throughput_capacity);
                _E2B(&p, &ret->transmitter_link_metrics[i].link_availability);
                _E2B(&p, &ret->transmitter_link_metrics[i].phy_rate);
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                free(ret->transmitter_link_metrics);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        case TLV_TYPE_RECEIVER_LINK_METRIC:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.12"

            struct receiverLinkMetricTLV  *ret;

            uint16_t len;
            uint8_t  i;

            ret = (struct receiverLinkMetricTLV *)memalloc(sizeof(struct receiverLinkMetricTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // According to the standard, the length *must* be "12+23*n" where
            // "n" is "1" or greater
            //
            if ((12+23*1) > len)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }
            if (0 != (len-12)%23)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->tlv.type = TLV_TYPE_RECEIVER_LINK_METRIC;

            _EnB(&p, ret->local_al_address,    6);
            _EnB(&p, ret->neighbor_al_address, 6);

            ret->receiver_link_metrics_nr = (len-12)/23;

            ret->receiver_link_metrics = (struct _receiverLinkMetricEntries *)memalloc(sizeof(struct _receiverLinkMetricEntries) * ret->receiver_link_metrics_nr);

            for (i=0; i < ret->receiver_link_metrics_nr; i++)
            {
                _EnB(&p,  ret->receiver_link_metrics[i].local_interface_address,    6);
                _EnB(&p,  ret->receiver_link_metrics[i].neighbor_interface_address, 6);

                _E2B(&p, &ret->receiver_link_metrics[i].intf_type);
                _E4B(&p, &ret->receiver_link_metrics[i].packet_errors);
                _E4B(&p, &ret->receiver_link_metrics[i].packets_received);
                _E1B(&p, &ret->receiver_link_metrics[i].rssi);
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                free(ret->receiver_link_metrics);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        case TLV_TYPE_LINK_METRIC_RESULT_CODE:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.13"

            struct linkMetricResultCodeTLV  *ret;

            uint16_t len;

            ret = (struct linkMetricResultCodeTLV *)memalloc(sizeof(struct linkMetricResultCodeTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // According to the standard, the length *must* be 1
            //
            if (1 != len)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->tlv.type = TLV_TYPE_LINK_METRIC_RESULT_CODE;

            _E1B(&p, &ret->result_code);

            return &ret->tlv;
        }

        case TLV_TYPE_SEARCHED_ROLE:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.14"

            struct searchedRoleTLV  *ret;

            uint16_t len;

            ret = (struct searchedRoleTLV *)memalloc(sizeof(struct searchedRoleTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // According to the standard, the length *must* be 1
            //
            if (1 != len)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->tlv.type = TLV_TYPE_SEARCHED_ROLE;

            _E1B(&p, &ret->role);

            return &ret->tlv;
        }

        case TLV_TYPE_AUTOCONFIG_FREQ_BAND:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.15"

            struct autoconfigFreqBandTLV  *ret;

            uint16_t len;

            ret = (struct autoconfigFreqBandTLV *)memalloc(sizeof(struct autoconfigFreqBandTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // According to the standard, the length *must* be 1
            //
            if (1 != len)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->tlv.type = TLV_TYPE_AUTOCONFIG_FREQ_BAND;

            _E1B(&p, &ret->freq_band);

            return &ret->tlv;
        }

        case TLV_TYPE_SUPPORTED_ROLE:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.16"

            struct supportedRoleTLV  *ret;

            uint16_t len;

            ret = (struct supportedRoleTLV *)memalloc(sizeof(struct supportedRoleTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // According to the standard, the length *must* be 1
            //
            if (1 != len)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->tlv.type = TLV_TYPE_SUPPORTED_ROLE;

            _E1B(&p, &ret->role);

            return &ret->tlv;
        }

        case TLV_TYPE_SUPPORTED_FREQ_BAND:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.17"

            struct supportedFreqBandTLV  *ret;

            uint16_t len;

            ret = (struct supportedFreqBandTLV *)memalloc(sizeof(struct supportedFreqBandTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // According to the standard, the length *must* be 1
            //
            if (1 != len)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->tlv.type = TLV_TYPE_SUPPORTED_FREQ_BAND;

            _E1B(&p, &ret->freq_band);

            return &ret->tlv;
        }

        case TLV_TYPE_WSC:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.18"

            struct wscTLV  *ret;

            uint16_t len;

            ret = (struct wscTLV *)memalloc(sizeof(struct wscTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            ret->tlv.type       = TLV_TYPE_WSC;
            ret->wsc_frame_size = len;

            if (len>0)
            {
                ret->wsc_frame      = (uint8_t *)memalloc(len);
                _EnB(&p, ret->wsc_frame, len);
            }

            return &ret->tlv;
        }

        case TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.19"

            struct pushButtonEventNotificationTLV  *ret;

            uint16_t len;
            uint8_t i;

            ret = (struct pushButtonEventNotificationTLV *)memalloc(sizeof(struct pushButtonEventNotificationTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            ret->tlv.type = TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION;

            if (0 == len)
            {
#ifdef FIX_BROKEN_TLVS
                // Malformed packet. Even if there are NO bridging tuples, the
                // Malformed packet. Even if there are NO media types, the
                // length should be "1" (which is the length of the next field,
                // that would containing a "zero", indicating the number of
                // media types).
                // *However*, because at least one other implementation sets
                // the 'length' to zero to indicate "no media types", we will
                // also accept this type of "malformed" packet.
                //
                ret->media_types_nr = 0;
                return &ret->tlv;
#else
                free(ret);
                return NULL;
#endif
            }

            _E1B(&p, &ret->media_types_nr);

            ret->media_types = (struct _mediaTypeEntries *)memalloc(sizeof(struct _mediaTypeEntries) * ret->media_types_nr);

            for (i=0; i < ret->media_types_nr; i++)
            {
                _E2B(&p, &ret->media_types[i].media_type);
                _E1B(&p, &ret->media_types[i].media_specific_data_size);

                if (
                     (MEDIA_TYPE_IEEE_802_11B_2_4_GHZ == ret->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11G_2_4_GHZ == ret->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11A_5_GHZ   == ret->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_2_4_GHZ == ret->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_5_GHZ   == ret->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AC_5_GHZ  == ret->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AD_60_GHZ == ret->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AF_GHZ    == ret->media_types[i].media_type)
                   )
                {
                    uint8_t aux;

                    if (10 != ret->media_types[i].media_specific_data_size)
                    {
                        // Malformed packet
                        //
                        free(ret->media_types);
                        free(ret);
                        return NULL;
                    }

                    _EnB(&p, ret->media_types[i].media_specific_data.ieee80211.network_membership, 6);
                    _E1B(&p, &aux);
                    ret->media_types[i].media_specific_data.ieee80211.role = aux >> 4;
                    _E1B(&p, &ret->media_types[i].media_specific_data.ieee80211.ap_channel_band);
                    _E1B(&p, &ret->media_types[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_1);
                    _E1B(&p, &ret->media_types[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_2);

                }
                else if (
                          (MEDIA_TYPE_IEEE_1901_WAVELET == ret->media_types[i].media_type) ||
                          (MEDIA_TYPE_IEEE_1901_FFT     == ret->media_types[i].media_type)
                        )
                {
                    if (7 != ret->media_types[i].media_specific_data_size)
                    {
                        // Malformed packet
                        //
                        free(ret->media_types);
                        free(ret);
                        return NULL;
                    }
                    _EnB(&p, ret->media_types[i].media_specific_data.ieee1901.network_identifier, 7);
                }
                else
                {
                    if (0 != ret->media_types[i].media_specific_data_size)
                    {
                        // Malformed packet
                        //
                        free(ret->media_types);
                        free(ret);
                        return NULL;
                    }
                }
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                free(ret->media_types);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        case TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.20"

            struct pushButtonJoinNotificationTLV  *ret;

            uint16_t len;

            ret = (struct pushButtonJoinNotificationTLV *)memalloc(sizeof(struct pushButtonJoinNotificationTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // According to the standard, the length *must* be 20
            //
            if (20 != len)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->tlv.type = TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION;

            _EnB(&p,  ret->al_mac_address, 6);
            _E2B(&p, &ret->message_identifier);
            _EnB(&p,  ret->mac_address, 6);
            _EnB(&p,  ret->new_mac_address, 6);

            return &ret->tlv;
        }

        case TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.21"

            struct genericPhyDeviceInformationTypeTLV  *ret;

            uint16_t len;
            uint8_t  i;

            ret = (struct genericPhyDeviceInformationTypeTLV *)memalloc(sizeof(struct genericPhyDeviceInformationTypeTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            ret->tlv.type = TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION;

            _EnB(&p,  ret->al_mac_address, 6);
            _E1B(&p, &ret->local_interfaces_nr);

            if (ret->local_interfaces_nr > 0)
            {
                ret->local_interfaces = (struct _genericPhyDeviceEntries *)memalloc(sizeof(struct _genericPhyDeviceEntries) * ret->local_interfaces_nr);

                for (i=0; i < ret->local_interfaces_nr; i++)
                {
                    _EnB(&p,  ret->local_interfaces[i].local_interface_address,                 6);
                    _EnB(&p,  ret->local_interfaces[i].generic_phy_common_data.oui,             3);
                    _E1B(&p, &ret->local_interfaces[i].generic_phy_common_data.variant_index);
                    _EnB(&p,  ret->local_interfaces[i].variant_name,                           32);
                    _E1B(&p, &ret->local_interfaces[i].generic_phy_description_xml_url_len);
                    _E1B(&p, &ret->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr);

                    if (ret->local_interfaces[i].generic_phy_description_xml_url_len > 0)
                    {
                        ret->local_interfaces[i].generic_phy_description_xml_url = (char *)memalloc(ret->local_interfaces[i].generic_phy_description_xml_url_len);
                        _EnB(&p, ret->local_interfaces[i].generic_phy_description_xml_url, ret->local_interfaces[i].generic_phy_description_xml_url_len);
                    }

                    if (ret->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr > 0)
                    {
                        ret->local_interfaces[i].generic_phy_common_data.media_specific_bytes = (uint8_t *)memalloc(ret->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr);
                        _EnB(&p, ret->local_interfaces[i].generic_phy_common_data.media_specific_bytes, ret->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr);
                    }
                }
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                for (i=0; i < ret->local_interfaces_nr; i++)
                {
                    if (ret->local_interfaces[i].generic_phy_description_xml_url_len > 0)
                    {
                        free(ret->local_interfaces[i].generic_phy_description_xml_url);
                    }

                    if (ret->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr > 0)
                    {
                        free(ret->local_interfaces[i].generic_phy_common_data.media_specific_bytes);
                    }
                }
                free(ret->local_interfaces);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        case TLV_TYPE_DEVICE_IDENTIFICATION:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.22"

            struct deviceIdentificationTypeTLV  *ret;

            uint16_t len;

            ret = (struct deviceIdentificationTypeTLV *)memalloc(sizeof(struct deviceIdentificationTypeTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // According to the standard, the length *must* be 20
            //
            if (192 != len)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->tlv.type = TLV_TYPE_DEVICE_IDENTIFICATION;

            _EnB(&p,  ret->friendly_name,      64);
            _EnB(&p,  ret->manufacturer_name,  64);
            _EnB(&p,  ret->manufacturer_model, 64);

            return &ret->tlv;
        }

        case TLV_TYPE_CONTROL_URL:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.23"

            struct controlUrlTypeTLV  *ret;

            uint16_t len;

            ret = (struct controlUrlTypeTLV *)memalloc(sizeof(struct controlUrlTypeTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            ret->tlv.type       = TLV_TYPE_CONTROL_URL;

            if (len>0)
            {
                ret->url            = (char *)memalloc(len);
                _EnB(&p, ret->url, len);
            }

            return &ret->tlv;
        }

        case TLV_TYPE_IPV4:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.24"

            struct ipv4TypeTLV  *ret;

            uint16_t len;
            uint8_t  i, j;

            ret = (struct ipv4TypeTLV *)memalloc(sizeof(struct ipv4TypeTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            ret->tlv.type = TLV_TYPE_IPV4;

            if (0 == len)
            {
#ifdef FIX_BROKEN_TLVS
                // Malformed packet. Even if there are NO bridging tuples, the
                // Malformed packet. Even if there are NO entris, the length
                // should be "1" (which is the length of the next field, that
                // would containing a "zero", indicating the number of
                // entries).
                // *However*, because at least one other implementation sets
                // the 'length' to zero to indicate "no entries", we will also
                // accept this type of "malformed" packet.
                //
                ret->ipv4_interfaces_nr = 0;
                return &ret->tlv;
#else
                free(ret);
                return NULL;
#endif
            }

            _E1B(&p, &ret->ipv4_interfaces_nr);

            if (ret->ipv4_interfaces_nr > 0)
            {
                ret->ipv4_interfaces = (struct _ipv4InterfaceEntries *)memalloc(sizeof(struct _ipv4InterfaceEntries) * ret->ipv4_interfaces_nr);

                for (i=0; i < ret->ipv4_interfaces_nr; i++)
                {
                    _EnB(&p,  ret->ipv4_interfaces[i].mac_address, 6);
                    _E1B(&p, &ret->ipv4_interfaces[i].ipv4_nr);

                    if (ret->ipv4_interfaces[i].ipv4_nr > 0)
                    {
                        ret->ipv4_interfaces[i].ipv4 = (struct _ipv4Entries *)memalloc(sizeof(struct _ipv4Entries) * ret->ipv4_interfaces[i].ipv4_nr);

                        for (j=0; j < ret->ipv4_interfaces[i].ipv4_nr; j++)
                        {
                            _E1B(&p, &ret->ipv4_interfaces[i].ipv4[j].type);
                            _EnB(&p,  ret->ipv4_interfaces[i].ipv4[j].ipv4_address,     4);
                            _EnB(&p,  ret->ipv4_interfaces[i].ipv4[j].ipv4_dhcp_server, 4);
                        }
                    }
                }
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                for (i=0; i < ret->ipv4_interfaces_nr; i++)
                {
                    if (ret->ipv4_interfaces[i].ipv4_nr > 0)
                    {
                        free(ret->ipv4_interfaces[i].ipv4);
                    }
                }
                free(ret->ipv4_interfaces);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        case TLV_TYPE_IPV6:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.25"

            struct ipv6TypeTLV  *ret;

            uint16_t len;
            uint8_t  i, j;

            ret = (struct ipv6TypeTLV *)memalloc(sizeof(struct ipv6TypeTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            ret->tlv.type = TLV_TYPE_IPV6;

            if (0 == len)
            {
#ifdef FIX_BROKEN_TLVS
                // Malformed packet. Even if there are NO bridging tuples, the
                // Malformed packet. Even if there are NO entris, the length
                // should be "1" (which is the length of the next field, that
                // would containing a "zero", indicating the number of
                // entries).
                // *However*, because at least one other implementation sets
                // the 'length' to zero to indicate "no entries", we will also
                // accept this type of "malformed" packet.
                //
                ret->ipv6_interfaces_nr = 0;
                return &ret->tlv;
#else
                free(ret);
                return NULL;
#endif
            }

            _E1B(&p, &ret->ipv6_interfaces_nr);

            if (ret->ipv6_interfaces_nr > 0)
            {
                ret->ipv6_interfaces = (struct _ipv6InterfaceEntries *)memalloc(sizeof(struct _ipv6InterfaceEntries) * ret->ipv6_interfaces_nr);

                for (i=0; i < ret->ipv6_interfaces_nr; i++)
                {
                    _EnB(&p,  ret->ipv6_interfaces[i].mac_address,              6);
                    _EnB(&p,  ret->ipv6_interfaces[i].ipv6_link_local_address, 16);
                    _E1B(&p, &ret->ipv6_interfaces[i].ipv6_nr);

                    if (ret->ipv6_interfaces[i].ipv6_nr > 0)
                    {
                        ret->ipv6_interfaces[i].ipv6 = (struct _ipv6Entries *)memalloc(sizeof(struct _ipv6Entries) * ret->ipv6_interfaces[i].ipv6_nr);

                        for (j=0; j < ret->ipv6_interfaces[i].ipv6_nr; j++)
                        {
                            _E1B(&p, &ret->ipv6_interfaces[i].ipv6[j].type);
                            _EnB(&p,  ret->ipv6_interfaces[i].ipv6[j].ipv6_address,        16);
                            _EnB(&p,  ret->ipv6_interfaces[i].ipv6[j].ipv6_address_origin, 16);
                        }
                    }
                }
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                for (i=0; i < ret->ipv6_interfaces_nr; i++)
                {
                    if (ret->ipv6_interfaces[i].ipv6_nr > 0)
                    {
                        free(ret->ipv6_interfaces[i].ipv6);
                    }
                }
                free(ret->ipv6_interfaces);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        case TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.26"

            struct pushButtonGenericPhyEventNotificationTLV  *ret;

            uint16_t len;
            uint8_t  i;

            ret = (struct pushButtonGenericPhyEventNotificationTLV *)memalloc(sizeof(struct pushButtonGenericPhyEventNotificationTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            ret->tlv.type = TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION;

            if (0 == len)
            {
#ifdef FIX_BROKEN_TLVS
                // Malformed packet. Even if there are NO bridging tuples, the
                // Malformed packet. Even if there are NO interfaces, the length
                // should be "1" (which is the length of the next field, that
                // would containing a "zero", indicating the number of
                // interfaces).
                // *However*, because at least one other implementation sets
                // the 'length' to zero to indicate "no interfaces", we will
                // also accept this type of "malformed" packet.
                //
                ret->local_interfaces_nr = 0;
                return &ret->tlv;
#else
                free(ret);
                return NULL;
#endif
            }

            _E1B(&p, &ret->local_interfaces_nr);

            if (ret->local_interfaces_nr > 0)
            {
                ret->local_interfaces = (struct _genericPhyCommonData *)memalloc(sizeof(struct _genericPhyCommonData) * ret->local_interfaces_nr);

                for (i=0; i < ret->local_interfaces_nr; i++)
                {
                    _EnB(&p,  ret->local_interfaces[i].oui, 3);
                    _E1B(&p, &ret->local_interfaces[i].variant_index);
                    _E1B(&p, &ret->local_interfaces[i].media_specific_bytes_nr);

                    if (ret->local_interfaces[i].media_specific_bytes_nr > 0)
                    {
                        ret->local_interfaces[i].media_specific_bytes = (uint8_t *)memalloc(ret->local_interfaces[i].media_specific_bytes_nr);
                        _EnB(&p, ret->local_interfaces[i].media_specific_bytes, ret->local_interfaces[i].media_specific_bytes_nr);
                    }
                }
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                for (i=0; i < ret->local_interfaces_nr; i++)
                {
                    if (ret->local_interfaces[i].media_specific_bytes_nr > 0)
                    {
                        free(ret->local_interfaces[i].media_specific_bytes);
                    }
                }
                free(ret->local_interfaces);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        case TLV_TYPE_1905_PROFILE_VERSION:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.27"

            struct x1905ProfileVersionTLV  *ret;

            uint16_t len;

            ret = (struct x1905ProfileVersionTLV *)memalloc(sizeof(struct x1905ProfileVersionTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // According to the standard, the length *must* be 1
            //
            if (1 != len)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->tlv.type = TLV_TYPE_1905_PROFILE_VERSION;

            _E1B(&p, &ret->profile);

            return &ret->tlv;
        }

        case TLV_TYPE_POWER_OFF_INTERFACE:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.28"

            struct powerOffInterfaceTLV  *ret;

            uint16_t len;
            uint8_t  i;

            ret = (struct powerOffInterfaceTLV *)memalloc(sizeof(struct powerOffInterfaceTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            ret->tlv.type = TLV_TYPE_POWER_OFF_INTERFACE;

            if (0 == len)
            {
#ifdef FIX_BROKEN_TLVS
                // Malformed packet. Even if there are NO interfaces, the length
                // should be "1" (which is the length of the next field, that
                // would contain a "zero", indicating the number of interfaces)
                // *However*, because at least one other implementation sets
                // the 'length' to zero to indicate "no interfaces", we will
                // also accept this type of "malformed" packet.
                //
                ret->power_off_interfaces_nr = 0;
                return &ret->tlv;
#else
                free(ret);
                return NULL;
#endif
            }

            _E1B(&p, &ret->power_off_interfaces_nr);

            if (ret->power_off_interfaces_nr > 0)
            {
                ret->power_off_interfaces = (struct _powerOffInterfaceEntries *)memalloc(sizeof(struct _powerOffInterfaceEntries) * ret->power_off_interfaces_nr);

                for (i=0; i < ret->power_off_interfaces_nr; i++)
                {
                    _EnB(&p,  ret->power_off_interfaces[i].interface_address, 6);
                    _E2B(&p, &ret->power_off_interfaces[i].media_type);
                    _EnB(&p,  ret->power_off_interfaces[i].generic_phy_common_data.oui, 3);
                    _E1B(&p, &ret->power_off_interfaces[i].generic_phy_common_data.variant_index);
                    _E1B(&p, &ret->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr);

                    if (ret->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr > 0)
                    {
                        ret->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes = (uint8_t *)memalloc(ret->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr);
                        _EnB(&p, ret->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes, ret->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr);
                    }
                }
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                for (i=0; i < ret->power_off_interfaces_nr; i++)
                {
                    if (ret->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr > 0)
                    {
                        free(ret->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes);
                    }
                }
                free(ret->power_off_interfaces);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        case TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.29"

            struct interfacePowerChangeInformationTLV  *ret;

            uint16_t len;
            uint8_t  i;

            ret = (struct interfacePowerChangeInformationTLV *)memalloc(sizeof(struct interfacePowerChangeInformationTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            ret->tlv.type = TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION;

            if (0 == len)
            {
#ifdef FIX_BROKEN_TLVS
                // Malformed packet. Even if there are NO bridging tuples, the
                // Malformed packet. Even if there are NO interfaces, the length
                // should be "1" (which is the length of the next field, that
                // would containing a "zero", indicating the number of
                // interfaces).
                // *However*, because at least one other implementation sets
                // the 'length' to zero to indicate "no interfaces", we will
                // also accept this type of "malformed" packet.
                //
                ret->power_change_interfaces_nr = 0;
                return &ret->tlv;
#else
                free(ret);
                return NULL;
#endif
            }

            _E1B(&p, &ret->power_change_interfaces_nr);

            if (ret->power_change_interfaces_nr > 0)
            {
                ret->power_change_interfaces = (struct _powerChangeInformationEntries *)memalloc(sizeof(struct _powerChangeInformationEntries) * ret->power_change_interfaces_nr);

                for (i=0; i < ret->power_change_interfaces_nr; i++)
                {
                    _EnB(&p,  ret->power_change_interfaces[i].interface_address, 6);
                    _E1B(&p, &ret->power_change_interfaces[i].requested_power_state);
                }
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                free(ret->power_change_interfaces);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        case TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.30"

            struct interfacePowerChangeStatusTLV  *ret;

            uint16_t len;
            uint8_t  i;

            ret = (struct interfacePowerChangeStatusTLV *)memalloc(sizeof(struct interfacePowerChangeStatusTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            ret->tlv.type = TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS;

            if (0 == len)
            {
#ifdef FIX_BROKEN_TLVS
                // Malformed packet. Even if there are NO bridging tuples, the
                // Malformed packet. Even if there are NO interfaces, the length
                // should be "1" (which is the length of the next field, that
                // would containing a "zero", indicating the number of
                // interfaces).
                // *However*, because at least one other implementation sets
                // the 'length' to zero to indicate "no interfaces", we will
                // also accept this type of "malformed" packet.
                //
                ret->power_change_interfaces_nr = 0;
                return &ret->tlv;
#else
                free(ret);
                return NULL;
#endif
            }

            _E1B(&p, &ret->power_change_interfaces_nr);

            if (ret->power_change_interfaces_nr > 0)
            {
                ret->power_change_interfaces = (struct _powerChangeStatusEntries *)memalloc(sizeof(struct _powerChangeStatusEntries) * ret->power_change_interfaces_nr);

                for (i=0; i < ret->power_change_interfaces_nr; i++)
                {
                    _EnB(&p,  ret->power_change_interfaces[i].interface_address, 6);
                    _E1B(&p, &ret->power_change_interfaces[i].result);
                }
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                if (ret->power_change_interfaces_nr > 0)
                {
                    free(ret->power_change_interfaces);
                }
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        case TLV_TYPE_L2_NEIGHBOR_DEVICE:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.31"

            struct l2NeighborDeviceTLV  *ret;

            uint16_t len;
            uint8_t  i, j, k;

            ret = (struct l2NeighborDeviceTLV *)memalloc(sizeof(struct l2NeighborDeviceTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            ret->tlv.type = TLV_TYPE_L2_NEIGHBOR_DEVICE;

            if (0 == len)
            {
#ifdef FIX_BROKEN_TLVS
                // Malformed packet. Even if there are NO bridging tuples, the
                // Malformed packet. Even if there are NO interfaces, the length
                // should be "1" (which is the length of the next field, that
                // would containing a "zero", indicating the number of
                // interfaces).
                // *However*, because at least one other implementation sets
                // the 'length' to zero to indicate "no interfaces", we will
                // also accept this type of "malformed" packet.
                //
                ret->local_interfaces_nr = 0;
                return &ret->tlv;
#else
                free(ret);
                return NULL;
#endif
            }

            _E1B(&p, &ret->local_interfaces_nr);

            if (ret->local_interfaces_nr > 0)
            {
                ret->local_interfaces = (struct _l2InterfacesEntries *)memalloc(sizeof(struct _l2InterfacesEntries) * ret->local_interfaces_nr);

                for (i=0; i < ret->local_interfaces_nr; i++)
                {
                    _EnB(&p,  ret->local_interfaces[i].local_mac_address, 6);
                    _E2B(&p, &ret->local_interfaces[i].l2_neighbors_nr);

                    if (ret->local_interfaces[i].l2_neighbors_nr > 0)
                    {
                        ret->local_interfaces[i].l2_neighbors = (struct _l2NeighborsEntries *)memalloc(sizeof(struct _l2NeighborsEntries) * ret->local_interfaces[i].l2_neighbors_nr);

                        for (j=0; j < ret->local_interfaces[i].l2_neighbors_nr; j++)
                        {
                            _EnB(&p,  ret->local_interfaces[i].l2_neighbors[j].l2_neighbor_mac_address, 6);
                            _E2B(&p, &ret->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr);

                            if (ret->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr > 0)
                            {
                                ret->local_interfaces[i].l2_neighbors[j].behind_mac_addresses = (uint8_t (*)[6])memalloc(sizeof(uint8_t[6]) * ret->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr);

                                for (k=0; k < ret->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr; k++)
                                {
                                    _EnB(&p,  ret->local_interfaces[i].l2_neighbors[j].behind_mac_addresses[k], 6);
                                }
                            }
                        }
                    }
                }
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                for (i=0; i < ret->local_interfaces_nr; i++)
                {
                    for (j=0; j < ret->local_interfaces[i].l2_neighbors_nr; j++)
                    {
                        free(ret->local_interfaces[i].l2_neighbors[j].behind_mac_addresses);
                    }
                    free(ret->local_interfaces[i].l2_neighbors);
                }
                free(ret->local_interfaces);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        default:
        {
            uint16_t len;
            DECLARE_HLIST_HEAD(dummy);
            bool parsed;
            p = packet_stream + 1;
            _E2B(&p, &len);
            parsed = tlv_parse(tlv_1905_defs, &dummy, packet_stream, len + 3);
            if (!parsed)
            {
                // Ignore
                //
                return NULL;
            }
            else
            {
                struct tlv *tlv = container_of(dummy.next, struct tlv, s.h.l);
                hlist_head_init(&tlv->s.h.l);
                return tlv;
            }
        }

    }

    // This code cannot be reached
    //
    return NULL;
}


uint8_t *forge_1905_TLV_from_structure(const struct tlv *tlv, uint16_t *len)
{
    if (NULL == tlv)
    {
        return NULL;
    }

    // The first byte of any of the valid structures is always the "tlv.type"
    // field.
    //
    switch (tlv->type)
    {
        case TLV_TYPE_DEVICE_INFORMATION_TYPE:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.5"

            uint8_t *ret, *p;
            struct deviceInformationTypeTLV *m;

            uint16_t tlv_length;

            uint8_t i;

            m = (struct deviceInformationTypeTLV *)tlv;

            tlv_length = 7;  // AL MAC (6 bytes) + number of ifaces (1 bytes)
            for (i=0; i<m->local_interfaces_nr; i++)
            {
                tlv_length += 6 + 2 + 1;  // MAC (6 bytes) + media type (2
                                          // bytes) + number of octets (1 byte)

                tlv_length += m->local_interfaces[i].media_specific_data_size;
            }
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,            &p);
            _I2B(&tlv_length,             &p);
            _InB( m->al_mac_address,      &p, 6);
            _I1B(&m->local_interfaces_nr, &p);

            for (i=0; i<m->local_interfaces_nr; i++)
            {
                _InB( m->local_interfaces[i].mac_address,              &p, 6);
                _I2B(&m->local_interfaces[i].media_type,               &p);
                _I1B(&m->local_interfaces[i].media_specific_data_size, &p);

                if (
                     (MEDIA_TYPE_IEEE_802_11B_2_4_GHZ == m->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11G_2_4_GHZ == m->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11A_5_GHZ   == m->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_2_4_GHZ == m->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_5_GHZ   == m->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AC_5_GHZ  == m->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AD_60_GHZ == m->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AF_GHZ    == m->local_interfaces[i].media_type)
                   )
                {
                    uint8_t aux;

                    if (10 != m->local_interfaces[i].media_specific_data_size)
                    {
                        // Malformed structure
                        //
                        free(ret);
                        return NULL;
                    }

                    _InB(m->local_interfaces[i].media_specific_data.ieee80211.network_membership,                   &p, 6);
                    aux = m->local_interfaces[i].media_specific_data.ieee80211.role << 4;
                    _I1B(&aux,                                                                                      &p);
                    _I1B(&m->local_interfaces[i].media_specific_data.ieee80211.ap_channel_band,                     &p);
                    _I1B(&m->local_interfaces[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_1, &p);
                    _I1B(&m->local_interfaces[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_2, &p);

                }
                else if (
                          (MEDIA_TYPE_IEEE_1901_WAVELET == m->local_interfaces[i].media_type) ||
                          (MEDIA_TYPE_IEEE_1901_FFT     == m->local_interfaces[i].media_type)
                        )
                {
                    if (7 != m->local_interfaces[i].media_specific_data_size)
                    {
                        // Malformed structure
                        //
                        free(ret);
                        return NULL;
                    }
                    _InB(m->local_interfaces[i].media_specific_data.ieee1901.network_identifier, &p, 7);
                }
                else
                {
                    if (0 != m->local_interfaces[i].media_specific_data_size)
                    {
                        // Malformed structure
                        //
                        free(ret);
                        return NULL;
                    }
                }
            }

            return ret;
        }

        case TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.6"

            uint8_t *ret, *p;
            struct deviceBridgingCapabilityTLV *m;

            uint16_t tlv_length;

            uint8_t i, j;

            m = (struct deviceBridgingCapabilityTLV *)tlv;

            tlv_length = 1;  // number of bridging tuples (1 bytes)
            for (i=0; i<m->bridging_tuples_nr; i++)
            {
                tlv_length += 1;  // number of MAC addresses (1 bytes)
                tlv_length += 6 * m->bridging_tuples[i].bridging_tuple_macs_nr;
            }
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,           &p);
            _I2B(&tlv_length,            &p);
            _I1B(&m->bridging_tuples_nr, &p);

            for (i=0; i<m->bridging_tuples_nr; i++)
            {
                _I1B(&m->bridging_tuples[i].bridging_tuple_macs_nr, &p);

                for (j=0; j<m->bridging_tuples[i].bridging_tuple_macs_nr; j++)
                {
                    _InB(m->bridging_tuples[i].bridging_tuple_macs[j].mac_address,  &p, 6);
                }
            }

            return ret;
        }

        case TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.8"

            uint8_t *ret, *p;
            struct non1905NeighborDeviceListTLV *m;

            uint16_t tlv_length;

            uint8_t i;

            m = (struct non1905NeighborDeviceListTLV *)tlv;

            tlv_length = 6 + 6*m->non_1905_neighbors_nr;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,            &p);
            _I2B(&tlv_length,             &p);
            _InB( m->local_mac_address,   &p, 6);

            for (i=0; i<m->non_1905_neighbors_nr; i++)
            {
                _InB(m->non_1905_neighbors[i].mac_address, &p, 6);
            }

            return ret;
        }

        case TLV_TYPE_NEIGHBOR_DEVICE_LIST:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.9"

            uint8_t *ret, *p;
            struct neighborDeviceListTLV *m;

            uint16_t tlv_length;

            uint8_t i;

            m = (struct neighborDeviceListTLV *)tlv;

            tlv_length = 6 + 7*m->neighbors_nr;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,            &p);
            _I2B(&tlv_length,             &p);
            _InB( m->local_mac_address,   &p, 6);

            for (i=0; i<m->neighbors_nr; i++)
            {
                uint8_t aux;

                _InB(m->neighbors[i].mac_address, &p, 6);

                if (1 == m->neighbors[i].bridge_flag)
                {
                    aux = 1 << 7;
                    _I1B(&aux, &p);
                }
                else
                {
                    aux = 0;
                    _I1B(&aux, &p);
                }
            }

            return ret;
        }

        case TLV_TYPE_TRANSMITTER_LINK_METRIC:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.11"

            uint8_t *ret, *p;
            struct transmitterLinkMetricTLV *m;

            uint16_t tlv_length;

            uint8_t i;

            m = (struct transmitterLinkMetricTLV *)tlv;

            tlv_length = 12 + 29*m->transmitter_link_metrics_nr;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,            &p);
            _I2B(&tlv_length,             &p);
            _InB( m->local_al_address,    &p, 6);
            _InB( m->neighbor_al_address, &p, 6);

            for (i=0; i<m->transmitter_link_metrics_nr; i++)
            {
                _InB( m->transmitter_link_metrics[i].local_interface_address,    &p, 6);
                _InB( m->transmitter_link_metrics[i].neighbor_interface_address, &p, 6);
                _I2B(&m->transmitter_link_metrics[i].intf_type,                  &p);
                _I1B(&m->transmitter_link_metrics[i].bridge_flag,                &p);
                _I4B(&m->transmitter_link_metrics[i].packet_errors,              &p);
                _I4B(&m->transmitter_link_metrics[i].transmitted_packets,        &p);
                _I2B(&m->transmitter_link_metrics[i].mac_throughput_capacity,    &p);
                _I2B(&m->transmitter_link_metrics[i].link_availability,          &p);
                _I2B(&m->transmitter_link_metrics[i].phy_rate,                   &p);
            }

            return ret;
        }

        case TLV_TYPE_RECEIVER_LINK_METRIC:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.12"

            uint8_t *ret, *p;
            struct receiverLinkMetricTLV *m;

            uint16_t tlv_length;

            uint8_t i;

            m = (struct receiverLinkMetricTLV *)tlv;

            tlv_length = 12 + 23*m->receiver_link_metrics_nr;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,            &p);
            _I2B(&tlv_length,             &p);
            _InB( m->local_al_address,    &p, 6);
            _InB( m->neighbor_al_address, &p, 6);

            for (i=0; i<m->receiver_link_metrics_nr; i++)
            {
                _InB( m->receiver_link_metrics[i].local_interface_address,    &p, 6);
                _InB( m->receiver_link_metrics[i].neighbor_interface_address, &p, 6);
                _I2B(&m->receiver_link_metrics[i].intf_type,                  &p);
                _I4B(&m->receiver_link_metrics[i].packet_errors,              &p);
                _I4B(&m->receiver_link_metrics[i].packets_received,           &p);
                _I1B(&m->receiver_link_metrics[i].rssi,                       &p);
            }

            return ret;
        }

        case TLV_TYPE_LINK_METRIC_RESULT_CODE:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.13"

            uint8_t *ret, *p;
            struct linkMetricResultCodeTLV *m;

            uint16_t tlv_length;

            m = (struct linkMetricResultCodeTLV *)tlv;

            tlv_length = 1;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,     &p);
            _I2B(&tlv_length,      &p);

            if (m->result_code != LINK_METRIC_RESULT_CODE_TLV_INVALID_NEIGHBOR)
            {
                // Malformed structure
                //
                free(ret);
                return NULL;
            }

            _I1B(&m->result_code,  &p);

            return ret;
        }

        case TLV_TYPE_SEARCHED_ROLE:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.14"

            uint8_t *ret, *p;
            struct searchedRoleTLV *m;

            uint16_t tlv_length;

            m = (struct searchedRoleTLV *)tlv;

            tlv_length = 1;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,     &p);
            _I2B(&tlv_length,      &p);

            if (m->role != IEEE80211_ROLE_REGISTRAR)
            {
                // Malformed structure
                //
                free(ret);
                return NULL;
            }

            _I1B(&m->role,  &p);

            return ret;
        }

        case TLV_TYPE_AUTOCONFIG_FREQ_BAND:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.14"

            uint8_t *ret, *p;
            struct autoconfigFreqBandTLV *m;

            uint16_t tlv_length;

            m = (struct autoconfigFreqBandTLV *)tlv;

            tlv_length = 1;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,     &p);
            _I2B(&tlv_length,      &p);

            if (
                 (m->freq_band != IEEE80211_FREQUENCY_BAND_2_4_GHZ) &&
                 (m->freq_band != IEEE80211_FREQUENCY_BAND_5_GHZ)   &&
                 (m->freq_band != IEEE80211_FREQUENCY_BAND_60_GHZ)
               )
            {
                // Malformed structure
                //
                free(ret);
                return NULL;
            }

            _I1B(&m->freq_band,  &p);

            return ret;
        }

        case TLV_TYPE_SUPPORTED_ROLE:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.16"

            uint8_t *ret, *p;
            struct supportedRoleTLV *m;

            uint16_t tlv_length;

            m = (struct supportedRoleTLV *)tlv;

            tlv_length = 1;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,     &p);
            _I2B(&tlv_length,      &p);

            if (m->role != IEEE80211_ROLE_REGISTRAR)
            {
                // Malformed structure
                //
                free(ret);
                return NULL;
            }

            _I1B(&m->role,  &p);

            return ret;
        }

        case TLV_TYPE_SUPPORTED_FREQ_BAND:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.17"

            uint8_t *ret, *p;
            struct supportedFreqBandTLV *m;

            uint16_t tlv_length;

            m = (struct supportedFreqBandTLV *)tlv;

            tlv_length = 1;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,     &p);
            _I2B(&tlv_length,      &p);

            if (
                 (m->freq_band != IEEE80211_FREQUENCY_BAND_2_4_GHZ) &&
                 (m->freq_band != IEEE80211_FREQUENCY_BAND_5_GHZ)   &&
                 (m->freq_band != IEEE80211_FREQUENCY_BAND_60_GHZ)
               )
            {
                // Malformed structure
                //
                free(ret);
                return NULL;
            }

            _I1B(&m->freq_band,  &p);

            return ret;
        }

        case TLV_TYPE_WSC:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.18"

            uint8_t *ret, *p;
            struct wscTLV *m;

            uint16_t tlv_length;

            m = (struct wscTLV *)tlv;

            tlv_length = m->wsc_frame_size;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,     &p);
            _I2B(&tlv_length,      &p);
            _InB( m->wsc_frame,    &p, m->wsc_frame_size);

            return ret;
        }

        case TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.19"

            uint8_t *ret, *p;
            struct pushButtonEventNotificationTLV *m;

            uint16_t tlv_length;

            uint8_t i;

            m = (struct pushButtonEventNotificationTLV *)tlv;

            tlv_length = 1;  // number of media types (1 byte)
            for (i=0; i<m->media_types_nr; i++)
            {
                tlv_length += 2 + 1;  //  media type (2 bytes) +
                                      //  number of octets (1 byte)

                tlv_length += m->media_types[i].media_specific_data_size;
            }
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,        &p);
            _I2B(&tlv_length,         &p);
            _I1B(&m->media_types_nr,  &p);

            for (i=0; i<m->media_types_nr; i++)
            {
                _I2B(&m->media_types[i].media_type,               &p);
                _I1B(&m->media_types[i].media_specific_data_size, &p);

                if (
                     (MEDIA_TYPE_IEEE_802_11B_2_4_GHZ == m->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11G_2_4_GHZ == m->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11A_5_GHZ   == m->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_2_4_GHZ == m->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_5_GHZ   == m->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AC_5_GHZ  == m->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AD_60_GHZ == m->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AF_GHZ    == m->media_types[i].media_type)
                   )
                {
                    uint8_t aux;

                    if (10 != m->media_types[i].media_specific_data_size)
                    {
                        // Malformed structure
                        //
                        free(ret);
                        return NULL;
                    }

                    _InB(m->media_types[i].media_specific_data.ieee80211.network_membership,                   &p, 6);
                    aux = m->media_types[i].media_specific_data.ieee80211.role << 4;
                    _I1B(&aux,                                                                                 &p);
                    _I1B(&m->media_types[i].media_specific_data.ieee80211.ap_channel_band,                     &p);
                    _I1B(&m->media_types[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_1, &p);
                    _I1B(&m->media_types[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_2, &p);

                }
                else if (
                          (MEDIA_TYPE_IEEE_1901_WAVELET == m->media_types[i].media_type) ||
                          (MEDIA_TYPE_IEEE_1901_FFT     == m->media_types[i].media_type)
                        )
                {
                    if (7 != m->media_types[i].media_specific_data_size)
                    {
                        // Malformed structure
                        //
                        free(ret);
                        return NULL;
                    }
                    _InB(m->media_types[i].media_specific_data.ieee1901.network_identifier, &p, 7);
                }
                else
                {
                    if (0 != m->media_types[i].media_specific_data_size)
                    {
                        // Malformed structure
                        //
                        free(ret);
                        return NULL;
                    }
                }
            }

            return ret;
        }

        case TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.20"

            uint8_t *ret, *p;
            struct pushButtonJoinNotificationTLV *m;

            uint16_t tlv_length;

            m = (struct pushButtonJoinNotificationTLV *)tlv;

            tlv_length = 20;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,            &p);
            _I2B(&tlv_length,             &p);
            _InB( m->al_mac_address,      &p, 6);
            _I2B(&m->message_identifier,  &p);
            _InB( m->mac_address,         &p, 6);
            _InB( m->new_mac_address,     &p, 6);

            return ret;
        }

        case TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.21"

            uint8_t *ret, *p;
            struct genericPhyDeviceInformationTypeTLV *m;

            uint16_t tlv_length;

            uint8_t i;

            m = (struct genericPhyDeviceInformationTypeTLV *)tlv;

            tlv_length  = 6;  // AL MAC address (6 bytes)
            tlv_length += 1;  // number of local interfaces (1 bytes)
            for (i=0; i<m->local_interfaces_nr; i++)
            {
                tlv_length += 6;  // local interface MAC address (6 bytes)
                tlv_length += 3;  // OUI (2 bytes)
                tlv_length += 1;  // variant_index (1 byte)
                tlv_length += 32; // variant_name (32 bytes)
                tlv_length += 1;  // URL len (1 byte)
                tlv_length += 1;  // media specific bytes number (1 bytes)
                tlv_length += m->local_interfaces[i].generic_phy_description_xml_url_len;
                                  // URL bytes
                tlv_length += m->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr;
                                  // media specific bytes
            }
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,            &p);
            _I2B(&tlv_length,             &p);
            _InB( m->al_mac_address,      &p,  6);
            _I1B(&m->local_interfaces_nr, &p);

            for (i=0; i<m->local_interfaces_nr; i++)
            {
                _InB( m->local_interfaces[i].local_interface_address,                         &p, 6);
                _InB( m->local_interfaces[i].generic_phy_common_data.oui,                     &p, 3);
                _I1B(&m->local_interfaces[i].generic_phy_common_data.variant_index,           &p);
                _InB( m->local_interfaces[i].variant_name,                                    &p, 32);
                _I1B(&m->local_interfaces[i].generic_phy_description_xml_url_len,             &p);
                _I1B(&m->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr, &p);
                if (m->local_interfaces[i].generic_phy_description_xml_url_len > 0)
                {
                    _InB( m->local_interfaces[i].generic_phy_description_xml_url, &p, m->local_interfaces[i].generic_phy_description_xml_url_len);
                }
                if (m->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr > 0)
                {
                    _InB( m->local_interfaces[i].generic_phy_common_data.media_specific_bytes, &p, m->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr);
                }
            }

            return ret;
        }

        case TLV_TYPE_DEVICE_IDENTIFICATION:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.21"

            uint8_t *ret, *p;
            struct deviceIdentificationTypeTLV *m;

            uint16_t tlv_length;

            m = (struct deviceIdentificationTypeTLV *)tlv;

            tlv_length = 192;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,           &p);
            _I2B(&tlv_length,            &p);
            _InB( m->friendly_name,      &p, 64);
            _InB( m->manufacturer_name,  &p, 64);
            _InB( m->manufacturer_model, &p, 64);

            return ret;
        }

        case TLV_TYPE_CONTROL_URL:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.23"

            uint8_t *ret, *p;
            struct controlUrlTypeTLV *m;

            uint16_t tlv_length;

            m = (struct controlUrlTypeTLV *)tlv;

            tlv_length = strlen(m->url)+1;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,     &p);
            _I2B(&tlv_length,      &p);
            _InB( m->url,          &p, tlv_length);

            return ret;
        }

        case TLV_TYPE_IPV4:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.24"

            uint8_t *ret, *p;
            struct ipv4TypeTLV *m;

            uint16_t tlv_length;

            uint8_t i, j;

            m = (struct ipv4TypeTLV *)tlv;

            tlv_length = 1;  // number of entries (1 bytes)
            for (i=0; i<m->ipv4_interfaces_nr; i++)
            {
                tlv_length += 6;  // interface MAC address (6 bytes)
                tlv_length += 1;  // number of IPv4s (1 bytes)
                tlv_length += (1+4+4) * m->ipv4_interfaces[i].ipv4_nr;
            }
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,           &p);
            _I2B(&tlv_length,            &p);
            _I1B(&m->ipv4_interfaces_nr, &p);

            for (i=0; i<m->ipv4_interfaces_nr; i++)
            {
                _InB( m->ipv4_interfaces[i].mac_address, &p, 6);
                _I1B(&m->ipv4_interfaces[i].ipv4_nr,     &p);

                for (j=0; j<m->ipv4_interfaces[i].ipv4_nr; j++)
                {
                    _I1B(&m->ipv4_interfaces[i].ipv4[j].type,             &p);
                    _InB( m->ipv4_interfaces[i].ipv4[j].ipv4_address,     &p, 4);
                    _InB( m->ipv4_interfaces[i].ipv4[j].ipv4_dhcp_server, &p, 4);
                }
            }

            return ret;
        }

        case TLV_TYPE_IPV6:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.25"

            uint8_t *ret, *p;
            struct ipv6TypeTLV *m;

            uint16_t tlv_length;

            uint8_t i, j;

            m = (struct ipv6TypeTLV *)tlv;

            tlv_length = 1;  // number of entries (1 bytes)
            for (i=0; i<m->ipv6_interfaces_nr; i++)
            {
                tlv_length += 6;  // interface MAC address (6 bytes)
                tlv_length += 16; // interface ipv6 local link address (16 bytes)
                tlv_length += 1;  // number of ipv6s (1 bytes)
                tlv_length += (1+16+16) * m->ipv6_interfaces[i].ipv6_nr;
            }
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,           &p);
            _I2B(&tlv_length,            &p);
            _I1B(&m->ipv6_interfaces_nr, &p);

            for (i=0; i<m->ipv6_interfaces_nr; i++)
            {
                _InB( m->ipv6_interfaces[i].mac_address,             &p,  6);
                _InB( m->ipv6_interfaces[i].ipv6_link_local_address, &p, 16);
                _I1B(&m->ipv6_interfaces[i].ipv6_nr,                 &p);

                for (j=0; j<m->ipv6_interfaces[i].ipv6_nr; j++)
                {
                    _I1B(&m->ipv6_interfaces[i].ipv6[j].type,                &p);
                    _InB( m->ipv6_interfaces[i].ipv6[j].ipv6_address,        &p, 16);
                    _InB( m->ipv6_interfaces[i].ipv6[j].ipv6_address_origin, &p, 16);
                }
            }

            return ret;
        }

        case TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.26"

            uint8_t *ret, *p;
            struct pushButtonGenericPhyEventNotificationTLV *m;

            uint16_t tlv_length;

            uint8_t i;

            m = (struct pushButtonGenericPhyEventNotificationTLV *)tlv;

            tlv_length = 1;  // number of local interfaces (1 bytes)
            for (i=0; i<m->local_interfaces_nr; i++)
            {
                tlv_length += 3;  // OUI (2 bytes)
                tlv_length += 1;  // variant_index (1 byte)
                tlv_length += 1;  // media specific bytes number (1 bytes)
                tlv_length += m->local_interfaces[i].media_specific_bytes_nr;
                                  // media specific bytes
            }
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,                &p);
            _I2B(&tlv_length,                 &p);
            _I1B(&m->local_interfaces_nr, &p);

            for (i=0; i<m->local_interfaces_nr; i++)
            {
                _InB( m->local_interfaces[i].oui,                     &p, 3);
                _I1B(&m->local_interfaces[i].variant_index,           &p);
                _I1B(&m->local_interfaces[i].media_specific_bytes_nr, &p);
                if (m->local_interfaces[i].media_specific_bytes_nr > 0)
                {
                    _InB( m->local_interfaces[i].media_specific_bytes, &p, m->local_interfaces[i].media_specific_bytes_nr);
                }
            }

            return ret;
        }

        case TLV_TYPE_1905_PROFILE_VERSION:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.27"

            uint8_t *ret, *p;
            struct x1905ProfileVersionTLV *m;

            uint16_t tlv_length;

            m = (struct x1905ProfileVersionTLV *)tlv;

            tlv_length = 1;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,     &p);
            _I2B(&tlv_length,      &p);

            if (
                 m->profile != PROFILE_1905_1 &&
                 m->profile != PROFILE_1905_1A
               )
            {
                // Malformed structure
                //
                free(ret);
                return NULL;
            }

            _I1B(&m->profile,  &p);

            return ret;
        }

        case TLV_TYPE_POWER_OFF_INTERFACE:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.28"

            uint8_t *ret, *p;
            struct powerOffInterfaceTLV *m;

            uint16_t tlv_length;

            uint8_t i;

            m = (struct powerOffInterfaceTLV *)tlv;

            tlv_length = 1;  // number of power off interfaces (1 bytes)
            for (i=0; i<m->power_off_interfaces_nr; i++)
            {
                tlv_length += 6;  // interface MAC address (6 bytes)
                tlv_length += 2;  // media type (2 bytes)
                tlv_length += 3;  // OUI (2 bytes)
                tlv_length += 1;  // variant_index (1 byte)
                tlv_length += 1;  // media specific bytes number (1 bytes)
                tlv_length += m->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr;
                                  // media specific bytes
            }
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,                &p);
            _I2B(&tlv_length,                 &p);
            _I1B(&m->power_off_interfaces_nr, &p);

            for (i=0; i<m->power_off_interfaces_nr; i++)
            {
                _InB( m->power_off_interfaces[i].interface_address,                               &p, 6);
                _I2B(&m->power_off_interfaces[i].media_type,                                      &p);
                _InB( m->power_off_interfaces[i].generic_phy_common_data.oui,                     &p, 3);
                _I1B(&m->power_off_interfaces[i].generic_phy_common_data.variant_index,           &p);
                _I1B(&m->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr, &p);
                if (m->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr > 0)
                {
                    _InB( m->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes, &p, m->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr);
                }
            }

            return ret;
        }

        case TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.29"

            uint8_t *ret, *p;
            struct interfacePowerChangeInformationTLV *m;

            uint16_t tlv_length;

            uint8_t i;

            m = (struct interfacePowerChangeInformationTLV *)tlv;

            tlv_length  = 1;  // number of interfaces (1 bytes)
            tlv_length += (6+1) * m->power_change_interfaces_nr;

            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,                   &p);
            _I2B(&tlv_length,                    &p);
            _I1B(&m->power_change_interfaces_nr, &p);

            for (i=0; i<m->power_change_interfaces_nr; i++)
            {
                _InB( m->power_change_interfaces[i].interface_address,     &p, 6);
                _I1B(&m->power_change_interfaces[i].requested_power_state, &p);
            }

            return ret;
        }

        case TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.30"

            uint8_t *ret, *p;
            struct interfacePowerChangeStatusTLV *m;

            uint16_t tlv_length;

            uint8_t i;

            m = (struct interfacePowerChangeStatusTLV *)tlv;

            tlv_length  = 1;  // number of interfaces (1 bytes)
            tlv_length += (6+1) * m->power_change_interfaces_nr;

            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,                   &p);
            _I2B(&tlv_length,                    &p);
            _I1B(&m->power_change_interfaces_nr, &p);

            for (i=0; i<m->power_change_interfaces_nr; i++)
            {
                _InB( m->power_change_interfaces[i].interface_address, &p, 6);
                _I1B(&m->power_change_interfaces[i].result,            &p);
            }

            return ret;
        }

        case TLV_TYPE_L2_NEIGHBOR_DEVICE:
        {
            // This forging is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.31"

            uint8_t *ret, *p;
            struct l2NeighborDeviceTLV *m;

            uint16_t tlv_length;

            uint8_t i, j, k;

            m = (struct l2NeighborDeviceTLV *)tlv;

            tlv_length = 1;  // number of entries (1 bytes)
            for (i=0; i<m->local_interfaces_nr; i++)
            {
                tlv_length += 6;  // interface MAC address (6 bytes)
                tlv_length += 2;  // number of neighbors (2 bytes)

                for (j=0; j<m->local_interfaces[i].l2_neighbors_nr; j++)
                {
                    tlv_length += 6;  // neighbor MAC address (6 bytes)
                    tlv_length += 2;  // number of "behind" MACs (1 bytes)
                    tlv_length += 6 * m->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr;
                }
            }
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,            &p);
            _I2B(&tlv_length,             &p);
            _I1B(&m->local_interfaces_nr, &p);

            for (i=0; i<m->local_interfaces_nr; i++)
            {
                _InB( m->local_interfaces[i].local_mac_address, &p, 6);
                _I2B(&m->local_interfaces[i].l2_neighbors_nr,   &p);

                for (j=0; j<m->local_interfaces[i].l2_neighbors_nr; j++)
                {
                    _InB( m->local_interfaces[i].l2_neighbors[j].l2_neighbor_mac_address, &p, 6);
                    _I2B(&m->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr, &p);

                    for (k=0; k<m->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr; k++)
                    {
                        _InB( m->local_interfaces[i].l2_neighbors[j].behind_mac_addresses[k], &p, 6);
                    }
                }
            }

            return ret;
        }

        default:
        {
            uint8_t *ret = NULL;
            size_t length;
            DECLARE_HLIST_HEAD(dummy);
            tlv_add(tlv_1905_defs, &dummy, (struct tlv*)tlv);
            if (!tlv_forge(tlv_1905_defs, &dummy, MAX_NETWORK_SEGMENT_SIZE, &ret, &length))
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Failed to forge TLV %s\n",
                                            convert_1905_TLV_type_to_string(tlv->type));
                ret = NULL;
            }
            *len = length;
            hlist_head_init((hlist_head*)&tlv->s.h.l);
            return ret;
        }

    }

    // This code cannot be reached
    //
    return NULL;
}


void free_1905_TLV_structure(struct tlv *tlv)
{
    if (NULL == tlv)
    {
        return;
    }

    // The first byte of any of the valid structures is always the "tlv_type"
    // field.
    //
    switch (tlv->type)
    {
        case TLV_TYPE_DEVICE_INFORMATION_TYPE:
        {
            struct deviceInformationTypeTLV *m;

            m = (struct deviceInformationTypeTLV *)tlv;

            if (m->local_interfaces_nr > 0 && NULL != m->local_interfaces)
            {
                free(m->local_interfaces);
            }
            free(m);

            return;
        }

        case TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES:
        {
            struct deviceBridgingCapabilityTLV *m;
            uint8_t i;

            m = (struct deviceBridgingCapabilityTLV *)tlv;

            for (i=0; i < m->bridging_tuples_nr; i++)
            {
                if (m->bridging_tuples[i].bridging_tuple_macs_nr > 0 && NULL != m->bridging_tuples[i].bridging_tuple_macs)
                {
                    free(m->bridging_tuples[i].bridging_tuple_macs);
                }
            }
            if (m->bridging_tuples_nr > 0 && NULL != m->bridging_tuples)
            {
                free(m->bridging_tuples);
            }
            free(m);

            return;
        }

        case TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST:
        {
            struct non1905NeighborDeviceListTLV *m;

            m = (struct non1905NeighborDeviceListTLV *)tlv;

            if (m->non_1905_neighbors_nr > 0 && NULL != m->non_1905_neighbors)
            {
                free(m->non_1905_neighbors);
            }
            free(m);

            return;
        }

        case TLV_TYPE_NEIGHBOR_DEVICE_LIST:
        {
            struct neighborDeviceListTLV *m;

            m = (struct neighborDeviceListTLV *)tlv;

            if (m->neighbors_nr > 0 && NULL != m->neighbors)
            {
                free(m->neighbors);
            }
            free(m);

            return;
        }


        case TLV_TYPE_TRANSMITTER_LINK_METRIC:
        {
            struct transmitterLinkMetricTLV *m;

            m = (struct transmitterLinkMetricTLV *)tlv;

            if (m->transmitter_link_metrics_nr > 0 && NULL != m->transmitter_link_metrics)
            {
                free(m->transmitter_link_metrics);
            }
            free(m);

            return;
        }

        case TLV_TYPE_RECEIVER_LINK_METRIC:
        {
            struct receiverLinkMetricTLV *m;

            m = (struct receiverLinkMetricTLV *)tlv;

            if (m->receiver_link_metrics_nr > 0 && NULL != m->receiver_link_metrics)
            {
                free(m->receiver_link_metrics);
            }
            free(m);

            return;
        }

        case TLV_TYPE_WSC:
        {
            struct wscTLV *m;

            m = (struct wscTLV *)tlv;

            if (m->wsc_frame_size >0 && NULL != m->wsc_frame)
            {
                free(m->wsc_frame);
            }
            free(m);

            return;
        }

        case TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION:
        {
            struct pushButtonEventNotificationTLV *m;

            m = (struct pushButtonEventNotificationTLV *)tlv;

            if (m->media_types_nr > 0 && NULL != m->media_types)
            {
                free(m->media_types);
            }
            free(m);

            return;
        }

        case TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION:
        {
            struct genericPhyDeviceInformationTypeTLV *m;
            uint8_t i;

            m = (struct genericPhyDeviceInformationTypeTLV *)tlv;

            for (i=0; i < m->local_interfaces_nr; i++)
            {
                if (m->local_interfaces[i].generic_phy_description_xml_url_len > 0 && NULL != m->local_interfaces[i].generic_phy_description_xml_url)
                {
                    free(m->local_interfaces[i].generic_phy_description_xml_url);
                }

                if (m->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr > 0 && NULL != m->local_interfaces[i].generic_phy_common_data.media_specific_bytes)
                {
                    free(m->local_interfaces[i].generic_phy_common_data.media_specific_bytes);
                }
            }
            if (m->local_interfaces_nr > 0 && NULL != m->local_interfaces)
            {
                free(m->local_interfaces);
            }
            free(m);

            return;
        }

        case TLV_TYPE_CONTROL_URL:
        {
            struct controlUrlTypeTLV *m;

            m = (struct controlUrlTypeTLV *)tlv;

            if (NULL != m->url)
            {
                free(m->url);
            }
            free(m);

            return;
        }

        case TLV_TYPE_IPV4:
        {
            struct ipv4TypeTLV *m;
            uint8_t i;

            m = (struct ipv4TypeTLV *)tlv;

            for (i=0; i < m->ipv4_interfaces_nr; i++)
            {
                if (m->ipv4_interfaces[i].ipv4_nr > 0 && NULL != m->ipv4_interfaces[i].ipv4)
                {
                    free(m->ipv4_interfaces[i].ipv4);
                }
            }
            if (m->ipv4_interfaces_nr > 0 && NULL != m->ipv4_interfaces)
            {
                free(m->ipv4_interfaces);
            }
            free(m);

            return;
        }

        case TLV_TYPE_IPV6:
        {
            struct ipv6TypeTLV *m;
            uint8_t i;

            m = (struct ipv6TypeTLV *)tlv;

            for (i=0; i < m->ipv6_interfaces_nr; i++)
            {
                if (m->ipv6_interfaces[i].ipv6_nr > 0 && NULL != m->ipv6_interfaces[i].ipv6)
                {
                    free(m->ipv6_interfaces[i].ipv6);
                }
            }
            if (m->ipv6_interfaces_nr > 0 && NULL != m->ipv6_interfaces)
            {
                free(m->ipv6_interfaces);
            }
            free(m);

            return;
        }

        case TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION:
        {
            struct pushButtonGenericPhyEventNotificationTLV *m;

            m = (struct pushButtonGenericPhyEventNotificationTLV *)tlv;

            if (m->local_interfaces_nr > 0 && NULL != m->local_interfaces)
            {
                free(m->local_interfaces);
            }
            free(m);

            return;
        }

        case TLV_TYPE_POWER_OFF_INTERFACE:
        {
            struct powerOffInterfaceTLV *m;
            uint8_t i;

            m = (struct powerOffInterfaceTLV *)tlv;

            for (i=0; i < m->power_off_interfaces_nr; i++)
            {
                if (m->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr > 0 && NULL != m->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes)
                {
                    free(m->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes);
                }
            }
            if (m->power_off_interfaces_nr > 0 && NULL != m->power_off_interfaces)
            {
                free(m->power_off_interfaces);
            }
            free(m);

            return;
        }

        case TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION:
        {
            struct interfacePowerChangeInformationTLV *m;

            m = (struct interfacePowerChangeInformationTLV *)tlv;

            if (m->power_change_interfaces_nr > 0 && NULL != m->power_change_interfaces)
            {
                free(m->power_change_interfaces);
            }
            free(m);

            return;
        }

        case TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS:
        {
            struct interfacePowerChangeStatusTLV *m;

            m = (struct interfacePowerChangeStatusTLV *)tlv;

            if (m->power_change_interfaces_nr > 0 && NULL != m->power_change_interfaces)
            {
                free(m->power_change_interfaces);
            }
            free(m);

            return;
        }

        case TLV_TYPE_L2_NEIGHBOR_DEVICE:
        {
            struct l2NeighborDeviceTLV *m;
            uint8_t i, j;

            m = (struct l2NeighborDeviceTLV *)tlv;

            for (i=0; i < m->local_interfaces_nr; i++)
            {
                if (m->local_interfaces[i].l2_neighbors_nr > 0 && NULL != m->local_interfaces[i].l2_neighbors)
                {
                    for (j=0; j < m->local_interfaces[i].l2_neighbors_nr; j++)
                    {
                        if (m->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr > 0 && NULL != m->local_interfaces[i].l2_neighbors[j].behind_mac_addresses)
                        {
                            free(m->local_interfaces[i].l2_neighbors[j].behind_mac_addresses);
                        }
                    }
                    free(m->local_interfaces[i].l2_neighbors);
                }
            }
            if (m->local_interfaces_nr > 0 && NULL != m->local_interfaces)
            {
                free(m->local_interfaces);
            }
            free(m);

            return;
        }


        default:
        {
            DECLARE_HLIST_HEAD(dummy);
            hlist_head_init(&tlv->s.h.children[0]);
            hlist_head_init(&tlv->s.h.children[1]);
            tlv_add(tlv_1905_defs, &dummy, tlv);
            tlv_free(tlv_1905_defs, &dummy);
            return;
        }
    }

    // This code cannot be reached
    //
    return;
}


uint8_t compare_1905_TLV_structures(struct tlv *tlv_1, struct tlv *tlv_2)
{
    if (NULL == tlv_1 || NULL == tlv_2)
    {
        return 1;
    }

    // The first byte of any of the valid structures is always the "tlv_type"
    // field.
    //
    if (tlv_1->type != tlv_2->type)
    {
        return 1;
    }
    switch (tlv_1->type)
    {
        case TLV_TYPE_DEVICE_INFORMATION_TYPE:
        {
            struct deviceInformationTypeTLV *p1, *p2;
            uint8_t i;

            p1 = (struct deviceInformationTypeTLV *)tlv_1;
            p2 = (struct deviceInformationTypeTLV *)tlv_2;

            if (
                 memcmp(p1->al_mac_address,         p2->al_mac_address, 6) !=0  ||
                                 p1->local_interfaces_nr !=  p2->local_interfaces_nr
               )
            {
                return 1;
            }

            if (p1->local_interfaces_nr > 0 && (NULL == p1->local_interfaces || NULL == p2->local_interfaces))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->local_interfaces_nr; i++)
            {
                if (
                     memcmp(p1->local_interfaces[i].mac_address,                 p2->local_interfaces[i].mac_address, 6) !=0      ||
                                     p1->local_interfaces[i].media_type               !=  p2->local_interfaces[i].media_type               ||
                                     p1->local_interfaces[i].media_specific_data_size !=  p2->local_interfaces[i].media_specific_data_size
                   )
                {
                    return 1;
                }

                if (
                     (MEDIA_TYPE_IEEE_802_11B_2_4_GHZ == p1->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11G_2_4_GHZ == p1->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11A_5_GHZ   == p1->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_2_4_GHZ == p1->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_5_GHZ   == p1->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AC_5_GHZ  == p1->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AD_60_GHZ == p1->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AF_GHZ    == p1->local_interfaces[i].media_type)
                   )
                {
                    if (
                         memcmp(p1->local_interfaces[i].media_specific_data.ieee80211.network_membership,                     p2->local_interfaces[i].media_specific_data.ieee80211.network_membership,  6) !=0          ||
                                         p1->local_interfaces[i].media_specific_data.ieee80211.role                                !=  p2->local_interfaces[i].media_specific_data.ieee80211.role                                 ||
                                         p1->local_interfaces[i].media_specific_data.ieee80211.ap_channel_band                     !=  p2->local_interfaces[i].media_specific_data.ieee80211.ap_channel_band                      ||
                                         p1->local_interfaces[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_1 !=  p2->local_interfaces[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_1
                       )
                    {
                        return 1;
                    }

                }
                else if (
                          (MEDIA_TYPE_IEEE_1901_WAVELET == p1->local_interfaces[i].media_type) ||
                          (MEDIA_TYPE_IEEE_1901_FFT     == p1->local_interfaces[i].media_type)
                        )
                {
                    if (
                         memcmp(p1->local_interfaces[i].media_specific_data.ieee1901.network_identifier,  p2->local_interfaces[i].media_specific_data.ieee1901.network_identifier,  6) !=0
                       )
                    {
                        return 1;
                    }
                }
            }

            return 0;
        }

        case TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES:
        {
            struct deviceBridgingCapabilityTLV *p1, *p2;
            uint8_t i, j;

            p1 = (struct deviceBridgingCapabilityTLV *)tlv_1;
            p2 = (struct deviceBridgingCapabilityTLV *)tlv_2;

            if (
                 p1->bridging_tuples_nr != p2->bridging_tuples_nr
               )
            {
                return 1;
            }

            if (p1->bridging_tuples_nr > 0 && (NULL == p1->bridging_tuples || NULL == p2->bridging_tuples))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->bridging_tuples_nr; i++)
            {
                if (
                     p1->bridging_tuples[i].bridging_tuple_macs_nr  !=  p2->bridging_tuples[i].bridging_tuple_macs_nr
                   )
                {
                    return 1;
                }

                for (j=0; j<p1->bridging_tuples[i].bridging_tuple_macs_nr; j++)
                {
                    if (
                         memcmp(p1->bridging_tuples[i].bridging_tuple_macs[j].mac_address,  p2->bridging_tuples[i].bridging_tuple_macs[j].mac_address, 6) !=0
                       )
                    {
                        return 1;
                    }
                }
            }

            return 0;
        }

        case TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST:
        {
            struct non1905NeighborDeviceListTLV *p1, *p2;
            uint8_t i;

            p1 = (struct non1905NeighborDeviceListTLV *)tlv_1;
            p2 = (struct non1905NeighborDeviceListTLV *)tlv_2;

            if (
                 memcmp(p1->local_mac_address,        p2->local_mac_address, 6) !=0  ||
                                 p1->non_1905_neighbors_nr !=  p2->non_1905_neighbors_nr
               )
            {
                return 1;
            }

            if (p1->non_1905_neighbors_nr > 0 && (NULL == p1->non_1905_neighbors || NULL == p2->non_1905_neighbors))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->non_1905_neighbors_nr; i++)
            {
                if (
                     memcmp(p1->non_1905_neighbors[i].mac_address,     p2->non_1905_neighbors[i].mac_address, 6) !=0
                   )
                {
                    return 1;
                }
            }

            return 0;
        }

        case TLV_TYPE_NEIGHBOR_DEVICE_LIST:
        {
            struct neighborDeviceListTLV *p1, *p2;
            uint8_t i;

            p1 = (struct neighborDeviceListTLV *)tlv_1;
            p2 = (struct neighborDeviceListTLV *)tlv_2;

            if (
                 memcmp(p1->local_mac_address,     p2->local_mac_address, 6) !=0  ||
                                 p1->neighbors_nr       !=  p2->neighbors_nr
               )
            {
                return 1;
            }

            if (p1->neighbors_nr > 0 && (NULL == p1->neighbors || NULL == p2->neighbors))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->neighbors_nr; i++)
            {
                if (
                     memcmp(p1->neighbors[i].mac_address,     p2->neighbors[i].mac_address, 6) !=0  ||
                                     p1->neighbors[i].bridge_flag  !=  p2->neighbors[i].bridge_flag
                   )
                {
                    return 1;
                }
            }

            return 0;
        }

        case TLV_TYPE_TRANSMITTER_LINK_METRIC:
        {
            struct transmitterLinkMetricTLV *p1, *p2;
            uint8_t i;

            p1 = (struct transmitterLinkMetricTLV *)tlv_1;
            p2 = (struct transmitterLinkMetricTLV *)tlv_2;

            if (
                 memcmp(p1->local_al_address,              p2->local_al_address,    6) !=0  ||
                 memcmp(p1->neighbor_al_address,           p2->neighbor_al_address, 6) !=0  ||
                                 p1->transmitter_link_metrics_nr != p2->transmitter_link_metrics_nr
               )
            {
                return 1;
            }

            if (p1->transmitter_link_metrics_nr > 0 && (NULL == p1->transmitter_link_metrics || NULL == p2->transmitter_link_metrics))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->transmitter_link_metrics_nr; i++)
            {
                if (
                     memcmp(p1->transmitter_link_metrics[i].local_interface_address,       p2->transmitter_link_metrics[i].local_interface_address,    6) !=0  ||
                     memcmp(p1->transmitter_link_metrics[i].neighbor_interface_address,    p2->transmitter_link_metrics[i].neighbor_interface_address, 6) !=0  ||
                                     p1->transmitter_link_metrics[i].intf_type                  !=  p2->transmitter_link_metrics[i].intf_type                           ||
                                     p1->transmitter_link_metrics[i].bridge_flag                !=  p2->transmitter_link_metrics[i].bridge_flag                         ||
                                     p1->transmitter_link_metrics[i].packet_errors              !=  p2->transmitter_link_metrics[i].packet_errors                       ||
                                     p1->transmitter_link_metrics[i].transmitted_packets        !=  p2->transmitter_link_metrics[i].transmitted_packets                 ||
                                     p1->transmitter_link_metrics[i].mac_throughput_capacity    !=  p2->transmitter_link_metrics[i].mac_throughput_capacity             ||
                                     p1->transmitter_link_metrics[i].link_availability          !=  p2->transmitter_link_metrics[i].link_availability                   ||
                                     p1->transmitter_link_metrics[i].phy_rate                   !=  p2->transmitter_link_metrics[i].phy_rate
                   )
                {
                    return 1;
                }
            }

            return 0;
        }

        case TLV_TYPE_RECEIVER_LINK_METRIC:
        {
            struct receiverLinkMetricTLV *p1, *p2;
            uint8_t i;

            p1 = (struct receiverLinkMetricTLV *)tlv_1;
            p2 = (struct receiverLinkMetricTLV *)tlv_2;

            if (
                 memcmp(p1->local_al_address,           p2->local_al_address,    6) !=0  ||
                 memcmp(p1->neighbor_al_address,        p2->neighbor_al_address, 6) !=0  ||
                                 p1->receiver_link_metrics_nr != p2->receiver_link_metrics_nr
               )
            {
                return 1;
            }

            if (p1->receiver_link_metrics_nr > 0 && (NULL == p1->receiver_link_metrics || NULL == p2->receiver_link_metrics))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->receiver_link_metrics_nr; i++)
            {
                if (
                     memcmp(p1->receiver_link_metrics[i].local_interface_address,       p2->receiver_link_metrics[i].local_interface_address,    6) !=0  ||
                     memcmp(p1->receiver_link_metrics[i].neighbor_interface_address,    p2->receiver_link_metrics[i].neighbor_interface_address, 6) !=0  ||
                                     p1->receiver_link_metrics[i].intf_type                  !=  p2->receiver_link_metrics[i].intf_type                           ||
                                     p1->receiver_link_metrics[i].packet_errors              !=  p2->receiver_link_metrics[i].packet_errors                       ||
                                     p1->receiver_link_metrics[i].packets_received           !=  p2->receiver_link_metrics[i].packets_received                    ||
                                     p1->receiver_link_metrics[i].rssi                       !=  p2->receiver_link_metrics[i].rssi
                   )
                {
                    return 1;
                }
            }

            return 0;
        }

        case TLV_TYPE_LINK_METRIC_RESULT_CODE:
        {
            struct linkMetricResultCodeTLV *p1, *p2;

            p1 = (struct linkMetricResultCodeTLV *)tlv_1;
            p2 = (struct linkMetricResultCodeTLV *)tlv_2;

            if (
                 p1->result_code != p2->result_code
               )
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        case TLV_TYPE_SEARCHED_ROLE:
        {
            struct searchedRoleTLV *p1, *p2;

            p1 = (struct searchedRoleTLV *)tlv_1;
            p2 = (struct searchedRoleTLV *)tlv_2;

            if (
                 p1->role != p2->role
               )
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        case TLV_TYPE_AUTOCONFIG_FREQ_BAND:
        {
            struct autoconfigFreqBandTLV *p1, *p2;

            p1 = (struct autoconfigFreqBandTLV *)tlv_1;
            p2 = (struct autoconfigFreqBandTLV *)tlv_2;

            if (
                 p1->freq_band != p2->freq_band
               )
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        case TLV_TYPE_SUPPORTED_ROLE:
        {
            struct supportedRoleTLV *p1, *p2;

            p1 = (struct supportedRoleTLV *)tlv_1;
            p2 = (struct supportedRoleTLV *)tlv_2;

            if (
                 p1->role != p2->role
               )
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        case TLV_TYPE_SUPPORTED_FREQ_BAND:
        {
            struct supportedFreqBandTLV *p1, *p2;

            p1 = (struct supportedFreqBandTLV *)tlv_1;
            p2 = (struct supportedFreqBandTLV *)tlv_2;

            if (
                 p1->freq_band != p2->freq_band
               )
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        case TLV_TYPE_WSC:
        {
            struct wscTLV *p1, *p2;

            p1 = (struct wscTLV *)tlv_1;
            p2 = (struct wscTLV *)tlv_2;

            if(
                                p1->wsc_frame_size  !=  p2->wsc_frame_size                     ||
                memcmp(p1->wsc_frame,          p2->wsc_frame,       p1->wsc_frame_size) !=0
              )
            {
                return 1;
            }

            return 0;
        }

        case TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION:
        {
            struct pushButtonEventNotificationTLV *p1, *p2;
            uint8_t i;

            p1 = (struct pushButtonEventNotificationTLV *)tlv_1;
            p2 = (struct pushButtonEventNotificationTLV *)tlv_2;

            if (p1->media_types_nr !=  p2->media_types_nr)
            {
                return 1;
            }

            if (p1->media_types_nr > 0 && (NULL == p1->media_types || NULL == p2->media_types))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->media_types_nr; i++)
            {
                if (
                     p1->media_types[i].media_type               !=  p2->media_types[i].media_type               ||
                     p1->media_types[i].media_specific_data_size !=  p2->media_types[i].media_specific_data_size
                   )
                {
                    return 1;
                }

                if (
                     (MEDIA_TYPE_IEEE_802_11B_2_4_GHZ == p1->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11G_2_4_GHZ == p1->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11A_5_GHZ   == p1->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_2_4_GHZ == p1->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_5_GHZ   == p1->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AC_5_GHZ  == p1->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AD_60_GHZ == p1->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AF_GHZ    == p1->media_types[i].media_type)
                   )
                {
                    if (
                         memcmp(p1->media_types[i].media_specific_data.ieee80211.network_membership,                     p2->media_types[i].media_specific_data.ieee80211.network_membership,  6) !=0          ||
                                         p1->media_types[i].media_specific_data.ieee80211.role                                !=  p2->media_types[i].media_specific_data.ieee80211.role                                 ||
                                         p1->media_types[i].media_specific_data.ieee80211.ap_channel_band                     !=  p2->media_types[i].media_specific_data.ieee80211.ap_channel_band                      ||
                                         p1->media_types[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_1 !=  p2->media_types[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_1
                       )
                    {
                        return 1;
                    }

                }
                else if (
                          (MEDIA_TYPE_IEEE_1901_WAVELET == p1->media_types[i].media_type) ||
                          (MEDIA_TYPE_IEEE_1901_FFT     == p1->media_types[i].media_type)
                        )
                {
                    if (
                         memcmp(p1->media_types[i].media_specific_data.ieee1901.network_identifier,  p2->media_types[i].media_specific_data.ieee1901.network_identifier,  6) !=0
                       )
                    {
                        return 1;
                    }
                }
            }

            return 0;
        }

        case TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION:
        {
            struct pushButtonJoinNotificationTLV *p1, *p2;

            p1 = (struct pushButtonJoinNotificationTLV *)tlv_1;
            p2 = (struct pushButtonJoinNotificationTLV *)tlv_2;

            if (
                 memcmp(p1->al_mac_address,       p2->al_mac_address, 6) !=0  ||
                                 p1->message_identifier != p2->message_identifier      ||
                 memcmp(p1->mac_address,          p2->al_mac_address, 6) !=0  ||
                 memcmp(p1->new_mac_address,      p2->al_mac_address, 6) !=0
               )
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        case TLV_TYPE_DEVICE_IDENTIFICATION:
        {
            struct deviceIdentificationTypeTLV *p1, *p2;

            p1 = (struct deviceIdentificationTypeTLV *)tlv_1;
            p2 = (struct deviceIdentificationTypeTLV *)tlv_2;

            if (
                 memcmp(p1->friendly_name,       p2->friendly_name,      64) !=0  ||
                 memcmp(p1->manufacturer_name,   p2->manufacturer_name,  64) !=0  ||
                 memcmp(p1->manufacturer_model,  p2->manufacturer_model, 64) !=0
               )
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        case TLV_TYPE_CONTROL_URL:
        {
            struct controlUrlTypeTLV *p1, *p2;

            p1 = (struct controlUrlTypeTLV *)tlv_1;
            p2 = (struct controlUrlTypeTLV *)tlv_2;

            if(
                memcmp(p1->url, p2->url, strlen(p1->url)+1) !=0
              )
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        case TLV_TYPE_IPV4:
        {
            struct ipv4TypeTLV *p1, *p2;
            uint8_t i, j;

            p1 = (struct ipv4TypeTLV *)tlv_1;
            p2 = (struct ipv4TypeTLV *)tlv_2;

            if (
                 p1->ipv4_interfaces_nr != p2->ipv4_interfaces_nr
               )
            {
                return 1;
            }

            if (p1->ipv4_interfaces_nr > 0 && (NULL == p1->ipv4_interfaces || NULL == p2->ipv4_interfaces))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->ipv4_interfaces_nr; i++)
            {
                if (
                     memcmp(p1->ipv4_interfaces[i].mac_address,     p2->ipv4_interfaces[i].mac_address, 6) !=0   ||
                                     p1->ipv4_interfaces[i].ipv4_nr      !=  p2->ipv4_interfaces[i].ipv4_nr
                   )
                {
                    return 1;
                }

                for (j=0; j<p1->ipv4_interfaces[i].ipv4_nr; j++)
                {
                    if (
                                         p1->ipv4_interfaces[i].ipv4[j].type              !=  p2->ipv4_interfaces[i].ipv4[j].type                       ||
                         memcmp(p1->ipv4_interfaces[i].ipv4[j].ipv4_address,         p2->ipv4_interfaces[i].ipv4[j].ipv4_address,     4) !=0   ||
                         memcmp(p1->ipv4_interfaces[i].ipv4[j].ipv4_dhcp_server,     p2->ipv4_interfaces[i].ipv4[j].ipv4_dhcp_server, 4) !=0
                       )
                    {
                        return 1;
                    }
                }
            }

            return 0;
        }

        case TLV_TYPE_IPV6:
        {
            struct ipv6TypeTLV *p1, *p2;
            uint8_t i, j;

            p1 = (struct ipv6TypeTLV *)tlv_1;
            p2 = (struct ipv6TypeTLV *)tlv_2;

            if (
                 p1->ipv6_interfaces_nr != p2->ipv6_interfaces_nr
               )
            {
                return 1;
            }

            if (p1->ipv6_interfaces_nr > 0 && (NULL == p1->ipv6_interfaces || NULL == p2->ipv6_interfaces))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->ipv6_interfaces_nr; i++)
            {
                if (
                     memcmp(p1->ipv6_interfaces[i].mac_address,     p2->ipv6_interfaces[i].mac_address, 6) !=0   ||
                                     p1->ipv6_interfaces[i].ipv6_nr      !=  p2->ipv6_interfaces[i].ipv6_nr
                   )
                {
                    return 1;
                }

                for (j=0; j<p1->ipv6_interfaces[i].ipv6_nr; j++)
                {
                    if (
                                         p1->ipv6_interfaces[i].ipv6[j].type                 !=  p2->ipv6_interfaces[i].ipv6[j].type                       ||
                         memcmp(p1->ipv6_interfaces[i].ipv6[j].ipv6_address,            p2->ipv6_interfaces[i].ipv6[j].ipv6_address,        16) !=0   ||
                         memcmp(p1->ipv6_interfaces[i].ipv6[j].ipv6_address_origin,     p2->ipv6_interfaces[i].ipv6[j].ipv6_address_origin, 16) !=0
                       )
                    {
                        return 1;
                    }
                }
            }

            return 0;
        }

        case TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION:
        {
            struct genericPhyDeviceInformationTypeTLV *p1, *p2;
            uint8_t i;

            p1 = (struct genericPhyDeviceInformationTypeTLV *)tlv_1;
            p2 = (struct genericPhyDeviceInformationTypeTLV *)tlv_2;

            if (
                 memcmp(p1->al_mac_address,        p2->al_mac_address,     6) !=0  ||
                                 p1->local_interfaces_nr != p2->local_interfaces_nr
               )
            {
                return 1;
            }

            if (p1->local_interfaces_nr > 0 && (NULL == p1->local_interfaces || NULL == p2->local_interfaces))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->local_interfaces_nr; i++)
            {
                if (
                     memcmp(p1->local_interfaces[i].local_interface_address,                             p2->local_interfaces[i].local_interface_address,                      6) !=0  ||
                     memcmp(p1->local_interfaces[i].generic_phy_common_data.oui,                         p2->local_interfaces[i].generic_phy_common_data.oui,                  3) !=0  ||
                                     p1->local_interfaces[i].generic_phy_common_data.variant_index            !=  p2->local_interfaces[i].generic_phy_common_data.variant_index                 ||
                     memcmp(p1->local_interfaces[i].variant_name,                                        p2->local_interfaces[i].variant_name,                                32) !=0  ||
                                     p1->local_interfaces[i].generic_phy_description_xml_url_len              !=  p2->local_interfaces[i].generic_phy_description_xml_url_len                   ||
                                     p1->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr  !=  p2->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr       ||
                     memcmp(p1->local_interfaces[i].generic_phy_description_xml_url,                     p2->local_interfaces[i].generic_phy_description_xml_url,              p1->local_interfaces[i].generic_phy_description_xml_url_len) !=0  ||
                     memcmp(p1->local_interfaces[i].generic_phy_common_data.media_specific_bytes,        p2->local_interfaces[i].generic_phy_common_data.media_specific_bytes, p1->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr) !=0
                   )
                {
                    return 1;
                }
            }

            return 0;
        }

        case TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION:
        {
            struct pushButtonGenericPhyEventNotificationTLV *p1, *p2;
            uint8_t i;

            p1 = (struct pushButtonGenericPhyEventNotificationTLV *)tlv_1;
            p2 = (struct pushButtonGenericPhyEventNotificationTLV *)tlv_2;

            if (
                 p1->local_interfaces_nr != p2->local_interfaces_nr
               )
            {
                return 1;
            }

            if (p1->local_interfaces_nr > 0 && (NULL == p1->local_interfaces || NULL == p2->local_interfaces))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->local_interfaces_nr; i++)
            {
                if (
                     memcmp(p1->local_interfaces[i].oui,                         p2->local_interfaces[i].oui,                  3) !=0  ||
                                     p1->local_interfaces[i].variant_index            !=  p2->local_interfaces[i].variant_index                 ||
                                     p1->local_interfaces[i].media_specific_bytes_nr  !=  p2->local_interfaces[i].media_specific_bytes_nr       ||
                     memcmp(p1->local_interfaces[i].media_specific_bytes,        p2->local_interfaces[i].media_specific_bytes, p1->local_interfaces[i].media_specific_bytes_nr) !=0
                   )
                {
                    return 1;
                }
            }

            return 0;
        }

        case TLV_TYPE_1905_PROFILE_VERSION:
        {
            struct x1905ProfileVersionTLV *p1, *p2;

            p1 = (struct x1905ProfileVersionTLV *)tlv_1;
            p2 = (struct x1905ProfileVersionTLV *)tlv_2;

            if (
                 p1->profile != p2->profile
               )
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        case TLV_TYPE_POWER_OFF_INTERFACE:
        {
            struct powerOffInterfaceTLV *p1, *p2;
            uint8_t i;

            p1 = (struct powerOffInterfaceTLV *)tlv_1;
            p2 = (struct powerOffInterfaceTLV *)tlv_2;

            if (
                 p1->power_off_interfaces_nr != p2->power_off_interfaces_nr
               )
            {
                return 1;
            }

            if (p1->power_off_interfaces_nr > 0 && (NULL == p1->power_off_interfaces || NULL == p2->power_off_interfaces))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->power_off_interfaces_nr; i++)
            {
                if (
                     memcmp(p1->power_off_interfaces[i].interface_address,                                   p2->power_off_interfaces[i].interface_address,                            6) !=0  ||
                                     p1->power_off_interfaces[i].media_type                                       !=  p2->power_off_interfaces[i].media_type                                            ||
                     memcmp(p1->power_off_interfaces[i].generic_phy_common_data.oui,                         p2->power_off_interfaces[i].generic_phy_common_data.oui,                  3) !=0  ||
                                     p1->power_off_interfaces[i].generic_phy_common_data.variant_index            !=  p2->power_off_interfaces[i].generic_phy_common_data.variant_index                 ||
                                     p1->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr  !=  p2->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr       ||
                     memcmp(p1->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes,        p2->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes, p1->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr) !=0
                   )
                {
                    return 1;
                }
            }

            return 0;
        }

        case TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION:
        {
            struct interfacePowerChangeInformationTLV *p1, *p2;
            uint8_t i;

            p1 = (struct interfacePowerChangeInformationTLV *)tlv_1;
            p2 = (struct interfacePowerChangeInformationTLV *)tlv_2;

            if (
                 p1->power_change_interfaces_nr != p2->power_change_interfaces_nr
               )
            {
                return 1;
            }

            if (p1->power_change_interfaces_nr > 0 && (NULL == p1->power_change_interfaces || NULL == p2->power_change_interfaces))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->power_change_interfaces_nr; i++)
            {
                if (
                     memcmp(p1->power_change_interfaces[i].interface_address,        p2->power_change_interfaces[i].interface_address,     6) !=0  ||
                                     p1->power_change_interfaces[i].requested_power_state !=  p2->power_change_interfaces[i].requested_power_state
                   )
                {
                    return 1;
                }
            }

            return 0;
        }

        case TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS:
        {
            struct interfacePowerChangeStatusTLV *p1, *p2;
            uint8_t i;

            p1 = (struct interfacePowerChangeStatusTLV *)tlv_1;
            p2 = (struct interfacePowerChangeStatusTLV *)tlv_2;

            if (
                 p1->power_change_interfaces_nr != p2->power_change_interfaces_nr
               )
            {
                return 1;
            }

            if (p1->power_change_interfaces_nr > 0 && (NULL == p1->power_change_interfaces || NULL == p2->power_change_interfaces))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->power_change_interfaces_nr; i++)
            {
                if (
                     memcmp(p1->power_change_interfaces[i].interface_address,     p2->power_change_interfaces[i].interface_address,  6) !=0  ||
                                     p1->power_change_interfaces[i].result             !=  p2->power_change_interfaces[i].result
                   )
                {
                    return 1;
                }
            }

            return 0;
        }

        case TLV_TYPE_L2_NEIGHBOR_DEVICE:
        {
            struct l2NeighborDeviceTLV *p1, *p2;
            uint8_t i, j, k;

            p1 = (struct l2NeighborDeviceTLV *)tlv_1;
            p2 = (struct l2NeighborDeviceTLV *)tlv_2;

            if (
                 p1->local_interfaces_nr != p2->local_interfaces_nr
               )
            {
                return 1;
            }

            if (p1->local_interfaces_nr > 0 && (NULL == p1->local_interfaces || NULL == p2->local_interfaces))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->local_interfaces_nr; i++)
            {
                if (
                     memcmp(p1->local_interfaces[i].local_mac_address,     p2->local_interfaces[i].local_mac_address, 6) !=0   ||
                                     p1->local_interfaces[i].l2_neighbors_nr    !=  p2->local_interfaces[i].l2_neighbors_nr
                   )
                {
                    return 1;
                }

                if (p1->local_interfaces[i].l2_neighbors_nr > 0 && (NULL == p1->local_interfaces[i].l2_neighbors || NULL == p2->local_interfaces[i].l2_neighbors))
                {
                    // Malformed structure
                    //
                    return 1;
                }

                for (j=0; j<p1->local_interfaces[i].l2_neighbors_nr; j++)
                {
                    if (
                         memcmp(p1->local_interfaces[i].l2_neighbors[j].l2_neighbor_mac_address,     p2->local_interfaces[i].l2_neighbors[j].l2_neighbor_mac_address,        6) !=0   ||
                                         p1->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr  !=  p2->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr
                       )
                    {
                        return 1;
                    }

                    if (p1->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr > 0 && (NULL == p1->local_interfaces[i].l2_neighbors[j].behind_mac_addresses || NULL == p2->local_interfaces[i].l2_neighbors[j].behind_mac_addresses))
                    {
                        // Malformed structure
                        //
                        return 1;
                    }

                    for (k=0; k<p1->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr; k++)
                    {
                        if (
                             memcmp(p1->local_interfaces[i].l2_neighbors[j].behind_mac_addresses[k], p2->local_interfaces[i].l2_neighbors[j].behind_mac_addresses[k], 6) !=0
                           )
                        {
                            return 1;
                        }
                    }
                }
            }

            return 0;
        }

        default:
        {
            uint8_t ret;
            DECLARE_HLIST_HEAD(dummy1);
            tlv_add(tlv_1905_defs, &dummy1, tlv_1);
            DECLARE_HLIST_HEAD(dummy2);
            tlv_add(tlv_1905_defs, &dummy2, tlv_2);
            if (tlv_compare(tlv_1905_defs, &dummy1, &dummy2))
            {
                ret = 0;
            }
            else
            {
                ret = 1;
            }
            hlist_head_init(&tlv_1->s.h.l);
            hlist_head_init(&tlv_2->s.h.l);

            return ret;
        }
    }

    // This code cannot be reached
    //
    return 1;
}


void visit_1905_TLV_structure(struct tlv *tlv, visitor_callback callback, void (*write_function)(const char *fmt, ...), const char *prefix)
{
    // In order to make it easier for the callback() function to present
    // useful information, append the type of the TLV to the prefix
    //
    char tlv_prefix[MAX_PREFIX];

    if (NULL == tlv)
    {
        return;
    }

    snprintf(tlv_prefix, MAX_PREFIX-1, "%sTLV(%s)->",
                      prefix,
                      convert_1905_TLV_type_to_string(tlv->type));
    tlv_prefix[MAX_PREFIX-1] = 0x0;

    // The first byte of any of the valid structures is always the "tlv_type"
    // field.
    //
    switch (tlv->type)
    {
        case TLV_TYPE_DEVICE_INFORMATION_TYPE:
        {
            struct deviceInformationTypeTLV *p;
            uint8_t i;

            p = (struct deviceInformationTypeTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->al_mac_address),      "al_mac_address",       "0x%02x",   p->al_mac_address);
            callback(write_function, tlv_prefix, sizeof(p->local_interfaces_nr), "local_interfaces_nr",  "%d",       &p->local_interfaces_nr);
            for (i=0; i < p->local_interfaces_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%slocal_interfaces[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].mac_address),              "mac_address",              "0x%02x",   p->local_interfaces[i].mac_address);
                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].media_type),               "media_type",               "0x%04x",  &p->local_interfaces[i].media_type);
                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].media_specific_data_size), "media_specific_data_size", "%d",      &p->local_interfaces[i].media_specific_data_size);

                if (
                     (MEDIA_TYPE_IEEE_802_11B_2_4_GHZ == p->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11G_2_4_GHZ == p->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11A_5_GHZ   == p->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_2_4_GHZ == p->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_5_GHZ   == p->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AC_5_GHZ  == p->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AD_60_GHZ == p->local_interfaces[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AF_GHZ    == p->local_interfaces[i].media_type)
                   )
                {
                    callback(write_function, new_prefix, sizeof(p->local_interfaces[i].media_specific_data.ieee80211.network_membership),                  "network_membership",                   "0x%02x",   p->local_interfaces[i].media_specific_data.ieee80211.network_membership);
                    callback(write_function, new_prefix, sizeof(p->local_interfaces[i].media_specific_data.ieee80211.role),                                "role",                                 "%d",      &p->local_interfaces[i].media_specific_data.ieee80211.role);
                    callback(write_function, new_prefix, sizeof(p->local_interfaces[i].media_specific_data.ieee80211.ap_channel_band),                     "ap_channel_band",                      "%d",      &p->local_interfaces[i].media_specific_data.ieee80211.ap_channel_band);
                    callback(write_function, new_prefix, sizeof(p->local_interfaces[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_1), "ap_channel_center_frequency_index_1",  "%d",      &p->local_interfaces[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_1);
                    callback(write_function, new_prefix, sizeof(p->local_interfaces[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_2), "ap_channel_center_frequency_index_2",  "%d",      &p->local_interfaces[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_2);
                }
                else if (
                          (MEDIA_TYPE_IEEE_1901_WAVELET == p->local_interfaces[i].media_type) ||
                          (MEDIA_TYPE_IEEE_1901_FFT     == p->local_interfaces[i].media_type)
                        )
                {
                    callback(write_function, new_prefix, sizeof(p->local_interfaces[i].media_specific_data.ieee1901.network_identifier), "network_identifier", "0x%02x", p->local_interfaces[i].media_specific_data.ieee1901.network_identifier);
                }

            }

            return;
        }

        case TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES:
        {
            struct deviceBridgingCapabilityTLV *p;
            uint8_t i, j;

            p = (struct deviceBridgingCapabilityTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->bridging_tuples_nr), "bridging_tuples_nr", "%d",  &p->bridging_tuples_nr);
            for (i=0; i < p->bridging_tuples_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%sbridging_tuples[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->bridging_tuples[i].bridging_tuple_macs_nr), "bridging_tuple_macs_nr", "%d",  &p->bridging_tuples[i].bridging_tuple_macs_nr);

                for (j=0; j < p->bridging_tuples[i].bridging_tuple_macs_nr; j++)
                {
                    snprintf(new_prefix, MAX_PREFIX-1, "%sbridging_tuples[%d]->bridging_tuple_macs[%d]->", tlv_prefix, i, j);
                    new_prefix[MAX_PREFIX-1] = 0x0;

                    callback(write_function, new_prefix, sizeof(p->bridging_tuples[i].bridging_tuple_macs[j].mac_address), "mac_address", "0x%02x",  p->bridging_tuples[i].bridging_tuple_macs[j].mac_address);
                }
            }

            return;
        }

        case TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST:
        {
            struct non1905NeighborDeviceListTLV *p;
            uint8_t i;

            p = (struct non1905NeighborDeviceListTLV *)tlv;

            if (p->non_1905_neighbors_nr > 0 && NULL == p->non_1905_neighbors)
            {
                // Malformed structure
                return;
            }

            callback(write_function, tlv_prefix, sizeof(p->local_mac_address),     "local_mac_address",     "0x%02x",   p->local_mac_address);
            callback(write_function, tlv_prefix, sizeof(p->non_1905_neighbors_nr), "non_1905_neighbors_nr", "%d",      &p->non_1905_neighbors_nr);
            for (i=0; i < p->non_1905_neighbors_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%snon_1905_neighbors[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->non_1905_neighbors[i].mac_address), "mac_address", "0x%02x", p->non_1905_neighbors[i].mac_address);
            }

            return;
        }

        case TLV_TYPE_NEIGHBOR_DEVICE_LIST:
        {
            struct neighborDeviceListTLV *p;
            uint8_t i;

            p = (struct neighborDeviceListTLV *)tlv;

            if (p->neighbors_nr > 0 && NULL == p->neighbors)
            {
                // Malformed structure
                return;
            }

            callback(write_function, tlv_prefix, sizeof(p->local_mac_address), "local_mac_address",  "0x%02x",   p->local_mac_address);
            callback(write_function, tlv_prefix, sizeof(p->neighbors_nr),      "neighbors_nr",       "%d",      &p->neighbors_nr);
            for (i=0; i < p->neighbors_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%sneighbors[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->neighbors[i].mac_address), "mac_address", "0x%02x",  p->neighbors[i].mac_address);
                callback(write_function, new_prefix, sizeof(p->neighbors[i].bridge_flag), "bridge_flag", "%d",     &p->neighbors[i].bridge_flag);
            }

            return;
        }

        case TLV_TYPE_TRANSMITTER_LINK_METRIC:
        {
            struct transmitterLinkMetricTLV *p;
            uint8_t i;

            p = (struct transmitterLinkMetricTLV *)tlv;

            if (NULL == p->transmitter_link_metrics)
            {
                // Malformed structure
                return;
            }

            callback(write_function, tlv_prefix, sizeof(p->local_al_address),            "local_al_address",            "0x%02x",   p->local_al_address);
            callback(write_function, tlv_prefix, sizeof(p->neighbor_al_address),         "neighbor_al_address",         "0x%02x",   p->neighbor_al_address);
            callback(write_function, tlv_prefix, sizeof(p->transmitter_link_metrics_nr), "transmitter_link_metrics_nr", "%d",      &p->transmitter_link_metrics_nr);
            for (i=0; i < p->transmitter_link_metrics_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%stransmitter_link_metrics[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].local_interface_address),    "local_interface_address",    "0x%02x",   p->transmitter_link_metrics[i].local_interface_address);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].neighbor_interface_address), "neighbor_interface_address", "0x%02x",   p->transmitter_link_metrics[i].neighbor_interface_address);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].intf_type),                  "intf_type",                  "0x%04x",  &p->transmitter_link_metrics[i].intf_type);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].bridge_flag),                "bridge_flag",                "%d",      &p->transmitter_link_metrics[i].bridge_flag);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].packet_errors),              "packet_errors",              "%d",      &p->transmitter_link_metrics[i].packet_errors);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].transmitted_packets),        "transmitted_packets",        "%d",      &p->transmitter_link_metrics[i].transmitted_packets);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].mac_throughput_capacity),    "mac_throughput_capacity",    "%d",      &p->transmitter_link_metrics[i].mac_throughput_capacity);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].link_availability),          "link_availability",          "%d",      &p->transmitter_link_metrics[i].link_availability);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].phy_rate),                   "phy_rate",                   "%d",      &p->transmitter_link_metrics[i].phy_rate);
            }

            return;
        }

        case TLV_TYPE_RECEIVER_LINK_METRIC:
        {
            struct receiverLinkMetricTLV *p;
            uint8_t i;

            p = (struct receiverLinkMetricTLV *)tlv;

            if (NULL == p->receiver_link_metrics)
            {
                // Malformed structure
                return;
            }

            callback(write_function, tlv_prefix, sizeof(p->local_al_address),         "local_al_address",         "0x%02x",   p->local_al_address);
            callback(write_function, tlv_prefix, sizeof(p->neighbor_al_address),      "neighbor_al_address",      "0x%02x",   p->neighbor_al_address);
            callback(write_function, tlv_prefix, sizeof(p->receiver_link_metrics_nr), "receiver_link_metrics_nr", "%d",      &p->receiver_link_metrics_nr);
            for (i=0; i < p->receiver_link_metrics_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%sreceiver_link_metrics[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->receiver_link_metrics[i].local_interface_address),    "local_interface_address",    "0x%02x",   p->receiver_link_metrics[i].local_interface_address);
                callback(write_function, new_prefix, sizeof(p->receiver_link_metrics[i].neighbor_interface_address), "neighbor_interface_address", "0x%02x",   p->receiver_link_metrics[i].neighbor_interface_address);
                callback(write_function, new_prefix, sizeof(p->receiver_link_metrics[i].intf_type),                  "intf_type",                  "0x%04x",  &p->receiver_link_metrics[i].intf_type);
                callback(write_function, new_prefix, sizeof(p->receiver_link_metrics[i].packet_errors),              "packet_errors",              "%d",      &p->receiver_link_metrics[i].packet_errors);
                callback(write_function, new_prefix, sizeof(p->receiver_link_metrics[i].packets_received),           "packets_received",           "%d",      &p->receiver_link_metrics[i].packets_received);
                callback(write_function, new_prefix, sizeof(p->receiver_link_metrics[i].rssi),                       "rssi",                       "%d",      &p->receiver_link_metrics[i].rssi);
            }

            return;
        }

        case TLV_TYPE_LINK_METRIC_RESULT_CODE:
        {
            struct linkMetricResultCodeTLV *p;

            p = (struct linkMetricResultCodeTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->result_code), "result_code",  "%d",  &p->result_code);

            return;
        }

        case TLV_TYPE_SEARCHED_ROLE:
        {
            struct searchedRoleTLV *p;

            p = (struct searchedRoleTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->role), "role",  "%d",  &p->role);

            return;
        }

        case TLV_TYPE_AUTOCONFIG_FREQ_BAND:
        {
            struct autoconfigFreqBandTLV *p;

            p = (struct autoconfigFreqBandTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->freq_band), "freq_band",  "%d",  &p->freq_band);

            return;
        }

        case TLV_TYPE_SUPPORTED_ROLE:
        {
            struct supportedRoleTLV *p;

            p = (struct supportedRoleTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->role), "role",  "%d",  &p->role);

            return;
        }

        case TLV_TYPE_SUPPORTED_FREQ_BAND:
        {
            struct supportedFreqBandTLV *p;

            p = (struct supportedFreqBandTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->freq_band), "freq_band",  "%d",  &p->freq_band);

            return;
        }

        case TLV_TYPE_WSC:
        {
            struct wscTLV *p;

            p = (struct wscTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->wsc_frame_size), "wsc_frame_size",  "%d",      &p->wsc_frame_size);
            callback(write_function, tlv_prefix, p->wsc_frame_size,         "wsc_frame",       "0x%02x",   p->wsc_frame);

            return;
        }

        case TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION:
        {
            struct pushButtonEventNotificationTLV *p;
            uint8_t i;

            p = (struct pushButtonEventNotificationTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->media_types_nr), "media_types_nr",  "0x%02x",  &p->media_types_nr);
            for (i=0; i < p->media_types_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%smedia_types[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->media_types[i].media_type),               "media_type",               "0x%04x",  &p->media_types[i].media_type);
                callback(write_function, new_prefix, sizeof(p->media_types[i].media_specific_data_size), "media_specific_data_size", "%d",      &p->media_types[i].media_specific_data_size);

                if (
                     (MEDIA_TYPE_IEEE_802_11B_2_4_GHZ == p->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11G_2_4_GHZ == p->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11A_5_GHZ   == p->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_2_4_GHZ == p->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11N_5_GHZ   == p->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AC_5_GHZ  == p->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AD_60_GHZ == p->media_types[i].media_type) ||
                     (MEDIA_TYPE_IEEE_802_11AF_GHZ    == p->media_types[i].media_type)
                   )
                {
                    callback(write_function, new_prefix, sizeof(p->media_types[i].media_specific_data.ieee80211.network_membership),                  "network_membership",                   "0x%02x",   p->media_types[i].media_specific_data.ieee80211.network_membership);
                    callback(write_function, new_prefix, sizeof(p->media_types[i].media_specific_data.ieee80211.role),                                "role",                                 "%d",      &p->media_types[i].media_specific_data.ieee80211.role);
                    callback(write_function, new_prefix, sizeof(p->media_types[i].media_specific_data.ieee80211.ap_channel_band),                     "ap_channel_band",                      "%d",      &p->media_types[i].media_specific_data.ieee80211.ap_channel_band);
                    callback(write_function, new_prefix, sizeof(p->media_types[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_1), "ap_channel_center_frequency_index_1",  "%d",      &p->media_types[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_1);
                    callback(write_function, new_prefix, sizeof(p->media_types[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_2), "ap_channel_center_frequency_index_2",  "%d",      &p->media_types[i].media_specific_data.ieee80211.ap_channel_center_frequency_index_2);
                }
                else if (
                          (MEDIA_TYPE_IEEE_1901_WAVELET == p->media_types[i].media_type) ||
                          (MEDIA_TYPE_IEEE_1901_FFT     == p->media_types[i].media_type)
                        )
                {
                    callback(write_function, new_prefix, sizeof(p->media_types[i].media_specific_data.ieee1901.network_identifier), "network_identifier", "0x%02x", p->media_types[i].media_specific_data.ieee1901.network_identifier);
                }

            }

            return;
        }

        case TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION:
        {
            struct pushButtonJoinNotificationTLV *p;

            p = (struct pushButtonJoinNotificationTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->al_mac_address),     "al_mac_address",      "0x%02x",   p->al_mac_address);
            callback(write_function, tlv_prefix, sizeof(p->message_identifier), "message_identifier",  "%d",      &p->message_identifier);
            callback(write_function, tlv_prefix, sizeof(p->mac_address),        "mac_address",         "0x%02x",   p->mac_address);
            callback(write_function, tlv_prefix, sizeof(p->new_mac_address),    "new_mac_address",     "0x%02x",   p->new_mac_address);
            return;
        }

        case TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION:
        {
            struct genericPhyDeviceInformationTypeTLV *p;
            uint8_t i;

            p = (struct genericPhyDeviceInformationTypeTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->al_mac_address),      "al_mac_address",      "0x%02x",  &p->al_mac_address);
            callback(write_function, tlv_prefix, sizeof(p->local_interfaces_nr), "local_interfaces_nr", "%d",      &p->local_interfaces_nr);
            for (i=0; i < p->local_interfaces_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%slocal_interfaces[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].local_interface_address),                         "local_interface_address",             "0x%02x",   p->local_interfaces[i].local_interface_address);
                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].generic_phy_common_data.oui),                     "oui",                                 "0x%02x",   p->local_interfaces[i].generic_phy_common_data.oui);
                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].generic_phy_common_data.variant_index),           "variant_index",                       "%d",      &p->local_interfaces[i].generic_phy_common_data.variant_index);
                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].variant_name),                                    "variant_name",                        "%s",       p->local_interfaces[i].variant_name);
                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].generic_phy_description_xml_url_len),             "generic_phy_description_xml_url_len", "%d",      &p->local_interfaces[i].generic_phy_description_xml_url_len);
                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr), "media_specific_bytes_nr",             "%d",      &p->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr);
                callback(write_function, new_prefix, p->local_interfaces[i].generic_phy_description_xml_url_len,                     "generic_phy_description_xml_url",     "%s",       p->local_interfaces[i].generic_phy_description_xml_url);
                callback(write_function, new_prefix, p->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr,         "media_specific_bytes",                "0x%02x",   p->local_interfaces[i].generic_phy_common_data.media_specific_bytes);
            }

            return;
        }

        case TLV_TYPE_DEVICE_IDENTIFICATION:
        {
            struct deviceIdentificationTypeTLV *p;

            p = (struct deviceIdentificationTypeTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->friendly_name),      "friendly_name",       "%s",   p->friendly_name);
            callback(write_function, tlv_prefix, sizeof(p->manufacturer_name),  "manufacturer_name",   "%s",   p->manufacturer_name);
            callback(write_function, tlv_prefix, sizeof(p->manufacturer_model), "manufacturer_model",  "%s",   p->manufacturer_model);
            return;
        }

        case TLV_TYPE_CONTROL_URL:
        {
            struct controlUrlTypeTLV *p;

            p = (struct controlUrlTypeTLV *)tlv;

            callback(write_function, tlv_prefix, strlen(p->url)+1, "url", "%s", p->url);

            return;
        }

        case TLV_TYPE_IPV4:
        {
            struct ipv4TypeTLV *p;
            uint8_t i, j;

            p = (struct ipv4TypeTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->ipv4_interfaces_nr), "ipv4_interfaces_nr", "%d",  &p->ipv4_interfaces_nr);
            for (i=0; i < p->ipv4_interfaces_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%sipv4_interfaces[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->ipv4_interfaces[i].mac_address), "mac_address", "0x%02x",   p->ipv4_interfaces[i].mac_address);
                callback(write_function, new_prefix, sizeof(p->ipv4_interfaces[i].ipv4_nr),     "ipv4_nr",     "%d",      &p->ipv4_interfaces[i].ipv4_nr);

                for (j=0; j < p->ipv4_interfaces[i].ipv4_nr; j++)
                {
                    snprintf(new_prefix, MAX_PREFIX-1, "%sipv4_interfaces[%d]->ipv4[%d]->", tlv_prefix, i, j);
                    new_prefix[MAX_PREFIX-1] = 0x0;

                    callback(write_function, new_prefix, sizeof(p->ipv4_interfaces[i].ipv4[j].type),             "type",             "%d",     &p->ipv4_interfaces[i].ipv4[j].type);
                    callback(write_function, new_prefix, sizeof(p->ipv4_interfaces[i].ipv4[j].ipv4_address),     "ipv4_address",     "%ipv4",   p->ipv4_interfaces[i].ipv4[j].ipv4_address);
                    callback(write_function, new_prefix, sizeof(p->ipv4_interfaces[i].ipv4[j].ipv4_dhcp_server), "ipv4_dhcp_server", "%ipv4",   p->ipv4_interfaces[i].ipv4[j].ipv4_dhcp_server);
                }
            }

            return;
        }

        case TLV_TYPE_IPV6:
        {
            struct ipv6TypeTLV *p;
            uint8_t i, j;

            p = (struct ipv6TypeTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->ipv6_interfaces_nr), "ipv6_interfaces_nr", "%d",  &p->ipv6_interfaces_nr);
            for (i=0; i < p->ipv6_interfaces_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%sipv6_interfaces[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->ipv6_interfaces[i].mac_address), "mac_address", "0x%02x",   p->ipv6_interfaces[i].mac_address);
                callback(write_function, new_prefix, sizeof(p->ipv6_interfaces[i].ipv6_nr),     "ipv6_nr",     "%d",      &p->ipv6_interfaces[i].ipv6_nr);

                for (j=0; j < p->ipv6_interfaces[i].ipv6_nr; j++)
                {
                    snprintf(new_prefix, MAX_PREFIX-1, "%sipv6_interfaces[%d]->ipv6[%d]->", tlv_prefix, i, j);
                    new_prefix[MAX_PREFIX-1] = 0x0;

                    callback(write_function, new_prefix, sizeof(p->ipv6_interfaces[i].ipv6[j].type),                "type",                "%d",      &p->ipv6_interfaces[i].ipv6[j].type);
                    callback(write_function, new_prefix, sizeof(p->ipv6_interfaces[i].ipv6[j].ipv6_address),        "ipv6_address",        "0x%02x",   p->ipv6_interfaces[i].ipv6[j].ipv6_address);
                    callback(write_function, new_prefix, sizeof(p->ipv6_interfaces[i].ipv6[j].ipv6_address_origin), "ipv6_address_origin", "0x%02x",   p->ipv6_interfaces[i].ipv6[j].ipv6_address_origin);
                }
            }

            return;
        }

        case TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION:
        {
            struct pushButtonGenericPhyEventNotificationTLV *p;
            uint8_t i;

            p = (struct pushButtonGenericPhyEventNotificationTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->local_interfaces_nr), "local_interfaces_nr", "%d",  &p->local_interfaces_nr);
            for (i=0; i < p->local_interfaces_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%slocal_interfaces[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].oui),                     "oui",                     "0x%02x",   p->local_interfaces[i].oui);
                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].variant_index),           "variant_index",           "%d",      &p->local_interfaces[i].variant_index);
                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].media_specific_bytes_nr), "media_specific_bytes_nr", "%d",      &p->local_interfaces[i].media_specific_bytes_nr);
                callback(write_function, new_prefix, p->local_interfaces[i].media_specific_bytes_nr,         "media_specific_bytes",    "0x%02x",   p->local_interfaces[i].media_specific_bytes);
            }

            return;
        }

        case TLV_TYPE_1905_PROFILE_VERSION:
        {
            struct x1905ProfileVersionTLV *p;

            p = (struct x1905ProfileVersionTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->profile), "profile",  "%d",  &p->profile);

            return;
        }

        case TLV_TYPE_POWER_OFF_INTERFACE:
        {
            struct powerOffInterfaceTLV *p;
            uint8_t i;

            p = (struct powerOffInterfaceTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->power_off_interfaces_nr), "power_off_interfaces_nr", "%d",  &p->power_off_interfaces_nr);
            for (i=0; i < p->power_off_interfaces_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%spower_off_interfaces[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->power_off_interfaces[i].interface_address),                               "interface_address",       "0x%02x",   p->power_off_interfaces[i].interface_address);
                callback(write_function, new_prefix, sizeof(p->power_off_interfaces[i].media_type),                                      "media_type",              "0x%04x",  &p->power_off_interfaces[i].media_type);
                callback(write_function, new_prefix, sizeof(p->power_off_interfaces[i].generic_phy_common_data.oui),                     "oui",                     "0x%02x",   p->power_off_interfaces[i].generic_phy_common_data.oui);
                callback(write_function, new_prefix, sizeof(p->power_off_interfaces[i].generic_phy_common_data.variant_index),           "variant_index",           "%d",      &p->power_off_interfaces[i].generic_phy_common_data.variant_index);
                callback(write_function, new_prefix, sizeof(p->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr), "media_specific_bytes_nr", "%d",      &p->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr);
                callback(write_function, new_prefix, p->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr,         "media_specific_bytes",    "0x%02x",   p->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes);
            }

            return;
        }

        case TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION:
        {
            struct interfacePowerChangeInformationTLV *p;
            uint8_t i;

            p = (struct interfacePowerChangeInformationTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->power_change_interfaces_nr), "power_change_interfaces_nr", "%d",  &p->power_change_interfaces_nr);
            for (i=0; i < p->power_change_interfaces_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%spower_change_interfaces[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->power_change_interfaces[i].interface_address),     "interface_address",       "0x%02x",   p->power_change_interfaces[i].interface_address);
                callback(write_function, new_prefix, sizeof(p->power_change_interfaces[i].requested_power_state), "requested_power_state",   "0x%02x",  &p->power_change_interfaces[i].requested_power_state);
            }

            return;
        }

        case TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS:
        {
            struct interfacePowerChangeStatusTLV *p;
            uint8_t i;

            p = (struct interfacePowerChangeStatusTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->power_change_interfaces_nr), "power_change_interfaces_nr", "%d",  &p->power_change_interfaces_nr);
            for (i=0; i < p->power_change_interfaces_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%spower_change_interfaces[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->power_change_interfaces[i].interface_address), "interface_address",  "0x%02x",  p->power_change_interfaces[i].interface_address);
                callback(write_function, new_prefix, sizeof(p->power_change_interfaces[i].result),            "result",             "%d",     &p->power_change_interfaces[i].result);
            }

            return;
        }

        case TLV_TYPE_L2_NEIGHBOR_DEVICE:
        {
            struct l2NeighborDeviceTLV *p;
            uint8_t i, j, k;

            p = (struct l2NeighborDeviceTLV *)tlv;

            callback(write_function, tlv_prefix, sizeof(p->local_interfaces_nr), "local_interfaces_nr", "%d",  &p->local_interfaces_nr);
            for (i=0; i < p->local_interfaces_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%slocal_interfaces[%d]->", tlv_prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].local_mac_address), "local_mac_address", "0x%02x",   p->local_interfaces[i].local_mac_address);
                callback(write_function, new_prefix, sizeof(p->local_interfaces[i].l2_neighbors_nr),   "l2_neighbors_nr",   "%d",      &p->local_interfaces[i].l2_neighbors_nr);

                for (j=0; j < p->local_interfaces[i].l2_neighbors_nr; j++)
                {
                    snprintf(new_prefix, MAX_PREFIX-1, "%slocal_interfaces[%d]->l2_neighbors[%d]->", tlv_prefix, i, j);
                    new_prefix[MAX_PREFIX-1] = 0x0;

                    callback(write_function, new_prefix, sizeof(p->local_interfaces[i].l2_neighbors[j].l2_neighbor_mac_address), "l2_neighbor_mac_address", "0x%02x",   p->local_interfaces[i].l2_neighbors[j].l2_neighbor_mac_address);
                    callback(write_function, new_prefix, sizeof(p->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr), "behind_mac_addresses_nr", "%d",      &p->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr);

                    for (k=0; k < p->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr; k++)
                    {
                        snprintf(new_prefix, MAX_PREFIX-1, "%slocal_interfaces[%d]->l2_neighbors[%d]->behind_mac_addresses[%d]", tlv_prefix, i, j, k);
                        new_prefix[MAX_PREFIX-1] = 0x0;

                        callback(write_function, new_prefix, 6, "behind_mac_addresses", "0x%02x", p->local_interfaces[i].l2_neighbors[j].behind_mac_addresses[k]);
                    }
                }
            }

            return;
        }

        default:
        {
            DECLARE_HLIST_HEAD(dummy);
            tlv_add(tlv_1905_defs, &dummy, tlv);
            tlv_print(tlv_1905_defs, &dummy, write_function, prefix);
            hlist_head_init(&tlv->s.h.l);

            return;
        }
    }

    // This code cannot be reached
    //
    return;
}

const char *convert_1905_TLV_type_to_string(uint8_t tlv_type)
{
    switch (tlv_type)
    {
        case TLV_TYPE_DEVICE_INFORMATION_TYPE:
            return "TLV_TYPE_DEVICE_INFORMATION_TYPE";
        case TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES:
            return "TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES";
        case TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST:
            return "TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST";
        case TLV_TYPE_NEIGHBOR_DEVICE_LIST:
            return "TLV_TYPE_NEIGHBOR_DEVICE_LIST";
        case TLV_TYPE_TRANSMITTER_LINK_METRIC:
            return "TLV_TYPE_TRANSMITTER_LINK_METRIC";
        case TLV_TYPE_RECEIVER_LINK_METRIC:
            return "TLV_TYPE_RECEIVER_LINK_METRIC";
        case TLV_TYPE_LINK_METRIC_RESULT_CODE:
            return "TLV_TYPE_LINK_METRIC_RESULT_CODE";
        case TLV_TYPE_SEARCHED_ROLE:
            return "TLV_TYPE_SEARCHED_ROLE";
        case TLV_TYPE_AUTOCONFIG_FREQ_BAND:
            return "TLV_TYPE_AUTOCONFIG_FREQ_BAND";
        case TLV_TYPE_SUPPORTED_ROLE:
            return "TLV_TYPE_SUPPORTED_ROLE";
        case TLV_TYPE_SUPPORTED_FREQ_BAND:
            return "TLV_TYPE_SUPPORTED_FREQ_BAND";
        case TLV_TYPE_WSC:
            return "TLV_TYPE_WSC";
        case TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION:
            return "TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION";
        case TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION:
            return "TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION";
        case TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION:
            return "TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION";
        case TLV_TYPE_DEVICE_IDENTIFICATION:
            return "TLV_TYPE_DEVICE_IDENTIFICATION";
        case TLV_TYPE_CONTROL_URL:
            return "TLV_TYPE_CONTROL_URL";
        case TLV_TYPE_IPV4:
            return "TLV_TYPE_IPV4";
        case TLV_TYPE_IPV6:
            return "TLV_TYPE_IPV6";
        case TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION:
            return "TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION";
        case TLV_TYPE_1905_PROFILE_VERSION:
            return "TLV_TYPE_1905_PROFILE_VERSION";
        case TLV_TYPE_POWER_OFF_INTERFACE:
            return "TLV_TYPE_POWER_OFF_INTERFACE";
        case TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION:
            return "TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION";
        case TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS:
            return "TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS";
        case TLV_TYPE_L2_NEIGHBOR_DEVICE:
            return "TLV_TYPE_L2_NEIGHBOR_DEVICE";
        default:
        {
            const struct tlv_def *tlv_def = tlv_find_def(tlv_1905_defs, tlv_type);
            if (tlv_def == NULL)
            {
                return "Unknown";
            }
            else
            {
                return tlv_def->desc.name;
            }
        }
    }

    // This code cannot be reached
    //
    return "";
}

