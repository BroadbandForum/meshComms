/*
 *  Broadband Forum IEEE 1905.1/1a stack
 *
 *  Copyright (c) 2017, Broadband Forum
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

#include "platform.h"
#include "utils.h"

#include <string.h> // memcmp(), strncat()
#include <stdio.h> // snprintf()

////////////////////////////////////////////////////////////////////////////////
// Public API
////////////////////////////////////////////////////////////////////////////////
//
void print_callback(void (*write_function)(const char *fmt, ...), const char *prefix, uint8_t size, const char *name, const char *fmt, const void *p)
{

       if (0 == memcmp(fmt, "%s", 3))
       {
           // Strings are printed with triple quotes surrounding them
           //
           write_function("%s%s: \"\"\"%s\"\"\"\n", prefix, name, p);
           return;
       }
       else if (0 == memcmp(fmt, "%ipv4", 6))
       {
           // This is needed because "size == 4" on IPv4 addresses, but we don't
           // want them to be treated as 4 bytes integers, so we change the
           // "fmt" to "%d" and do *not* returns (so that the IPv4 ends up being
           // printed as a sequence of bytes.
           //
           fmt = "%d";
       }
       else
       {
           #define FMT_LINE_SIZE 20
           char fmt_line[FMT_LINE_SIZE];

           fmt_line[0] = 0x0;

           snprintf(fmt_line, FMT_LINE_SIZE-1, "%%s%%s: %s\n", fmt);
           fmt_line[FMT_LINE_SIZE-1] = 0x0;

           if (1 == size)
           {
               write_function(fmt_line, prefix, name, *(const uint8_t *)p);

               return;
           }
           else if (2 == size)
           {
               write_function(fmt_line, prefix, name, *(const uint16_t *)p);

               return;
           }
           else if (4 == size)
           {
               write_function(fmt_line, prefix, name, *(const uint32_t *)p);

               return;
           }
       }

       // If we get here, it's either an IPv4 address or a sequence of bytes
       //
       {
           #define AUX1_SIZE 200  // Store a whole output line
           #define AUX2_SIZE  20  // Store a fmt conversion

           uint16_t i, j;

           char aux1[AUX1_SIZE];
           char aux2[AUX2_SIZE];

           size_t aux1_offset = 0;
           size_t space_left = AUX1_SIZE;
           /* Take into account prefix length */
           space_left -= strlen(prefix);
           space_left -= strlen(name);
           space_left -= 2; // ": "

           aux1[0] = '\0';

           for (i=0; i<size; i++)
           {
               // Write one element to aux2
               //
               j = snprintf(aux2, AUX2_SIZE-1, fmt, *((const uint8_t *)p+i));
               if (j >= AUX2_SIZE)
               {
                   j = AUX2_SIZE - 1;
                   aux2[AUX2_SIZE-1] = '\0';
               }

               // Check if there is enough space left in "aux1"
               //
               //   NOTE: The "+2" is because we are going to append to "aux1"
               //         the contents of "aux2" plus a ", " string (which is
               //         three chars long)
               //         The second "+2" is because of the final "\n"
               //
               if (j+2+2 > space_left)
               {
                   // No more space left
                   // Since space_left starts as AUX1_SIZE - 2, and we keep space for 2 additional characters in the
                   // condition above, there is certainly space left for ...
                   strcpy(&aux1[aux1_offset], "...");
                   space_left -= 3;
                   aux1_offset += 3;
                   break;
               }
               // Because of the test above, there is certainly space for aux2.
               strcpy(&aux1[aux1_offset], aux2);
               space_left -= j;
               aux1_offset += j;

               strcpy(&aux1[aux1_offset], ", ");
               space_left -= 2;
               aux1_offset += 2;
           }

           write_function("%s%s: %s\n", prefix, name, aux1);
       }

       return;
}


