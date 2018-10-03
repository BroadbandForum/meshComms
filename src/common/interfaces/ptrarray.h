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

#ifndef PTRARRAY_H
#define PTRARRAY_H

/** @file
 *  @brief Pointer array structure.
 *
 * This file implements a container implemented as a pointer array.
 *
 * To be type-safe, this functionality is fully defined by macros. The pointer array consists of a length member and
 * a data pointer.
 */

#include <utils.h> /* container_of() */
#include <stdbool.h>

/** @brief Declare an array of @a type.
 *
 * This is typically added as a struct member. It cannot be used directly as a function parameter, but it can be used
 * in a typedef.
 *
 * The pointer array must be initialised to 0 before using it, either with memset or by implicit initialisation of
 * static variables.
 *
 * To get the number of elements, access the length member directly.
 *
 * To iterate, use:
 * @code
    unsigned i;
    for (i = 0; i < ptrarray.length; i++)
    {
        ... ptrarray.data[i] ...
    }
 * @endcode
 *
 * Note that @a type may be any type (it must be a scalar type because other macros use assignment and comparison on
 * it). Normally it is a pointer type (hence the name pointer array).
 */
#define PTRARRAY(type) \
    struct { \
        unsigned length; \
        type *data; \
    }

/** @brief Add an element to a pointer array.
 *
 * The element is always added at the end.
 *
 * @param ptrarray A pointer array declared with PTRARRAY().
 * @param item A pointer to the element to by added. Must be a pointer to the same type as in the PTRARRAY declaration.
 */
#define PTRARRAY_ADD(ptrarray, item) \
    do { \
        (ptrarray).data = memrealloc((ptrarray).data, ((ptrarray).length + 1) * sizeof((ptrarray).data)); \
        (ptrarray).data[(ptrarray).length] = item; \
        (ptrarray).length++; \
    } while (0)

/** @brief Find a pointer in the pointer array.
 *
 * @return The index if found, @a ptrarray.length if not found.
 */
#define PTRARRAY_FIND(ptrarray, item) ({ \
        unsigned ptrarray_find_i; \
        for (ptrarray_find_i = 0; ptrarray_find_i < (ptrarray).length; ptrarray_find_i++) \
        { \
            if ((ptrarray).data[ptrarray_find_i] == (item)) \
                break; \
        } \
        ptrarray_find_i; \
    })

/** @brief Remove an item at a certain index from a pointer array.
 *
 * For convenience when used in combination with PTRARRAY_FIND, if @a index is equal to the length of the array, nothing
 * is removed.
 */
#define PTRARRAY_REMOVE(ptrarray, index) \
    do { \
        if (index >= (ptrarray).length) \
            break; \
        (ptrarray).length--; \
        if ((ptrarray).length > 0) \
        { \
            memmove((ptrarray).data + index, (ptrarray).data + (index) + 1, \
                    ((ptrarray).length - (index)) * sizeof((ptrarray).data)); \
            (ptrarray).data = memrealloc((ptrarray).data, ((ptrarray).length) * sizeof((ptrarray).data)); \
        } else { \
            free((ptrarray).data); \
            (ptrarray).data = NULL; \
        } \
    } while (0)

/** @brief Remove an item from a pointer array. */
#define PTRARRAY_REMOVE_ELEMENT(ptrarray, item) \
    PTRARRAY_REMOVE(ptrarray, PTRARRAY_FIND(ptrarray, item))


/** @brief Remove all elements from the pointer array. */
#define PTRARRAY_CLEAR(ptrarray) \
    do { \
        free((ptrarray).data); \
        (ptrarray).data = NULL; \
        (ptrarray).length = 0; \
    } while(0)

#endif // PTRARRAY_H
