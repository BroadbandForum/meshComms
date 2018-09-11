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

/** @file
 * @brief Template of TLV definitions.
 *
 * To define a TLV, several functions have to be defined. These functions are always very similar to each other. This
 * file is a template that can be used to automate the definition of the TLV functions. This template defines
 * implementations for all virtual functions defined in tlv_def.
 *
 * This file is named tlv_template.h to make sure it is seen as a C file that can be included. However, it is meant to
 * be included multiple times, so there are no include guards.
 *
 * The template is to be used as in the following example.
 * @code
 * #define TLV_NAME        foo
 * #define TLV_FIELD1_NAME field1
 * #include <tlv_template.h>
 *
 * #define TLV_NAME          bar
 * #define TLV_FIELD1_NAME   bar1
 * #define TLV_FIELD1_LENGTH 2
 * #define TLV_FIELD2_NAME   bar2
 * #define TLV_PARSE         2
 *
 * bool tlv_parse_field2_foo(const struct tlv_def *def __attribute__((unused)),
 *                           struct fooTLV *self,
 *                           const uint8_t **buf,
 *                           size_t *length)
 * {
 *     uint8_t bar2;
 *     _E1BL(buf, &bar2, length);
 *     if (bar2 > 0x03)
 *     {
 *         PLATFORM_PRINTF_DEBUG_ERROR("Invalid bar2: 0x%02x\n", self->bar2);
 *         return false;
 *     }
 *     self->bar2 = bar2;
 *     return true;
 * }
 *
 * #include <tlv_template.h>
 * @endcode
 *
 * Before including tlv_template.h, the macros TLV_NAME and TLV_FIELD1_NAME must be defined. More macros may be defined
 * to further tweak the functions.
 */

/** @def TLV_NAME
 * @brief The name of the TLV type to instantiate.
 *
 * When TLV_NAME is set to fooBar, the corresponding struct type will be @a fooBarTLV, and the functions will be
 * called @a tlv_parse_fooBar etc.
 */

/** @def TLV_FIELD1_NAME
 * @brief The name of the first field of the TLV value.
 *
 * At least one field must be defined, so this macro must be defined.
 *
 * This will be used as the member name in the struct, and also for printing.
 */

/** @def TLV_FIELD1_LENGTH
 * @brief Define the field as a fixed-with basic type.
 *
 * This macro can be set to 1, 2 or 4. If it is defined, default parse, length, forge, print and compare fragments
 * will be generated using uint8_t, uint16_t or uint32_t. The parse and forge functions will do endianness conversion.
 *
 * If this macro is not defined, default parse, length, forge, print and compare fragments will be generated based on
 * the size of the struct member. This is appropriate for e.g. MAC addresses.
 *
 * For more complicated fields, including dynamic length, define TLV_PARSE, TLV_FORGE, TLV_PRINT, and TLV_COMPARE
 * macros, and define parse, forge, print and compare functions for the specific fields.
 *
 * For dynamically allocated sub-structures you also need to override the free and the length methods. Since these are
 * really simple, you have to override the complete length and free functions, and define TLV_LENGTH_BODY and
 * TLV_FREE_BODY to signal they are overridden.
 */

/** @def TLV_PARSE
 * @brief Use a custom parse function for the fields in this bitmask.
 *
 * If this macro is defined, you must define a custom parse function for the fields in this bitmask. For example, to
 * create a custom parse function for fields 1 and 3, set TLV_PARSE to 0b101.
 *
 * The function for parsing field1 is called tlv_parse_field1_fooBar.
 *
 * @fn static bool tlv_parse_field1_fooBar(const struct tlv_def *def __attribute__((unused)),
 *                                         struct fooBarTLV *self,
 *                                         const uint8_t **buf,
 *                                         size_t *length)
 *
 * @brief custom parse function for fooBar.
 *
 * @param def TLV definition structure of fooBar.
 *
 * @param self Pointer to a freshly created structure of the right type, to be updated by this fragment.
 *
 * @param buf Buffer from which to parse. To be passed to _ExBL().
 *
 * @param length Remaining length of @a buffer. To be passed to _ExBL().
 *
 * @return true if parsed successfully, false if something is wrong with the stream. In that case, @a self will be
 * freed.
 */

/** @def TLV_PARSE_BODY(self,buffer,length)
 * @brief Custom parse function.
 *
 * @param self Pointer to a freshly created structure of the right type, to be updated by this fragment.
 *
 * @param buffer Buffer from which to parse. To be passed to _ExBL().
 *
 * @param length Remaining length of @a buffer. To be passed to _ExBL().
 *
 * This macro overrides all of the automatically generated fragments. In case of error, the fragment should print
 * something and goto err_out.
 */

/** @def TLV_LENGTH_BODY(self)
 * @brief Custom length function.
 *
 * @param self Pointer to the TLV, cast to the correct type.
 *
 * This macro overrides all the of the automatically generated fragments to calculate the length. No equivalent macro
 * exists for individual fields: if it is necessary to override field length, it is generally easier to define
 * everything together.
 */

/** @def TLV_FORGE
 * @brief Use a custom forge function for the fields in this bitmask.
 *
 * If this macro is defined, you must define a custom forge function for the fields in this bitmask. For example, to
 * create a custom forge function for fields 1 and 3, set TLV_FORGE to 0b101.
 *
 * The function for parsing field1 of fooBar is called tlv_forge_field1_fooBar.
 *
 * @fn static bool tlv_forge_field1_fooBar(const struct fooBarTLV *self,
 *                                         uint8_t **buf,
 *                                         size_t *length)
 *
 * @brief custom forge function for fooBar.
 *
 * @param self Pointer to the TLV, cast to the correct type.
 *
 * @param buf Buffer in which to forge. To be passed to _IxBL().
 *
 * @param length Remaining length of @a buf. To be passed to _IxBL().
 *
 *
 * @return true if forged successfully, false if something is wrong.
 */

/** @def TLV_FORGE_BODY(self,buf,length)
 * @brief Custom forge function.
 *
 * @param self Pointer to the TLV, cast to the correct type.
 *
 * @param buf Buffer in which to forge. To be passed to _IxBL().
 *
 * @param length Remaining length of @a buf. To be passed to _IxBL().
 *
 * This macro overrides all of the automatically generated fragments. In case of error, the fragment should return
 * false.
 */

/** @todo document TLV_PRINT, TLV_COMPARE, TLV_LENGTH, TLV_FREE */


/** @internal
 * These are internal macros which should be defined only once, so they're protected by include guards.
 * @{
 */
#ifndef TLV_TEMPLATE_H
#define TLV_TEMPLATE_H

#include <tlv.h>
#include <platform.h>
#include <packet_tools.h>
#include <utils.h>

#define TLV_TEMPLATE_STR_INNER(s) #s
#define TLV_TEMPLATE_STR(s) TLV_TEMPLATE_STR_INNER(s)

#define TLV_TEMPLATE_FUNCTION_NAME_INNER2(func,tlv_name) \
    tlv_##func##_##tlv_name
#define TLV_TEMPLATE_FUNCTION_NAME_INNER(func,tlv_name) TLV_TEMPLATE_FUNCTION_NAME_INNER2(func,tlv_name)
#define TLV_TEMPLATE_FUNCTION_NAME(func) \
    TLV_TEMPLATE_FUNCTION_NAME_INNER(func,TLV_NAME)

#define TLV_TEMPLATE_STRUCT_NAME_INNER2(tlv_name) \
    struct tlv_name ## TLV
#define TLV_TEMPLATE_STRUCT_NAME_INNER(tlv_name) TLV_TEMPLATE_STRUCT_NAME_INNER2(tlv_name)
#define TLV_TEMPLATE_STRUCT_NAME TLV_TEMPLATE_STRUCT_NAME_INNER(TLV_NAME)

#endif
/** @} */

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
#define TLV_LENGTH(self)  0
#define TLV_FIELD1_NAME   field1
#define TLV_FIELD1_LENGTH 2
#define TLV_FIELD2_NAME   field2

#endif

#ifndef TLV_FIELD1_NAME
#error "At least one TLV field must be defined"
#endif

#ifndef TLV_PARSE
#define TLV_PARSE 0
#endif
#ifndef TLV_FORGE
#define TLV_FORGE 0
#endif
#ifndef TLV_PRINT
#define TLV_PRINT 0
#endif
#ifndef TLV_COMPARE
#define TLV_COMPARE 0
#endif


#define TLV_FIELD_NAME    TLV_FIELD1_NAME
#define TLV_FIELD_LENGTH  TLV_FIELD1_LENGTH
#define TLV_FIELD_NUM     1
#include "tlv_template_inner.h"

#ifdef TLV_FIELD2_NAME
#define TLV_FIELD_NAME    TLV_FIELD2_NAME
#define TLV_FIELD_LENGTH  TLV_FIELD2_LENGTH
#define TLV_FIELD_NUM     2
#include "tlv_template_inner.h"
#endif // TLV_FIELD2_NAME

#ifdef TLV_FIELD3_NAME
#define TLV_FIELD_NAME    TLV_FIELD3_NAME
#define TLV_FIELD_LENGTH  TLV_FIELD3_LENGTH
#define TLV_FIELD_NUM     3
#include "tlv_template_inner.h"
#endif // TLV_FIELD3_NAME

static const struct hlist_description TLV_TEMPLATE_FUNCTION_NAME(desc) = {
    .name = TLV_TEMPLATE_STR(TLV_NAME),
    .size = sizeof(TLV_TEMPLATE_STRUCT_NAME),
    .fields = {HLIST_DESCRIBE_SENTINEL,},
    .children = {NULL,},
};

static struct tlv *TLV_TEMPLATE_FUNCTION_NAME(parse)(const struct tlv_def *def __attribute__((unused)),
                                                     const uint8_t *buffer __attribute__((unused)),
                                                     size_t length __attribute__((unused)))
{
    TLV_TEMPLATE_STRUCT_NAME *self = HLIST_ALLOC(&TLV_TEMPLATE_FUNCTION_NAME(desc), TLV_TEMPLATE_STRUCT_NAME, tlv.h, NULL);

#ifdef TLV_PARSE_BODY
    TLV_PARSE_BODY(self);
#undef TLV_PARSE_BODY
#else // TLV_PARSE_BODY

    if (!TLV_TEMPLATE_FUNCTION_NAME(parse_field1)(def, self, &buffer, &length))
        goto err_out;

#ifdef TLV_FIELD2_NAME
    if (!TLV_TEMPLATE_FUNCTION_NAME(parse_field2)(def, self, &buffer, &length))
        goto err_out;
#endif // TLV_FIELD2_NAME

#ifdef TLV_FIELD3_NAME
    if (!TLV_TEMPLATE_FUNCTION_NAME(parse_field3)(def, self, &buffer, &length))
        goto err_out;
#endif // TLV_FIELD3_NAME

    /* Check for trailing garbage that didn't get parsed. */
    if (length > 0)
        goto err_out;

#endif // TLV_PARSE_BODY

    return (struct tlv *)self;

err_out:
    if (def->free != NULL)
        def->free((struct tlv *)self);
    else
        free(self);
    return NULL;
}

static uint16_t TLV_TEMPLATE_FUNCTION_NAME(length)(const struct tlv *tlv)
{
    const TLV_TEMPLATE_STRUCT_NAME *self __attribute__((unused)) = (const TLV_TEMPLATE_STRUCT_NAME *)tlv;

#ifdef TLV_LENGTH_BODY
#undef TLV_LENGTH_BODY
    return TLV_TEMPLATE_FUNCTION_NAME(length_body)(self);
#else
    uint16_t ret = 0;

#ifdef TLV_FIELD1_LENGTH
    ret += TLV_FIELD1_LENGTH;
#else
    ret += sizeof(self->TLV_FIELD1_NAME);
#endif

#ifdef TLV_FIELD2_NAME
#ifdef TLV_FIELD2_LENGTH
    ret += TLV_FIELD2_LENGTH;
#else
    ret += sizeof(self->TLV_FIELD2_NAME);
#endif
#endif

#ifdef TLV_FIELD3_NAME
#ifdef TLV_FIELD3_LENGTH
    ret += TLV_FIELD3_LENGTH;
#else
    ret += sizeof(self->TLV_FIELD3_NAME);
#endif
#endif

    return ret;
#endif
}

static bool TLV_TEMPLATE_FUNCTION_NAME(forge)(const struct tlv *tlv,
                                              uint8_t **buf __attribute__((unused)),
                                              size_t *length __attribute__((unused)))
{
    const TLV_TEMPLATE_STRUCT_NAME *self = (const TLV_TEMPLATE_STRUCT_NAME *)tlv;

#ifdef TLV_FORGE_BODY
    TLV_FORGE_BODY(self, buf, length);
#undef TLV_FORGE_BODY
#else // TLV_FORGE_BODY

    if (!TLV_TEMPLATE_FUNCTION_NAME(forge_field1)(self, buf, length))
        return false;

#ifdef TLV_FIELD2_NAME
    if (!TLV_TEMPLATE_FUNCTION_NAME(forge_field2)(self, buf, length))
        return false;
#endif // TLV_FIELD2_NAME

#ifdef TLV_FIELD3_NAME
    if (!TLV_TEMPLATE_FUNCTION_NAME(forge_field3)(self, buf, length))
        return false;
#endif // TLV_FIELD3_NAME

#endif // TLV_FORGE_BODY

    return true;
}

static void TLV_TEMPLATE_FUNCTION_NAME(print)(const struct tlv *tlv, void (*write_function)(const char *fmt, ...), const char *prefix)
{
    const TLV_TEMPLATE_STRUCT_NAME *self = (const TLV_TEMPLATE_STRUCT_NAME *)tlv;

#ifdef TLV_PRINT_BODY
    TLV_PRINT_BODY(self);
#undef TLV_PRINT_BODY
#else // TLV_PRINT_BODY

    TLV_TEMPLATE_FUNCTION_NAME(print_field1)(self, write_function, prefix);

#ifdef TLV_FIELD2_NAME
    TLV_TEMPLATE_FUNCTION_NAME(print_field2)(self, write_function, prefix);
#endif // TLV_FIELD2_NAME

#ifdef TLV_FIELD3_NAME
    TLV_TEMPLATE_FUNCTION_NAME(print_field3)(self, write_function, prefix);
#endif // TLV_FIELD3_NAME
#endif // TLV_PRINT_BODY
}

#ifndef TLV_NEW
static void TLV_TEMPLATE_FUNCTION_NAME(free)(struct tlv *tlv)
{
    TLV_TEMPLATE_STRUCT_NAME *self = (TLV_TEMPLATE_STRUCT_NAME *)tlv;

#ifdef TLV_FREE_BODY
#undef TLV_FREE_BODY
    return TLV_TEMPLATE_FUNCTION_NAME(free_body)(self);
#endif

    free(self);
}

static bool TLV_TEMPLATE_FUNCTION_NAME(compare)(const struct tlv *tlv1, const struct tlv *tlv2)
{
    const TLV_TEMPLATE_STRUCT_NAME *self1 = (const TLV_TEMPLATE_STRUCT_NAME *)tlv1;
    const TLV_TEMPLATE_STRUCT_NAME *self2 = (const TLV_TEMPLATE_STRUCT_NAME *)tlv2;

    if (!TLV_TEMPLATE_FUNCTION_NAME(compare_field1)(self1, self2))
        return false;

#ifdef TLV_FIELD2_NAME
    if (!TLV_TEMPLATE_FUNCTION_NAME(compare_field2)(self1, self2))
        return false;
#endif // TLV_FIELD2_NAME

#ifdef TLV_FIELD3_NAME
    if (!TLV_TEMPLATE_FUNCTION_NAME(compare_field3)(self1, self2))
        return false;
#endif // TLV_FIELD3_NAME

    return true;
}
#else
#undef TLV_NEW
#endif

#undef TLV_NAME
#undef TLV_FIELD1_NAME
#ifdef TLV_FIELD1_LENGTH
#undef TLV_FIELD1_LENGTH
#endif
#ifdef TLV_FIELD2_NAME
#undef TLV_FIELD2_NAME
#endif
#ifdef TLV_FIELD2_LENGTH
#undef TLV_FIELD2_LENGTH
#endif
#ifdef TLV_FIELD3_NAME
#undef TLV_FIELD3_NAME
#endif
#ifdef TLV_FIELD3_LENGTH
#undef TLV_FIELD3_LENGTH
#endif
#undef TLV_PARSE
#undef TLV_FORGE
#undef TLV_PRINT
#undef TLV_COMPARE
