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

#include <hlist.h>
#include <utils.h> /* memalloc() */
#include <string.h> /* memset() */

struct hlist_item *hlist_alloc(const hlist_description *desc, hlist_head *parent)
{
    size_t size = desc->size;
    hlist_item *ret = memalloc(size);
    memset(ret, 0, size);
    hlist_head_init(&ret->l);
    ret->desc = desc;
    hlist_head_init(&ret->children[0]);
    hlist_head_init(&ret->children[1]);
    if (parent)
    {
        hlist_add_tail(parent, ret);
    }
    return ret;
}

void hlist_delete(hlist_head *list)
{
    hlist_head *next = list->next;

    /* Since we will free all items in the list, it is not needed to remove them from the list. However, the normal
     * hlist_for_each macro would use the next pointer after the item has been freed. Therefore, we use an open-coded
     * iteration here. */
    while (next != list)
    {
        hlist_item *item = container_of(next, hlist_item, l);
        next = next->next;
        hlist_head_init(&item->l);
        hlist_delete_item(item);
    }
    /* We still have to make sure the list is empty, in case it is reused later. */
    list->prev = list;
    list->next = list;
}

void hlist_delete_item(hlist_item *item)
{
    assert(hlist_empty(&item->l));
    hlist_delete(&item->children[0]);
    hlist_delete(&item->children[1]);
    free(item);
}


int hlist_compare(hlist_head *h1, hlist_head *h2)
{
    int ret = 0;
    hlist_head *cur1;
    hlist_head *cur2;
    /* Open-code hlist_for_each because we need to iterate over both at once. */
    for (cur1 = h1->next, cur2 = h2->next;
         ret == 0 && cur1 != h1 && cur2 != h2;
         cur1 = cur1->next, cur2 = cur2->next)
    {
        ret = hlist_compare_item(container_of(cur1, hlist_item, l), container_of(cur2, hlist_item, l));
    }
    if (ret == 0)
    {
        /* We reached the end of the list. Check if one of the lists is longer. */
        if (cur1 != h1)
            ret = 1;
        else if (cur2 != h2)
            ret = -1;
        else
            ret = 0;
    }
    return ret;
}

int hlist_compare_item(hlist_item *item1, hlist_item *item2)
{
    int ret;
    unsigned i;

    assert(item1->desc == item2->desc);

    ret = memcmp((char*)item1 + sizeof(hlist_item), (char*)item2 + sizeof(hlist_item),
                 item1->desc->size - sizeof(hlist_item));
    for (i = 0; ret == 0 && i < ARRAY_SIZE(item1->children); i++)
    {
        ret = hlist_compare(&item1->children[i], &item2->children[i]);
    }
    return ret;
}

void hlist_print(const hlist_head *list, bool include_index, void (*write_function)(const char *fmt, ...), const char *prefix)
{
    const hlist_item *item;
    char new_prefix[100];
    unsigned i = 0;

    hlist_for_each_item(item, *list)
    {
        if (include_index)
        {
            snprintf(new_prefix, sizeof(new_prefix)-1, "%s%s[%u]", prefix, item->desc->name, i);
        } else {
            snprintf(new_prefix, sizeof(new_prefix)-1, "%s%s", prefix, item->desc->name);
        }
        hlist_print_item(item, write_function, new_prefix);
        i++;
    }
}

void hlist_print_item(const hlist_item *item, void (*write_function)(const char *fmt, ...), const char *prefix)
{
    size_t i;
    char new_prefix[100];

    /* Construct the new prefix. */
    snprintf(new_prefix, sizeof(new_prefix)-1, "%s->", prefix);

    /* First print the fields. */
    for (i = 0; i < ARRAY_SIZE(item->desc->fields) && item->desc->fields[i].name != NULL; i++)
    {
        hlist_print_field(item, &item->desc->fields[i], write_function, new_prefix);
    }
    /* Next print the children. */
    for (i = 0; i < ARRAY_SIZE(item->children) && item->desc->children[i] != NULL; i++)
    {
        hlist_print(&item->children[i], true, write_function, new_prefix);
    }
}

void hlist_print_field(const hlist_item *item, const hlist_field_description *field_desc,
                       void (*write_function)(const char *fmt, ...), const char *prefix)
{
    unsigned value;
    char *pvalue = ((char*)item) + field_desc->offset;

    switch (field_desc->format)
    {
        case hlist_format_hex:
        case hlist_format_dec:
        case hlist_format_unsigned:

            switch (field_desc->size)
            {
                case 1:
                    value = *(const uint8_t*)pvalue;
                    break;
                case 2:
                    value = *(const uint16_t*)pvalue;
                    break;
                case 4:
                    value = *(const uint32_t*)pvalue;
                    break;
                default:
                    assert(field_desc->format == hlist_format_hex);
                    /* @todo */
                    return;
            }
            write_function("%s%s: ", prefix, field_desc->name);
            switch (field_desc->format)
            {
                case hlist_format_hex:
                    write_function("0x%02x", value);
                    break;
                case hlist_format_dec:
                    write_function("%d", value);
                    break;
                case hlist_format_unsigned:
                    write_function("%u", value);
                    break;
                default:
                    assert(0);
                    break;
            }
            write_function("\n");
            break;

        case hlist_format_mac:
        case hlist_format_ipv4:
        case hlist_format_ipv6:
            break;

        default:
            assert(0);
            break;
    }
}


