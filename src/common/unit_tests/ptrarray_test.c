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

#include <ptrarray.h>
#include "platform.h"
#include <stdarg.h>
#include <string.h>

static PTRARRAY(unsigned) ptrarray;

static int check_count(unsigned expected_count)
{
    if (ptrarray.length != expected_count)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("ptrarray length %u but expected %u\n", ptrarray.length, expected_count);
        return 1;
    }
    else
    {
        return 0;
    }
}

static int check_values(const unsigned values[])
{
    for (unsigned i = 0; i < ptrarray.length; i++)
    {
        if (values[i] == 0)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("ptrarray includes unexpected element %u\n", ptrarray.data[i]);
            return 1;
        }
        if (values[i] != ptrarray.data[i])
        {
            PLATFORM_PRINTF_DEBUG_WARNING("dlist includes unexpected element %u != %u\n", ptrarray.data[i], values[i]);
            return 1;
        }
    }
    return 0;
}

int main()
{
    int ret = 0;

    ret += check_count(0);

    PTRARRAY_ADD(ptrarray, 1);
    ret += check_count(1);
    ret += check_values((unsigned[]){1, 0});
    PTRARRAY_ADD(ptrarray, 2);
    ret += check_count(2);
    ret += check_values((unsigned[]){1, 2, 0});
    PTRARRAY_ADD(ptrarray, 3);
    ret += check_count(3);
    ret += check_values((unsigned[]){1, 2, 3, 0});

    if (PTRARRAY_FIND(ptrarray, 2) != 1)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Element '2' not found in ptrarray");
        ret++;
    }
    if (PTRARRAY_FIND(ptrarray, 0) < 3)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Element '4' found in ptrarray");
        ret++;
    }

    PTRARRAY_REMOVE(ptrarray, 0);
    ret += check_count(2);
    ret += check_values((unsigned[]){2, 3, 0});

    PTRARRAY_REMOVE_ELEMENT(ptrarray, 1);
    /* Element 1 not in list => nothing changed */
    ret += check_count(2);
    ret += check_values((unsigned[]){2, 3, 0});

    PTRARRAY_REMOVE_ELEMENT(ptrarray, 3);
    ret += check_count(1);
    ret += check_values((unsigned[]){2, 0});

    /* Same element can be added multiple times. */
    PTRARRAY_ADD(ptrarray, 2);
    ret += check_count(2);
    ret += check_values((unsigned[]){2, 2, 0});

    PTRARRAY_CLEAR(ptrarray);
    ret += check_count(0);
    PTRARRAY_ADD(ptrarray, 1);
    ret += check_count(1);
    ret += check_values((unsigned[]){1, 0});

    PTRARRAY_CLEAR(ptrarray);

    return ret;
}
