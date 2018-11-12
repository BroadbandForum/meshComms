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
#include "utils.h"

#include <stdarg.h>   // va_list
#include <stdio.h>    // vsnprintf

#include "al_send.h"
#include "al_datamodel.h"
#include "al_utils.h"

#include "1905_tlvs.h"
#include "1905_cmdus.h"
#include "1905_alme.h"
#include "1905_l2.h"
#include "lldp_tlvs.h"
#include "lldp_payload.h"

#include "platform_os.h"
#include "platform_interfaces.h"
#include "platform_alme_server.h"

#include "al_extension.h"

#include <datamodel.h>
#include <string.h> // memset(), memcmp(), ...

////////////////////////////////////////////////////////////////////////////////
// Private functions and data
////////////////////////////////////////////////////////////////////////////////

//******************************************************************************
//******* Functions to fill (and free) TLVs with local data ********************
//******************************************************************************
//
// Note that not *all* types of TLVs have a corresponding function in this
// section. Only those that either:
//
//   a) Are called from more than one place.
//   b) In order to be filled, the local device/node needs to be queried.
//
// Acording to these rules, some of the TLVs that do *not* have a corresponding
// function in this section are, for example, all "power change" related TLVs,
// LLDP TLVS, etc... These will be manually "filled" and "freed" in the specific
// "send*()" function that makes use of them.

// Given a pointer to a preallocated "deviceInformationTypeTLV" structure, fill
// it with all the pertaining information retrieved from the local device.
//
void _obtainLocalDeviceInfoTLV(struct deviceInformationTypeTLV *device_info)
{
    uint8_t  al_mac_address[6];

    char   **interfaces_names;
    uint8_t    interfaces_names_nr;
    uint8_t    i;

    memcpy(al_mac_address, DMalMacGet(), 6);

    device_info->tlv.type            = TLV_TYPE_DEVICE_INFORMATION_TYPE;
    device_info->al_mac_address[0]   = al_mac_address[0];
    device_info->al_mac_address[1]   = al_mac_address[1];
    device_info->al_mac_address[2]   = al_mac_address[2];
    device_info->al_mac_address[3]   = al_mac_address[3];
    device_info->al_mac_address[4]   = al_mac_address[4];
    device_info->al_mac_address[5]   = al_mac_address[5];
    device_info->local_interfaces_nr = 0;
    device_info->local_interfaces    = NULL;

    interfaces_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&interfaces_names_nr);

    // Add all interfaces that are *not* in "POWER OFF" mode
    //
    for (i=0; i<interfaces_names_nr; i++)
    {
        struct interfaceInfo *x;

        if (NULL == (x = PLATFORM_GET_1905_INTERFACE_INFO(interfaces_names[i])))
        {
            // Error retrieving information for this interface.
            // Ignore it.
            //
            continue;
        }

        if (INTERFACE_POWER_STATE_OFF == x->power_state)
        {
            // Ignore interfaces that are in "POWER OFF" mode (they will
            // be included in the "power off" TLV, later, on this same
            // CMDU)
            //
            free_1905_INTERFACE_INFO(x);
            continue;
        }

        if (0 == device_info->local_interfaces_nr)
        {
            device_info->local_interfaces = (struct _localInterfaceEntries *)memalloc(sizeof(struct _localInterfaceEntries));
        }
        else
        {
            device_info->local_interfaces = (struct _localInterfaceEntries *)memrealloc(device_info->local_interfaces, sizeof(struct _localInterfaceEntries) *(device_info->local_interfaces_nr + 1));
        }

        device_info->local_interfaces[device_info->local_interfaces_nr].mac_address[0] = x->mac_address[0];
        device_info->local_interfaces[device_info->local_interfaces_nr].mac_address[1] = x->mac_address[1];
        device_info->local_interfaces[device_info->local_interfaces_nr].mac_address[2] = x->mac_address[2];
        device_info->local_interfaces[device_info->local_interfaces_nr].mac_address[3] = x->mac_address[3];
        device_info->local_interfaces[device_info->local_interfaces_nr].mac_address[4] = x->mac_address[4];
        device_info->local_interfaces[device_info->local_interfaces_nr].mac_address[5] = x->mac_address[5];
        device_info->local_interfaces[device_info->local_interfaces_nr].media_type     = x->interface_type;
        switch (x->interface_type)
        {
            case INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ:
            case INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ:
            case INTERFACE_TYPE_IEEE_802_11A_5_GHZ:
            case INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ:
            case INTERFACE_TYPE_IEEE_802_11N_5_GHZ:
            case INTERFACE_TYPE_IEEE_802_11AC_5_GHZ:
            case INTERFACE_TYPE_IEEE_802_11AD_60_GHZ:
            case INTERFACE_TYPE_IEEE_802_11AF_GHZ:
            {
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data_size                                          = 10;
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee80211.network_membership[0]               = x->interface_type_data.ieee80211.bssid[0];
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee80211.network_membership[1]               = x->interface_type_data.ieee80211.bssid[1];
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee80211.network_membership[2]               = x->interface_type_data.ieee80211.bssid[2];
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee80211.network_membership[3]               = x->interface_type_data.ieee80211.bssid[3];
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee80211.network_membership[4]               = x->interface_type_data.ieee80211.bssid[4];
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee80211.network_membership[5]               = x->interface_type_data.ieee80211.bssid[5];
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee80211.role                                = x->interface_type_data.ieee80211.role;
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee80211.ap_channel_band                     = x->interface_type_data.ieee80211.ap_channel_band;
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee80211.ap_channel_center_frequency_index_1 = x->interface_type_data.ieee80211.ap_channel_center_frequency_index_1;
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee80211.ap_channel_center_frequency_index_2 = x->interface_type_data.ieee80211.ap_channel_center_frequency_index_2;
                break;
            }
            case INTERFACE_TYPE_IEEE_1901_FFT:
            {
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data_size                           = 7;
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee1901.network_identifier[0] = x->interface_type_data.ieee1901.network_identifier[0];
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee1901.network_identifier[1] = x->interface_type_data.ieee1901.network_identifier[1];
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee1901.network_identifier[2] = x->interface_type_data.ieee1901.network_identifier[2];
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee1901.network_identifier[3] = x->interface_type_data.ieee1901.network_identifier[3];
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee1901.network_identifier[4] = x->interface_type_data.ieee1901.network_identifier[4];
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee1901.network_identifier[5] = x->interface_type_data.ieee1901.network_identifier[5];
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.ieee1901.network_identifier[6] = x->interface_type_data.ieee1901.network_identifier[6];
                break;
            }
            default:
            {
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data_size  = 0;
                device_info->local_interfaces[device_info->local_interfaces_nr].media_specific_data.dummy = 0;
                break;
            }
        }
        device_info->local_interfaces_nr++;

        free_1905_INTERFACE_INFO(x);
    }

    free_LIST_OF_1905_INTERFACES(interfaces_names, interfaces_names_nr);
}

// Free the contents of the provided "deviceInformationTypeTLV" structure (ie.
// only what was allocated by "_obtainLocalDeviceInfoTLV()", and not the
// "deviceInformationTypeTLV" structure itself, which is the caller's
// responsability)
//
void _freeLocalDeviceInfoTLV(struct deviceInformationTypeTLV *device_info)
{
    free(device_info->local_interfaces);
}

// Given a pointer to a preallocated "deviceBridgingCapabilityTLV" structure,
// fill it with all the pertaining information retrieved from the local device.
//
void _obtainLocalBridgingCapabilitiesTLV(struct deviceBridgingCapabilityTLV *bridge_info)
{
    struct bridge *br;
    uint8_t          br_nr;
    uint8_t          i, j;

    bridge_info->tlv.type = TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES;

    if ((NULL == (br = PLATFORM_GET_LIST_OF_BRIDGES(&br_nr))) || 0 == br_nr)
    {
        // No bridge info
        //
        bridge_info->bridging_tuples_nr = 0;
        bridge_info->bridging_tuples    = NULL;
    }
    else
    {
        bridge_info->bridging_tuples_nr = br_nr;
        bridge_info->bridging_tuples    = (struct _bridgingTupleEntries *)memalloc(sizeof(struct _bridgingTupleEntries) * br_nr);

        for (i=0; i<br_nr; i++)
        {
            bridge_info->bridging_tuples[i].bridging_tuple_macs_nr = br[i].bridged_interfaces_nr;

            if (0 == br[i].bridged_interfaces_nr)
            {
                bridge_info->bridging_tuples[i].bridging_tuple_macs = NULL;
            }
            else
            {
                bridge_info->bridging_tuples[i].bridging_tuple_macs = (struct _bridgingTupleMacEntries *)memalloc(sizeof(struct _bridgingTupleMacEntries) * br[i].bridged_interfaces_nr);

                for (j=0; j<bridge_info->bridging_tuples[i].bridging_tuple_macs_nr; j++)
                {
                    uint8_t mac_address[6];

                    memcpy(mac_address, DMinterfaceNameToMac(br[i].bridged_interfaces[j]), 6);
                    bridge_info->bridging_tuples[i].bridging_tuple_macs[j].mac_address[0] = mac_address[0];
                    bridge_info->bridging_tuples[i].bridging_tuple_macs[j].mac_address[1] = mac_address[1];
                    bridge_info->bridging_tuples[i].bridging_tuple_macs[j].mac_address[2] = mac_address[2];
                    bridge_info->bridging_tuples[i].bridging_tuple_macs[j].mac_address[3] = mac_address[3];
                    bridge_info->bridging_tuples[i].bridging_tuple_macs[j].mac_address[4] = mac_address[4];
                    bridge_info->bridging_tuples[i].bridging_tuple_macs[j].mac_address[5] = mac_address[5];
                }
            }
        }
        free_LIST_OF_BRIDGES(br, br_nr);
    }
}

// Free the contents of the provided "deviceBridgingCapabilityTLV" structure
// (ie.  only what was allocated by "_obtainLocalBridgingCapabilitiesTLV()",
// and not the "deviceBridgingCapabilityTLV" structure itself, which is the
// caller's responsability)
//
void _freeLocalBridgingCapabilitiesTLV(struct deviceBridgingCapabilityTLV *bridge_info)
{
    uint8_t i;

    if (bridge_info->bridging_tuples_nr > 0)
    {
        for (i=0; i<bridge_info->bridging_tuples_nr; i++)
        {
            if ( bridge_info->bridging_tuples[i].bridging_tuple_macs_nr > 0)
            {
                free(bridge_info->bridging_tuples[i].bridging_tuple_macs);
            }
        }
        free(bridge_info->bridging_tuples);
    }
}

// Modify the provided pointers so that they now point to a list of pointers to
// "non1905NeighborDeviceListTLV" and "neighborDeviceListTLV" structures filled
// with all the pertaining information retrieved from the local device.
//
// Example: this is how you would use this function:
//
//   struct non1905NeighborDeviceListTLV **a;   uint8_t a_nr;
//   struct neighborDeviceListTLV        **b;   uint8_t b_nr;
//
//   _obtainLocalNeighborsTLV(&a, &a_nr, &b, &b_nr);
//
//   // a[0] -------> ptr to the first "non1905NeighborDeviceListTLV" structure
//   // a[1] -------> ptr to the second "non1905NeighborDeviceListTLV" structure
//   // ...
//   // a[a_nr-1] --> ptr to the last "non1905NeighborDeviceListTLV" structure
//
//   // b[0] -------> ptr to the first "neighborDeviceListTLV" structure
//   // b[1] -------> ptr to the second "neighborDeviceListTLV" structure
//   // ...
//   // b[b_nr-1] --> ptr to the last "neighborDeviceListTLV" structure
//
void _obtainLocalNeighborsTLV(struct non1905NeighborDeviceListTLV ***non_1905_neighbors, uint8_t *non_1905_neighbors_nr, struct neighborDeviceListTLV ***neighbors, uint8_t *neighbors_nr)
{
    char                  **interfaces_names;
    uint8_t                   interfaces_names_nr;
    uint8_t                   i, j, k;

    *non_1905_neighbors    = NULL;
    *neighbors             = NULL;

    *non_1905_neighbors_nr = 0;
    *neighbors_nr          = 0;

    uint8_t (*al_mac_addresses)[6];
    uint8_t al_mac_addresses_nr;

    interfaces_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&interfaces_names_nr);

    for (i=0; i<interfaces_names_nr; i++)
    {
        struct interfaceInfo                 *x;

        struct non1905NeighborDeviceListTLV  *no;
        struct neighborDeviceListTLV         *yes;

        if (NULL == (x = PLATFORM_GET_1905_INTERFACE_INFO(interfaces_names[i])))
        {
            PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve neighbors of interface %s\n", interfaces_names[i]);
            continue;
        }

        al_mac_addresses = DMgetListOfInterfaceNeighbors(interfaces_names[i], &al_mac_addresses_nr);

        no  = (struct non1905NeighborDeviceListTLV *)memalloc(sizeof(struct non1905NeighborDeviceListTLV));
        yes = (struct neighborDeviceListTLV *)       memalloc(sizeof(struct neighborDeviceListTLV));

        no->tlv.type              = TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST;
        no->local_mac_address[0]  = x->mac_address[0];
        no->local_mac_address[1]  = x->mac_address[1];
        no->local_mac_address[2]  = x->mac_address[2];
        no->local_mac_address[3]  = x->mac_address[3];
        no->local_mac_address[4]  = x->mac_address[4];
        no->local_mac_address[5]  = x->mac_address[5];
        no->non_1905_neighbors_nr = 0;
        no->non_1905_neighbors    = NULL;

        yes->tlv.type              = TLV_TYPE_NEIGHBOR_DEVICE_LIST;
        yes->local_mac_address[0]  = x->mac_address[0];
        yes->local_mac_address[1]  = x->mac_address[1];
        yes->local_mac_address[2]  = x->mac_address[2];
        yes->local_mac_address[3]  = x->mac_address[3];
        yes->local_mac_address[4]  = x->mac_address[4];
        yes->local_mac_address[5]  = x->mac_address[5];
        yes->neighbors_nr          = 0;
        yes->neighbors             = NULL;

        // Decide if each neighbor is a 1905 or a non-1905 neighbor
        //
        if (x->neighbor_mac_addresses_nr != INTERFACE_NEIGHBORS_UNKNOWN)
        {
            uint8_t *al_mac_address_has_been_reported;

            // Keep track of all the AL MACs that the interface reports he is
            // seeing.
            //
            if (0 != al_mac_addresses_nr)
            {
                // Originally, none of the neighbors in the data model has been
                // reported...
                //
                al_mac_address_has_been_reported = (uint8_t *)memalloc(sizeof(uint8_t) * al_mac_addresses_nr);
                memset(al_mac_address_has_been_reported, 0x0, al_mac_addresses_nr);
            }

            for (j=0; j<x->neighbor_mac_addresses_nr; j++)
            {
                uint8_t *al_mac;
                uint8_t k;

                al_mac = DMmacToAlMac(x->neighbor_mac_addresses[j]);

                if (NULL == al_mac)
                {
                    // Non-1905 neighbor

                    uint8_t already_added;

                    // Make sure it has not already been added
                    //
                    already_added = 0;
                    for (k=0; k<no->non_1905_neighbors_nr; k++)
                    {
                        if (0 == memcmp(x->neighbor_mac_addresses[j], no->non_1905_neighbors[k].mac_address, 6))
                        {
                            already_added = 1;
                            break;
                        }
                    }

                    if (0 == already_added)
                    {
                        // This is a new neighbor
                        //
                        if (0 == no->non_1905_neighbors_nr)
                        {
                            no->non_1905_neighbors = (struct _non1905neighborEntries *)memalloc(sizeof(struct _non1905neighborEntries));
                        }
                        else
                        {
                            no->non_1905_neighbors = (struct _non1905neighborEntries *)memrealloc(no->non_1905_neighbors, sizeof(struct _non1905neighborEntries)*(no->non_1905_neighbors_nr+1));
                        }

                        no->non_1905_neighbors[no->non_1905_neighbors_nr].mac_address[0] = x->neighbor_mac_addresses[j][0];
                        no->non_1905_neighbors[no->non_1905_neighbors_nr].mac_address[1] = x->neighbor_mac_addresses[j][1];
                        no->non_1905_neighbors[no->non_1905_neighbors_nr].mac_address[2] = x->neighbor_mac_addresses[j][2];
                        no->non_1905_neighbors[no->non_1905_neighbors_nr].mac_address[3] = x->neighbor_mac_addresses[j][3];
                        no->non_1905_neighbors[no->non_1905_neighbors_nr].mac_address[4] = x->neighbor_mac_addresses[j][4];
                        no->non_1905_neighbors[no->non_1905_neighbors_nr].mac_address[5] = x->neighbor_mac_addresses[j][5];

                        no->non_1905_neighbors_nr++;
                    }
                }
                else
                {
                    // 1905 neighbor

                    uint8_t already_added;

                    // Mark this AL MAC as reported
                    //
                    for (k=0; k<al_mac_addresses_nr; k++)
                    {
                        if (0 == memcmp(al_mac, al_mac_addresses[k], 6))
                        {
                            al_mac_address_has_been_reported[k] = 1;
                            break;
                        }
                    }

                    // Make sure it has not already been added
                    //
                    already_added = 0;
                    for (k=0; k<yes->neighbors_nr; k++)
                    {
                        if (0 == memcmp(al_mac, yes->neighbors[k].mac_address, 6))
                        {
                            already_added = 1;
                            break;
                        }
                    }

                    if (0 == already_added)
                    {
                        // This is a new neighbor
                        //
                        if (0 == yes->neighbors_nr)
                        {
                            yes->neighbors = (struct _neighborEntries *)memalloc(sizeof(struct _neighborEntries));
                        }
                        else
                        {
                            yes->neighbors = (struct _neighborEntries *)memrealloc(yes->neighbors, sizeof(struct _neighborEntries)*(yes->neighbors_nr+1));
                        }

                        yes->neighbors[yes->neighbors_nr].mac_address[0] = al_mac[0];
                        yes->neighbors[yes->neighbors_nr].mac_address[1] = al_mac[1];
                        yes->neighbors[yes->neighbors_nr].mac_address[2] = al_mac[2];
                        yes->neighbors[yes->neighbors_nr].mac_address[3] = al_mac[3];
                        yes->neighbors[yes->neighbors_nr].mac_address[4] = al_mac[4];
                        yes->neighbors[yes->neighbors_nr].mac_address[5] = al_mac[5];
                        yes->neighbors[yes->neighbors_nr].bridge_flag    = DMisNeighborBridged(interfaces_names[i], al_mac);

                        yes->neighbors_nr++;
                    }

                    free(al_mac);
                }
            }
            free_1905_INTERFACE_INFO(x);

            // Update the datamodel so that those neighbours whose MAC addresses
            // have not been reported are removed.
            // This will speed up the "removal" of nodes.
            //
            for (j=0; j<al_mac_addresses_nr; j++)
            {
                if (0 == al_mac_address_has_been_reported[j])
                {
                    DMremoveALNeighborFromInterface(al_mac_addresses[j], interfaces_names[i]);
                    DMrunGarbageCollector();
                }
            }
            if (al_mac_addresses_nr > 0 && NULL != al_mac_address_has_been_reported)
            {
                free(al_mac_address_has_been_reported);
            }
        }
        else
        {
            // The interface reports that it has no way of knowing which MAC
            // neighbors are connected to it.
            // In these cases, *at least* the already known 1905 neighbors
            // (which were discovered by us -not the platform- thanks to the
            // topology discovery process) should be returned.

            for (j=0; j<al_mac_addresses_nr; j++)
            {
                uint8_t already_added;

                // Make sure it has not already been added
                //
                already_added = 0;
                for (k=0; k<yes->neighbors_nr; k++)
                {
                    if (0 == memcmp(al_mac_addresses[j], yes->neighbors[k].mac_address, 6))
                    {
                        already_added = 1;
                        break;
                    }
                }

                if (0 == already_added)
                {
                    // This is a new neighbor
                    //
                    if (0 == yes->neighbors_nr)
                    {
                        yes->neighbors = (struct _neighborEntries *)memalloc(sizeof(struct _neighborEntries));
                    }
                    else
                    {
                        yes->neighbors = (struct _neighborEntries *)memrealloc(yes->neighbors, sizeof(struct _neighborEntries)*(yes->neighbors_nr+1));
                    }

                    yes->neighbors[yes->neighbors_nr].mac_address[0] = al_mac_addresses[j][0];
                    yes->neighbors[yes->neighbors_nr].mac_address[1] = al_mac_addresses[j][1];
                    yes->neighbors[yes->neighbors_nr].mac_address[2] = al_mac_addresses[j][2];
                    yes->neighbors[yes->neighbors_nr].mac_address[3] = al_mac_addresses[j][3];
                    yes->neighbors[yes->neighbors_nr].mac_address[4] = al_mac_addresses[j][4];
                    yes->neighbors[yes->neighbors_nr].mac_address[5] = al_mac_addresses[j][5];
                    yes->neighbors[yes->neighbors_nr].bridge_flag    = DMisNeighborBridged(interfaces_names[i], al_mac_addresses[j]);

                    yes->neighbors_nr++;

                }
            }
        }
        free(al_mac_addresses);

        // At this point we have, for this particular interface, all the
        // non 1905 neighbors in "no" and all 1905 neighbors in "yes".
        // We just need to add "no" and "yes" to the "non_1905_neighbors"
        // and "neighbors" lists and proceed to the next interface.
        //
        if (no->non_1905_neighbors_nr > 0)
        {
            // Add this to the list of non-1905 neighbor TLVs
            //
            if (0 == *non_1905_neighbors_nr)
            {
                *non_1905_neighbors = (struct non1905NeighborDeviceListTLV **)memalloc(sizeof(struct non1905NeighborDeviceListTLV *));
            }
            else
            {
                *non_1905_neighbors = (struct non1905NeighborDeviceListTLV **)memrealloc(*non_1905_neighbors, sizeof(struct non1905NeighborDeviceListTLV *)*(*non_1905_neighbors_nr+1));
            }

            (*non_1905_neighbors)[*non_1905_neighbors_nr] = no;
            (*non_1905_neighbors_nr)++;
        }
        else
        {
            free(no);
        }

        if (yes->neighbors_nr > 0)
        {
            // Add this to the list of non-1905 neighbor TLVs
            //
            if (0 == *neighbors_nr)
            {
                *neighbors = (struct neighborDeviceListTLV **)memalloc(sizeof(struct neighborDeviceListTLV *));
            }
            else
            {
                *neighbors = (struct neighborDeviceListTLV **)memrealloc(*neighbors, sizeof(struct neighborDeviceListTLV *)*(*neighbors_nr+1));
            }

            (*neighbors)[*neighbors_nr] = yes;
            (*neighbors_nr)++;
        }
        else
        {
            free(yes);
        }
    }

    free_LIST_OF_1905_INTERFACES(interfaces_names, interfaces_names_nr);
}


// Free the contents of the pointers filled by a previous call to
// "_obtainLocalNeighborsTLV()".
// This function is called with the same arguments as
// "_obtainLocalNeighborsTLV()"
//
void _freeLocalNeighborsTLV(struct non1905NeighborDeviceListTLV ***non_1905_neighbors, uint8_t *non_1905_neighbors_nr, struct neighborDeviceListTLV ***neighbors, uint8_t *neighbors_nr)
{
    uint8_t i;

    if (*non_1905_neighbors_nr > 0)
    {
        for (i=0; i<*non_1905_neighbors_nr; i++)
        {
            if ((*non_1905_neighbors)[i]->non_1905_neighbors_nr > 0)
            {
                free((*non_1905_neighbors)[i]->non_1905_neighbors);
            }
            free((*non_1905_neighbors)[i]);
        }
        free(*non_1905_neighbors);
    }
    if (*neighbors_nr > 0)
    {
        for (i=0; i<*neighbors_nr; i++)
        {
            if ((*neighbors)[i]->neighbors_nr > 0)
            {
                free((*neighbors)[i]->neighbors);
            }
            free((*neighbors)[i]);
        }
        free(*neighbors);
    }
}

// Given a pointer to a preallocated "powerOffInterfaceTLV" structure, fill
// it with all the pertaining information retrieved from the local device.
//
void _obtainLocalPowerOffInterfacesTLV(struct powerOffInterfaceTLV *power_off)
{
    char                  **interfaces_names;
    uint8_t                   interfaces_names_nr;
    uint8_t                   i;

    power_off->tlv.type                = TLV_TYPE_POWER_OFF_INTERFACE;
    power_off->power_off_interfaces_nr = 0;
    power_off->power_off_interfaces    = NULL;

    interfaces_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&interfaces_names_nr);

    // Search for interfaces in "POWER OFF" mode
    //
    for (i=0; i<interfaces_names_nr; i++)
    {
        struct interfaceInfo *x;

        if (NULL == (x = PLATFORM_GET_1905_INTERFACE_INFO(interfaces_names[i])))
        {
            // Error retrieving information for this interface.
            // Ignore it.
            //
            continue;
        }

        if (INTERFACE_POWER_STATE_OFF != x->power_state)
        {
            // Ignore interfaces that are not in "POWER OFF" mode
            //
            free_1905_INTERFACE_INFO(x);
            continue;
        }

        if (0 == power_off->power_off_interfaces_nr)
        {
            power_off->power_off_interfaces = (struct _powerOffInterfaceEntries *)memalloc(sizeof(struct _powerOffInterfaceEntries));
        }
        else
        {
            power_off->power_off_interfaces = (struct _powerOffInterfaceEntries *)memrealloc(power_off->power_off_interfaces, sizeof(struct _powerOffInterfaceEntries) * (power_off->power_off_interfaces_nr + 1));
        }

        memcpy(power_off->power_off_interfaces[power_off->power_off_interfaces_nr].interface_address,   x->mac_address, 6);


        // "Translate" from "INTERFACE_TYPE_*" to "MEDIA_TYPE_*"
        //
        switch(x->interface_type)
        {
            case INTERFACE_TYPE_IEEE_802_3U_FAST_ETHERNET:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_IEEE_802_3U_FAST_ETHERNET;
                break;
            }
            case INTERFACE_TYPE_IEEE_802_3AB_GIGABIT_ETHERNET:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_IEEE_802_3AB_GIGABIT_ETHERNET;
                break;
            }
            case INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_IEEE_802_11B_2_4_GHZ;
                break;
            }
            case INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_IEEE_802_11G_2_4_GHZ;
                break;
            }
            case INTERFACE_TYPE_IEEE_802_11A_5_GHZ:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_IEEE_802_11A_5_GHZ;
                break;
            }
            case INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_IEEE_802_11N_2_4_GHZ;
                break;
            }
            case INTERFACE_TYPE_IEEE_802_11N_5_GHZ:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_IEEE_802_11N_5_GHZ;
                break;
            }
            case INTERFACE_TYPE_IEEE_802_11AC_5_GHZ:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_IEEE_802_11AC_5_GHZ;
                break;
            }
            case INTERFACE_TYPE_IEEE_802_11AD_60_GHZ:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_IEEE_802_11AD_60_GHZ;
                break;
            }
            case INTERFACE_TYPE_IEEE_802_11AF_GHZ:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_IEEE_802_11AF_GHZ;
                break;
            }
            case INTERFACE_TYPE_IEEE_1901_WAVELET:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_IEEE_1901_WAVELET;
                break;
            }
            case INTERFACE_TYPE_IEEE_1901_FFT:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_IEEE_1901_FFT;
                break;
            }
            case INTERFACE_TYPE_MOCA_V1_1:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_MOCA_V1_1;
                break;
            }
            case INTERFACE_TYPE_UNKNOWN:
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type = MEDIA_TYPE_UNKNOWN;
                break;
            }
        }

        // Only when the media type is "MEDIA_TYPE_UNKNOWN", fill the
        // rest of fields
        //
        if (MEDIA_TYPE_UNKNOWN != power_off->power_off_interfaces[power_off->power_off_interfaces_nr].media_type)
        {
            // Set everything to "zero"
            //
            power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.oui[0]                  = 0x00;
            power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.oui[1]                  = 0x00;
            power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.oui[2]                  = 0x00;
            power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.variant_index           = 0;
            power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.media_specific_bytes_nr = 0;
            power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.media_specific_bytes    = NULL;
        }
        else
        {
            uint16_t  len;
            uint8_t  *m;

            power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.oui[0]                  = x->interface_type_data.other.oui[0];
            power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.oui[1]                  = x->interface_type_data.other.oui[1];
            power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.oui[2]                  = x->interface_type_data.other.oui[2];
            power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.variant_index           = x->interface_type_data.other.variant_index;

            m = forge_media_specific_blob(&x->interface_type_data.other, &len);

            if (NULL != m)
            {
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.media_specific_bytes_nr = len;
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.media_specific_bytes    = m;
            }
            else
            {
                // Ignore media specific data
                //
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.media_specific_bytes_nr = 0;
                power_off->power_off_interfaces[power_off->power_off_interfaces_nr].generic_phy_common_data.media_specific_bytes    = NULL;
            }
        }
        power_off->power_off_interfaces_nr++;

        free_1905_INTERFACE_INFO(x);
    }

    free_LIST_OF_1905_INTERFACES(interfaces_names, interfaces_names_nr);
}

// Free the contents of the provided "powerOffInterfaceTLV" structure (ie.
// only what was allocated by "_obtainLocalPowerOffInterfacesTLV()", and not
// the "powerOffInterfaceTLV" structure itself, which is the caller's
// responsability)
//
void _freeLocalPowerOffInterfacesTLV(struct powerOffInterfaceTLV *power_off)
{
    uint8_t i;

    if (power_off->power_off_interfaces_nr > 0)
    {
        for (i=0; i<power_off->power_off_interfaces_nr; i++)
        {
            if (MEDIA_TYPE_UNKNOWN != power_off->power_off_interfaces[i].media_type)
            {
                if (
                     0    != power_off->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes_nr  &&
                     NULL != power_off->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes
                   )
                {
                    free_media_specific_blob(power_off->power_off_interfaces[i].generic_phy_common_data.media_specific_bytes);
                }
            }
        }
        free(power_off->power_off_interfaces);
    }
}

// Given a pointer to a preallocated "l2NeighborDeviceTLV" structure, fill it
// with all the pertaining information retrieved from the local device.
//
void _obtainLocalL2NeighborsTLV(struct l2NeighborDeviceTLV *l2_neighbors)
{
    char                  **interfaces_names;
    uint8_t                   interfaces_names_nr;
    uint8_t                   i, j;

    l2_neighbors->tlv.type                = TLV_TYPE_L2_NEIGHBOR_DEVICE;
    l2_neighbors->local_interfaces_nr     = 0;

    interfaces_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&interfaces_names_nr);

    // Search for interfaces in "POWER OFF" mode
    //
    for (i=0; i<interfaces_names_nr; i++)
    {
        struct interfaceInfo *x;

        if (NULL == (x = PLATFORM_GET_1905_INTERFACE_INFO(interfaces_names[i])))
        {
            // Error retrieving information for this interface.
            // Ignore it.
            //
            continue;
        }

        if (0 == x->neighbor_mac_addresses_nr || INTERFACE_NEIGHBORS_UNKNOWN == x->neighbor_mac_addresses_nr)
        {
            // Ignore interfaces that do not have (or cannot report) L2
            // neighbors
            //
            free_1905_INTERFACE_INFO(x);
            continue;
        }

        if (0 == l2_neighbors->local_interfaces_nr)
        {
            l2_neighbors->local_interfaces = (struct _l2InterfacesEntries *)memalloc(sizeof(struct _l2InterfacesEntries));
        }
        else
        {
            l2_neighbors->local_interfaces = (struct _l2InterfacesEntries *)memrealloc(l2_neighbors->local_interfaces, sizeof(struct _l2InterfacesEntries) * (l2_neighbors->local_interfaces_nr + 1));
        }

        memcpy(l2_neighbors->local_interfaces[l2_neighbors->local_interfaces_nr].local_mac_address, x->mac_address, 6);

        l2_neighbors->local_interfaces[l2_neighbors->local_interfaces_nr].l2_neighbors_nr = x->neighbor_mac_addresses_nr;
        l2_neighbors->local_interfaces[l2_neighbors->local_interfaces_nr].l2_neighbors    = (struct _l2NeighborsEntries *)memalloc(sizeof(struct _l2NeighborsEntries) * x->neighbor_mac_addresses_nr);

        for (j=0; j<x->neighbor_mac_addresses_nr; j++)
        {
            memcpy(l2_neighbors->local_interfaces[l2_neighbors->local_interfaces_nr].l2_neighbors[j].l2_neighbor_mac_address, x->neighbor_mac_addresses[j], 6);

            // TODO: Modify "struct interfaceInfo" in "platform_interfaces.h"
            // to provide "behind MACs" information.
            // But first... find out WHAT "behind MACs" really means!!
            //
            l2_neighbors->local_interfaces[l2_neighbors->local_interfaces_nr].l2_neighbors[j].behind_mac_addresses_nr = 0;
            l2_neighbors->local_interfaces[l2_neighbors->local_interfaces_nr].l2_neighbors[j].behind_mac_addresses    = NULL;
        }

        l2_neighbors->local_interfaces_nr++;

        free_1905_INTERFACE_INFO(x);
    }

    free_LIST_OF_1905_INTERFACES(interfaces_names, interfaces_names_nr);
}

// Free the contents of the provided "l2NeighborDeviceTLV" structure (ie.  only
// what was allocated by "_obtainLocalL2NeighborsTLV()", and not the
// "l2NeighborDeviceTLV" structure itself, which is the caller's responsability)
//
void _freeLocalL2NeighborsTLV(struct l2NeighborDeviceTLV *l2_neighbors)
{
    uint8_t i, j;

    if (l2_neighbors->local_interfaces_nr > 0)
    {
        for (i=0; i<l2_neighbors->local_interfaces_nr; i++)
        {
            if (
                 0    != l2_neighbors->local_interfaces[i].l2_neighbors_nr  &&
                 NULL != l2_neighbors->local_interfaces[i].l2_neighbors
               )
            {
                for (j=0; j<l2_neighbors->local_interfaces[i].l2_neighbors_nr; j++)
                {
                    if (
                         0    != l2_neighbors->local_interfaces[i].l2_neighbors[j].behind_mac_addresses_nr  &&
                         NULL != l2_neighbors->local_interfaces[i].l2_neighbors[j].behind_mac_addresses
                       )
                    {
                        free(l2_neighbors->local_interfaces[i].l2_neighbors[j].behind_mac_addresses);
                    }
                }
                free(l2_neighbors->local_interfaces[i].l2_neighbors);
            }
        }
        free(l2_neighbors->local_interfaces);
    }
}

// Given a pointer to a preallocated "alMacAddressTypeTLV" structure, fill it
// with all the pertaining information retrieved from the local device.
//
static struct alMacAddressTypeTLV *_obtainLocalAlMacAddressTLV(dlist_head *parent)
{
    struct alMacAddressTypeTLV *al_mac_tlv =
            X1905_TLV_ALLOC(alMacAddressType, TLV_TYPE_AL_MAC_ADDRESS_TYPE, parent);
    memcpy(al_mac_tlv->al_mac_address, DMalMacGet(), 6);
    return al_mac_tlv;
}

// Return a list of Tx metrics TLV and/or a list of Rx metrics TLV involving
// the local node and the neighbor whose AL MAC address matches
// 'specific_neighbor'.
//
// 'destination' can be either 'LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS' (in which
// case argument 'specific_neighbor' is ignored) or
// 'LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR' (in which case 'specific_neighbor'
// is the AL MAC of the 1905 node at the other end of the link whose metrics
// are being reported.
//
// 'metrics_type' can be 'LINK_METRIC_QUERY_TLV_TX_LINK_METRICS_ONLY',
// 'LINK_METRIC_QUERY_TLV_RX_LINK_METRICS_ONLY' or
// 'LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS'.
//
// 'tx' is an output argument which will be set to NULL if 'metrics_type' is
// set to 'LINK_METRIC_QUERY_TLV_RX_LINK_METRICS_ONLY' or point to a list of
// 1 or more (depending on the value of 'destination') pointers to Tx metrics
// TLVs.
//
// 'rx' is an output argument which will be set to NULL if 'metrics_type' is
// set to 'LINK_METRIC_QUERY_TLV_TX_LINK_METRICS_ONLY' or point to a list of
// 1 or more (depending on the value of 'destination') pointers to Rx metrics
// TLVs.
//
// 'nr' is an output argument set to the number of elements if rx and/or tx.
//
// If there is a problem (example: a specific neighbor was not found), this
// function returns '0', otherwise it returns '1'.
//
void _obtainLocalMetricsTLVs(uint8_t destination, uint8_t *specific_neighbor, uint8_t metrics_type,
                             struct transmitterLinkMetricTLV ***tx,
                             struct receiverLinkMetricTLV    ***rx,
                             uint8_t *nr)
{
    uint8_t (*al_mac_addresses)[6];
    uint8_t   al_mac_addresses_nr;

    struct transmitterLinkMetricTLV   **tx_tlvs;
    struct receiverLinkMetricTLV      **rx_tlvs;

    uint8_t total_tlvs;
    uint8_t i, j;

    al_mac_addresses = DMgetListOfNeighbors(&al_mac_addresses_nr);

    // We will need either 1 or 'al_mac_addresses_nr' Rx and/or Tx TLVs,
    // depending on the value of the 'destination' argument (ie. one Rx and/or
    // Tx TLV for each neighbor whose metrics we are going to report)
    //
    rx_tlvs = NULL;
    tx_tlvs = NULL;
    if (al_mac_addresses_nr > 0)
    {
        if (LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS == destination)
        {
            if (
                 LINK_METRIC_QUERY_TLV_TX_LINK_METRICS_ONLY        == metrics_type ||
                 LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS == metrics_type
               )
            {
                tx_tlvs = (struct transmitterLinkMetricTLV **)memalloc(sizeof(struct transmitterLinkMetricTLV*) * al_mac_addresses_nr);
            }
            if (
                 LINK_METRIC_QUERY_TLV_RX_LINK_METRICS_ONLY        == metrics_type ||
                 LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS == metrics_type
               )
            {
                rx_tlvs = (struct receiverLinkMetricTLV **)memalloc(sizeof(struct receiverLinkMetricTLV*) * al_mac_addresses_nr);
            }
        }
        else
        {
            if (
                 LINK_METRIC_QUERY_TLV_TX_LINK_METRICS_ONLY        == metrics_type ||
                 LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS == metrics_type
               )
            {
                tx_tlvs = (struct transmitterLinkMetricTLV **)memalloc(sizeof(struct transmitterLinkMetricTLV*) * 1);
            }
            if (
                 LINK_METRIC_QUERY_TLV_RX_LINK_METRICS_ONLY        == metrics_type ||
                 LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS == metrics_type
               )
            {
                rx_tlvs = (struct receiverLinkMetricTLV **)memalloc(sizeof(struct receiverLinkMetricTLV*) * 1);
            }
        }
    }

    // Next, for each neighbor, fill the corresponding TLV structure (Rx, Tx or
    // both) that contains the information regarding all possible links that
    // "join" our local node with that neighbor.
    //
    total_tlvs = 0;
    for (i=0; i<al_mac_addresses_nr; i++)
    {
        uint8_t  (*remote_macs)[6];
        char   **local_interfaces;
        uint8_t    links_nr;

        // Check if we are really interested in obtaining metrics information
        // regarding this particular neighbor
        //
        if (
             LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR == destination          &&
             0 != memcmp(al_mac_addresses[i], specific_neighbor, 6)
           )
        {
            // Not interested
            //
            continue;
        }

        // Obtain the list of "links" that connect our AL node with this
        // specific neighbor.
        //
        remote_macs = DMgetListOfLinksWithNeighbor(al_mac_addresses[i], &local_interfaces, &links_nr);

        if (links_nr > 0)
        {

            // If there are 1 or more links between the local node and the
            // neighbor, first fill the TLV "header"
            //
            if (NULL != tx_tlvs)
            {
                tx_tlvs[total_tlvs] = (struct transmitterLinkMetricTLV *)memalloc(sizeof(struct transmitterLinkMetricTLV));

                                tx_tlvs[total_tlvs]->tlv.type                    = TLV_TYPE_TRANSMITTER_LINK_METRIC;
                memcpy(tx_tlvs[total_tlvs]->local_al_address,             DMalMacGet(),                       6);
                memcpy(tx_tlvs[total_tlvs]->neighbor_al_address,          al_mac_addresses[i],                6);
                                tx_tlvs[total_tlvs]->transmitter_link_metrics_nr = links_nr;
                                tx_tlvs[total_tlvs]->transmitter_link_metrics    = memalloc(sizeof(struct _transmitterLinkMetricEntries) * links_nr);
            }
            if (NULL != rx_tlvs)
            {
                rx_tlvs[total_tlvs] = (struct receiverLinkMetricTLV *)memalloc(sizeof(struct receiverLinkMetricTLV));

                                rx_tlvs[total_tlvs]->tlv.type                    = TLV_TYPE_RECEIVER_LINK_METRIC;
                memcpy(rx_tlvs[total_tlvs]->local_al_address,             DMalMacGet(),                       6);
                memcpy(rx_tlvs[total_tlvs]->neighbor_al_address,          al_mac_addresses[i],                6);
                                rx_tlvs[total_tlvs]->receiver_link_metrics_nr    = links_nr;
                                rx_tlvs[total_tlvs]->receiver_link_metrics       = memalloc(sizeof(struct _receiverLinkMetricEntries) * links_nr);
            }

            // ...and then, for each link, fill the specific link information:
            //
            for (j=0; j<links_nr; j++)
            {
                struct interfaceInfo *f;
                struct linkMetrics   *l;

                f = PLATFORM_GET_1905_INTERFACE_INFO(local_interfaces[j]);
                l = PLATFORM_GET_LINK_METRICS(local_interfaces[j], remote_macs[j]);

                if (NULL != tx_tlvs)
                {
                    memcpy(tx_tlvs[total_tlvs]->transmitter_link_metrics[j].local_interface_address,    DMinterfaceNameToMac(local_interfaces[j]), 6);
                    memcpy(tx_tlvs[total_tlvs]->transmitter_link_metrics[j].neighbor_interface_address, remote_macs[j],                            6);

                    if (NULL == f)
                    {
                        tx_tlvs[total_tlvs]->transmitter_link_metrics[j].intf_type = MEDIA_TYPE_UNKNOWN;
                    }
                    else
                    {
                        tx_tlvs[total_tlvs]->transmitter_link_metrics[j].intf_type = f->interface_type;
                    }
                    tx_tlvs[total_tlvs]->transmitter_link_metrics[j].bridge_flag = DMisLinkBridged(local_interfaces[j], al_mac_addresses[i], remote_macs[j]);

                    if (NULL == l)
                    {
                        tx_tlvs[total_tlvs]->transmitter_link_metrics[j].packet_errors           =  0;
                        tx_tlvs[total_tlvs]->transmitter_link_metrics[j].transmitted_packets     =  0;
                        tx_tlvs[total_tlvs]->transmitter_link_metrics[j].mac_throughput_capacity =  0;
                        tx_tlvs[total_tlvs]->transmitter_link_metrics[j].link_availability       =  0;
                        tx_tlvs[total_tlvs]->transmitter_link_metrics[j].phy_rate                =  0;
                    }
                    else
                    {
                        tx_tlvs[total_tlvs]->transmitter_link_metrics[j].packet_errors           =  l->tx_packet_errors;
                        tx_tlvs[total_tlvs]->transmitter_link_metrics[j].transmitted_packets     =  l->tx_packet_ok;
                        tx_tlvs[total_tlvs]->transmitter_link_metrics[j].mac_throughput_capacity =  l->tx_max_xput;
                        tx_tlvs[total_tlvs]->transmitter_link_metrics[j].link_availability       =  l->tx_link_availability;
                        tx_tlvs[total_tlvs]->transmitter_link_metrics[j].phy_rate                =  l->tx_phy_rate;
                    }
                }

                if (NULL != rx_tlvs)
                {
                    memcpy(rx_tlvs[total_tlvs]->receiver_link_metrics[j].local_interface_address,    DMinterfaceNameToMac(local_interfaces[j]), 6);
                    memcpy(rx_tlvs[total_tlvs]->receiver_link_metrics[j].neighbor_interface_address, remote_macs[j],                            6);

                    if (NULL == f)
                    {
                        rx_tlvs[total_tlvs]->receiver_link_metrics[j].intf_type = MEDIA_TYPE_UNKNOWN;
                    }
                    else
                    {
                        rx_tlvs[total_tlvs]->receiver_link_metrics[j].intf_type = f->interface_type;
                    }

                    if (NULL == l)
                    {
                        rx_tlvs[total_tlvs]->receiver_link_metrics[j].packet_errors           =  0;
                        rx_tlvs[total_tlvs]->receiver_link_metrics[j].packets_received        =  0;
                        rx_tlvs[total_tlvs]->receiver_link_metrics[j].rssi                    =  0;
                    }
                    else
                    {
                        rx_tlvs[total_tlvs]->receiver_link_metrics[j].packet_errors           =  l->rx_packet_errors;
                        rx_tlvs[total_tlvs]->receiver_link_metrics[j].packets_received        =  l->rx_packet_ok;
                        rx_tlvs[total_tlvs]->receiver_link_metrics[j].rssi                    =  l->rx_rssi;
                    }
                }

                if (NULL != f)
                {
                    free_1905_INTERFACE_INFO(f);
                }
                if (NULL != l)
                {
                    free_LINK_METRICS(l);
                }
            }

            total_tlvs++;
        }

        DMfreeListOfLinksWithNeighbor(remote_macs, local_interfaces, links_nr);
    }

    free(al_mac_addresses);

    if (
         LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR == destination &&
         total_tlvs == 0
       )
    {
        // Specific neighbor not found
        //
        *nr = 0;
        *tx = NULL;
        *rx = NULL;
    }
    else
    {
        *nr = total_tlvs;
        *tx = tx_tlvs;
        *rx = rx_tlvs;
    }
}

// Free the contents of the pointers filled by a previous call to
// "_obtainLocalMetricsTLVs()".
// This function is called with the same three last arguments as
// "_obtainLocalMetricsTLVs()"
//
void _freeLocalMetricsTLVs(struct transmitterLinkMetricTLV ***tx_tlvs, struct receiverLinkMetricTLV ***rx_tlvs, uint8_t *total_tlvs)
{
    uint8_t i;

    if (NULL != tx_tlvs && NULL != *tx_tlvs)
    {
        for (i=0; i<*total_tlvs; i++)
        {
            if ((*tx_tlvs)[i]->transmitter_link_metrics_nr > 0 && NULL != (*tx_tlvs)[i]->transmitter_link_metrics)
            {
                free((*tx_tlvs)[i]->transmitter_link_metrics);
            }
            free((*tx_tlvs)[i]);
        }
        free(*tx_tlvs);
    }
    if (NULL != rx_tlvs && NULL != *rx_tlvs)
    {
        for (i=0; i<*total_tlvs; i++)
        {
            if ((*rx_tlvs)[i]->receiver_link_metrics_nr > 0 && NULL != (*rx_tlvs)[i]->receiver_link_metrics)
            {
                free((*rx_tlvs)[i]->receiver_link_metrics);
            }
            free((*rx_tlvs)[i]);
        }
        free(*rx_tlvs);
    }
}

// This function is needed to present Tx and Rx TLVs in the way they are
// expected when contained inside an ALME-GET-METRIC.response reply.
//
// Let's explain this in more detail.
//
// Tx and Rx TLVs are "designed" to contain (each one of them) all possible
// links between two AL entities.
//
// In other words, if an AL has 3 neighbors, then 3 Rx (and 3 Tx) TLVs is all
// that is needed to contain all the information we might ever want.
//
// Example:
//
//   In this scenario, "AL 1" just has one neighbor ("AL 2") and thus only one
//   TLV is needed (even if there are *two* possible links)
//
//          eth0 ---------- eth3
//                    |
//     AL 1            -----eth2  AL 2
//
//          eth1 ---------- eth0
//
//   In particular, this unique TLV would be structured like this:
//
//     TLV:
//       - metrics AL1/eth0 <--> AL2/eth3
//       - metrics AL1/eth0 <--> AL2/eth2
//       - metrics AL1/eth1 <--> AL2/eth0
//
// However, when replying to an "ALME-GET-METRIC.response" message, for some
// reason, each Tx/Rx TLV in the list can only contain information for *one*
// local interface.
//
// In other words, in the example above, *even* if just one TLV has the
// capacity of providing information for both interfaces, we would have to
// create *two* TLVs instead, structured like this:
//
//   TLV1:
//     - metrics AL1/eth0 <--> AL2/eth3
//     - metrics AL1/eth0 <--> AL2/eth2
//
//   TLV2:
//     - metrics AL1/eth1 <--> AL2/eth0
//
// This is *obviously* an error in the standard (it causes more memory usage
// and repeated member structures that are not necessary)... but we have to live
// with it.
//
// Anyway... function "_obtainLocalMetricsTLVs()" returns a list of pointers to
// Tx/Rx TLVs where each element contains all the links between two neighbors.
// This function ("_reStructureMetricsTLVs()") takes that output and then
// returns a bigger list where each of the TLVs now only contains information
// regarding one local interface.
//
// The "restructured" returned pointers must be freed with a call to
// "_freeLocalMetricsTLVs()" when they are no longer needed
//
// Note: this function should always return "1". If it ever returns "0" it means
// there is a design error and the function implementation should be reviewed
//
uint8_t _reStructureMetricsTLVs(struct transmitterLinkMetricTLV ***tx,
                              struct receiverLinkMetricTLV   ***rx,
                              uint8_t *nr)
{
    struct transmitterLinkMetricTLV   **tx_tlvs;
    struct receiverLinkMetricTLV      **rx_tlvs;

    struct transmitterLinkMetricTLV   **new_tx_tlvs;
    struct receiverLinkMetricTLV      **new_rx_tlvs;

    uint8_t total_tlvs;
    uint8_t new_total_tlvs_rx;
    uint8_t new_total_tlvs_tx;

    char   **interfaces_names;
    uint8_t    interfaces_names_nr;

    interfaces_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&interfaces_names_nr);

    uint8_t i, j, k;

    tx_tlvs           = *tx;
    rx_tlvs           = *rx;
    total_tlvs        = *nr;

    new_tx_tlvs       = NULL;
    new_rx_tlvs       = NULL;
    new_total_tlvs_rx = 0;
    new_total_tlvs_tx = 0;

    // For each neighbor
    //
    for (i=0; i<total_tlvs; i++)
    {
        // Each "old" TLV (representing a neighbor) will "expand" into as many
        // "new" TLVs as local interfaces can be used to reach that neighbor.
        //
        if (NULL != tx_tlvs)
        {
            // For each local interface
            //
            for (j=0; j<interfaces_names_nr; j++)
            {
                // ...find all TLV metrics associated to this local interface
                //
                for (k=0; k<tx_tlvs[i]->transmitter_link_metrics_nr; k++)
                {
                    if (0 == memcmp(DMinterfaceNameToMac(interfaces_names[j]), tx_tlvs[i]->transmitter_link_metrics[k].local_interface_address, 6))
                    {
                        // ...and add them
                        //
                        if (NULL == new_tx_tlvs)
                        {
                            // ...as NEW TLVs, if this is the first time
                            //
                            new_tx_tlvs    = (struct transmitterLinkMetricTLV **)memalloc(sizeof(struct transmitterLinkMetricTLV*));
                            new_tx_tlvs[0] = (struct transmitterLinkMetricTLV  *)memalloc(sizeof(struct transmitterLinkMetricTLV));

                                            new_tx_tlvs[0]->tlv.type                    = tx_tlvs[i]->tlv.type;
                            memcpy(new_tx_tlvs[0]->local_al_address,             tx_tlvs[i]->local_al_address,    6);
                            memcpy(new_tx_tlvs[0]->neighbor_al_address,          tx_tlvs[i]->neighbor_al_address, 6);
                                            new_tx_tlvs[0]->transmitter_link_metrics_nr = 1;
                                            new_tx_tlvs[0]->transmitter_link_metrics    = (struct _transmitterLinkMetricEntries *)memalloc(sizeof(struct _transmitterLinkMetricEntries));
                            memcpy(new_tx_tlvs[0]->transmitter_link_metrics,     &(tx_tlvs[i]->transmitter_link_metrics[k]), sizeof(struct _transmitterLinkMetricEntries));

                            new_total_tlvs_tx = 1;
                        }
                        else
                        {
                            // ...or as either NEW TLVs or part of a previously
                            // created TLV which is also associated to this same
                            // local interface.
                            //
                            if (
                                 0 == memcmp(new_tx_tlvs[new_total_tlvs_tx-1]->transmitter_link_metrics[0].local_interface_address, tx_tlvs[i]->transmitter_link_metrics[k].local_interface_address, 6) &&
                                 0 == memcmp(new_tx_tlvs[new_total_tlvs_tx-1]->neighbor_al_address,                                 tx_tlvs[i]->neighbor_al_address,                                 6)
                               )
                            {
                                // Part of a previously created one. Append the
                                // metrics info.
                                //
                                                 new_tx_tlvs[new_total_tlvs_tx-1]->transmitter_link_metrics                                                                 = (struct _transmitterLinkMetricEntries *)memrealloc(new_tx_tlvs[new_total_tlvs_tx-1]->transmitter_link_metrics, sizeof(struct _transmitterLinkMetricEntries)*(new_tx_tlvs[new_total_tlvs_tx-1]->transmitter_link_metrics_nr + 1));
                                memcpy(&new_tx_tlvs[new_total_tlvs_tx-1]->transmitter_link_metrics[new_tx_tlvs[new_total_tlvs_tx-1]->transmitter_link_metrics_nr],   &(tx_tlvs[i]->transmitter_link_metrics[k]), sizeof(struct _transmitterLinkMetricEntries));
                                                 new_tx_tlvs[new_total_tlvs_tx-1]->transmitter_link_metrics_nr++;
                            }
                            else
                            {
                                // New interface. Create new TLV.
                                //
                                new_tx_tlvs                    = (struct transmitterLinkMetricTLV **)memrealloc(new_tx_tlvs, sizeof(struct transmitterLinkMetricTLV*)*(new_total_tlvs_tx + 1));
                                new_tx_tlvs[new_total_tlvs_tx] = (struct transmitterLinkMetricTLV  *)memalloc(sizeof(struct transmitterLinkMetricTLV));

                                                new_tx_tlvs[new_total_tlvs_tx]->tlv.type                    = tx_tlvs[i]->tlv.type;
                                memcpy(new_tx_tlvs[new_total_tlvs_tx]->local_al_address,             tx_tlvs[i]->local_al_address,    6);
                                memcpy(new_tx_tlvs[new_total_tlvs_tx]->neighbor_al_address,          tx_tlvs[i]->neighbor_al_address, 6);
                                                new_tx_tlvs[new_total_tlvs_tx]->transmitter_link_metrics_nr = 1;
                                                new_tx_tlvs[new_total_tlvs_tx]->transmitter_link_metrics    = (struct _transmitterLinkMetricEntries *)memalloc(sizeof(struct _transmitterLinkMetricEntries));
                                memcpy(new_tx_tlvs[new_total_tlvs_tx]->transmitter_link_metrics,     &tx_tlvs[i]->transmitter_link_metrics[k], sizeof(struct _transmitterLinkMetricEntries));

                                new_total_tlvs_tx++;
                            }
                        }
                    }
                }
            }
        }

        // Repeat THE SAME for the "rx_tlvs" (this is "semi" copy & paste code,
        // because there are differences in the way structures and members are
        // called)
        //
        if (NULL != rx_tlvs)
        {
            // For each local interface
            //
            for (j=0; j<interfaces_names_nr; j++)
            {
                // ...find all TLV metrics associated to this local interface
                //
                for (k=0; k<rx_tlvs[i]->receiver_link_metrics_nr; k++)
                {
                    if (0 == memcmp(DMinterfaceNameToMac(interfaces_names[j]), rx_tlvs[i]->receiver_link_metrics[k].local_interface_address, 6))
                    {
                        // ...and add them
                        //
                        if (NULL == new_rx_tlvs)
                        {
                            // ...as NEW TLVs, if this is the first time
                            //
                            new_rx_tlvs    = (struct receiverLinkMetricTLV **)memalloc(sizeof(struct receiverLinkMetricTLV*));
                            new_rx_tlvs[0] = (struct receiverLinkMetricTLV  *)memalloc(sizeof(struct receiverLinkMetricTLV));

                                            new_rx_tlvs[0]->tlv.type                    = rx_tlvs[i]->tlv.type;
                            memcpy(new_rx_tlvs[0]->local_al_address,             rx_tlvs[i]->local_al_address,    6);
                            memcpy(new_rx_tlvs[0]->neighbor_al_address,          rx_tlvs[i]->neighbor_al_address, 6);
                                            new_rx_tlvs[0]->receiver_link_metrics_nr    = 1;
                                            new_rx_tlvs[0]->receiver_link_metrics       = (struct _receiverLinkMetricEntries *)memalloc(sizeof(struct _receiverLinkMetricEntries));
                            memcpy(new_rx_tlvs[0]->receiver_link_metrics,        &rx_tlvs[i]->receiver_link_metrics[k], sizeof(struct _receiverLinkMetricEntries));

                            new_total_tlvs_rx = 1;
                        }
                        else
                        {
                            // ...or as either NEW TLVs or part of a previously
                            // created TLV which is also associated to this same
                            // local interface.
                            //
                            if (
                                 0 == memcmp(new_rx_tlvs[new_total_tlvs_rx-1]->receiver_link_metrics[0].local_interface_address, rx_tlvs[i]->receiver_link_metrics[k].local_interface_address, 6) &&
                                 0 == memcmp(new_rx_tlvs[new_total_tlvs_rx-1]->neighbor_al_address,                              rx_tlvs[i]->neighbor_al_address,                              6)
                               )
                            {
                                // Part of a previously created one. Append the
                                // metrics info.
                                //
                                                 new_rx_tlvs[new_total_tlvs_rx-1]->receiver_link_metrics                                                             = (struct _receiverLinkMetricEntries *)memrealloc(new_rx_tlvs[new_total_tlvs_rx-1]->receiver_link_metrics, sizeof(struct _receiverLinkMetricEntries)*(new_rx_tlvs[new_total_tlvs_rx-1]->receiver_link_metrics_nr + 1));
                                memcpy(&new_rx_tlvs[new_total_tlvs_rx-1]->receiver_link_metrics[new_rx_tlvs[new_total_tlvs_rx-1]->receiver_link_metrics_nr],  (&rx_tlvs[i]->receiver_link_metrics[k]), sizeof(struct _receiverLinkMetricEntries));
                                                 new_rx_tlvs[new_total_tlvs_rx-1]->receiver_link_metrics_nr++;
                            }
                            else
                            {
                                // New interface. Create new TLV.
                                //
                                new_rx_tlvs                    = (struct receiverLinkMetricTLV **)memrealloc(new_rx_tlvs, sizeof(struct receiverLinkMetricTLV*)*(new_total_tlvs_rx + 1));
                                new_rx_tlvs[new_total_tlvs_rx] = (struct receiverLinkMetricTLV  *)memalloc(sizeof(struct receiverLinkMetricTLV));

                                                new_rx_tlvs[new_total_tlvs_rx]->tlv.type                    = rx_tlvs[i]->tlv.type;
                                memcpy(new_rx_tlvs[new_total_tlvs_rx]->local_al_address,             rx_tlvs[i]->local_al_address,    6);
                                memcpy(new_rx_tlvs[new_total_tlvs_rx]->neighbor_al_address,          rx_tlvs[i]->neighbor_al_address, 6);
                                                new_rx_tlvs[new_total_tlvs_rx]->receiver_link_metrics_nr    = 1;
                                                new_rx_tlvs[new_total_tlvs_rx]->receiver_link_metrics       = (struct _receiverLinkMetricEntries *)memalloc(sizeof(struct _receiverLinkMetricEntries));
                                memcpy(new_rx_tlvs[new_total_tlvs_rx]->receiver_link_metrics,        &rx_tlvs[i]->receiver_link_metrics[k], sizeof(struct _receiverLinkMetricEntries));

                                new_total_tlvs_rx++;
                            }
                        }
                    }
                }
            }
        }
    }

    free_LIST_OF_1905_INTERFACES(interfaces_names, interfaces_names_nr);

    // Free the old structures
    //
    _freeLocalMetricsTLVs(&tx_tlvs, &rx_tlvs, &total_tlvs);

    if (new_total_tlvs_rx != new_total_tlvs_tx)
    {
        // Something went terribly wrong. This should NEVER happen
        //
        PLATFORM_PRINTF_DEBUG_ERROR("_reStructureMetricsTLVs contains a design error. Review it!\n");
        return 0;
    }

    // And return the new ones
    //
    *tx = new_tx_tlvs;
    *rx = new_rx_tlvs;
    *nr = new_total_tlvs_tx;

    return 1;
}

// Given a pointer to a preallocated "genericPhyDeviceInformationTypeTLV"
// structure, fill it with all the pertaining information retrieved from the
// local device.
//
static struct supportedServiceTLV *_obtainLocalSupportedServicesTLV(dlist_head *parent)
{
    /* Always and agent, controller if registrarIsLocal() is true. */
    return supportedServiceTLVAlloc(parent, registrarIsLocal(), true);
}

// Given a pointer to a preallocated "supportedServiceTLV"
// structure, fill it with all the pertaining information retrieved from the
// local device.
//
static struct apOperationalBssTLV *_obtainLocalApOperationalBssTLV(dlist_head *parent)
{
    char **ifs_names;
    uint8_t  ifs_nr;
    uint8_t  i;

    struct apOperationalBssTLV *tlv = X1905_TLV_ALLOC(apOperationalBss, TLV_TYPE_AP_OPERATIONAL_BSS, parent);

    ifs_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&ifs_nr);

    /* @todo For now, 1 interface == 1 radio == 1 BSS */
    for (i=0; i<ifs_nr; i++)
    {
        struct interfaceInfo *x;

        x = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
        if (NULL == x)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);
        }
        else
        {
            switch (x->interface_type)
            {
                case INTERFACE_TYPE_IEEE_802_11AC_5_GHZ:
                case INTERFACE_TYPE_IEEE_802_11AD_60_GHZ:
                case INTERFACE_TYPE_IEEE_802_11AF_GHZ:
                case INTERFACE_TYPE_IEEE_802_11A_5_GHZ:
                case INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ:
                case INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ:
                case INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ:
                case INTERFACE_TYPE_IEEE_802_11N_5_GHZ:
                    /* Only operational interfaces, i.e. which are AP and have a BSSID */
                    if (x->interface_type_data.ieee80211.role == IEEE80211_ROLE_AP &&
                        memcmp(x->interface_type_data.ieee80211.bssid, "\0\0\0\0\0\0", 6) != 0)
                    {
                        struct _apOperationalBssRadio *radio = apOperationalBssTLVAddRadio(tlv, x->mac_address);
                        struct ssid ssid;
                        ssid.length = strlen(x->interface_type_data.ieee80211.ssid);
                        assert(ssid.length < SSID_MAX_LEN);
                        memcpy(ssid.ssid, x->interface_type_data.ieee80211.ssid, ssid.length);
                        apOperationalBssRadioAddBss(radio, x->interface_type_data.ieee80211.bssid, ssid);
                    }
                    break;
                default:
                    break;
            }
        }
    }
    return tlv;
}

static struct tlv *_obtainLocalRadioBasicCapabilitiesTLV(const struct radio *radio)
{
    struct apRadioBasicCapabilitiesTLV *apRadioBasicCapabilities =
            X1905_TLV_ALLOC(apRadioBasicCapabilities, TLV_TYPE_AP_RADIO_BASIC_CAPABILITIES, NULL);
    apRadioBasicCapabilities->maxbss = (uint8_t) radio->maxBSS;
    memcpy(apRadioBasicCapabilities->radio_uid, radio->uid, 6);
    /* @todo determine classes and inoperable channels */
}

// Given a pointer to a preallocated "genericPhyDeviceInformationTypeTLV"
// structure, fill it with all the pertaining information retrieved from the
// local device.
//
void _obtainLocalGenericPhyTLV(struct genericPhyDeviceInformationTypeTLV *generic_phy)
{
    uint8_t  al_mac_address[6];
    uint8_t  i;

    char   **interfaces_names;
    uint8_t    interfaces_names_nr;

    memcpy(al_mac_address, DMalMacGet(), 6);

    generic_phy->tlv.type          = TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION;
    generic_phy->al_mac_address[0] = al_mac_address[0];
    generic_phy->al_mac_address[1] = al_mac_address[1];
    generic_phy->al_mac_address[2] = al_mac_address[2];
    generic_phy->al_mac_address[3] = al_mac_address[3];
    generic_phy->al_mac_address[4] = al_mac_address[4];
    generic_phy->al_mac_address[5] = al_mac_address[5];

    interfaces_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&interfaces_names_nr);

    generic_phy->local_interfaces_nr = 0;
    generic_phy->local_interfaces    = NULL;

    for (i=0; i<interfaces_names_nr; i++)
    {
        struct interfaceInfo *x;

        if (NULL == (x = PLATFORM_GET_1905_INTERFACE_INFO(interfaces_names[i])))
        {
            // Error retrieving information for this interface.
            // Ignore it.
            //
            PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", interfaces_names[i]);
            continue;
        }

        if (INTERFACE_TYPE_UNKNOWN == x->interface_type)
        {
            // We are only interested in "generic" interfaces

            uint16_t  len;
            uint8_t  *m;

            if (0 == generic_phy->local_interfaces_nr)
            {
                generic_phy->local_interfaces = (struct _genericPhyDeviceEntries *)memalloc(sizeof(struct _genericPhyDeviceEntries));
            }
            else
            {
                generic_phy->local_interfaces = (struct _genericPhyDeviceEntries *)memrealloc(generic_phy->local_interfaces, sizeof(struct _genericPhyDeviceEntries) * (generic_phy->local_interfaces_nr + 1));
            }

            memcpy(generic_phy->local_interfaces[generic_phy->local_interfaces_nr].local_interface_address,                          x->mac_address, 6);
                            generic_phy->local_interfaces[generic_phy->local_interfaces_nr].generic_phy_common_data.oui[0]                  = x->interface_type_data.other.oui[0];
                            generic_phy->local_interfaces[generic_phy->local_interfaces_nr].generic_phy_common_data.oui[1]                  = x->interface_type_data.other.oui[1];
                            generic_phy->local_interfaces[generic_phy->local_interfaces_nr].generic_phy_common_data.oui[2]                  = x->interface_type_data.other.oui[2];
                            generic_phy->local_interfaces[generic_phy->local_interfaces_nr].generic_phy_common_data.variant_index           = x->interface_type_data.other.variant_index;

            m = forge_media_specific_blob(&x->interface_type_data.other, &len);
            if (NULL != m)
            {
                            generic_phy->local_interfaces[generic_phy->local_interfaces_nr].generic_phy_common_data.media_specific_bytes_nr = len;
                            generic_phy->local_interfaces[generic_phy->local_interfaces_nr].generic_phy_common_data.media_specific_bytes    = m;
            }
            else
            {
                            generic_phy->local_interfaces[generic_phy->local_interfaces_nr].generic_phy_common_data.media_specific_bytes_nr = 0;
                            generic_phy->local_interfaces[generic_phy->local_interfaces_nr].generic_phy_common_data.media_specific_bytes    = NULL;
            }

            memcpy(generic_phy->local_interfaces[generic_phy->local_interfaces_nr].variant_name,                                     x->interface_type_data.other.variant_name, strlen(x->interface_type_data.other.variant_name)+1);
                            generic_phy->local_interfaces[generic_phy->local_interfaces_nr].generic_phy_description_xml_url_len             = strlen(x->interface_type_data.other.generic_phy_description_xml_url)+1;

                            generic_phy->local_interfaces[generic_phy->local_interfaces_nr].generic_phy_description_xml_url                 = memalloc(generic_phy->local_interfaces[generic_phy->local_interfaces_nr].generic_phy_description_xml_url_len);
            memcpy(generic_phy->local_interfaces[generic_phy->local_interfaces_nr].generic_phy_description_xml_url,                  x->interface_type_data.other.generic_phy_description_xml_url, generic_phy->local_interfaces[generic_phy->local_interfaces_nr].generic_phy_description_xml_url_len);

            generic_phy->local_interfaces_nr++;
        }
        free_1905_INTERFACE_INFO(x);
    }

    free_LIST_OF_1905_INTERFACES(interfaces_names, interfaces_names_nr);
}

// Free the contents of the provided "genericPhyDeviceInformationTypeTLV"
// structure (ie.  only what was allocated by "_obtainLocalGenericPhyTLV()",
// and not the "genericPhyDeviceInformationTypeTLV" structure itself, which is
// the caller's responsability)
//
void _freeLocalGenericPhyTLV(struct genericPhyDeviceInformationTypeTLV *generic_phy)
{
    uint8_t i;

    if (generic_phy->local_interfaces_nr > 0)
    {
        for (i=0; i<generic_phy->local_interfaces_nr; i++)
        {
            if (
                 0    != generic_phy->local_interfaces[i].generic_phy_common_data.media_specific_bytes_nr  &&
                 NULL != generic_phy->local_interfaces[i].generic_phy_common_data.media_specific_bytes
               )
            {
                free_media_specific_blob(generic_phy->local_interfaces[i].generic_phy_common_data.media_specific_bytes);
            }

            if (
                 0    != generic_phy->local_interfaces[i].generic_phy_description_xml_url_len  &&
                 NULL != generic_phy->local_interfaces[i].generic_phy_description_xml_url
               )
            {
                free(generic_phy->local_interfaces[i].generic_phy_description_xml_url);
            }
        }
        free(generic_phy->local_interfaces);
    }
}

// Given a pointer to a preallocated "x1905ProfileVersionTLV" structure, fill
// it with all the pertaining information retrieved from the local device.
//
void _obtainLocalProfileTLV(struct x1905ProfileVersionTLV *profile)
{
    profile->tlv.type = TLV_TYPE_1905_PROFILE_VERSION;
    profile->profile  = PROFILE_1905_1A;
}

// Free the contents of the provided "x1905ProfileVersionTLV" structure (ie.
// only what was allocated by "_obtainLocalProfileTLV()", and not the
// "x1905ProfileVersionTLV" structure itself, which is the caller's
// responsability)
//
void _freeLocalProfileTLV(__attribute__((unused)) struct x1905ProfileVersionTLV *profile)
{
    // Nothing needs to be freed
}

// Given a pointer to a preallocated "deviceIdentificationTypeTLV" structure,
// fill it with all the pertaining information retrieved from the local device.
//
void _obtainLocalDeviceIdentificationTLV(struct deviceIdentificationTypeTLV *identification)
{
    struct deviceInfo    *x;

    x = PLATFORM_GET_DEVICE_INFO();
    if (NULL == x)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not retrieve device info\n");
        return;
    }

    memset(identification, 0x0, sizeof(struct deviceIdentificationTypeTLV));

    identification->tlv.type = TLV_TYPE_DEVICE_IDENTIFICATION;

    memcpy(identification->friendly_name,      x->friendly_name,      strlen(x->friendly_name)+1);
    memcpy(identification->manufacturer_name,  x->manufacturer_name,  strlen(x->manufacturer_name)+1);
    memcpy(identification->manufacturer_model, x->manufacturer_model, strlen(x->manufacturer_model)+1);
}

// Free the contents of the provided "deviceIdentificationTypeTLV" structure
// (ie.  only what was allocated by "_obtainLocalDeviceIdentificationTLV()",
// and not the "deviceIdentificationTypeTLV" structure itself, which is the
// caller's responsability)
//
void _freeLocalDeviceIdentificationTLV(__attribute__((unused)) struct deviceIdentificationTypeTLV *identification)
{
    // Nothing needs to be freed
}

// Given a pointer to a preallocated "controlUrlTypeTLV" structure, fill
// it with all the pertaining information retrieved from the local device.
//
void _obtainLocalControlUrlTLV(struct controlUrlTypeTLV *control_url)
{
    struct deviceInfo    *x;

    x = PLATFORM_GET_DEVICE_INFO();
    if (NULL == x)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not retrieve device info\n");
        control_url->url  = NULL;
        return;
    }

    control_url->tlv.type = TLV_TYPE_CONTROL_URL;
    if (NULL != x->control_url)
    {
        control_url->url  = strdup(x->control_url);
    }
    else
    {
        control_url->url  = NULL;
    }
}

// Free the contents of the provided "controlUrlTypeTLV" structure (ie.  only
// what was allocated by "_obtainLocalControlUrlTLV()", and not the
// "controlUrlTypeTLV" structure itself, which is the caller's responsability)
//
void _freeLocalControlUrlTLV(__attribute__((unused)) struct controlUrlTypeTLV *control_url)
{
    if (NULL != control_url->url)
    {
        free(control_url->url);
    }
}

// Given two pointers to a preallocated "ipv4TypeTLV" and "ipv6TypeTLV"
// structures, fill them with all the pertaining information retrieved from the
// local device.
//
void _obtainLocalIpsTLVs(struct ipv4TypeTLV *ipv4, struct ipv6TypeTLV *ipv6)
{
    char **ifs_names;
    uint8_t  ifs_nr;

    uint8_t i, j;

    ipv4->tlv.type                 = TLV_TYPE_IPV4;
    ipv4->ipv4_interfaces_nr       = 0;
    ipv6->tlv.type                 = TLV_TYPE_IPV6;
    ipv6->ipv6_interfaces_nr       = 0;

    ifs_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&ifs_nr);

    for (i=0; i<ifs_nr; i++)
    {
        struct interfaceInfo *y;

        y = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
        if (NULL == y)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);
            continue;
        }

        if (0 != y->ipv4_nr)
        {
            if (0 == ipv4->ipv4_interfaces_nr)
            {
                ipv4->ipv4_interfaces = (struct _ipv4InterfaceEntries *)memalloc(sizeof(struct _ipv4InterfaceEntries));
            }
            else
            {
                ipv4->ipv4_interfaces = (struct _ipv4InterfaceEntries *)memrealloc(ipv4->ipv4_interfaces, sizeof(struct _ipv4InterfaceEntries) * (ipv4->ipv4_interfaces_nr + 1));
            }

            memcpy(ipv4->ipv4_interfaces[ipv4->ipv4_interfaces_nr].mac_address,   y->mac_address, 6);
                            ipv4->ipv4_interfaces[ipv4->ipv4_interfaces_nr].ipv4_nr      = y->ipv4_nr;
                            ipv4->ipv4_interfaces[ipv4->ipv4_interfaces_nr].ipv4         = (struct _ipv4Entries *)memalloc(sizeof(struct _ipv4Entries) * y->ipv4_nr);
            for (j=0; j<y->ipv4_nr; j++)
            {
                switch (y->ipv4[j].type)
                {
                    case IPV4_UNKNOWN:
                    {
                        ipv4->ipv4_interfaces[ipv4->ipv4_interfaces_nr].ipv4[j].type = IPV4_TYPE_UNKNOWN;
                        break;
                    }
                    case IPV4_DHCP:
                    {
                        ipv4->ipv4_interfaces[ipv4->ipv4_interfaces_nr].ipv4[j].type = IPV4_TYPE_DHCP;
                        break;
                    }
                    case IPV4_STATIC:
                    {
                        ipv4->ipv4_interfaces[ipv4->ipv4_interfaces_nr].ipv4[j].type = IPV4_TYPE_STATIC;
                        break;
                    }
                    case IPV4_AUTOIP:
                    {
                        ipv4->ipv4_interfaces[ipv4->ipv4_interfaces_nr].ipv4[j].type = IPV4_TYPE_AUTOIP;
                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Invalid IPv4 type %d\n", y->ipv4[j].type);
                        break;
                    }
                }
                memcpy(ipv4->ipv4_interfaces[ipv4->ipv4_interfaces_nr].ipv4[j].ipv4_address,     y->ipv4[j].address,     4);
                memcpy(ipv4->ipv4_interfaces[ipv4->ipv4_interfaces_nr].ipv4[j].ipv4_dhcp_server, y->ipv4[j].dhcp_server, 4);
            }

            ipv4->ipv4_interfaces_nr++;
        }

        if (0 != y->ipv6_nr)
        {
            if (0 == ipv6->ipv6_interfaces_nr)
            {
                ipv6->ipv6_interfaces = (struct _ipv6InterfaceEntries *)memalloc(sizeof(struct _ipv6InterfaceEntries));
            }
            else
            {
                ipv6->ipv6_interfaces = (struct _ipv6InterfaceEntries *)memrealloc(ipv6->ipv6_interfaces, sizeof(struct _ipv6InterfaceEntries) * (ipv6->ipv6_interfaces_nr + 1));
            }

            memcpy(ipv6->ipv6_interfaces[ipv6->ipv6_interfaces_nr].mac_address,   y->mac_address, 6);
                            ipv6->ipv6_interfaces[ipv6->ipv6_interfaces_nr].ipv6_nr      = y->ipv6_nr;
                            ipv6->ipv6_interfaces[ipv6->ipv6_interfaces_nr].ipv6         = (struct _ipv6Entries *)memalloc(sizeof(struct _ipv6Entries) * y->ipv6_nr);
            for (j=0; j<y->ipv6_nr; j++)
            {
                switch (y->ipv6[j].type)
                {
                    case IPV6_UNKNOWN:
                    {
                        ipv6->ipv6_interfaces[ipv6->ipv6_interfaces_nr].ipv6[j].type = IPV6_TYPE_UNKNOWN;
                        break;
                    }
                    case IPV6_DHCP:
                    {
                        ipv6->ipv6_interfaces[ipv6->ipv6_interfaces_nr].ipv6[j].type = IPV6_TYPE_DHCP;
                        break;
                    }
                    case IPV6_STATIC:
                    {
                        ipv6->ipv6_interfaces[ipv6->ipv6_interfaces_nr].ipv6[j].type = IPV6_TYPE_STATIC;
                        break;
                    }
                    case IPV6_SLAAC:
                    {
                        ipv6->ipv6_interfaces[ipv6->ipv6_interfaces_nr].ipv6[j].type = IPV6_TYPE_SLAAC;
                        break;
                    }
                    default:
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("Invalid IPv6 type %d\n", y->ipv6[j].type);
                        break;
                    }
                }
                memcpy(ipv6->ipv6_interfaces[ipv6->ipv6_interfaces_nr].ipv6[j].ipv6_address,        y->ipv6[j].address,  16);
                memcpy(ipv6->ipv6_interfaces[ipv6->ipv6_interfaces_nr].ipv6[j].ipv6_address_origin, y->ipv6[j].origin,   16);
            }

            ipv6->ipv6_interfaces_nr++;
        }

        free_1905_INTERFACE_INFO(y);
    }

    free_LIST_OF_1905_INTERFACES(ifs_names, ifs_nr);
}

// Free the contents of the provided "ipv4TypeTLV" and "ipv6TypeTLV" structures
// (ie.  only what was allocated by "_obtainLocalIpsTLVs()", and not the
// "ipv4TypeTLV"/"ipv6TypeTLV" structures themselved, which is the caller's
// responsability)
//
void _freeLocalIpsTLVs(struct ipv4TypeTLV *ipv4, struct ipv6TypeTLV *ipv6)
{
    uint8_t i;

    if (0 != ipv4->ipv4_interfaces_nr && NULL != ipv4->ipv4_interfaces)
    {
        for (i=0; i<ipv4->ipv4_interfaces_nr; i++)
        {
            if (0 != ipv4->ipv4_interfaces[i].ipv4_nr && NULL != ipv4->ipv4_interfaces[i].ipv4)
            {
                free(ipv4->ipv4_interfaces[i].ipv4);
            }
        }
        free(ipv4->ipv4_interfaces);
    }
    if (0 != ipv6->ipv6_interfaces_nr && NULL != ipv6->ipv6_interfaces)
    {
        for (i=0; i<ipv6->ipv6_interfaces_nr; i++)
        {
            if (0 != ipv6->ipv6_interfaces[i].ipv6_nr && NULL != ipv6->ipv6_interfaces[i].ipv6)
            {
                free(ipv6->ipv6_interfaces[i].ipv6);
            }
        }
        free(ipv6->ipv6_interfaces);
    }
}

//******************************************************************************
//******* "Buffer writer" stuff (read below) ***********************************
//******************************************************************************
//
// The following "buffer writer" related variables and functions are used to
// "trick" the "DMdumpNetworkDevices()" function to print to a memory buffer
// instead of to a file descriptor (ex: STDOUT)
//
//   TODO: Review this mechanism so that such a huge malloc is not needed.
//         Because the information contained in this buffer is meant to be sent
//         through a TCP socket, maybe we could allocate small chunks and keep
//         sending them through the socket... however this would require several
//         changes in the way things operate now...
//         Think about it (and, who knows... maybe we decide to leave it as it
//         is now after all).
//
#define MEMORY_BUFFER_SIZE (63*1024)
char   *memory_buffer     = NULL;
uint16_t  memory_buffer_i   = 0;

void _memoryBufferWriterInit()
{
    memory_buffer     = (char *)memalloc(MEMORY_BUFFER_SIZE);
    memory_buffer_i   = 0;
}
void _memoryBufferWriter(const char *fmt, ...)
{
    va_list arglist;

    if (memory_buffer_i >= MEMORY_BUFFER_SIZE-1)
    {
        // Too big...
        //
        PLATFORM_PRINTF_DEBUG_WARNING("Memory buffer overflow.\n");
        return;
    }

    va_start(arglist, fmt);
    vsnprintf(memory_buffer + memory_buffer_i, MEMORY_BUFFER_SIZE - memory_buffer_i, fmt, arglist);
    va_end(arglist);

    memory_buffer[MEMORY_BUFFER_SIZE-1] = 0x0;
    memory_buffer_i = strlen(memory_buffer);
    memory_buffer[memory_buffer_i] = 0x0;

    return;
}
void _memoryBufferWriterEnd()
{
    if (NULL != memory_buffer)
    {
        free(memory_buffer);

        memory_buffer_i   = 0;
    }
    return;
}

//******************************************************************************
//******* Local device data dump ***********************************************
//******************************************************************************
//
// This function is used to update the entry of the database associated to the
// local node.
//
// Let me explain this in more detail: the database contains information of
// all nodes (the local and remote ones):
//
//   - For remote nodes, every time a response CMDU is received, the TLVs
//     contained in that CMDU are added to the entry of the database associated
//     to that remote node (or updated, if they already existed)
//
//   - For the local node, however, we must "manually" force an "update" so that
//     the database entry associated to the local node contains updated
//     information. *This* is exactly what this function does.
//
// When should we call this function? Well... we are only interested in updating
// this local entry when someone is going to look at it which, as of today,
// only happens when a special ("custom") ALME is received
// ("CUSTOM_COMMAND_DUMP_NETWORK_DEVICES") and, as a result, we must send the
// local information as part of the response.
//
void _updateLocalDeviceData()
{
    struct deviceInformationTypeTLV            *info;
    struct deviceBridgingCapabilityTLV        **bridges;
    struct non1905NeighborDeviceListTLV       **non1905_neighbors; uint8_t non1905_neighbors_nr;
    struct neighborDeviceListTLV              **x1905_neighbors;   uint8_t x1905_neighbors_nr;
    struct powerOffInterfaceTLV               **power_off;
    struct l2NeighborDeviceTLV                **l2_neighbors;
    struct supportedServiceTLV                 *supported_service_tlv;
    struct genericPhyDeviceInformationTypeTLV  *generic_phy;
    struct x1905ProfileVersionTLV              *profile;
    struct deviceIdentificationTypeTLV         *identification;
    struct controlUrlTypeTLV                   *control_url;
    struct ipv4TypeTLV                         *ipv4;
    struct ipv6TypeTLV                         *ipv6;

    struct transmitterLinkMetricTLV           **tx_tlvs;
    struct receiverLinkMetricTLV              **rx_tlvs;
    uint8_t                                       total_metrics_tlvs;
    uint8_t                                       i;

    struct vendorSpecificTLV                  **extensions;        uint8_t extensions_nr;

    // We need to allocate these structures in the heap (instead of simply
    // declaring variables in the stack) because they are going to be "saved"
    // in the database when calling "DMupdate*()"
    //
    info            = (struct deviceInformationTypeTLV*)          memalloc(sizeof(struct deviceInformationTypeTLV));
    bridges         = (struct deviceBridgingCapabilityTLV**)      memalloc(sizeof(struct deviceBridgingCapabilityTLV*));
    bridges[0]      = (struct deviceBridgingCapabilityTLV*)       memalloc(sizeof(struct deviceBridgingCapabilityTLV));
    power_off       = (struct powerOffInterfaceTLV**)             memalloc(sizeof(struct powerOffInterfaceTLV*));
    power_off[0]    = (struct powerOffInterfaceTLV*)              memalloc(sizeof(struct powerOffInterfaceTLV));
    l2_neighbors    = (struct l2NeighborDeviceTLV**)              memalloc(sizeof(struct l2NeighborDeviceTLV*));
    l2_neighbors[0] = (struct l2NeighborDeviceTLV*)               memalloc(sizeof(struct l2NeighborDeviceTLV));
    generic_phy     = (struct genericPhyDeviceInformationTypeTLV*)memalloc(sizeof(struct genericPhyDeviceInformationTypeTLV));
    profile         = (struct x1905ProfileVersionTLV*)            memalloc(sizeof(struct x1905ProfileVersionTLV));
    identification  = (struct deviceIdentificationTypeTLV*)       memalloc(sizeof(struct deviceIdentificationTypeTLV));
    control_url     = (struct controlUrlTypeTLV*)                 memalloc(sizeof(struct controlUrlTypeTLV));
    ipv4            = (struct ipv4TypeTLV*)                       memalloc(sizeof(struct ipv4TypeTLV));
    ipv6            = (struct ipv6TypeTLV*)                       memalloc(sizeof(struct ipv6TypeTLV));

    _obtainLocalDeviceInfoTLV           (info);
    _obtainLocalBridgingCapabilitiesTLV (bridges[0]);
    _obtainLocalNeighborsTLV            (&non1905_neighbors, &non1905_neighbors_nr, &x1905_neighbors, &x1905_neighbors_nr);
    _obtainLocalPowerOffInterfacesTLV   (power_off[0]);
    _obtainLocalL2NeighborsTLV          (l2_neighbors[0]);
    supported_service_tlv = _obtainLocalSupportedServicesTLV    (NULL);
    _obtainLocalGenericPhyTLV           (generic_phy);
    _obtainLocalProfileTLV              (profile);
    _obtainLocalDeviceIdentificationTLV (identification);
    _obtainLocalControlUrlTLV           (control_url);
    _obtainLocalIpsTLVs                 (ipv4, ipv6);

    _obtainLocalMetricsTLVs             (LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS,
                                         NULL,
                                         LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS,
                                         &tx_tlvs,
                                         &rx_tlvs,
                                         &total_metrics_tlvs);

    // Retrieve extra (non-standard) local info by third-party implementations
    // (i.e. BBF obtains non-1905 link metrics info)
    //
    obtainExtendedLocalInfo(&extensions, &extensions_nr);

    // The following function will take care of "freeing" the allocated memory
    // if needed
    //
    DMupdateNetworkDeviceInfo(info->al_mac_address,
                              1, info,
                              1, bridges,           1,
                              1, non1905_neighbors, non1905_neighbors_nr,
                              1, x1905_neighbors,   x1905_neighbors_nr,
                              1, power_off,         1,
                              1, l2_neighbors,      1,
                              1, supported_service_tlv,
                              1, generic_phy,
                              1, profile,
                              1, identification,
                              1, control_url,
                              1, ipv4,
                              1, ipv6);

    // The next function, however, takes care only of the pointers to metrics
    // information, and not of the memory used to hold the list of pointers
    // themselves...
    //
    for (i=0; i<total_metrics_tlvs; i++)
    {
        DMupdateNetworkDeviceMetrics((uint8_t *)tx_tlvs[i]);
        DMupdateNetworkDeviceMetrics((uint8_t *)rx_tlvs[i]);
    }

    // ... and that's why we need to do this (the "0" is so that pointers
    // themselves are not freed, as they are now responsibility of the
    // database)
    //
    total_metrics_tlvs = 0;
    _freeLocalMetricsTLVs(&tx_tlvs, &rx_tlvs, &total_metrics_tlvs);

    // Update the datamodel with the extended info (Vendor Specific TLVs).
    // The next function, however, takes care only of the pointers to the TLVs,
    // and not of the memory used to hold the list of pointers themselves...
    //
    updateExtendedInfo(extensions, extensions_nr,  DMalMacGet());

    // ... and that's why we need to release here the memory used to hold the
    // list of TLV pointers. TLV pointers themselves are now responsibility of
    // the database. The "0" is so that pointers to the TLVs are not freed.
    //
    extensions_nr = 0;
    freeExtendedLocalInfo(&extensions, &extensions_nr);
}

////////////////////////////////////////////////////////////////////////////////
// Public functions (exported only to files in this same folder)
////////////////////////////////////////////////////////////////////////////////

uint8_t send1905RawPacket(const char *interface_name, uint16_t mid, const uint8_t *dst_mac_address, struct CMDU *cmdu)
{
    uint8_t  **streams;
    uint16_t  *streams_lens;

    uint8_t total_streams, x;

    // Insert protocol extensions to the CMDU, which has been already built at
    // this point.
    //
    send1905CmduExtensions(cmdu);

    PLATFORM_PRINTF_DEBUG_DETAIL("Contents of CMDU to send:\n");
    visit_1905_CMDU_structure(cmdu, print_callback, PLATFORM_PRINTF_DEBUG_DETAIL, "");

    streams = forge_1905_CMDU_from_structure(cmdu, &streams_lens);
    if (NULL == streams)
    {
        // Could not forge the packet. Error?
        //
        PLATFORM_PRINTF_DEBUG_WARNING("forge_1905_CMDU_from_structure() failed!\n");
        return 0;
    }

    // Free previously allocated CMDU extensions (no longer needed)
    //
    free1905CmduExtensions(cmdu);

    total_streams = 0;
    while(streams[total_streams])
    {
        total_streams++;
    }

    if (0 == total_streams)
    {
        // Could not forge the packet. Error?
        //
        PLATFORM_PRINTF_DEBUG_WARNING("forge_1905_CMDU_from_structure() returned 0 streams!\n");

        free_1905_CMDU_packets(streams);
        free(streams_lens);
        return 0;
    }

    x = 0;
    while(streams[x])
    {
        PLATFORM_PRINTF_DEBUG_DETAIL("Sending 1905 message on interface %s, MID %d, fragment %d/%d\n", interface_name, mid, x+1, total_streams);
        if (0 == PLATFORM_SEND_RAW_PACKET(interface_name,
                                          dst_mac_address,
                                          DMalMacGet(),
                                          ETHERTYPE_1905,
                                          streams[x],
                                          streams_lens[x]))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("Packet could not be sent!\n");
        }

        x++;
    }

    free_1905_CMDU_packets(streams);
    free(streams_lens);

    return 1;
}

uint8_t send1905RawALME(uint8_t alme_client_id, uint8_t *alme)
{
    uint8_t    *packet_out;
    uint16_t    packet_out_len;

    PLATFORM_PRINTF_DEBUG_DETAIL("Contents of ALME reply to send:\n");
    visit_1905_ALME_structure((uint8_t *)alme, print_callback, PLATFORM_PRINTF_DEBUG_DETAIL, "");

    // Use the getIntfListResponseALME structure to forge the packet
    // bit stream
    //
    packet_out = forge_1905_ALME_from_structure((uint8_t *)alme, &packet_out_len);
    if (NULL == packet_out)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("forge_1905_ALME_from_structure() failed.\n");

        PLATFORM_SEND_ALME_REPLY(alme_client_id, NULL, 0);
        return 0;
    }

    // Send the ALME reply back
    //
    PLATFORM_SEND_ALME_REPLY(alme_client_id, packet_out, packet_out_len);

    free_1905_ALME_packet(packet_out);

    return 1;
}

uint8_t send1905TopologyDiscoveryPacket(char *interface_name, uint16_t mid)
{
    // The "topology discovery" message is a CMDU with two TLVs:
    //   - One AL MAC address type TLV
    //   - One MAC address type TLV

    uint8_t  interface_mac_address[6];

    uint8_t  mcast_address[] = MCAST_1905;

    struct CMDU                discovery_message;
    struct alMacAddressTypeTLV *al_mac_addr_tlv;
    struct macAddressTypeTLV   *mac_addr_tlv = X1905_TLV_ALLOC(macAddressType, TLV_TYPE_MAC_ADDRESS_TYPE, NULL);

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_TOPOLOGY_DISCOVERY (%s)\n", interface_name);

    memcpy(interface_mac_address, DMinterfaceNameToMac(interface_name), 6);

    // Fill the AL MAC address type TLV
    //
    al_mac_addr_tlv = _obtainLocalAlMacAddressTLV(NULL);

    // Fill the MAC address type TLV
    //
    memcpy(mac_addr_tlv->mac_address, interface_mac_address, 6);

    // Build the CMDU
    //
    discovery_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    discovery_message.message_type    = CMDU_TYPE_TOPOLOGY_DISCOVERY;
    discovery_message.message_id      = mid;
    discovery_message.relay_indicator = 0;
    discovery_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *)*3);
    discovery_message.list_of_TLVs[0] = &al_mac_addr_tlv->tlv;
    discovery_message.list_of_TLVs[1] = &mac_addr_tlv->tlv;
    discovery_message.list_of_TLVs[2] = NULL;

    // Send the packet
    //
    if (0 == send1905RawPacket(interface_name, mid, mcast_address, &discovery_message))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not send the 1905 packet\n");
        free(discovery_message.list_of_TLVs);
        return 0;
    }

    free(discovery_message.list_of_TLVs);
    return 1;
}

uint8_t send1905TopologyQueryPacket(char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address)
{
    // The "topology query" message is a CMDU with no TLVs

    uint8_t ret;

    struct CMDU  query_message;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_TOPOLOGY_QUERY (%s)\n", interface_name);
    PLATFORM_PRINTF_DEBUG_DETAIL("Sending to %02x:%02x:%02x:%02x:%02x:%02x\n", destination_al_mac_address[0], destination_al_mac_address[1], destination_al_mac_address[2], destination_al_mac_address[3], destination_al_mac_address[4], destination_al_mac_address[5]);

    // Build the CMDU
    //
    query_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    query_message.message_type    = CMDU_TYPE_TOPOLOGY_QUERY;
    query_message.message_id      = mid;
    query_message.relay_indicator = 0;
    query_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *));
    query_message.list_of_TLVs[0] = NULL;

    // Send the packet
    //
    if (0 == send1905RawPacket(interface_name, mid, destination_al_mac_address, &query_message))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Could not send packet\n");
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    // Free memory
    //
    free(query_message.list_of_TLVs);

    return ret;
}

uint8_t send1905TopologyResponsePacket(char *interface_name, uint16_t mid, uint8_t* destination_al_mac_address)
{
    // The "topology response" message is a CMDU with the following TLVs:
    //   - One device information type TLV
    //   - Zero or one device bridging capability TLVs
    //   - Zero or more non-1905 neighbor device list TLVs
    //   - Zero or more 1905 neighbor device list TLVs
    //   - Zero or more power off interface TLVs
    //   - Zero or more L2 neighbor device TLVs
    //
    // The "Multi-AP Specification Version 1.0" adds the following TLVs:
    //   - Zero or one supported service TLV
    //   - One AP Operational BSS TLV
    //   - Zero or one Associated Clients TLV
    //
    //   NOTE: The "non-1905 neighbor" and the "L2 neighbor" device TLVs are
    //   kind of overlaping... but this is what the standard says.
    //
    //   NOTE: Regarding the "device bridging capability", "power off interface"
    //   and "L2 neighbor device" TLVs, the standard says "zero or more" but
    //   it should be "zero or one", as one single TLV of these types can carry
    //   many entries.
    //   That's why in this implementation we are just sending zero or one (no
    //   more!) TLVs of these type. However, in reception (see
    //   "process1905Cmdu()") we will be ready to receive more.
    //
    //   NOTE: Since a compliant implementation should ignore unknown TLVs, we can simply always send the Multi-AP
    //   TLVs

    uint8_t  ret;

    struct CMDU                            response_message;
    struct deviceInformationTypeTLV        device_info;
    struct deviceBridgingCapabilityTLV     bridge_info;
    struct non1905NeighborDeviceListTLV  **non_1905_neighbors;
    struct neighborDeviceListTLV         **neighbors;
    struct powerOffInterfaceTLV            power_off;
    struct l2NeighborDeviceTLV             l2_neighbors;
    struct supportedServiceTLV            *supported_service_tlv;
    struct apOperationalBssTLV            *ap_operational_bss_tlv;

    uint8_t                                 non_1905_neighbors_nr;
    uint8_t                                 neighbors_nr;

    uint8_t                                 total_tlvs            = 0;
    uint8_t                                 i, j;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_TOPOLOGY_RESPONSE (%s)\n", interface_name);
    PLATFORM_PRINTF_DEBUG_DETAIL("Sending to %02x:%02x:%02x:%02x:%02x:%02x\n", destination_al_mac_address[0], destination_al_mac_address[1], destination_al_mac_address[2], destination_al_mac_address[3], destination_al_mac_address[4], destination_al_mac_address[5]);

    // Fill all the needed TLVs
    //
    _obtainLocalDeviceInfoTLV          (&device_info);
    _obtainLocalBridgingCapabilitiesTLV(&bridge_info);
    _obtainLocalNeighborsTLV           (&non_1905_neighbors, &non_1905_neighbors_nr, &neighbors, &neighbors_nr);
    _obtainLocalPowerOffInterfacesTLV  (&power_off);
    _obtainLocalL2NeighborsTLV         (&l2_neighbors);

    // Build the CMDU
    //
    total_tlvs = 1;                      // Device information type TLV

#ifndef SEND_EMPTY_TLVS
    if (bridge_info.bridging_tuples_nr != 0)
#endif
    {
        total_tlvs++;                    // Device bridging capability TLV
    }

    total_tlvs += neighbors_nr;          // Non-1905 neighbor device list TLVs
    total_tlvs += non_1905_neighbors_nr; // 1905 Neighbor device list TLVs

#ifndef SEND_EMPTY_TLVS
    if (power_off.power_off_interfaces_nr != 0)
#endif
    {
        total_tlvs++;                    // Power off interface TLV
    }

#ifndef SEND_EMPTY_TLVS
    if (l2_neighbors.local_interfaces_nr != 0)
#endif
    {
        total_tlvs++;                    // L2 neighbor device TLV
    }

    supported_service_tlv = _obtainLocalSupportedServicesTLV(NULL);
    total_tlvs++;
    ap_operational_bss_tlv = _obtainLocalApOperationalBssTLV(NULL);
    total_tlvs++;

    response_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    response_message.message_type    = CMDU_TYPE_TOPOLOGY_RESPONSE;
    response_message.message_id      = mid;
    response_message.relay_indicator = 0;
    response_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *)*(total_tlvs+1));
    response_message.list_of_TLVs[0] = &device_info.tlv;

    i = 1;
#ifndef SEND_EMPTY_TLVS
    if (bridge_info.bridging_tuples_nr != 0)
#endif
    {
        response_message.list_of_TLVs[i++] = &bridge_info.tlv;
    }

    for (j=0; j<non_1905_neighbors_nr; j++)
    {
        response_message.list_of_TLVs[i++] = &non_1905_neighbors[j]->tlv;
    }

    for (j=0; j<neighbors_nr; j++)
    {
        response_message.list_of_TLVs[i++] = &neighbors[j]->tlv;
    }

#ifndef SEND_EMPTY_TLVS
    if (power_off.power_off_interfaces_nr != 0)
#endif
    {
        response_message.list_of_TLVs[i++] = &power_off.tlv;
    }

#ifndef SEND_EMPTY_TLVS
    if (l2_neighbors.local_interfaces_nr != 0)
#endif
    {
        response_message.list_of_TLVs[i++] = &l2_neighbors.tlv;
    }

    response_message.list_of_TLVs[i++] = &supported_service_tlv->tlv;
    response_message.list_of_TLVs[i++] = &ap_operational_bss_tlv->tlv;

    response_message.list_of_TLVs[i] = NULL;

    // Send the packet
    //
    if (0 == send1905RawPacket(interface_name, mid, destination_al_mac_address, &response_message))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Could not send packet\n");
        ret = 0 ;
    }
    else
    {
        ret = 1;
    }

    // Free all allocated (and no longer needed) memory
    //
    _freeLocalDeviceInfoTLV          (&device_info);
    _freeLocalBridgingCapabilitiesTLV(&bridge_info);
    _freeLocalNeighborsTLV           (&non_1905_neighbors, &non_1905_neighbors_nr, &neighbors, &neighbors_nr);
    _freeLocalPowerOffInterfacesTLV  (&power_off);
    _freeLocalL2NeighborsTLV         (&l2_neighbors);
    /** @todo free supported services */
    /** @todo free ap_operational_bss_tlv */

    free(response_message.list_of_TLVs);

    return ret;
}

uint8_t send1905TopologyNotificationPacket(char *interface_name, uint16_t mid)
{
    // The "topology discovery" message is a CMDU with one TLVs:
    //   - One AL MAC address type TLV

    uint8_t  ret;

    uint8_t  mcast_address[] = MCAST_1905;

    struct CMDU                discovery_message;
    struct alMacAddressTypeTLV *al_mac_addr_tlv;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_TOPOLOGY_NOTIFICATION (%s)\n", interface_name);

    // Fill all the needed TLVs
    //
    al_mac_addr_tlv = _obtainLocalAlMacAddressTLV(NULL);

    // Build the CMDU
    //
    discovery_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    discovery_message.message_type    = CMDU_TYPE_TOPOLOGY_NOTIFICATION;
    discovery_message.message_id      = mid;
    discovery_message.relay_indicator = 0;
    discovery_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *)*3);
    discovery_message.list_of_TLVs[0] = &al_mac_addr_tlv->tlv;
    discovery_message.list_of_TLVs[1] = NULL;

    // Send the packet
    //
    if (0 == send1905RawPacket(interface_name, mid, mcast_address, &discovery_message))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not send the 1905 packet\n");
        free(discovery_message.list_of_TLVs);
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    free(discovery_message.list_of_TLVs);

    return ret;
}

uint8_t send1905MetricsQueryPacket(char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address)
{
    // The "metrics query" message is a CMDU with one TLV:
    //   - One link metric query TLV

    uint8_t ret;

    struct CMDU                query_message;
    struct linkMetricQueryTLV  *metric_query_tlv;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_LINK_METRIC_QUERY (%s)\n", interface_name);
    PLATFORM_PRINTF_DEBUG_DETAIL("Sending to %02x:%02x:%02x:%02x:%02x:%02x\n", destination_al_mac_address[0], destination_al_mac_address[1], destination_al_mac_address[2], destination_al_mac_address[3], destination_al_mac_address[4], destination_al_mac_address[5]);

    // Fill all the needed TLVs
    //
    metric_query_tlv = linkMetricQueryTLVAllocAll(NULL, LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS);

    // Build the CMDU
    //
    query_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    query_message.message_type    = CMDU_TYPE_LINK_METRIC_QUERY;
    query_message.message_id      = mid;
    query_message.relay_indicator = 0;
    query_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *)*2);
    query_message.list_of_TLVs[0] = &metric_query_tlv->tlv;
    query_message.list_of_TLVs[1] = NULL;

    if (0 == send1905RawPacket(interface_name, mid, destination_al_mac_address, &query_message))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Could not send packet\n");
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    // Free all allocated (and no longer needed) memory
    //
    free(query_message.list_of_TLVs);

    return ret;
}

uint8_t send1905MetricsResponsePacket(char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address, uint8_t destination, uint8_t *specific_neighbor, uint8_t metrics_type)
{
    // The "metrics response" message can be either:
    //
    //   A) A CMDU containing either:
    //      - One Tx link metrics
    //      - One Rx link metrics
    //      - One Rx and one Tx link metrics
    //      ... containing info regarding the link between the current node
    //      and the AL entity whose AL MAC is 'specific_neighbor'
    //
    //   B) A CMDU made by concatenating many CMDUs of "type A" (one for each of
    //      its 1905 neighbors).
    //
    // Case (A) happens when
    //
    //   'destination' == LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR
    //
    // ...while case (B) takes place when
    //
    //   'destination' == LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS
    //
    uint8_t   ret;

    struct CMDU                        response_message;
    struct transmitterLinkMetricTLV  **tx_tlvs;
    struct receiverLinkMetricTLV     **rx_tlvs;

    uint8_t total_tlvs;
    uint8_t i, j;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_LINK_METRIC_RESPONSE (%s)\n", interface_name);
    PLATFORM_PRINTF_DEBUG_DETAIL("Sending to %02x:%02x:%02x:%02x:%02x:%02x\n", destination_al_mac_address[0], destination_al_mac_address[1], destination_al_mac_address[2], destination_al_mac_address[3], destination_al_mac_address[4], destination_al_mac_address[5]);

    // Fill all the needed TLVs
    //
    _obtainLocalMetricsTLVs(destination, specific_neighbor, metrics_type,
                            &tx_tlvs, &rx_tlvs, &total_tlvs);

    // Build the CMDU
    //
    response_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    response_message.message_type    = CMDU_TYPE_LINK_METRIC_RESPONSE;
    response_message.message_id      = mid;
    response_message.relay_indicator = 0;

    if (NULL == tx_tlvs || NULL == rx_tlvs)
    {
        response_message.list_of_TLVs = (struct tlv **)memalloc(sizeof(struct tlv *)*(total_tlvs+1));
    }
    else
    {
        response_message.list_of_TLVs = (struct tlv **)memalloc(sizeof(struct tlv *)*((2*total_tlvs) + 1));
    }

    j = 0;
    if (NULL != tx_tlvs)
    {
        for (i=0; i<total_tlvs; i++)
        {
            response_message.list_of_TLVs[j++] = &tx_tlvs[i]->tlv;
        }
    }
    if (NULL != rx_tlvs)
    {
        for (i=0; i<total_tlvs; i++)
        {
            response_message.list_of_TLVs[j++] = &rx_tlvs[i]->tlv;
        }
    }

    response_message.list_of_TLVs[j++] = NULL;

    // Send the packet
    //
    if (0 == send1905RawPacket(interface_name, mid, destination_al_mac_address, &response_message))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not allocate memory for TLV buffer\n");
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    // Free all allocated (and no longer needed) memory
    //
    _freeLocalMetricsTLVs(&tx_tlvs, &rx_tlvs, &total_tlvs);

    free(response_message.list_of_TLVs);

    return ret;
}

uint8_t send1905PushButtonEventNotificationPacket(char *interface_name, uint16_t mid, char **all_interfaces_names, uint8_t *push_button_mask, uint8_t nr)
{
    // The "push button event notification" message is a CMDU with two TLVs:
    //   - One AL MAC address type TLV
    //   - One push button event notification TLV
    //   - Zero or one push button generic phy event notification

    uint8_t ret;

    uint8_t  mcast_address[] = MCAST_1905;

    uint8_t  media_types_nr;
    uint8_t  generic_media_types_nr;
    uint8_t  i, j;

    struct CMDU                                       notification_message;
    struct pushButtonEventNotificationTLV             pb_event_tlv;
    struct pushButtonGenericPhyEventNotificationTLV   pbg_event_tlv;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION (%s)\n", interface_name);

    // Fill the push button event notification TLV
    //
    media_types_nr         = 0;
    generic_media_types_nr = 0;
    for (i=0; i<nr; i++)
    {
        if (0 == push_button_mask[i])
        {
            media_types_nr++;
        }
    }

    pb_event_tlv.tlv.type       = TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION;
    pb_event_tlv.media_types_nr = media_types_nr;

    if (media_types_nr > 0)
    {
        pb_event_tlv.media_types = (struct _mediaTypeEntries *)memalloc(sizeof(struct _mediaTypeEntries) * media_types_nr);
    }
    else
    {
        pb_event_tlv.media_types = NULL;
    }

    j=0;
    for (i=0; i<nr; i++)
    {
        if (0 == push_button_mask[i])
        {
            struct interfaceInfo *x;

            x = PLATFORM_GET_1905_INTERFACE_INFO(all_interfaces_names[i]);
            if (NULL == x)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", all_interfaces_names[i]);

                pb_event_tlv.media_types[j].media_type               = MEDIA_TYPE_UNKNOWN;
                pb_event_tlv.media_types[j].media_specific_data_size = 0;
            }
            else
            {
                 // "Translate" from "INTERFACE_TYPE_*" to "MEDIA_TYPE_*"
                 //
                switch(x->interface_type)
                {
                    case INTERFACE_TYPE_IEEE_802_3U_FAST_ETHERNET:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_IEEE_802_3U_FAST_ETHERNET;
                        break;
                    }
                    case INTERFACE_TYPE_IEEE_802_3AB_GIGABIT_ETHERNET:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_IEEE_802_3AB_GIGABIT_ETHERNET;
                        break;
                    }
                    case INTERFACE_TYPE_IEEE_802_11B_2_4_GHZ:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_IEEE_802_11B_2_4_GHZ;
                        break;
                    }
                    case INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_IEEE_802_11G_2_4_GHZ;
                        break;
                    }
                    case INTERFACE_TYPE_IEEE_802_11A_5_GHZ:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_IEEE_802_11A_5_GHZ;
                        break;
                    }
                    case INTERFACE_TYPE_IEEE_802_11N_2_4_GHZ:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_IEEE_802_11N_2_4_GHZ;
                        break;
                    }
                    case INTERFACE_TYPE_IEEE_802_11N_5_GHZ:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_IEEE_802_11N_5_GHZ;
                        break;
                    }
                    case INTERFACE_TYPE_IEEE_802_11AC_5_GHZ:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_IEEE_802_11AC_5_GHZ;
                        break;
                    }
                    case INTERFACE_TYPE_IEEE_802_11AD_60_GHZ:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_IEEE_802_11AD_60_GHZ;
                        break;
                    }
                    case INTERFACE_TYPE_IEEE_802_11AF_GHZ:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_IEEE_802_11AF_GHZ;
                        break;
                    }
                    case INTERFACE_TYPE_IEEE_1901_WAVELET:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_IEEE_1901_WAVELET;
                        break;
                    }
                    case INTERFACE_TYPE_IEEE_1901_FFT:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_IEEE_1901_FFT;
                        break;
                    }
                    case INTERFACE_TYPE_MOCA_V1_1:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_MOCA_V1_1;
                        break;
                    }
                    case INTERFACE_TYPE_UNKNOWN:
                    {
                        pb_event_tlv.media_types[j].media_type = MEDIA_TYPE_UNKNOWN;
                        break;
                    }
                }

                // Fill the rest of media specific fields
                //
                switch(pb_event_tlv.media_types[j].media_type)
                {
                    case MEDIA_TYPE_IEEE_802_3U_FAST_ETHERNET:
                    case MEDIA_TYPE_IEEE_802_3AB_GIGABIT_ETHERNET:
                    case MEDIA_TYPE_MOCA_V1_1:
                    {
                        // These interfaces don't require extra data
                        //
                        pb_event_tlv.media_types[j].media_specific_data_size = 0;

                        break;
                    }
                    case MEDIA_TYPE_IEEE_802_11B_2_4_GHZ:
                    case MEDIA_TYPE_IEEE_802_11G_2_4_GHZ:
                    case MEDIA_TYPE_IEEE_802_11A_5_GHZ:
                    case MEDIA_TYPE_IEEE_802_11N_2_4_GHZ:
                    case MEDIA_TYPE_IEEE_802_11N_5_GHZ:
                    case MEDIA_TYPE_IEEE_802_11AC_5_GHZ:
                    case MEDIA_TYPE_IEEE_802_11AD_60_GHZ:
                    case MEDIA_TYPE_IEEE_802_11AF_GHZ:
                    {
                        pb_event_tlv.media_types[j].media_specific_data_size                                          = 10;
                        pb_event_tlv.media_types[j].media_specific_data.ieee80211.network_membership[0]               = x->interface_type_data.ieee80211.bssid[0];
                        pb_event_tlv.media_types[j].media_specific_data.ieee80211.network_membership[1]               = x->interface_type_data.ieee80211.bssid[1];
                        pb_event_tlv.media_types[j].media_specific_data.ieee80211.network_membership[2]               = x->interface_type_data.ieee80211.bssid[2];
                        pb_event_tlv.media_types[j].media_specific_data.ieee80211.network_membership[3]               = x->interface_type_data.ieee80211.bssid[3];
                        pb_event_tlv.media_types[j].media_specific_data.ieee80211.network_membership[4]               = x->interface_type_data.ieee80211.bssid[4];
                        pb_event_tlv.media_types[j].media_specific_data.ieee80211.network_membership[5]               = x->interface_type_data.ieee80211.bssid[5];
                        pb_event_tlv.media_types[j].media_specific_data.ieee80211.role                                = x->interface_type_data.ieee80211.role;
                        pb_event_tlv.media_types[j].media_specific_data.ieee80211.ap_channel_band                     = x->interface_type_data.ieee80211.ap_channel_band;
                        pb_event_tlv.media_types[j].media_specific_data.ieee80211.ap_channel_center_frequency_index_1 = x->interface_type_data.ieee80211.ap_channel_center_frequency_index_1;
                        pb_event_tlv.media_types[j].media_specific_data.ieee80211.ap_channel_center_frequency_index_2 = x->interface_type_data.ieee80211.ap_channel_center_frequency_index_2;

                        break;
                    }
                    case MEDIA_TYPE_IEEE_1901_WAVELET:
                    case MEDIA_TYPE_IEEE_1901_FFT:
                    {
                        pb_event_tlv.media_types[j].media_specific_data_size = 7;
                        pb_event_tlv.media_types[j].media_specific_data.ieee1901.network_identifier[0] = x->interface_type_data.ieee1901.network_identifier[0];
                        pb_event_tlv.media_types[j].media_specific_data.ieee1901.network_identifier[1] = x->interface_type_data.ieee1901.network_identifier[1];
                        pb_event_tlv.media_types[j].media_specific_data.ieee1901.network_identifier[2] = x->interface_type_data.ieee1901.network_identifier[2];
                        pb_event_tlv.media_types[j].media_specific_data.ieee1901.network_identifier[3] = x->interface_type_data.ieee1901.network_identifier[3];
                        pb_event_tlv.media_types[j].media_specific_data.ieee1901.network_identifier[4] = x->interface_type_data.ieee1901.network_identifier[4];
                        pb_event_tlv.media_types[j].media_specific_data.ieee1901.network_identifier[5] = x->interface_type_data.ieee1901.network_identifier[5];
                        pb_event_tlv.media_types[j].media_specific_data.ieee1901.network_identifier[6] = x->interface_type_data.ieee1901.network_identifier[6];

                        break;
                    }
                    case MEDIA_TYPE_UNKNOWN:
                    {
                        // Do not include extra data here. It will be included
                        // in the accompanying "push button generic phy
                        // notification TVL"
                        //
                        generic_media_types_nr++;
                        pb_event_tlv.media_types[j].media_specific_data_size = 0;

                        break;
                    }
                }
            }
            j++;

            if (NULL != x)
            {
                free_1905_INTERFACE_INFO(x);
            }
        }
    }

    // Fill the push button generic event notification TLV
    //
    pbg_event_tlv.tlv.type            = TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION;
    pbg_event_tlv.local_interfaces_nr = generic_media_types_nr;

    if (0 == generic_media_types_nr)
    {
        pbg_event_tlv.local_interfaces = NULL;
    }
    else
    {
        pbg_event_tlv.local_interfaces = (struct _genericPhyCommonData *)memalloc(sizeof(struct _genericPhyCommonData) * generic_media_types_nr);

        j=0;
        for (i=0; i<nr; i++)
        {
            if (0 == push_button_mask[i])
            {
                struct interfaceInfo        *x;

                x = PLATFORM_GET_1905_INTERFACE_INFO(all_interfaces_names[i]);
                if (NULL == x)
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", all_interfaces_names[i]);

                    continue;
                }

                if (INTERFACE_TYPE_UNKNOWN == x->interface_type)
                {
                    // We only care about "generic" interfaces

                    uint16_t len;
                    uint8_t  *m;

                    pbg_event_tlv.local_interfaces[j].oui[0]        = x->interface_type_data.other.oui[0];
                    pbg_event_tlv.local_interfaces[j].oui[1]        = x->interface_type_data.other.oui[1];
                    pbg_event_tlv.local_interfaces[j].oui[2]        = x->interface_type_data.other.oui[2];
                    pbg_event_tlv.local_interfaces[j].variant_index = x->interface_type_data.other.variant_index;

                    m = forge_media_specific_blob(&x->interface_type_data.other, &len);

                    if (NULL != m)
                    {
                        pbg_event_tlv.local_interfaces[j].media_specific_bytes_nr = len;
                        pbg_event_tlv.local_interfaces[j].media_specific_bytes    = m;
                    }
                    else
                    {
                        // Ignore media specific data
                        //
                        pbg_event_tlv.local_interfaces[j].media_specific_bytes_nr = 0;
                        pbg_event_tlv.local_interfaces[j].media_specific_bytes    = NULL;
                    }

                    j++;
                }
                free_1905_INTERFACE_INFO(x);
            }
        }
    }

    // Build the CMDU
    //
    notification_message.message_version     = CMDU_MESSAGE_VERSION_1905_1_2013;
    notification_message.message_type        = CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION;
    notification_message.message_id          = mid;
    notification_message.relay_indicator     = 1;

    if (generic_media_types_nr != 0)
    {
        notification_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *)*4);
        notification_message.list_of_TLVs[2] = &pbg_event_tlv.tlv;
        notification_message.list_of_TLVs[3] = NULL;
    }
    else
    {
        notification_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *)*3);
        notification_message.list_of_TLVs[2] = NULL;
    }

    notification_message.list_of_TLVs[0] = &_obtainLocalAlMacAddressTLV(NULL)->tlv;
    notification_message.list_of_TLVs[1] = &pb_event_tlv.tlv;

    // Send the packet
    //
    if (0 == send1905RawPacket(interface_name, mid, mcast_address, &notification_message))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not send the 1905 packet\n");
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    if (media_types_nr > 0)
    {
        free(pb_event_tlv.media_types);
    }
    if (generic_media_types_nr > 0)
    {
        for (i=0; i<generic_media_types_nr; i++)
        {
            if (NULL != pbg_event_tlv.local_interfaces[i].media_specific_bytes)
            {
                free_media_specific_blob(pbg_event_tlv.local_interfaces[i].media_specific_bytes);
            }
        }
        free(pbg_event_tlv.local_interfaces);
    }

    free(notification_message.list_of_TLVs);

    return ret;
}

uint8_t send1905PushButtonJoinNotificationPacket(char *interface_name, uint16_t mid, uint8_t *original_al_mac_address, uint16_t original_mid, uint8_t *local_mac_address, uint8_t *new_mac_address)
{
    // The "push button join notification" message is a CMDU with two TLVs:
    //   - One AL MAC address type TLV
    //   - One push button join notification TLV

    uint8_t ret;

    uint8_t  al_mac_address[6];
    uint8_t  mcast_address[] = MCAST_1905;

    struct CMDU                             notification_message;
    struct pushButtonJoinNotificationTLV    pb_join_tlv;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION (%s)\n", interface_name);

    memcpy(al_mac_address, DMalMacGet(), 6);

    // Fill the push button join notification TLV
    //
    pb_join_tlv.tlv.type           = TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION;
    pb_join_tlv.al_mac_address[0]  = original_al_mac_address[0];
    pb_join_tlv.al_mac_address[1]  = original_al_mac_address[1];
    pb_join_tlv.al_mac_address[2]  = original_al_mac_address[2];
    pb_join_tlv.al_mac_address[3]  = original_al_mac_address[3];
    pb_join_tlv.al_mac_address[4]  = original_al_mac_address[4];
    pb_join_tlv.al_mac_address[5]  = original_al_mac_address[5];
    pb_join_tlv.message_identifier = original_mid;
    pb_join_tlv.mac_address[0]     = local_mac_address[0];
    pb_join_tlv.mac_address[1]     = local_mac_address[1];
    pb_join_tlv.mac_address[2]     = local_mac_address[2];
    pb_join_tlv.mac_address[3]     = local_mac_address[3];
    pb_join_tlv.mac_address[4]     = local_mac_address[4];
    pb_join_tlv.mac_address[5]     = local_mac_address[5];
    pb_join_tlv.new_mac_address[0] = local_mac_address[0];
    pb_join_tlv.new_mac_address[1] = new_mac_address[1];
    pb_join_tlv.new_mac_address[2] = new_mac_address[2];
    pb_join_tlv.new_mac_address[3] = new_mac_address[3];
    pb_join_tlv.new_mac_address[4] = new_mac_address[4];
    pb_join_tlv.new_mac_address[5] = new_mac_address[5];

    // Build the CMDU
    //
    notification_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    notification_message.message_type    = CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION;
    notification_message.message_id      = mid;
    notification_message.relay_indicator = 1;
    notification_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *)*3);
    notification_message.list_of_TLVs[0] = &_obtainLocalAlMacAddressTLV(NULL)->tlv;
    notification_message.list_of_TLVs[1] = &pb_join_tlv.tlv;
    notification_message.list_of_TLVs[2] = NULL;

    // Send the packet
    //
    if (0 == send1905RawPacket(interface_name, mid, mcast_address, &notification_message))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not send the 1905 packet\n");
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    free(notification_message.list_of_TLVs);

    return ret;
}

uint8_t send1905APAutoconfigurationSearchPacket(char *interface_name, uint16_t mid, uint8_t freq_band)
{
    // The "AP-autoconfiguration search" message is a CMDU with three TLVs:
    //   - One AL MAC address type TLV
    //   - One searched role TLV
    //   - One autoconfig freq band TLV

    uint8_t ret;

    uint8_t  mcast_address[] = MCAST_1905;

    struct CMDU                   search_message;
    struct searchedRoleTLV        searched_role_tlv;
    struct autoconfigFreqBandTLV  ac_freq_band_tlv;
    struct supportedServiceTLV    *supported_service_tlv;
    struct supportedServiceTLV    *searched_service_tlv;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH (%s)\n", interface_name);

    // Fill the searched role TLV
    //
    searched_role_tlv.tlv.type = TLV_TYPE_SEARCHED_ROLE;
    searched_role_tlv.role     = IEEE80211_ROLE_AP;

    // Fill the autoconfig freq band TLV
    //
    ac_freq_band_tlv.tlv.type  = TLV_TYPE_AUTOCONFIG_FREQ_BAND;
    ac_freq_band_tlv.freq_band = freq_band;

    supported_service_tlv = _obtainLocalSupportedServicesTLV(NULL);

    // Fill the searched service TLV.
    //
    searched_service_tlv = searchedServiceTLVAlloc(NULL, true);

    // Build the CMDU
    //
    search_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    search_message.message_type    = CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH;
    search_message.message_id      = mid;
    search_message.relay_indicator = 1;
    search_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *)*6);
    search_message.list_of_TLVs[0] = &_obtainLocalAlMacAddressTLV(NULL)->tlv;
    search_message.list_of_TLVs[1] = &searched_role_tlv.tlv;
    search_message.list_of_TLVs[2] = &ac_freq_band_tlv.tlv;
    search_message.list_of_TLVs[3] = &supported_service_tlv->tlv;
    search_message.list_of_TLVs[4] = &searched_service_tlv->tlv;
    search_message.list_of_TLVs[5] = NULL;

    if (0 == send1905RawPacket(interface_name, mid, mcast_address, &search_message))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Could not send packet\n");
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    // Free memory
    //
    /** @todo free supported services */

    free(search_message.list_of_TLVs);

    return ret;
}

uint8_t send1905APAutoconfigurationResponsePacket(char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address,
                                                uint8_t freq_band, bool include_easymesh)
{
    // The "AP-autoconfiguration response" message is a CMDU with two TLVs:
    //   - One supported role TLV
    //   - One supported freq band TLV

    uint8_t ret;

    struct CMDU                  response_message;
    struct supportedRoleTLV      supported_role_tlv;
    struct supportedFreqBandTLV  supported_freq_band_tlv;
    struct supportedServiceTLV   *supported_service_tlv;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE (%s)\n", interface_name);

    // Fill the supported role TLV
    //
    supported_role_tlv.tlv.type = TLV_TYPE_SUPPORTED_ROLE;
    supported_role_tlv.role     = IEEE80211_ROLE_AP;

    // Fill the supported freq band TLV
    //
    supported_freq_band_tlv.tlv.type  = TLV_TYPE_SUPPORTED_FREQ_BAND;
    supported_freq_band_tlv.freq_band = freq_band;

    supported_service_tlv = _obtainLocalSupportedServicesTLV(NULL);

    // Build the CMDU
    //
    response_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    response_message.message_type    = CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE;
    response_message.message_id      = mid;
    response_message.relay_indicator = 0;
    response_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *)*4);
    response_message.list_of_TLVs[0] = &supported_role_tlv.tlv;
    response_message.list_of_TLVs[1] = &supported_freq_band_tlv.tlv;
    if (include_easymesh)
    {
        response_message.list_of_TLVs[2] = &supported_service_tlv->tlv;
        response_message.list_of_TLVs[3] = NULL;
    }
    else
    {
        response_message.list_of_TLVs[2] = NULL;
    }

    if (0 == send1905RawPacket(interface_name, mid, destination_al_mac_address, &response_message))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Could not send packet\n");
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    // Free memory
    //
    /** @todo free supported services */
    free(response_message.list_of_TLVs);

    return ret;
}

uint8_t send1905APAutoconfigurationWSCPacket(const char *interface_name, uint16_t mid, const uint8_t *destination_al_mac_address,
                                             const uint8_t *wsc_frame, uint16_t wsc_frame_size,
                                             const struct radio *radio, bool send_radio_basic_capabilities,
                                             const mac_address radio_uid, bool send_radio_identifier)
{
    // The "AP-autoconfiguration WSC" message is a CMDU with TLVs:
    //   - One or more WSC TLVs (one per SSID to configure)
    //   - In the Multi-AP case, a AP Radio Basic Capabilities in M1 and AP Radio Identifier in M2.
    // @todo support sending multiple WSC TLVs.

    uint8_t ret;

    struct CMDU     data_message;
    struct wscTLV   wsc_tlv;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_AP_AUTOCONFIGURATION_WSC (%s)\n", interface_name);

    // Fill the WSC TLV
    //
    wsc_tlv.tlv.type       = TLV_TYPE_WSC;
    wsc_tlv.wsc_frame_size = wsc_frame_size;
    wsc_tlv.wsc_frame      = (uint8_t *)memalloc(wsc_frame_size);
    memcpy(wsc_tlv.wsc_frame, wsc_frame, wsc_frame_size);

    // Build the CMDU
    //
    data_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    data_message.message_type    = CMDU_TYPE_AP_AUTOCONFIGURATION_WSC;
    data_message.message_id      = mid;
    data_message.relay_indicator = 0;
    data_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *)*3);
    data_message.list_of_TLVs[0] = &wsc_tlv.tlv;
    data_message.list_of_TLVs[1] = NULL;
    data_message.list_of_TLVs[2] = NULL;

    if (send_radio_basic_capabilities)
    {
        data_message.list_of_TLVs[1] = _obtainLocalRadioBasicCapabilitiesTLV(radio);
    }
    else if (send_radio_identifier)
    {
        struct apRadioIdentifierTLV *apRadioIdentifier =
                X1905_TLV_ALLOC(apRadioIdentifier, TLV_TYPE_AP_RADIO_IDENTIFIER, NULL);

        memcpy(&apRadioIdentifier->radio_uid, radio_uid, sizeof(mac_address));
        data_message.list_of_TLVs[1] = &apRadioIdentifier->tlv;
    }

    if (0 == send1905RawPacket(interface_name, mid, destination_al_mac_address, &data_message))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Could not send packet\n");
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    // Free memory
    //
    free(wsc_tlv.wsc_frame);
    free(data_message.list_of_TLVs);

    return ret;
}

uint8_t send1905GenericPhyQueryPacket(char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address)
{
    // The "generic phy query" message is a CMDU with no TLVs

    uint8_t ret;

    struct CMDU  query_message;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_GENERIC_PHY_QUERY (%s)\n", interface_name);
    PLATFORM_PRINTF_DEBUG_DETAIL("Sending to %02x:%02x:%02x:%02x:%02x:%02x\n", destination_al_mac_address[0], destination_al_mac_address[1], destination_al_mac_address[2], destination_al_mac_address[3], destination_al_mac_address[4], destination_al_mac_address[5]);

    // Build the CMDU
    //
    query_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    query_message.message_type    = CMDU_TYPE_GENERIC_PHY_QUERY;
    query_message.message_id      = mid;
    query_message.relay_indicator = 0;
    query_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *));
    query_message.list_of_TLVs[0] = NULL;

    // Send packet
    //
    if (0 == send1905RawPacket(interface_name, mid, destination_al_mac_address, &query_message))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Could not send packet\n");
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    // Free memory
    //
    free(query_message.list_of_TLVs);

    return ret;
}

uint8_t send1905GenericPhyResponsePacket(char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address)
{
    // The "generic phy response" message is a CMDU with the following TLVs:
    //   - One generic phy device information type TLV

    uint8_t  ret;

    struct CMDU                                response_message;
    struct genericPhyDeviceInformationTypeTLV  generic_phy;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_GENERIC_PHY_RESPONSE (%s)\n", interface_name);
    PLATFORM_PRINTF_DEBUG_DETAIL("Sending to %02x:%02x:%02x:%02x:%02x:%02x\n", destination_al_mac_address[0], destination_al_mac_address[1], destination_al_mac_address[2], destination_al_mac_address[3], destination_al_mac_address[4], destination_al_mac_address[5]);

    // Fill all the needed TLVs
    //
    _obtainLocalGenericPhyTLV(&generic_phy);

    // Build the CMDU
    //
    response_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    response_message.message_type    = CMDU_TYPE_GENERIC_PHY_RESPONSE;
    response_message.message_id      = mid;
    response_message.relay_indicator = 0;
    response_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *)*2);
    response_message.list_of_TLVs[0] = &generic_phy.tlv;
    response_message.list_of_TLVs[1] = NULL;

    // Send the packet
    //
    if (0 == send1905RawPacket(interface_name, mid, destination_al_mac_address, &response_message))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Could not send packet\n");
        ret = 0 ;
    }
    else
    {
        ret = 1;
    }

    // Free all allocated (and no longer needed) memory
    //
    _freeLocalGenericPhyTLV(&generic_phy);

    free(response_message.list_of_TLVs);

    return ret;
}

uint8_t send1905HighLayerQueryPacket(char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address)
{
    // The "high level phy query" message is a CMDU with no TLVs

    uint8_t ret;

    struct CMDU  query_message;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_HIGHER_LAYER_QUERY (%s)\n", interface_name);
    PLATFORM_PRINTF_DEBUG_DETAIL("Sending to %02x:%02x:%02x:%02x:%02x:%02x\n", destination_al_mac_address[0], destination_al_mac_address[1], destination_al_mac_address[2], destination_al_mac_address[3], destination_al_mac_address[4], destination_al_mac_address[5]);

    // Build the CMDU
    //
    query_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    query_message.message_type    = CMDU_TYPE_HIGHER_LAYER_QUERY;
    query_message.message_id      = mid;
    query_message.relay_indicator = 0;
    query_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *));
    query_message.list_of_TLVs[0] = NULL;

    // Send the packet
    //
    if (0 == send1905RawPacket(interface_name, mid, destination_al_mac_address, &query_message))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Could not send packet\n");
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    // Free memory
    //
    free(query_message.list_of_TLVs);

    return ret;
}

uint8_t send1905HighLayerResponsePacket(char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address)
{
    // The "high layer response" message is a CMDU with the following TLVs:
    //   - One AL MAC address type TLV
    //   - One 1905 profile version TLV
    //   - One device identification type TLV
    //   - Zero or one control URL type TLV
    //   - Zero or one IPv4 type TLV
    //   - Zero or one IPv6 type TLV

    uint8_t total_tlvs;
    uint8_t i;

    struct CMDU                         response_message;
    struct x1905ProfileVersionTLV       profile_tlv;
    struct deviceIdentificationTypeTLV  identification_tlv;
    struct controlUrlTypeTLV            control_tlv;
    struct ipv4TypeTLV                  ipv4_tlv;
    struct ipv6TypeTLV                  ipv6_tlv;

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_HIGHER_LAYER_RESPONSE (%s)\n", interface_name);

    _obtainLocalProfileTLV              (&profile_tlv);
    _obtainLocalDeviceIdentificationTLV (&identification_tlv);
    _obtainLocalControlUrlTLV           (&control_tlv);
    _obtainLocalIpsTLVs                 (&ipv4_tlv, &ipv6_tlv);

    // Build the CMDU
    //
    total_tlvs = 3; // AL MAC, profile and identification

    if (NULL != control_tlv.url)
    {
        total_tlvs++;
    }
#ifndef SEND_EMPTY_TLVS
    if (0 != ipv4_tlv.ipv4_interfaces_nr)
#endif
    {
        total_tlvs++;
    }
#ifndef SEND_EMPTY_TLVS
    if (0 != ipv6_tlv.ipv6_interfaces_nr)
#endif
    {
        total_tlvs++;
    }

    response_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    response_message.message_type    = CMDU_TYPE_HIGHER_LAYER_RESPONSE;
    response_message.message_id      = mid;
    response_message.relay_indicator = 0;
    response_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *)*(total_tlvs+1));
    response_message.list_of_TLVs[0] = &_obtainLocalAlMacAddressTLV(NULL)->tlv;
    response_message.list_of_TLVs[1] = &profile_tlv.tlv;
    response_message.list_of_TLVs[2] = &identification_tlv.tlv;

    i = 3;
    if (NULL != control_tlv.url)
    {
        response_message.list_of_TLVs[i++] = &control_tlv.tlv;
    }
#ifndef SEND_EMPTY_TLVS
    if (0 != ipv4_tlv.ipv4_interfaces_nr)
#endif
    {
        response_message.list_of_TLVs[i++] = &ipv4_tlv.tlv;
    }
#ifndef SEND_EMPTY_TLVS
    if (0 != ipv6_tlv.ipv6_interfaces_nr)
#endif
    {
        response_message.list_of_TLVs[i++] = &ipv6_tlv.tlv;
    }

    response_message.list_of_TLVs[i++] = NULL;

    // Send the packet
    //
    if (0 == send1905RawPacket(interface_name, mid, destination_al_mac_address, &response_message))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not send packet\n");
        free(response_message.list_of_TLVs);
        return 0;
    }

    _freeLocalProfileTLV              (&profile_tlv);
    _freeLocalDeviceIdentificationTLV (&identification_tlv);
    _freeLocalControlUrlTLV           (&control_tlv);
    _freeLocalIpsTLVs                 (&ipv4_tlv, &ipv6_tlv);

    free(response_message.list_of_TLVs);

    return 1;
}

uint8_t send1905InterfacePowerChangeRequestPacket(char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address, uint8_t (*remote_interfaces)[6], uint8_t *new_states, uint8_t nr)
{
    // NOTE: Right now this function is *not* being used from anywhere. The
    //       reason is that the standard does not say under which circumstances
    //       this packet should be generated.
    //       There should probably exist an HLE primitive that triggers this
    //       event, but there isn't.
    //       I'll just let this function here implemented for the future.

    // The "high layer response" message is a CMDU with the following TLVs:
    //   - One or more interface power change information type TLVs
    //
    // However, it doesn't really make sense to send more than one (after all,
    // one single TLV can contain information regarding as many remote
    // interfaces as desired). The original wording is probably an error in the
    // standard and it should read like this:
    //   - One interface power change information type TLV
    //
    // So... here we are only going to send *one* TLV containing all the remote
    // interfaces requested new states

    uint8_t ret;

    struct CMDU                                request_message;
    struct interfacePowerChangeInformationTLV  power_change;

    uint8_t i;

    if (0 == nr)
    {
        return 1;
    }

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_INTERFACE_POWER_CHANGE_REQUEST (%s)\n", interface_name);
    PLATFORM_PRINTF_DEBUG_DETAIL("Sending to %02x:%02x:%02x:%02x:%02x:%02x\n", destination_al_mac_address[0], destination_al_mac_address[1], destination_al_mac_address[2], destination_al_mac_address[3], destination_al_mac_address[4], destination_al_mac_address[5]);

    // Fill the interface power change information type TLV
    //
    power_change.tlv.type                   = TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION;
    power_change.power_change_interfaces_nr = nr;
    power_change.power_change_interfaces    = (struct _powerChangeInformationEntries *)memalloc(sizeof(struct _powerChangeInformationEntries)*nr);

    for (i=0; i<nr; i++)
    {
        memcpy(power_change.power_change_interfaces[i].interface_address,        remote_interfaces[i], 6);
                        power_change.power_change_interfaces[i].requested_power_state  =  new_states[i];
    }

    // Build the CMDU
    //
    request_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    request_message.message_type    = CMDU_TYPE_INTERFACE_POWER_CHANGE_REQUEST;
    request_message.message_id      = mid;
    request_message.relay_indicator = 0;
    request_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *) * 2);
    request_message.list_of_TLVs[0] = &power_change.tlv;
    request_message.list_of_TLVs[1] = NULL;

    // Send the packet
    //
    if (0 == send1905RawPacket(interface_name, mid, destination_al_mac_address, &request_message))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Could not send packet\n");
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    // Free no longer needed memory
    //
    free(power_change.power_change_interfaces);

    free(request_message.list_of_TLVs);

    return ret;
}

uint8_t send1905InterfacePowerChangeResponsePacket(char *interface_name, uint16_t mid, uint8_t *destination_al_mac_address, uint8_t (*local_interfaces)[6], uint8_t *results, uint8_t nr)
{
    // The "high layer response" message is a CMDU with the following TLVs:
    //   - One or more interface power change status TLVs
    //
    // However, it doesn't really make sense to send more than one (after all,
    // one single TLV can contain information regarding as many remote
    // interfaces as desired). The original wording is probably an error in the
    // standard and it should read like this:
    //   - One interface power change status TLV
    //
    // So... here we are only going to send *one* TLV containing all the local
    // interfaces requested status

    uint8_t ret;

    struct CMDU                           response_message;
    struct interfacePowerChangeStatusTLV  power_change;

    uint8_t i;

    if (0 == nr)
    {
        return 1;
    }

    PLATFORM_PRINTF_DEBUG_INFO("--> CMDU_TYPE_INTERFACE_POWER_CHANGE_response (%s)\n", interface_name);
    PLATFORM_PRINTF_DEBUG_DETAIL("Sending to %02x:%02x:%02x:%02x:%02x:%02x\n", destination_al_mac_address[0], destination_al_mac_address[1], destination_al_mac_address[2], destination_al_mac_address[3], destination_al_mac_address[4], destination_al_mac_address[5]);

    // Fill the interface power change information type TLV
    //
    power_change.tlv.type                   = TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION;
    power_change.power_change_interfaces_nr = nr;
    power_change.power_change_interfaces    = (struct _powerChangeStatusEntries *)memalloc(sizeof(struct _powerChangeStatusEntries)*nr);

    for (i=0; i<nr; i++)
    {
        memcpy(power_change.power_change_interfaces[i].interface_address,     local_interfaces[i], 6);
                        power_change.power_change_interfaces[i].result              =  results[i];
    }

    // Build the CMDU
    //
    response_message.message_version = CMDU_MESSAGE_VERSION_1905_1_2013;
    response_message.message_type    = CMDU_TYPE_INTERFACE_POWER_CHANGE_RESPONSE;
    response_message.message_id      = mid;
    response_message.relay_indicator = 0;
    response_message.list_of_TLVs    = (struct tlv **)memalloc(sizeof(struct tlv *) * 2);
    response_message.list_of_TLVs[0] = &power_change.tlv;
    response_message.list_of_TLVs[1] = NULL;

    // Send the packet
    //
    if (0 == send1905RawPacket(interface_name, mid, destination_al_mac_address, &response_message))
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Could not send packet\n");
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    // Free no longer needed memory
    //
    free(power_change.power_change_interfaces);

    free(response_message.list_of_TLVs);

    return ret;
}

uint8_t sendLLDPBridgeDiscoveryPacket(char *interface_name)
{
    uint8_t  al_mac_address[6];
    uint8_t  interface_mac_address[6];

    struct chassisIdTLV      chassis_id_tlv;
    struct portIdTLV         port_id_tlv;
    struct timeToLiveTypeTLV time_to_live_tlv;

    struct PAYLOAD payload;

    uint8_t  *stream;
    uint16_t  stream_len;

    PLATFORM_PRINTF_DEBUG_INFO("--> LLDP BRIDGE DISCOVERY (%s)\n", interface_name);

    memcpy(al_mac_address,        DMalMacGet(),                         6);
    memcpy(interface_mac_address, DMinterfaceNameToMac(interface_name), 6);

    // Fill the chassis ID TLV
    //
    chassis_id_tlv.tlv.type           = TLV_TYPE_CHASSIS_ID;
    chassis_id_tlv.chassis_id_subtype = CHASSIS_ID_TLV_SUBTYPE_MAC_ADDRESS;
    chassis_id_tlv.chassis_id[0]      = al_mac_address[0];
    chassis_id_tlv.chassis_id[1]      = al_mac_address[1];
    chassis_id_tlv.chassis_id[2]      = al_mac_address[2];
    chassis_id_tlv.chassis_id[3]      = al_mac_address[3];
    chassis_id_tlv.chassis_id[4]      = al_mac_address[4];
    chassis_id_tlv.chassis_id[5]      = al_mac_address[5];

    // Fill the port ID TLV
    //
    port_id_tlv.tlv.type            = TLV_TYPE_PORT_ID;
    port_id_tlv.port_id_subtype     = PORT_ID_TLV_SUBTYPE_MAC_ADDRESS;
    port_id_tlv.port_id[0]          = interface_mac_address[0];
    port_id_tlv.port_id[1]          = interface_mac_address[1];
    port_id_tlv.port_id[2]          = interface_mac_address[2];
    port_id_tlv.port_id[3]          = interface_mac_address[3];
    port_id_tlv.port_id[4]          = interface_mac_address[4];
    port_id_tlv.port_id[5]          = interface_mac_address[5];

    // Fill the time to live TLV
    //
    time_to_live_tlv.tlv.type       = TLV_TYPE_TIME_TO_LIVE;
    time_to_live_tlv.ttl            = TIME_TO_LIVE_TLV_1905_DEFAULT_VALUE;

    // Forge the LLDP payload containing all these TLVs
    //
    payload.list_of_TLVs[0] = &chassis_id_tlv.tlv;
    payload.list_of_TLVs[1] = &port_id_tlv.tlv;
    payload.list_of_TLVs[2] = &time_to_live_tlv.tlv;
    payload.list_of_TLVs[3] = NULL;

    stream = forge_lldp_PAYLOAD_from_structure(&payload, &stream_len);

    // Finally, send the packet!
    //
    {
        uint8_t   mcast_address[] = MCAST_LLDP;

        PLATFORM_PRINTF_DEBUG_DETAIL("Sending LLDP bridge discovery message on interface %s\n", interface_name);
        if (0 == PLATFORM_SEND_RAW_PACKET(interface_name,
                                          mcast_address,
                                          interface_mac_address,
                                          ETHERTYPE_LLDP,
                                          stream,
                                          stream_len))
        {
            PLATFORM_PRINTF_DEBUG_ERROR("Packet could not be sent!\n");
        }
    }

    // Free memory
    //
    free_lldp_PAYLOAD_packet(stream);

    return 1;
}

uint8_t send1905InterfaceListResponseALME(uint8_t alme_client_id)
{
    uint8_t   ret;

    struct getIntfListResponseALME  *out;

    char **ifs_names;
    uint8_t  ifs_nr;

    PLATFORM_PRINTF_DEBUG_INFO("--> ALME_TYPE_GET_INTF_LIST_RESPONSE\n");

    // Fill the requested ALME response
    //
    out = (struct getIntfListResponseALME *)memalloc(sizeof(struct getIntfListResponseALME));
    out->alme_type = ALME_TYPE_GET_INTF_LIST_RESPONSE;

    ifs_names = PLATFORM_GET_LIST_OF_1905_INTERFACES(&ifs_nr);
    if (0 == ifs_nr)
    {
        out->interface_descriptors_nr = 0;
        out->interface_descriptors    = NULL;
    }
    else
    {
        uint8_t i;

        out->interface_descriptors_nr = ifs_nr;
        out->interface_descriptors    = (struct _intfDescriptorEntries *)memalloc(sizeof(struct _intfDescriptorEntries) * ifs_nr);

        for (i=0; i<out->interface_descriptors_nr; i++)
        {
            struct interfaceInfo *x;

            x = PLATFORM_GET_1905_INTERFACE_INFO(ifs_names[i]);
            if (NULL == x)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Could not retrieve info of interface %s\n", ifs_names[i]);

                out->interface_descriptors[i].interface_address[0]     = 0x00;
                out->interface_descriptors[i].interface_address[1]     = 0x00;
                out->interface_descriptors[i].interface_address[2]     = 0x00;
                out->interface_descriptors[i].interface_address[3]     = 0x00;
                out->interface_descriptors[i].interface_address[4]     = 0x00;
                out->interface_descriptors[i].interface_address[5]     = 0x00;
                out->interface_descriptors[i].interface_type           = MEDIA_TYPE_UNKNOWN;
                out->interface_descriptors[i].bridge_flag              = 0;
                out->interface_descriptors[i].vendor_specific_info_nr  = 0;
                out->interface_descriptors[i].vendor_specific_info     = NULL;
            }
            else
            {
                out->interface_descriptors[i].interface_address[0]     = x->mac_address[0];
                out->interface_descriptors[i].interface_address[1]     = x->mac_address[1];
                out->interface_descriptors[i].interface_address[2]     = x->mac_address[2];
                out->interface_descriptors[i].interface_address[3]     = x->mac_address[3];
                out->interface_descriptors[i].interface_address[4]     = x->mac_address[4];
                out->interface_descriptors[i].interface_address[5]     = x->mac_address[5];
                out->interface_descriptors[i].interface_type           = x->interface_type;

                out->interface_descriptors[i].bridge_flag              = DMisInterfaceBridged(ifs_names[i]);

                if (0 == x->vendor_specific_elements_nr)
                {
                    out->interface_descriptors[i].vendor_specific_info_nr  = 0;
                    out->interface_descriptors[i].vendor_specific_info     = NULL;
                }
                else
                {
                    uint8_t j;

                    out->interface_descriptors[i].vendor_specific_info_nr  = x->vendor_specific_elements_nr;
                    out->interface_descriptors[i].vendor_specific_info     = (struct _vendorSpecificInfoEntries *)memalloc(sizeof(struct _vendorSpecificInfoEntries) * x->vendor_specific_elements_nr);

                    for (j=0; j<out->interface_descriptors[i].vendor_specific_info_nr; j++)
                    {
                        out->interface_descriptors[i].vendor_specific_info[j].ie_type      = 1;
                        out->interface_descriptors[i].vendor_specific_info[j].length_field = x->vendor_specific_elements[j].vendor_data_len + 3;
                        out->interface_descriptors[i].vendor_specific_info[j].oui[0]       = x->vendor_specific_elements[j].oui[0];
                        out->interface_descriptors[i].vendor_specific_info[j].oui[1]       = x->vendor_specific_elements[j].oui[1];
                        out->interface_descriptors[i].vendor_specific_info[j].oui[2]       = x->vendor_specific_elements[j].oui[2];

                        out->interface_descriptors[i].vendor_specific_info[j].vendor_si    = (uint8_t *)memalloc(x->vendor_specific_elements[j].vendor_data_len);
                        memcpy(out->interface_descriptors[i].vendor_specific_info[j].vendor_si, x->vendor_specific_elements[j].vendor_data, x->vendor_specific_elements[j].vendor_data_len);
                    }
                }
            }

            if (NULL != x)
            {
                free_1905_INTERFACE_INFO(x);
            }
        }
    }

    free_LIST_OF_1905_INTERFACES(ifs_names, ifs_nr);

    // Send the packet
    //
    if (0 == send1905RawALME(alme_client_id, (uint8_t *)out))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not send the 1905 ALME reply\n");
        ret = 1;
    }
    else
    {
        ret = 0;
    }

    // Free memory
    //
    free_1905_ALME_structure((uint8_t *)out);

    return ret;
}

uint8_t send1905MetricsResponseALME(uint8_t alme_client_id, uint8_t *mac_address)
{
    uint8_t   ret;

    struct getMetricResponseALME      *out;

    struct transmitterLinkMetricTLV  **tx_tlvs;
    struct receiverLinkMetricTLV     **rx_tlvs;

    uint8_t total_tlvs;
    uint8_t res;

    uint8_t i;

    PLATFORM_PRINTF_DEBUG_INFO("--> ALME_TYPE_GET_METRIC_RESPONSE\n");

    // Fill the requested ALME response
    //
    if (NULL == mac_address)
    {
        _obtainLocalMetricsTLVs(LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS, NULL,
                                LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS,
                                &tx_tlvs, &rx_tlvs, &total_tlvs);
    }
    else
    {
        _obtainLocalMetricsTLVs(LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR, mac_address,
                                LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS,
                                &tx_tlvs, &rx_tlvs, &total_tlvs);
    }

    // Reorder Tx/Rx TLVs in the way they are expected inside an ALME metrics
    // response (which is different from what you have in a "regular" TLV for
    // some strange reason, maybe a "bug" in the standard)
    //
    res = _reStructureMetricsTLVs(&tx_tlvs, &rx_tlvs, &total_tlvs);

    out = (struct getMetricResponseALME *)memalloc(sizeof(struct getMetricResponseALME));
    out->alme_type  = ALME_TYPE_GET_METRIC_RESPONSE;

    if (0 == total_tlvs || 0 == res)
    {
        out->reason_code = REASON_CODE_UNMATCHED_NEIGHBOR_MAC_ADDRESS;
        out->metrics_nr  = 0;
        out->metrics     = NULL;
    }
    else
    {
        out->reason_code = REASON_CODE_SUCCESS;
        out->metrics_nr  = total_tlvs;

        if (total_tlvs > 0)
        {
            out->metrics = (struct _metricDescriptorsEntries *)memalloc(sizeof(struct _metricDescriptorsEntries)*total_tlvs);
        }
        else
        {
            out->metrics = NULL;
        }

        for (i=0; i<total_tlvs; i++)
        {
            memcpy(out->metrics[i].neighbor_dev_address,   tx_tlvs[i]->neighbor_al_address, 6);
            memcpy(out->metrics[i].local_intf_address,     tx_tlvs[i]->transmitter_link_metrics[0].local_interface_address, 6);
                            out->metrics[i].bridge_flag           = DMisLinkBridged(out->metrics[i].local_intf_address, out->metrics[i].neighbor_dev_address, tx_tlvs[i]->transmitter_link_metrics[0].neighbor_interface_address);
                            out->metrics[i].tx_metric             = tx_tlvs[i];
                            out->metrics[i].rx_metric             = rx_tlvs[i];
        }
    }

    // Send the packet
    //
    if (0 == send1905RawALME(alme_client_id, (uint8_t *)out))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not send the 1905 ALME reply\n");
        ret = 1;
    }
    else
    {
        ret = 0;
    }

    // Before free'ing the "out" structure, set Tx and Rx pointer to NULL so
    // that "free_1905_ALME_structure()" ignores them.
    // They will be freed later, in "_freeLocalMetricsTLVs()"
    //
    out->metrics_nr = 0;
    for (i=0; i<total_tlvs; i++)
    {
        out->metrics[i].tx_metric = NULL;
        out->metrics[i].rx_metric = NULL;
    }
    free(out->metrics);
    out->metrics = NULL;
    free_1905_ALME_structure((uint8_t *)out);

    _freeLocalMetricsTLVs(&tx_tlvs, &rx_tlvs, &total_tlvs);

    return ret;
}

uint8_t send1905CustomCommandResponseALME(uint8_t alme_client_id, uint8_t command)
{
    uint8_t   ret;

    struct customCommandResponseALME  *out;

    PLATFORM_PRINTF_DEBUG_INFO("--> ALME_TYPE_CUSTOM_COMMAND_RESPONSE\n");

    // Fill the requested ALME response
    //
    out = (struct customCommandResponseALME *)memalloc(sizeof(struct customCommandResponseALME));
    out->alme_type = ALME_TYPE_CUSTOM_COMMAND_RESPONSE;

    switch (command)
    {
        case CUSTOM_COMMAND_DUMP_NETWORK_DEVICES:
        {
            // Update the information regarding the local node
            //
            _updateLocalDeviceData();

            // Dump the database (which contains information from the local and
            // remote nodes) into a text buffer and send that as a reponse
            //
            _memoryBufferWriterInit();

            DMdumpNetworkDevices(_memoryBufferWriter);

            memory_buffer[memory_buffer_i] = 0x0;

            out->bytes_nr = memory_buffer_i+1;
            out->bytes    = memory_buffer;

            break;
        }
    }

    // Send the packet
    //
    if (0 == send1905RawALME(alme_client_id, (uint8_t *)out))
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Could not send the 1905 ALME reply\n");
        ret = 1;
    }
    else
    {
        ret = 0;
    }

    // Free memory not needed anymore
    //
    if (CUSTOM_COMMAND_DUMP_NETWORK_DEVICES == command)
    {
        // Here we will free the memory buffer *and* set the pointer in the
        // "out" structure to NULL, so that later "free_1905_ALME_structure()"
        // doesn't try to free it again
        //
        _memoryBufferWriterEnd();
        out->bytes_nr = 0;
        out->bytes    = NULL;
    }
    free_1905_ALME_structure((uint8_t *)out);

    return ret;
}

