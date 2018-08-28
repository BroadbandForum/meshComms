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

// In the comments below, every time a reference is made (ex: "See Section 6.4"
// or "See Table 6-11") we are talking about the contents of the following
// document:
//
//   "IEEE Std 1905.1-2013"

#include "platform.h"
#include "utils.h"
#include "packet_tools.h"

#include "1905_tlvs.h"
#include "1905_cmdus.h"
#include "1905_alme.h"
#include "1905_l2.h"
#include "lldp_tlvs.h"
#include "lldp_payload.h"

#include "al.h"
#include "al_datamodel.h"
#include "al_send.h"
#include "al_recv.h"
#include "al_utils.h"
#include "al_extension.h"

#include "platform_interfaces.h"
#include "platform_os.h"
#include "platform_alme_server.h"

#include <string.h> // memcmp(), memcpy(), ...

#define TIMER_TOKEN_DISCOVERY          (1)
#define TIMER_TOKEN_GARBAGE_COLLECTOR  (2)


////////////////////////////////////////////////////////////////////////////////
// Private functions and data
////////////////////////////////////////////////////////////////////////////////

// CMDUs can be received in multiple fragments/packets when they are too big to
// fit in a single "network transmission unit" (which is never bigger than
// MAX_NETWORK_SEGMENT_SIZE).
//
// Fragments that belong to one same CMDU contain the same 'mid' and different
// 'fragment id' values. In addition, the last fragment is the only one to
// contain the 'last fragment indicator' field set.
//
//   NOTE: This is all also explained in "Sections 7.1.1 and 7.1.2"
//
// This function will "buffer" fragments until either all pieces arrive or a
// timer expires (in which case all previous fragments are discarded/ignored)
//
//   NOTE: Instead of a timer, we will use a buffer that holds up to
//         MAX_MIDS_IN_FLIGHT CMDUs.
//         If we are still waiting for MAX_MIDS_IN_FLIGHT CMDUs to be completed
//         (ie. we haven't received all their fragments yet), and a new fragment
//         for a new CMDU arrives, we will discard all fragments from the
//         oldest one.
//
// Every time this function is called, two things can happen:
//
//   1. The just received fragment was the last one needed to complete a CMDU.
//      In this case, the CMDU structure result of all those fragments being
//      parsed is returned.
//
//   2. The just received fragment is not yet the last one needed to complete a
//      CMDU. In this case the fragment is internally buffered (ie. the caller
//      does not need to keep the passed buffer around in memory) and this
//      function returns NULL.
//
// This function received two arguments:
//
//   - 'packet_buffer' is a pointer to the received stream containing a
//     fragment (or a whole) CMDU
//
//   - 'len' is the length of this 'packet_buffer' in bytes
//
struct CMDU *_reAssembleFragmentedCMDUs(uint8_t *packet_buffer, uint16_t len)
{
    #define MAX_MIDS_IN_FLIGHT     5
    #define MAX_FRAGMENTS_PER_MID  3

    // This is a static structure used to store the fragments belonging to up to
    // 'MAX_MIDS_IN_FLIGHT' CMDU messages.
    // Initially all entries are marked as "empty" by setting the 'in_use' field
    // to "0"
    //
    static struct _midsInFlight
    {
        uint8_t in_use;  // Is this entry free?

        uint8_t mid;     // 'mid' associated to this CMDU

        uint8_t src_addr[6];
        uint8_t dst_addr[6];
                       // These two (together with the 'mid' field) will be used
                       // to identify fragments belonging to one same CMDU.

        uint8_t fragments[MAX_FRAGMENTS_PER_MID];
                       // Each entry represents a fragment number.
                       //   - "1" means that fragment has been received
                       //   - "0" means no fragment with that number has been
                       //     received.

        uint8_t last_fragment;
                       // Number of the fragment carrying the
                       // 'last_fragment_indicator' flag.
                       // This is always a number between 0 and
                       // MAX_FRAGMENTS_PER_MID-1.
                       // Iniitally it is set to "MAX_FRAGMENTS_PER_MID",
                       // meaning that no fragment with the
                       // 'last_fragment_indicator' flag has been received yet.

        uint8_t *streams[MAX_FRAGMENTS_PER_MID+1];
                       // Each of the bit streams associated to each fragment
                       //
                       // The size is "MAX_FRAGMENTS_PER_MID+1" instead of
                       // "MAX_FRAGMENTS_PER_MID" to store a final NULL entry
                       // (this makes it easier to later call
                       // "parse_1905_CMDU_header_from_packet()"

        uint32_t age;    // Used to keep track of which is the oldest CMDU for
                       // which a fragment was received (so that we can free
                       // it when the CMDUs buffer is full)

    } mids_in_flight[MAX_MIDS_IN_FLIGHT] = \
    {[ 0 ... MAX_MIDS_IN_FLIGHT-1 ] = (struct _midsInFlight) { .in_use = 0 }};

    static uint32_t current_age = 0;

    uint8_t  i, j;
    uint8_t *p;
    struct CMDU_header cmdu_header;

    if (!parse_1905_CMDU_header_from_packet(packet_buffer, len, &cmdu_header))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not retrieve CMDU header from bit stream\n");
        return NULL;
    }
    PLATFORM_PRINTF_DEBUG_DETAIL("mid = %d, fragment_id = %d, last_fragment_indicator = %d\n",
                                 cmdu_header.mid, cmdu_header.fragment_id, cmdu_header.last_fragment_indicator);

    // Skip over ethernet header
    p = packet_buffer + (6+6+2);
    len -= (6+6+2);

    // Find the set of streams associated to this 'mid' and add the just
    // received stream to its set of streams
    //
    for (i = 0; i<MAX_MIDS_IN_FLIGHT; i++)
    {
        if (
                                              1        ==  mids_in_flight[i].in_use          &&
                                  cmdu_header.mid      ==  mids_in_flight[i].mid             &&
             0 == memcmp(cmdu_header.dst_addr,    mids_in_flight[i].dst_addr, 6)    &&
             0 == memcmp(cmdu_header.src_addr,    mids_in_flight[i].src_addr, 6)
           )
        {
            // Fragments for this 'mid' have previously been received. Add this
            // new one to the set.

            // ...but first check for errors
            //
            if (cmdu_header.fragment_id > MAX_FRAGMENTS_PER_MID)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Too many fragments (%d) for one same CMDU (max supported is %d)\n",
                                            cmdu_header.fragment_id, MAX_FRAGMENTS_PER_MID);
                PLATFORM_PRINTF_DEBUG_ERROR("  mid      = %d\n", cmdu_header.mid);
                PLATFORM_PRINTF_DEBUG_ERROR("  src_addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
                                            cmdu_header.src_addr[0], cmdu_header.src_addr[1], cmdu_header.src_addr[2],
                                            cmdu_header.src_addr[3], cmdu_header.src_addr[4], cmdu_header.src_addr[5]);
                PLATFORM_PRINTF_DEBUG_ERROR("  dst_addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
                                            cmdu_header.dst_addr[0], cmdu_header.dst_addr[1], cmdu_header.dst_addr[2],
                                            cmdu_header.dst_addr[3], cmdu_header.dst_addr[4], cmdu_header.dst_addr[5]);
                return NULL;
            }

            if (1 == mids_in_flight[i].fragments[cmdu_header.fragment_id])
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Ignoring duplicated fragment #%d\n", cmdu_header.fragment_id);
                PLATFORM_PRINTF_DEBUG_WARNING("  mid      = %d\n", cmdu_header.mid);
                PLATFORM_PRINTF_DEBUG_WARNING("  src_addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
                                              cmdu_header.src_addr[0], cmdu_header.src_addr[1], cmdu_header.src_addr[2],
                                              cmdu_header.src_addr[3], cmdu_header.src_addr[4], cmdu_header.src_addr[5]);
                PLATFORM_PRINTF_DEBUG_WARNING("  dst_addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
                                              cmdu_header.dst_addr[0], cmdu_header.dst_addr[1], cmdu_header.dst_addr[2],
                                              cmdu_header.dst_addr[3], cmdu_header.dst_addr[4], cmdu_header.dst_addr[5]);
                return NULL;
            }

            if (1 == cmdu_header.last_fragment_indicator && MAX_FRAGMENTS_PER_MID != mids_in_flight[i].last_fragment)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("This fragment (#%d) and a previously received one (#%d) both contain the 'last_fragment_indicator' flag set. Ignoring...\n",
                                              cmdu_header.fragment_id, mids_in_flight[i].last_fragment);
                PLATFORM_PRINTF_DEBUG_WARNING("  mid      = %d\n", cmdu_header.mid);
                PLATFORM_PRINTF_DEBUG_WARNING("  src_addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
                                              cmdu_header.src_addr[0], cmdu_header.src_addr[1], cmdu_header.src_addr[2],
                                              cmdu_header.src_addr[3], cmdu_header.src_addr[4], cmdu_header.src_addr[5]);
                PLATFORM_PRINTF_DEBUG_WARNING("  dst_addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
                                              cmdu_header.dst_addr[0], cmdu_header.dst_addr[1], cmdu_header.dst_addr[2],
                                              cmdu_header.dst_addr[3], cmdu_header.dst_addr[4], cmdu_header.dst_addr[5]);
                return NULL;
            }

            // ...and now actually save the stream for later
            //
            mids_in_flight[i].fragments[cmdu_header.fragment_id] = 1;

            if (1 == cmdu_header.last_fragment_indicator)
            {
                mids_in_flight[i].last_fragment = cmdu_header.fragment_id;
            }

            mids_in_flight[i].streams[cmdu_header.fragment_id] = (uint8_t *)memalloc((sizeof(uint8_t) * len));
            memcpy(mids_in_flight[i].streams[cmdu_header.fragment_id], p, len);

            mids_in_flight[i].age = current_age++;

            break;
        }
    }

    // If we get inside the next "if()", that means no previous entry matches
    // this 'mid' + 'src_addr' + 'dst_addr' tuple.
    // What we have to do then is to search for an empty slot and add this as
    // the first stream associated to this new tuple.
    //
    if (MAX_MIDS_IN_FLIGHT == i)
    {
        for (i = 0; i<MAX_MIDS_IN_FLIGHT; i++)
        {
            if (0 == mids_in_flight[i].in_use)
            {
                break;
            }
        }

        if (MAX_MIDS_IN_FLIGHT == i)
        {
            // All slots are in use!!
            //
            // We need to discard the oldest one (ie. the one with the lowest
            // 'age')
            //
            uint32_t lowest_age;

            lowest_age = mids_in_flight[0].age;
            j          = 0;

            for (i=1; i<MAX_MIDS_IN_FLIGHT; i++)
            {
                if (mids_in_flight[i].age < lowest_age)
                {
                    lowest_age = mids_in_flight[i].age;
                    j          = i;
                }
            }

            PLATFORM_PRINTF_DEBUG_WARNING("Discarding old CMDU fragments to make room for the just received one. CMDU being discarded:\n");
            PLATFORM_PRINTF_DEBUG_WARNING("  mid      = %d\n", mids_in_flight[j].mid);
            PLATFORM_PRINTF_DEBUG_WARNING("  mids_in_flight[j].src_addr = %02x:%02x:%02x:%02x:%02x:%02x\n", mids_in_flight[j].src_addr[0], mids_in_flight[j].src_addr[1], mids_in_flight[j].src_addr[2], mids_in_flight[j].src_addr[3], mids_in_flight[j].src_addr[4], mids_in_flight[j].src_addr[5]);
            PLATFORM_PRINTF_DEBUG_WARNING("  mids_in_flight[j].dst_addr = %02x:%02x:%02x:%02x:%02x:%02x\n", mids_in_flight[j].dst_addr[0], mids_in_flight[j].dst_addr[1], mids_in_flight[j].dst_addr[2], mids_in_flight[j].dst_addr[3], mids_in_flight[j].dst_addr[4], mids_in_flight[j].dst_addr[5]);

            for (i=0; i<MAX_FRAGMENTS_PER_MID; i++)
            {
                if (1 == mids_in_flight[j].fragments[i] && NULL != mids_in_flight[j].streams[i])
                {
                    free(mids_in_flight[j].streams[i]);
                }
            }

            mids_in_flight[j].in_use = 0;

            i = j;
        }

        // Now that we have our empty slot, initialize it and fill it with the
        // just received stream:
        //
        mids_in_flight[i].in_use = 1;
        mids_in_flight[i].mid    = cmdu_header.mid;

        memcpy(mids_in_flight[i].src_addr, cmdu_header.src_addr, 6);
        memcpy(mids_in_flight[i].dst_addr, cmdu_header.dst_addr, 6);

        for (j=0; j<MAX_FRAGMENTS_PER_MID; j++)
        {
            mids_in_flight[i].fragments[j] = 0;
            mids_in_flight[i].streams[j]   = NULL;
        }
        mids_in_flight[i].streams[MAX_FRAGMENTS_PER_MID] = NULL;

        mids_in_flight[i].fragments[cmdu_header.fragment_id]  = 1;
        mids_in_flight[i].streams[cmdu_header.fragment_id]    = (uint8_t *)memalloc((sizeof(uint8_t) * len));
        memcpy(mids_in_flight[i].streams[cmdu_header.fragment_id], p, len);

        if (1 == cmdu_header.last_fragment_indicator)
        {
            mids_in_flight[i].last_fragment = cmdu_header.fragment_id;
        }
        else
        {
            mids_in_flight[i].last_fragment = MAX_FRAGMENTS_PER_MID;
              // NOTE: This means "no 'last_fragment_indicator' flag has been
              //       received yet.
        }

        mids_in_flight[i].age = current_age++;
    }

    // At this point we have an entry in the 'mids_in_flight' array (entry 'i')
    // where a new stream/fragment has been added.
    //
    // We now have to check if we have received all fragments for this 'mid'
    // and, if so, process them and obtain a CMDU structure that will be
    // returned to the caller of the function.
    //
    // Otherwise, return NULL.
    //
    if (MAX_FRAGMENTS_PER_MID != mids_in_flight[i].last_fragment)
    {
        struct CMDU *c;

        for (j=0; j<=mids_in_flight[i].last_fragment; j++)
        {
            if (0 == mids_in_flight[i].fragments[j])
            {
                PLATFORM_PRINTF_DEBUG_DETAIL("We still have to wait for more fragments to complete the CMDU message\n");
                return NULL;
            }
        }

        c = parse_1905_CMDU_from_packets(mids_in_flight[i].streams);

        if (NULL == c)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("parse_1905_CMDU_header_from_packet() failed\n");
        }
        else
        {
            PLATFORM_PRINTF_DEBUG_DETAIL("All fragments belonging to this CMDU have already been received and the CMDU structure is ready\n");
        }

        for (j=0; j<=mids_in_flight[i].last_fragment; j++)
        {
            free(mids_in_flight[i].streams[j]);
        }
        mids_in_flight[i].in_use = 0;

        return c;
    }

    PLATFORM_PRINTF_DEBUG_DETAIL("The last fragment has not yet been received\n");
    return NULL;
}

// Returns '1' if the packet has already been processed in the past and thus,
// should be discarded (to avoid network storms). '0' otherwise.
//
// According to what is explained in "Sections 7.5, 7.6 and 7.7" if a
// defragmented packet whose "AL MAC address TLV" and "message id" match one
// that has already been received in the past, then it should be discarded.
//
// I *personally* think the standard is "slightly" wrong here because *not* all
// CMDUs contain an "AL MAC address TLV".
// We could use the ethernet source address instead, however this would only
// work for those messages that are *not* relayed (one same duplicated relayed
// message can arrive at our local node with two different ethernet source
// addresses).
// Fortunately for us, all relayed CMDUs *do* contain an "AL MAC address TLV",
// thus this is what we are going to do:
//
//   1. If the CMDU is a relayed one, check against the "AL MAC" contained in
//      the "AL MAC address TLV"
//
//   2. If the CMDU is *not* a relayed one, check against the ethernet source
//      address
//
// This function keeps track of the latest MAX_DUPLICATES_LOG_ENTRIES tuples
// of ("mac_address", "message_id") and:
//
//   1. If the provided tuple matches an already existing one, this function
//      returns '1'
//
//   2. Otherwise, the entry is added (discarding, if needed, the oldest entry)
//      and this function returns '0'
//
uint8_t _checkDuplicates(uint8_t *src_mac_address, struct CMDU *c)
{
    #define MAX_DUPLICATES_LOG_ENTRIES 10

    static uint8_t  mac_addresses[MAX_DUPLICATES_LOG_ENTRIES][6];
    static uint16_t message_ids  [MAX_DUPLICATES_LOG_ENTRIES];

    static uint8_t start = 0;
    static uint8_t total = 0;

    uint8_t mac_address[6];

    uint8_t i;

    if(
        CMDU_TYPE_TOPOLOGY_RESPONSE               == c->message_type ||
        CMDU_TYPE_LINK_METRIC_RESPONSE            == c->message_type ||
        CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE   == c->message_type ||
        CMDU_TYPE_HIGHER_LAYER_RESPONSE           == c->message_type ||
        CMDU_TYPE_INTERFACE_POWER_CHANGE_RESPONSE == c->message_type ||
        CMDU_TYPE_GENERIC_PHY_RESPONSE            == c->message_type
      )
    {
        // This is a "hack" until a better way to handle MIDs is found.
        //
        // Let me explain.
        //
        // According to the standard, each AL entity generates its monotonically
        // increasing MIDs every time a new packet is sent.
        // The only exception to this rule is when generating a "response". In
        // these cases the same MID contained in the original query must be
        // used.
        //
        // Imagine we have two ALs that are started in different moments:
        //
        //        AL 1               AL 2
        //        ====               ====
        //   t=0  --- MID=1 -->
        //   t=1  --- MID=2 -->
        //   t=2  --- MID=3 -->      <-- MID=1 --
        //   t=3  --- MID=4 -->      <-- MID=2 --
        //   t=4  --- MID=5 -->      <-- MID=3 --
        //
        // In "t=2", "AL 2" learns that, in the future, messages from "AL 1" with
        // a "MID=3" should be discarded.
        //
        // Now, imagine in "t=4" the message "AL 2" sends (with "MID=3") is a
        // query that triggers a response from "AL 1" (which *must* have the
        // same MID, ie., "MID=3").
        //
        // HOWEVER, because of what "AL 2" learnt in "t=2", this response will
        // be discarded!
        //
        // In oder words... until the standard clarifies how MIDs should be
        // generated to avoid this problem, we will just accept (and process)
        // all response messages... even if they are duplicates.
        //
        return 0;
    }

    // For relayed CMDUs, use the AL MAC, otherwise use the ethernet src MAC.
    //
    memcpy(mac_address, src_mac_address, 6);
    if (1 == c->relay_indicator)
    {
        uint8_t i;
        uint8_t *p;

        i = 0;
        while (NULL != (p = c->list_of_TLVs[i]))
        {
            if (TLV_TYPE_AL_MAC_ADDRESS_TYPE == *p)
            {
                struct alMacAddressTypeTLV *t = (struct alMacAddressTypeTLV *)p;

                memcpy(mac_address, t->al_mac_address, 6);
                break;
            }
            i++;
        }
    }

    // Also, discard relayed CMDUs whose AL MAC is our own (that means someone
    // is retrasnmitting us back a message we originally created)
    //
    if (1 == c->relay_indicator)
    {
        if (0 == memcmp(mac_address, DMalMacGet(), 6))
        {
            return 1;
        }
    }

    // Find if the ("mac_address", "message_id") tuple is already present in the
    // database
    //
    for (i=0; i<total; i++)
    {
        uint8_t index;

        index = (start + i) % MAX_DUPLICATES_LOG_ENTRIES;

        if (
             0 == memcmp(mac_addresses[index],    mac_address, 6) &&
                                  message_ids[index]    == c->message_id
           )
        {
            // The entry already exists!
            //
            return 1;
        }
    }

    // This is a new entry, insert it into the cache and return "0"
    //
    if (total < MAX_DUPLICATES_LOG_ENTRIES)
    {
        // There is space for new entries
        //
        uint8_t index;

        index = (start + total) % MAX_DUPLICATES_LOG_ENTRIES;

        memcpy(mac_addresses[index], mac_address, 6);
        message_ids[index] = c->message_id;

        total++;
    }
    else
    {
        // We need to replace the oldest entry
        //
        memcpy(mac_addresses[start], mac_address, 6);
        message_ids[start] = c->message_id;

        start++;

        start = start % MAX_DUPLICATES_LOG_ENTRIES;
    }

    return 0;
}

// According to "Section 7.6", if a received packet has the "relayed multicast"
// bit set, after processing, we must forward it on all authenticated 1905
// interfaces (except on the one where it was received).
//
// This function checks if the provided 'c' structure has that "relayed
// multicast" flag set and, if so, retransmits it on all local interfaces
// (except for the one whose MAC address matches 'receiving_interface_addr') to
// 'destination_mac_addr' and the same "message id" (MID) as the one contained
// in the originally received 'c' structure.
//
void _checkForwarding(uint8_t *receiving_interface_addr, uint8_t *destination_mac_addr, struct CMDU *c)
{
    uint8_t i;

    if (c->relay_indicator)
    {
        char **ifs_names;
        uint8_t  ifs_nr;

        char *aux;

        PLATFORM_PRINTF_DEBUG_DETAIL("Relay multicast flag set. Forwarding...\n");

        ifs_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&ifs_nr);
        for (i=0; i<ifs_nr; i++)
        {
            uint8_t authenticated;
            uint8_t power_state;

            struct interfaceInfo *x;

            x = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
            if (NULL == x)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);
                authenticated = 0;
                power_state   = INTERFACE_POWER_STATE_OFF;
            }
            else
            {
                authenticated = x->is_secured;
                power_state   = x->power_state;
            }

            if (
                (0 == authenticated                                                                     ) ||
                ((power_state != INTERFACE_POWER_STATE_ON) && (power_state!= INTERFACE_POWER_STATE_SAVE)) ||
                (0 == memcmp(x->mac_address, receiving_interface_addr, 6))
               )
            {
                // Do not forward the message on this interface
                //
                if (NULL != x)
                {
                    free_1905_INTERFACE_INFO(x);
                }
                continue;
            }

            if (NULL != x)
            {
                free_1905_INTERFACE_INFO(x);
            }

            // Retransmit message
            //
            switch (c->message_type)
            {
                case CMDU_TYPE_TOPOLOGY_DISCOVERY:
                {
                    aux = "CMDU_TYPE_TOPOLOGY_DISCOVERY";
                    break;
                }
                case CMDU_TYPE_TOPOLOGY_NOTIFICATION:
                {
                    aux = "CMDU_TYPE_TOPOLOGY_NOTIFICATION";
                    break;
                }
                case CMDU_TYPE_TOPOLOGY_QUERY:
                {
                    aux = "CMDU_TYPE_TOPOLOGY_QUERY";
                    break;
                }
                case CMDU_TYPE_TOPOLOGY_RESPONSE:
                {
                    aux = "CMDU_TYPE_TOPOLOGY_RESPONSE";
                    break;
                }
                case CMDU_TYPE_VENDOR_SPECIFIC:
                {
                    aux = "CMDU_TYPE_VENDOR_SPECIFIC";
                    break;
                }
                case CMDU_TYPE_LINK_METRIC_QUERY:
                {
                    aux = "CMDU_TYPE_LINK_METRIC_QUERY";
                    break;
                }
                case CMDU_TYPE_LINK_METRIC_RESPONSE:
                {
                    aux = "CMDU_TYPE_LINK_METRIC_RESPONSE";
                    break;
                }
                case CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH:
                {
                    aux = "CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH";
                    break;
                }
                case CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE:
                {
                    aux = "CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE";
                    break;
                }
                case CMDU_TYPE_AP_AUTOCONFIGURATION_WSC:
                {
                    aux = "CMDU_TYPE_AP_AUTOCONFIGURATION_WSC";
                    break;
                }
                case CMDU_TYPE_AP_AUTOCONFIGURATION_RENEW:
                {
                    aux = "CMDU_TYPE_AP_AUTOCONFIGURATION_RENEW";
                    break;
                }
                case CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION:
                {
                    aux = "CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION";
                    break;
                }
                case CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION:
                {
                    aux = "CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION";
                    break;
                }
                default:
                {
                    aux = "UNKNOWN";
                    break;
                }
            }
            PLATFORM_PRINTF_DEBUG_INFO("--> %s (forwarding from %s to %s)\n", aux, DMmacToInterfaceName(receiving_interface_addr), ifs_names[i]);

            if (0 == send1905RawPacket(ifs_names[i], c->message_id, destination_mac_addr, c))
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not retransmit 1905 message on interface %s\n", x->name);
            }
        }
        free_LIST_OF_1905_INTERFACES(ifs_names, ifs_nr);
    }

    return;
}

// This function sends an "AP-autoconfig search" message on all authenticated
// interfaces BUT ONLY if there is at least one unconfigured AP interface on
// this node.
//
// A function has been created for this because the same code is executed from
// three different places:
//
//   - When a new interface becomes authenticated
//
//   - When the *local* push button is pressed and there is at least one
//     interface which does not support this configuration mechanism (ex:
//     ethernet)
//
//   - After a local unconfigured AP interface becomes configured (this is
//     needed in the unlikely situation where there are more than one
//     unconfigured APs in the same node)
//
void _triggerAPSearchProcess(void)
{
    uint8_t  i;
    uint16_t mid;

    char **ifs_names;
    uint8_t  ifs_nr;

    uint8_t unconfigured_ap_exists = 0;
    uint8_t unconfigured_ap_band   = 0;

    ifs_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&ifs_nr);

    for (i=0; i<ifs_nr; i++)
    {
        struct interfaceInfo *x;

        x = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
        if (NULL == x)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);
            continue;
        }

        if (
            (INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ == x->interface_type ||
             INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ == x->interface_type ||
             INTERFACE_TYPE_IEEE_802_11A_5_GHZ   == x->interface_type ||
             INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ == x->interface_type ||
             INTERFACE_TYPE_IEEE_802_11N_5_GHZ   == x->interface_type ||
             INTERFACE_TYPE_IEEE_802_11AC_5_GHZ  == x->interface_type ||
             INTERFACE_TYPE_IEEE_802_11AD_60_GHZ == x->interface_type)   &&
            IEEE80211_ROLE_AP == x->interface_type_data.ieee80211.role   &&
            (0x0 == x->interface_type_data.ieee80211.bssid[0] &&
             0x0 == x->interface_type_data.ieee80211.bssid[1] &&
             0x0 == x->interface_type_data.ieee80211.bssid[2] &&
             0x0 == x->interface_type_data.ieee80211.bssid[3] &&
             0x0 == x->interface_type_data.ieee80211.bssid[4] &&
             0x0 == x->interface_type_data.ieee80211.bssid[5])
           )
        {
           unconfigured_ap_exists = 1;

           if (
                INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ == x->interface_type ||
                INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ == x->interface_type ||
                INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ == x->interface_type
              )
           {
               unconfigured_ap_band = IEEE80211_FREQUENCY_BAND_2_4_GHZ;
           }
           else if (
                INTERFACE_TYPE_IEEE_802_11A_5_GHZ  == x->interface_type ||
                INTERFACE_TYPE_IEEE_802_11N_5_GHZ  == x->interface_type ||
                INTERFACE_TYPE_IEEE_802_11AC_5_GHZ == x->interface_type
                )
           {
               unconfigured_ap_band = IEEE80211_FREQUENCY_BAND_5_GHZ;
           }
           else if (
                INTERFACE_TYPE_IEEE_802_11AD_60_GHZ == x->interface_type
                )
           {
               unconfigured_ap_band = IEEE80211_FREQUENCY_BAND_60_GHZ;
           }
           else
           {
               PLATFORM_PRINTF_DEBUG_WARNING("Unknown interface type %s\n", x->interface_type);
               unconfigured_ap_exists = 0;

               free_1905_INTERFACE_INFO(x);
               continue;
           }

           break;
        }

        free_1905_INTERFACE_INFO(x);
    }

    if (1 == unconfigured_ap_exists)
    {
        mid = getNextMid();
        for (i=0; i<ifs_nr; i++)
        {
            uint8_t authenticated;
            uint8_t power_state;

            struct interfaceInfo *x;

            x = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
            if (NULL == x)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);
                authenticated = 0;
                power_state   = INTERFACE_POWER_STATE_OFF;
            }
            else
            {
                authenticated = x->is_secured;
                power_state   = x->power_state;
            }

            if (
                (0 == authenticated                                                                     ) ||
                ((power_state != INTERFACE_POWER_STATE_ON) && (power_state!= INTERFACE_POWER_STATE_SAVE))
               )
            {
                // Do not send the message on this interface
                //
                if (NULL != x)
                {
                    free_1905_INTERFACE_INFO(x);
                }
                continue;
            }

            if (NULL != x)
            {
                free_1905_INTERFACE_INFO(x);
            }

            if (0 == send1905APAutoconfigurationSearchPacket(ifs_names[i], mid, unconfigured_ap_band))
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not send 1905 AP-autoconfiguration search message\n");
            }
        }
    }

    free_LIST_OF_1905_INTERFACES(ifs_names, ifs_nr);
}


////////////////////////////////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////////////////////////////////

uint8_t start1905AL(uint8_t *al_mac_address, uint8_t map_whole_network_flag, char *registrar_interface)
{
    uint8_t   queue_id;
    uint8_t  *queue_message;

    char   **interfaces_names;
    uint8_t    interfaces_nr;

    uint8_t i;

    // Initialize platform-specific code
    //
    if (0 == PLATFORM_INIT())
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Failed to initialize platform\n");
        return AL_ERROR_OS;
    }

    if (NULL == al_mac_address)
    {
        // Invalid arguments
        //
        PLATFORM_PRINTF_DEBUG_ERROR("NULL AL MAC address not allowed\n");
        return AL_ERROR_INVALID_ARGUMENTS;
    }

    // Insert the provided AL MAC address into the database
    //
    DMinit();
    DMalMacSet(al_mac_address);
    DMmapWholeNetworkSet(map_whole_network_flag);
    PLATFORM_PRINTF_DEBUG_DETAIL("Starting AL entity (AL MAC = %02x:%02x:%02x:%02x:%02x:%02x). Map whole network = %d...\n",
                                al_mac_address[0],
                                al_mac_address[1],
                                al_mac_address[2],
                                al_mac_address[3],
                                al_mac_address[4],
                                al_mac_address[5],
                                map_whole_network_flag);

    // Obtain the list of interfaces that the AL entity is going to manage
    //
    PLATFORM_PRINTF_DEBUG_DETAIL("Retrieving list of interfaces visible to the 1905 AL entity...\n");
    interfaces_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&interfaces_nr);
    if (NULL == interfaces_names)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("No interfaces detected\n");
        return AL_ERROR_NO_INTERFACES;
    }

    for (i=0; i<interfaces_nr; i++)
    {
        struct interfaceInfo *x;

        x = PLATFORM_GET_1905_INTERFACE_INFO(interfaces_names[i]);
        if (NULL == x)
        {
            PLATFORM_PRINTF_DEBUG_ERROR("Could not retrieve interface info for %s\n", interfaces_names[i]);
            continue;
        }

        DMinsertInterface(x->name, x->mac_address);

        PLATFORM_PRINTF_DEBUG_DETAIL("    - %s --> %02x:%02x:%02x:%02x:%02x:%02x\n",
                                   x->name,
                                   x->mac_address[0],
                                   x->mac_address[1],
                                   x->mac_address[2],
                                   x->mac_address[3],
                                   x->mac_address[4],
                                   x->mac_address[5]);

        // If this interface is the designated 1905 network registrar
        // interface, save its MAC address to the database
        //
        if (NULL != registrar_interface)
        {
            if (0 == memcmp(x->name, registrar_interface, strlen(x->name)))
            {
                if (
                     INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ != x->interface_type  &&
                     INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ != x->interface_type  &&
                     INTERFACE_TYPE_IEEE_802_11A_5_GHZ   != x->interface_type  &&
                     INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ != x->interface_type  &&
                     INTERFACE_TYPE_IEEE_802_11N_5_GHZ   != x->interface_type  &&
                     INTERFACE_TYPE_IEEE_802_11AC_5_GHZ  != x->interface_type  &&
                     INTERFACE_TYPE_IEEE_802_11AD_60_GHZ != x->interface_type  &&
                     INTERFACE_TYPE_IEEE_802_11AF_GHZ    != x->interface_type
                    )
                {
                    PLATFORM_PRINTF_DEBUG_ERROR("Interface %s is not a 802.11 interface and thus cannot act as a registrar!\n",x->name);

                    free_1905_INTERFACE_INFO(x);
                    return AL_ERROR_INTERFACE_ERROR;
                }
                else
                {
                    DMregistrarMacSet(x->mac_address);
                }
            }
        }

        free_1905_INTERFACE_INFO(x);
    }

    // Create a queue that will later be used by the platform code to notify us
    // when certain types of "events" take place
    //
    PLATFORM_PRINTF_DEBUG_DETAIL("Creating events queue...\n");
    queue_id = PLATFORM_CREATE_QUEUE("AL_events");
    if (0 == queue_id)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not create events queue\n");
        return AL_ERROR_OS;
    }

    // We are interested in processing 1905 packets that arrive on any of the
    // 1905 interfaces.
    // For this we are going to tell the platform code that we want to receive
    // a message on the just created queue every time a new 1905 packet arrives
    // on any of those interfaces.
    //
    PLATFORM_PRINTF_DEBUG_DETAIL("Registering packet arrival event for each interface...\n");
    for (i=0; i<interfaces_nr; i++)
    {
        struct event1905Packet aux;

                        aux.interface_name       = interfaces_names[i];
        memcpy(aux.interface_mac_address, DMinterfaceNameToMac(aux.interface_name), 6);
        memcpy(aux.al_mac_address,        DMalMacGet(),                             6);

        if (0 == PLATFORM_REGISTER_QUEUE_EVENT(queue_id, PLATFORM_QUEUE_EVENT_NEW_1905_PACKET, &aux))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("Could not register callback for 1905 packets in interface %s\n", interfaces_names[i]);
            return AL_ERROR_OS;
        }
        PLATFORM_PRINTF_DEBUG_DETAIL("    - %s --> OK\n", interfaces_names[i]);
    }
    free_LIST_OF_1905_INTERFACES(interfaces_names, interfaces_nr);

    // We are also interested in processing a 60 seconds timeout event (so that
    // we can send new discovery messages into the network)
    //
    PLATFORM_PRINTF_DEBUG_DETAIL("Registering DISCOVERY time out event (periodic)...\n");
    {
        struct eventTimeOut aux;

        aux.timeout_ms = 60000;  // 60 seconds
        aux.token      = TIMER_TOKEN_DISCOVERY;

        if (0 == PLATFORM_REGISTER_QUEUE_EVENT(queue_id, PLATFORM_QUEUE_EVENT_TIMEOUT_PERIODIC, &aux))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("Could not register timer callback\n");
            return AL_ERROR_OS;
        }
    }

    // ...and a slighlty higher timeout to "clean" the database from nodes that
    // have left the network without notice
    //
    PLATFORM_PRINTF_DEBUG_DETAIL("Registering GARBAGE COLLECTOR time out event (periodic)...\n");
    {
        struct eventTimeOut aux;

        aux.timeout_ms = 70000;  // 70 seconds
        aux.token      = TIMER_TOKEN_GARBAGE_COLLECTOR;

        if (0 == PLATFORM_REGISTER_QUEUE_EVENT(queue_id, PLATFORM_QUEUE_EVENT_TIMEOUT_PERIODIC, &aux))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("Could not register timer callback\n");
            return AL_ERROR_OS;
        }
    }

    // As soon as we enter the queue message processing loop we want to start
    // the discovery process as if a "DISCOVERY timeout" event had just
    // happened.
    // In other words, we want the first "DISCOVERY timeout" event to take place
    // at t=0 and then every 60 seconds.
    // In order to "force" this first event at t=0 we use a new timer event,
    // but this time this is a one time (ie. non-periodic) timer which will
    // time out in just one second from now.
    //
    PLATFORM_PRINTF_DEBUG_DETAIL("Registering a one time forced DISCOVERY event...\n");
    {
        struct eventTimeOut aux;

        aux.timeout_ms = 1;
        aux.token      = TIMER_TOKEN_DISCOVERY;

        if (0 == PLATFORM_REGISTER_QUEUE_EVENT(queue_id, PLATFORM_QUEUE_EVENT_TIMEOUT, &aux))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("Could not register timer callback\n");
            return AL_ERROR_OS;
        }
    }

    // Do also register the ALME interface (ie. we want ALME REQUEST messages to
    // be inserted into the queue so that we can process them)
    //
    PLATFORM_PRINTF_DEBUG_DETAIL("Registering the ALME interface...\n");
    if (0 == PLATFORM_REGISTER_QUEUE_EVENT(queue_id, PLATFORM_QUEUE_EVENT_NEW_ALME_MESSAGE, NULL))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not register ALME messages callback\n");
        return AL_ERROR_OS;
    }

    // ...and the "push button" event, so that when the platform detects that
    // the user has pressed the button associated to the "push button"
    // configuration mechanism, we are notified.
    //
    PLATFORM_PRINTF_DEBUG_DETAIL("Registering the PUSH BUTTON event...\n");
    if (0 == PLATFORM_REGISTER_QUEUE_EVENT(queue_id, PLATFORM_QUEUE_EVENT_PUSH_BUTTON, NULL))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not register 'push button' event\n");
        return AL_ERROR_OS;
    }

    // ...and the "new authenticated link" event, needed to produce the "push
    // button join notification" message.
    //
    PLATFORM_PRINTF_DEBUG_DETAIL("Registering the NEW AUTHENTICATED LINK event...\n");
    if (0 == PLATFORM_REGISTER_QUEUE_EVENT(queue_id, PLATFORM_QUEUE_EVENT_AUTHENTICATED_LINK, NULL))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not register 'authenticated link' event\n");
        return AL_ERROR_OS;
    }

    // ...and the "topology change notification" event, needed to inform the
    // other 1905 nodes that some aspect of our local topology has changed.
    //
    PLATFORM_PRINTF_DEBUG_DETAIL("Registering the TOPOLOGY CHANGE NOTIFICATION event...\n");
    if (0 == PLATFORM_REGISTER_QUEUE_EVENT(queue_id, PLATFORM_QUEUE_EVENT_TOPOLOGY_CHANGE_NOTIFICATION, NULL))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not register 'topology change' event\n");
        return AL_ERROR_OS;
    }

    // Any third-party software based on ieee1905 can extend the protocol
    // behaviour
    //
    if (0 == start1905ALExtensions())
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not register all 1905 protocol extensions\n");
        return AL_ERROR_PROTOCOL_EXTENSION;
    }

    // Prepare the message queue
    //
    PLATFORM_PRINTF_DEBUG_DETAIL("Allocating memory to hold a queue message...\n");
    queue_message = (uint8_t *)memalloc(MAX_NETWORK_SEGMENT_SIZE+3);

    PLATFORM_PRINTF_DEBUG_DETAIL("Entering read-process loop...\n");
    while(1)
    {
        uint8_t  *p;
        uint8_t   message_type;
        uint16_t  message_len;

        PLATFORM_PRINTF_DEBUG_DETAIL("\n");
        PLATFORM_PRINTF_DEBUG_DETAIL("Waiting for new queue message...\n");
        if (0 == PLATFORM_READ_QUEUE(queue_id, queue_message))
        {
            PLATFORM_PRINTF_DEBUG_WARNING("Something went wrong while trying to retrieve a new message from the queue. Ignoring...\n");
            continue;
        }

        // The first byte of 'queue_message' tells us the type of message that
        // we have just received
        //
        p = &queue_message[0];
        _E1B(&p, &message_type);
        _E2B(&p, &message_len);

        switch(message_type)
        {
            case PLATFORM_QUEUE_EVENT_NEW_1905_PACKET:
            {
                uint8_t *q;

                struct interfaceInfo *x;

                uint8_t  dst_addr[6];
                uint8_t  src_addr[6];
                uint16_t ether_type;

                uint8_t  receiving_interface_addr[6];
                char  *receiving_interface_name;

                // The first six bytes of the message payload contain the MAC
                // address of the interface where the packet was received
                //
                _EnB(&p, receiving_interface_addr, 6);

                receiving_interface_name = DMmacToInterfaceName(receiving_interface_addr);
                if (NULL == receiving_interface_name)
                {
                    PLATFORM_PRINTF_DEBUG_ERROR("A packet was receiving on MAC %02x:%02x:%02x:%02x:%02x:%02x, which does not match any local interface\n",receiving_interface_addr[0], receiving_interface_addr[1], receiving_interface_addr[2], receiving_interface_addr[3], receiving_interface_addr[4], receiving_interface_addr[5]);
                    continue;
                }

                x = PLATFORM_GET_1905_INTERFACE_INFO(receiving_interface_name);
                if (NULL == x)
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", receiving_interface_name);
                    continue;
                }
                if (0 == x->is_secured)
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("This interface (%s) is not secured. No packets should be received. Ignoring...\n", receiving_interface_name);
                    free_1905_INTERFACE_INFO(x);
                    continue;
                }
                free_1905_INTERFACE_INFO(x);

                q = p;

                // The next bytes are the actual packet payload (ie. the
                // ethernet payload)
                //
                _EnB(&q, dst_addr, 6);
                _EnB(&q, src_addr, 6);
                _E2B(&q, &ether_type);

                PLATFORM_PRINTF_DEBUG_DETAIL("New queue message arrived: packet captured on interface %s\n", receiving_interface_name);
                PLATFORM_PRINTF_DEBUG_DETAIL("    Dst address: %02x:%02x:%02x:%02x:%02x:%02x\n", dst_addr[0], dst_addr[1], dst_addr[2], dst_addr[3], dst_addr[4], dst_addr[5]);
                PLATFORM_PRINTF_DEBUG_DETAIL("    Src address: %02x:%02x:%02x:%02x:%02x:%02x\n", src_addr[0], src_addr[1], src_addr[2], src_addr[3], src_addr[4], src_addr[5]);
                PLATFORM_PRINTF_DEBUG_DETAIL("    Ether type : 0x%04x\n", ether_type);

                switch(ether_type)
                {
                    case ETHERTYPE_LLDP:
                    {
                        struct PAYLOAD *payload;

                        PLATFORM_PRINTF_DEBUG_DETAIL("LLDP message received.\n");

                        payload = parse_lldp_PAYLOAD_from_packet(q);

                        if (NULL == payload)
                        {
                            PLATFORM_PRINTF_DEBUG_WARNING("Invalid bridge discovery message. Ignoring...\n");
                        }
                        else
                        {
                            PLATFORM_PRINTF_DEBUG_DETAIL("LLDP message contents:\n");
                            visit_lldp_PAYLOAD_structure(payload, print_callback, PLATFORM_PRINTF_DEBUG_DETAIL, "");

                            processLlpdPayload(payload, receiving_interface_addr);

                            free_lldp_PAYLOAD_structure(payload);
                        }

                        break;
                    }

                    case ETHERTYPE_1905:
                    {
                        struct CMDU *c;

                        PLATFORM_PRINTF_DEBUG_DETAIL("CMDU message received. Reassembling...\n");

                        c = _reAssembleFragmentedCMDUs(p, message_len);

                        if (NULL == c)
                        {
                            // This was just a fragment part of a big CMDU.
                            // The data has been internally cached, waiting for
                            // the rest of pieces.
                        }
                        else
                        {
                            if (
                                 1 == _checkDuplicates(src_addr, c)
                               )
                            {
                               PLATFORM_PRINTF_DEBUG_WARNING("Receiving on %s a CMDU which is a duplicate of a previous one (mid = %d). Discarding...\n", receiving_interface_name, c->message_id);
                            }
                            else
                            {
                                uint8_t res;

                                PLATFORM_PRINTF_DEBUG_DETAIL("CMDU message contents:\n");
                                visit_1905_CMDU_structure(c, print_callback, PLATFORM_PRINTF_DEBUG_DETAIL, "");

                                // Process the message on the local node
                                //
                                res = process1905Cmdu(c, receiving_interface_addr, src_addr, queue_id);
                                if (PROCESS_CMDU_OK_TRIGGER_AP_SEARCH == res)
                                {
                                    _triggerAPSearchProcess();
                                }

                                // It might be necessary to retransmit this
                                // message on the rest of interfaces (depending
                                // on the "relayed multicast" flag
                                //
                                _checkForwarding(receiving_interface_addr, dst_addr, c);
                            }

                            free_1905_CMDU_structure(c);
                        }

                        break;
                    }

                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unknown ethertype 0x%04x!! Ignoring...\n", ether_type);
                        break;
                    }
                }

                break;
            }

            case PLATFORM_QUEUE_EVENT_NEW_ALME_MESSAGE:
            {
                // ALME messages contain:
                //
                //   1- one byte with the "client id" (which must be used when
                //      later calling "PLATFORM_SEND_ALME_REPLY()")
                //
                //   2- the bit stream representation of an ALME TLV.
                //
                // We just need to convert it into a struct and process it:
                //
                uint8_t   alme_client_id;
                uint8_t  *alme_tlv;

                _E1B(&p, &alme_client_id);

                PLATFORM_PRINTF_DEBUG_DETAIL("New queue message arrived: ALME message (client ID = %d).\n", alme_client_id);

                alme_tlv = parse_1905_ALME_from_packet(p);
                if (NULL == alme_tlv)
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("Invalid ALME message. Ignoring...\n");
                }

                PLATFORM_PRINTF_DEBUG_DETAIL("ALME message contents:\n");
                visit_1905_ALME_structure((uint8_t *)alme_tlv, print_callback, PLATFORM_PRINTF_DEBUG_DETAIL, "");

                process1905Alme(alme_tlv, alme_client_id);

                free_1905_ALME_structure(alme_tlv);

                break;
            }

            case PLATFORM_QUEUE_EVENT_TIMEOUT:
            case PLATFORM_QUEUE_EVENT_TIMEOUT_PERIODIC:
            {
                uint32_t  timer_id;

                // The message payload of this type of messages only contains
                // four bytes with the "timer ID" that expired.
                //
                _E4B(&p, &timer_id);

                PLATFORM_PRINTF_DEBUG_DETAIL("New queue message arrived: timer 0x%08x expired\n", timer_id);

                switch(timer_id)
                {
                    case TIMER_TOKEN_DISCOVERY:
                    {
                        uint16_t mid;

                        char **ifs_names;
                        uint8_t  ifs_nr;

                        // According to "Section 8.2.1.1" and "Section 8.2.1.2"
                        // we now have to send a "Topology discovery message"
                        // followed by a "802.1 bridge discovery message" but,
                        // according to the rules in "Section 7.2", only on each
                        // and every of the *authenticated* 1905 interfaces
                        // that are in the state of "PWR_ON" or "PWR_SAVE"
                        //
                        ifs_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&ifs_nr);
                        mid       = getNextMid();
                        for (i=0; i<ifs_nr; i++)
                        {
                            uint8_t authenticated;
                            uint8_t power_state;

                            struct interfaceInfo *x;

                            x = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
                            if (NULL == x)
                            {
                                PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);
                                authenticated = 0;
                                power_state   = INTERFACE_POWER_STATE_OFF;
                            }
                            else
                            {
                                authenticated = x->is_secured;
                                power_state   = x->power_state;

                                free_1905_INTERFACE_INFO(x);
                            }

                            if (
                                (0 == authenticated                                                                     ) ||
                                ((power_state != INTERFACE_POWER_STATE_ON) && (power_state!= INTERFACE_POWER_STATE_SAVE))
                               )
                            {
                                // Do not send the discovery messages on this
                                // interface
                                //
                                continue;
                            }

                            // Topology discovery message
                            //
                            if (0 == send1905TopologyDiscoveryPacket(ifs_names[i], mid))
                            {
                                PLATFORM_PRINTF_DEBUG_WARNING("Could not send 1905 topology discovery message\n");
                            }

                            // 802.1 bridge discovery message
                            //
                            if (0 == sendLLDPBridgeDiscoveryPacket(ifs_names[i]))
                            {
                                PLATFORM_PRINTF_DEBUG_WARNING("Could not send LLDP bridge discovery message\n");
                            }
                        }
                        free_LIST_OF_1905_INTERFACES(ifs_names, ifs_nr);

                        break;
                    }

                    case TIMER_TOKEN_GARBAGE_COLLECTOR:
                    {
                        PLATFORM_PRINTF_DEBUG_DETAIL("Running garbage collector...\n");

                        if (DMrunGarbageCollector() > 0)
                        {
                            uint16_t mid;

                            char **ifs_names;
                            uint8_t  ifs_nr;

                            PLATFORM_PRINTF_DEBUG_DETAIL("Some elements were removed. Sending a topology change notification...");

                            // According to "Section 8.2.2.3" and "Section
                            // 7.2", we now have to send a "Topology
                            // Notification message) on all *authenticated*
                            // interfaces that are in the state of "PWR_ON" or
                            // "PWR_SAVE"
                            //
                            ifs_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&ifs_nr);
                            mid       = getNextMid();
                            for (i=0; i<ifs_nr; i++)
                            {
                                uint8_t authenticated;
                                uint8_t power_state;

                                struct interfaceInfo *x;

                                x = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
                                if (NULL == x)
                                {
                                    PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);
                                    authenticated = 0;
                                    power_state   = INTERFACE_POWER_STATE_OFF;
                                }
                                else
                                {
                                    authenticated = x->is_secured;
                                    power_state   = x->power_state;

                                    free_1905_INTERFACE_INFO(x);
                                }

                                if (
                                    (0 == authenticated                                                                     ) ||
                                    ((power_state != INTERFACE_POWER_STATE_ON) && (power_state!= INTERFACE_POWER_STATE_SAVE))
                                   )
                                {
                                    // Do not send the topology notification  messages on
                                    // this interface
                                    //
                                    continue;
                                }

                                // Topology notification message
                                //
                                if (0 == send1905TopologyNotificationPacket(ifs_names[i], mid))
                                {
                                    PLATFORM_PRINTF_DEBUG_WARNING("Could not send 1905 topology discovery message\n");
                                }
                            }
                            free_LIST_OF_1905_INTERFACES(ifs_names, ifs_nr);
                        }
                        break;
                    }

                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unknown timer ID!! Ignoring...\n");
                        break;
                    }
                }

                break;
            }

            case PLATFORM_QUEUE_EVENT_PUSH_BUTTON:
            {
                uint16_t mid;

                char **ifs_names;
                uint8_t  ifs_nr;

                uint8_t  *no_push_button;
                uint8_t   at_least_one_unsupported_interface;

                PLATFORM_PRINTF_DEBUG_DETAIL("New queue message arrived: push button event\n");

                // According to "Section 9.2.2.1", we must first make sure that
                // none of the interfaces is in the middle of a previous "push
                // button" configuration sequence.
                //
                ifs_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&ifs_nr);
                mid       = getNextMid();

                for (i=0; i<ifs_nr; i++)
                {
                    struct interfaceInfo *x;

                    x = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
                    if (NULL == x)
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);
                        break;
                    }
                    else
                    {
                        if (1 == x->push_button_on_going)
                        {
                            PLATFORM_PRINTF_DEBUG_INFO("Interface %s is in the middle of a previous 'push button' configuration sequence. Ignoring new event...\n", ifs_names[i]);

                            free_1905_INTERFACE_INFO(x);
                            break;
                        }
                        free_1905_INTERFACE_INFO(x);
                    }

                }
                if (i < ifs_nr)
                {
                    // Don't do anything
                    //
                    break;
                }

                // If we get here, none of the interfaces is in the middle of a
                // "push button" configuration process, thus we can initialize
                // the "push button event" on all of our interfaces that support
                // it.
                //
                // Let's see which interfaces support it and keep track of
                // those who don't by setting the corresponding byte in array
                // "no_push_button" to '1'
                //
                no_push_button = (uint8_t *)memalloc(sizeof(uint8_t) * ifs_nr);

                for (i=0; i<ifs_nr; i++)
                {
                    struct interfaceInfo *x;

                    x = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
                    if (NULL == x)
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);

                        no_push_button[i] = 1;
                        break;
                    }
                    else
                    {
                        if (POWER_STATE_PWR_OFF == x->power_state)
                        {
                            // Ignore interfaces that are switched off
                            //
                            PLATFORM_PRINTF_DEBUG_DETAIL("Skipping interface %s because it is powered off\n", ifs_names[i]);
                            no_push_button[i] = 1;
                        }
                        else if (2 == x->push_button_on_going)
                        {
                            // This interface does not support the "push button"
                            // configuration process
                            //
                            PLATFORM_PRINTF_DEBUG_DETAIL("Skipping interface %s because it does not support the push button configuration mechanism\n", ifs_names[i]);
                            no_push_button[i] = 2;

                            // NOTE: "2" will be used as a special marker to
                            //       later trigger the AP search process (see
                            //       below)
                        }
                        else if (
                                  (INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ == x->interface_type ||
                                   INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ == x->interface_type ||
                                   INTERFACE_TYPE_IEEE_802_11A_5_GHZ   == x->interface_type ||
                                   INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ == x->interface_type ||
                                   INTERFACE_TYPE_IEEE_802_11N_5_GHZ   == x->interface_type ||
                                   INTERFACE_TYPE_IEEE_802_11AC_5_GHZ  == x->interface_type ||
                                   INTERFACE_TYPE_IEEE_802_11AD_60_GHZ == x->interface_type ||
                                   INTERFACE_TYPE_IEEE_802_11AF_GHZ    == x->interface_type)   &&
                                  IEEE80211_ROLE_AP != x->interface_type_data.ieee80211.role   &&
                                  (0x0 != x->interface_type_data.ieee80211.bssid[0] ||
                                   0x0 != x->interface_type_data.ieee80211.bssid[1] ||
                                   0x0 != x->interface_type_data.ieee80211.bssid[2] ||
                                   0x0 != x->interface_type_data.ieee80211.bssid[3] ||
                                   0x0 != x->interface_type_data.ieee80211.bssid[4] ||
                                   0x0 != x->interface_type_data.ieee80211.bssid[5]
                                  )
                           )
                        {
                            // According to "Section 9.2.2.1", an 802.11 STA
                            // which is already paired with an AP must *not*
                            // start the "push button" configuration process.
                            //
                            PLATFORM_PRINTF_DEBUG_DETAIL("Skipping interface %s because it is a wifi STA already associated to an AP\n", ifs_names[i]);
                            no_push_button[i] = 1;
                        }
                        else
                        {
                            no_push_button[i] = 0;
                        }

                        free_1905_INTERFACE_INFO(x);
                    }
                }

                // We now have the list of interfaces that need to start their
                // "push button" configuration process. Let's do it:
                //
                at_least_one_unsupported_interface = 0;
                for (i=0; i<ifs_nr; i++)
                {
                    if (0 == no_push_button[i])
                    {
                        PLATFORM_PRINTF_DEBUG_INFO("Starting push button configuration process on interface %s\n", ifs_names[i]);
                        PLATFORM_START_PUSH_BUTTON_CONFIGURATION(ifs_names[i], queue_id, DMalMacGet(), mid);
                    }
                    if (2 == no_push_button[i])
                    {
                        at_least_one_unsupported_interface = 1;
                    }
                }
                if (1 == at_least_one_unsupported_interface)
                {
                    // The reason for doing this is the next one:
                    //
                    // Imagine one device with two interfaces: an unconfigured
                    // AP wifi interface and an ethernet interface.
                    //
                    // If we press the button we *need*  to send the
                    // "AP search" CMDU... however, because the ethernet interface
                    // never starts the "push button configuration" process
                    // (because it is not supported!), the interface can never
                    // "become authenticated" and trigger the AP search
                    // process.
                    //
                    // That's why we do it here, manually
                    //
                    _triggerAPSearchProcess();
                }

                // Finally, send the notification message (so that the rest of
                // 1905 nodes is aware of this situation) but only on already
                // authenticated interfaces.
                //
                for (i=0; i<ifs_nr; i++)
                {
                    uint8_t authenticated;
                    uint8_t power_state;

                    struct interfaceInfo *x;

                    x = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
                    if (NULL == x)
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);
                        authenticated = 0;
                        power_state   = INTERFACE_POWER_STATE_OFF;
                    }
                    else
                    {
                        authenticated = x->is_secured;
                        power_state   = x->power_state;

                        free_1905_INTERFACE_INFO(x);
                    }

                    if (
                        (0 == authenticated                                                                     ) ||
                        ((power_state != INTERFACE_POWER_STATE_ON) && (power_state!= INTERFACE_POWER_STATE_SAVE))
                       )
                    {
                        // Do not send the push button event notification
                        // message on this interface interface
                        //
                        continue;
                    }

                    // Push button notification message
                    //
                    if (0 == send1905PushButtonEventNotificationPacket(ifs_names[i], mid, ifs_names, no_push_button, ifs_nr))
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Could not send 1905 push button event notification message\n");
                    }
                }

                free_LIST_OF_1905_INTERFACES(ifs_names, ifs_nr);

                break;
            }

            case PLATFORM_QUEUE_EVENT_AUTHENTICATED_LINK:
            {
                // Two different things need to be done when a new interface is
                // authenticated:
                //
                //   1. According to "Section 9.2.2.3", a "push button join
                //      notification" message must be generated and sent.
                //
                //   2. According to "Section 10.1", the "AP-autoconfiguration"
                //      process is triggered.

                uint16_t mid;

                uint8_t   local_mac_addr[6];
                uint8_t   new_mac_addr[6];
                uint8_t   original_al_mac_addr[6];
                uint16_t  original_mid;

                char **ifs_names;
                uint8_t  ifs_nr;

                // The first six bytes of the message payload contain the MAC
                // address of the interface where the "push button"
                // configuration process succeeded.
                //
                _EnB(&p, local_mac_addr, 6);

                // The next six bytes contain the MAC address of the interface
                // successfully authenticated at the other end.
                //
                _EnB(&p, new_mac_addr, 6);

                // The next six bytes contain the original AL MAC address that
                // started everything.
                //
                _EnB(&p, original_al_mac_addr, 6);

                // Finally, the last two bytes contains the MID of that original
                // message
                //
                _E2B(&p, &original_mid);

                PLATFORM_PRINTF_DEBUG_DETAIL("New queue message arrived: authenticated link\n");
                PLATFORM_PRINTF_DEBUG_DETAIL("    Local interface:        %02x:%02x:%02x:%02x:%02x:%02x\n", local_mac_addr[0], local_mac_addr[1], local_mac_addr[2], local_mac_addr[3], local_mac_addr[4], local_mac_addr[5]);
                PLATFORM_PRINTF_DEBUG_DETAIL("    New (remote) interface: %02x:%02x:%02x:%02x:%02x:%02x\n", new_mac_addr[0], new_mac_addr[1], new_mac_addr[2], new_mac_addr[3], new_mac_addr[4], new_mac_addr[5]);
                PLATFORM_PRINTF_DEBUG_DETAIL("    Original AL MAC       : %02x:%02x:%02x:%02x:%02x:%02x\n", original_al_mac_addr[0], original_al_mac_addr[1], original_al_mac_addr[2], original_al_mac_addr[3], original_al_mac_addr[4], original_al_mac_addr[5]);
                PLATFORM_PRINTF_DEBUG_DETAIL("    Original MID          : %d\n", original_mid);

                ifs_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&ifs_nr);

                // If "new_mac_addr" is NULL, this means the interface was
                // "authenticated" as a whole (not at "link level").
                // This happens for ethernet interfaces.
                // In these cases we must *not* send the "push button join
                // notification" message (note, however, that the "AP-
                // autoconfiguration" process does need to be triggered, which
                // is done later)
                //
                if (
                     new_mac_addr[0] == 0x00 &&
                     new_mac_addr[1] == 0x00 &&
                     new_mac_addr[2] == 0x00 &&
                     new_mac_addr[3] == 0x00 &&
                     new_mac_addr[4] == 0x00 &&
                     new_mac_addr[5] == 0x00
                   )
                {
                    PLATFORM_PRINTF_DEBUG_DETAIL("NULL new (remote) interface. No 'push button join notification' will be sent.\n");
                }
                else
                {
                    // Send the "push button join notification" message on all
                    // authenticated interfaces (except for the one just
                    // authenticated)
                    //
                    mid       = getNextMid();
                    for (i=0; i<ifs_nr; i++)
                    {
                        uint8_t authenticated;
                        uint8_t power_state;

                        struct interfaceInfo *x;

                        x = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
                        if (NULL == x)
                        {
                            PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);
                            authenticated = 0;
                            power_state   = INTERFACE_POWER_STATE_OFF;
                        }
                        else
                        {
                            authenticated = x->is_secured;
                            power_state   = x->power_state;
                        }


                        if (
                            (0 == authenticated                                                                     ) ||
                            ((power_state != INTERFACE_POWER_STATE_ON) && (power_state!= INTERFACE_POWER_STATE_SAVE)) ||
                            (0 == memcmp(x->mac_address, local_mac_addr, 6))
                           )
                        {
                            // Do not send the message on this interface
                            //
                            if (NULL != x)
                            {
                                free_1905_INTERFACE_INFO(x);
                            }
                            continue;
                        }

                        if (NULL != x)
                        {
                            free_1905_INTERFACE_INFO(x);
                        }

                        if (0 == send1905PushButtonJoinNotificationPacket(ifs_names[i], mid, original_al_mac_addr, original_mid, local_mac_addr, new_mac_addr))
                        {
                            PLATFORM_PRINTF_DEBUG_WARNING("Could not send 1905 topology discovery message\n");
                        }
                    }
                }

                free_LIST_OF_1905_INTERFACES(ifs_names, ifs_nr);

                // Finally, trigger the "AP-autoconfiguration" process
                //
                _triggerAPSearchProcess();

                break;
            }

            case PLATFORM_QUEUE_EVENT_TOPOLOGY_CHANGE_NOTIFICATION:
            {
                uint16_t mid;

                char **ifs_names;
                uint8_t  ifs_nr;

                PLATFORM_PRINTF_DEBUG_DETAIL("New queue message arrived: topology change notification event\n");

                // TODO:
                //   1. Find which L2 neighbors are no longer available
                //   2. Set their timestamp to 0
                //   3. Call DMrunGarbageCollector() to remove them from the
                //      database
                //
                // Until this is done, nodes will only be removed from the
                // database when the "TIMER_TOKEN_GARBAGE_COLLECTOR" timer
                // expires.

                // According to "Section 8.2.2.3" and "Section 7.2", we now
                // have to send a "Topology Notification" message) on all
                // *authenticated* interfaces that are in the state of "PWR_ON"
                // or "PWR_SAVE"
                //
                ifs_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&ifs_nr);
                mid       = getNextMid();
                for (i=0; i<ifs_nr; i++)
                {
                    uint8_t authenticated;
                    uint8_t power_state;

                    struct interfaceInfo *x;

                    x = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
                    if (NULL == x)
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);
                        authenticated = 0;
                        power_state   = INTERFACE_POWER_STATE_OFF;
                    }
                    else
                    {
                        authenticated = x->is_secured;
                        power_state   = x->power_state;

                        free_1905_INTERFACE_INFO(x);
                    }

                    if (
                        (0 == authenticated                                                                     ) ||
                        ((power_state != INTERFACE_POWER_STATE_ON) && (power_state!= INTERFACE_POWER_STATE_SAVE))
                       )
                    {
                        // Do not send the topology notification  messages on
                        // this interface
                        //
                        continue;
                    }

                    // Topology notification message
                    //
                    if (0 == send1905TopologyNotificationPacket(ifs_names[i], mid))
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Could not send 1905 topology discovery message\n");
                    }
                }
                free_LIST_OF_1905_INTERFACES(ifs_names, ifs_nr);

                break;
            }

            default:
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Unknown queue message type (%d)\n", message_type);

                break;
            }
        }
    }

    return 0;
}



