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

#include <stdio.h> // snprintf()
#include <stdlib.h> // atoi()

#include <sys/types.h> // open()
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> // close()/read()

#include "netlink_funcs.h"
#include "hlist.h"

extern const char *__sysfs_ieee80211;

int phy_lookup(struct _phy *p, const char *name)
{
    char    buf[256];
    int     fd, n;

    #define _GET_FILE_CONTENT(b) do { \
        int fd, n; \
        if ( (fd = open((b), O_RDONLY)) < 0 ) \
            return -1; \
        if ( (n = read(fd, (b), sizeof(b)-1)) <= 0 ) { \
            close(fd); \
            return -1; \
        } \
        close(fd); \
        (b)[n] = 0; \
    } while (0)

    snprintf(buf, sizeof(buf), "%s/%s/index", __sysfs_ieee80211, name);
    _GET_FILE_CONTENT(buf);
    p->index = atoi(buf);

    snprintf(buf, sizeof(buf), "%s/%s/macaddress", __sysfs_ieee80211, name);
    _GET_FILE_CONTENT(buf);
    asciiToMac(buf, &p->mac);

    #undef _GET_FILE_CONTENT

    return 1;
}
