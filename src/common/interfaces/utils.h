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

// This is an auxiliary function which is used when calling the "visit_*()"
// family of functions so that the contents of the memory structures can be
// printed on screen for debugging/logging purposes.
//
// The documentation for any of the "visit_*()" function explains what this
// functions should do and look like.
//
void print_callback (void (*write_function)(const char *fmt, ...), const char *prefix, INT8U size, const char *name, const char *fmt, void *p);

#endif
