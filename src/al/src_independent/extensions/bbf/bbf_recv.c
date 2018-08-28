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

#include "al_datamodel.h"
#include "al_recv.h"
#include "al_extension.h" // VendorSpecificTLVDuplicate
#include "1905_tlvs.h"
#include "bbf_tlvs.h"
#include "bbf_send.h"     // CBKUpdateBBFExtendedInfo

#include <string.h> // memcmp(), memcpy(), ...

extern uint8_t   bbf_query; // from bbf_send.c


////////////////////////////////////////////////////////////////////////////////
// CMDU extension callbacks
////////////////////////////////////////////////////////////////////////////////

uint8_t CBKprocess1905BBFExtensions(struct CMDU *memory_structure)
{
    uint8_t     *p;
    uint8_t      i;

    struct vendorSpecificTLV   *vs_tlv;

    if (NULL == memory_structure)
    {
        // Invalid param
        //
        return 0;
    }

    if (NULL == memory_structure->list_of_TLVs)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Malformed structure.");
        return 0;
    }

    // BBF protocol extension: Metrics of non-1905 links. Interested only on:
    //
    // CMDU_TYPE_LINK_METRIC_QUERY
    // `--> TLV_TYPE_VENDOR_SPECIFIC (BBF oui)
    //      `--> BBF_TLV_TYPE_NON_1905_LINK_METRIC_QUERY
    //
    // CMDU_TYPE_LINK_METRIC_RESPONSE
    // `--> TLV_TYPE_VENDOR_SPECIFIC (BBF oui)
    //      |--> BBF_TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC
    //      |--> BBF_TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC
    //      `--> BBF_TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE
    //
    // Future expectations: non-1905 link metrics should be included in the
    // IEEE1905 standard. Meanwhile, a BBF protocol extension can be used.
    //
    switch (memory_structure->message_type)
    {
        case CMDU_TYPE_LINK_METRIC_QUERY:
        {
            uint8_t                      *tlv;

            i = 0;
            while (NULL != (p = memory_structure->list_of_TLVs[i]))
            {
                // Protocol extensions are always embedded inside a Vendor
                // Specific TLV. Ignore other TLVs
                //
                if (*p == TLV_TYPE_VENDOR_SPECIFIC)
                {
                    vs_tlv = (struct vendorSpecificTLV *)p;

                    // Process only embedded BBF TLVs
                    //
                    if (0 == memcmp(vs_tlv->vendorOUI, BBF_OUI, 3))
                    {
                        tlv = parse_bbf_TLV_from_packet(vs_tlv->m);

                        if (NULL == tlv)
                        {
                            PLATFORM_PRINTF_DEBUG_ERROR("Malformed non-1905 Link Metric Query Tlv");
                        }
                        else
                        {
                            if (*tlv == BBF_TLV_TYPE_NON_1905_LINK_METRIC_QUERY)
                            {
                                // BBF query TLV has been received.
                                // CMDU response must contain BBF metric TLVs
                                //
                                bbf_query = 1;
                            }
                            else
                            {
                                PLATFORM_PRINTF_DEBUG_ERROR("Unexpected BBF protocol extension TLV");
                            }

                            // Release BBF TLV
                            //
                            free_bbf_TLV_structure(tlv);
                        }
                        // Only one BBF TLV is expected in this CMDU
                        //
                        break;
                    }
                }
                i++;
            }

            break;
        }

        case CMDU_TYPE_LINK_METRIC_RESPONSE:
        {
          struct transmitterLinkMetricTLV *transmitter_tlv = NULL;
          struct receiverLinkMetricTLV    *receiver_tlv = NULL;
          struct vendorSpecificTLV       **extensions;
          uint8_t                            extensions_nr;
          uint8_t                           *bbf_tlv;
          uint8_t                            std_FROM_al_mac_address[6];
          uint8_t                            no_std_FROM_al_mac_address[6];

          i = 0;
          extensions_nr = 0;
          extensions = NULL;
          while (NULL != (p = memory_structure->list_of_TLVs[i]))
          {
              // Protocol extensions are always embedded inside a Vendor
              // Specific TLV. Ignore other TLVs
              //
              if (*p == TLV_TYPE_VENDOR_SPECIFIC)
              {
                  vs_tlv = (struct vendorSpecificTLV *)p;

                  // Process only embedded BBF TLVs
                  //
                  if (0 == memcmp(vs_tlv->vendorOUI, BBF_OUI, 3))
                  {
                      bbf_tlv = parse_bbf_TLV_from_packet(vs_tlv->m);

                      if (NULL == bbf_tlv)
                      {
                          PLATFORM_PRINTF_DEBUG_ERROR("Malformed non-1905 Link Metric Query Tlv\n");
                      }
                      else
                      {
                          if ((*bbf_tlv == BBF_TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC) ||
                              (*bbf_tlv == BBF_TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC) )
                          {
                              // Prepare a list of TLV extensions to update the
                              // datamodel
                              //
                              if (NULL == extensions)
                              {
                                  extensions = (struct vendorSpecificTLV **)memalloc(sizeof(struct vendorSpecificTLV *));
                              }
                              else
                              {
                                  extensions = (struct vendorSpecificTLV **)PLATFORM_REALLOC(extensions, sizeof(struct vendorSpecificTLV *) * (extensions_nr + 1));
                              }

                              // Store a clone of the TLV included in the CMDU,
                              // because the main stack will release it
                              //
                              extensions[extensions_nr] = vendorSpecificTLVDuplicate(vs_tlv);
                              extensions_nr++;

                              // Get the AL MAC of the neighbor who provides
                              // these metrics
                              //
                              if (*bbf_tlv == BBF_TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC)
                              {
                                  transmitter_tlv = (struct transmitterLinkMetricTLV *)bbf_tlv;
                                  memcpy(no_std_FROM_al_mac_address, transmitter_tlv->local_al_address, 6);
                              }
                              else
                              {
                                  receiver_tlv = (struct receiverLinkMetricTLV *)bbf_tlv;
                                  memcpy(no_std_FROM_al_mac_address, receiver_tlv->local_al_address, 6);
                              }
                          }
                          else if (*bbf_tlv == BBF_TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE)
                          {
                              // Do nothing. No metrics to update
                              //
                          }
                          else
                          {
                              PLATFORM_PRINTF_DEBUG_ERROR("Unexpected BBF protocol extension TLV\n");
                          }

                          // Release the parsed BBF TLV (no longer used)
                          //
                          free_bbf_TLV_structure(bbf_tlv);
                      }
                  }
              }
              // Non-1905 metrics info is updated when a LinkMetrics CMDU is
              // received. How? All existing metrics are removed and the new
              // ones are added to the datamodel.
              //
              // The problem arises when a LinkMetrics CMDU does not include
              // non-1905 metrics info, because the CMDU's sender does not have
              // any non-1905 neighbor just in this moment.
              //
              // Following the update procedure, we need to remove all the
              // existing metrics in the datamodel, and add the new ones (none
              // this time) But, because there is not any non-standard TLV to
              // process, there is no way to know the AL MAC of the device from
              // whom we need to remove the metrics info
              //
              // Little trick: process standard metrics TLVs to get the CMDU's
              // sender AL MAC.
              //
              else if (*p == TLV_TYPE_TRANSMITTER_LINK_METRIC)
              {
                  struct transmitterLinkMetricTLV *metrics;

                  metrics = (struct transmitterLinkMetricTLV *)p;

                  memcpy(std_FROM_al_mac_address, metrics->local_al_address, 6);
              }
              else if (*p == TLV_TYPE_RECEIVER_LINK_METRIC)
              {
                  struct receiverLinkMetricTLV *metrics;

                  metrics = (struct receiverLinkMetricTLV *)p;

                  memcpy(std_FROM_al_mac_address, metrics->local_al_address, 6);
              }

              i++;
          }

          // Even when there is not any non-1905 metrics TLV, we need to remove
          // existing metrics from the datamodel. In fact, the absence of these
          // TLVs indicates that this device has zero non-1905 neighbors right
          // now.
          //
          // Instead of passing NULL as an argument to
          // 'CBKUpdateBBFExtendedInfo' we create a TLV to indicate that we
          // need to remove the metrics
          // (BBF_TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE).
          //
          if (NULL == extensions)
          {
              struct vendorSpecificTLV          *vendor_specific;
              struct linkMetricResultCodeTLV    *result_tlvs;

              // A 'result code' TLV will indicate that there are not available
              // metrics This (mark) will later force the update of metrics
              // extensions
              //
              result_tlvs = (struct linkMetricResultCodeTLV *)memalloc(sizeof(struct linkMetricResultCodeTLV));
              result_tlvs->tlv_type  = BBF_TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE;
              result_tlvs->result_code = LINK_METRIC_RESULT_CODE_TLV_INVALID_NEIGHBOR;

              vendor_specific = vendorSpecificTLVEmbedExtension(result_tlvs, forge_bbf_TLV_from_structure, (uint8_t *)BBF_OUI);

              extensions = (struct vendorSpecificTLV **)memalloc(sizeof(struct vendorSpecificTLV *));
              extensions[extensions_nr++] = vendor_specific;

              // Because there is not any non1905-metrics TLV, we use the AL
              // MAC retrieved from the standard metrics TLVs.
              //
              memcpy(no_std_FROM_al_mac_address, std_FROM_al_mac_address, 6);
          }

          CBKUpdateBBFExtendedInfo(extensions, extensions_nr, no_std_FROM_al_mac_address);

          break;

        }

        default:
        {
            break;
        }
    }

    return 1;
}
