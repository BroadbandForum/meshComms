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

#include "platform.h"

#include "al_recv.h"
#include "al_datamodel.h"
#include "al_utils.h"
#include "al_send.h"
#include "al_wsc.h"
#include "al_extension.h"

#include "1905_tlvs.h"
#include "1905_cmdus.h"
#include "1905_alme.h"
#include "1905_l2.h"
#include "lldp_tlvs.h"
#include "lldp_payload.h"

#include "platform_interfaces.h"
#include "platform_alme_server.h"

#include <string.h> // memcmp(), memcpy(), ...

////////////////////////////////////////////////////////////////////////////////
// Public functions (exported only to files in this same folder)
////////////////////////////////////////////////////////////////////////////////

uint8_t process1905Cmdu(struct CMDU *c, uint8_t *receiving_interface_addr, uint8_t *src_addr, uint8_t queue_id)
{
    if (NULL == c)
    {
        return PROCESS_CMDU_KO;
    }

    // Third party implementations maybe need to process some protocol
    // extensions
    //
    process1905CmduExtensions(c);

    switch (c->message_type)
    {
        case CMDU_TYPE_TOPOLOGY_DISCOVERY:
        {
            // When a "topology discovery" is received we must update our
            // internal database (that keeps track of which AL MACs and
            // interface MACs are seen on each interface) and send a "topology
            // query" message asking for more details.

            struct tlv *p;
            uint8_t  i;

            uint8_t  dummy_mac_address[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            uint8_t  al_mac_address[6];
            uint8_t  mac_address[6];

            uint8_t  first_discovery;
            uint32_t ellapsed;

            memcpy(al_mac_address, dummy_mac_address, 6);
            memcpy(mac_address,    dummy_mac_address, 6);

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_TOPOLOGY_DISCOVERY (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            // We need to update the data model structure, which keeps track
            // of local interfaces, neighbors, and neighbors' interfaces, and
            // what type of discovery messages ("topology discovery" and/or
            // "bridge discovery") have been received on each link.

            if (NULL == c->list_of_TLVs)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
                break;
            }

            // First, extract the AL MAC and MAC addresses of the interface
            // which transmitted this "topology discovery" message
            //
            i = 0;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_AL_MAC_ADDRESS_TYPE:
                    {
                        struct alMacAddressTypeTLV *t = (struct alMacAddressTypeTLV *)p;

                        memcpy(al_mac_address, t->al_mac_address, 6);

                        break;
                    }
                    case TLV_TYPE_MAC_ADDRESS_TYPE:
                    {
                        struct macAddressTypeTLV *t = (struct macAddressTypeTLV *)p;

                        memcpy(mac_address, t->mac_address, 6);

                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unexpected TLV (%d) type inside CMDU\n", p->type);
                        break;
                    }
                }
                i++;
            }

            // Make sure that both the AL MAC and MAC addresses were contained
            // in the CMDU
            //
            if (0 == memcmp(al_mac_address, dummy_mac_address, 6) ||
                0 == memcmp(mac_address,    dummy_mac_address, 6))
            {
                PLATFORM_PRINTF_DEBUG_WARNING("More TLVs were expected inside this CMDU\n");
                return PROCESS_CMDU_KO;
            }

            PLATFORM_PRINTF_DEBUG_DETAIL("AL MAC address = %02x:%02x:%02x:%02x:%02x:%02x\n", al_mac_address[0], al_mac_address[1], al_mac_address[2], al_mac_address[3], al_mac_address[4], al_mac_address[5]);
            PLATFORM_PRINTF_DEBUG_DETAIL("MAC    address = %02x:%02x:%02x:%02x:%02x:%02x\n", mac_address[0],    mac_address[1],    mac_address[2],    mac_address[3],    mac_address[4],    mac_address[5]);

            // Next, update the data model
            //
            if (1 == (first_discovery = DMupdateDiscoveryTimeStamps(receiving_interface_addr, al_mac_address, mac_address, TIMESTAMP_TOPOLOGY_DISCOVERY, &ellapsed)))
            {
#ifdef SPEED_UP_DISCOVERY
                // If the data model did not contain an entry for this neighbor,
                // "manually" (ie. "out of cycle") send a "Topology Discovery"
                // message on the receiving interface.
                // This will speed up the network discovery process, so that
                // the new node does not have to wait until our "60 seconds"
                // timer expires for him to "discover" us
                //
                PLATFORM_PRINTF_DEBUG_DETAIL("Is this a new node? Re-scheduling a Topology Discovery so that he 'discovers' us\n");

                if (0 == send1905TopologyDiscoveryPacket(DMmacToInterfaceName(receiving_interface_addr), getNextMid()))
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("Could not send 1905 topology discovery message\n");
                }
#endif
            }

            // Finally, query the advertising neighbor for (much) more detailed
            // information (but only if we haven't recently queried it!)
            // This will make the other end send us a
            // CMDU_TYPE_TOPOLOGY_RESPONSE message, which we will later
            // process.
            //
            if (
                 0 == DMnetworkDeviceInfoNeedsUpdate(al_mac_address) ||  // Recently received a Topology Response or....
                 (2 == first_discovery && ellapsed < 5000)               // ...recently (<5 seconds) received a Topology Discovery

               )
            {
                // The first condition prevents us from re-asking (ie.
                // re-sending "Topology Queries") to one same node (we already
                // knew of) faster than once every minute.
                //
                // The second condition prevents us from flooding new nodes
                // (from which we haven't received a "Topology Response" yet)
                // with "Topology Queries" faster than once every 5 seconds)
                //
                break;
            }

            if ( 0 == send1905TopologyQueryPacket(DMmacToInterfaceName(receiving_interface_addr), getNextMid(), al_mac_address))
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not send 'topology query' message\n");
            }

            break;
        }
        case CMDU_TYPE_TOPOLOGY_NOTIFICATION:
        {
            // When a "topology notification" is received we must send a new
            // "topology query" to the sender.
            // The "sender" AL MAC address is contained in the unique TLV
            // embedded in the just received "topology notification" CMDU.

            struct tlv *p;
            uint8_t  i;

            uint8_t dummy_mac_address[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            uint8_t  al_mac_address[6];


            memcpy(al_mac_address, dummy_mac_address, 6);

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_TOPOLOGY_NOTIFICATION (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            if (NULL == c->list_of_TLVs)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
                break;
            }

            // Extract the AL MAC addresses of the interface which transmitted
            // this "topology notification" message
            //
            i = 0;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_AL_MAC_ADDRESS_TYPE:
                    {
                        struct alMacAddressTypeTLV *t = (struct alMacAddressTypeTLV *)p;

                        memcpy(al_mac_address, t->al_mac_address, 6);

                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unexpected TLV (%d) type inside CMDU\n", p->type);
                        break;
                    }
                }
                i++;
            }

            // Make sure that both the AL MAC and MAC addresses were contained
            // in the CMDU
            //
            if (0 == memcmp(al_mac_address, dummy_mac_address, 6))
            {
                PLATFORM_PRINTF_DEBUG_WARNING("More TLVs were expected inside this CMDU\n");
                return PROCESS_CMDU_KO;
            }

            PLATFORM_PRINTF_DEBUG_DETAIL("AL MAC address = %02x:%02x:%02x:%02x:%02x:%02x\n", al_mac_address[0], al_mac_address[1], al_mac_address[2], al_mac_address[3], al_mac_address[4], al_mac_address[5]);

#ifdef SPEED_UP_DISCOVERY
            // We will send a topology discovery back. Why is this useful?
            // Well... imagine a node that has just entered the secure network.
            // The first thing this node will do is sending a
            // "topology notification" which, when received by us, will trigger
            // a "topology query".
            // However, unless we send a "topology discovery" back, the new node
            // will not query us for a while (until we actually send our
            // periodic "topology discovery").
            //
             PLATFORM_PRINTF_DEBUG_DETAIL("Is this a new node? Re-scheduling a Topology Discovery so that he 'discovers' us\n");

             if (0 == send1905TopologyDiscoveryPacket(DMmacToInterfaceName(receiving_interface_addr), getNextMid()))
             {
                 PLATFORM_PRINTF_DEBUG_WARNING("Could not send 1905 topology discovery message\n");
             }
#endif
            // Finally, query the informing node.
            // Note that we don't have to check (as we did in the "topology
            // discovery" case) if we recently updated the data model or not.
            // This is because a "topology notification" *always* implies
            // network changes and thus the device must always be (re)-queried.
            //
            if ( 0 == send1905TopologyQueryPacket(DMmacToInterfaceName(receiving_interface_addr), getNextMid(), al_mac_address))
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not send 'topology query' message\n");
            }

            break;
        }
        case CMDU_TYPE_TOPOLOGY_QUERY:
        {
            // When a "topology query" is received we must obtain a series of
            // information from the platform and then package and send it back
            // in a "topology response" message.

            uint8_t *dst_mac;
            uint8_t *p;

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_TOPOLOGY_QUERY (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            // We must send the response to the AL MAC of the node who sent the
            // query, however, this AL MAC is *not* contained in the query.
            // The only thing we can do at this point is try to search our AL
            // neighbors data base for a matching MAC.
            //
            p = DMmacToAlMac(src_addr);

            if (NULL == p)
            {
                // The standard says we should always send to the AL MAC
                // address, however, in these cases, instead of just dropping
                // the packet, sending the response to the 'src' address from
                // the TOPOLOGY QUERY seems the right thing to do.
                //
                dst_mac = src_addr;
                PLATFORM_PRINTF_DEBUG_WARNING("Unknown destination AL MAC. Using the 'src' MAC from the TOPOLOGY QUERY (%02x:%02x:%02x:%02x:%02x:%02x)\n", dst_mac[0],dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5]);
            }
            else
            {
                dst_mac = p;
            }

            if ( 0 == send1905TopologyResponsePacket(DMmacToInterfaceName(receiving_interface_addr), c->message_id, dst_mac))
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not send 'topology query' message\n");
            }

            if (NULL != p)
            {
                free(p);
            }

            break;
        }
        case CMDU_TYPE_TOPOLOGY_RESPONSE:
        {
            // When a "topology response" is received we must update our
            // internal database (that keeps track of which 1905 devices are
            // present in the network)

            struct tlv *p;
            uint8_t  i;

            struct deviceInformationTypeTLV      *info = NULL;
            struct deviceBridgingCapabilityTLV  **x    = NULL;
            struct non1905NeighborDeviceListTLV **y    = NULL;
            struct neighborDeviceListTLV        **z    = NULL;
            struct powerOffInterfaceTLV         **q    = NULL;
            struct l2NeighborDeviceTLV          **r    = NULL;
            struct supportedServiceTLV           *s    = NULL;

            uint8_t bridges_nr;
            uint8_t non1905_neighbors_nr;
            uint8_t x1905_neighbors_nr;
            uint8_t power_off_nr;
            uint8_t l2_neighbors_nr;

            uint8_t xi, yi, zi, qi, ri;

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_TOPOLOGY_RESPONSE (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            if (NULL == c->list_of_TLVs)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
                break;
            }

            // First, extract the device info TLV and "count" how many bridging
            // capability TLVs, non-1905 neighbors TLVs and 1905 neighbors TLVs
            // there are
            //
            bridges_nr           = 0;
            non1905_neighbors_nr = 0;
            x1905_neighbors_nr   = 0;
            power_off_nr         = 0;
            l2_neighbors_nr      = 0;
            i                    = 0;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_DEVICE_INFORMATION_TYPE:
                    {
                        info = (struct deviceInformationTypeTLV *)p;
                        break;
                    }
                    case TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES:
                    {
                        bridges_nr++;
                        break;
                    }
                    case TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST:
                    {
                        non1905_neighbors_nr++;
                        break;
                    }
                    case TLV_TYPE_NEIGHBOR_DEVICE_LIST:
                    {
                        x1905_neighbors_nr++;
                        break;
                    }
                    case TLV_TYPE_POWER_OFF_INTERFACE:
                    {
                        power_off_nr++;
                        break;
                    }
                    case TLV_TYPE_L2_NEIGHBOR_DEVICE:
                    {
                        l2_neighbors_nr++;
                        break;
                    }
                    case TLV_TYPE_SUPPORTED_SERVICE:
                    {
                        s = (struct supportedServiceTLV *)p;
                        break;
                    }
                    case TLV_TYPE_VENDOR_SPECIFIC:
                    {
                        // According to the standard, zero or more Vendor
                        // Specific TLVs may be present.
                        //
                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unexpected TLV (%d) type inside CMDU\n", p->type);
                        break;
                    }
                }
                i++;
            }

            // Next, now that we know how many TLVs of each type there are,
            // create an array of pointers big enough to contain them and fill
            // it.
            //
            if (bridges_nr > 0)
            {
                x = (struct deviceBridgingCapabilityTLV  **)memalloc(sizeof(struct deviceBridgingCapabilityTLV *)  * bridges_nr);
            }
            if (non1905_neighbors_nr > 0)
            {
                y = (struct non1905NeighborDeviceListTLV **)memalloc(sizeof(struct non1905NeighborDeviceListTLV *) * non1905_neighbors_nr);
            }
            if (x1905_neighbors_nr > 0)
            {
                z = (struct neighborDeviceListTLV        **)memalloc(sizeof(struct neighborDeviceListTLV        *) * x1905_neighbors_nr);
            }
            if (power_off_nr > 0)
            {
                q = (struct powerOffInterfaceTLV         **)memalloc(sizeof(struct powerOffInterfaceTLV         *) * power_off_nr);
            }
            if (l2_neighbors_nr > 0)
            {
                r = (struct l2NeighborDeviceTLV          **)memalloc(sizeof(struct l2NeighborDeviceTLV          *) * l2_neighbors_nr);
            }

            xi = 0;
            yi = 0;
            zi = 0;
            qi = 0;
            ri = 0;
            i  = 0;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_DEVICE_INFORMATION_TYPE:
                    {
                        break;
                    }
                    case TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES:
                    {
                        x[xi++] = (struct deviceBridgingCapabilityTLV *)p;
                        break;
                    }
                    case TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST:
                    {
                        y[yi++] = (struct non1905NeighborDeviceListTLV *)p;
                        break;
                    }
                    case TLV_TYPE_NEIGHBOR_DEVICE_LIST:
                    {
                        z[zi++] = (struct neighborDeviceListTLV *)p;
                        break;
                    }
                    case TLV_TYPE_POWER_OFF_INTERFACE:
                    {
                        q[qi++] = (struct powerOffInterfaceTLV *)p;
                        break;
                    }
                    case TLV_TYPE_L2_NEIGHBOR_DEVICE:
                    {
                        r[ri++] = (struct l2NeighborDeviceTLV *)p;
                        break;
                    }
                    case TLV_TYPE_SUPPORTED_SERVICE:
                    {
                        break;
                    }
                    case TLV_TYPE_VENDOR_SPECIFIC:
                    {
                        // According to the standard, zero or more Vendor
                        // Specific TLVs may be present.
                        //
                        free_1905_TLV_structure(p);
                        break;
                    }
                    default:
                    {
                        // We are not interested in other TLVs. Free them
                        //
                        free_1905_TLV_structure(p);
                        break;
                    }
                }
                i++;
            }

            // The CMDU structure is not needed anymore, but we cannot just let
            // the caller call "free_1905_CMDU_structure()", because it would
            // also free the TLVs references that we need (ie. those saved in
            // the "x", "y", "z", "q" and "r" pointer arrays).
            // The "fix" is easy: just set "c->list_of_TLVs" to NULL so that
            // when the caller calls "free_1905_CMDU_structure()", this function
            // ignores (ie. does not free) the "list_of_TLVs" list.
            //
            // This will work because and memory won't be lost because:
            //
            //   1. Those TLVs contained in "c->list_of_TLVs" that we are not
            //      keeping track off have already been freed (see the "default"
            //      case in the previous "switch" structure).
            //
            //   2. The rest of them will be freed when the data model entry
            //      is replaced/deleted.
            //
            //   3. Setting "C->list_of_TLVs" to NULL will cause
            //      "free_1905_CMDU_structure()" to ignore this list.
            //
            free(c->list_of_TLVs);
            c->list_of_TLVs = NULL;

            // Next, update the database. This will take care of duplicate
            // entries (and free TLVs if needed)
            //
            PLATFORM_PRINTF_DEBUG_DETAIL("Updating network devices database...\n");
            DMupdateNetworkDeviceInfo(info->al_mac_address,
                                      1, info,
                                      1, x, bridges_nr,
                                      1, y, non1905_neighbors_nr,
                                      1, z, x1905_neighbors_nr,
                                      1, q, power_off_nr,
                                      1, r, l2_neighbors_nr,
                                      1, s,
                                      0, NULL,
                                      0, NULL,
                                      0, NULL,
                                      0, NULL,
                                      0, NULL,
                                      0, NULL);

            // Show all network devices (ie. print them through the logging
            // system)
            //
            DMdumpNetworkDevices(PLATFORM_PRINTF_DEBUG_DETAIL);

            // And finally, send other queries to the device so that we can
            // keep updating the database once the responses are received
            //
            if ( 0 == send1905MetricsQueryPacket(DMmacToInterfaceName(receiving_interface_addr), getNextMid(), info->al_mac_address))
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not send 'metrics query' message\n");
            }
            if ( 0 == send1905HighLayerQueryPacket(DMmacToInterfaceName(receiving_interface_addr), getNextMid(), info->al_mac_address))
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not send 'high layer query' message\n");
            }
            for (i=0; i<info->local_interfaces_nr; i++)
            {
                if (MEDIA_TYPE_UNKNOWN == info->local_interfaces[i].media_type)
                {
                    // There is *at least* one generic inteface in the response,
                    // thus query for more information
                    //
                    if ( 0 == send1905GenericPhyQueryPacket(DMmacToInterfaceName(receiving_interface_addr), getNextMid(), info->al_mac_address))
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Could not send 'generic phy query' message\n");
                    }
                    break;
                }
            }

            // There is one extra thing that needs to be done: send topology
            // query to neighbor's neighbors.
            //
            // This is not strictly necessary for 1905 to work. In fact, as I
            // think the protocol was designed, every node should only be aware
            // of its *direct* neighbors; and it is the HLE responsability to
            // query each node and build the network topology map.
            //
            // However, the 1905 datamodel standard document, interestingly
            // (and, I think, erroneously) includes information from all the
            // nodes (even those that are not direct neighbors).
            //
            // Here we are going to retrieve that information but, because this
            // requires much more memory in the AL node, we will only do this
            // if the user actually expressed his desire to do so when starting
            // the AL entity.
            //
            if (1 == DMmapWholeNetworkGet())
            {
                // For each neighbor interface
                //
                for (i=0; i<zi; i++)
                {
                    uint8_t j;

                    // For each neighbor's neighbor on that interface
                    //
                    for (j=0; j<z[i]->neighbors_nr; j++)
                    {
                        uint8_t ii, jj;

                        // Discard the current node (obviously)
                        //
                        if (0 == memcmp(DMalMacGet(), z[i]->neighbors[j].mac_address, 6))
                        {
                            continue;
                        }

                        // Discard nodes I have just asked for
                        //
                        for (ii=0; ii<i; ii++)
                        {
                            for (jj=0; jj<z[ii]->neighbors_nr; jj++)
                            {
                                if (0 == memcmp(z[ii]->neighbors[jj].mac_address, z[i]->neighbors[j].mac_address, 6))
                                {
                                    continue;
                                }
                            }
                        }

                        // Discard neighbors whose information was updated
                        // recently (ie. no need to flood the network)
                        //
                        if (0 == DMnetworkDeviceInfoNeedsUpdate(z[i]->neighbors[j].mac_address))
                        {
                            continue;
                        }

                        if ( 0 == send1905TopologyQueryPacket(DMmacToInterfaceName(receiving_interface_addr), getNextMid(), z[i]->neighbors[j].mac_address))
                        {
                            PLATFORM_PRINTF_DEBUG_WARNING("Could not send 'topology query' message\n");
                        }
                    }
                }
            }

            break;
        }
        case CMDU_TYPE_VENDOR_SPECIFIC:
        {
            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_VENDOR_SPECIFIC (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            // TODO: Implement vendor specific hooks. Maybe, for now, we should
            // simply call a new "PLATFORM_VENDOR_SPECIFIC_CALLBACK()" function

            break;
        }
        case CMDU_TYPE_LINK_METRIC_QUERY:
        {
            struct tlv *p;
            uint8_t  i;

            uint8_t *dst_mac;
            uint8_t *al_mac;

            struct linkMetricQueryTLV *t;

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_LINK_METRIC_QUERY (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            if (NULL == c->list_of_TLVs)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
                break;
            }

            // First, search for the "struct linkMetricQueryTLV"
            //
            i = 0;
            t = NULL;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_LINK_METRIC_QUERY:
                    {
                        t = (struct linkMetricQueryTLV *)p;
                        break;
                    }
                    case TLV_TYPE_VENDOR_SPECIFIC:
                    {
                        // According to the standard, zero or more Vendor
                        // Specific TLVs may be present.
                        //
                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unexpected TLV (%d) type inside CMDU\n", p->type);
                        break;
                    }
                }
                i++;
            }

            if (NULL == t)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("More TLVs were expected inside this CMDU\n");
                return PROCESS_CMDU_KO;
            }

            if      (LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS == t->destination)
            {
                PLATFORM_PRINTF_DEBUG_DETAIL("Destination = all neighbors\n");
            }
            else if (LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR == t->destination)
            {
                PLATFORM_PRINTF_DEBUG_DETAIL("Destination = specific neighbor (%02x:%02x:%02x:%02x:%02x:%02x)\n", t->specific_neighbor[0], t->specific_neighbor[1], t->specific_neighbor[2], t->specific_neighbor[3], t->specific_neighbor[4], t->specific_neighbor[5]);
            }
            else
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Unexpected 'destination' (%d)\n", t->destination);
                return PROCESS_CMDU_KO;
            }

            if      (LINK_METRIC_QUERY_TLV_TX_LINK_METRICS_ONLY == t->link_metrics_type)
            {
                PLATFORM_PRINTF_DEBUG_DETAIL("Type        = Tx metrics only\n");
            }
            else if (LINK_METRIC_QUERY_TLV_RX_LINK_METRICS_ONLY == t->link_metrics_type)
            {
                PLATFORM_PRINTF_DEBUG_DETAIL("Type        = Rx metrics only\n");
            }
            if (LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS == t->link_metrics_type)
            {
                PLATFORM_PRINTF_DEBUG_DETAIL("Type        = Tx and Rx metrics\n");
            }
            else
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Unexpected 'type' (%d)\n", t->link_metrics_type);
                return PROCESS_CMDU_KO;
            }

            // And finally, send a "metrics response" to the requesting neighbor

            // We must send the response to the AL MAC of the node who sent the
            // query, however, this AL MAC is *not* contained in the query.
            // The only thing we can do at this point is try to search our AL
            // neighbors data base for a matching MAC.
            //
            al_mac = DMmacToAlMac(src_addr);

            if (NULL == al_mac)
            {
                // The standard says we should always send to the AL MAC
                // address, however, in these cases, instead of just dropping
                // the packet, sending the response to the 'src' address from
                // the METRICS QUERY seems the right thing to do.
                //
                dst_mac = src_addr;
                PLATFORM_PRINTF_DEBUG_WARNING("Unknown destination AL MAC. Using the 'src' MAC from the METRICS QUERY (%02x:%02x:%02x:%02x:%02x:%02x)\n", dst_mac[0],dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5]);
            }
            else
            {
                dst_mac = al_mac;
            }

            if ( 0 == send1905MetricsResponsePacket(DMmacToInterfaceName(receiving_interface_addr), c->message_id, dst_mac, t->destination, t->specific_neighbor, t->link_metrics_type))
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not send 'metrics response' message\n");
            }

            if (NULL != al_mac)
            {
                free(al_mac);
            }

            break;
        }
        case CMDU_TYPE_LINK_METRIC_RESPONSE:
        {
            // When a "metrics response" is received we must update our
            // internal database (that keeps track of which 1905 devices are
            // present in the network)

            struct tlv *p;
            uint8_t  i;

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_LINK_METRIC_RESPONSE (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            if (NULL == c->list_of_TLVs)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
                break;
            }

            // Call "DMupdateNetworkDeviceMetrics()" for each TLV
            //
            PLATFORM_PRINTF_DEBUG_DETAIL("Updating network devices database...\n");

            i = 0;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_TRANSMITTER_LINK_METRIC:
                    case TLV_TYPE_RECEIVER_LINK_METRIC:
                    {
                        DMupdateNetworkDeviceMetrics((uint8_t*)p);
                        break;
                    }
                    case TLV_TYPE_VENDOR_SPECIFIC:
                    {
                        // According to the standard, zero or more Vendor
                        // Specific TLVs may be present.
                        //
                        free_1905_TLV_structure(p);
                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unexpected TLV (%d) type inside CMDU\n", p->type);

                        free_1905_TLV_structure(p);
                        break;
                    }
                }
                i++;
            }

            // References to the TLVs cannot be freed by the caller (see the
            // comment in "case CMDU_TYPE_TOPOLOGY_RESPONSE:" to understand the
            // following two lines).
            //
            free(c->list_of_TLVs);
            c->list_of_TLVs = NULL;

            // Show all network devices (ie. print them through the logging
            // system)
            //
            DMdumpNetworkDevices(PLATFORM_PRINTF_DEBUG_DETAIL);

            break;
        }
        case CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH:
        {
            // When a "AP-autoconfig search" is received then, *only* if one
            // of our interfaces is the network AP registrar, an "AP-autoconfig
            // response" message must be sent.
            // Otherwise, the message is ignored.

            struct tlv *p;
            uint8_t i;

            uint8_t dummy_mac_address[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            char **ifs_names;
            uint8_t  ifs_nr;

            uint8_t searched_role_is_present;
            uint8_t searched_role;

            uint8_t freq_band_is_present;
            uint8_t freq_band;

            bool supported_service_is_present = false;
            bool searched_service_is_present = false;

            uint8_t  al_mac_address[6];

            searched_role_is_present = 0;
            freq_band_is_present     = 0;
            memcpy(al_mac_address, dummy_mac_address, 6);

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            if (NULL == c->list_of_TLVs)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
                break;
            }

            // First, parse the incomming packet to find out three things:
            // - The AL MAC of the node searching for AP-autoconfiguration
            //   parameters.
            // - The "searched role" contained in the "searched role TLV" (must
            //   be "REGISTRAR")
            // - The "freq band" contained in the "autoconfig freq band TLV"
            //   (must match the one of our local registrar interface)
            //
            i = 0;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_AL_MAC_ADDRESS_TYPE:
                    {
                        struct alMacAddressTypeTLV *t = (struct alMacAddressTypeTLV *)p;

                        memcpy(al_mac_address, t->al_mac_address, 6);

                        break;
                    }
                    case TLV_TYPE_SEARCHED_ROLE:
                    {
                        struct searchedRoleTLV *t = (struct searchedRoleTLV *)p;

                        searched_role_is_present = 1;
                        searched_role            = t->role;

                        break;
                    }
                    case TLV_TYPE_AUTOCONFIG_FREQ_BAND:
                    {
                        struct autoconfigFreqBandTLV *t = (struct autoconfigFreqBandTLV *)p;

                        freq_band_is_present = 1;
                        freq_band            = t->freq_band;

                        break;
                    }
                    case TLV_TYPE_SUPPORTED_SERVICE:
                    {
                        /* We don't actually care about the contents. */
                        supported_service_is_present = true;
                        break;
                    }
                    case TLV_TYPE_SEARCHED_SERVICE:
                    {
                        /* We don't actually care about the contents. */
                        searched_service_is_present = true;
                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unexpected TLV (%d) type inside CMDU\n", p->type);
                        break;
                    }
                }
                i++;
            }

            // Make sure that all needed parameters were present in the message
            //
            if (
                 0 == memcmp(al_mac_address, dummy_mac_address, 6) ||
                 0 == searched_role_is_present                              ||
                 0 == freq_band_is_present
               )
            {
                PLATFORM_PRINTF_DEBUG_WARNING("More TLVs were expected inside this CMDU\n");
                return PROCESS_CMDU_KO;
            }

            if (IEEE80211_ROLE_AP != searched_role)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Unexpected 'searched role'\n");
                return PROCESS_CMDU_KO;
            }

            // Check our local interfaces, looking for one acting as the
            // registrar.
            // If one is found, send the response.
            //
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
                     IEEE80211_ROLE_AP == x->interface_type_data.ieee80211.role     &&
                     0 == memcmp(x->mac_address, DMregistrarMacGet(), 6)   &&
                     (
                       (
                        (INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ == x->interface_type   ||
                         INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ == x->interface_type   ||
                         INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ == x->interface_type)  &&
                        freq_band == IEEE80211_FREQUENCY_BAND_2_4_GHZ
                       )                                                               ||
                       (
                        (INTERFACE_TYPE_IEEE_802_11A_5_GHZ   == x->interface_type   ||
                         INTERFACE_TYPE_IEEE_802_11N_5_GHZ   == x->interface_type   ||
                         INTERFACE_TYPE_IEEE_802_11AC_5_GHZ  == x->interface_type)  &&
                        freq_band == IEEE80211_FREQUENCY_BAND_5_GHZ
                       )                                                               ||
                       (
                        (INTERFACE_TYPE_IEEE_802_11AD_60_GHZ == x->interface_type)  &&
                        freq_band == IEEE80211_FREQUENCY_BAND_60_GHZ
                       )
                     )
                   )
                {
                    PLATFORM_PRINTF_DEBUG_DETAIL("Interface %s is AP, registrar, and uses the same freq band. Sending response...\n",ifs_names[i]);

                    if ( 0 == send1905APAutoconfigurationResponsePacket(DMmacToInterfaceName(receiving_interface_addr), c->message_id, al_mac_address, freq_band,
                                                                        supported_service_is_present || searched_service_is_present))
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Could not send 'AP autoconfiguration response' message\n");
                    }

                    free_1905_INTERFACE_INFO(x);
                    break;
                }
                else
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("Interface %s is not AP, or not registrar, or does not use the same freq band (interface type = %d, freq_band = %d,  role = %d)\n", ifs_names[i], x->interface_type, freq_band, x->interface_type_data.ieee80211.role );
                }

                free_1905_INTERFACE_INFO(x);
            }

            free_LIST_OF_1905_INTERFACES(ifs_names, ifs_nr);

            break;
        }
        case CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE:
        {
            // When a "AP-autoconfig response" is received then we have to
            // search for the first interface which is an unconfigured AP with
            // the same freq band as the one contained in the message and send
            // a AP-autoconfig WSC-M1

            struct tlv *p;
            uint8_t i;

            char **ifs_names;
            uint8_t  ifs_nr;

            uint8_t supported_role_is_present;
            uint8_t supported_role;

            uint8_t supported_freq_band_is_present;
            uint8_t supported_freq_band;

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            if (NULL == c->list_of_TLVs)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
                break;
            }

            // First, parse the incomming packet to find out two things:
            //   parameters.
            // - The "supported role" contained in the "supported role TLV"
            //   (must be "REGISTRAR")
            // - The "supported freq band" contained in the "supported freq
            // band TLV" (must match the one of our local unconfigured
            // interface)
            //
            i = 0;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_SUPPORTED_ROLE:
                    {
                        struct supportedRoleTLV *t = (struct supportedRoleTLV *)p;

                        supported_role_is_present = 1;
                        supported_role            = t->role;

                        break;
                    }
                    case TLV_TYPE_SUPPORTED_FREQ_BAND:
                    {
                        struct supportedFreqBandTLV *t = (struct supportedFreqBandTLV *)p;

                        supported_freq_band_is_present = 1;
                        supported_freq_band            = t->freq_band;

                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unexpected TLV (%d) type inside CMDU\n", p->type);
                        break;
                    }
                }
                i++;
            }

            // Make sure that all needed parameters were present in the message
            //
            if (
                 0 == supported_role_is_present      ||
                 0 == supported_freq_band_is_present
               )
            {
                PLATFORM_PRINTF_DEBUG_WARNING("More TLVs were expected inside this CMDU\n");
                return PROCESS_CMDU_KO;
            }

            if (IEEE80211_ROLE_AP != supported_role)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Unexpected 'searched role'\n");
                return PROCESS_CMDU_KO;
            }

            // Check our local interfaces, looking for an unconfigured AP
            // interface that matches the freq band.
            // If one is found, send the response.
            //
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
                     IEEE80211_ROLE_AP == x->interface_type_data.ieee80211.role     &&
                     0x00 == x->interface_type_data.ieee80211.bssid[0]              &&
                     0x00 == x->interface_type_data.ieee80211.bssid[1]              &&
                     0x00 == x->interface_type_data.ieee80211.bssid[2]              &&
                     0x00 == x->interface_type_data.ieee80211.bssid[3]              &&
                     0x00 == x->interface_type_data.ieee80211.bssid[4]              &&
                     0x00 == x->interface_type_data.ieee80211.bssid[5]              &&
                     (
                       (
                        (INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ == x->interface_type   ||
                         INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ == x->interface_type   ||
                         INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ == x->interface_type)  &&
                        supported_freq_band == IEEE80211_FREQUENCY_BAND_2_4_GHZ
                       )                                                               ||
                       (
                        (INTERFACE_TYPE_IEEE_802_11A_5_GHZ   == x->interface_type   ||
                         INTERFACE_TYPE_IEEE_802_11N_5_GHZ   == x->interface_type   ||
                         INTERFACE_TYPE_IEEE_802_11AC_5_GHZ  == x->interface_type)  &&
                        supported_freq_band == IEEE80211_FREQUENCY_BAND_5_GHZ
                       )                                                               ||
                       (
                        (INTERFACE_TYPE_IEEE_802_11AD_60_GHZ == x->interface_type)  &&
                        supported_freq_band == IEEE80211_FREQUENCY_BAND_60_GHZ
                       )
                     )
                   )
                {
                    uint8_t   *m1;
                    uint16_t   m1_size;
                    void    *key;

                    uint8_t *dst_mac;
                    uint8_t *p;

                    PLATFORM_PRINTF_DEBUG_DETAIL("Interface %s is an unconfigured AP and uses the same freq band. Sending WSC-M1...\n",ifs_names[i]);

                    // Obtain WSC-M1 and send the WSC TLV
                    //
                    wscBuildM1(ifs_names[i], &m1, &m1_size, &key);

                    // We must send the WSC TLV to the AL MAC of the node who
                    // sent the response, however, this AL MAC is *not*
                    // contained in the response. The only thing we can do at
                    // this point is try to search our AL neighbors data base
                    // for a matching MAC.
                    //
                    p = DMmacToAlMac(src_addr);

                    if (NULL == p)
                    {
                        // The standard says we should always send to the AL
                        // MAC address, however, in these cases, instead of
                        // just dropping the packet, sending the response to
                        // the 'src' address from the AUTOCONFIGURATION RESPONSE
                        // seems the right thing to do.
                        //
                        dst_mac = src_addr;
                        PLATFORM_PRINTF_DEBUG_WARNING("Unknown destination AL MAC. Using the 'src' MAC from the AUTOCONFIGURATION RESPONSE (%02x:%02x:%02x:%02x:%02x:%02x)\n", dst_mac[0],dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5]);
                    }
                    else
                    {
                        dst_mac = p;
                    }

                    if ( 0 == send1905APAutoconfigurationWSCPacket(DMmacToInterfaceName(receiving_interface_addr), getNextMid(), dst_mac, m1, m1_size))
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Could not send 'AP autoconfiguration WSC-M1' message\n");
                    }

                    if (NULL != p)
                    {
                        free(p);
                    }

                    free_1905_INTERFACE_INFO(x);
                    break;
                }

                free_1905_INTERFACE_INFO(x);
            }

            free_LIST_OF_1905_INTERFACES(ifs_names, ifs_nr);

            break;
        }
        case CMDU_TYPE_AP_AUTOCONFIGURATION_WSC:
        {
            struct tlv *p;
            uint8_t i;

            uint8_t  *wsc_frame;
            uint16_t  wsc_frame_size;

            uint8_t wsc_type;

            // When a "AP-autoconfig WSC" is received we first have to find out
            // if the contained message is M1 or M2.
            // If it is M1, send an M2 response.
            // If it is M2, apply the received configuration.

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_AP_AUTOCONFIGURATION_WSC (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            if (NULL == c->list_of_TLVs)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
                break;
            }

            wsc_frame      = NULL;
            wsc_frame_size = 0;
            i = 0;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_WSC:
                    {
                        struct wscTLV *t = (struct wscTLV *)p;

                        wsc_frame      = t->wsc_frame;
                        wsc_frame_size = t->wsc_frame_size;

                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unexpected TLV (%d) type inside CMDU\n", p->type);
                        break;
                    }
                }
                i++;
            }

            // Make sure there was a WSC TLV in the message
            //
            if ( NULL == wsc_frame)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("More TLVs were expected inside this CMDU\n");
                return PROCESS_CMDU_KO;
            }

            wsc_type = wscGetType(wsc_frame, wsc_frame_size);

            if (WSC_TYPE_M2 == wsc_type)
            {
                // Process it and apply the configuration to the corresponding
                // interface.
                //
                wscProcessM2(NULL, NULL, 0, wsc_frame, wsc_frame_size);
                  // NOTE: this function will automatically free M1.

                // One more thing: This node *might* have other unconfigured AP
                // interfaces (in addition to the one we have just configured),
                // thus, re-trigger the AP discovery process, just in case.
                // Note that this function will do nothing if there are no
                // unconfigured AP interfaces remaining.
                //
                // TODO: Uncomment this once "wscProcessM2()" is capable of
                // actually configuring the AP so that it does not appear as
                // "unconfigured" anymore (or else we will enter an infinite
                // loop!)
                //return PROCESS_CMDU_OK_TRIGGER_AP_SEARCH;
            }
            else if (WSC_TYPE_M1 == wsc_type)
            {
                // We hadn't previously sent an M1 (ie. we are the registrar),
                // thus the contents of the just received message must be M1.
                //
                // Process it and send an M2 response.
                //
                uint8_t   *m2;
                uint16_t   m2_size;

                uint8_t   *dst_mac;
                uint8_t   *p;

                wscBuildM2(wsc_frame, wsc_frame_size, &m2, &m2_size);

                // We must send M2 to the AL MAC of the node who sent M1,
                // however, this AL MAC is *not* contained in M1.
                // The only thing we can do at this point is try to search our
                // AL neighbors data base for a matching MAC.
                //
                p = DMmacToAlMac(src_addr);

                if (NULL == p)
                {
                    // The standard says we should always send to the AL MAC
                    // address, however, in these cases, instead of just
                    // dropping the packet, sending M2 to the 'src' address
                    // from the M1 seems the right thing to do.
                    //
                    dst_mac = src_addr;
                    PLATFORM_PRINTF_DEBUG_WARNING("Unknown destination AL MAC. Using the 'src' MAC from M1 (%02x:%02x:%02x:%02x:%02x:%02x)\n", dst_mac[0],dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5]);
                }
                else
                {
                    dst_mac = p;
                }

                if ( 0 == send1905APAutoconfigurationWSCPacket(DMmacToInterfaceName(receiving_interface_addr), getNextMid(), dst_mac, m2, m2_size))
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("Could not send 'AP autoconfiguration WSC-M2' message\n");
                }

                if (NULL != p)
                {
                    free(p);
                }

                wscFreeM2(m2, m2_size);
            }
            else
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Unknown type of WSC message!\n");
            }

            break;
        }
        case CMDU_TYPE_AP_AUTOCONFIGURATION_RENEW:
        {
            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_AP_AUTOCONFIGURATION_RENEW (%s)\n", DMmacToInterfaceName(receiving_interface_addr));
            // TODO

            break;
        }
        case CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION:
        {
            // According to "Section 9.2.2.2", when a "push button event
            // notification" is received we have to:
            //
            //   1. Transition *all* interfaces to POWER_STATE_PWR_ON
            //
            //   2. Start the "push button" configuration process in all those
            //     interfaces that:
            //       2.1 Are not 802.11
            //       2.2 Are 802.11 APs, configured as "registrars", but only if
            //           the received message did not contain 802.11 media type
            //           information.

            struct tlv *p;
            uint8_t i;

            uint8_t dummy_mac_address[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            char **ifs_names;
            uint8_t  ifs_nr;

            uint8_t wifi_data_is_present;

            uint8_t  al_mac_address[6];

            wifi_data_is_present = 0;
            memcpy(al_mac_address, dummy_mac_address, 6);

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            if (NULL == c->list_of_TLVs)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
                break;
            }

            // First, parse the incomming packet to find out if the 'push
            // button' event TLV contains 802.11 data.
            //
            i = 0;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_AL_MAC_ADDRESS_TYPE:
                    {
                        struct alMacAddressTypeTLV *t = (struct alMacAddressTypeTLV *)p;

                        memcpy(al_mac_address, t->al_mac_address, 6);

                        break;
                    }
                    case TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION:
                    {
                        uint8_t j;
                        struct pushButtonEventNotificationTLV *t = (struct pushButtonEventNotificationTLV *)p;


                        for (j=0; j<t->media_types_nr; j++)
                        {
                            if (
                                 INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ == t->media_types[j].media_type ||
                                 INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ == t->media_types[j].media_type ||
                                 INTERFACE_TYPE_IEEE_802_11A_5_GHZ   == t->media_types[j].media_type ||
                                 INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ == t->media_types[j].media_type ||
                                 INTERFACE_TYPE_IEEE_802_11N_5_GHZ   == t->media_types[j].media_type ||
                                 INTERFACE_TYPE_IEEE_802_11AC_5_GHZ  == t->media_types[j].media_type ||
                                 INTERFACE_TYPE_IEEE_802_11AD_60_GHZ == t->media_types[j].media_type ||
                                 INTERFACE_TYPE_IEEE_802_11AF_GHZ    == t->media_types[j].media_type
                                )
                            {
                                wifi_data_is_present = 1;
                                break;
                            }
                        }

                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unexpected TLV (%d) type inside CMDU\n", p->type);
                        break;
                    }
                }
                i++;
            }

            if (0 == memcmp(al_mac_address, dummy_mac_address, 6))
            {
                PLATFORM_PRINTF_DEBUG_WARNING("More TLVs were expected inside this CMDU\n");
                return PROCESS_CMDU_KO;
            }

            // Next, switch on all interfaces
            //
            ifs_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&ifs_nr);

            PLATFORM_PRINTF_DEBUG_DETAIL("Transitioning all local interfaces to POWER_ON\n");

#ifndef DO_NOT_ACCEPT_UNAUTHENTICATED_COMMANDS
            for (i=0; i<ifs_nr; i++)
            {
                PLATFORM_SET_INTERFACE_POWER_MODE(ifs_names[i], INTERFACE_POWER_STATE_ON);
            }
#endif
            // Finally, for those non wifi interfaces (or a wifi interface whose
            // MAC address matches the network registrar MAC address), start
            // the "push button" configuration process.
            //
            PLATFORM_PRINTF_DEBUG_DETAIL("Starting 'push button' configuration process on all compatible interfaces\n");
            for (i=0; i<ifs_nr; i++)
            {
                struct interfaceInfo *x;

                x = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
                if (NULL == x)
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);
                    continue;
                }

                if (2 == x->push_button_on_going)
                {
                    PLATFORM_PRINTF_DEBUG_DETAIL("%s is not compatible. Skipping...\n",ifs_names[i]);

                    free_1905_INTERFACE_INFO(x);
                    continue;
                }

                if (
                     INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ == x->interface_type   ||
                     INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ == x->interface_type   ||
                     INTERFACE_TYPE_IEEE_802_11A_5_GHZ   == x->interface_type   ||
                     INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ == x->interface_type   ||
                     INTERFACE_TYPE_IEEE_802_11N_5_GHZ   == x->interface_type   ||
                     INTERFACE_TYPE_IEEE_802_11AC_5_GHZ  == x->interface_type   ||
                     INTERFACE_TYPE_IEEE_802_11AD_60_GHZ == x->interface_type   ||
                     INTERFACE_TYPE_IEEE_802_11AF_GHZ    == x->interface_type
                   )
                {
                     if (
                          IEEE80211_ROLE_AP != x->interface_type_data.ieee80211.role ||
                          0 != memcmp(x->mac_address, DMregistrarMacGet(), 6)
                         )
                     {
                         PLATFORM_PRINTF_DEBUG_DETAIL("This wifi interface is not the registrar. Skipping...\n",ifs_names[i]);

                         free_1905_INTERFACE_INFO(x);
                         continue;
                     }
                     else if (0 == wifi_data_is_present)
                     {
                         PLATFORM_PRINTF_DEBUG_DETAIL("This wifi interface is the registrar, but the 'push button event notification' message did not contain wifi data. Skipping...\n",ifs_names[i]);

                         free_1905_INTERFACE_INFO(x);
                         continue;
                     }
                }

                free_1905_INTERFACE_INFO(x);

                PLATFORM_PRINTF_DEBUG_INFO("Starting push button configuration process on interface %s\n", ifs_names[i]);
                if (0 == PLATFORM_START_PUSH_BUTTON_CONFIGURATION(ifs_names[i], queue_id, al_mac_address, c->message_id))
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("Could not start 'push button' configuration process on interface\n",ifs_names[i]);
                }
            }

            free_LIST_OF_1905_INTERFACES(ifs_names, ifs_nr);

            break;
        }
        case CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION:
        {
            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION (%s)\n", DMmacToInterfaceName(receiving_interface_addr));
            // TODO: Somehow "signal" upper layers (?)

            break;
        }
        case CMDU_TYPE_GENERIC_PHY_QUERY:
        {
            // When a "generic phy query" is received we must reply with the
            // list of local "generic" interfaces inside a "generic phy
            // response" CMDU.
            // Not that even if we don't have any "generic" interface (ie. its
            // 'media type' is "MEDIA_TYPE_UNKNOWN") the response will be sent
            // (containing a TLV that says there are "zero" generic interfaces)

            uint8_t *dst_mac;
            uint8_t *p;

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_GENERIC_PHY_QUERY (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            // We must send the response to the AL MAC of the node who sent the
            // query, however, this AL MAC is *not* contained in the query.
            // The only thing we can do at this point is try to search our AL
            // neighbors data base for a matching MAC.
            //
            p = DMmacToAlMac(src_addr);

            if (NULL == p)
            {
                // The standard says we should always send to the AL MAC
                // address, however, in these cases, instead of just dropping
                // the packet, sending the response to the 'src' address from
                // the TOPOLOGY QUERY seems the right thing to do.
                //
                dst_mac = src_addr;
                PLATFORM_PRINTF_DEBUG_WARNING("Unknown destination AL MAC. Using the 'src' MAC from the GENERIC PHY QUERY (%02x:%02x:%02x:%02x:%02x:%02x)\n", dst_mac[0],dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5]);
            }
            else
            {
                dst_mac = p;
            }

            if ( 0 == send1905GenericPhyResponsePacket(DMmacToInterfaceName(receiving_interface_addr), c->message_id, dst_mac))
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not send 'topology query' message\n");
            }

            if (NULL != p)
            {
                free(p);
            }

            break;
        }
        case CMDU_TYPE_GENERIC_PHY_RESPONSE:
        {
            // When a "generic phy response" is received we must update our
            // internal database (that keeps track of which 1905 devices are
            // present in the network)

            struct genericPhyDeviceInformationTypeTLV *t;

            struct tlv *p;
            uint8_t  i;

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_GENERIC_PHY_RESPONSE (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            if (NULL == c->list_of_TLVs)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
                break;
            }

            // Call "DMupdateGenericPhyInfo()" for the "generic phy
            // device information type TLV"  contained in this CMDU.
            //
            i = 0;
            t = NULL;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION:
                    {
                        t = (struct genericPhyDeviceInformationTypeTLV *)p;
                        break;
                    }
                    case TLV_TYPE_VENDOR_SPECIFIC:
                    {
                        // According to the standard, zero or more Vendor
                        // Specific TLVs may be present.
                        //
                        free_1905_TLV_structure(p);
                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unexpected TLV (%d) type inside CMDU\n", p->type);

                        free_1905_TLV_structure(p);
                        break;
                    }
                }
                i++;
            }

            if (NULL == t)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("More TLVs were expected inside this CMDU\n");
                return PROCESS_CMDU_KO;
            }

            PLATFORM_PRINTF_DEBUG_DETAIL("Updating network devices database...\n");
            DMupdateNetworkDeviceInfo(t->al_mac_address,
                                      0, NULL,
                                      0, NULL, 0,
                                      0, NULL, 0,
                                      0, NULL, 0,
                                      0, NULL, 0,
                                      0, NULL, 0,
                                      0, NULL,
                                      1, t,
                                      0, NULL,
                                      0, NULL,
                                      0, NULL,
                                      0, NULL,
                                      0, NULL);

            // References to the TLVs cannot be freed by the caller (see the
            // comment in "case CMDU_TYPE_TOPOLOGY_RESPONSE:" to understand the
            // following two lines).
            //
            free(c->list_of_TLVs);
            c->list_of_TLVs = NULL;

            // Show all network devices (ie. print them through the logging
            // system)
            //
            DMdumpNetworkDevices(PLATFORM_PRINTF_DEBUG_DETAIL);

            break;
        }
        case CMDU_TYPE_HIGHER_LAYER_QUERY:
        {
            // When a "high layer query" is received we must reply with the
            // list of items inside a "high layer response" CMDU.

            uint8_t *dst_mac;
            uint8_t *al_mac;

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_HIGHER_LAYER_QUERY (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            // We must send the response to the AL MAC of the node who sent the
            // query, however, this AL MAC is *not* contained in the query.
            // The only thing we can do at this point is try to search our AL
            // neighbors data base for a matching MAC.
            //
            al_mac = DMmacToAlMac(src_addr);

            if (NULL == al_mac)
            {
                // The standard says we should always send to the AL MAC
                // address, however, in these cases, instead of just dropping
                // the packet, sending the response to the 'src' address from
                // the TOPOLOGY QUERY seems the right thing to do.
                //
                dst_mac = src_addr;
                PLATFORM_PRINTF_DEBUG_WARNING("Unknown destination AL MAC. Using the 'src' MAC from the HIGH LAYER QUERY (%02x:%02x:%02x:%02x:%02x:%02x)\n", dst_mac[0],dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5]);
            }
            else
            {
                dst_mac = al_mac;
            }

            if ( 0 == send1905HighLayerResponsePacket(DMmacToInterfaceName(receiving_interface_addr), c->message_id, dst_mac))
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not send 'high layer response' message\n");
            }

            if (NULL != al_mac)
            {
                free(al_mac);
            }

            break;
        }
        case CMDU_TYPE_HIGHER_LAYER_RESPONSE:
        {
            // When a "high layer response" is received we must update our
            // internal database (that keeps track of which 1905 devices are
            // present in the network)

            struct x1905ProfileVersionTLV      *profile         = NULL;
            struct deviceIdentificationTypeTLV *identification  = NULL;
            struct controlUrlTypeTLV           *control_url     = NULL;
            struct ipv4TypeTLV                 *ipv4            = NULL;
            struct ipv6TypeTLV                 *ipv6            = NULL;

            uint8_t  al_mac_address[6];
            uint8_t  al_mac_address_is_present;

            struct tlv *p;
            uint8_t  i;

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_HIGHER_LAYER_RESPONSE (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            if (NULL == c->list_of_TLVs)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
                break;
            }

            // Call "DMupdateGenericPhyInfo()" with each of the TLVs contained
            // in this CMDU
            //
            PLATFORM_PRINTF_DEBUG_DETAIL("Updating network devices database...\n");

            i                         = 0;
            al_mac_address_is_present = 0;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_AL_MAC_ADDRESS_TYPE:
                    {
                        struct alMacAddressTypeTLV *t = (struct alMacAddressTypeTLV *)p;

                        memcpy(al_mac_address, t->al_mac_address, 6);

                        al_mac_address_is_present = 1;

                        free_1905_TLV_structure(p);
                        break;
                    }
                    case TLV_TYPE_1905_PROFILE_VERSION:
                    {
                        profile = (struct x1905ProfileVersionTLV *)p;
                        break;
                    }
                    case TLV_TYPE_DEVICE_IDENTIFICATION:
                    {
                        identification = (struct deviceIdentificationTypeTLV *)p;
                        break;
                    }
                    case TLV_TYPE_CONTROL_URL:
                    {
                        control_url = (struct controlUrlTypeTLV *)p;
                        break;
                    }
                    case TLV_TYPE_IPV4:
                    {
                        ipv4 = (struct ipv4TypeTLV *)p;
                        break;
                    }
                    case TLV_TYPE_IPV6:
                    {
                        ipv6 = (struct ipv6TypeTLV *)p;
                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unexpected TLV (%d) type inside CMDU\n", p->type);

                        free_1905_TLV_structure(p);
                        break;
                    }
                }
                i++;
            }

            if (0 == al_mac_address_is_present)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("More TLVs were expected inside this CMDU\n");
                return PROCESS_CMDU_KO;
            }

            // Next, update the database. This will take care of duplicate
            // entries (and free the TLV if needed)
            //
            PLATFORM_PRINTF_DEBUG_DETAIL("Updating network devices database...\n");
            DMupdateNetworkDeviceInfo(al_mac_address,
                                      0, NULL,
                                      0, NULL, 0,
                                      0, NULL, 0,
                                      0, NULL, 0,
                                      0, NULL, 0,
                                      0, NULL, 0,
                                      0, NULL,
                                      0, NULL,
                                      1, profile,
                                      1, identification,
                                      1, control_url,
                                      1, ipv4,
                                      1, ipv6);

            // References to the TLVs cannot be freed by the caller (see the
            // comment in "case CMDU_TYPE_TOPOLOGY_RESPONSE:" to understand the
            // following two lines).
            //
            free(c->list_of_TLVs);
            c->list_of_TLVs = NULL;

            // Show all network devices (ie. print them through the logging
            // system)
            //
            DMdumpNetworkDevices(PLATFORM_PRINTF_DEBUG_DETAIL);

            break;
        }
        case CMDU_TYPE_INTERFACE_POWER_CHANGE_REQUEST:
        {
            // When an "interface power change" request is received we need to
            // set the local interfaces to the requested power modes and reply
            // back with the result of these operations

            struct interfacePowerChangeInformationTLV *t;

            struct tlv *p;
            uint8_t  i;

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_INTERFACE_POWER_CHANGE_REQUEST (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            if (NULL == c->list_of_TLVs)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
                break;
            }

            // Search for the "interface power change information type" TLV
            //
            i = 0;
            t = NULL;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION:
                    {
                        t = (struct interfacePowerChangeInformationTLV *)p;
                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unexpected TLV (%d) type inside CMDU\n", p->type);

                        break;
                    }
                }
                i++;
            }

            if (NULL == t)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("More TLVs were expected inside this CMDU\n");
                return PROCESS_CMDU_KO;
            }

            for (i=0; i<t->power_change_interfaces_nr; i++)
            {
                uint8_t r;
                uint8_t results;

#ifndef DO_NOT_ACCEPT_UNAUTHENTICATED_COMMANDS
                r = PLATFORM_SET_INTERFACE_POWER_MODE(DMmacToInterfaceName(t->power_change_interfaces[i].interface_address), t->power_change_interfaces[i].requested_power_state);
#else
                r = INTERFACE_POWER_RESULT_KO;
#endif

                switch (r)
                {
                    case INTERFACE_POWER_RESULT_EXPECTED:
                    {
                        results = POWER_STATE_RESULT_COMPLETED;
                        break;
                    }
                    case INTERFACE_POWER_RESULT_NO_CHANGE:
                    {
                        results = POWER_STATE_RESULT_NO_CHANGE;
                        break;
                    }
                    case INTERFACE_POWER_RESULT_ALTERNATIVE:
                    {
                        results = POWER_STATE_RESULT_ALTERNATIVE_CHANGE;
                        break;
                    }
                    case INTERFACE_POWER_RESULT_KO:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("  Could not set power mode on interface %s\n",DMmacToInterfaceName(t->power_change_interfaces[i].interface_address));
                        results = POWER_STATE_RESULT_NO_CHANGE;
                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("  Unknown power mode return value: %d\n",r);
                        results = POWER_STATE_RESULT_NO_CHANGE;
                        break;
                    }
                }

                PLATFORM_PRINTF_DEBUG_DETAIL("  Setting interface #%d %s (%02x:%02x:%02x:%02x:%02x:%02x) to %s --> %s\n", i,
                                             DMmacToInterfaceName(t->power_change_interfaces[i].interface_address),
                                             t->power_change_interfaces[i].interface_address[0], t->power_change_interfaces[i].interface_address[1], t->power_change_interfaces[i].interface_address[2], t->power_change_interfaces[i].interface_address[3], t->power_change_interfaces[i].interface_address[4], t->power_change_interfaces[i].interface_address[5],
                                             t->power_change_interfaces[i].requested_power_state == POWER_STATE_REQUEST_OFF  ? "POWER OFF"  :
                                             t->power_change_interfaces[i].requested_power_state == POWER_STATE_REQUEST_ON   ? "POWER ON"   :
                                             t->power_change_interfaces[i].requested_power_state == POWER_STATE_REQUEST_SAVE ? "POWER SAVE" :
                                             "Unknown",
                                             results == POWER_STATE_RESULT_COMPLETED ? "Completed" :
                                             results == POWER_STATE_RESULT_NO_CHANGE ? "No change" :
                                             results == POWER_STATE_RESULT_ALTERNATIVE_CHANGE ? "Alternative change" :
                                             "Unknown"
                                             );
            }

            break;
        }
        case CMDU_TYPE_INTERFACE_POWER_CHANGE_RESPONSE:
        {
            // When an "interface power change" response is received we don't
            // need to do anything special. Simply log the event.

            struct interfacePowerChangeStatusTLV *t;

            struct tlv *p;
            uint8_t  i;

            PLATFORM_PRINTF_DEBUG_INFO("<-- CMDU_TYPE_INTERFACE_POWER_CHANGE_RESPONSE (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

            if (NULL == c->list_of_TLVs)
            {
                PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
                break;
            }

            // Search for the "interface power change status" TLV
            //
            i = 0;
            t = NULL;
            while (NULL != (p = c->list_of_TLVs[i]))
            {
                switch (p->type)
                {
                    case TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS:
                    {
                        t = (struct interfacePowerChangeStatusTLV *)p;
                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Unexpected TLV (%d) type inside CMDU\n", p->type);

                        break;
                    }
                }
                i++;
            }

            if (NULL == t)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("More TLVs were expected inside this CMDU\n");
                return PROCESS_CMDU_KO;
            }

            for (i=0; i<t->power_change_interfaces_nr; i++)
            {
                PLATFORM_PRINTF_DEBUG_DETAIL("  Interface #%d %s (%02x:%02x:%02x:%02x:%02x:%02x) --> %s\n", i,
                                             DMmacToInterfaceName(t->power_change_interfaces[i].interface_address),
                                             t->power_change_interfaces[i].interface_address[0], t->power_change_interfaces[i].interface_address[1], t->power_change_interfaces[i].interface_address[2], t->power_change_interfaces[i].interface_address[3], t->power_change_interfaces[i].interface_address[4], t->power_change_interfaces[i].interface_address[5],
                                             t->power_change_interfaces[i].result == POWER_STATE_RESULT_COMPLETED ? "Completed" :
                                             t->power_change_interfaces[i].result == POWER_STATE_RESULT_NO_CHANGE ? "No change" :
                                             t->power_change_interfaces[i].result == POWER_STATE_RESULT_ALTERNATIVE_CHANGE ? "Alternative change" :
                                             "Unknown");
            }

            break;
        }

        default:
        {
            break;
        }
    }

    return PROCESS_CMDU_OK;
}

uint8_t processLlpdPayload(struct PAYLOAD *payload, uint8_t *receiving_interface_addr)
{
    struct tlv *p;
    uint8_t  i;

    uint8_t dummy_mac_address[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t  al_mac_address[6];
    uint8_t  mac_address[6];

    memcpy(al_mac_address, dummy_mac_address, 6);
    memcpy(mac_address,    dummy_mac_address, 6);

    if (NULL == payload)
    {
        return 0;
    }

    PLATFORM_PRINTF_DEBUG_INFO("<-- LLDP BRIDGE DISCOVERY (%s)\n", DMmacToInterfaceName(receiving_interface_addr));

    // We need to update the data model structure, which keeps track
    // of local interfaces, neighbors, and neighbors' interfaces, and
    // what type of discovery messages ("topology discovery" and/or
    // "bridge discovery") have been received on each link.

    // First, extract the AL MAC and MAC addresses of the interface
    // which transmitted this bridge discovery message
    //
    i = 0;
    while (NULL != (p = payload->list_of_TLVs[i]))
    {
        switch (p->type)
        {
            case TLV_TYPE_CHASSIS_ID:
            {
                struct chassisIdTLV *t = (struct chassisIdTLV *)p;

                if (CHASSIS_ID_TLV_SUBTYPE_MAC_ADDRESS == t->chassis_id_subtype)
                {
                    memcpy(al_mac_address, t->chassis_id, 6);
                }

                break;
            }
            case TLV_TYPE_MAC_ADDRESS_TYPE:
            {
                struct portIdTLV *t = (struct portIdTLV *)p;

                if (PORT_ID_TLV_SUBTYPE_MAC_ADDRESS == t->port_id_subtype)
                {
                    memcpy(mac_address, t->port_id, 6);
                }

                break;
            }
            case TLV_TYPE_TIME_TO_LIVE:
            {
                break;
            }
            default:
            {
                PLATFORM_PRINTF_DEBUG_DETAIL("Ignoring TLV type %d\n", *p);
                break;
            }
        }
        i++;
    }

    // Make sure that both the AL MAC and MAC addresses were contained
    // in the CMDU
    //
    if (0 == memcmp(al_mac_address, dummy_mac_address, 6) ||
        0 == memcmp(mac_address,    dummy_mac_address, 6))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("More TLVs were expected inside this LLDP message\n");
        return 0;
    }

    PLATFORM_PRINTF_DEBUG_DETAIL("AL MAC address = %02x:%02x:%02x:%02x:%02x:%02x\n", al_mac_address[0], al_mac_address[1], al_mac_address[2], al_mac_address[3], al_mac_address[4], al_mac_address[5]);
    PLATFORM_PRINTF_DEBUG_DETAIL("MAC    address = %02x:%02x:%02x:%02x:%02x:%02x\n", mac_address[0],    mac_address[1],    mac_address[2],    mac_address[3],    mac_address[4],    mac_address[5]);

    // Finally, update the data model
    //
    if (0 == DMupdateDiscoveryTimeStamps(receiving_interface_addr, al_mac_address, mac_address, TIMESTAMP_BRIDGE_DISCOVERY, NULL))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Problems updating data model with topology response TLVs\n");
        return 0;
    }

    return 1;
}

uint8_t process1905Alme(uint8_t *alme_tlv, uint8_t alme_client_id)
{
    if (NULL == alme_tlv)
    {
        return 0;
    }

    // The first byte of the 'alme_tlv' structure always contains its type
    //
    switch (*alme_tlv)
    {
        case ALME_TYPE_GET_INTF_LIST_REQUEST:
        {
            // Obtain the list of local interfaces, retrieve detailed info for
            // each of them, build a response, and send it back.
            //
            PLATFORM_PRINTF_DEBUG_INFO("<-- ALME_TYPE_GET_INTF_LIST_REQUEST\n");

            send1905InterfaceListResponseALME(alme_client_id);

            break;
        }
        case ALME_TYPE_SET_INTF_PWR_STATE_REQUEST:
        {
            PLATFORM_PRINTF_DEBUG_INFO("<-- ALME_TYPE_SET_INTF_PWR_STATE_REQUEST\n");
            break;
        }
        case ALME_TYPE_GET_INTF_PWR_STATE_REQUEST:
        {
            PLATFORM_PRINTF_DEBUG_INFO("<-- ALME_TYPE_GET_INTF_PWR_STATE_REQUEST\n");
            break;
        }
        case ALME_TYPE_SET_FWD_RULE_REQUEST:
        {
            PLATFORM_PRINTF_DEBUG_INFO("<-- ALME_TYPE_SET_FWD_RULE_REQUEST\n");
            break;
        }
        case ALME_TYPE_GET_FWD_RULES_REQUEST:
        {
            PLATFORM_PRINTF_DEBUG_INFO("<-- ALME_TYPE_GET_FWD_RULES_REQUEST\n");
            break;
        }
        case ALME_TYPE_MODIFY_FWD_RULE_REQUEST:
        {
            PLATFORM_PRINTF_DEBUG_INFO("<-- ALME_TYPE_MODIFY_FWD_RULE_REQUEST\n");
            break;
        }
        case ALME_TYPE_REMOVE_FWD_RULE_REQUEST:
        {
            PLATFORM_PRINTF_DEBUG_INFO("<-- ALME_TYPE_MODIFY_FWD_RULE_CONFIRM\n");
            break;
        }
        case ALME_TYPE_GET_METRIC_REQUEST:
        {
            // Obtain the requested metrics, build a response, and send it back.
            //
            struct getMetricRequestALME *p;

            uint8_t dummy_mac_address[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

            PLATFORM_PRINTF_DEBUG_INFO("<-- ALME_TYPE_GET_METRIC_REQUEST\n");

            p = (struct getMetricRequestALME *)alme_tlv;

            if (0 == memcmp(p->interface_address, dummy_mac_address, 6))
            {
                // Request metrics against all neighbors
                //
                send1905MetricsResponseALME(alme_client_id, NULL);
            }
            else
            {
                // Request metrics against one specific neighbor
                //
                send1905MetricsResponseALME(alme_client_id, p->interface_address);
            }

            break;
        }
        case ALME_TYPE_CUSTOM_COMMAND_REQUEST:
        {
            struct customCommandRequestALME *p;

            PLATFORM_PRINTF_DEBUG_INFO("<-- ALME_TYPE_CUSTOM_COMMAND_REQUEST\n");

            p = (struct customCommandRequestALME *)alme_tlv;

            send1905CustomCommandResponseALME(alme_client_id, p->command);

            break;
        }
        case ALME_TYPE_GET_INTF_LIST_RESPONSE:
        case ALME_TYPE_SET_INTF_PWR_STATE_CONFIRM:
        case ALME_TYPE_GET_INTF_PWR_STATE_RESPONSE:
        case ALME_TYPE_SET_FWD_RULE_CONFIRM:
        case ALME_TYPE_GET_FWD_RULES_RESPONSE:
        case ALME_TYPE_MODIFY_FWD_RULE_CONFIRM:
        case ALME_TYPE_REMOVE_FWD_RULE_CONFIRM:
        case ALME_TYPE_GET_METRIC_RESPONSE:
        case ALME_TYPE_CUSTOM_COMMAND_RESPONSE:
        {
            // These messages should never be receiving by an AL entity. It is
            // the AL entity the one who generates them and then sends them to
            // the HLE.
            //
            PLATFORM_PRINTF_DEBUG_WARNING("ALME RESPONSE/CONFIRM message received (type = %d). Ignoring...\n", *alme_tlv);
            break;
        }

        default:
        {
            PLATFORM_PRINTF_DEBUG_WARNING("Unknown ALME message received (type = %d). Ignoring...\n", *alme_tlv);
            break;
        }
    }

    return 1;
}

