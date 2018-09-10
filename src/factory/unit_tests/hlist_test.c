/*
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

struct htest1 {
    hlist_item h;
    unsigned data;
};

struct htest2 {
    hlist_item h;
    char data;
};

/** @brief Test hlist_compare and HLIST_COMPARE_ITEM on @a ht1 and @a ht1b. */
static int check_compare(struct htest1 *ht1, struct htest1 *ht1b, int expected_result, const char *reason)
{
    int ret = 0;
    int result;
    result = HLIST_COMPARE_ITEM(ht1, ht1b, h);
    result = (result > 0 ? 1 : (result < 0 ? -1 : 0));
    if (result != expected_result)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("HLIST_COMPARE_ITEM result %d but expected %d for %s\n", result, expected_result, reason);
        ret++;
    }
    result = hlist_compare(ht1->h.l.prev, ht1b->h.l.prev);
    result = (result > 0 ? 1 : (result < 0 ? -1 : 0));
    if (result != expected_result)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("hlist_compare result %d but expected %d for %s\n", result, expected_result, reason);
        ret++;
    }
    return ret;
}

int main()
{
    int ret = 0;
    hlist_head list1;
    hlist_head list2;
    struct htest1 *ht1, *ht1b;
    struct htest2 *ht2;

    hlist_head_init(&list1);
    ht1 = HLIST_ALLOC(struct htest1, h, &list1);
    ht1->data = 242;
    HLIST_ALLOC(struct htest2, h, &ht1->h.children[0])->data = 42;
    HLIST_ALLOC(struct htest2, h, &ht1->h.children[0])->data = 43;

    /* Construct the same contents again */
    hlist_head_init(&list2);
    ht1b = HLIST_ALLOC(struct htest1, h, &list2);
    ht1b->data = 242;
    HLIST_ALLOC(struct htest2, h, &ht1b->h.children[0])->data = 42;
    ret += check_compare(ht1, ht1b, 1, "ht1b with shorter child list");

    ht2 = HLIST_ALLOC(struct htest2, h, &ht1b->h.children[0]);
    ht2->data = 42;
    ret += check_compare(ht1, ht1b, 1, "ht1b with smaller child data");
    ht2->data = 44;
    ret += check_compare(ht1, ht1b, -1, "ht1b with larger child data");
    ht2->data = 43;
    ret += check_compare(ht1, ht1b, 0, "ht1b with equal child data");

    ht1b->data = 241;
    ret += check_compare(ht1, ht1b, 1, "ht1b with smaller data");
    ht1b->data = 243;
    ret += check_compare(ht1, ht1b, -1, "ht1b with larger data");
    ht1b->data = 242;

    HLIST_ALLOC(struct htest2, h, &ht1b->h.children[0])->data = 43;
    ret += check_compare(ht1, ht1b, -1, "ht1b with longer child list");

    hlist_delete(&list1);
    /* Remove ht1b from list2 by just resetting both lists */
    hlist_head_init(&ht1b->h.l);
    hlist_head_init(&list2);
    hlist_delete_item(&ht1b->h);
    /* Deleting an empty list works. */
    hlist_delete(&list2);

    return ret;
}
