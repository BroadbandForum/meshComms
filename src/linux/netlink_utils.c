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
/*
 *  Code based on the 'iw' project from http://git.sipsolutions.net/iw.git/
 *
 *  Copyright (c) 2007, 2008        Johannes Berg
 *  Copyright (c) 2007              Andy Lutomirski
 *  Copyright (c) 2007              Mike Kershaw
 *  Copyright (c) 2008-2009         Luis R. Rodriguez
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h> // snprintf()
#include <stdlib.h> // atoi()

#include <sys/types.h> // open()
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> // close()/read()

#include "netlink_funcs.h"
#include "nl80211.h"
#include "hlist.h"

int phy_lookup(const char *basedir, char *name, mac_address *mac, int *index)
{
    char    buf[128];
    int     fd, n;

    #define _GET_FILE_CONTENT(b) do { \
        if ( (fd = open((b), O_RDONLY)) < 0 ) \
            return  0; \
        if ( (n = read(fd, (b), sizeof(b)-1)) <= 0 ) { \
            close(fd); \
            return -1; \
        } \
        close(fd); \
        (b)[n] = 0; \
        /* strip trailing newline, if present */ \
        if ((b)[n-1] == '\n') { \
            (b)[n-1] = 0; \
        } \
    } while (0)

    snprintf(buf, sizeof(buf), "%s/index", basedir);
    _GET_FILE_CONTENT(buf);
    *index = atoi(buf);

    snprintf(buf, sizeof(buf), "%s/macaddress", basedir);
    _GET_FILE_CONTENT(buf);
    asciiToMac(buf, mac);

    snprintf(buf, sizeof(buf), "%s/name", basedir);
    _GET_FILE_CONTENT(buf);
    strncpy(name, buf, T_RADIO_NAME_SZ-1);
    name[T_RADIO_NAME_SZ] = 0;

    #undef _GET_FILE_CONTENT

    return 1;
}

int ieee80211_channel_to_frequency(int chan, enum nl80211_band band)
{
	/* see 802.11 17.3.8.3.2 and Annex J
	 * there are overlapping channel numbers in 5GHz and 2GHz bands */
	if (chan <= 0)
		return 0; /* not supported */
	switch (band) {
	case NL80211_BAND_2GHZ:
		if (chan == 14)
			return 2484;
		else if (chan < 14)
			return 2407 + chan * 5;
		break;
	case NL80211_BAND_5GHZ:
		if (chan >= 182 && chan <= 196)
			return 4000 + chan * 5;
		else
			return 5000 + chan * 5;
		break;
	case NL80211_BAND_60GHZ:
		if (chan < 5)
			return 56160 + chan * 2160;
		break;
	default:
		;
	}
	return 0; /* not supported */
}

int ieee80211_frequency_to_channel(int freq)
{
	/* see 802.11-2007 17.3.8.3.2 and Annex J */
	if (freq == 2484)
		return 14;
	else if (freq < 2484)
		return (freq - 2407) / 5;
	else if (freq >= 4910 && freq <= 4980)
		return (freq - 4000) / 5;
	else if (freq <= 45000) /* DMG band lower limit */
		return (freq - 5000) / 5;
	else if (freq >= 58320 && freq <= 64800)
		return (freq - 56160) / 2160;
	else
		return 0;
}
