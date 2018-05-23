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
#include "packet_tools.h"

////////////////////////////////////////////////////////////////////////////////
// Auxiliary, static tables
////////////////////////////////////////////////////////////////////////////////

//   WARNING:
//     If the CMDU message type changes (ie. the definition of  CMD_TYPE_*)
//     the following tables will have to be adapted (as the array index depends
//     on that).
//     Fortunately this should never happen.

// These tables marks, for each CMDU type of message, which TLVs are:
//
//   1. Require to be present zero or more times
//
//   2. required to be present exactly once
//
// The values in these tables were obtained from "IEEE Std 1905.1-2013, Section
// 6.3"
//
//
// TODO:
//     Right now this mechanism only considers either "zero or more" or "exactly
//     one" possibilities... however, in the "1a" update of the standard, there
//     are new types of TLVs that can appear "zero or one" and "one or more"
//     times.
//     For now I'm treating:
//       A) ...the "zero or one" type as "zero or more" (this
//          happens with the "push button generic phy event notification TLV",
//          the "control URL TLV" and the "IPv4/v6 TLVs") and...
//       B) ...the "one or more" type as "exactly one" (the "interface power
//          change information type TLV" and the "interface power change status
//          TLV").
//     Case (B) is not really a problem (in fact, I think "one or more" is an
//     error in the standard for these TLVs... as it should be "exactly one"...
//     maybe this will be corrected in a future update).
//     However, because of case (A), we could end up considering valid CMDUs
//     with, for example, more than one "IPv4 TLVs" (which is clearly an error).
//
//
//
static INT32U _zeroormore_tlvs_for_cmdu[] =
{
    /* CMDU_TYPE_TOPOLOGY_DISCOVERY             */  0x00000000,
    /* CMDU_TYPE_TOPOLOGY_NOTIFICATION          */  0x00000000,
    /* CMDU_TYPE_TOPOLOGY_QUERY                 */  0x00000000,
    /* CMDU_TYPE_TOPOLOGY_RESPONSE              */  1 << TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES   | 1 << TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST | 1<< TLV_TYPE_NEIGHBOR_DEVICE_LIST | 1 << TLV_TYPE_POWER_OFF_INTERFACE | 1 << TLV_TYPE_L2_NEIGHBOR_DEVICE,
    /* CMDU_TYPE_VENDOR_SPECIFIC                */  0xffffffff,
    /* CMDU_TYPE_LINK_METRIC_QUERY              */  0x00000000,
    /* CMDU_TYPE_LINK_METRIC_RESPONSE           */  1 << TLV_TYPE_TRANSMITTER_LINK_METRIC | 1 << TLV_TYPE_RECEIVER_LINK_METRIC,
    /* CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH    */  0x00000000,
    /* CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE  */  0x00000000,
    /* CMDU_TYPE_AP_AUTOCONFIGURATION_WSC       */  0x00000000,
    /* CMDU_TYPE_AP_AUTOCONFIGURATION_RENEW     */  0x00000000,
    /* CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION */  1 << TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION,
    /* CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION  */  0x00000000,
    /* CMDU_TYPE_HIGHER_LAYER_QUERY             */  0x00000000,
    /* CMDU_TYPE_HIGHER_LAYER_RESPONSE          */  1 << TLV_TYPE_CONTROL_URL | 1 << TLV_TYPE_IPV4 | 1 << TLV_TYPE_IPV6,
    /* CMDU_TYPE_INTERFACE_POWER_CHANGE_REQUEST */  0x00000000,
    /* CMDU_TYPE_INTERFACE_POWER_CHANGE_RESPONSE*/  0x00000000,
    /* CMDU_TYPE_GENERIC_PHY_QUERY              */  0x00000000,
    /* CMDU_TYPE_GENERIC_PHY_RESPONSE           */  0x00000000,
};
static INT32U _exactlyone_tlvs_for_cmdu[] = \
{
    /* CMDU_TYPE_TOPOLOGY_DISCOVERY             */  1 << TLV_TYPE_AL_MAC_ADDRESS_TYPE     | 1 << TLV_TYPE_MAC_ADDRESS_TYPE,
    /* CMDU_TYPE_TOPOLOGY_NOTIFICATION          */  1 << TLV_TYPE_AL_MAC_ADDRESS_TYPE,
    /* CMDU_TYPE_TOPOLOGY_QUERY                 */  0x00000000,
    /* CMDU_TYPE_TOPOLOGY_RESPONSE              */  1 << TLV_TYPE_DEVICE_INFORMATION_TYPE,
    /* CMDU_TYPE_VENDOR_SPECIFIC                */  0x00000000,
    /* CMDU_TYPE_LINK_METRIC_QUERY              */  1 << TLV_TYPE_LINK_METRIC_QUERY,
    /* CMDU_TYPE_LINK_METRIC_RESPONSE           */  0x00000000,
    /* CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH    */  1 << TLV_TYPE_AL_MAC_ADDRESS_TYPE     | 1 << TLV_TYPE_SEARCHED_ROLE                  | 1 << TLV_TYPE_AUTOCONFIG_FREQ_BAND,
    /* CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE  */  1 << TLV_TYPE_SUPPORTED_ROLE          | 1 << TLV_TYPE_SUPPORTED_FREQ_BAND,
    /* CMDU_TYPE_AP_AUTOCONFIGURATION_WSC       */  1 << TLV_TYPE_WSC,
    /* CMDU_TYPE_AP_AUTOCONFIGURATION_RENEW     */  1 << TLV_TYPE_AL_MAC_ADDRESS_TYPE     | 1 << TLV_TYPE_SUPPORTED_ROLE                 | 1 << TLV_TYPE_SUPPORTED_FREQ_BAND,
    /* CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION */  1 << TLV_TYPE_AL_MAC_ADDRESS_TYPE     | 1 << TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION,
    /* CMDU_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION  */  1 << TLV_TYPE_AL_MAC_ADDRESS_TYPE     | 1 << TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION,
    /* CMDU_TYPE_HIGHER_LAYER_QUERY             */  0x00000000,
    /* CMDU_TYPE_HIGHER_LAYER_RESPONSE          */  1 << TLV_TYPE_AL_MAC_ADDRESS_TYPE     | 1 << TLV_TYPE_1905_PROFILE_VERSION           | 1 << TLV_TYPE_DEVICE_IDENTIFICATION,
    /* CMDU_TYPE_INTERFACE_POWER_CHANGE_REQUEST */  1 << TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION,
    /* CMDU_TYPE_INTERFACE_POWER_CHANGE_RESPONSE*/  1 << TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS,
    /* CMDU_TYPE_GENERIC_PHY_QUERY              */  0x00000000,
    /* CMDU_TYPE_GENERIC_PHY_RESPONSE           */  1 << TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION,
};

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
static INT8U _check_CMDU_rules(struct CMDU *p, INT8U rules_type)
{
    INT8U  i;
    INT8U  structure_has_been_modified;
    INT8U  counter[TLV_TYPE_LAST];
    INT8U  tlvs_to_remove[TLV_TYPE_LAST];

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
    for (i=0; i<=TLV_TYPE_LAST; i++)
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

    // Rules 1.a and 2.c check the same thing : make sure the structure
    // contains, *at least*, the required TLVs
    //
    // If not, return '0'
    //
    // The required TLVs are those contained in the "_exactlyone_tlvs_for_cmdu"
    // table.
    //
    for (i=0; i<=TLV_TYPE_LAST; i++)
    {
        if (
             (1 != counter[i])                                       &&
             (_exactlyone_tlvs_for_cmdu[p->message_type] & (1 << i))
             )
        {
            PLATFORM_PRINTF_DEBUG_WARNING("TLV %s should appear once on this CMDU, but it appears %d times\n", convert_1905_TLV_type_to_string(i), counter[i]);
            return 0;
        }
    }

    // Rules 1.b and 2.b also both check for the same thing (unexpected TLVs),
    // but they act in different ways:
    //
    //   * In case 'rules_type' == CHECK_CMDU_TX_RULES, return '0'
    //   * In case 'rules_type' == CHECK_CMDU_RX_RULES, remove the unexpected
    //     TLVs (and later, when all other checks have been performed, return
    //     '1' to indicate that the structure has been modified)
    //
    // Unexpected TLVs are those that do not appear in neither the
    // "_exactlyone_tlvs_for_cmdu" nor the "_zeroormore_tlvs_for_cmdu" tables
    //
    for (i=0; i<=TLV_TYPE_LAST; i++)
    {
        if (
             (0 != counter[i])                                        &&
             (i != TLV_TYPE_VENDOR_SPECIFIC)                          &&
             !(_zeroormore_tlvs_for_cmdu[p->message_type] & (1 << i)) &&
             !(_exactlyone_tlvs_for_cmdu[p->message_type] & (1 << i))
             )
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


INT8U **forge_1905_CMDU_from_structure(struct CMDU *memory_structure, INT16U **lens)
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


INT8U parse_1905_CMDU_header_from_packet(INT8U *stream, INT16U *mid, INT8U *fragment_id, INT8U *last_fragment_indicator)
{
    INT8U   message_version;
    INT8U   reserved_field;
    INT16U  message_type;
    INT8U   indicators;

    if ((NULL == stream) || (NULL == mid) || (NULL == fragment_id) || (NULL == last_fragment_indicator))
    {
        // Invalid params
        //
        return 0;
    }

    // Let's parse the header fields
    //
    _E1B(&stream, &message_version);
    _E1B(&stream, &reserved_field);
    _E2B(&stream, &message_type);
    _E2B(&stream, mid);
    _E1B(&stream, fragment_id);
    _E1B(&stream, &indicators);

    *last_fragment_indicator = (indicators & 0x80) >> 7; // MSB and 2nd MSB

    return 1;
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


INT8U compare_1905_CMDU_structures(struct CMDU *memory_structure_1, struct CMDU *memory_structure_2)
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


void visit_1905_CMDU_structure(struct CMDU *memory_structure, void (*callback)(void (*write_function)(const char *fmt, ...), const char *prefix, INT8U size, const char *name, const char *fmt, void *p), void (*write_function)(const char *fmt, ...), const char *prefix)
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
        // In order to make it easier for the callback() function to present
        // useful information, append the type of the TLV to the prefix
        //
        char new_prefix[MAX_PREFIX];

        switch(*(memory_structure->list_of_TLVs[i]))
        {
            case TLV_TYPE_END_OF_MESSAGE:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(END_OF_MESSAGE)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_VENDOR_SPECIFIC:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(VENDOR_SPECIFIC)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_AL_MAC_ADDRESS_TYPE:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(AL_MAC_ADDRESS_TYPE)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_MAC_ADDRESS_TYPE:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(MAC_ADDRESS_TYPE)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_DEVICE_INFORMATION_TYPE:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(DEVICE_INFORMATION_TYPE)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_DEVICE_BRIDGING_CAPABILITIES:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(DEVICE_BRIDGING_CAPABILITIES)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_NON_1905_NEIGHBOR_DEVICE_LIST:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(NON_1905_NEIGHBOR_DEVICE_LIST)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_NEIGHBOR_DEVICE_LIST:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(NEIGHBOR_DEVICE_LIST)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_LINK_METRIC_QUERY:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(LINK_METRIC_QUERY)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_TRANSMITTER_LINK_METRIC:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(TRANSMITTER_LINK_METRIC)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_RECEIVER_LINK_METRIC:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(RECEIVER_LINK_METRIC)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_LINK_METRIC_RESULT_CODE:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(LINK_METRIC_RESULT_CODE)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_SEARCHED_ROLE:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(SEARCHED_ROLE)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_AUTOCONFIG_FREQ_BAND:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(AUTOCONFIG_FREQ_BAND)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_SUPPORTED_ROLE:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(SUPPORTED_ROLE)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_SUPPORTED_FREQ_BAND:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(SUPPORTED_FREQ_BAND)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_WSC:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(WSC)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(PUSH_BUTTON_EVENT_NOTIFICATION)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_PUSH_BUTTON_JOIN_NOTIFICATION:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(PUSH_BUTTON_JOIN_NOTIFICATION)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }
            case TLV_TYPE_GENERIC_PHY_DEVICE_INFORMATION:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(GENERIC_PHY_DEVICE_INFORMATION)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }
            case TLV_TYPE_DEVICE_IDENTIFICATION:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(DEVICE_IDENTIFICATION)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }
            case TLV_TYPE_CONTROL_URL:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(CONTROL_URL)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }
            case TLV_TYPE_IPV4:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(IPV4)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }
            case TLV_TYPE_IPV6:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(IPV6)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }
            case TLV_TYPE_GENERIC_PHY_EVENT_NOTIFICATION:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(GENERIC_PHY_EVENT_NOTIFICATION)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }
            case TLV_TYPE_1905_PROFILE_VERSION:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(1905_PROFILE_VERSION)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }
            case TLV_TYPE_POWER_OFF_INTERFACE:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(POWER_OFF_INTERFACE)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }
            case TLV_TYPE_INTERFACE_POWER_CHANGE_INFORMATION:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(INTERFACE_POWER_CHANGE_INFORMATION)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }
            case TLV_TYPE_INTERFACE_POWER_CHANGE_STATUS:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(INTERFACE_POWER_CHANGE_STATUS)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }
            case TLV_TYPE_L2_NEIGHBOR_DEVICE:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(L2_NEIGHBOR_DEVICE)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }
            default:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(**UNKNOWN**)->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }
        }

        visit_1905_TLV_structure(memory_structure->list_of_TLVs[i], callback, write_function, new_prefix);
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

