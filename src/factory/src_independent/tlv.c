#include "tlv.h"

#include <packet_tools.h>
#include <utils.h>

#include <errno.h> // errno
#include <stdlib.h> // malloc
#include <string.h> // strerror

struct tlv_list
{
    size_t tlv_nr;
    struct tlv **tlvs;
};

const struct tlv_def *tlv_find_def(tlv_defs_t defs, uint8_t tlv_type)
{
    return &defs[tlv_type];
}

const struct tlv_def *tlv_find_tlv_def(tlv_defs_t defs, const struct tlv *tlv)
{
    return tlv_find_def(defs, tlv->type);
}

struct tlv_list *tlv_parse(tlv_defs_t defs, const uint8_t *buffer, size_t length)
{
    struct tlv_list *ret = malloc(sizeof(struct tlv_list));

    if (ret == NULL)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Failed to allocate tlv_list: (%d) %s\n", errno, strerror(errno));
        return ret;
    }
    ret->tlv_nr = 0;
    ret->tlvs = NULL;

    while (length >= 3)    // Minimal TLV: 1 byte type, 2 bytes length
    {
        uint8_t tlv_type;
        uint16_t tlv_length;
        const struct tlv_def *tlv_def;
        struct tlv *tlv_new;

        _E1BL(&buffer, &tlv_type, &length);
        _E2BL(&buffer, &tlv_length, &length);
        if (tlv_length > length)
        {
            PLATFORM_PRINTF_DEBUG_ERROR("TLV(%u) of length %u but only %u bytes left in buffer\n",
                                        tlv_type, tlv_length, (unsigned)length);
            goto err_out;
        }

        tlv_def = tlv_find_def(defs, tlv_type);
        if (tlv_def->name == NULL)
        {
            struct tlv_unknown *tlv;
            PLATFORM_PRINTF_DEBUG_WARNING("Unknown TLV type %u of length %u\n",
                                          (unsigned)tlv_type, (unsigned)tlv_length);
            tlv = PLATFORM_MALLOC(sizeof(struct tlv_unknown));
            tlv->value = PLATFORM_MALLOC(tlv_length);
            tlv->length = tlv_length;
            memcpy(tlv->value, buffer, tlv_length);
            tlv_new = &tlv->tlv;
        }
        else if (tlv_def->parse == NULL)
        {
            /* Default parse function only works for 0-length TLVs */
            if (tlv_length == 0)
            {
                tlv_new = PLATFORM_MALLOC(sizeof(struct tlv));
            }
            else
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Implementation error: no parse function for TLV %s length %u\n",
                                            tlv_def->name, (unsigned)tlv_length);
                goto err_out;
            }
        }
        else
        {
            tlv_new = tlv_def->parse(tlv_def, buffer, tlv_length);
        }
        if (tlv_new == NULL)
        {
            goto err_out;
        }
        tlv_new->type = tlv_type;
        if (!tlv_add(defs, ret, tlv_new))
        {
            /* tlv_add already prints an error */
            tlv_def ? tlv_def->free(tlv_new) : free(tlv_new);
            goto err_out;
        }
        buffer += tlv_length;
        length -= tlv_length;
    }

    return ret;

err_out:
    tlv_free(defs, ret);
    return NULL;
}

bool tlv_forge(tlv_defs_t defs, const struct tlv_list *tlvs, size_t max_length, uint8_t **buffer, size_t *length)
{
    size_t i;
    size_t total_length;
    uint8_t *p;

    /* First, calculate total_length. */
    total_length = 0;
    for (i = 0; i < tlvs->tlv_nr; i++)
    {
        const struct tlv *tlv = tlvs->tlvs[i];
        const struct tlv_def *tlv_def = tlv_find_tlv_def(defs, tlv);
        if (tlv_def->name == NULL)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("tlv_forge: skipping unknown TLV %u\n", tlv->type);
        }
        else if (tlv_def->length == NULL)
        {
            /* Assume 0-length TLV */
            total_length += 3;
        }
        else if (tlv_def->forge == NULL)
        {
            PLATFORM_PRINTF_DEBUG_ERROR("No forge defined for TVL %s\n", tlv_def->name);
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
    for (i = 0; i < tlvs->tlv_nr; i++)
    {
        const struct tlv *tlv = tlvs->tlvs[i];
        const struct tlv_def *tlv_def = tlv_find_tlv_def(defs, tlv);
        uint16_t tlv_length = tlv_def->length(tlv);

        if (!_I1BL(&tlv->type, &p, &total_length))
            goto err_out;
        if (!_I2BL(&tlv_length, &p, &total_length))
            goto err_out;
        if (!tlv_def->forge(tlv, &p, &total_length))
            goto err_out;
    }
    if (total_length != 0)
        goto err_out;
    return true;

err_out:
    PLATFORM_PRINTF_DEBUG_ERROR("TLV list forging implementation error.\n");
    free(*buffer);
    return false;
}

void tlv_print(tlv_defs_t defs, const struct tlv_list *tlvs, void (*write_function)(const char *fmt, ...), const char *prefix)
{
    size_t i;

    for (i = 0; i < tlvs->tlv_nr; i++)
    {
        struct tlv *tlv = tlvs->tlvs[i];
        const struct tlv_def *tlv_def = tlv_find_tlv_def(defs, tlv);
        // In order to make it easier for the callback() function to present
        // useful information, append the type of the TLV to the prefix
        //
        char new_prefix[100];

        PLATFORM_SNPRINTF(new_prefix, sizeof(new_prefix)-1, "%sTLV(%s)->",
                          prefix, (tlv_def->name == NULL) ? "Unknown" : tlv_def->name);
        new_prefix[sizeof(new_prefix)-1] = '\0';

        if (tlv_def->print == NULL)
        {
            write_function("%s\n", new_prefix);
        }
        else
        {
            tlv_def->print(tlv, write_function, new_prefix);
        }
    }
}

void tlv_free(tlv_defs_t defs, struct tlv_list *tlvs)
{
    size_t i;

    if (tlvs == NULL)
        return;

    for (i = 0; i < tlvs->tlv_nr; i++)
    {
        struct tlv *tlv = tlvs->tlvs[i];
        const struct tlv_def *tlv_def = tlv_find_tlv_def(defs, tlv);
        if (tlv_def->free == NULL)
        {
            PLATFORM_FREE(tlv);
        }
        else
        {
            tlv_def->free(tlv);
        }
    }
    PLATFORM_FREE(tlvs->tlvs);
    PLATFORM_FREE(tlvs);
}

bool tlv_compare(tlv_defs_t defs, const struct tlv_list *tlvs1, const struct tlv_list *tlvs2)
{
    size_t i;

    /** @todo this assumes TLVs are ordered the same */

    if (tlvs1->tlv_nr != tlvs2->tlv_nr)
    {
        return false;
    }

    for (i = 0; i < tlvs1->tlv_nr; i++)
    {
        struct tlv *tlv1 = tlvs1->tlvs[i];
        struct tlv *tlv2 = tlvs2->tlvs[i];
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
        // else assume equal
    }

    return true;
}

bool tlv_add(tlv_defs_t defs, struct tlv_list *tlvs, struct tlv *tlv)
{
    /** @todo keep ordered, check for duplicates, handle aggregation */
    tlvs->tlv_nr++;
    tlvs->tlvs = PLATFORM_REALLOC(tlvs->tlvs, tlvs->tlv_nr * sizeof(struct tlv*));
    tlvs->tlvs[tlvs->tlv_nr - 1] = tlv;
    return true;
}
