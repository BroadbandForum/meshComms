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

#ifndef HLIST_H
#define HLIST_H
/** @file
 *  @brief Hierarchical linked list.
 *
 * This file implements a hierarchical linked list.
 */

#include <utils.h> /* container_of() */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h> /* size_t */

/** @brief Maximum number of children in a hlist_item.
 *
 * To simplify object creation and destruction, the hlist children are put in an array with this number of elements.
 * It is not possible to create a hlist with more children than this.
 */
#define HLIST_MAX_CHILDREN 2

/**
 * MAC address handling. Although not directly related to hlistss, many users of hlistss manipulate MAC addresses. In
 * addition, it is one of the types handled by the tlv_struct_print_list functions. Thus, it's relevant to include it in this
 * file.
 * @{
 */

/** @brief Definition of a MAC address. */
typedef uint8_t mac_address[6];

/* The following are copied from hostapd, Copyright (c) 2002-2007, Jouni Malinen <j@w1.fi>
 * This software may be distributed under the terms of the BSD license.
 */
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
/** @} */

/** @brief Reference to a hlist.
 *
 * Use hlist_empty() to check if the list is empty.
 */
typedef struct hlist_head {
    struct hlist_head *next;
    struct hlist_head *prev;
} hlist_head;

/** @brief Hierarchical linked list.
 *
 * This is a doubly-linked list which also keeps track of child objects. The children are used to be able to do
 * generic operations on the entire hierarchical structure: comparison, deallocation.
 *
 * A @a hlist_item can only be dynamically allocated, through the HLIST_ALLOC macro.
 *
 * The @a hlist_item must be embedded in a larger structure and must be the first member of that struct.
 *
 * Example of how to use @a hlist_item in a hierarchical structure:
 * @code
    struct g {
        hlist_item h_g;
        char data[4];
    };

    struct f {
        hlist_item h_f;
        char data[2];
    };

    struct f *f1 = HLIST_ALLOC(struct f, h_f, NULL);
    HLIST_ALLOC(struct g, h_g, &f1->h_f.children[0]);
 * @endcode
 *
 * It is advisable to wrap the allocation macros in a type-specific allocation function (which can also initialize the
 * other struct members).
 *
 * A single item (that has not been added to a list and has no children) can be freed with the normal free() function.
 * An entire list, including children, can be freed with hlist_delete(), or a single item with hlist_delete_item().
 */
typedef struct hlist_item {
    hlist_head l;
    struct hlist_head children[HLIST_MAX_CHILDREN];
} hlist_item;

/** @brief Initialise an empty hlist_head.
 *
 * Must be called for user-allocated hlist_head before doing any manipulation of the @a head.
 */
static inline void hlist_head_init(hlist_head *head)
{
    head->next = head;
    head->prev = head;
}

/** @brief Declaration of static or stack hlist_head variable. May be preceded by the @c static keyword. */
#define DECLARE_HLIST_HEAD(name) hlist_head name = {&name, &name}

/** @brief Add an element at the front of a list.
 *
 * @param list The list head.
 * @param item The new element to be added to the list.
 * @pre @a list and @a item are not NULL.
 * @pre @a item is not part of a list.
 */
static inline void hlist_add_head(hlist_head *list, hlist_item *item)
{
    item->l.next = list->next;
    item->l.prev = list;
    list->next->prev = &item->l;
    list->next = &item->l;
}

/** @brief Add an element at the back of a list.
 *
 * @param list The list head.
 * @param item The new element to be added to the list.
 * @pre @a list and @a item are not NULL.
 * @pre @a item is not part of a list.
 */
static inline void hlist_add_tail(hlist_head *list, hlist_item *item)
{
    hlist_add_head(list->prev, item);
}

/** @brief Check if the list is empty. */
static inline bool hlist_empty(const hlist_head *list)
{
    return list->next == list;
}

/** @brief Iterate over a hlist (non-recursively)
 *
 * @param item the iterator variable, of type @a type.
 * @param head the head of the hlist.
 * @param type the type of hlist items.
 * @param hlist_member the member of @a type that is of type hlist_item.
 */
#define hlist_for_each(item, head, type, hlist_member) \
    for ((item) = container_of((head).next, type, hlist_member.l); \
         &(item)->hlist_member.l != &(head); \
         (item) = container_of((item)->hlist_member.l.next, type, hlist_member.l))

/** @brief Iterate over a hlist (non-recursively)
 *
 * Like hlist_for_each, but @a item is of type hlist_item instead of its supertype.
 */
#define hlist_for_each_item(item, head) \
    for ((item) = container_of((head).next, hlist_item, l);\
         &(item)->l != &(head); \
         (item) = container_of((item)->l.next, hlist_item, l))

static inline size_t hlist_count(const hlist_head *list)
{
    size_t count = 0;
    hlist_item *item;
    hlist_for_each_item(item, *list)
    {
        count++;
    }
    return count;
}

/** @brief Allocate a hlist_item structure.
 *
 * @internal
 *
 * Do not call this function directly, use the HLIST_ALLOC* family of macros.
 */
struct hlist_item *hlist_alloc(size_t size, hlist_head *parent);

/** @brief Type-safe allocation of a hlist_item.
 *
 * @return A new object of type @a type with all hlist_item members properly initialised.
 * @param type Type of the object to allocate.
 * @param hlist_member Name of the member of @a type that is an hlist_item. Must be the first member.
 * @param parent The parent hlist_head to which to append the new item, or NULL to allocate a loose item.
 *
 * All the lists are initialized to empty. The entire allocated structure is initialised to 0.
 */
#define HLIST_ALLOC(type, hlist_member, parent) ({ \
        _Static_assert(offsetof(type, hlist_member) == 0, "hlist member must be first of struct"); \
        hlist_item *allocced = hlist_alloc(sizeof(type), parent); \
        container_of(allocced, type, hlist_member); \
    })

/** @brief Delete a hlist.
 *
 * Recursively delete all elements from @a list. @a list itself is not free()'d, so it can be a static or
 * stack-allocated variable.
 */
void hlist_delete(hlist_head *list);

/** @brief Delete a hlist item.
 *
 * Recursively delete all child elements from @a item, including @a item itself.
 *
 * @pre @a item must not be part of a list.
 */
void hlist_delete_item(hlist_item *item);

#endif // HLIST_H
