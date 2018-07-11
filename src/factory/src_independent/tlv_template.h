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
 * #define TLV_PARSE_EXTRA(self)   \
 *   do {                          \
 *     if (self->bar2 > 0x03)      \
 *     {                           \
 *       PLATFORM_PRINTF_DEBUG_ERROR("Invalid bar2: 0x%02x\n", self->bar2); \
 *       goto err_out;             \
 *     }                           \
 *   while(0)
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
 * For more complicated fields, including dynamic length, define TLV_FIELD1_PARSE, TLV_FIELD1_FORGE, TLV_FIELD1_PRINT,
 * and TLV_FIELD1_COMPARE macros. For dynamically allocated sub-structures you also need to define TLV_FIELD1_FREE and
 * TLV_LENGTH_BODY to calculate the total length.
 */

/** @def TLV_FIELD1_PARSE(self,buffer,length)
 * @brief Custom parse fragment for field1.
 *
 * @param self Pointer to a freshly created structure of the right type, to be updated by this fragment.
 *
 * @param buffer Buffer from which to parse. To be passed to _ExBL().
 *
 * @param length Remaining length of @a buffer. To be passed to _ExBL().
 *
 * Define this macro if the default parse fragment doesn't work, e.g. for dynamic length. In case of error, the fragment
 * should print something and goto err_out.
 */

/** @def TLV_PARSE_EXTRA(self,buffer,length)
 * @brief Extra checks after parsing.
 *
 * @param self Pointer to a freshly created structure of the right type, all fields have been filled in.
 *
 * This macro can be defined to do additional checks or additional modifications after parsing all fields.  It can also
 * parse additional fields beyond what is supported by this template.
 *
 * In case of error, the fragment should print something and goto err_out.
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

/** @def TLV_FIELD1_FORGE(self,buf,length)
 * @brief Custom forge fragment for field1.
 *
 * @param self Pointer to the TLV, cast to the correct type.
 *
 * @param buf Buffer in which to forge. To be passed to _IxBL().
 *
 * @param length Remaining length of @a buf. To be passed to _IxBL().
 *
 * Define this macro if the default forge fragment doesn't work, e.g. for dynamic length. In case of error, the fragment
 * should return false.
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

#define TLV_TEMPLATE_ExBL_INNER(length) _E ## length ## BL
#define TLV_TEMPLATE_ExBL(length) TLV_TEMPLATE_ExBL_INNER(length)

#define TLV_TEMPLATE_IxBL_INNER(length) _I ## length ## BL
#define TLV_TEMPLATE_IxBL(length) TLV_TEMPLATE_IxBL_INNER(length)

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

static struct tlv *TLV_TEMPLATE_FUNCTION_NAME(parse)(const struct tlv_def *def __attribute__((unused)),
                                                     const uint8_t *buffer __attribute__((unused)),
                                                     size_t length __attribute__((unused)))
{
    TLV_TEMPLATE_STRUCT_NAME *self = PLATFORM_MALLOC(sizeof(TLV_TEMPLATE_STRUCT_NAME));

    PLATFORM_MEMSET(self, 0, sizeof(*self));

#ifdef TLV_PARSE_BODY
    TLV_PARSE_BODY(self);
#undef TLV_PARSE_BODY

#else // TLV_PARSE_BODY

#if defined(TLV_FIELD1_PARSE)
    TLV_FIELD1_PARSE(self, buffer, length);
#undef TLV_FIELD1_PARSE
#else // TLV_FIELD1_PARSE
#if defined(TLV_FIELD1_LENGTH)
    if (!TLV_TEMPLATE_ExBL(TLV_FIELD1_LENGTH)(&buffer, &self->TLV_FIELD1_NAME, &length))
#else // TLV_FIELD1_LENGTH
    if (!_EnBL(&buffer, self->TLV_FIELD1_NAME, sizeof(self->TLV_FIELD1_NAME), &length))
#endif // TLV_FIELD1_LENGTH
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Malformed %s TLV: no " TLV_TEMPLATE_STR(TLV_FIELD1_NAME) "\n", def->name);
        goto err_out;
    }
#endif // TLV_FIELD1_PARSE

#ifdef TLV_FIELD2_NAME
#if defined(TLV_FIELD2_PARSE)
    TLV_FIELD2_PARSE(self, buffer, length);
#undef TLV_FIELD2_PARSE
#else // TLV_FIELD2_PARSE
#if defined(TLV_FIELD2_LENGTH)
    if (!TLV_TEMPLATE_ExBL(TLV_FIELD2_LENGTH)(&buffer, &self->TLV_FIELD2_NAME, &length))
#else // TLV_FIELD2_LENGTH
    if (!_EnBL(&buffer, self->TLV_FIELD2_NAME, sizeof(self->TLV_FIELD2_NAME), &length))
#endif // TLV_FIELD2_LENGTH
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Malformed %s TLV: no " TLV_TEMPLATE_STR(TLV_FIELD2_NAME) "\n", def->name);
        goto err_out;
    }
#endif // TLV_FIELD2_PARSE
#endif // TLV_FIELD2_NAME

#ifdef TLV_FIELD3_NAME
#if defined(TLV_FIELD3_PARSE)
    TLV_FIELD3_PARSE(self, buffer, length);
#undef TLV_FIELD3_PARSE
#else // TLV_FIELD3_PARSE
#if defined(TLV_FIELD3_LENGTH)
    if (!TLV_TEMPLATE_ExBL(TLV_FIELD3_LENGTH)(&buffer, &self->TLV_FIELD3_NAME, &length))
#else // TLV_FIELD3_LENGTH
    if (!_EnBL(&buffer, self->TLV_FIELD3_NAME, sizeof(self->TLV_FIELD3_NAME), &length))
#endif // TLV_FIELD3_LENGTH
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Malformed %s TLV: no " TLV_TEMPLATE_STR(TLV_FIELD3_NAME) "\n", def->name);
        goto err_out;
    }
#endif // TLV_FIELD3_PARSE
#endif // TLV_FIELD3_NAME


#ifdef TLV_PARSE_EXTRA
    TLV_PARSE_EXTRA(self,buffer,length);
#undef TLV_PARSE_EXTRA
#endif

#endif // TLV_PARSE_BODY

    return (struct tlv *)self;

err_out:
    def->free((struct tlv *)self);
    return NULL;
}

static uint16_t TLV_TEMPLATE_FUNCTION_NAME(length)(const struct tlv *tlv)
{
    const TLV_TEMPLATE_STRUCT_NAME *self __attribute__((unused)) = (const TLV_TEMPLATE_STRUCT_NAME *)tlv;

#if defined(TLV_LENGTH_BODY)
    TLV_LENGTH_BODY(self);
#undef TLV_LENGTH_BODY
#else // TLV_LENGTH_BODY

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
#endif // TLV_LENGTH_BODY
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

#if defined(TLV_FIELD1_FORGE)
    TLV_FIELD1_FORGE(self, buf, length);
#undef TLV_FIELD1_FORGE
#else // TLV_FIELD1_FORGE
#if defined(TLV_FIELD1_LENGTH)
    if (!TLV_TEMPLATE_IxBL(TLV_FIELD1_LENGTH)(&self->TLV_FIELD1_NAME, buf, length))
        return false;
#else // TLV_FIELD1_LENGTH
    if (!_InBL(self->TLV_FIELD1_NAME, buf, sizeof(self->TLV_FIELD1_NAME), length))
        return false;
#endif // TLV_FIELD1_LENGTH
#endif // TLV_FIELD1_FORGE

#ifdef TLV_FIELD2_NAME
#if defined(TLV_FIELD2_FORGE)
    TLV_FIELD2_FORGE(self, buf, length);
#undef TLV_FIELD2_FORGE
#else // TLV_FIELD2_FORGE
#if defined(TLV_FIELD2_LENGTH)
    if (!TLV_TEMPLATE_IxBL(TLV_FIELD2_LENGTH)(&self->TLV_FIELD2_NAME, buf, length))
        return false;
#else // TLV_FIELD2_LENGTH
    if (!_InBL(self->TLV_FIELD2_NAME, buf, sizeof(self->TLV_FIELD2_NAME), length))
        return false;
#endif // TLV_FIELD2_LENGTH
#endif // TLV_FIELD2_FORGE
#endif // TLV_FIELD2_NAME

#ifdef TLV_FIELD3_NAME
#if defined(TLV_FIELD3_FORGE)
    TLV_FIELD3_FORGE(self, buf, length);
#undef TLV_FIELD3_FORGE
#else // TLV_FIELD3_FORGE
#if defined(TLV_FIELD3_LENGTH)
    if (!TLV_TEMPLATE_IxBL(TLV_FIELD3_LENGTH)(&self->TLV_FIELD3_NAME, buf, length))
        return false;
#else // TLV_FIELD3_LENGTH
    if (!_InBL(self->TLV_FIELD3_NAME, buf, sizeof(self->TLV_FIELD3_NAME), length))
        return false;
#endif // TLV_FIELD3_LENGTH
#endif // TLV_FIELD3_FORGE
#endif // TLV_FIELD3_NAME

#ifdef TLV_FORGE_EXTRA
    TLV_FORGE_EXTRA(self);
#undef TLV_FORGE_EXTRA
#endif

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

#if defined(TLV_FIELD1_PRINT)
    TLV_FIELD1_PRINT(self, write_function, prefix);
#undef TLV_FIELD1_PRINT
#else // TLV_FIELD1_PRINT
#if defined(TLV_FIELD1_LENGTH)
    print_callback(write_function, prefix, TLV_FIELD1_LENGTH, TLV_TEMPLATE_STR(TLV_FIELD1_NAME), "%d", &self->TLV_FIELD1_NAME);
#else // TLV_FIELD1_LENGTH
    print_callback(write_function, prefix, sizeof(self->TLV_FIELD1_NAME), TLV_TEMPLATE_STR(TLV_FIELD1_NAME), "0x%02x", &self->TLV_FIELD1_NAME);
#endif // TLV_FIELD1_LENGTH
#endif // TLV_FIELD1_PRINT

#ifdef TLV_FIELD2_NAME
#if defined(TLV_FIELD2_PRINT)
    TLV_FIELD2_PRINT(self, write_function, prefix);
#undef TLV_FIELD2_PRINT
#else // TLV_FIELD2_PRINT
#if defined(TLV_FIELD2_LENGTH)
    print_callback(write_function, prefix, TLV_FIELD2_LENGTH, TLV_TEMPLATE_STR(TLV_FIELD2_NAME), "%d", &self->TLV_FIELD2_NAME);
#else // TLV_FIELD2_LENGTH
    print_callback(write_function, prefix, sizeof(self->TLV_FIELD2_NAME), TLV_TEMPLATE_STR(TLV_FIELD2_NAME), "0x%02x", &self->TLV_FIELD2_NAME);
#endif // TLV_FIELD2_LENGTH
#endif // TLV_FIELD2_PRINT
#endif // TLV_FIELD2_NAME

#ifdef TLV_FIELD3_NAME
#if defined(TLV_FIELD3_PRINT)
    TLV_FIELD3_PRINT(self, write_function, prefix);
#undef TLV_FIELD3_PRINT
#else // TLV_FIELD3_PRINT
#if defined(TLV_FIELD3_LENGTH)
    print_callback(write_function, prefix, TLV_FIELD3_LENGTH, TLV_TEMPLATE_STR(TLV_FIELD3_NAME), "%d", &self->TLV_FIELD3_NAME);
#else // TLV_FIELD3_LENGTH
    print_callback(write_function, prefix, sizeof(self->TLV_FIELD3_NAME), TLV_TEMPLATE_STR(TLV_FIELD3_NAME), "0x%02x", &self->TLV_FIELD3_NAME);
#endif // TLV_FIELD3_LENGTH
#endif // TLV_FIELD3_PRINT
#endif // TLV_FIELD3_NAME
}

static void TLV_TEMPLATE_FUNCTION_NAME(free)(struct tlv *tlv)
{
    TLV_TEMPLATE_STRUCT_NAME *self = (TLV_TEMPLATE_STRUCT_NAME *)tlv;

#if defined(TLV_FIELD1_FREE)
    TLV_FIELD1_FREE(self);
#undef TLV_FIELD1_FREE
#endif

#if defined(TLV_FIELD2_FREE)
    TLV_FIELD2_FREE(self);
#undef TLV_FIELD2_FREE
#endif

#if defined(TLV_FIELD3_FREE)
    TLV_FIELD3_FREE(self);
#undef TLV_FIELD3_FREE
#endif

    PLATFORM_FREE(self);
}

static bool TLV_TEMPLATE_FUNCTION_NAME(compare)(const struct tlv *tlv1, const struct tlv *tlv2)
{
    const TLV_TEMPLATE_STRUCT_NAME *self1 = (const TLV_TEMPLATE_STRUCT_NAME *)tlv1;
    const TLV_TEMPLATE_STRUCT_NAME *self2 = (const TLV_TEMPLATE_STRUCT_NAME *)tlv2;

#if defined (TLV_FIELD1_COMPARE)
    TLV_FIELD1_COMPARE(self1,self2);
#undef TLV_FIELD1_COMPARE
#else // TLV_FIELD1_COMPARE
#ifdef TLV_FIELD1_LENGTH
    if (self1->TLV_FIELD1_NAME != self2->TLV_FIELD1_NAME)
        return false;
#else
    if (PLATFORM_MEMCMP(self1->TLV_FIELD1_NAME, self2->TLV_FIELD1_NAME, sizeof(self1->TLV_FIELD1_NAME)) != 0)
        return false;
#endif
#endif // TLV_FIELD1_COMPARE

#ifdef TLV_FIELD2_NAME
#if defined (TLV_FIELD2_COMPARE)
    TLV_FIELD2_COMPARE(self1,self2);
#undef TLV_FIELD2_COMPARE
#else // TLV_FIELD1_COMPARE
#ifdef TLV_FIELD2_LENGTH
    if (self1->TLV_FIELD2_NAME != self2->TLV_FIELD2_NAME)
        return false;
#else
    if (PLATFORM_MEMCMP(self1->TLV_FIELD2_NAME, self2->TLV_FIELD2_NAME, sizeof(self1->TLV_FIELD2_NAME)) != 0)
        return false;
#endif
#endif // TLV_FIELD2_COMPARE
#endif // TLV_FIELD2_NAME

#ifdef TLV_FIELD3_NAME
#if defined (TLV_FIELD3_COMPARE)
    TLV_FIELD3_COMPARE(self1,self2);
#undef TLV_FIELD3_COMPARE
#else // TLV_FIELD1_COMPARE
#ifdef TLV_FIELD3_LENGTH
    if (self1->TLV_FIELD3_NAME != self2->TLV_FIELD3_NAME)
        return false;
#else
    if (PLATFORM_MEMCMP(self1->TLV_FIELD3_NAME, self2->TLV_FIELD3_NAME, sizeof(self1->TLV_FIELD3_NAME)) != 0)
        return false;
#endif
#endif // TLV_FIELD3_COMPARE
#endif // TLV_FIELD3_NAME

#ifdef TLV_COMPARE_EXTRA
    TLV_COMPARE_EXTRA(self1, self2);
#undef TLV_COMPARE_EXTRA
#endif

#endif

    return true;
}

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

