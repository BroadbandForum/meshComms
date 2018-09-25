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

struct hlist_item *hlist_alloc(size_t size, hlist_head *parent)
{
    hlist_item *ret = memalloc(size);
    memset(ret, 0, size);
    hlist_head_init(&ret->l);
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
    hlist_head_init(list);
}

void hlist_delete_item(hlist_item *item)
{
    assert(hlist_empty(&item->l));
    hlist_delete(&item->children[0]);
    hlist_delete(&item->children[1]);
    free(item);
}
