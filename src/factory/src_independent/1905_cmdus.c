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

#include "1905_cmdus.h"
#include "1905_tlvs.h"
#include "1905_l2.h"
#include "packet_tools.h"
#include "tlv.h"

/** @brief Specification of the constraint of how many times a something may occur. */
enum count_required {
    count_required_zero = 0,      /**< @brief TLV is not allowed in this CMDU. */
    count_required_zero_or_one,   /**< @brief TLV is optional in this CMDU. */
    count_required_zero_or_more,  /**< @brief TLV is optional and may occur several times in this CMDU. */
    count_required_one,           /**< @brief TLV is required in this CMDU. */
    count_required_one_or_more,   /**< @brief TLV is required and may occur several times in this CMDU. */
    /** @brief Sentinel value marking the end of an array of cmdu_tlv_count_required.
     *
     * This takes the value 0, so that automatic 0 initialisation fills in the sentinel.
     *
     * It can also be the same value as count_required_zero because the latter doesn't occur in a
     * cmdu_tlv_count_required list: TLVs that are required not to be present are simply not mentioned in the list.
     */
    count_required_sentinel = 0,
};

/** @brief Specification of the constraint of how many times a specific TLV type may occur in a CMDU. */
struct cmdu_tlv_count_required {
    uint8_t type;               /**< TLV type to which this constraint applies. */
    enum count_required count;  /**< The constraint for this TLV type. count_required_zero is not used. */
};

/** @brief Static information about CMDUs. */
struct cmdu_info {
    /** @brief List of constraints of how many times each TLV type may occur in a CMDU.
     *
     * TLV types that are not allowed do not appear in the list.
     *
     * The list is an array where the last entry has cmdu_tlv_count_required::count_required == count_required_sentinel.
     */
    const struct cmdu_tlv_count_required *tlv_count_required;
};

/** @brief Definition of the static information of each CMDU type.
 *
 * This array is indexed by CMDU type.
 */
static const struct cmdu_info cmdu_info[] =
{
    [CMDU_TYPE_TOPOLOGY_DISCOVERY] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_AL_MAC_ADDRESS_TYPE, count_required_one},
            {TLV_TYPE_MAC_ADDRESS_TYPE, count_required_one},
            {0, count_required_sentinel},
        },
    },
    [CMDU_TYPE_TOPOLOGY_NOTIFICATION] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_AL_MAC_ADDRESS_TYPE, count_required_one},
            {0, count_required_sentinel},
        },
    },
    [CMDU_TYPE_TOPOLOGY_RESPONSE] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES, count_required_zero_or_more},
            {TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST, count_required_zero_or_more},
            {TLV_TYPE_NEIGHBOR_DEVICE_LIST, count_required_zero_or_more},
            {TLV_TYPE_POWER_OFF_INTERFACE, count_required_zero_or_more},
            {TLV_TYPE_L2_NEIGHBOR_DEVICE, count_required_zero_or_more},
            {TLV_TYPE_DEVICE_INFORMATION_TYPE, count_required_one},
            {TLV_TYPE_SUPPORTED_SERVICE, count_required_zero_or_one},
            {0, count_required_sentinel},
        },
    },
    /* CMDU_TYPE_VENDOR_SPECIFIC is a special case since any TLV is allowed. */
    [CMDU_TYPE_LINK_METRIC_QUERY] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_LINK_METRIC_QUERY, count_required_one},
            {0, count_required_sentinel},
        },
    },
    [CMDU_TYPE_LINK_METRIC_RESPONSE] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_TRANSMITTER_LINK_METRIC, count_required_zero_or_more},
            {TLV_TYPE_RECEIVER_LINK_METRIC, count_required_zero_or_more},
            {0, count_required_sentinel},
        },
    },
    [CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_AL_MAC_ADDRESS_TYPE, count_required_one},
            {TLV_TYPE_SEARCHED_ROLE, count_required_one},
            {TLV_TYPE_AUTOCONFIG_FREQ_BAND, count_required_one},
            {TLV_TYPE_SUPPORTED_SERVICE, count_required_zero_or_one},
            {TLV_TYPE_SEARCHED_SERVICE, count_required_zero_or_one},
            {0, count_required_sentinel},
        },
    },
    [CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_SUPPORTED_ROLE, count_required_one},
            {TLV_TYPE_SUPPORTED_FREQ_BAND, count_required_one},
            {TLV_TYPE_SUPPORTED_SERVICE, count_required_zero_or_one},
            {0, count_required_sentinel},
        },
    },
    [CMDU_TYPE_AP_AUTOCONFIGURATION_WSC] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_WSC, count_required_one},
            {0, count_required_sentinel},
        },
    },
    [CMDU_TYPE_AP_AUTOCONFIGURATION_RENEW] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_AL_MAC_ADDRESS_TYPE, count_required_one},
            {TLV_TYPE_SUPPORTED_ROLE, count_required_one},
            {TLV_TYPE_SUPPORTED_FREQ_BAND, count_required_one},
            {0, count_required_sentinel},
        },
    },
    [CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION, count_required_zero_or_one},
            {TLV_TYPE_AL_MAC_ADDRESS_TYPE, count_required_one},
            {TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION, count_required_one},
            {0, count_required_sentinel},
        },
    },
    [CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_AL_MAC_ADDRESS_TYPE, count_required_one},
            {TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION, count_required_one},
            {0, count_required_sentinel},
        },
    },
    [CMDU_TYPE_HIGHER_LAYER_RESPONSE] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_CONTROL_URL, count_required_zero_or_one},
            {TLV_TYPE_IPV4, count_required_zero_or_one},
            {TLV_TYPE_IPV6, count_required_zero_or_one},
            {TLV_TYPE_AL_MAC_ADDRESS_TYPE, count_required_one},
            {TLV_TYPE_1905_PROFILE_VERSION, count_required_one},
            {TLV_TYPE_DEVICE_IDENTIFICATION, count_required_one},
            {0, count_required_sentinel},
        },
    },
    [CMDU_TYPE_INTERFACE_POWER_CHANGE_REQUEST] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION, count_required_one_or_more},
            {0, count_required_sentinel},
        },
    },
    [CMDU_TYPE_INTERFACE_POWER_CHANGE_RESPONSE] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS, count_required_one_or_more},
            {0, count_required_sentinel},
        },
    },
    [CMDU_TYPE_GENERIC_PHY_RESPONSE] = {
        .tlv_count_required = (const struct cmdu_tlv_count_required[]){
            {TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION, count_required_one},
            {0, count_required_sentinel},
        },
    },
};


////////////////////////////////////////////////////////////////////////////////
// Auxiliary, static tables
////////////////////////////////////////////////////////////////////////////////

// The following table tells us the value of the 'relay_indicator' flag for
// each type of CMDU message.
//
// The values were obtained from "IEEE Std 1905.1-2013, Table 6-4"
//
// Note that '0xff' is a special value that means: "this CMDU message type can
// have the flag set to either '0' or '1' and its actual value for this
// particular message must be specified in some other way"
//
static INT8U _relayed_CMDU[] = \
{
    /* CMDU_TYPE_TOPOLOGY_DISCOVERY             */  0,
    /* CMDU_TYPE_TOPOLOGY_NOTIFICATION          */  1,
    /* CMDU_TYPE_TOPOLOGY_QUERY                 */  0,
    /* CMDU_TYPE_TOPOLOGY_QUERY                 */  0,
    /* CMDU_TYPE_VENDOR_SPECIFIC                */  0xff,
    /* CMDU_TYPE_LINK_METRIC_QUERY              */  0,
    /* CMDU_TYPE_LINK_METRIC_RESPONSE           */  0,
    /* CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH    */  1,
    /* CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE  */  0,
    /* CMDU_TYPE_AP_AUTOCONFIGURATION_WSC       */  0,
    /* CMDU_TYPE_AP_AUTOCONFIGURATION_RENEW     */  1,
    /* CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION */  1,
    /* CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION  */  1,
    /* CMDU_TYPE_HIGHER_LAYER_QUERY             */  0,
    /* CMDU_TYPE_HIGHER_LAYER_RESPONSE          */  0,
    /* CMDU_TYPE_INTERFACE_POWER_CHANGE_REQUEST */  0,
    /* CMDU_TYPE_INTERFACE_POWER_CHANGE_RESPONSE*/  0,
    /* CMDU_TYPE_GENERIC_PHY_QUERY              */  0,
    /* CMDU_TYPE_GENERIC_PHY_RESPONSE           */  0,
};


////////////////////////////////////////////////////////////////////////////////
// Auxiliary static functions
////////////////////////////////////////////////////////////////////////////////

// Each CMDU must follow some rules regarding which TLVs they can contain
// depending on their type.
//
// This is extracted from "IEEE Std 1905.1-2013, Section 6.2":
//
//   1. When generating a CMDU:
//      a) It shall include all of the TLVs that are listed for the message
//      b) It shall not include any other TLV that is not listed for the message
//      c) It may additionally include zero or more vendor specific TLVs
//
//   2. When receiving a CMDU:
//      a) It may process or ignore any vendor specific TLVs
//      b) It shall ignore all TLVs that are not specified for the message
//      c) It shall ignore the entire message if the message does not include
//         all of the TLVs that are listed for this message
//
// This function receives a pointer to a CMDU structure, 'p' and a 'rules_type'
// value:
//
//   * If 'rules_type' == CHECK_CMDU_TX_RULES, the function will check the
//     structure against the "generating a CMDU" rules (ie. rules 1.a, 1.b and
//     1.c).
//     If any of them is broken this function returns "0" (and 'p' is *not*
//     freed, as this is the caller's responsability)
//
//   * If 'rules_type' == CHECK_CMDU_RX_RULES, the function will check the
//     structure against the "receiving a CMDU" rules (ie. rules 2.a, 2.b and
//     2.c)
//     Regarding rule 2.a, we have chosen to preserve vendor specific TLVs in
//     the structure.
//     Rule 2.b is special in that non-vendor specific TLVs that are not
//     specified for the message type are removed (ie. the 'p' structure is
//     modified!)
//     Rule 2.c is special in that if it is broken, 'p' is freed
//
//  Note a small asymmetry: with 'rules_type' == CHECK_CMDU_TX_RULES,
//  unexpected options cause the function to fail while with 'rules_type' ==
//  CHECK_CMDU_RX_RULES they are simply removed (and freed) from the structure.
//  If you think about it, this is the correct behaviour: in transmission,
//  do not let invalid packets to be generated, while in reception, if invalid
//  packets are receive, ignore the unexpected pieces but process the rest.
//
//  In both cases, this function returns:
//    '0' --> If 'p' did not respect the rules and could not be "fixed"
//    '1' --> If 'p' was modified (ie. it is now valid). This can only happen
//            when 'rules_type' == CHECK_CMDU_RX_RULES
//    '2' --> If 'p' was not modifed (ie. it was valid from the beginning)
//
#define CHECK_CMDU_TX_RULES (1)
#define CHECK_CMDU_RX_RULES (2)
static INT8U _check_CMDU_rules(const struct CMDU *p, INT8U rules_type)
{
    unsigned  i;
    INT8U  structure_has_been_modified;
    INT8U  counter[TLV_TYPE_NUM];
    INT8U  tlvs_to_remove[TLV_TYPE_NUM];

    if ((NULL == p) || (NULL == p->list_of_TLVs))
    {
        // Invalid arguments
        //
        PLATFORM_PRINTF_DEBUG_ERROR("Invalid CMDU structure\n");
        return 0;
    }

    // First of all, count how many times each type of TLV message appears in
    // the structure. We will use this information later
    //
    for (i=0; i<TLV_TYPE_NUM; i++)
    {
        counter[i]        = 0;
        tlvs_to_remove[i] = 0;
    }

    i = 0;
    while (NULL != p->list_of_TLVs[i])
    {
        counter[*(p->list_of_TLVs[i])]++;
        i++;
    }

    for (i=0; i<TLV_TYPE_NUM; i++)
    {
        enum count_required required_count = count_required_zero;

        // Search the required count
        if (p->message_id == CMDU_TYPE_VENDOR_SPECIFIC)
        {
            // Special case for vendor specific CMDU: it can contain any TLV
            required_count = count_required_zero_or_more;
        }
        else if (i == TLV_TYPE_VENDOR_SPECIFIC)
        {
            // Special case for vendor specific TLV: it is always allowed
            required_count = count_required_zero_or_more;
        }
        else if (cmdu_info[p->message_type].tlv_count_required == NULL)
        {
            // No required counts specified for this CMDU, so required count is 0 for all TLVs
            required_count = count_required_zero;
        }
        else
        {
            const struct cmdu_tlv_count_required *count_required;
            for (count_required = cmdu_info[p->message_type].tlv_count_required;
                 count_required->count != count_required_sentinel;
                 count_required++)
            {
                if (count_required->type == i)
                {
                    required_count = count_required->count;
                    break;
                }
            }
            /* If not found in the list, required_count is still zero. */
        }

        switch (required_count)
        {
            case count_required_zero:
                // Rules 1.b and 2.b both check for the same thing (unexpected TLVs),
                // but they act in different ways:
                //
                //   * In case 'rules_type' == CHECK_CMDU_TX_RULES, return '0'
                //   * In case 'rules_type' == CHECK_CMDU_RX_RULES, remove the unexpected
                //     TLVs (and later, when all other checks have been performed, return
                //     '1' to indicate that the structure has been modified)
                if (counter[i] != 0)
                {
                    if (CHECK_CMDU_TX_RULES == rules_type)
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("TLV %s should not appear on this CMDU, but it appears %d times\n", convert_1905_TLV_type_to_string(i), counter[i]);
                        return 0;
                    }
                    else
                    {
                        tlvs_to_remove[i] = 1;
                    }
                }
                break;
            case count_required_zero_or_more:
                // Nothing to check, always OK.
                break;
            case count_required_zero_or_one:
                // Rule 1.b requires this TLV to be present no more than once.
                // Rule 2.b requires us to ignore the unexpected TLVs. However, that rule doesn't say which one should
                // be ignored and which one to take into account. So it makes sense to ignore the entire CMDU instead.
                // So in both cases, we return 0 if the TLV occurs more than once.
                if (counter[i] > 1)
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("TLV %s should appear at most once on this CMDU, but it appears %d times\n",
                                                  convert_1905_TLV_type_to_string(i), counter[i]);
                    return 0;
                }
                break;
            case count_required_one:
                // Rules 1.a and 2.c check the same thing : make sure the structure
                // contains, *at least*, the required TLVs
                //
                // If not, return '0'
                if (counter[i] != 1)
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("TLV %s should appear once on this CMDU, but it appears %d times\n", convert_1905_TLV_type_to_string(i), counter[i]);
                    return 0;
                }
                break;

            case count_required_one_or_more:
                // Rules 1.a and 2.c check the same thing : make sure the structure
                // contains, *at least*, the required TLVs
                //
                // If not, return '0'
                if (counter[i] == 0)
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("TLV %s should appear at least once on this CMDU, but it appears %d times\n", convert_1905_TLV_type_to_string(i), counter[i]);
                    return 0;
                }
                break;

            default:
                PLATFORM_PRINTF_DEBUG_ERROR("Programming error: invalid required count %u\n", required_count);
                return 0;
        }
    }

    i = 0;
    structure_has_been_modified = 0;
    while (NULL != p->list_of_TLVs[i])
    {
        // Here we will just traverse the list of TLVs and remove the ones
        // that shouldn't be there.
        // When this happens, mark the structure as 'modified' so that we can
        // later return the appropriate return code.
        //
        //   NOTE:
        //     When removing TLVs they are first freed and the list of
        //     pointers ('list_of_TLVs') is simply overwritten.
        //     The original piece of memory that holds all pointers is not
        //     redimensioned, though, as it would make things unnecessary more
        //     complex.
        //     In other words:
        //
        //       Before removal:
        //         list_of_TLVs --> [p1, p2, p3, NULL]
        //
        //       After removing p2:
        //         list_of_TLVs --> [p1, p3, NULL, NULL]
        //
        //       ...and not:
        //         list_of_TLVs --> [p1, p3, NULL]
        //
        if (1 == tlvs_to_remove[*(p->list_of_TLVs[i])])
        {
            INT8U j;

            free_1905_TLV_structure(p->list_of_TLVs[i]);

            structure_has_been_modified = 1;
            j = i + 1;
            while (p->list_of_TLVs[j])
            {
                p->list_of_TLVs[j-1] = p->list_of_TLVs[j];
                j++;
            }
            p->list_of_TLVs[j-1] = p->list_of_TLVs[j];
        }
        else
        {
           i++;
        }
    }

    // Regarding rules 1.c and 2.a, we don't really have to do anything special,
    // thus we can return now
    //
    if (1 == structure_has_been_modified)
    {
        return 1;
    }
    else
    {
        return 2;
    }
}



////////////////////////////////////////////////////////////////////////////////
// Actual API functions
////////////////////////////////////////////////////////////////////////////////

struct CMDU *parse_1905_CMDU_from_packets(INT8U **packet_streams)
{
    struct CMDU *ret;

    INT8U  fragments_nr;
    INT8U  current_fragment;

    INT8U  tlvs_nr;

    INT8U  error;

    if (NULL == packet_streams)
    {
        // Invalid arguments
        //
        PLATFORM_PRINTF_DEBUG_ERROR("NULL packet_streams\n");
        return NULL;
    }

    // Find out how many streams/fragments we have received
    //
    fragments_nr = 0;
    while (*(packet_streams+fragments_nr))
    {
        fragments_nr++;
    }
    if (0 == fragments_nr)
    {
        // No streams supplied!
        //
        PLATFORM_PRINTF_DEBUG_ERROR("No fragments supplied\n");
        return NULL;
    }

    // Allocate the return structure.
    // Initially it will contain an empty list of TLVs that we will later
    // re-allocate and fill.
    //
    ret = (struct CMDU *)PLATFORM_MALLOC(sizeof(struct CMDU) * 1);
    ret->list_of_TLVs = (INT8U **)PLATFORM_MALLOC(sizeof(INT8U *) * 1);
    ret->list_of_TLVs[0] = NULL;
    tlvs_nr = 0;

    // Next, parse each fragment
    //
    error = 0;
    for (current_fragment = 0; current_fragment<fragments_nr; current_fragment++)
    {
        INT8U *p;
        INT8U i;

        INT8U   message_version;
        INT8U   reserved_field;
        INT16U  message_type;
        INT16U  message_id;
        INT8U   fragment_id;
        INT8U   indicators;

        INT8U   relay_indicator;
        INT8U   last_fragment_indicator;

        INT8U *parsed;

        // We want to traverse fragments in order, thus lets search for the
        // fragment whose 'fragment_id' matches 'current_fragment' (which will
        // monotonically increase starting at '0')
        //
        for (i=0; i<fragments_nr; i++)
        {
            p = *(packet_streams+i);

            // The 'fragment_id' field is the 7th byte (offset 6)
            //
            if (current_fragment == *(p+6))
            {
                break;
            }
        }
        if (i == fragments_nr)
        {
            // One of the fragments is missing!
            //
            error = 1;
            break;
        }

        // At this point 'p' points to the stream whose 'fragment_id' is
        // 'current_fragment'

        // Let's parse the header fields
        //
        _E1B(&p, &message_version);
        _E1B(&p, &reserved_field);
        _E2B(&p, &message_type);
        _E2B(&p, &message_id);
        _E1B(&p, &fragment_id);
        _E1B(&p, &indicators);

        last_fragment_indicator = (indicators & 0x80) >> 7; // MSB and 2nd MSB
        relay_indicator         = (indicators & 0x40) >> 6; // of the
                                                            // 'indicators'
                                                            // field

        if (0 == current_fragment)
        {
            // This is the first fragment, thus fill the 'common' values.
            // We will later (in later fragments) check that their values always
            // remain the same
            //
            ret->message_version = message_version;
            ret->message_type    = message_type;
            ret->message_id      = message_id;
            ret->relay_indicator = relay_indicator;
        }
        else
        {
            // Check for consistency in all 'common' values
            //
           if (
                (ret->message_version != message_version) ||
                (ret->message_type    != message_type)    ||
                (ret->message_id      != message_id)      ||
                (ret->relay_indicator != relay_indicator)
              )
           {
               // Fragments with different common fields were detected!
               //
               error = 2;
               break;
           }
        }

        // Regarding the 'relay_indicator', depending on the message type, it
        // can only have a valid specific value
        //
        if (0xff == _relayed_CMDU[message_type])
        {
            // Special, case. All values are allowed
        }
        else
        {
            // Check if the value for this type of message is valid
            //
            if (_relayed_CMDU[message_type] != relay_indicator)
            {
                // Malformed packet
                //
                error = 3;
                break;
            }
        }

        // Regarding the 'last_fragment_indicator' flag, the following condition
        // must be met: the last fragement (and only it!) must have it set to
        // '1'
        //
        if ((1 == last_fragment_indicator) && (current_fragment < fragments_nr-1))
        {
            // 'last_fragment_indicator' appeared *before* the last fragment
            //
            error = 4;
            break;
        }
        if ((0 == last_fragment_indicator) && (current_fragment == fragments_nr-1))
        {
            // 'last_fragment_indicator' did not appear in the last fragment
            //
            error = 5;
            break;
        }

        // We can now parse the TLVs. 'p' is pointing to the first one at this
        // moment
        //
        while (1)
        {
            parsed = parse_1905_TLV_from_packet(p);
            if (NULL == parsed)
            {
                // Error while parsing a TLV
                // Dump TLV for visual inspection

                char aux1[200];
                char aux2[10];

                INT8U *p2 = p;
                INT16U len;

                INT8U  first_time;
                INT8U  j;
                INT8U  aux;

                _E1B(&p2, &aux);
                _E2B(&p2, &len);

                PLATFORM_PRINTF_DEBUG_WARNING("Parsing error. Dumping bytes: \n", error);

                // Limit dump length
                //
                if (len > 200)
                {
                    len = 200;
                }

                aux1[0]    = 0x0;
                aux2[0]    = 0x0;
                first_time = 1;
                for (j=0; j<len+3; j++)
                {
                    PLATFORM_SNPRINTF(aux2, 6, "0x%02x ", p[j]);
                    PLATFORM_STRNCAT(aux1, aux2, 200-PLATFORM_STRLEN(aux1)-1);

                    if (0 != j && 0 == (j+1)%8)
                    {
                        if (1 == first_time)
                        {
                            PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM]   - Payload        = %s\n", aux1);
                            first_time = 0;
                        }
                        else
                        {
                            PLATFORM_PRINTF_DEBUG_DETAIL("[PLATFORM]                      %s\n", aux1);
                        }
                        aux1[0] = 0x0;
                    }
                }

                error = 6;
                break;
            }

            if (TLV_TYPE_END_OF_MESSAGE == *parsed)
            {
                // No more TLVs
                //
                free_1905_TLV_structure(parsed);
                break;
            }

            // Advance 'p' to the next TLV.
            //
            INT8U  tlv_type;
            INT16U tlv_len;

            _E1B(&p, &tlv_type);
            _E2B(&p, &tlv_len);

            p += tlv_len;

            // Add this new TLV to the list (the list needs to be re-allocated
            // with more space first)
            //
            tlvs_nr++;
            ret->list_of_TLVs = (INT8U **)PLATFORM_REALLOC(ret->list_of_TLVs, sizeof(INT8U *) * (tlvs_nr+1));
            ret->list_of_TLVs[tlvs_nr-1] = parsed;
            ret->list_of_TLVs[tlvs_nr]   = NULL;
        }
        if (0 != error)
        {
            break;
        }
    }

    if (0 == error)
    {
        // Ok then... we now have our output structure properly filled.
        // However, there is one last battery of checks we must perform:
        //
        //   - CMDU_TYPE_VENDOR_SPECIFIC: The first TLV *must* be of type
        //     TLV_TYPE_VENDOR_SPECIFIC
        //
        //   - All the other message types: Some TLVs (different for each of
        //     them) can only appear once, others can appear zero or more times
        //     and others must be ignored.
        //     The '_check_CMDU_rules()' takes care of this for us.
        //
        PLATFORM_PRINTF_DEBUG_DETAIL("CMDU type: %s\n", convert_1905_CMDU_type_to_string(ret->message_type));

        if (CMDU_TYPE_VENDOR_SPECIFIC == ret->message_type)
        {
            if (NULL == ret->list_of_TLVs || NULL == ret->list_of_TLVs[0] || TLV_TYPE_VENDOR_SPECIFIC != *(ret->list_of_TLVs[0]))
            {
                error = 7;
            }
        }
        else
        {

            switch (_check_CMDU_rules(ret, CHECK_CMDU_RX_RULES))
            {
                case 0:
                {
                    // The structure was missing some required TLVs. This is
                    // a malformed packet which must be ignored.
                    //
                    PLATFORM_PRINTF_DEBUG_WARNING("Structure is missing some required TLVs\n");
                    PLATFORM_PRINTF_DEBUG_WARNING("List of present TLVs:\n");

                    if (NULL != ret->list_of_TLVs)
                    {
                        INT8U i;

                        i = 0;
                        while (ret->list_of_TLVs[i])
                        {
                            PLATFORM_PRINTF_DEBUG_WARNING("  - %s\n", convert_1905_TLV_type_to_string(*(ret->list_of_TLVs[i])));
                            i++;
                        }
                        PLATFORM_PRINTF_DEBUG_WARNING("  - <END>\n");
                    }
                    else
                    {
                        PLATFORM_PRINTF_DEBUG_WARNING("  - <NONE>\n");
                    }

                    free_1905_CMDU_structure(ret);
                    return NULL;
                }
                case 1:
                {
                    // The structure contained unxecpected TLVs. They have been
                    // removed for us.
                    //
                    break;
                }
                case 2:
                {
                    // The structure was perfect and '_check_CMDU_rules()' did
                    // not need to modify anything.
                    //
                    break;
                }
                default:
                {
                    // This point should never be reached
                    //
                    error = 8;
                    break;
                }
            }
        }
    }

    // Finally! If we get this far without errors we are already done, otherwise
    // free everything and return NULL
    //
    if (0 != error)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Parsing error %d\n", error);
        free_1905_CMDU_structure(ret);
        return NULL;
    }

    return ret;
}


INT8U **forge_1905_CMDU_from_structure(const struct CMDU *memory_structure, INT16U **lens)
{
    INT8U **ret;

    INT8U tlv_start;
    INT8U tlv_stop;

    INT8U fragments_nr;

    INT32U max_tlvs_block_size;

    INT8U error;

    error = 0;

    if (NULL == memory_structure || NULL == lens)
    {
        // Invalid arguments
        //
        return NULL;
    }
    if (NULL == memory_structure->list_of_TLVs)
    {
        // Invalid arguments
        //
        return NULL;
    }

    // Before anything else, let's check that the CMDU 'rules' are satisfied:
    //
    if (0 == _check_CMDU_rules(memory_structure, CHECK_CMDU_TX_RULES))
    {
        // Invalid arguments
        //
        return NULL;
    }

    // Allocate the return streams.
    // Initially we will just have an empty list (ie. it contains a single
    // element marking the end-of-list: a NULL pointer)
    //
    ret = (INT8U **)PLATFORM_MALLOC(sizeof(INT8U *) * 1);
    ret[0] = NULL;

    *lens = (INT16U *)PLATFORM_MALLOC(sizeof(INT16U) * 1);
    (*lens)[0] = 0;

    fragments_nr = 0;

    // Let's create as many streams as needed so that all of them fit in
    // MAX_NETWORK_SEGMENT_SIZE bytes.
    //
    // More specifically, each of the fragments that we are going to generate
    // will have a size equal to the sum of:
    //
    //   - 6 bytes (destination MAC address)
    //   - 6 bytes (origin MAC address)
    //   - 2 bytes (ETH type)
    //   - 1 byte  (CMDU message version)
    //   - 1 byte  (CMDU reserved field)
    //   - 2 bytes (CMDU message type)
    //   - 2 bytes (CMDU message id)
    //   - 1 byte  (CMDU fragment id)
    //   - 1 byte  (CMDU flags/indicators)
    //   - X bytes (size of all TLVs contained in the fragment)
    //   - 3 bytes (TLV_TYPE_END_OF_MESSAGE TLV)
    //
    // In other words, X (the size of all the TLVs that are going to be inside
    // this fragmen) can not be greater than MAX_NETWORK_SEGMENT_SIZE - 6 - 6 -
    // 2 - 1 - 1 - 2 - 2 - 1 - 1 - 3 = MAX_NETWORK_SEGMENT_SIZE - 25 bytes.
    //
    max_tlvs_block_size = MAX_NETWORK_SEGMENT_SIZE - 25;
    tlv_start           = 0;
    tlv_stop            = 0;
    do
    {
        INT8U *s;
        INT8U  i;

        INT16U current_X_size;

        INT8U reserved_field;
        INT8U fragment_id;
        INT8U indicators;

        INT8U no_space;

        current_X_size = 0;
        no_space       = 0;
        while(memory_structure->list_of_TLVs[tlv_stop])
        {
            INT8U  *p;
            INT8U  *tlv_stream;
            INT16U  tlv_stream_size;

            p = memory_structure->list_of_TLVs[tlv_stop];

            tlv_stream = forge_1905_TLV_from_structure(p, &tlv_stream_size);
            PLATFORM_FREE(tlv_stream);

            if (current_X_size + tlv_stream_size < max_tlvs_block_size)
            {
                tlv_stop++;
            }
            else
            {
                // There is no space for more TLVs
                //
                no_space = 1;
                break;
            }

            current_X_size += tlv_stream_size;
        }
        if (tlv_start == tlv_stop)
        {
            if (1 == no_space)
            {
                // One *single* TLV does not fit in a fragment!
                // This is an error... there is no way to split one single TLV into
                // several fragments according to the standard.
                //
                error = 1;
                break;
            }
            else
            {
                // If we end up here, it means tlv_start = tlv_stop = 0 --> this
                // CMDU contains no TLVs (which is something that can happen...
                // for example, in the "topology query" CMDU).
                // Just keep executing...
            }
        }

        // Now that we know how many TLVs are going to be embedded inside this
        // fragment (from 'tlv_start' up to -and not including- 'tlv_stop'),
        // let's build it
        //
        fragments_nr++;

        ret = (INT8U **)PLATFORM_REALLOC(ret, sizeof(INT8U *) * (fragments_nr + 1));
        ret[fragments_nr-1] = (INT8U *)PLATFORM_MALLOC(MAX_NETWORK_SEGMENT_SIZE);
        ret[fragments_nr]   = NULL;

        *lens = (INT16U *)PLATFORM_REALLOC(*lens, sizeof(INT16U *) * (fragments_nr + 1));
        (*lens)[fragments_nr-1] = 0; // To be updated a few lines later
        (*lens)[fragments_nr]   = 0;

        s = ret[fragments_nr-1];

        reserved_field = 0;
        fragment_id    = fragments_nr-1;
        indicators     = 0;

        // Set 'last_fragment_indicator' flag (bit #7)
        //
        if (NULL == memory_structure->list_of_TLVs[tlv_stop])
        {
            indicators |= 1 << 7;
        }

        // Set 'relay_indicator' flag (bit #6)
        //
        if (0xff == _relayed_CMDU[memory_structure->message_type])
        {
            // Special, case. Respect what the caller told us
            //
            indicators |= memory_structure->relay_indicator << 6;
        }
        else
        {
            // Use the fixed value for this type of message according to the
            // standard
            //
            indicators |= _relayed_CMDU[memory_structure->message_type] << 6;
        }

        _I1B(&memory_structure->message_version, &s);
        _I1B(&reserved_field,                    &s);
        _I2B(&memory_structure->message_type,    &s);
        _I2B(&memory_structure->message_id,      &s);
        _I1B(&fragment_id,                       &s);
        _I1B(&indicators,                        &s);

        for (i=tlv_start; i<tlv_stop; i++)
        {
            INT8U  *tlv_stream;
            INT16U  tlv_stream_size;

            tlv_stream = forge_1905_TLV_from_structure(memory_structure->list_of_TLVs[i], &tlv_stream_size);

            PLATFORM_MEMCPY(s, tlv_stream, tlv_stream_size);
            PLATFORM_FREE(tlv_stream);

            s += tlv_stream_size;
        }

        // Don't forget to add the last three octects representing the
        // TLV_TYPE_END_OF_MESSAGE message
        //
        *s = 0x0; s++;
        *s = 0x0; s++;
        *s = 0x0; s++;

        // Update the length return value
        //
        (*lens)[fragments_nr-1] = s - ret[fragments_nr-1];

        // And advance the TLV pointer so that, if more fragments are needed,
        // the next one starts where we have stopped.
        //
        tlv_start = tlv_stop;

    } while(memory_structure->list_of_TLVs[tlv_start]);

    // Finally! If we get this far without errors we are already done, otherwise
    // free everything and return NULL
    //
    if (0 != error)
    {
        free_1905_CMDU_packets(ret);
        PLATFORM_FREE(*lens);
        return NULL;
    }

    return ret;
}


bool parse_1905_CMDU_header_from_packet(INT8U *packet_buffer, size_t len, struct CMDU_header *cmdu_header)
{
    INT16U  ether_type;
    INT8U   message_version;
    INT8U   reserved_field;
    INT8U   indicators;

    if (NULL == packet_buffer || NULL == cmdu_header)
    {
        // Invalid params
        //
        return false;
    }

    if (len < 6+6+2+1+1+2+2+1+1)
    {
        // Not a valid CMDU, too small
        return false;
    }

    // Let's parse the header fields
    //
    _EnB(&packet_buffer, cmdu_header->dst_addr, 6);
    _EnB(&packet_buffer, cmdu_header->src_addr, 6);
    _E2B(&packet_buffer, &ether_type);
    if (ether_type != ETHERTYPE_1905)
    {
        // Wrong ether type, can't be a CMDU
        return false;
    }

    _E1B(&packet_buffer, &message_version);
    _E1B(&packet_buffer, &reserved_field);
    _E2B(&packet_buffer, &cmdu_header->message_type);
    _E2B(&packet_buffer, &cmdu_header->mid);
    _E1B(&packet_buffer, &cmdu_header->fragment_id);
    _E1B(&packet_buffer, &indicators);

    cmdu_header->last_fragment_indicator = (indicators & 0x80) >> 7; // MSB and 2nd MSB

    return true;
}


void free_1905_CMDU_structure(struct CMDU *memory_structure)
{

    if ((NULL != memory_structure) && (NULL != memory_structure->list_of_TLVs))
    {
        INT8U i;

        i = 0;
        while (memory_structure->list_of_TLVs[i])
        {
            free_1905_TLV_structure(memory_structure->list_of_TLVs[i]);
            i++;
        }
        PLATFORM_FREE(memory_structure->list_of_TLVs);
    }

    PLATFORM_FREE(memory_structure);

    return;
}


void free_1905_CMDU_packets(INT8U **packet_streams)
{
    INT8U i;

    if (NULL == packet_streams)
    {
        return;
    }

    i = 0;
    while (packet_streams[i])
    {
        PLATFORM_FREE(packet_streams[i]);
        i++;
    }
    PLATFORM_FREE(packet_streams);

    return;
}


INT8U compare_1905_CMDU_structures(const struct CMDU *memory_structure_1, const struct CMDU *memory_structure_2)
{
    INT8U i;

    if (NULL == memory_structure_1 || NULL == memory_structure_2)
    {
        return 1;
    }
    if (NULL == memory_structure_1->list_of_TLVs || NULL == memory_structure_2->list_of_TLVs)
    {
        return 1;
    }

    if (
         (memory_structure_1->message_version         != memory_structure_2->message_version)         ||
         (memory_structure_1->message_type            != memory_structure_2->message_type)            ||
         (memory_structure_1->message_id              != memory_structure_2->message_id)              ||
         (memory_structure_1->relay_indicator         != memory_structure_2->relay_indicator)
       )
    {
        return 1;
    }

    i = 0;
    while (1)
    {
        if (NULL == memory_structure_1->list_of_TLVs[i] && NULL == memory_structure_2->list_of_TLVs[i])
        {
            // No more TLVs to compare! Return '0' (structures are equal)
            //
            return 0;
        }

        if (0 != compare_1905_TLV_structures(memory_structure_1->list_of_TLVs[i], memory_structure_2->list_of_TLVs[i]))
        {
            // TLVs are not the same
            //
            return 1;
        }

        i++;
    }

    // This point should never be reached
    //
    return 1;
}


void visit_1905_CMDU_structure(const struct CMDU *memory_structure, visitor_callback callback, void (*write_function)(const char *fmt, ...), const char *prefix)
{
    // Buffer size to store a prefix string that will be used to show each
    // element of a structure on screen
    //
    #define MAX_PREFIX  100

    INT8U i;

    if (NULL == memory_structure)
    {
        return;
    }

    callback(write_function, prefix, sizeof(memory_structure->message_version), "message_version", "%d",  &memory_structure->message_version);
    callback(write_function, prefix, sizeof(memory_structure->message_type),    "message_type",    "%d",  &memory_structure->message_type);
    callback(write_function, prefix, sizeof(memory_structure->message_id),      "message_id",      "%d",  &memory_structure->message_id);
    callback(write_function, prefix, sizeof(memory_structure->relay_indicator), "relay_indicator", "%d",  &memory_structure->relay_indicator);

    if (NULL == memory_structure->list_of_TLVs)
    {
        return;
    }


    i = 0;
    while (NULL != memory_structure->list_of_TLVs[i])
    {
        visit_1905_TLV_structure(memory_structure->list_of_TLVs[i], callback, write_function, prefix);
        i++;
    }

    return;
}

char *convert_1905_CMDU_type_to_string(INT8U cmdu_type)
{
    switch (cmdu_type)
    {
        case CMDU_TYPE_TOPOLOGY_DISCOVERY:
            return "CMDU_TYPE_TOPOLOGY_DISCOVERY";
        case CMDU_TYPE_TOPOLOGY_NOTIFICATION:
            return "CMDU_TYPE_TOPOLOGY_NOTIFICATION";
        case CMDU_TYPE_TOPOLOGY_QUERY:
            return "CMDU_TYPE_TOPOLOGY_QUERY";
        case CMDU_TYPE_TOPOLOGY_RESPONSE:
            return "CMDU_TYPE_TOPOLOGY_RESPONSE";
        case CMDU_TYPE_VENDOR_SPECIFIC:
            return "CMDU_TYPE_VENDOR_SPECIFIC";
        case CMDU_TYPE_LINK_METRIC_QUERY:
            return "CMDU_TYPE_LINK_METRIC_QUERY";
        case CMDU_TYPE_LINK_METRIC_RESPONSE:
            return "CMDU_TYPE_LINK_METRIC_RESPONSE";
        case CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH:
            return "CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH";
        case CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE:
            return "CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE";
        case CMDU_TYPE_AP_AUTOCONFIGURATION_WSC:
            return "CMDU_TYPE_AP_AUTOCONFIGURATION_WSC";
        case CMDU_TYPE_AP_AUTOCONFIGURATION_RENEW:
            return "CMDU_TYPE_AP_AUTOCONFIGURATION_RENEW";
        case CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION:
            return "CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION";
        case CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION:
            return "CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION";
        case CMDU_TYPE_GENERIC_PHY_QUERY:
            return "CMDU_TYPE_GENERIC_PHY_QUERY";
        case CMDU_TYPE_GENERIC_PHY_RESPONSE:
            return "CMDU_TYPE_GENERIC_PHY_RESPONSE";
        case CMDU_TYPE_HIGHER_LAYER_QUERY:
            return "CMDU_TYPE_HIGHER_LAYER_QUERY";
        case CMDU_TYPE_HIGHER_LAYER_RESPONSE:
            return "CMDU_TYPE_HIGHER_LAYER_RESPONSE";
        case CMDU_TYPE_INTERFACE_POWER_CHANGE_REQUEST:
            return "CMDU_TYPE_INTERFACE_POWER_CHANGE_REQUEST";
        case CMDU_TYPE_INTERFACE_POWER_CHANGE_RESPONSE:
            return "CMDU_TYPE_INTERFACE_POWER_CHANGE_RESPONSE";
        default:
            return "Unknown";
    }
}

