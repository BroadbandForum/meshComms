/*
 *  prplMesh Wi-Fi Multi-AP
 *
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

#include "tlv.h"

#include <packet_tools.h>
#include <utils.h>
#include <platform.h>

#include <errno.h> // errno
#include <stdlib.h> // malloc
#include <string.h> // memcpy, strerror
#include <stdio.h>  // snprintf

const struct tlv_def *tlv_find_def(tlv_defs_t defs, uint8_t tlv_type)
{
    return &defs[tlv_type];
}

const struct tlv_def *tlv_find_tlv_def(tlv_defs_t defs, const struct tlv *tlv)
{
    return tlv_find_def(defs, tlv->type);
}


static bool tlv_parse_field(hlist_item *item, const hlist_field_description *desc, size_t size, const uint8_t **buffer, size_t *length)
{
    char *pfield = (char*)item + desc->offset;
    if (size == 0)
    {
        size = desc->size;
    }
    switch (size)
    {
        case 1:
            return _E1BL(buffer, (uint8_t*)pfield, length);
        case 2:
            return _E2BL(buffer, (uint16_t*)pfield, length);
        case 4:
            return _E4BL(buffer, (uint32_t*)pfield, length);
        default:
            return _EnBL(buffer, (uint32_t*)pfield, size, length);
    }
}

static hlist_item *tlv_parse_single(const hlist_description *desc, hlist_head *parent, const uint8_t **buffer, size_t *length)
{
    size_t i;

    hlist_item *item = hlist_alloc(desc, parent);
    for (i = 0; i < ARRAY_SIZE(item->desc->fields) && item->desc->fields[i].name != NULL; i++)
    {
        if (!tlv_parse_field(item, &item->desc->fields[i], 0, buffer, length))
            goto err_out;
    }
    for (i = 0; i < ARRAY_SIZE(item->children) && item->desc->children[i] != NULL; i++)
    {
        uint8_t children_nr;
        uint8_t j;

        _E1BL(buffer, &children_nr, length);
        for (j = 0; j < children_nr; j++)
        {
            const hlist_item *child = tlv_parse_single(item->desc->children[i], &item->children[i], buffer, length);
            if (child == NULL)
                goto err_out;
        }
    }
    return item;

err_out:
    hlist_delete_item(item);
    return NULL;
}

bool tlv_parse(tlv_defs_t defs, hlist_head *tlvs, const uint8_t *buffer, size_t length)
{
    while (length >= 3)    // Minimal TLV: 1 byte type, 2 bytes length
    {
        uint8_t tlv_type;
        uint16_t tlv_length_uint16;
        size_t tlv_length;
        const struct tlv_def *tlv_def;
        struct tlv *tlv_new;

        _E1BL(&buffer, &tlv_type, &length);
        _E2BL(&buffer, &tlv_length_uint16, &length);
        tlv_length = tlv_length_uint16;
        if (tlv_length > length)
        {
            PLATFORM_PRINTF_DEBUG_ERROR("TLV(%u) of length %u but only %u bytes left in buffer\n",
                                        tlv_type, tlv_length, (unsigned)length);
            goto err_out;
        }

        tlv_def = tlv_find_def(defs, tlv_type);
        if (tlv_def->desc.name == NULL)
        {
            struct tlv_unknown *tlv;
            PLATFORM_PRINTF_DEBUG_WARNING("Unknown TLV type %u of length %u\n",
                                          (unsigned)tlv_type, (unsigned)tlv_length);
            tlv = memalloc(sizeof(struct tlv_unknown));
            tlv->value = memalloc(tlv_length);
            tlv->length = tlv_length_uint16;
            memcpy(tlv->value, buffer, tlv_length);
            tlv_new = &tlv->tlv;
        }
        else if (tlv_def->parse == NULL)
        {
            /* Special case for 0-length TLVs */
            if (tlv_length == 0)
            {
                tlv_new = memalloc(sizeof(struct tlv));
            }
            else
            {
                /* @todo clean this up */
                length -= tlv_length;
                hlist_item *tlv_new_item = tlv_parse_single(&tlv_def->desc, NULL, &buffer, &tlv_length);
                if (tlv_new_item == NULL)
                    goto err_out;
                if (tlv_length != 0)
                {
                    PLATFORM_PRINTF_DEBUG_ERROR("Remaining garbage (%u bytes) after parsing TLV %s\n",
                                                (unsigned)tlv_length, tlv_def->desc.name);
                    hlist_delete_item(tlv_new_item);
                    goto err_out;
                }
                tlv_new = container_of(tlv_new_item, struct tlv, h);
            }
        }
        else
        {
            tlv_new = tlv_def->parse(tlv_def, buffer, tlv_length);
            /* @todo check no remaining bytes in tlv_length */
        }
        if (tlv_new == NULL)
        {
            goto err_out;
        }
        tlv_new->type = tlv_type;
        if (!tlv_add(defs, tlvs, tlv_new))
        {
            /* tlv_add already prints an error */
            tlv_def ? tlv_def->free(tlv_new) : free(tlv_new);
            goto err_out;
        }
        buffer += tlv_length;
        length -= tlv_length;
    }

    return true;

err_out:
    tlv_free(defs, tlvs);
    return false;
}


static bool tlv_forge_field(const hlist_item *item, const hlist_field_description *desc, size_t size, uint8_t **buffer, size_t *length)
{
    const char *pfield = (const char*)item + desc->offset;
    if (size == 0)
    {
        size = desc->size;
    }
    switch (size)
    {
        case 1:
            return _I1BL((const uint8_t*)pfield, buffer, length);
        case 2:
            return _I2BL((const uint16_t*)pfield, buffer, length);
        case 4:
            return _I4BL((const uint32_t*)pfield, buffer, length);
        default:
            return _InBL((const uint32_t*)pfield, buffer, size, length);
    }
}

static bool tlv_forge_single(const hlist_item *item, uint8_t **buffer, size_t *length)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(item->desc->fields) && item->desc->fields[i].name != NULL; i++)
    {
        if (!tlv_forge_field(item, &item->desc->fields[i], 0, buffer, length))
            return false;
    }
    for (i = 0; i < ARRAY_SIZE(item->children) && item->desc->children[i] != NULL; i++)
    {
        const hlist_item *child;
        size_t children_nr = hlist_count(&item->children[i]);
        uint8_t children_nr_uint8;
        if (children_nr > 255)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("TLV with more than 255 children.\n");
            return false;
        }
        children_nr_uint8 = children_nr;
        _I1BL(&children_nr_uint8, buffer, length);
        hlist_for_each_item(child, item->children[i])
        {
            if (!tlv_forge_single(child, buffer, length))
                return false;
        }
    }
    return true;
}

static uint16_t tlv_length_single(const hlist_item *item)
{
    uint16_t length = 0;
    size_t i;
    for (i = 0; i < ARRAY_SIZE(item->desc->fields) && item->desc->fields[i].name != NULL; i++)
    {
        /* If one of the fields has a different size in serialisation than in the struct, the length() method must be
         * overridden. */
        length += item->desc->fields[i].size;
    }
    /* Next print the children. */
    for (i = 0; i < ARRAY_SIZE(item->children) && item->desc->children[i] != NULL; i++)
    {
        const hlist_item *child;
        length += 1; /* num_children */
        hlist_for_each_item(child, item->children[i])
        {
            length += tlv_length_single(child);
        }
    }

    return length;
}

bool tlv_forge(tlv_defs_t defs, const hlist_head *tlvs, size_t max_length, uint8_t **buffer, size_t *length)
{
    size_t total_length;
    uint8_t *p;
    const struct tlv *tlv;

    /* First, calculate total_length. */
    total_length = 0;
    hlist_for_each(tlv, *tlvs, struct tlv, h)
    {
        const struct tlv_def *tlv_def = tlv_find_tlv_def(defs, tlv);
        if (tlv_def->desc.name == NULL)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("tlv_forge: skipping unknown TLV %u\n", tlv->type);
        }
        else if (tlv_def->length == NULL)
        {
            total_length += 3;
            total_length += tlv_length_single(&tlv->h);
        }
        else if (tlv_def->forge == NULL)
        {
            PLATFORM_PRINTF_DEBUG_ERROR("No forge defined for TVL %s\n", tlv_def->desc.name);
            return false;
        }
        else
        {
            /* Add 3 bytes for type + length */
            total_length += tlv_def->length(tlv) + 3;
        }
    }

    /* Now, allocate the buffer and fill it. */
    /** @todo support splitting over packets. */
    if (total_length > max_length)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("TLV list doesn't fit, %u > %u.\n", (unsigned)total_length, (unsigned)max_length);
        return false;
    }

    /** @todo foresee headroom */
    *length = total_length;
    *buffer = malloc(total_length);
    p = *buffer;
    hlist_for_each(tlv, *tlvs, struct tlv, h)
    {
        const struct tlv_def *tlv_def = tlv_find_tlv_def(defs, tlv);
        uint16_t tlv_length;
        if (tlv_def->length == NULL)
            tlv_length = tlv_length_single(&tlv->h);
        else
            tlv_length = tlv_def->length(tlv);

        if (!_I1BL(&tlv->type, &p, &total_length))
            goto err_out;
        if (!_I2BL(&tlv_length, &p, &total_length))
            goto err_out;
        if (tlv_def->forge == NULL)
        {
            if (!tlv_forge_single(&tlv->h, &p, &total_length))
                goto err_out;
        }
        else
        {
            if (!tlv_def->forge(tlv, &p, &total_length))
                goto err_out;
        }
    }
    if (total_length != 0)
        goto err_out;
    return true;

err_out:
    PLATFORM_PRINTF_DEBUG_ERROR("TLV list forging implementation error.\n");
    free(*buffer);
    return false;
}

void tlv_print(tlv_defs_t defs, const hlist_head *tlvs, void (*write_function)(const char *fmt, ...), const char *prefix)
{
    const struct tlv *tlv;

    hlist_for_each(tlv, *tlvs, struct tlv, h)
    {
        const struct tlv_def *tlv_def = tlv_find_tlv_def(defs, tlv);
        // In order to make it easier for the callback() function to present
        // useful information, append the type of the TLV to the prefix
        //
        char new_prefix[100];

        if (tlv_def->print == NULL)
        {
            /* @todo this is a hack waiting for the removal of the tlv_def->print function */
            snprintf(new_prefix, sizeof(new_prefix)-1, "%s%s", prefix, tlv_def->desc.name);
            hlist_print_item(&tlv->h, write_function, new_prefix);
        }
        else
        {
            snprintf(new_prefix, sizeof(new_prefix)-1, "%sTLV(%s)->",
                              prefix, (tlv_def->desc.name == NULL) ? "Unknown" : tlv_def->desc.name);
            new_prefix[sizeof(new_prefix)-1] = '\0';
            tlv_def->print(tlv, write_function, new_prefix);
        }
    }
}

void tlv_free(tlv_defs_t defs, hlist_head *tlvs)
{
    hlist_head *next = tlvs->next;

    /* Since we will free all items in the list, it is not needed to remove them from the list. However, the normal
     * hlist_for_each macro would use the next pointer after the item has been freed. Therefore, we use an open-coded
     * iteration here. */
    while (next != tlvs)
    {
        struct tlv *tlv = container_of(next, struct tlv, h.l);
        next = next->next;
        hlist_head_init(&tlv->h.l);
        const struct tlv_def *tlv_def = tlv_find_tlv_def(defs, tlv);
        if (tlv_def->free == NULL)
        {
            hlist_delete_item(&tlv->h);
        }
        else
        {
            tlv_def->free(tlv);
        }
    }
    /* We still have to make sure the list is empty, in case it is reused later. */
    hlist_head_init(tlvs);
}

bool tlv_compare(tlv_defs_t defs, const hlist_head *tlvs1, const hlist_head *tlvs2)
{
    /** @todo this assumes TLVs are ordered the same */
    bool ret = true;
    hlist_head *cur1;
    hlist_head *cur2;
    /* Open-code hlist_for_each because we need to iterate over both at once. */
    for (cur1 = tlvs1->next, cur2 = tlvs2->next;
         ret && cur1 != tlvs1 && cur2 != tlvs2;
         cur1 = cur1->next, cur2 = cur2->next)
    {
        struct tlv *tlv1 = container_of(cur1, struct tlv, h.l);
        struct tlv *tlv2 = container_of(cur2, struct tlv, h.l);
        const struct tlv_def *tlv_def = tlv_find_def(defs, tlv1->type);
        if (tlv1->type != tlv2->type)
        {
            return false;
        }
        if (tlv_def->compare != NULL)
        {
            if (!tlv_def->compare(tlv1, tlv2))
            {
                return false;
            }
        }
        else
        {
            ret = HLIST_COMPARE_ITEM(tlv1, tlv2, h) == 0;
        }
    }
    return ret;
}

bool tlv_add(tlv_defs_t defs, hlist_head *tlvs, struct tlv *tlv)
{
    /** @todo keep ordered, check for duplicates, handle aggregation */
    hlist_add_tail(tlvs, &tlv->h);
    return true;
}
