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
#include "platform.h"
#include <stdarg.h>
#include <string.h>

struct htest1 {
    hlist_item h;
    unsigned data;
};

struct htest2 {
    hlist_item h;
    char data;
    char data2[15]; /* The rest of data, used to test the various print formats. */
};

static int check_count(dlist_head *list, size_t expected_count)
{
    size_t real_count = dlist_count(list);
    if (real_count != expected_count)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("dlist_count result %u but expected %u\n", (unsigned)real_count, (unsigned)expected_count);
        return 1;
    }
    else
    {
        return 0;
    }
}

int main()
{
    int ret = 0;
    dlist_head list;
    struct htest1 *ht1;

    dlist_head_init(&list);
    ret += check_count(&list, 0);
    ht1 = HLIST_ALLOC(struct htest1, h, &list);
    ht1->data = 242;
    HLIST_ALLOC(struct htest2, h, &ht1->h.children[0])->data = 42;
    HLIST_ALLOC(struct htest2, h, &ht1->h.children[0])->data = 43;

    ret += check_count(&list, 1);
    ret += check_count(&ht1->h.children[0], 2);

    dlist_remove(&ht1->h);
    ret += check_count(&list, 0);
    hlist_delete_item(&ht1->h);
    /* This also deletes the two htest2 children, but there's no way to check that. */

    /* Deleting an empty list works. */
    hlist_delete(&list);
    ret += check_count(&list, 0);

    return ret;
}
