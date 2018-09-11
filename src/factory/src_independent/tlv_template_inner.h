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

/** @internal
 * This is the inner kitchen of tlv_template.h. No user serviceable components inside.
 */

#ifndef TLV_TEMPLATE_INNER_H
#define TLV_TEMPLATE_INNER_H

#include <tlv.h>
#include <platform.h>
#include <packet_tools.h>
#include <utils.h>

#define TLV_TEMPLATE_FIELD_FUNCTION_NAME_INNER2(func,n,tlv_name) \
    tlv_##func##_##field##n##_##tlv_name
#define TLV_TEMPLATE_FIELD_FUNCTION_NAME_INNER(func,n,tlv_name) \
    TLV_TEMPLATE_FIELD_FUNCTION_NAME_INNER2(func,n,tlv_name)
#define TLV_TEMPLATE_FIELD_FUNCTION_NAME(func) \
    TLV_TEMPLATE_FIELD_FUNCTION_NAME_INNER(func,TLV_FIELD_NUM,TLV_NAME)

#define TLV_TEMPLATE_ExBL_INNER(length) _E ## length ## BL
#define TLV_TEMPLATE_ExBL(length) TLV_TEMPLATE_ExBL_INNER(length)

#define TLV_TEMPLATE_IxBL_INNER(length) _I ## length ## BL
#define TLV_TEMPLATE_IxBL(length) TLV_TEMPLATE_IxBL_INNER(length)

#endif // TLV_TEMPLATE_INNER_H

#ifndef TLV_NAME
/* Include an example here, which will define TLV_NAME some fields. This way, an IDE that interprets
 * macros will do proper syntax highlighting of the code in this file.
 */
struct exampleTLV {
    struct tlv tlv;
    uint16_t   field1;
    uint8_t    field2[6];
};

#define TLV_NAME          example
#define TLV_NAME                 example
#define TLV_TEMPLATE_STRUCT_NAME struct exampleTLV
#define TLV_FIELD1_NAME          field1
#define TLV_FIELD_NAME           TLV_FIELD1_NAME
#define TLV_FIELD_NUM            1
#define TLV_FIELD1_LENGTH        2
#define TLV_FIELD_LENGTH         TLV_FIELD1_LENGTH
#define TLV_PARSE                1
#define TLV_FORGE                2
#define TLV_PRINT                0
#define TLV_COMPARE              0

#define TLV_TEMPLATE_STR_INNER(s) #s
#define TLV_TEMPLATE_STR(s) TLV_TEMPLATE_STR_INNER(s)

#endif // TLV_NAME

#define TLV_FIELD_MASK (1 << (TLV_FIELD_NUM - 1)) // Just to simplify the macros below

#if !((TLV_PARSE) & TLV_FIELD_MASK)
static bool TLV_TEMPLATE_FIELD_FUNCTION_NAME(parse)(const struct tlv_def *def __attribute__((unused)),
                                                    TLV_TEMPLATE_STRUCT_NAME *self,
                                                    const uint8_t **buf,
                                                    size_t *length)
{
#if TLV_FIELD_LENGTH > 0
    if (!TLV_TEMPLATE_ExBL(TLV_FIELD_LENGTH)(buf, &self->TLV_FIELD_NAME, length))
#else
    if (!_EnBL(buf, self->TLV_FIELD_NAME, sizeof(self->TLV_FIELD_NAME), length))
#endif
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Malformed %s TLV: no " TLV_TEMPLATE_STR(TLV_FIELD_NAME) "\n", def->name);
        return false;
    }
    return true;
}
#endif // TLV_PARSE


#if !((TLV_FORGE) & TLV_FIELD_MASK)
static bool TLV_TEMPLATE_FIELD_FUNCTION_NAME(forge)(const TLV_TEMPLATE_STRUCT_NAME *self,
                                                    uint8_t **buf,
                                                    size_t *length)
{
#if TLV_FIELD_LENGTH > 0
    if (!TLV_TEMPLATE_IxBL(TLV_FIELD_LENGTH)(&self->TLV_FIELD_NAME, buf, length))
        return false;
#else // TLV_FIELD_LENGTH
    if (!_InBL(self->TLV_FIELD_NAME, buf, sizeof(self->TLV_FIELD_NAME), length))
        return false;
#endif // TLV_FIELD_LENGTH
    return true;
}
#endif // TLV_FORGE

#if !((TLV_PRINT) & TLV_FIELD_MASK)
static void TLV_TEMPLATE_FIELD_FUNCTION_NAME(print)(const TLV_TEMPLATE_STRUCT_NAME *self,
                                                    void (*write_function)(const char *fmt, ...),
                                                    const char *prefix)
{
#if TLV_FIELD_LENGTH > 0
    print_callback(write_function, prefix, TLV_FIELD_LENGTH, TLV_TEMPLATE_STR(TLV_FIELD_NAME),
                   "%d", &self->TLV_FIELD_NAME);
#else
    print_callback(write_function, prefix, sizeof(self->TLV_FIELD_NAME), TLV_TEMPLATE_STR(TLV_FIELD_NAME),
                   "0x%02x", &self->TLV_FIELD_NAME);
#endif
}
#endif

#if !((TLV_COMPARE) & TLV_FIELD_MASK) && !defined(TLV_NEW)
static bool TLV_TEMPLATE_FIELD_FUNCTION_NAME(compare)(const TLV_TEMPLATE_STRUCT_NAME *self1,
                                                      const TLV_TEMPLATE_STRUCT_NAME *self2)
{
#if TLV_FIELD_LENGTH > 0
    return (self1->TLV_FIELD1_NAME == self2->TLV_FIELD1_NAME);
#else
    return (memcmp(self1->TLV_FIELD_NAME, self2->TLV_FIELD_NAME, sizeof(self1->TLV_FIELD_NAME)) == 0);
#endif
}
#endif

#undef TLV_FIELD_NAME
#undef TLV_FIELD_LENGTH
#undef TLV_FIELD_NUM
#undef TLV_FIELD_MASK
