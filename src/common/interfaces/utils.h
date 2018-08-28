/*
 *  Broadband Forum IEEE 1905.1/1a stack
 *
 *  Copyright (c) 2017, Broadband Forum
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

#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <stdlib.h> // malloc(), NULL
#include <stdio.h> // fprintf

/** @brief Get the number of elements in an array.
 *
 * Note that this simpel macro may evaluate its argument 0, 1 or 2 times, and that it doesn't check at all if the
 * parameter is indeed an array. Calling it with a pointer parameter will lead to incorrect results.
 */
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*(a)))


/** @ brief Allocate a chunk of 'n' bytes and return a pointer to it.
 *
 * If no memory can be allocated, this function exits immediately.
 */
static inline void *memalloc(size_t size)
{
    void *p;

    p = malloc(size);

    if (NULL == p)
    {
        fprintf(stderr, "ERROR: Out of memory!\n");
        exit(1);
    }

    return p;
}

typedef void (*visitor_callback) (void (*write_function)(const char *fmt, ...), const char *prefix, uint8_t size, const char *name, const char *fmt, const void *p);

// This is an auxiliary function which is used when calling the "visit_*()"
// family of functions so that the contents of the memory structures can be
// printed on screen for debugging/logging purposes.
//
// The documentation for any of the "visit_*()" function explains what this
// functions should do and look like.
//
void print_callback (void (*write_function)(const char *fmt, ...), const char *prefix, uint8_t size, const char *name, const char *fmt, const void *p);

#endif
