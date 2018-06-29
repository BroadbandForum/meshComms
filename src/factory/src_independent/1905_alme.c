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

#include "1905_alme.h"
#include "1905_tlvs.h"
#include "packet_tools.h"

////////////////////////////////////////////////////////////////////////////////
// Custom (non-standarized) packet structure for standard ALME primitives:
////////////////////////////////////////////////////////////////////////////////
//
// ALME-GET-INTF-LIST.request
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x01
//
//
// ALME-GET-INTF-LIST.response
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x02
//  byte # 1: interface_descriptors_nr
//
//  byte # 2: interface_address[0]                                         |
//  byte # 3: interface_address[1]                                         |
//  byte # 4: interface_address[2]                                         |
//  byte # 5: interface_address[3]                                         |
//  byte # 6: interface_address[4]                                         | repeat
//  byte # 7: interface_address[5]                                         | "interface_descriptors_nr"
//  byte # 8: interface_type MSB                                           | times
//  byte # 9: interface_type LSB                                           |
//  byte #10: bridge_flag                                                  |
//  byte #11: vendor_specific_info_nr                                      |
//                                                                         |
//  byte #12: ie_type MSB                  |                               |
//  byte #13: ie_type LSB                  |                               |
//  byte #14: length_field MSB             |                               |
//  byte #15: length_field LSB             |                               |
//  byte #16: oui[0]                       | repeat                        |
//  byte #17: oui[1]                       | "vendor_specific_info_nr"     |
//  byte #18: oui[2]                       | times                         |
//  byte #19: vendor_si[0]                 |                               |
//  byte #20: vendor_si[1]                 |                               |
//  ...                                    |                               |
//  byte #N : vendor_si[length_field-1]    |                               |
//
//
// ALME-SET-INTF-PWR-STATE.request
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x03
//  byte # 1: interface_address[0]
//  byte # 2: interface_address[1]
//  byte # 3: interface_address[2]
//  byte # 4: interface_address[3]
//  byte # 5: interface_address[4]
//  byte # 6: interface_address[5]
//  byte # 7: power_state
//
//
// ALME-SET-INTF-PWR-STATE.confirm
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x04
//  byte # 1: interface_address[0]
//  byte # 2: interface_address[1]
//  byte # 3: interface_address[2]
//  byte # 4: interface_address[3]
//  byte # 5: interface_address[4]
//  byte # 6: interface_address[5]
//  byte # 7: reason_code
//
//
// ALME-GET-INTF-PWR-STATE.request
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x05
//  byte # 1: interface_address[0]
//  byte # 2: interface_address[1]
//  byte # 3: interface_address[2]
//  byte # 4: interface_address[3]
//  byte # 5: interface_address[4]
//  byte # 6: interface_address[5]
//
//
// ALME-GET-INTF-PWR-STATE.response
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x06
//  byte # 1: interface_address[0]
//  byte # 2: interface_address[1]
//  byte # 3: interface_address[2]
//  byte # 4: interface_address[3]
//  byte # 5: interface_address[4]
//  byte # 6: interface_address[5]
//  byte # 7: reason_code
//
// ALME-SET-FWD-RULE.request
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x07
//  byte # 1: mac_da[0]
//  byte # 2: mac_da[1]
//  byte # 3: mac_da[2]
//  byte # 4: mac_da[3]
//  byte # 5: mac_da[4]
//  byte # 6: mac_da[5]
//  byte # 7: mac_da_flag
//  byte # 8: mac_sa[0]
//  byte # 9: mac_sa[1]
//  byte #10: mac_sa[2]
//  byte #11: mac_sa[3]
//  byte #12: mac_sa[4]
//  byte #13: mac_sa[5]
//  byte #14: mac_sa_flag
//  byte #15: ether_type MSB
//  byte #16: ether_type LSB
//  byte #17: ether_type_flag
//  byte #18: 0x00 | vid 4 MSbits   -> "vid" is
//  byte #19: vid LSB                  12 bits long
//  byte #20: vid_flag
//  byte #21: 0x00 | pcp 3 LSBits   -> "pcp" is 3 bits long
//  byte #22: pcp_flag
//  byte #23: addresses_nr
//
//  byte #24: addresses[0][0]    |
//  byte #25: addresses[0][1]    | repeat
//  byte #26: addresses[0][2]    | "addresses_nr"
//  byte #27: addresses[0][3]    | times
//  byte #28: addresses[0][4]    | (with addresses[1],
//  byte #29: addresses[0][5]    | addresses[2], etc...)
//
//
// ALME-SET-FWD-RULE.confirm
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x08
//  byte # 1: rule_id MSB
//  byte # 2: rule_id LSB
//  byte # 3: reason_code
//
//
// ALME-GET-FWD-RULE.request
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x09
//
//
// ALME-GET-FWD-RULE.response
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x10
//  byte # 1: rules_nr
//
//  byte # 2: mac_da[0]                                                    |
//  byte # 3: mac_da[1]                                                    |
//  byte # 4: mac_da[2]                                                    |
//  byte # 5: mac_da[3]                                                    |
//  byte # 6: mac_da[4]                                                    |
//  byte # 7: mac_da[5]                                                    |
//  byte # 8: mac_da_flag                                                  |
//  byte # 9: mac_sa[0]                                                    |
//  byte #10: mac_sa[1]                                                    |
//  byte #11: mac_sa[2]                                                    |
//  byte #12: mac_sa[3]                                                    | repeat
//  byte #13: mac_sa[4]                                                    | "rules_nr"
//  byte #14: mac_sa[5]                                                    | times
//  byte #15: mac_sa_flag                                                  |
//  byte #16: ether_type MSB                                               |
//  byte #17: ether_type LSB                                               |
//  byte #18: ether_type_flag                                              |
//  byte #19: 0x00 | vid 4 MSbits   -> "vid" is                            |
//  byte #20: vid LSB                  12 bits long                        |
//  byte #21: vid_flag                                                     |
//  byte #22: 0x00 | pcp 3 LSBits   -> "pcp" is 3 bits long                |
//  byte #23: pcp_flag                                                     |
//  byte #24: addresses_nr                                                 |
//                                                                         |
//  byte #25: addresses[0][0]    |                                         |
//  byte #26: addresses[0][1]    | repeat                                  |
//  byte #27: addresses[0][2]    | "addresses_nr"                          |
//  byte #28: addresses[0][3]    | times                                   |
//  byte #29: addresses[0][4]    | (with addresses[1],                     |
//  byte #30: addresses[0][5]    | addresses[2], etc...)                   |
//                                                                         |
//  byte #N  : last_matched MSB                                            |
//  byte #N+1: last_matched LSB                                            |
//
//
// ALME-MODIFY-FWD-RULE.request
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x0a
//  byte # 1: rule_id MSB
//  byte # 2: rule_id LSB
//  byte # 3: addresses_nr
//
//  byte #25: addresses[0][0]    |
//  byte #26: addresses[0][1]    | repeat
//  byte #27: addresses[0][2]    | "addresses_nr"
//  byte #28: addresses[0][3]    | times
//  byte #29: addresses[0][4]    | (with addresses[1],
//  byte #30: addresses[0][5]    | addresses[2], etc...)
//
//
// ALME-MODIFY-FWD-RULE.confirm
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x0b
//  byte # 1: rule_id MSB
//  byte # 2: rule_id LSB
//  byte # 3: reason_code
//
//
// ALME-REMOVE-FWD-RULE.request
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x0c
//  byte # 1: rule_id MSB
//  byte # 2: rule_id LSB
//
//
// ALME-REMOVE-FWD-RULE.confirm
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x0d
//  byte # 1: rule_id MSB
//  byte # 2: rule_id LSB
//  byte # 3: reason_code
//
//
// ALME-GET-METRIC.request
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x0e
//
//  byte # 1: interface_address[0]
//  byte # 2: interface_address[1]
//  byte # 3: interface_address[2]
//  byte # 4: interface_address[3]
//  byte # 5: interface_address[4]
//  byte # 6: interface_address[5]
//
//
// ALME-GET-METRIC.response
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0x0f
//  byte # 1: metrics_nr
//
//  byte # 2: neighbor_dev_address[0]                                      |
//  byte # 3: neighbor_dev_address[1]                                      | repeat
//  byte # 4: neighbor_dev_address[2]                                      | "metrics_nr"
//  byte # 5: neighbor_dev_address[3]                                      | times
//  byte # 6: neighbor_dev_address[4]                                      | (one time for
//  byte # 7: neighbor_dev_address[5]                                      | each local interface
//  byte # 8: local_intf_address[0]                                        | connected to a
//  byte # 9: local_intf_address[1]                                        | remote interface
//  byte #10: local_intf_address[2]                                        | of the neighbor
//  byte #11: local_intf_address[3]                                        | node)
//  byte #12: local_intf_address[4]                                        |
//  byte #13: local_intf_address[5]                                        |
//  byte #14: bridge_flag                                                  |
//                                                                         |
//  byte #15: tlv_type   = 0x9 (transmitter link metrics)                  |
//  byte #16: tlv_length = 12 + 29*1                                       |
//  byte #17: tlv_value[0]         | Only contains one metrics element:    |
//  ...                            | the one involving                     |
//  byte #57: tlv_value[12+29-1]   | "local_intf_address"                  |
//                                                                         |
//  byte #58: tlv_type   = 0x10 (receiver link metrics)                    |
//  byte #59: tlv_length = 12 + 23*1                                       |
//  byte #60: tlv_value[0]         | Only contains one metrics element:    |
//  ...                            | the one involving                     |
//  byte #94: tlv_value[12+23-1]   | "local_intf_address"                  |
//
//  NOTES:
//    * The contents of bytes #17 to #57 are defined in "IEEE Std 1905.1-2013
//      Table 6-17" with "n=1" (ie. only one connected interface -the one
//      that matches "local_intf_address"- is reported)
//    * The contents of bytes #60 to #94 are defined in "IEEE Std 1905.1-2013
//      Table 6-19" with "n=1" (ie. only one connected interface -the one
//      that matches "local_intf_address"- is reported)


////////////////////////////////////////////////////////////////////////////////
// Private (non-standarized) packet structure for custom (not present in the
// standard) ALME primitives:
////////////////////////////////////////////////////////////////////////////////
//
// NOTE: We are using "reserved" 'alme_type' values. We might have to remove
// these new "custom" ALMEs if the standard is ever updated to make use of these
// types.
//
// ALME-CUSTOM-COMMAND.request
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0xf0
//  byte # 1: command
//
// ALME-CUSTOM-COMMAND.response
// ----------------------------------------------------------------------------
//  byte # 0: alme_type = 0xf0
//  byte # 1: length MSB
//  byte # 2: length LSB
//  byte # 3: data[0]                         |
//  ...                                       | Custom response payload
//  byte # (length + 3 - 1) : data[length-1]  |



////////////////////////////////////////////////////////////////////////////////
// Actual API functions
////////////////////////////////////////////////////////////////////////////////

INT8U *parse_1905_ALME_from_packet(INT8U *packet_stream)
{
    if (NULL == packet_stream)
    {
        return NULL;
    }

    // The first byte of the stream is the "Type" field from the ALME structure.
    // Valid values for this byte are the following ones...
    //
    switch (*packet_stream)
    {
        case ALME_TYPE_GET_INTF_LIST_REQUEST:
        {
            struct getIntfListRequestALME  *ret;

            INT8U *p;

            p = packet_stream;

            ret = (struct getIntfListRequestALME *)PLATFORM_MALLOC(sizeof(struct getIntfListRequestALME));

            _E1B(&p, &ret->alme_type);

            return (INT8U *)ret;
        }

        case ALME_TYPE_GET_INTF_LIST_RESPONSE:
        {
            struct getIntfListResponseALME  *ret;

            INT8U *p;
            INT8U i, j;

            p = packet_stream;

            ret = (struct getIntfListResponseALME *)PLATFORM_MALLOC(sizeof(struct getIntfListResponseALME));

            _E1B(&p, &ret->alme_type);
            _E1B(&p, &ret->interface_descriptors_nr);

            if (ret->interface_descriptors_nr > 0)
            {
                ret->interface_descriptors = (struct _intfDescriptorEntries *)PLATFORM_MALLOC(sizeof(struct _intfDescriptorEntries) * ret->interface_descriptors_nr);

                for (i=0; i<ret->interface_descriptors_nr; i++)
                {
                    _EnB(&p,  ret->interface_descriptors[i].interface_address, 6);
                    _E2B(&p, &ret->interface_descriptors[i].interface_type);
                    _E1B(&p, &ret->interface_descriptors[i].bridge_flag);
                    _E1B(&p, &ret->interface_descriptors[i].vendor_specific_info_nr);

                    if (ret->interface_descriptors[i].vendor_specific_info_nr > 0)
                    {
                        ret->interface_descriptors[i].vendor_specific_info = (struct _vendorSpecificInfoEntries *)PLATFORM_MALLOC(sizeof(struct _vendorSpecificInfoEntries) * ret->interface_descriptors[i].vendor_specific_info_nr);

                        for (j=0; j<ret->interface_descriptors[i].vendor_specific_info_nr; j++)
                        {
                            _E2B(&p, &ret->interface_descriptors[i].vendor_specific_info[j].ie_type);
                            _E2B(&p, &ret->interface_descriptors[i].vendor_specific_info[j].length_field);
                            _EnB(&p,  ret->interface_descriptors[i].vendor_specific_info[j].oui, 3);

                            if (ret->interface_descriptors[i].vendor_specific_info[j].length_field > 3)
                            {
                                ret->interface_descriptors[i].vendor_specific_info[j].vendor_si = (INT8U *)PLATFORM_MALLOC(sizeof(INT8U) * ((ret->interface_descriptors[i].vendor_specific_info[j].length_field - 3)));

                                _EnB(&p, ret->interface_descriptors[i].vendor_specific_info[j].vendor_si, ret->interface_descriptors[i].vendor_specific_info[j].length_field - 3);
                            }
                            else
                            {
                                ret->interface_descriptors[i].vendor_specific_info[j].vendor_si = NULL;
                            }
                        }
                    }
                }
            }

            return (INT8U *)ret;
        }

        case ALME_TYPE_SET_INTF_PWR_STATE_REQUEST:
        {
            struct setIntfPwrStateRequestALME  *ret;

            INT8U *p;

            p = packet_stream;

            ret = (struct setIntfPwrStateRequestALME *)PLATFORM_MALLOC(sizeof(struct setIntfPwrStateRequestALME));

            _E1B(&p, &ret->alme_type);
            _EnB(&p,  ret->interface_address, 6);
            _E1B(&p, &ret->power_state);

            return (INT8U *)ret;
        }

        case ALME_TYPE_SET_INTF_PWR_STATE_CONFIRM:
        {
            struct setIntfPwrStateConfirmALME  *ret;

            INT8U *p;

            p = packet_stream;

            ret = (struct setIntfPwrStateConfirmALME *)PLATFORM_MALLOC(sizeof(struct setIntfPwrStateConfirmALME));

            _E1B(&p, &ret->alme_type);
            _EnB(&p,  ret->interface_address, 6);
            _E1B(&p, &ret->reason_code);

            return (INT8U *)ret;
        }

        case ALME_TYPE_GET_INTF_PWR_STATE_REQUEST:
        {
            struct getIntfPwrStateRequestALME  *ret;

            INT8U *p;

            p = packet_stream;

            ret = (struct getIntfPwrStateRequestALME *)PLATFORM_MALLOC(sizeof(struct getIntfPwrStateRequestALME));

            _E1B(&p, &ret->alme_type);
            _EnB(&p,  ret->interface_address, 6);

            return (INT8U *)ret;
        }

        case ALME_TYPE_GET_INTF_PWR_STATE_RESPONSE:
        {
            struct getIntfPwrStateResponseALME  *ret;

            INT8U *p;

            p = packet_stream;

            ret = (struct getIntfPwrStateResponseALME *)PLATFORM_MALLOC(sizeof(struct getIntfPwrStateResponseALME));

            _E1B(&p, &ret->alme_type);
            _EnB(&p,  ret->interface_address, 6);
            _E1B(&p, &ret->power_state);

            return (INT8U *)ret;
        }

        case ALME_TYPE_SET_FWD_RULE_REQUEST:
        {
            struct setFwdRuleRequestALME  *ret;

            INT8U *p;
            INT8U i;

            p = packet_stream;

            ret = (struct setFwdRuleRequestALME *)PLATFORM_MALLOC(sizeof(struct setFwdRuleRequestALME));

            _E1B(&p, &ret->alme_type);
            _EnB(&p,  ret->classification_set.mac_da, 6);
            _E1B(&p, &ret->classification_set.mac_da_flag);
            _EnB(&p,  ret->classification_set.mac_sa, 6);
            _E1B(&p, &ret->classification_set.mac_sa_flag);
            _E2B(&p, &ret->classification_set.ether_type);
            _E1B(&p, &ret->classification_set.ether_type_flag);
            _E2B(&p, &ret->classification_set.vid);
            _E1B(&p, &ret->classification_set.vid_flag);
            _E1B(&p, &ret->classification_set.pcp);
            _E1B(&p, &ret->classification_set.pcp_flag);
            _E1B(&p, &ret->addresses_nr);

            if (ret->addresses_nr > 0)
            {
                ret->addresses = (INT8U (*)[6])PLATFORM_MALLOC(sizeof(INT8U[6]) * ret->addresses_nr);

                for (i=0; i<ret->addresses_nr; i++)
                {
                    _EnB(&p, ret->addresses[i], 6);
                }
            }

            return (INT8U *)ret;
        }

        case ALME_TYPE_SET_FWD_RULE_CONFIRM:
        {
            struct setFwdRuleConfirmALME  *ret;

            INT8U *p;

            p = packet_stream;

            ret = (struct setFwdRuleConfirmALME *)PLATFORM_MALLOC(sizeof(struct setFwdRuleConfirmALME));

            _E1B(&p, &ret->alme_type);
            _E2B(&p, &ret->rule_id);
            _E1B(&p, &ret->reason_code);

            return (INT8U *)ret;
        }

        case ALME_TYPE_GET_FWD_RULES_REQUEST:
        {
            struct getFwdRulesRequestALME  *ret;

            INT8U *p;

            p = packet_stream;

            ret = (struct getFwdRulesRequestALME *)PLATFORM_MALLOC(sizeof(struct getFwdRulesRequestALME));

            _E1B(&p, &ret->alme_type);

            return (INT8U *)ret;
        }

        case ALME_TYPE_GET_FWD_RULES_RESPONSE:
        {
            struct getFwdRulesResponseALME  *ret;

            INT8U *p;
            INT8U i, j;

            p = packet_stream;

            ret = (struct getFwdRulesResponseALME *)PLATFORM_MALLOC(sizeof(struct getFwdRulesResponseALME));

            _E1B(&p, &ret->alme_type);
            _E1B(&p, &ret->rules_nr);

            if (ret->rules_nr > 0)
            {
                ret->rules = (struct _fwdRuleListEntries *)PLATFORM_MALLOC(sizeof(struct _fwdRuleListEntries) * ret->rules_nr);

                for (i=0; i<ret->rules_nr; i++)
                {
                    _EnB(&p,  ret->rules[i].classification_set.mac_da, 6);
                    _E1B(&p, &ret->rules[i].classification_set.mac_da_flag);
                    _EnB(&p,  ret->rules[i].classification_set.mac_sa, 6);
                    _E1B(&p, &ret->rules[i].classification_set.mac_sa_flag);
                    _E2B(&p, &ret->rules[i].classification_set.ether_type);
                    _E1B(&p, &ret->rules[i].classification_set.ether_type_flag);
                    _E2B(&p, &ret->rules[i].classification_set.vid);
                    _E1B(&p, &ret->rules[i].classification_set.vid_flag);
                    _E1B(&p, &ret->rules[i].classification_set.pcp);
                    _E1B(&p, &ret->rules[i].classification_set.pcp_flag);
                    _E1B(&p, &ret->rules[i].addresses_nr);

                    if (ret->rules[i].addresses_nr > 0)
                    {
                        ret->rules[i].addresses = (INT8U (*)[6])PLATFORM_MALLOC(sizeof(INT8U[6]) * ret->rules[i].addresses_nr);

                        for (j=0; j<ret->rules[i].addresses_nr; j++)
                        {
                            _EnB(&p, ret->rules[i].addresses[j], 6);
                        }
                    }

                    _E2B(&p, &ret->rules[i].last_matched);
                }
            }

            return (INT8U *)ret;
        }

        case ALME_TYPE_MODIFY_FWD_RULE_REQUEST:
        {
            struct modifyFwdRuleRequestALME  *ret;

            INT8U *p;
            INT8U i;

            p = packet_stream;

            ret = (struct modifyFwdRuleRequestALME *)PLATFORM_MALLOC(sizeof(struct modifyFwdRuleRequestALME));

            _E1B(&p, &ret->alme_type);
            _E2B(&p, &ret->rule_id);
            _E1B(&p, &ret->addresses_nr);

            if (ret->addresses_nr > 0)
            {
                ret->addresses = (INT8U (*)[6])PLATFORM_MALLOC(sizeof(INT8U[6]) * ret->addresses_nr);

                for (i=0; i<ret->addresses_nr; i++)
                {
                    _EnB(&p, ret->addresses[i], 6);
                }
            }

            return (INT8U *)ret;
        }

        case ALME_TYPE_MODIFY_FWD_RULE_CONFIRM:
        {
            struct modifyFwdRuleConfirmALME  *ret;

            INT8U *p;

            p = packet_stream;

            ret = (struct modifyFwdRuleConfirmALME *)PLATFORM_MALLOC(sizeof(struct modifyFwdRuleConfirmALME));

            _E1B(&p, &ret->alme_type);
            _E2B(&p, &ret->rule_id);
            _E1B(&p, &ret->reason_code);

            return (INT8U *)ret;
        }

        case ALME_TYPE_REMOVE_FWD_RULE_REQUEST:
        {
            struct removeFwdRuleRequestALME  *ret;

            INT8U *p;

            p = packet_stream;

            ret = (struct removeFwdRuleRequestALME *)PLATFORM_MALLOC(sizeof(struct removeFwdRuleRequestALME));

            _E1B(&p, &ret->alme_type);
            _E2B(&p, &ret->rule_id);

            return (INT8U *)ret;
        }

        case ALME_TYPE_REMOVE_FWD_RULE_CONFIRM:
        {
            struct removeFwdRuleConfirmALME  *ret;

            INT8U *p;

            p = packet_stream;

            ret = (struct removeFwdRuleConfirmALME *)PLATFORM_MALLOC(sizeof(struct removeFwdRuleConfirmALME));

            _E1B(&p, &ret->alme_type);
            _E2B(&p, &ret->rule_id);
            _E1B(&p, &ret->reason_code);

            return (INT8U *)ret;
        }

        case ALME_TYPE_GET_METRIC_REQUEST:
        {
            struct getMetricRequestALME  *ret;

            INT8U *p;

            p = packet_stream;

            ret = (struct getMetricRequestALME *)PLATFORM_MALLOC(sizeof(struct getMetricRequestALME));

            _E1B(&p, &ret->alme_type);
            _EnB(&p,  ret->interface_address, 6);

            return (INT8U *)ret;
        }

        case ALME_TYPE_GET_METRIC_RESPONSE:
        {
            struct getMetricResponseALME  *ret;

            INT8U *p;
            INT8U i;

            p = packet_stream;

            ret = (struct getMetricResponseALME *)PLATFORM_MALLOC(sizeof(struct getMetricResponseALME));

            _E1B(&p, &ret->alme_type);
            _E1B(&p, &ret->metrics_nr);

            if (ret->metrics_nr > 0)
            {
                ret->metrics = (struct _metricDescriptorsEntries *)PLATFORM_MALLOC(sizeof(struct _metricDescriptorsEntries) * ret->metrics_nr);

                for (i=0; i<ret->metrics_nr; i++)
                {
                    struct transmitterLinkMetricTLV *tx;
                    struct receiverLinkMetricTLV    *rx;

                    INT8U  aux1;
                    INT16U aux2;

                    _EnB(&p,  ret->metrics[i].neighbor_dev_address, 6);
                    _EnB(&p,  ret->metrics[i].local_intf_address,   6);
                    _E1B(&p, &ret->metrics[i].bridge_flag);

                    tx = (struct transmitterLinkMetricTLV *)parse_1905_TLV_from_packet(p);

                    if (
                                         NULL                                                     ==  tx                                       ||
                         PLATFORM_MEMCMP(tx->neighbor_al_address,                                     ret->metrics[i].neighbor_dev_address, 6) ||
                                         1                                                        !=  tx->transmitter_link_metrics_nr          ||
                                         NULL                                                     ==  tx->transmitter_link_metrics             ||
                         PLATFORM_MEMCMP(tx->transmitter_link_metrics[0].local_interface_address,    ret->metrics[i].local_intf_address, 6)
                       )
                    {
                        // Parsing error
                        //
                        if (NULL != tx)
                        {
                            free_1905_TLV_structure((INT8U *)tx);
                        }
                        PLATFORM_FREE(ret->metrics);
                        PLATFORM_FREE(ret);
                        return NULL;
                    }

                    _E1B(&p, &aux1);
                    _E2B(&p, &aux2);
                    p += aux2;

                    rx = (struct receiverLinkMetricTLV *)parse_1905_TLV_from_packet(p);

                    if (
                                         NULL                                                     ==  rx                                       ||
                         PLATFORM_MEMCMP(rx->neighbor_al_address,                                     ret->metrics[i].neighbor_dev_address, 6) ||
                                         1                                                        !=  rx->receiver_link_metrics_nr             ||
                                         NULL                                                     ==  rx->receiver_link_metrics                ||
                         PLATFORM_MEMCMP(rx->receiver_link_metrics[0].local_interface_address,    ret->metrics[i].local_intf_address, 6)
                       )
                    {
                        // Parsing error
                        //
                        if (NULL != rx)
                        {
                            free_1905_TLV_structure((INT8U *)rx);
                        }
                        PLATFORM_FREE(ret->metrics);
                        PLATFORM_FREE(ret);
                        return NULL;
                    }

                    _E1B(&p, &aux1);
                    _E2B(&p, &aux2);
                    p += aux2;

                    ret->metrics[i].tx_metric = tx;
                    ret->metrics[i].rx_metric = rx;
                }
            }

            _E1B(&p, &ret->reason_code);

            return (INT8U *)ret;
        }

        case ALME_TYPE_CUSTOM_COMMAND_REQUEST:
        {
            struct customCommandRequestALME  *ret;

            INT8U *p;

            p = packet_stream;

            ret = (struct customCommandRequestALME *)PLATFORM_MALLOC(sizeof(struct customCommandRequestALME));

            _E1B(&p, &ret->alme_type);
            _E1B(&p, &ret->command);

            return (INT8U *)ret;
        }

        case ALME_TYPE_CUSTOM_COMMAND_RESPONSE:
        {
            struct customCommandResponseALME  *ret;

            INT8U *p;

            p = packet_stream;

            ret = (struct customCommandResponseALME *)PLATFORM_MALLOC(sizeof(struct customCommandResponseALME));

            _E1B(&p, &ret->alme_type);
            _E2B(&p, &ret->bytes_nr);

            if (ret->bytes_nr > 0)
            {
                ret->bytes = (char *)PLATFORM_MALLOC(ret->bytes_nr);
                _EnB(&p, ret->bytes, ret->bytes_nr);
            }

            return (INT8U *)ret;
        }

        default:
        {
            // Ignore
            //
            return NULL;
        }

    }

    // This code cannot be reached
    //
    return NULL;
}

INT8U *forge_1905_ALME_from_structure(INT8U *memory_structure, INT16U *len)
{
    if (NULL == memory_structure)
    {
        return NULL;
    }

    // The first byte of any of the valid structures is always the "tlv_type"
    // field.
    //
    switch (*memory_structure)
    {
        case ALME_TYPE_GET_INTF_LIST_REQUEST:
        {
            INT8U *ret, *p;
            struct getIntfListRequestALME *m;

            m = (struct getIntfListRequestALME *)memory_structure;

            *len = 1; // alme_type

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,  &p);

            return ret;
        }

        case ALME_TYPE_GET_INTF_LIST_RESPONSE:
        {
            INT8U *ret, *p;
            struct getIntfListResponseALME *m;

            INT8U i, j;

            m = (struct getIntfListResponseALME *)memory_structure;

            *len = 2; // alme_type + interface_descriptors_nr
            for (i=0; i<m->interface_descriptors_nr; i++)
            {
                *len += 6; // interface_address
                *len += 2; // interface_type
                *len += 1; // bridge_flag
                *len += 1; // vendor_specific_info_nr

                for (j=0; j<m->interface_descriptors[i].vendor_specific_info_nr; j++)
                {
                    *len += 7; //ie_type + length_field + oui
                    *len += m->interface_descriptors[i].vendor_specific_info[j].length_field - 3; // vendor_si
                }
            }

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,                 &p);
            _I1B(&m->interface_descriptors_nr,  &p);

            for (i=0; i<m->interface_descriptors_nr; i++)
            {
                _InB( m->interface_descriptors[i].interface_address,       &p, 6);
                _I2B(&m->interface_descriptors[i].interface_type,          &p);
                _I1B(&m->interface_descriptors[i].bridge_flag,             &p);
                _I1B(&m->interface_descriptors[i].vendor_specific_info_nr, &p);

                for (j=0; j<m->interface_descriptors[i].vendor_specific_info_nr; j++)
                {
                    _I2B(&m->interface_descriptors[i].vendor_specific_info[j].ie_type,      &p);
                    _I2B(&m->interface_descriptors[i].vendor_specific_info[j].length_field, &p);
                    _InB( m->interface_descriptors[i].vendor_specific_info[j].oui,          &p, 3);

                    if (m->interface_descriptors[i].vendor_specific_info[j].length_field > 3)
                    {
                        _InB( m->interface_descriptors[i].vendor_specific_info[j].vendor_si, &p, m->interface_descriptors[i].vendor_specific_info[j].length_field - 3);
                    }
                }
            }

            return ret;
        }

        case ALME_TYPE_SET_INTF_PWR_STATE_REQUEST:
        {
            INT8U *ret, *p;
            struct setIntfPwrStateRequestALME *m;

            m = (struct setIntfPwrStateRequestALME *)memory_structure;

            *len = 8; // alme_type + interface_address + power_state

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,          &p);
            _InB( m->interface_address,  &p,  6);
            _I1B(&m->power_state,        &p);

            return ret;
        }

        case ALME_TYPE_SET_INTF_PWR_STATE_CONFIRM:
        {
            INT8U *ret, *p;
            struct setIntfPwrStateConfirmALME *m;

            m = (struct setIntfPwrStateConfirmALME *)memory_structure;

            *len = 8; // alme_type + interface_address + reason_code

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,          &p);
            _InB( m->interface_address,  &p,  6);
            _I1B(&m->reason_code,        &p);

            return ret;
        }

        case ALME_TYPE_GET_INTF_PWR_STATE_REQUEST:
        {
            INT8U *ret, *p;
            struct getIntfPwrStateRequestALME *m;

            m = (struct getIntfPwrStateRequestALME *)memory_structure;

            *len = 7; // alme_type + interface_address

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,          &p);
            _InB( m->interface_address,  &p,  6);

            return ret;
        }

        case ALME_TYPE_GET_INTF_PWR_STATE_RESPONSE:
        {
            INT8U *ret, *p;
            struct getIntfPwrStateResponseALME *m;

            m = (struct getIntfPwrStateResponseALME *)memory_structure;

            *len = 8; // alme_type + interface_address + power_state

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,          &p);
            _InB( m->interface_address,  &p,  6);
            _I1B(&m->power_state,        &p);

            return ret;
        }

        case ALME_TYPE_SET_FWD_RULE_REQUEST:
        {
            INT8U *ret, *p;
            struct setFwdRuleRequestALME *m;

            INT8U i;

            m = (struct setFwdRuleRequestALME *)memory_structure;

            *len  = 24;  // alme_type + classification_set + addresses_nr
            *len += 6 * m->addresses_nr; // addresses

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,                           &p);
            _InB( m->classification_set.mac_da,           &p,  6);
            _I1B(&m->classification_set.mac_da_flag,      &p);
            _InB( m->classification_set.mac_sa,           &p,  6);
            _I1B(&m->classification_set.mac_sa_flag,      &p);
            _I2B(&m->classification_set.ether_type,       &p);
            _I1B(&m->classification_set.ether_type_flag,  &p);
            _I2B(&m->classification_set.vid,              &p);
            _I1B(&m->classification_set.vid_flag,         &p);
            _I1B(&m->classification_set.pcp,              &p);
            _I1B(&m->classification_set.pcp_flag,         &p);
            _I1B(&m->addresses_nr,                        &p);

            for (i=0; i<m->addresses_nr; i++)
            {
                _InB( m->addresses[i], &p, 6);
            }

            return ret;
        }

        case ALME_TYPE_SET_FWD_RULE_CONFIRM:
        {
            INT8U *ret, *p;
            struct setFwdRuleConfirmALME *m;

            m = (struct setFwdRuleConfirmALME *)memory_structure;

            *len = 4; // alme_type + rule_id + reason_code

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,    &p);
            _I2B(&m->rule_id,      &p);
            _I1B(&m->reason_code,  &p);

            return ret;
        }

        case ALME_TYPE_GET_FWD_RULES_REQUEST:
        {
            INT8U *ret, *p;
            struct getFwdRulesRequestALME *m;

            m = (struct getFwdRulesRequestALME *)memory_structure;

            *len = 1; // alme_type

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,          &p);

            return ret;
        }

        case ALME_TYPE_GET_FWD_RULES_RESPONSE:
        {
            INT8U *ret, *p;
            struct getFwdRulesResponseALME *m;

            INT8U i, j;

            m = (struct getFwdRulesResponseALME *)memory_structure;

            *len = 2;  // alme_type + rules_nr
            for (i=0; i<m->rules_nr; i++)
            {
                *len += 23; // classification_set + addresses_nr
                *len += 6 * m->rules[i].addresses_nr; // addresses
                *len += 2; // last_matched
            }

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type, &p);
            _I1B(&m->rules_nr,  &p);

            for (i=0; i<m->rules_nr; i++)
            {
                _InB( m->rules[i].classification_set.mac_da,           &p,  6);
                _I1B(&m->rules[i].classification_set.mac_da_flag,      &p);
                _InB( m->rules[i].classification_set.mac_sa,           &p,  6);
                _I1B(&m->rules[i].classification_set.mac_sa_flag,      &p);
                _I2B(&m->rules[i].classification_set.ether_type,       &p);
                _I1B(&m->rules[i].classification_set.ether_type_flag,  &p);
                _I2B(&m->rules[i].classification_set.vid,              &p);
                _I1B(&m->rules[i].classification_set.vid_flag,         &p);
                _I1B(&m->rules[i].classification_set.pcp,              &p);
                _I1B(&m->rules[i].classification_set.pcp_flag,         &p);
                _I1B(&m->rules[i].addresses_nr,                        &p);

                for (j=0; j<m->rules[i].addresses_nr; j++)
                {
                    _InB( m->rules[i].addresses[j], &p, 6);
                }

                _I2B(&m->rules[i].last_matched,                        &p);
            }

            return ret;
        }

        case ALME_TYPE_MODIFY_FWD_RULE_REQUEST:
        {
            INT8U *ret, *p;
            struct modifyFwdRuleRequestALME *m;

            INT8U i;

            m = (struct modifyFwdRuleRequestALME *)memory_structure;

            *len  = 4;  // alme_type + rule_id + addresses_nr
            *len += 6 * m->addresses_nr; // addresses

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,    &p);
            _I2B(&m->rule_id,      &p);
            _I1B(&m->addresses_nr, &p);

            for (i=0; i<m->addresses_nr; i++)
            {
                _InB( m->addresses[i], &p, 6);
            }

            return ret;
        }

        case ALME_TYPE_MODIFY_FWD_RULE_CONFIRM:
        {
            INT8U *ret, *p;
            struct modifyFwdRuleConfirmALME *m;

            m = (struct modifyFwdRuleConfirmALME *)memory_structure;

            *len = 4;  // alme_type + rule_id + reason_code

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,    &p);
            _I2B(&m->rule_id,      &p);
            _I1B(&m->reason_code,  &p);

            return ret;
        }

        case ALME_TYPE_REMOVE_FWD_RULE_REQUEST:
        {
            INT8U *ret, *p;
            struct removeFwdRuleRequestALME *m;

            m = (struct removeFwdRuleRequestALME *)memory_structure;

            *len = 3;  // alme_type + rule_id

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,    &p);
            _I2B(&m->rule_id,      &p);

            return ret;
        }

        case ALME_TYPE_REMOVE_FWD_RULE_CONFIRM:
        {
            INT8U *ret, *p;
            struct removeFwdRuleConfirmALME *m;

            m = (struct removeFwdRuleConfirmALME *)memory_structure;

            *len = 4;  // alme_type + rule_id + reason_code

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,    &p);
            _I2B(&m->rule_id,      &p);
            _I1B(&m->reason_code,  &p);

            return ret;
        }

        case ALME_TYPE_GET_METRIC_REQUEST:
        {
            INT8U *ret, *p;
            struct getMetricRequestALME *m;

            m = (struct getMetricRequestALME *)memory_structure;

            *len  = 7;  // alme_type + interface_address

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,         &p);
            _InB( m->interface_address, &p, 6);

            return ret;
        }

        case ALME_TYPE_GET_METRIC_RESPONSE:
        {
            INT8U *ret, *p;
            struct getMetricResponseALME *m;

            INT8U i;

            m = (struct getMetricResponseALME *)memory_structure;

            *len = 2;  // alme_type + metrics_nr
            for (i=0; i<m->metrics_nr; i++)
            {
                INT8U  *metric_stream;
                INT16U  metric_stream_len;

                *len += 6; // neighbor_dev_address
                *len += 6; // local_intf_address
                *len += 1; // bridge_flag

                metric_stream = forge_1905_TLV_from_structure((INT8U *)m->metrics[i].tx_metric, &metric_stream_len);
                if (NULL == metric_stream || 0 == metric_stream_len)
                {
                    // Forging error
                    //
                    *len = 0;
                    return NULL;
                }

                *len += metric_stream_len;
                free_1905_TLV_packet(metric_stream);

                metric_stream = forge_1905_TLV_from_structure((INT8U *)m->metrics[i].rx_metric, &metric_stream_len);
                if (NULL == metric_stream || 0 == metric_stream_len)
                {
                    // Forging error
                    //
                    *len = 0;
                    return NULL;
                }

                *len += metric_stream_len;
                free_1905_TLV_packet(metric_stream);
            }
            *len += 1;  // reason_code

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type,  &p);
            _I1B(&m->metrics_nr, &p);

            for (i=0; i<m->metrics_nr; i++)
            {
                INT8U  *metric_stream;
                INT16U  metric_stream_len;

                struct transmitterLinkMetricTLV *tx;
                struct receiverLinkMetricTLV    *rx;

                _InB( m->metrics[i].neighbor_dev_address, &p, 6);
                _InB( m->metrics[i].local_intf_address,   &p, 6);
                _I1B(&m->metrics[i].bridge_flag,          &p);

                tx = m->metrics[i].tx_metric;
                rx = m->metrics[i].rx_metric;

                if (
                                         NULL                                                     ==  tx                                     ||
                         PLATFORM_MEMCMP(tx->neighbor_al_address,                                     m->metrics[i].neighbor_dev_address, 6) ||
                                         NULL                                                     ==  tx->transmitter_link_metrics           ||
                         PLATFORM_MEMCMP(tx->transmitter_link_metrics[0].local_interface_address,     m->metrics[i].local_intf_address, 6)   ||

                                         NULL                                                     ==  rx                                     ||
                         PLATFORM_MEMCMP(rx->neighbor_al_address,                                     m->metrics[i].neighbor_dev_address, 6) ||
                                         NULL                                                     ==  rx->receiver_link_metrics              ||
                         PLATFORM_MEMCMP(rx->receiver_link_metrics[0].local_interface_address,        m->metrics[i].local_intf_address, 6)
                   )
                {
                    // Malformed packet
                    //
                    *len = 0;
                    PLATFORM_FREE(ret);
                    return NULL;
                }

                metric_stream = forge_1905_TLV_from_structure((INT8U *)tx, &metric_stream_len);
                if (NULL == metric_stream || 0 == metric_stream_len)
                {
                    // Forging error
                    //
                    *len = 0;
                    PLATFORM_FREE(ret);
                    return NULL;
                }
                _InB( metric_stream,  &p, metric_stream_len);
                free_1905_TLV_packet(metric_stream);

                metric_stream = forge_1905_TLV_from_structure((INT8U *)rx, &metric_stream_len);
                if (NULL == metric_stream || 0 == metric_stream_len)
                {
                    // Forging error
                    //
                    *len = 0;
                    PLATFORM_FREE(ret);
                    return NULL;
                }
                _InB( metric_stream,  &p, metric_stream_len);
                free_1905_TLV_packet(metric_stream);
            }

            _I1B(&m->reason_code,  &p);

            return ret;
        }

        case ALME_TYPE_CUSTOM_COMMAND_REQUEST:
        {
            INT8U *ret, *p;
            struct customCommandRequestALME *m;

            m = (struct customCommandRequestALME *)memory_structure;

            *len  = 2;  // alme_type + command

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type, &p);
            _I1B(&m->command,   &p);

            return ret;
        }

        case ALME_TYPE_CUSTOM_COMMAND_RESPONSE:
        {
            INT8U *ret, *p;
            struct customCommandResponseALME *m;

            m = (struct customCommandResponseALME *)memory_structure;

            *len  = 3;  // alme_type + length
            *len += m->bytes_nr;

            p = ret = (INT8U *)PLATFORM_MALLOC(*len);

            _I1B(&m->alme_type, &p);
            _I2B(&m->bytes_nr,  &p);

            if (m->bytes_nr > 0)
            {
                _InB( m->bytes,  &p, m->bytes_nr);
            }

            return ret;
        }

        default:
        {
            // Ignore
            //
            return NULL;
        }

    }

    // This code cannot be reached
    //
    return NULL;
}


void free_1905_ALME_structure(INT8U *memory_structure)
{
    if (NULL == memory_structure)
    {
        return;
    }

    // The first byte of any of the valid structures is always the "alme_type"
    // field.
    //
    switch (*memory_structure)
    {
        case ALME_TYPE_GET_INTF_LIST_REQUEST:
        case ALME_TYPE_SET_INTF_PWR_STATE_REQUEST:
        case ALME_TYPE_SET_INTF_PWR_STATE_CONFIRM:
        case ALME_TYPE_GET_INTF_PWR_STATE_REQUEST:
        case ALME_TYPE_GET_INTF_PWR_STATE_RESPONSE:
        case ALME_TYPE_SET_FWD_RULE_CONFIRM:
        case ALME_TYPE_GET_FWD_RULES_REQUEST:
        case ALME_TYPE_MODIFY_FWD_RULE_CONFIRM:
        case ALME_TYPE_REMOVE_FWD_RULE_REQUEST:
        case ALME_TYPE_REMOVE_FWD_RULE_CONFIRM:
        case ALME_TYPE_GET_METRIC_REQUEST:
        case ALME_TYPE_CUSTOM_COMMAND_REQUEST:
        {
            PLATFORM_FREE(memory_structure);

            return;
        }

        case ALME_TYPE_GET_INTF_LIST_RESPONSE:
        {
            struct getIntfListResponseALME *m;
            INT8U i, j;

            m = (struct getIntfListResponseALME *)memory_structure;

            if (m->interface_descriptors_nr > 0 && NULL != m->interface_descriptors)
            {
                for (i=0; i < m->interface_descriptors_nr; i++)
                {
                    if (m->interface_descriptors[i].vendor_specific_info_nr > 0 && NULL != m->interface_descriptors[i].vendor_specific_info)
                    {
                        for (j=0; j < m->interface_descriptors[i].vendor_specific_info_nr; j++)
                        {
                            if (NULL != m->interface_descriptors[i].vendor_specific_info[j].vendor_si)
                            {
                                PLATFORM_FREE(m->interface_descriptors[i].vendor_specific_info[j].vendor_si);
                            }
                        }
                        PLATFORM_FREE(m->interface_descriptors[i].vendor_specific_info);
                    }
                }
                PLATFORM_FREE(m->interface_descriptors);
            }
            PLATFORM_FREE(m);

            return;
        }

        case ALME_TYPE_SET_FWD_RULE_REQUEST:
        {
            struct setFwdRuleRequestALME *m;

            m = (struct setFwdRuleRequestALME *)memory_structure;

            if (m->addresses_nr >0 && NULL != m->addresses)
            {
                PLATFORM_FREE(m->addresses);
            }
            PLATFORM_FREE(m);

            return;
        }

        case ALME_TYPE_GET_FWD_RULES_RESPONSE:
        {
            struct getFwdRulesResponseALME *m;
            INT8U i;

            m = (struct getFwdRulesResponseALME *)memory_structure;

            if (m->rules_nr > 0 && NULL != m->rules)
            {
                for (i=0; i < m->rules_nr; i++)
                {
                    if (m->rules[i].addresses)
                    {
                        PLATFORM_FREE(m->rules[i].addresses);
                    }
                }
                PLATFORM_FREE(m->rules);
            }
            PLATFORM_FREE(m);

            return;
        }

        case ALME_TYPE_MODIFY_FWD_RULE_REQUEST:
        {
            struct modifyFwdRuleRequestALME *m;

            m = (struct modifyFwdRuleRequestALME *)memory_structure;

            if (m->addresses_nr > 0 && NULL != m->addresses)
            {
                PLATFORM_FREE(m->addresses);
            }
            PLATFORM_FREE(m);

            return;
        }

        case ALME_TYPE_GET_METRIC_RESPONSE:
        {
            struct getMetricResponseALME *m;
            INT8U i;

            m = (struct getMetricResponseALME *)memory_structure;

            if (m->metrics_nr > 0 && NULL != m->metrics)
            {
                for (i=0; i < m->metrics_nr; i++)
                {
                    free_1905_TLV_structure((INT8U *)m->metrics[i].tx_metric);
                    free_1905_TLV_structure((INT8U *)m->metrics[i].rx_metric);
                }
                PLATFORM_FREE(m->metrics);
            }
            PLATFORM_FREE(m);


            return;
        }

        case ALME_TYPE_CUSTOM_COMMAND_RESPONSE:
        {
            struct customCommandResponseALME *m;

            m = (struct customCommandResponseALME *)memory_structure;

            if (m->bytes_nr > 0 && NULL != m->bytes)
            {
                PLATFORM_FREE(m->bytes);
            }
            PLATFORM_FREE(m);

            return;
        }


        default:
        {
            // Ignore
            //
            return;
        }
    }

    // This code cannot be reached
    //
    return;
}


INT8U compare_1905_ALME_structures(INT8U *memory_structure_1, INT8U *memory_structure_2)
{
    if (NULL == memory_structure_1 || NULL == memory_structure_2)
    {
        return 1;
    }

    // The first byte of any of the valid structures is always the "tlv_type"
    // field.
    //
    if (*memory_structure_1 != *memory_structure_2)
    {
        return 1;
    }
    switch (*memory_structure_1)
    {
        case ALME_TYPE_GET_INTF_LIST_REQUEST:
        {
            // Nothing to compare (this ALME primitive is always empty)
            //
            return 0;
        }

        case ALME_TYPE_GET_INTF_LIST_RESPONSE:
        {
            struct getIntfListResponseALME *p1, *p2;
            INT8U i, j;

            p1 = (struct getIntfListResponseALME *)memory_structure_1;
            p2 = (struct getIntfListResponseALME *)memory_structure_2;

            if (p1->interface_descriptors_nr !=  p2->interface_descriptors_nr)
            {
                return 1;
            }

            if (p1->interface_descriptors_nr > 0 && (NULL == p1->interface_descriptors || NULL == p2->interface_descriptors))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->interface_descriptors_nr; i++)
            {
                if (
                     PLATFORM_MEMCMP(p1->interface_descriptors[i].interface_address,           p2->interface_descriptors[i].interface_address, 6)    ||
                                     p1->interface_descriptors[i].interface_type           !=  p2->interface_descriptors[i].interface_type           ||
                                     p1->interface_descriptors[i].bridge_flag              !=  p2->interface_descriptors[i].bridge_flag              ||
                                     p1->interface_descriptors[i].vendor_specific_info_nr  !=  p2->interface_descriptors[i].vendor_specific_info_nr
                   )
                {
                    return 1;
                }

                if (p1->interface_descriptors[i].vendor_specific_info_nr > 0 && (NULL == p1->interface_descriptors[i].vendor_specific_info || NULL == p2->interface_descriptors[i].vendor_specific_info))
                {
                    // Malformed structure
                    //
                    return 1;
                }

                for (j=0; j<p1->interface_descriptors[i].vendor_specific_info_nr; j++)
                {
                    if (
                                         p1->interface_descriptors[i].vendor_specific_info[j].ie_type       !=  p2->interface_descriptors[i].vendor_specific_info[j].ie_type                  ||
                                         p1->interface_descriptors[i].vendor_specific_info[j].length_field  !=  p2->interface_descriptors[i].vendor_specific_info[j].length_field             ||
                         PLATFORM_MEMCMP(p1->interface_descriptors[i].vendor_specific_info[j].oui,              p2->interface_descriptors[i].vendor_specific_info[j].oui, 3)                  ||
                         PLATFORM_MEMCMP(p1->interface_descriptors[i].vendor_specific_info[j].vendor_si,        p2->interface_descriptors[i].vendor_specific_info[j].vendor_si, p1->interface_descriptors[i].vendor_specific_info[j].length_field - 3)
                       )
                    {
                        return 1;
                    }

                }
            }

            return 0;
        }

        case ALME_TYPE_SET_INTF_PWR_STATE_REQUEST:
        {
            struct setIntfPwrStateRequestALME *p1, *p2;

            p1 = (struct setIntfPwrStateRequestALME *)memory_structure_1;
            p2 = (struct setIntfPwrStateRequestALME *)memory_structure_2;

            if (
                 PLATFORM_MEMCMP(p1->interface_address,      p2->interface_address, 6)    ||
                                 p1->power_state         !=  p2->power_state
               )
            {
                return 1;
            }

            return 0;
        }

        case ALME_TYPE_SET_INTF_PWR_STATE_CONFIRM:
        {
            struct setIntfPwrStateConfirmALME *p1, *p2;

            p1 = (struct setIntfPwrStateConfirmALME *)memory_structure_1;
            p2 = (struct setIntfPwrStateConfirmALME *)memory_structure_2;

            if (
                 PLATFORM_MEMCMP(p1->interface_address,      p2->interface_address, 6)    ||
                                 p1->reason_code         !=  p2->reason_code
               )
            {
                return 1;
            }

            return 0;
        }

        case ALME_TYPE_GET_INTF_PWR_STATE_REQUEST:
        {
            struct getIntfPwrStateRequestALME *p1, *p2;

            p1 = (struct getIntfPwrStateRequestALME *)memory_structure_1;
            p2 = (struct getIntfPwrStateRequestALME *)memory_structure_2;

            if (PLATFORM_MEMCMP(p1->interface_address, p2->interface_address, 6))
            {
                return 1;
            }

            return 0;
        }

        case ALME_TYPE_GET_INTF_PWR_STATE_RESPONSE:
        {
            struct getIntfPwrStateResponseALME *p1, *p2;

            p1 = (struct getIntfPwrStateResponseALME *)memory_structure_1;
            p2 = (struct getIntfPwrStateResponseALME *)memory_structure_2;

            if (
                 PLATFORM_MEMCMP(p1->interface_address,      p2->interface_address, 6)    ||
                                 p1->power_state         !=  p2->power_state
               )
            {
                return 1;
            }

            return 0;
        }

        case ALME_TYPE_SET_FWD_RULE_REQUEST:
        {
            struct setFwdRuleRequestALME *p1, *p2;
            INT8U i;

            p1 = (struct setFwdRuleRequestALME *)memory_structure_1;
            p2 = (struct setFwdRuleRequestALME *)memory_structure_2;

            if (
                 PLATFORM_MEMCMP(p1->classification_set.mac_da,              p2->classification_set.mac_da, 6)      ||
                                 p1->classification_set.mac_da_flag      !=  p2->classification_set.mac_da_flag     ||
                 PLATFORM_MEMCMP(p1->classification_set.mac_sa,              p2->classification_set.mac_sa, 6)      ||
                                 p1->classification_set.mac_sa_flag      !=  p2->classification_set.mac_sa_flag     ||
                                 p1->classification_set.ether_type       !=  p2->classification_set.ether_type      ||
                                 p1->classification_set.ether_type_flag  !=  p2->classification_set.ether_type_flag ||
                                 p1->classification_set.vid              !=  p2->classification_set.vid             ||
                                 p1->classification_set.vid_flag         !=  p2->classification_set.vid_flag        ||
                                 p1->classification_set.pcp              !=  p2->classification_set.pcp             ||
                                 p1->classification_set.pcp_flag         !=  p2->classification_set.pcp_flag        ||
                                 p1->addresses_nr                        !=  p2->addresses_nr
               )
            {
                return 1;
            }

            if (p1->addresses_nr > 0 && (NULL == p1->addresses || NULL == p2->addresses))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->addresses_nr; i++)
            {
                if (PLATFORM_MEMCMP(p1->addresses[i], p2->addresses[i], 6))
                {
                    return 1;
                }
            }

            return 0;
        }

        case ALME_TYPE_SET_FWD_RULE_CONFIRM:
        {
            struct setFwdRuleConfirmALME *p1, *p2;

            p1 = (struct setFwdRuleConfirmALME *)memory_structure_1;
            p2 = (struct setFwdRuleConfirmALME *)memory_structure_2;

            if (
                 p1->rule_id      !=  p2->rule_id       ||
                 p1->reason_code  !=  p2->reason_code
               )
            {
                return 1;
            }

            return 0;
        }

        case ALME_TYPE_GET_FWD_RULES_REQUEST:
        {
            // Nothing to compare (this ALME primitive is always empty)
            //
            return 0;
        }

        case ALME_TYPE_GET_FWD_RULES_RESPONSE:
        {
            struct getFwdRulesResponseALME *p1, *p2;
            INT8U i, j;

            p1 = (struct getFwdRulesResponseALME *)memory_structure_1;
            p2 = (struct getFwdRulesResponseALME *)memory_structure_2;

            if (p1->rules_nr != p2->rules_nr)
            {
                return 1;
            }

            if (p1->rules_nr > 0 && (NULL == p1->rules || NULL == p2->rules))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->rules_nr; i++)
            {
                if (
                     PLATFORM_MEMCMP(p1->rules[i].classification_set.mac_da,              p2->rules[i].classification_set.mac_da, 6)      ||
                                     p1->rules[i].classification_set.mac_da_flag      !=  p2->rules[i].classification_set.mac_da_flag     ||
                     PLATFORM_MEMCMP(p1->rules[i].classification_set.mac_sa,              p2->rules[i].classification_set.mac_sa, 6)      ||
                                     p1->rules[i].classification_set.mac_sa_flag      !=  p2->rules[i].classification_set.mac_sa_flag     ||
                                     p1->rules[i].classification_set.ether_type       !=  p2->rules[i].classification_set.ether_type      ||
                                     p1->rules[i].classification_set.ether_type_flag  !=  p2->rules[i].classification_set.ether_type_flag ||
                                     p1->rules[i].classification_set.vid              !=  p2->rules[i].classification_set.vid             ||
                                     p1->rules[i].classification_set.vid_flag         !=  p2->rules[i].classification_set.vid_flag        ||
                                     p1->rules[i].classification_set.pcp              !=  p2->rules[i].classification_set.pcp             ||
                                     p1->rules[i].classification_set.pcp_flag         !=  p2->rules[i].classification_set.pcp_flag        ||
                                     p1->rules[i].addresses_nr                        !=  p2->rules[i].addresses_nr                       ||
                                     p1->rules[i].last_matched                        !=  p2->rules[i].last_matched
                   )
                {
                    return 1;
                }

                if (p1->rules[i].addresses_nr > 0 && (NULL == p1->rules[i].addresses || NULL == p2->rules[i].addresses))
                {
                    // Malformed structure
                    //
                    return 1;
                }

                for (j=0; j<p1->rules[i].addresses_nr; j++)
                {
                    if (PLATFORM_MEMCMP(p1->rules[i].addresses[j], p2->rules[i].addresses[j], 6))
                    {
                        return 1;
                    }
                }
            }

            return 0;
        }

        case ALME_TYPE_MODIFY_FWD_RULE_REQUEST:
        {
            struct modifyFwdRuleRequestALME *p1, *p2;
            INT8U i;

            p1 = (struct modifyFwdRuleRequestALME *)memory_structure_1;
            p2 = (struct modifyFwdRuleRequestALME *)memory_structure_2;

            if (
                 p1->rule_id       !=  p2->rule_id      ||
                 p1->addresses_nr  !=  p2->addresses_nr
               )
            {
                return 1;
            }

            if (p1->addresses_nr > 0 && (NULL == p1->addresses || NULL == p2->addresses))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->addresses_nr; i++)
            {
                if (PLATFORM_MEMCMP(p1->addresses[i], p2->addresses[i], 6))
                {
                    return 1;
                }
            }

            return 0;
        }

        case ALME_TYPE_MODIFY_FWD_RULE_CONFIRM:
        {
            struct modifyFwdRuleConfirmALME *p1, *p2;

            p1 = (struct modifyFwdRuleConfirmALME *)memory_structure_1;
            p2 = (struct modifyFwdRuleConfirmALME *)memory_structure_2;

            if (
                 p1->rule_id      !=  p2->rule_id      ||
                 p1->reason_code  !=  p2->reason_code
               )
            {
                return 1;
            }

            return 0;
        }

        case ALME_TYPE_REMOVE_FWD_RULE_REQUEST:
        {
            struct removeFwdRuleRequestALME *p1, *p2;

            p1 = (struct removeFwdRuleRequestALME *)memory_structure_1;
            p2 = (struct removeFwdRuleRequestALME *)memory_structure_2;

            if (p1->rule_id != p2->rule_id)
            {
                return 1;
            }

            return 0;
        }

        case ALME_TYPE_REMOVE_FWD_RULE_CONFIRM:
        {
            struct removeFwdRuleConfirmALME *p1, *p2;

            p1 = (struct removeFwdRuleConfirmALME *)memory_structure_1;
            p2 = (struct removeFwdRuleConfirmALME *)memory_structure_2;

            if (
                 p1->rule_id      !=  p2->rule_id      ||
                 p1->reason_code  !=  p2->reason_code
               )
            {
                return 1;
            }

            return 0;
        }

        case ALME_TYPE_GET_METRIC_REQUEST:
        {
            struct getMetricRequestALME *p1, *p2;

            p1 = (struct getMetricRequestALME *)memory_structure_1;
            p2 = (struct getMetricRequestALME *)memory_structure_2;

            if (PLATFORM_MEMCMP(p1->interface_address, p2->interface_address, 6))
            {
                return 1;
            }

            return 0;
        }

        case ALME_TYPE_GET_METRIC_RESPONSE:
        {
            struct getMetricResponseALME *p1, *p2;
            INT8U i;

            p1 = (struct getMetricResponseALME *)memory_structure_1;
            p2 = (struct getMetricResponseALME *)memory_structure_2;

            if (
                    p1->metrics_nr   !=  p2->metrics_nr   ||
                    p1->reason_code  !=  p2->reason_code
               )

            {
                return 1;
            }

            if (p1->metrics_nr > 0 && (NULL == p1->metrics || NULL == p2->metrics))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->metrics_nr; i++)
            {
                if (
                                           PLATFORM_MEMCMP(p1->metrics[i].neighbor_dev_address,               p2->metrics[i].neighbor_dev_address, 6)      ||
                                           PLATFORM_MEMCMP(p1->metrics[i].local_intf_address,                 p2->metrics[i].local_intf_address,   6)      ||
                                                           p1->metrics[i].bridge_flag            !=           p2->metrics[i].bridge_flag                   ||
                     compare_1905_TLV_structures((INT8U *)p1->metrics[i].tx_metric,                  (INT8U *)p2->metrics[i].tx_metric)                    ||
                     compare_1905_TLV_structures((INT8U *)p1->metrics[i].rx_metric,                  (INT8U *)p2->metrics[i].rx_metric)

                   )
                {
                    return 1;
                }
            }

            return 0;
        }

        case ALME_TYPE_CUSTOM_COMMAND_REQUEST:
        {
            struct customCommandRequestALME *p1, *p2;

            p1 = (struct customCommandRequestALME *)memory_structure_1;
            p2 = (struct customCommandRequestALME *)memory_structure_2;

            if (p1->command !=  p2->command)
            {
                return 1;
            }

            return 0;
        }

        case ALME_TYPE_CUSTOM_COMMAND_RESPONSE:
        {
            struct customCommandResponseALME *p1, *p2;

            p1 = (struct customCommandResponseALME *)memory_structure_1;
            p2 = (struct customCommandResponseALME *)memory_structure_2;

            if (
                                 p1->bytes_nr  !=  p2->bytes_nr             ||
                 PLATFORM_MEMCMP(p1->bytes,        p2->bytes, p1->bytes_nr)
               )

            {
                return 1;
            }

            return 0;
        }

        default:
        {
            // Unknown structure type
            //
            return 1;
        }
    }

    // This code cannot be reached
    //
    return 1;
}


void visit_1905_ALME_structure(INT8U *memory_structure, visitor_callback callback, void (*write_function)(const char *fmt, ...), const char *prefix)
{
    // Buffer size to store a prefix string that will be used to show each
    // element of a structure on screen
    //
    #define MAX_PREFIX  100

    if (NULL == memory_structure)
    {
        return;
    }

    // The first byte of any of the valid structures is always the "tlv_type"
    // field.
    //
    switch (*memory_structure)
    {
        case ALME_TYPE_GET_INTF_LIST_REQUEST:
        {
            // There is nothing to visit. This TLV is always empty
            //
            return;
        }

        case ALME_TYPE_GET_INTF_LIST_RESPONSE:
        {
            struct getIntfListResponseALME *p;
            INT8U i, j;

            p = (struct getIntfListResponseALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->interface_descriptors_nr), "interface_descriptors_nr",  "%d",  &p->interface_descriptors_nr);

            for (i=0; i < p->interface_descriptors_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sinterface_descriptors[%d]->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->interface_descriptors[i].interface_address),       "interface_address",        "0x%02x", p->interface_descriptors[i].interface_address);
                callback(write_function, new_prefix, sizeof(p->interface_descriptors[i].interface_type),          "media_type",               "%d",    &p->interface_descriptors[i].interface_type);
                callback(write_function, new_prefix, sizeof(p->interface_descriptors[i].bridge_flag),             "bridge_flag",              "%d",    &p->interface_descriptors[i].bridge_flag);
                callback(write_function, new_prefix, sizeof(p->interface_descriptors[i].vendor_specific_info_nr), "vendor_specific_info_nr",  "%d",    &p->interface_descriptors[i].vendor_specific_info_nr);

                for (j=0; j < p->interface_descriptors[i].vendor_specific_info_nr; j++)
                {
                    PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sinterface_descriptors[%d]->vendor_specific_info[%d]->", prefix, i, j);
                    new_prefix[MAX_PREFIX-1] = 0x0;

                    callback(write_function, new_prefix, sizeof(p->interface_descriptors[i].vendor_specific_info[j].ie_type),      "ie_type",      "%d",     &p->interface_descriptors[i].vendor_specific_info[j].ie_type);
                    callback(write_function, new_prefix, sizeof(p->interface_descriptors[i].vendor_specific_info[j].length_field), "length_field", "%d",     &p->interface_descriptors[i].vendor_specific_info[j].length_field);
                    callback(write_function, new_prefix, sizeof(p->interface_descriptors[i].vendor_specific_info[j].oui),          "oui",          "0x%02x",  p->interface_descriptors[i].vendor_specific_info[j].oui);
                    callback(write_function, new_prefix, p->interface_descriptors[i].vendor_specific_info[j].length_field - 3,     "vendor_si",    "0x%02x",  p->interface_descriptors[i].vendor_specific_info[j].vendor_si);
                }
            }

            return;
        }

        case ALME_TYPE_SET_INTF_PWR_STATE_REQUEST:
        {
            struct setIntfPwrStateRequestALME *p;

            p = (struct setIntfPwrStateRequestALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->interface_address),  "interface_address",  "0x%02x",  p->interface_address);
            callback(write_function, prefix, sizeof(p->power_state),        "power_state",        "%d",     &p->power_state);

            return;
        }

        case ALME_TYPE_SET_INTF_PWR_STATE_CONFIRM:
        {
            struct setIntfPwrStateConfirmALME *p;

            p = (struct setIntfPwrStateConfirmALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->interface_address),  "interface_address",  "0x%02x",   p->interface_address);
            callback(write_function, prefix, sizeof(p->reason_code),        "reason_code",        "%d",      &p->reason_code);

            return;
        }

        case ALME_TYPE_GET_INTF_PWR_STATE_REQUEST:
        {
            struct getIntfPwrStateRequestALME *p;

            p = (struct getIntfPwrStateRequestALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->interface_address),  "interface_address",  "0x%02x",   p->interface_address);

            return;
        }

        case ALME_TYPE_GET_INTF_PWR_STATE_RESPONSE:
        {
            struct getIntfPwrStateResponseALME *p;

            p = (struct getIntfPwrStateResponseALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->interface_address),  "interface_address",  "0x%02x",   p->interface_address);
            callback(write_function, prefix, sizeof(p->power_state),        "power_state",        "%d",      &p->power_state);

            return;
        }

        case ALME_TYPE_SET_FWD_RULE_REQUEST:
        {
            struct setFwdRuleRequestALME *p;
            INT8U i;

            p = (struct setFwdRuleRequestALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->classification_set.mac_da),           "classification_set.mac_da",           "0x%02x",  p->classification_set.mac_da);
            callback(write_function, prefix, sizeof(p->classification_set.mac_da_flag),      "classification_set.mac_da_flag",      "%d",     &p->classification_set.mac_da_flag);
            callback(write_function, prefix, sizeof(p->classification_set.mac_sa),           "classification_set.mac_sa",           "0x%02x",  p->classification_set.mac_sa);
            callback(write_function, prefix, sizeof(p->classification_set.mac_sa_flag),      "classification_set.mac_sa_flag",      "%d",     &p->classification_set.mac_sa_flag);
            callback(write_function, prefix, sizeof(p->classification_set.ether_type),       "classification_set.ether_type",       "%d",     &p->classification_set.ether_type);
            callback(write_function, prefix, sizeof(p->classification_set.ether_type_flag),  "classification_set.ether_type_flag",  "%d",     &p->classification_set.ether_type_flag);
            callback(write_function, prefix, sizeof(p->classification_set.vid),              "classification_set.vid",              "%d",     &p->classification_set.vid);
            callback(write_function, prefix, sizeof(p->classification_set.vid_flag),         "classification_set.vid_flag",         "%d",     &p->classification_set.vid_flag);
            callback(write_function, prefix, sizeof(p->classification_set.pcp),              "classification_set.pcp",              "%d",     &p->classification_set.pcp);
            callback(write_function, prefix, sizeof(p->classification_set.pcp_flag),         "classification_set.pcp_flag",         "%d",     &p->classification_set.pcp_flag);
            callback(write_function, prefix, sizeof(p->addresses_nr),                        "addresses_nr",                        "%d",     &p->addresses_nr);

            for (i=0; i < p->addresses_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%saddresses[%d]->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->addresses[i]), "", "0x%02x",  p->addresses[i]);
            }

            return;
        }

        case ALME_TYPE_SET_FWD_RULE_CONFIRM:
        {
            struct setFwdRuleConfirmALME *p;

            p = (struct setFwdRuleConfirmALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->rule_id),      "rule_id",      "%d",  &p->reason_code);
            callback(write_function, prefix, sizeof(p->reason_code),  "reason_code",  "%d",  &p->reason_code);

            return;
        }

        case ALME_TYPE_GET_FWD_RULES_REQUEST:
        {
            // There is nothing to visit. This TLV is always empty
            //
            return;
        }

        case ALME_TYPE_GET_FWD_RULES_RESPONSE:
        {
            struct getFwdRulesResponseALME *p;
            INT8U i, j;

            p = (struct getFwdRulesResponseALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->rules_nr), "rules_nr",  "%d",  &p->rules_nr);

            for (i=0; i < p->rules_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%srules[%d]->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->rules[i].addresses_nr), "addresses_nr",  "%d",  &p->rules[i].addresses_nr);

                for (j=0; j < p->rules[i].addresses_nr; j++)
                {
                    PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%srules[%d]->addresses[%d]", prefix, i, j);
                    new_prefix[MAX_PREFIX-1] = 0x0;

                    callback(write_function, new_prefix, sizeof(p->rules[i].addresses[j]), "", "0x%02x",  p->rules[i].addresses[j]);
                }

                callback(write_function, new_prefix, sizeof(p->rules[i].last_matched), "last_matched",  "%d",  &p->rules[i].last_matched);
            }

            return;
        }

        case ALME_TYPE_MODIFY_FWD_RULE_REQUEST:
        {
            struct modifyFwdRuleRequestALME *p;
            INT8U i;

            p = (struct modifyFwdRuleRequestALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->rule_id),      "rule_id",       "%d",  &p->rule_id);
            callback(write_function, prefix, sizeof(p->addresses_nr), "addresses_nr",  "%d",  &p->addresses_nr);

            for (i=0; i < p->addresses_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%saddresses[%d]->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->addresses[i]), "", "0x%02x",  p->addresses[i]);
            }

            return;
        }

        case ALME_TYPE_MODIFY_FWD_RULE_CONFIRM:
        {
            struct modifyFwdRuleConfirmALME *p;

            p = (struct modifyFwdRuleConfirmALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->rule_id),     "rule_id",      "%d",  &p->rule_id);
            callback(write_function, prefix, sizeof(p->reason_code), "reason_code",  "%d",  &p->reason_code);

            return;
        }

        case ALME_TYPE_REMOVE_FWD_RULE_REQUEST:
        {
            struct removeFwdRuleRequestALME *p;

            p = (struct removeFwdRuleRequestALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->rule_id), "rule_id",      "%d",  &p->rule_id);

            return;
        }

        case ALME_TYPE_REMOVE_FWD_RULE_CONFIRM:
        {
            struct removeFwdRuleConfirmALME *p;

            p = (struct removeFwdRuleConfirmALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->rule_id),     "rule_id",      "%d",  &p->rule_id);
            callback(write_function, prefix, sizeof(p->reason_code), "reason_code",  "%d",  &p->reason_code);

            return;
        }

        case ALME_TYPE_GET_METRIC_REQUEST:
        {
            struct getMetricRequestALME *p;

            p = (struct getMetricRequestALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->interface_address),  "rule_id",  "0x%02x",  p->interface_address);

            return;
        }

        case ALME_TYPE_GET_METRIC_RESPONSE:
        {
            struct getMetricResponseALME *p;
            INT8U i;

            p = (struct getMetricResponseALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->metrics_nr), "metrics_nr",  "%d",  &p->metrics_nr);

            for (i=0; i < p->metrics_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%smetrics[%d]->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->metrics[i].neighbor_dev_address), "neighbor_dev_address",        "0x%02x",   p->metrics[i].neighbor_dev_address);
                callback(write_function, new_prefix, sizeof(p->metrics[i].local_intf_address),   "local_intf_address",          "0x%02x",   p->metrics[i].local_intf_address);
                callback(write_function, new_prefix, sizeof(p->metrics[i].bridge_flag)   ,       "bridge_flag",                 "%d",      &p->metrics[i].bridge_flag);

                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%smetrics[%d]->tx_metric->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                visit_1905_TLV_structure((INT8U *)p->metrics[i].tx_metric, callback, write_function, new_prefix);

                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%smetrics[%d]->rx_metric->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                visit_1905_TLV_structure((INT8U *)p->metrics[i].rx_metric, callback, write_function, new_prefix);
            }

            return;
        }

        case ALME_TYPE_CUSTOM_COMMAND_REQUEST:
        {
            struct customCommandRequestALME *p;

            p = (struct customCommandRequestALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->command),  "command", "%d", &p->command);

            return;
        }

        case ALME_TYPE_CUSTOM_COMMAND_RESPONSE:
        {
            struct customCommandResponseALME *p;

            p = (struct customCommandResponseALME *)memory_structure;

            callback(write_function, prefix, sizeof(p->bytes_nr),  "bytes_nr", "%d",   &p->bytes_nr);
            callback(write_function, prefix, p->bytes_nr,          "bytes",    "%s",  p->bytes);

            return;
        }

        default:
        {
            // Ignore
            //
            return;
        }
    }

    // This code cannot be reached
    //
    return;
}

char *convert_1905_ALME_type_to_string(INT8U alme_type)
{
    switch (alme_type)
    {
        case ALME_TYPE_GET_INTF_LIST_REQUEST:
            return "ALME_TYPE_GET_INTF_LIST_REQUEST";
        case ALME_TYPE_GET_INTF_LIST_RESPONSE:
            return "ALME_TYPE_GET_INTF_LIST_RESPONSE";
        case ALME_TYPE_SET_INTF_PWR_STATE_REQUEST:
            return "ALME_TYPE_SET_INTF_PWR_STATE_REQUEST";
        case ALME_TYPE_SET_INTF_PWR_STATE_CONFIRM:
            return "ALME_TYPE_SET_INTF_PWR_STATE_CONFIRM";
        case ALME_TYPE_GET_INTF_PWR_STATE_REQUEST:
            return "ALME_TYPE_GET_INTF_PWR_STATE_REQUEST";
        case ALME_TYPE_GET_INTF_PWR_STATE_RESPONSE:
            return "ALME_TYPE_GET_INTF_PWR_STATE_RESPONSE";
        case ALME_TYPE_SET_FWD_RULE_REQUEST:
            return "ALME_TYPE_SET_FWD_RULE_REQUEST";
        case ALME_TYPE_SET_FWD_RULE_CONFIRM:
            return "ALME_TYPE_SET_FWD_RULE_CONFIRM";
        case ALME_TYPE_GET_FWD_RULES_REQUEST:
            return "ALME_TYPE_GET_FWD_RULES_REQUEST";
        case ALME_TYPE_GET_FWD_RULES_RESPONSE:
            return "ALME_TYPE_GET_FWD_RULES_RESPONSE";
        case ALME_TYPE_MODIFY_FWD_RULE_REQUEST:
            return "ALME_TYPE_MODIFY_FWD_RULE_REQUEST";
        case ALME_TYPE_MODIFY_FWD_RULE_CONFIRM:
            return "ALME_TYPE_MODIFY_FWD_RULE_CONFIRM";
        case ALME_TYPE_REMOVE_FWD_RULE_REQUEST:
            return "ALME_TYPE_REMOVE_FWD_RULE_REQUEST";
        case ALME_TYPE_REMOVE_FWD_RULE_CONFIRM:
            return "ALME_TYPE_REMOVE_FWD_RULE_CONFIRM";
        case ALME_TYPE_GET_METRIC_REQUEST:
            return "ALME_TYPE_GET_METRIC_REQUEST";
        case ALME_TYPE_GET_METRIC_RESPONSE:
            return "ALME_TYPE_GET_METRIC_RESPONSE";
        default:
            return "Unknown";
    }
}
