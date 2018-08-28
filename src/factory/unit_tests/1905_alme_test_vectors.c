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

#include "1905_alme.h"
#include "1905_tlvs.h"


// This file contains test vectors than can be used to check the
// "parse_1905_ALME_from_packet()" and "forge_1905_ALME_from_structure()"
// functions
//
// Each test vector is made up of three variables:
//
//   - A ALME primitive structure
//   - An array of bits representing the network packet
//   - An variable holding the length of the packet
//
// Note that some test vectors can be used to test both functions, while others
// can only be used to test "forge_1905_ALME_from_structure()" or
// "parse_1905_ALME_from_packet()":
//
//   - Test vectors marked with "ALME --> packet" can only be used to test the
//     "forge_1905_ALME_from_structure()" function.
//
//   - Test vectors marked with "ALME <-- packet" can only be used to test the
//     "parse_1905_ALME_from_packet()" function.
//
//   - All the other test vectors are marked with "ALME <--> packet", meaning
//     they can be used to test  both functions.
//
// The reason this is happening is that, according to the standard, sometimes
// bits are ignored/changed/forced-to-a-value when forging a packet. Thus, not
// all test vectors are "inversible" (ie. forge(parse(stream)) != x)
//
// This is how you use these test vectors:
//
//   A) stream = forge_1905_ALME_from_structure(alme_xxx, &stream_len);
//
//   B) tlv = parse_1905_ALME_from_packet(stream_xxx);
//

// NOTE: The translation from/to ALME primitive to/from packet stream is not
//       part of the 1905 standard.
//       The test vectors included in this file follow the packet stream
//       description contained in the comments section at the beginning of file
//       "1905_alme.c"

////////////////////////////////////////////////////////////////////////////////
//// Test vector 001 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct getIntfListRequestALME x1905_alme_structure_001 =
{
    .alme_type         = ALME_TYPE_GET_INTF_LIST_REQUEST,
};

uint8_t x1905_alme_stream_001[] =
{
    0x01,
};

uint16_t x1905_alme_stream_len_001 = 1;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 002 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct getIntfListResponseALME x1905_alme_structure_002 =
{
    .alme_type                 = ALME_TYPE_GET_INTF_LIST_RESPONSE,
    .interface_descriptors_nr  = 1,
    .interface_descriptors     =
        (struct _intfDescriptorEntries[]){
            {
                .interface_address       = {0x01, 0x02, 0x03, 0x01, 0x02, 0x03},
                .interface_type          = MEDIA_TYPE_IEEE_802_11AF_GHZ,
                .bridge_flag             = 0x01,
                .vendor_specific_info_nr = 0,
                .vendor_specific_info    = NULL,
            },
        }
};

uint8_t x1905_alme_stream_002[] =
{
    0x02,
    0x01,
    0x01, 0x02, 0x03, 0x01, 0x02, 0x03,
    0x01, 0x07,
    0x01,
    0x00,
};

uint16_t x1905_alme_stream_len_002 = 12;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 003 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct getIntfListResponseALME x1905_alme_structure_003 =
{
    .alme_type                 = ALME_TYPE_GET_INTF_LIST_RESPONSE,
    .interface_descriptors_nr  = 2,
    .interface_descriptors     =
        (struct _intfDescriptorEntries[]){
            {
                .interface_address       = {0x01, 0x02, 0x03, 0x01, 0x02, 0x03},
                .interface_type          = MEDIA_TYPE_IEEE_802_11AF_GHZ,
                .bridge_flag             = 0x01,
                .vendor_specific_info_nr = 2,
                .vendor_specific_info    =
                    (struct _vendorSpecificInfoEntries[]){
                        {
                            .ie_type      = 1,
                            .length_field = 11,
                            .oui          = {0x0a, 0x0b, 0x0c},
                            .vendor_si    = (uint8_t []){0xde, 0xde, 0xde, 0xde, 0xde, 0xde, 0xde, 0xaa},
                        },
                        {
                            .ie_type      = 1,
                            .length_field = 4,
                            .oui          = {0x0d, 0x0e, 0x0f},
                            .vendor_si    = (uint8_t []){0xff},
                        },
                    },
            },
            {
                .interface_address       = {0x01, 0x02, 0x03, 0x01, 0x02, 0x04},
                .interface_type          = MEDIA_TYPE_IEEE_1901_WAVELET,
                .bridge_flag             = 0x00,
                .vendor_specific_info_nr = 0,
                .vendor_specific_info    = NULL,
            },
        }
};

uint8_t x1905_alme_stream_003[] =
{
    0x02,
    0x02,
    0x01, 0x02, 0x03, 0x01, 0x02, 0x03,
    0x01, 0x07,
    0x01,
    0x02,
    0x00, 0x01,
    0x00, 0x0b,
    0x0a, 0x0b, 0x0c,
    0xde, 0xde, 0xde, 0xde, 0xde, 0xde, 0xde, 0xaa,
    0x00, 0x01,
    0x00, 0x04,
    0x0d, 0x0e, 0x0f,
    0xff,
    0x01, 0x02, 0x03, 0x01, 0x02, 0x04,
    0x02, 0x00,
    0x00,
    0x00,
};

uint16_t x1905_alme_stream_len_003 = 45;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 004 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct getIntfListResponseALME x1905_alme_structure_004 =
{
    .alme_type                 = ALME_TYPE_GET_INTF_LIST_RESPONSE,
    .interface_descriptors_nr  = 0,
    .interface_descriptors     = NULL,
};

uint8_t x1905_alme_stream_004[] =
{
    0x02,
    0x00,
};

uint16_t x1905_alme_stream_len_004 = 2;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 005 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct setIntfPwrStateRequestALME x1905_alme_structure_005 =
{
    .alme_type                 = ALME_TYPE_SET_INTF_PWR_STATE_REQUEST,
    .interface_address         = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
    .power_state               = POWER_STATE_PWR_ON,
};

uint8_t x1905_alme_stream_005[] =
{
    0x03,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x00,
};

uint16_t x1905_alme_stream_len_005 = 8;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 006 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct setIntfPwrStateRequestALME x1905_alme_structure_006 =
{
    .alme_type                 = ALME_TYPE_SET_INTF_PWR_STATE_REQUEST,
    .interface_address         = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
    .power_state               = POWER_STATE_PWR_OFF,
};

uint8_t x1905_alme_stream_006[] =
{
    0x03,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x02,
};

uint16_t x1905_alme_stream_len_006 = 8;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 007 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct setIntfPwrStateConfirmALME x1905_alme_structure_007 =
{
    .alme_type                 = ALME_TYPE_SET_INTF_PWR_STATE_CONFIRM,
    .interface_address         = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
    .reason_code               = REASON_CODE_SUCCESS,
};

uint8_t x1905_alme_stream_007[] =
{
    0x04,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x00,
};

uint16_t x1905_alme_stream_len_007 = 8;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 008 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct setIntfPwrStateConfirmALME x1905_alme_structure_008 =
{
    .alme_type                 = ALME_TYPE_SET_INTF_PWR_STATE_CONFIRM,
    .interface_address         = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
    .reason_code               = REASON_CODE_UNAVAILABLE_PWR_STATE,
};

uint8_t x1905_alme_stream_008[] =
{
    0x04,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x03,
};

uint16_t x1905_alme_stream_len_008 = 8;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 009 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct getIntfPwrStateRequestALME x1905_alme_structure_009 =
{
    .alme_type                 = ALME_TYPE_GET_INTF_PWR_STATE_REQUEST,
    .interface_address         = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
};

uint8_t x1905_alme_stream_009[] =
{
    0x05,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
};

uint16_t x1905_alme_stream_len_009 = 7;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 010 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct getIntfPwrStateResponseALME x1905_alme_structure_010 =
{
    .alme_type                 = ALME_TYPE_GET_INTF_PWR_STATE_RESPONSE,
    .interface_address         = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
    .power_state               = POWER_STATE_PWR_SAVE,
};

uint8_t x1905_alme_stream_010[] =
{
    0x06,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x01
};

uint16_t x1905_alme_stream_len_010 = 8;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 011 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct setFwdRuleRequestALME x1905_alme_structure_011 =
{
    .alme_type                 = ALME_TYPE_SET_FWD_RULE_REQUEST,
    .classification_set        =
        {
            .mac_da            = {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5},
            .mac_da_flag       = 1,
            .mac_sa            = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
            .mac_sa_flag       = 0,
            .ether_type        = 0x2020,
            .ether_type_flag   = 1,
            .vid               = 0x00,
            .vid_flag          = 0,
            .pcp               = 0x00,
            .pcp_flag          = 0,
        },
    .addresses_nr              = 1,
    .addresses                 =
        (uint8_t [][6]){
            {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
        },
};

uint8_t x1905_alme_stream_011[] =
{
    0x07,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5,
    0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,
    0x20, 0x20,
    0x01,
    0x00, 0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
};

uint16_t x1905_alme_stream_len_011 = 30;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 012 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct setFwdRuleRequestALME x1905_alme_structure_012 =
{
    .alme_type                 = ALME_TYPE_SET_FWD_RULE_REQUEST,
    .classification_set        =
        {
            .mac_da            = {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5},
            .mac_da_flag       = 1,
            .mac_sa            = {0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0},
            .mac_sa_flag       = 1,
            .ether_type        = 0x2020,
            .ether_type_flag   = 0,
            .vid               = 0x00,
            .vid_flag          = 0,
            .pcp               = 0x00,
            .pcp_flag          = 0,
        },
    .addresses_nr              = 3,
    .addresses                 =
        (uint8_t [][6]){
            {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
            {0x11, 0x12, 0x13, 0x14, 0x15, 0x16},
            {0x21, 0x22, 0x23, 0x24, 0x25, 0x26},
        },
};

uint8_t x1905_alme_stream_012[] =
{
    0x07,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5,
    0x01,
    0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0,
    0x01,
    0x20, 0x20,
    0x00,
    0x00, 0x00,
    0x00,
    0x00,
    0x00,
    0x03,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
};

uint16_t x1905_alme_stream_len_012 = 42;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 013 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct setFwdRuleConfirmALME x1905_alme_structure_013 =
{
    .alme_type                 = ALME_TYPE_SET_FWD_RULE_CONFIRM,
    .rule_id                   = 0x1007,
    .reason_code               = REASON_CODE_SUCCESS,
};

uint8_t x1905_alme_stream_013[] =
{
    0x08,
    0x10, 0x07,
    0x00
};

uint16_t x1905_alme_stream_len_013 = 4;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 014 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct getFwdRulesRequestALME x1905_alme_structure_014 =
{
    .alme_type                 = ALME_TYPE_GET_FWD_RULES_REQUEST,
};

uint8_t x1905_alme_stream_014[] =
{
    0x09,
};

uint16_t x1905_alme_stream_len_014 = 1;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 015 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct getFwdRulesResponseALME x1905_alme_structure_015 =
{
    .alme_type                 = ALME_TYPE_GET_FWD_RULES_RESPONSE,
    .rules_nr                  = 0,
    .rules                     = NULL,
};

uint8_t x1905_alme_stream_015[] =
{
    0x0a,
    0x00,
};

uint16_t x1905_alme_stream_len_015 = 2;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 016 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct getFwdRulesResponseALME x1905_alme_structure_016 =
{
    .alme_type                 = ALME_TYPE_GET_FWD_RULES_RESPONSE,
    .rules_nr                  = 1,
    .rules                     =
        (struct _fwdRuleListEntries[]){
            {
                .classification_set        =
                    {
                        .mac_da            = {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5},
                        .mac_da_flag       = 1,
                        .mac_sa            = {0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0},
                        .mac_sa_flag       = 1,
                        .ether_type        = 0x2020,
                        .ether_type_flag   = 0,
                        .vid               = 0x00,
                        .vid_flag          = 0,
                        .pcp               = 0x00,
                        .pcp_flag          = 0,
                    },
                .addresses_nr              = 3,
                .addresses                 =
                    (uint8_t [][6]){
                        {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
                        {0x11, 0x12, 0x13, 0x14, 0x15, 0x16},
                        {0x21, 0x22, 0x23, 0x24, 0x25, 0x26},
                    },
                .last_matched = 0x00a0,
            },
        },
};

uint8_t x1905_alme_stream_016[] =
{
    0x0a,
    0x01,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5,
    0x01,
    0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0,
    0x01,
    0x20, 0x20,
    0x00,
    0x00, 0x00,
    0x00,
    0x00,
    0x00,
    0x03,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
    0x00, 0xa0,
};

uint16_t x1905_alme_stream_len_016 = 45;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 017 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct getFwdRulesResponseALME x1905_alme_structure_017 =
{
    .alme_type                 = ALME_TYPE_GET_FWD_RULES_RESPONSE,
    .rules_nr                  = 2,
    .rules                     =
        (struct _fwdRuleListEntries[]){
            {
                .classification_set        =
                    {
                        .mac_da            = {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5},
                        .mac_da_flag       = 1,
                        .mac_sa            = {0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0},
                        .mac_sa_flag       = 1,
                        .ether_type        = 0x2020,
                        .ether_type_flag   = 0,
                        .vid               = 0x00,
                        .vid_flag          = 0,
                        .pcp               = 0x00,
                        .pcp_flag          = 0,
                    },
                .addresses_nr              = 3,
                .addresses                 =
                    (uint8_t [][6]){
                        {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
                        {0x11, 0x12, 0x13, 0x14, 0x15, 0x16},
                        {0x21, 0x22, 0x23, 0x24, 0x25, 0x26},
                    },
                .last_matched = 0x00a0,
            },
            {
                .classification_set        =
                    {
                        .mac_da            = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                        .mac_da_flag       = 0,
                        .mac_sa            = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
                        .mac_sa_flag       = 1,
                        .ether_type        = 0x0000,
                        .ether_type_flag   = 0,
                        .vid               = 0x00,
                        .vid_flag          = 0,
                        .pcp               = 0x00,
                        .pcp_flag          = 0,
                    },
                .addresses_nr              = 1,
                .addresses                 =
                    (uint8_t [][6]){
                        {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
                    },
                .last_matched = 0x0000,
            },
        },
};

uint8_t x1905_alme_stream_017[] =
{
    0x0a,
    0x02,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5,
    0x01,
    0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0,
    0x01,
    0x20, 0x20,
    0x00,
    0x00, 0x00,
    0x00,
    0x00,
    0x00,
    0x03,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
    0x00, 0xa0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0x01,
    0x00, 0x00,
    0x00,
    0x00, 0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x00, 0x00,
};

uint16_t x1905_alme_stream_len_017 = 76;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 018 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct modifyFwdRuleRequestALME x1905_alme_structure_018 =
{
    .alme_type                 = ALME_TYPE_MODIFY_FWD_RULE_REQUEST,
    .rule_id                   = 0x011a,
    .addresses_nr              = 2,
    .addresses                 =
        (uint8_t [][6]){
            {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
            {0x11, 0x12, 0x13, 0x14, 0x15, 0x16},
        },
};

uint8_t x1905_alme_stream_018[] =
{
    0x0b,
    0x01, 0x1a,
    0x02,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
};

uint16_t x1905_alme_stream_len_018 = 16;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 019 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct modifyFwdRuleConfirmALME x1905_alme_structure_019 =
{
    .alme_type                 = ALME_TYPE_MODIFY_FWD_RULE_CONFIRM,
    .rule_id                   = 0x011a,
    .reason_code               = REASON_CODE_SUCCESS,
};

uint8_t x1905_alme_stream_019[] =
{
    0x0c,
    0x01, 0x1a,
    0x00,
};

uint16_t x1905_alme_stream_len_019 = 4;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 020 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct modifyFwdRuleConfirmALME x1905_alme_structure_020 =
{
    .alme_type                 = ALME_TYPE_MODIFY_FWD_RULE_CONFIRM,
    .rule_id                   = 0x011a,
    .reason_code               = REASON_CODE_INVALID_RULE_ID,
};

uint8_t x1905_alme_stream_020[] =
{
    0x0c,
    0x01, 0x1a,
    0x05,
};

uint16_t x1905_alme_stream_len_020 = 4;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 021 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct removeFwdRuleRequestALME x1905_alme_structure_021 =
{
    .alme_type                 = ALME_TYPE_REMOVE_FWD_RULE_REQUEST,
    .rule_id                   = 0x011a,
};

uint8_t x1905_alme_stream_021[] =
{
    0x0d,
    0x01, 0x1a,
};

uint16_t x1905_alme_stream_len_021 = 3;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 022 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct removeFwdRuleConfirmALME x1905_alme_structure_022 =
{
    .alme_type                 = ALME_TYPE_REMOVE_FWD_RULE_CONFIRM,
    .rule_id                   = 0x011a,
    .reason_code               = REASON_CODE_SUCCESS,
};

uint8_t x1905_alme_stream_022[] =
{
    0x0e,
    0x01, 0x1a,
    0x00,
};

uint16_t x1905_alme_stream_len_022 = 4;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 023 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct getMetricRequestALME x1905_alme_structure_023 =
{
    .alme_type                 = ALME_TYPE_GET_METRIC_REQUEST,
    .interface_address         = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
};

uint8_t x1905_alme_stream_023[] =
{
    0x0f,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
};

uint16_t x1905_alme_stream_len_023 = 7;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 024 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct getMetricResponseALME x1905_alme_structure_024 =
{
    .alme_type                 = ALME_TYPE_GET_METRIC_RESPONSE,
    .metrics_nr                = 1,
    .metrics                   =
        (struct _metricDescriptorsEntries[]){
            {
                .neighbor_dev_address = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x02},
                .local_intf_address   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
                .bridge_flag          = 0,
                .tx_metric            =
                    (struct transmitterLinkMetricTLV[]){
                        {
                            .tlv.type                    = TLV_TYPE_TRANSMITTER_LINK_METRIC,
                            .local_al_address            = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x01},
                            .neighbor_al_address         = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x02},
                            .transmitter_link_metrics_nr = 1,
                            .transmitter_link_metrics    =
                                (struct _transmitterLinkMetricEntries[]){
                                    {
                                        .local_interface_address    = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
                                        .neighbor_interface_address = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
                                        .intf_type                  = MEDIA_TYPE_IEEE_802_11G_2_4_GHZ,
                                        .bridge_flag                = 0,
                                        .packet_errors              = 134,
                                        .transmitted_packets        = 1543209,
                                        .mac_throughput_capacity    = 400,
                                        .link_availability          = 50,
                                        .phy_rate                   = 520,
                                    },
                                }
                        },
                    },
                .rx_metric            =
                    (struct receiverLinkMetricTLV[]){
                        {
                            .tlv.type                    = TLV_TYPE_RECEIVER_LINK_METRIC,
                            .local_al_address            = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x01},
                            .neighbor_al_address         = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x02},
                            .receiver_link_metrics_nr    = 1,
                            .receiver_link_metrics       =
                                (struct _receiverLinkMetricEntries[]){
                                    {
                                        .local_interface_address    = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
                                        .neighbor_interface_address = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
                                        .intf_type                  = MEDIA_TYPE_IEEE_802_11G_2_4_GHZ,
                                        .packet_errors              = 27263110,
                                        .packets_received           = 27263111,
                                        .rssi                       = 2,
                                    },
                                },
                        },
                    },
            },
        },
    .reason_code               = REASON_CODE_SUCCESS,
};

uint8_t x1905_alme_stream_024[] =
{
    0x10,
    0x01,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00,

    0x09,
    0x00, 0x29,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x01,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x01, 0x01,
    0x00,
    0x00, 0x00, 0x00, 0x86,
    0x00, 0x17, 0x8c, 0x29,
    0x01, 0x90,
    0x00, 0x32,
    0x02, 0x08,

    0x0a,
    0x00, 0x23,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x01,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x01, 0x01,
    0x01, 0xa0, 0x00, 0x86,
    0x01, 0xa0, 0x00, 0x87,
    0x02,

    0x00,
};

uint16_t x1905_alme_stream_len_024 = 98;


////////////////////////////////////////////////////////////////////////////////
//// Test vector 025 (TLV <--> packet)
////////////////////////////////////////////////////////////////////////////////

struct getMetricResponseALME x1905_alme_structure_025 =
{
    .alme_type                 = ALME_TYPE_GET_METRIC_RESPONSE,
    .metrics_nr                = 0,
    .metrics                   = NULL,
    .reason_code               = REASON_CODE_UNMATCHED_NEIGHBOR_MAC_ADDRESS,
};

uint8_t x1905_alme_stream_025[] =
{
    0x10,
    0x00,
    0x07,
};

uint16_t x1905_alme_stream_len_025 = 3;


