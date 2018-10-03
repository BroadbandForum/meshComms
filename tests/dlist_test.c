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

#include <dlist.h>
#include "platform.h"
#include <stdarg.h>
#include <string.h>

struct dtest {
    dlist_item l;
    unsigned data;
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

static int check_values(dlist_head *list, const unsigned values[])
{
    struct dtest *t;
    unsigned i = 0;
    dlist_for_each(t, *list, l)
    {
        if (values[i] == 0)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("dlist includes unexpected element %u\n", t->data);
            return 1;
        }
        if (values[i] != t->data)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("dlist includes unexpected element %u != %u\n", t->data, values[i]);
            return 1;
        }
        i++;
    }
    return 0;
}

int main()
{
    int ret = 0;
    dlist_head list1;
    DEFINE_DLIST_HEAD(list2);
    struct dtest t1;
    struct dtest t2;
    struct dtest t3;

    dlist_head_init(&list1);
    ret += check_count(&list1, 0);
    ret += check_count(&list2, 0);

    if (!dlist_empty(&list1))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("dlist_empty() on empty list returns false\n");
        ret++;
    }

    t1.data = 1;
    t2.data = 2;
    t3.data = 3;

    dlist_add_head(&list1, &t1.l);
    ret += check_count(&list1, 1);
    ret += check_values(&list1, (unsigned[]){1, 0});
    dlist_add_head(&list1, &t2.l);
    ret += check_count(&list1, 2);
    ret += check_values(&list1, (unsigned[]){2, 1, 0});
    dlist_add_tail(&list1, &t3.l);
    ret += check_count(&list1, 3);
    ret += check_values(&list1, (unsigned[]){2, 1, 3, 0});

    dlist_remove(&t1.l);
    ret += check_count(&list1, 2);
    ret += check_values(&list1, (unsigned[]){2, 3, 0});

    if (dlist_empty(&list1))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("dlist_empty() on non-empty list returns true\n");
        ret++;
    }

    dlist_add_head(&list2, &t1.l);
    ret += check_count(&list2, 1);
    ret += check_values(&list2, (unsigned[]){1, 0});
    /* list1 hasn't changed */
    ret += check_count(&list1, 2);
    ret += check_values(&list1, (unsigned[]){2, 3, 0});

    return ret;
}
