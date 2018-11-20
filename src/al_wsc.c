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

#include "al_wsc.h"
#include "al_datamodel.h"
#include "packet_tools.h"

#include "platform_crypto.h"
#include "platform_interfaces.h"

#include <string.h> // memcmp(), memcpy(), ...


////////////////////////////////////////////////////////////////////////////////
// Private functions and data
////////////////////////////////////////////////////////////////////////////////

// "defines" used to fill the M1 and M2 message fields
//
#define ATTR_VERSION           (0x104a)
#define ATTR_MSG_TYPE          (0x1022)
    #define WPS_M1                 (0x04)
    #define WPS_M2                 (0x05)
#define ATTR_UUID_E            (0x1047)
#define ATTR_UUID_R            (0x1048)
#define ATTR_MAC_ADDR          (0x1020)
#define ATTR_ENROLLEE_NONCE    (0x101a)
#define ATTR_REGISTRAR_NONCE   (0x1039)
#define ATTR_PUBLIC_KEY        (0x1032)
#define ATTR_AUTH_TYPE_FLAGS   (0x1004)
#define ATTR_ENCR_TYPE_FLAGS   (0x1010)
#define ATTR_CONN_TYPE_FLAGS   (0x100d)
    #define WPS_CONN_ESS           (0x01)
    #define WPS_CONN_IBSS          (0x02)
#define ATTR_CONFIG_METHODS    (0x1008)
    #define WPS_CONFIG_VIRT_PUSHBUTTON (0x0280)
    #define WPS_CONFIG_PHY_PUSHBUTTON  (0x0480)
#define ATTR_WPS_STATE         (0x1044)
    #define WPS_STATE_NOT_CONFIGURED (1)
    #define WPS_STATE_CONFIGURED     (2)
#define ATTR_MANUFACTURER      (0x1021)
#define ATTR_MODEL_NAME        (0x1023)
#define ATTR_MODEL_NUMBER      (0x1024)
#define ATTR_SERIAL_NUMBER     (0x1042)
#define ATTR_PRIMARY_DEV_TYPE  (0x1054)
    #define WPS_DEV_COMPUTER                           (1)
        #define WPS_DEV_COMPUTER_PC                       (1)
        #define WPS_DEV_COMPUTER_SERVER                   (2)
        #define WPS_DEV_COMPUTER_MEDIA_CENTER             (3)
        #define WPS_DEV_COMPUTER_ULTRA_MOBILE             (4)
        #define WPS_DEV_COMPUTER_NOTEBOOK                 (5)
        #define WPS_DEV_COMPUTER_DESKTOP                  (6)
        #define WPS_DEV_COMPUTER_MID                      (7)
        #define WPS_DEV_COMPUTER_NETBOOK                  (8)
        #define WPS_DEV_COMPUTER_TABLET                   (9)
    #define WPS_DEV_INPUT                              (2)
        #define WPS_DEV_INPUT_KEYBOARD                    (1)
        #define WPS_DEV_INPUT_MOUSE                       (2)
        #define WPS_DEV_INPUT_JOYSTICK                    (3)
        #define WPS_DEV_INPUT_TRACKBALL                   (4)
        #define WPS_DEV_INPUT_GAMING                      (5)
        #define WPS_DEV_INPUT_REMOTE                      (6)
        #define WPS_DEV_INPUT_TOUCHSCREEN                 (7)
        #define WPS_DEV_INPUT_BIOMETRIC_READER            (8)
        #define WPS_DEV_INPUT_BARCODE_READER              (9)
    #define WPS_DEV_PRINTER                            (3)
        #define WPS_DEV_PRINTER_PRINTER                   (1)
        #define WPS_DEV_PRINTER_SCANNER                   (2)
        #define WPS_DEV_PRINTER_FAX                       (3)
        #define WPS_DEV_PRINTER_COPIER                    (4)
        #define WPS_DEV_PRINTER_ALL_IN_ONE                (5)
    #define WPS_DEV_CAMERA                             (4)
        #define WPS_DEV_CAMERA_DIGITAL_STILL_CAMERA       (1)
        #define WPS_DEV_CAMERA_VIDEO                      (2)
        #define WPS_DEV_CAMERA_WEB                        (3)
        #define WPS_DEV_CAMERA_SECURITY                   (4)
    #define WPS_DEV_STORAGE                            (5)
        #define WPS_DEV_STORAGE_NAS                       (1)
    #define WPS_DEV_NETWORK_INFRA                      (6)
        #define WPS_DEV_NETWORK_INFRA_AP                  (1)
        #define WPS_DEV_NETWORK_INFRA_ROUTER              (2)
        #define WPS_DEV_NETWORK_INFRA_SWITCH              (3)
        #define WPS_DEV_NETWORK_INFRA_GATEWAY             (4)
        #define WPS_DEV_NETWORK_INFRA_BRIDGE              (5)
    #define WPS_DEV_DISPLAY                            (7)
        #define WPS_DEV_DISPLAY_TV                        (1)
        #define WPS_DEV_DISPLAY_PICTURE_FRAME             (2)
        #define WPS_DEV_DISPLAY_PROJECTOR                 (3)
        #define WPS_DEV_DISPLAY_MONITOR                   (4)
    #define WPS_DEV_MULTIMEDIA                         (8)
        #define WPS_DEV_MULTIMEDIA_DAR                    (1)
        #define WPS_DEV_MULTIMEDIA_PVR                    (2)
        #define WPS_DEV_MULTIMEDIA_MCX                    (3)
        #define WPS_DEV_MULTIMEDIA_SET_TOP_BOX            (4)
        #define WPS_DEV_MULTIMEDIA_MEDIA_SERVER           (5)
        #define WPS_DEV_MULTIMEDIA_PORTABLE_VIDEO_PLAYER  (6)
    #define WPS_DEV_GAMING                             (9)
        #define WPS_DEV_GAMING_XBOX                       (1)
        #define WPS_DEV_GAMING_XBOX360                    (2)
        #define WPS_DEV_GAMING_PLAYSTATION                (3)
        #define WPS_DEV_GAMING_GAME_CONSOLE               (4)
        #define WPS_DEV_GAMING_PORTABLE_DEVICE            (5)
    #define WPS_DEV_PHONE                             (10)
        #define WPS_DEV_PHONE_WINDOWS_MOBILE              (1)
        #define WPS_DEV_PHONE_SINGLE_MODE                 (2)
        #define WPS_DEV_PHONE_DUAL_MODE                   (3)
        #define WPS_DEV_PHONE_SP_SINGLE_MODE              (4)
        #define WPS_DEV_PHONE_SP_DUAL_MODE                (5)
    #define WPS_DEV_AUDIO                             (11)
        #define WPS_DEV_AUDIO_TUNER_RECV                  (1)
        #define WPS_DEV_AUDIO_SPEAKERS                    (2)
        #define WPS_DEV_AUDIO_PMP                         (3)
        #define WPS_DEV_AUDIO_HEADSET                     (4)
        #define WPS_DEV_AUDIO_HEADPHONES                  (5)
        #define WPS_DEV_AUDIO_MICROPHONE                  (6)
        #define WPS_DEV_AUDIO_HOME_THEATRE                (7)
#define ATTR_DEV_NAME          (0x1011)
#define ATTR_RF_BANDS          (0x103c)
#define ATTR_ASSOC_STATE       (0x1002)
    #define WPS_ASSOC_NOT_ASSOC     (0)
    #define WPS_ASSOC_CONN_SUCCESS  (1)
#define ATTR_DEV_PASSWORD_ID   (0x1012)
    #define DEV_PW_PUSHBUTTON      (0x0004)
#define ATTR_CONFIG_ERROR      (0x1009)
    #define WPS_CFG_NO_ERROR       (0)
#define ATTR_OS_VERSION        (0x102d)
#define ATTR_VENDOR_EXTENSION  (0x1049)
    #define WPS_VENDOR_ID_WFA_1    (0x00)
    #define WPS_VENDOR_ID_WFA_2    (0x37)
    #define WPS_VENDOR_ID_WFA_3    (0x2A)
    #define WFA_ELEM_VERSION2      (0x00)
    #define WPS_VERSION            (0x20)
    #define WFA_ELEM_MULTI_AP_EXTENSION (0x06)
        /* Multi-AP extension subelement values */
        #define MULTI_AP_TEAR_DOWN      0x10
        #define MULTI_AP_FRONTHAUL_BSS	0x20
        #define MULTI_AP_BACKHAUL_BSS	0x40
        #define MULTI_AP_BACKHAUL_STA	0x80
#define ATTR_SSID              (0x1045)
#define ATTR_AUTH_TYPE         (0x1003)
#define ATTR_ENCR_TYPE         (0x100f)
#define ATTR_NETWORK_KEY       (0x1027)
#define ATTR_KEY_WRAP_AUTH     (0x101e)
#define ATTR_ENCR_SETTINGS     (0x1018)
#define ATTR_AUTHENTICATOR     (0x1005)

// Keys sizes
//
#define WPS_AUTHKEY_LEN    32
#define WPS_KEYWRAPKEY_LEN 16
#define WPS_EMSK_LEN       32


// This is the key derivation function used in the WPS standard to obtain a
// final hash that is later used for encryption.
//
// The output is stored in the memory buffer pointed by 'res', which must be
// "SHA256_MAC_LEN" bytes long (ie. 're_len' must always be "SHA256_MAC_LEN",
// even if it is an input argument)
//
void _wps_key_derivation_function(uint8_t *key, uint8_t *label_prefix, uint32_t label_prefix_len, char *label, uint8_t *res, uint32_t res_len)
{
    uint8_t i_buf[4];
    uint8_t key_bits[4];

    const uint8_t   *addr[4];
    uint32_t   len[4];

    uint32_t i, iter;

    uint8_t  hash[SHA256_MAC_LEN];
    uint8_t *opos;

    uint32_t left;

    uint8_t  *p;
    uint32_t  aux;

    aux = res_len * 8;
    p   = key_bits;

    _I4B(&aux, &p);

    addr[0] = i_buf;
    addr[1] = label_prefix;
    addr[2] = (uint8_t *) label;
    addr[3] = key_bits;
    len[0]  = sizeof(i_buf);
    len[1]  = label_prefix_len;
    len[2]  = strlen(label);
    len[3]  = sizeof(key_bits);

    iter = (res_len + SHA256_MAC_LEN - 1) / SHA256_MAC_LEN;
    opos = res;
    left = res_len;

    for (i = 1; i <= iter; i++)
    {
        p = i_buf;
        _I4B(&i, &p);

        PLATFORM_HMAC_SHA256(key, SHA256_MAC_LEN, 4, addr, len, hash);

        if (i < iter)
        {
            memcpy(opos, hash, SHA256_MAC_LEN);
            opos += SHA256_MAC_LEN;
            left -= SHA256_MAC_LEN;
        }
        else
        {
            memcpy(opos, hash, left);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////////////////////////////////

//
//////////////////////////////////////// Enrollee functions ////////////////////
//
bool wscBuildM1(struct radio *radio, const struct wscDeviceData *wsc_device_data)
{
    uint8_t *p;

    uint8_t  aux8;
    uint16_t aux16;
    uint32_t aux32;

    if (NULL == wsc_device_data || NULL == radio)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Invalid arguments to wscBuildM1()\n");
        return false;
    }

    if (radio->wsc_info != NULL)
    {
        free(radio->wsc_info->priv_key);
        memset(radio->wsc_info, 0, sizeof(*radio->wsc_info));
    }
    else
    {
        radio->wsc_info = zmemalloc(sizeof(*radio->wsc_info));
    }
    p      = radio->wsc_info->m1;

    // VERSION
    {
        aux16 = ATTR_VERSION;                                             _I2B(&aux16,     &p);
        aux16 = 1;                                                        _I2B(&aux16,     &p);
        aux8  = 0x10;                                                     _I1B(&aux8,      &p);
    }

    // MESSAGE TYPE
    {
        aux16 = ATTR_MSG_TYPE;                                            _I2B(&aux16,     &p);
        aux16 = 1;                                                        _I2B(&aux16,     &p);
        aux8  = WPS_M1;                                                   _I1B(&aux8,      &p);
    }

    // UUID
    {
        aux16 = ATTR_UUID_E;                                              _I2B(&aux16,     &p);
        aux16 = 16;                                                       _I2B(&aux16,     &p);
                                                                          _InB( wsc_device_data->uuid,   &p, 16);
    }

    // MAC ADDRESS
    // See "Multi-AP Specification Version 1.0" Section 7.1: "The Multi-AP Agent shall set the MAC Address attribute in M1 in
    // the AP-Autoconfiguration WSC message to the 1905 AL MAC address of the Multi-AP device."
    // IEEE 1905.1 does not specify which MAC address to use in M1 (only in M2, where it is the BSSID).
    {
        aux16 = ATTR_MAC_ADDR;                                            _I2B(&aux16,           &p);
        aux16 = 6;                                                        _I2B(&aux16,           &p);
        radio->wsc_info->mac = p;
                                                                          _InB(local_device->al_mac_addr,  &p, 6);
    }

    // ENROLLEE NONCE
    {
        uint8_t enrollee_nonce[16];

        PLATFORM_GET_RANDOM_BYTES(enrollee_nonce, 16);

        aux16 = ATTR_ENROLLEE_NONCE;                                      _I2B(&aux16,           &p);
        aux16 = 16;                                                       _I2B(&aux16,           &p);
        radio->wsc_info->nonce = p;
                                                                          _InB( enrollee_nonce,  &p, 16);
    }

    // PUBLIC KEY
    {
        uint8_t  *pub;
        uint16_t  pub_len;

        PLATFORM_GENERATE_DH_KEY_PAIR(&radio->wsc_info->priv_key, &radio->wsc_info->priv_key_len, &pub, &pub_len);
        // TODO: ZERO PAD the pub key (doesn't seem to be really needed though)

        aux16 = ATTR_PUBLIC_KEY;                                          _I2B(&aux16,       &p);
        aux16 = pub_len;                                                  _I2B(&aux16,       &p);
                                                                          _InB( pub,         &p, pub_len);
    }

    // AUTHENTICATION TYPES
    {
        /*
         * See "Multi-AP Specification Version 1.0" Section 7.1: "The Multi-AP
         * Agent shall set the Authentication Type Flags attribute in M1 in the
         * AP-Autoconfiguration WSC message to indicate support for
         * WPA2-Personal and Open System Authentication types."
         */
        uint16_t  auth_types = auth_mode_open | auth_mode_wpa2psk;
        aux16 = ATTR_AUTH_TYPE_FLAGS;                                     _I2B(&aux16,      &p);
        aux16 = 2;                                                        _I2B(&aux16,      &p);
                                                                          _I2B(&auth_types, &p);
    }

    // ENCRYPTION TYPES
    {
        /* Multi-AP Specification Version 1.0 says nothing about enryption
         * types, but this is implied by the authentication types. */
        uint16_t  encryption_types = WPS_ENCR_NONE | WPS_ENCR_AES;

        aux16 = ATTR_ENCR_TYPE_FLAGS;                                     _I2B(&aux16,            &p);
        aux16 = 2;                                                        _I2B(&aux16,            &p);
                                                                          _I2B(&encryption_types, &p);
    }

    // CONNECTION TYPES
    {
        // Two possible types: ESS or IBSS. In the 1905 context, enrollees will
        // always want to acts as "ESS" to create an "extended" network where
        // all APs share the same credentials as the registrar.

        aux16 = ATTR_CONN_TYPE_FLAGS;                                     _I2B(&aux16,     &p);
        aux16 = 1;                                                        _I2B(&aux16,     &p);
        aux8  = WPS_CONN_ESS;                                             _I1B(&aux8,      &p);
    }

    // CONFIGURATION METHODS
    {
        // In the 1905 context, the configuration methods the AP is willing to
        // offer will always be these two

        aux16 = ATTR_CONFIG_METHODS;                                      _I2B(&aux16,     &p);
        aux16 = 2;                                                        _I2B(&aux16,     &p);
        aux16 = WPS_CONFIG_PHY_PUSHBUTTON | WPS_CONFIG_VIRT_PUSHBUTTON;   _I2B(&aux16,     &p);
    }

    // WPS STATE
    {
        aux16 = ATTR_WPS_STATE;                                           _I2B(&aux16,     &p);
        aux16 = 1;                                                        _I2B(&aux16,     &p);
        aux8  = WPS_STATE_NOT_CONFIGURED;                                 _I1B(&aux8,      &p);
    }

    // MANUFACTURER
    {
        aux16 = ATTR_MANUFACTURER;                                        _I2B(&aux16,                &p);
        aux16 = strlen(wsc_device_data->manufacturer_name);                    _I2B(&aux16,                &p);
                                                                          _InB( wsc_device_data->manufacturer_name, &p, strlen(wsc_device_data->manufacturer_name));
    }

    // MODEL NAME
    {

        aux16 = ATTR_MODEL_NAME;                                          _I2B(&aux16,         &p);
        aux16 = strlen(wsc_device_data->model_name);                           _I2B(&aux16,         &p);
                                                                          _InB( wsc_device_data->model_name, &p, strlen(wsc_device_data->model_name));
    }

    // MODEL NUMBER
    {
        aux16 = ATTR_MODEL_NUMBER;                                        _I2B(&aux16,           &p);
        aux16 = strlen(wsc_device_data->model_number);                         _I2B(&aux16,           &p);
                                                                          _InB( wsc_device_data->model_number, &p, strlen(wsc_device_data->model_number));
    }

    // SERIAL NUMBER
    {
        aux16 = ATTR_SERIAL_NUMBER;                                       _I2B(&aux16,            &p);
        aux16 = strlen(wsc_device_data->serial_number);                        _I2B(&aux16,            &p);
                                                                          _InB( wsc_device_data->serial_number, &p, strlen(wsc_device_data->serial_number));
    }

    // PRIMARY DEVICE TYPE
    {
        // In the 1905 context, they node sending a M1 message will always be
        // (at least) a "network router"

        uint8_t oui[4]      = {0x00, 0x50, 0xf2, 0x00}; // Fixed value from the
                                                      // WSC spec

        aux16 = ATTR_PRIMARY_DEV_TYPE;                                    _I2B(&aux16,         &p);
        aux16 = 8;                                                        _I2B(&aux16,         &p);
        aux16 = WPS_DEV_NETWORK_INFRA;                                    _I2B(&aux16,         &p);
                                                                          _InB( oui,           &p, 4);
        aux16 = WPS_DEV_NETWORK_INFRA_ROUTER;                             _I2B(&aux16,         &p);
    }

    // DEVICE NAME
    {
        aux16 = ATTR_DEV_NAME;                                            _I2B(&aux16,           &p);
        aux16 = strlen(wsc_device_data->device_name);                          _I2B(&aux16,           &p);
                                                                          _InB( wsc_device_data->device_name,  &p, strlen(wsc_device_data->device_name));
    }

    // RF BANDS
    {
        unsigned i;
        uint8_t rf_bands = 0;
        for (i = 0; i < radio->bands.length; i++)
        {
            switch (radio->bands.data[i]->id)
            {
            case BAND_2GHZ:
                rf_bands |= WPS_RF_24GHZ;
                break;
            case BAND_5GHZ:
                rf_bands |= WPS_RF_50GHZ;
                break;
            case BAND_60GHZ:
                rf_bands |= WPS_RF_60GHZ;
                break;
            }
        }
        aux16 = ATTR_RF_BANDS;                                            _I2B(&aux16,         &p);
        aux16 = 1;                                                        _I2B(&aux16,         &p);
                                                                          _I1B(&rf_bands,      &p);
    }

    // ASSOCIATION STATE
    {
        aux16 = ATTR_ASSOC_STATE;                                         _I2B(&aux16,         &p);
        aux16 = 2;                                                        _I2B(&aux16,         &p);
        aux16 = WPS_ASSOC_NOT_ASSOC;                                      _I2B(&aux16,         &p);
    }

    // DEVICE PASSWORD ID
    {
        aux16 = ATTR_DEV_PASSWORD_ID;                                     _I2B(&aux16,         &p);
        aux16 = 2;                                                        _I2B(&aux16,         &p);
        aux16 = DEV_PW_PUSHBUTTON;                                        _I2B(&aux16,         &p);
    }

    // CONFIG ERROR
    {
        aux16 = ATTR_CONFIG_ERROR;                                        _I2B(&aux16,         &p);
        aux16 = 2;                                                        _I2B(&aux16,         &p);
        aux16 = WPS_CFG_NO_ERROR;                                         _I2B(&aux16,         &p);
    }

    // OS VERSION
    {
        // TODO: Fill with actual properties from the interface

        uint32_t os_version = 0x00000001;

        aux16 = ATTR_OS_VERSION;                                          _I2B(&aux16,         &p);
        aux16 = 4;                                                        _I2B(&aux16,         &p);
        aux32 = 0x80000000 | os_version;                                  _I4B(&aux32,         &p);
    }

    // VENDOR EXTENSIONS
    {
        aux16 = ATTR_VENDOR_EXTENSION;                                    _I2B(&aux16,         &p);
        aux16 = 6;                                                        _I2B(&aux16,         &p);
        aux8  = WPS_VENDOR_ID_WFA_1;                                      _I1B(&aux8,          &p);
        aux8  = WPS_VENDOR_ID_WFA_2;                                      _I1B(&aux8,          &p);
        aux8  = WPS_VENDOR_ID_WFA_3;                                      _I1B(&aux8,          &p);
        aux8  = WFA_ELEM_VERSION2;                                     _I1B(&aux8,          &p);
        aux8  = 1;                                                        _I1B(&aux8,          &p);
        aux8  = WPS_VERSION;                                              _I1B(&aux8,          &p);
    }

    return true;
}

bool wscProcessM2(struct radio *radio, const uint8_t *m2, uint16_t m2_size)
{
    const uint8_t         *p;

    // "Useful data" we want to extract from M2
    //
    struct bssInfo bssInfo;
    bool ssid_present = false;
    bool bssid_present = false;
    bool auth_type_present = false;
    bool encryption_type_present = false;

    bool multi_ap_ie_present = false;
    /* The following are only valid if multi_ap_ie_present is true */
    bool multi_ap_bSTA, multi_ap_bBSS, multi_ap_fBSS, multi_ap_teardown;

    // Keys we need to compute to authenticate and decrypt M2
    //
    uint8_t authkey   [WPS_AUTHKEY_LEN];
    uint8_t keywrapkey[WPS_KEYWRAPKEY_LEN];
    uint8_t emsk      [WPS_EMSK_LEN];

    // "Intermediary data" we also need to extract from M2 to obtain the keys
    // that will let us decrypt the "useful data" from M2
    //
    const uint8_t  *m2_nonce;                  uint8_t m2_nonce_present;
    const uint8_t  *m2_pubkey;                 uint8_t m2_pubkey_present;
    uint16_t  m2_pubkey_len;
    const uint8_t  *m2_encrypted_settings;     uint8_t m2_encrypted_settings_present;
    uint16_t  m2_encrypted_settings_len;
    const uint8_t  *m2_authenticator;          uint8_t m2_authenticator_present;

    if (radio == NULL || m2 == NULL)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Invalid parameters to wscProcessM2\n");
        return false;
    }
    if (radio->wsc_info == NULL || radio->wsc_info->mac == NULL || radio->wsc_info->nonce == NULL || radio->wsc_info->priv_key == NULL)
    {
        PLATFORM_PRINTF_DEBUG_ERROR("Incomplete wsc_info parameter to wscProcessM2\n");
        return false;
    }

    memset(&bssInfo, 0, sizeof(bssInfo));

    // Extract "intermediary data" from M2
    //
    m2_nonce_present               = 0;
    m2_pubkey_present              = 0;
    m2_encrypted_settings_present  = 0;
    m2_authenticator_present       = 0;
    p                         = m2;
    while (p - m2 < m2_size)
    {
        uint16_t attr_type;
        uint16_t attr_len;

        _E2B(&p, &attr_type);
        _E2B(&p, &attr_len);

        if (ATTR_REGISTRAR_NONCE == attr_type)
        {
            if (16 != attr_len)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Incorrect length (%d) for ATTR_REGISTRAR_NONCE\n", attr_len);
                return false;
            }
            m2_nonce = p;

            m2_nonce_present = 1;
        }
        else if (ATTR_PUBLIC_KEY == attr_type)
        {
            m2_pubkey_len = attr_len;
            m2_pubkey     = p;

            m2_pubkey_present = 1;
        }
        else if (ATTR_ENCR_SETTINGS == attr_type)
        {
            m2_encrypted_settings_len = attr_len;
            m2_encrypted_settings     = p;

            m2_encrypted_settings_present = 1;
        }
        else if (ATTR_AUTHENTICATOR == attr_type)
        {
            if (8 != attr_len)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Incorrect length (%d) for ATTR_AUTHENTICATOR\n", attr_len);
                return false;
            }
            m2_authenticator = p;

            m2_authenticator_present = 1;
        }
        else if (ATTR_VENDOR_EXTENSION == attr_type)
        {
            if (attr_len < 3)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Vendor extension attribute too short (%u) for OUI\n", attr_len);
                /* Ignore */
            }
            else if (p[0] == WPS_VENDOR_ID_WFA_1 &&
                     p[1] == WPS_VENDOR_ID_WFA_2 &&
                     p[2] == WPS_VENDOR_ID_WFA_3)
            {
                uint16_t ie_offset;
                uint8_t ie_type, ie_len;
                for (ie_offset = 3; ie_offset < attr_len; ie_offset += ie_len + 2)
                {
                    ie_type = p[ie_offset];
                    ie_len = p[ie_offset + 1];
                    if (ie_type == WFA_ELEM_MULTI_AP_EXTENSION)
                    {
                        if (ie_len != 1)
                        {
                            PLATFORM_PRINTF_DEBUG_WARNING("Multi-AP Extension IE with length %u\n", ie_len);
                            continue;
                        }
                        multi_ap_ie_present = true;
                        multi_ap_teardown = !!(p[ie_offset + 2] & MULTI_AP_TEAR_DOWN);
                        multi_ap_bBSS = !!(p[ie_offset + 2] & MULTI_AP_BACKHAUL_BSS);
                        multi_ap_bSTA = !!(p[ie_offset + 2] & MULTI_AP_BACKHAUL_STA);
                        multi_ap_fBSS = !!(p[ie_offset + 2] & MULTI_AP_FRONTHAUL_BSS);
                    }
                }
            }
        }
        p += attr_len;
    }

    /* Short-circuit here the case when the Multi-AP IE is present and the teardown bit is set. In that case, the encrypted settings
     * etc. are probably not present, since we don't need them.
     *
     * Note: we don't check for consistency (e.g. when teardown bit is set, there should be only one M2 and it should not have
     * any other bit set).
     */
    if (multi_ap_ie_present && multi_ap_teardown)
    {
        unsigned i;
        PLATFORM_PRINTF_DEBUG_DETAIL("Multi-AP M2 WSC with tear-down bit set.\n");
        for (i = 0; i < radio->configured_bsses.length; i++)
        {
            /* @todo Only tear down multi-AP configured BSSes, not locally configured ones. */
            interfaceTearDown(&radio->configured_bsses.data[i]->i);
        }
        return true;
    }

    if (
         0 == m2_nonce_present              ||
         0 == m2_pubkey_present             ||
         0 == m2_encrypted_settings_present ||
         0 == m2_authenticator_present
       )
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Missing attributes in the received M2 message\n");
        return false;
    }

    // With all the information we have just extracted from M1 and M2, obtain
    // the authentication/encryption keys.
    {
        uint8_t  *shared_secret;
        uint16_t  shared_secret_len;

        const uint8_t  *addr[3];
        uint32_t  len[3];

        uint8_t   dhkey[SHA256_MAC_LEN];
        uint8_t   kdk  [SHA256_MAC_LEN];

        uint8_t keys[WPS_AUTHKEY_LEN + WPS_KEYWRAPKEY_LEN + WPS_EMSK_LEN];

        // With the enrolle public key (which we received in M1) and our private
        // key (which we generated above), obtain the Diffie Hellman shared
        // secret (when receiving M2, the enrollee will be able to obtain this
        // same shared secret using its private key and ou public key -contained
        // in M2-)
        //
        PLATFORM_COMPUTE_DH_SHARED_SECRET(&shared_secret, &shared_secret_len, m2_pubkey, m2_pubkey_len, radio->wsc_info->priv_key, radio->wsc_info->priv_key_len);
        // TODO: ZERO PAD the shared_secret (doesn't seem to be really needed
        // though)

        // Next, obtain the SHA-256 digest of this shared secret. We will call
        // this "dhkey"
        //
        addr[0] = shared_secret;
        len[0]  = shared_secret_len;

        PLATFORM_SHA256(1, addr, len, dhkey);
        free(shared_secret);

        // Next, concatenate three things (the enrolle nonce contained in M1,
        // the enrolle MAC address, and the nonce we just generated before, and
        // calculate its HMAC (hash message authentication code) using "dhkey"
        // as the secret key.
        //
        addr[0] = radio->wsc_info->nonce;
        addr[1] = radio->wsc_info->mac;
        addr[2] = m2_nonce;
        len[0]  = 16;
        len[1]  = 6;
        len[2]  = 16;

        PLATFORM_HMAC_SHA256(dhkey, SHA256_MAC_LEN, 3, addr, len, kdk);

        // Finally, take "kdk" and using a function provided in the "Wi-Fi
        // simple configuration" standard, obtain THREE KEYS that we will use
        // later ("authkey", "keywrapkey" and "emsk")
        //
        _wps_key_derivation_function(kdk, NULL, 0, "Wi-Fi Easy and Secure Key Derivation", keys, sizeof(keys));

        memcpy(authkey,    keys,                                        WPS_AUTHKEY_LEN);
        memcpy(keywrapkey, keys + WPS_AUTHKEY_LEN,                      WPS_KEYWRAPKEY_LEN);
        memcpy(emsk,       keys + WPS_AUTHKEY_LEN + WPS_KEYWRAPKEY_LEN, WPS_EMSK_LEN);

        PLATFORM_PRINTF_DEBUG_DETAIL("WPS keys: \n");
        PLATFORM_PRINTF_DEBUG_DETAIL("  Registrar pubkey  (%3d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", m2_pubkey_len,  m2_pubkey[0], m2_pubkey[1], m2_pubkey[2], m2_pubkey[m2_pubkey_len-3], m2_pubkey[m2_pubkey_len-2], m2_pubkey[m2_pubkey_len-1]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  Enrollee privkey  (%3d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", radio->wsc_info->priv_key_len,  radio->wsc_info->priv_key[0], radio->wsc_info->priv_key[1], radio->wsc_info->priv_key[2], radio->wsc_info->priv_key[radio->wsc_info->priv_key_len-3], radio->wsc_info->priv_key[radio->wsc_info->priv_key_len-2], radio->wsc_info->priv_key[radio->wsc_info->priv_key_len-1]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  Shared secret     (%3d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", shared_secret_len, shared_secret[0], shared_secret[1], shared_secret[2], shared_secret[shared_secret_len-3], shared_secret[shared_secret_len-2], shared_secret[shared_secret_len-1]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  DH key            ( 32 bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", dhkey[0], dhkey[1], dhkey[2], dhkey[29], dhkey[30], dhkey[31]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  Enrollee nonce    ( 16 bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", radio->wsc_info->nonce[0], radio->wsc_info->nonce[1], radio->wsc_info->nonce[2], radio->wsc_info->nonce[13], radio->wsc_info->nonce[14], radio->wsc_info->nonce[15]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  Registrar nonce   ( 16 bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", m2_nonce[0], m2_nonce[1], m2_nonce[2], m2_nonce[13], m2_nonce[14], m2_nonce[15]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  KDK               ( 32 bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", kdk[0], kdk[1], kdk[2], kdk[29], kdk[30], kdk[31]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  authkey           (%3d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", WPS_AUTHKEY_LEN, authkey[0], authkey[1], authkey[2], authkey[WPS_AUTHKEY_LEN-3], authkey[WPS_AUTHKEY_LEN-2], authkey[WPS_AUTHKEY_LEN-1]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  keywrapkey        (%3d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", WPS_KEYWRAPKEY_LEN, keywrapkey[0], keywrapkey[1], keywrapkey[2], keywrapkey[WPS_KEYWRAPKEY_LEN-3], keywrapkey[WPS_KEYWRAPKEY_LEN-2], keywrapkey[WPS_KEYWRAPKEY_LEN-1]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  emsk              (%3d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", WPS_EMSK_LEN, emsk[0], emsk[1], emsk[2], emsk[WPS_EMSK_LEN-3], emsk[WPS_EMSK_LEN-2], emsk[WPS_EMSK_LEN-1]);
    }

    // With the just computed key, check the message authentication
    //
    {
        // Concatenate M1 and M2 (excluding the last 12 bytes, where the
        // authenticator attribute is contained) and calculate the HMAC, then
        // check it against the actual authenticator attribute value.
        //
        uint8_t   hash[SHA256_MAC_LEN];

        const uint8_t  *addr[2];
        uint32_t  len[2];

        addr[0] = radio->wsc_info->m1;
        addr[1] = m2;
        len[0]  = radio->wsc_info->m1_len;
        len[1]  = m2_size-12;

        PLATFORM_HMAC_SHA256(authkey, WPS_AUTHKEY_LEN, 2, addr, len, hash);

        if (memcmp(m2_authenticator, hash, 8) != 0)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("Message M2 authentication failed\n");
            return false;
        }
    }


    // With the just computed keys, decrypt the message and check the keywrap
    {
        uint8_t   *plain;
        uint32_t  plain_len;
        uint16_t  encryption_type;
        uint16_t  auth_type;

        uint8_t m2_keywrap_present;

        /* Decryption is done in-place in the receive buffer, so const cast is needed */
        plain     = (uint8_t *)m2_encrypted_settings + AES_BLOCK_SIZE;
        plain_len = m2_encrypted_settings_len - AES_BLOCK_SIZE;

        PLATFORM_PRINTF_DEBUG_DETAIL("AP settings before decryption (%d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", plain_len, plain[0], plain[1], plain[2], plain[plain_len-3], plain[plain_len-2], plain[plain_len-1]);
        PLATFORM_PRINTF_DEBUG_DETAIL("IV (%d bytes)                           : 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", AES_BLOCK_SIZE, m2_encrypted_settings[0], m2_encrypted_settings[1], m2_encrypted_settings[2], m2_encrypted_settings[AES_BLOCK_SIZE-3], m2_encrypted_settings[AES_BLOCK_SIZE-2], m2_encrypted_settings[AES_BLOCK_SIZE-1]);
        PLATFORM_AES_DECRYPT(keywrapkey, m2_encrypted_settings, plain, plain_len);
        PLATFORM_PRINTF_DEBUG_DETAIL("AP settings after  decryption (%d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", plain_len, plain[0], plain[1], plain[2], plain[plain_len-3], plain[plain_len-2], plain[plain_len-1]);


        // Remove padding
        //
        plain_len -= plain[plain_len-1];

        // Parse contents of decrypted settings
        //
        m2_keywrap_present        = 0;
        p                         = plain;
        while (p - plain < plain_len)
        {
            uint16_t attr_type;
            uint16_t attr_len;

            _E2B(&p, &attr_type);
            _E2B(&p, &attr_len);

            if (ATTR_SSID == attr_type)
            {
                if (attr_len <= sizeof(bssInfo.ssid.ssid))
                {
                    _EnB(&p, bssInfo.ssid.ssid, attr_len);
                    bssInfo.ssid.length = attr_len;
                    ssid_present = true;
                }
                else
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("Invalid SSID: too long (%u)\n", attr_len);
                }
            }
            else if (ATTR_AUTH_TYPE == attr_type)
            {
                _E2B(&p, &auth_type);
                auth_type_present = true;
            }
            else if (ATTR_ENCR_TYPE == attr_type)
            {
                _E2B(&p, &encryption_type);
                encryption_type_present = true;
            }
            else if (ATTR_NETWORK_KEY == attr_type)
            {
                if (attr_len <= sizeof(bssInfo.key))
                {
                    _EnB(&p, bssInfo.key, attr_len);
                    bssInfo.key_len = (uint8_t)attr_len;
                }
                else
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("Invalid SSID: too long (%u)\n", attr_len);
                }
            }
            else if (ATTR_MAC_ADDR == attr_type)
            {
                if (attr_len == 6)
                {
                    _EnB(&p, bssInfo.bssid, 6);
                    bssid_present = true;
                }
                else
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("Invalid BSSID length: %u\n", attr_len);
                }
            }
            else if (ATTR_KEY_WRAP_AUTH == attr_type)
            {
                // This attribute is always the last one contained in the plain
                // text, thus 4 bytes *before* where "p" is pointing right now
                // is the end of the plain text blob whose HMAC we are going to
                // compute to check the keywrap.
                //
                const uint8_t *end_of_hmac;
                uint8_t  hash[SHA256_MAC_LEN];

                const uint8_t  *addr[1];
                uint32_t  len[1];

                end_of_hmac = p - 4;

                addr[0] = plain;
                len[0]  = end_of_hmac-plain;

                PLATFORM_HMAC_SHA256(authkey, WPS_AUTHKEY_LEN, 1, addr, len, hash);

                if (memcmp(p, hash, 8) != 0)
                {
                    PLATFORM_PRINTF_DEBUG_WARNING("Message M2 keywrap failed\n");
                    return false;
                }

                p += attr_len;
                m2_keywrap_present = 1;
            }
            else
            {
                p += attr_len;
            }
        }
        if (
             !ssid_present                  ||
             !bssid_present                 ||
             !auth_type_present             ||
             !encryption_type_present       ||
             0 == bssInfo.key_len           ||
             0 == m2_keywrap_present
           )
        {
            PLATFORM_PRINTF_DEBUG_WARNING("Missing attributes in the configuration settings received in the M2 message\n");
            return false;
        }
        switch (auth_type)
        {
        case auth_mode_open:
            if (encryption_type != IEEE80211_ENCRYPTION_MODE_NONE)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Invalid encryption type %u for open mode\n", encryption_type);
                return false;
            }
            break;
        case auth_mode_wpa2:
        case auth_mode_wpa2psk:
            if (encryption_type != IEEE80211_ENCRYPTION_MODE_AES)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Invalid encryption type %u for WPA2 mode\n", encryption_type);
                return false;
            }
            break;
        default:
            PLATFORM_PRINTF_DEBUG_WARNING("Unsupported authentication type %u\n", auth_type);
            return false;
        }
        bssInfo.auth_mode = auth_type;
    }

    /* Take action depending on the multi_ap bits. */
    if (multi_ap_ie_present)
    {
        /* Note: we don't check for consistency. If bSTA is set, we ignore the other bits. */
        if (multi_ap_bSTA)
        {
            radioAddSta(radio, bssInfo);
        }
        else if (!multi_ap_bBSS && !multi_ap_fBSS)
        {
            PLATFORM_PRINTF_DEBUG_WARNING("Multi-AP IE present in WSC but no bits are set.\n");
        }
        else
        {
            bssInfo.backhaul = multi_ap_bBSS;
            bssInfo.backhaul_only = !multi_ap_fBSS;
            radioAddAp(radio, bssInfo);
        }
    }
    else
    {
        radioAddAp(radio, bssInfo);
    }

    return true;
}

//
//////////////////////////////////////// Registrar functions ///////////////////
//
bool wscParseM1(const uint8_t *m1, uint16_t m1_size, struct wscM1Info *m1_info)
{
    const uint8_t *p1 = m1;

    memset(m1_info, 0, sizeof(*m1_info));
    m1_info->m1 = m1;
    m1_info->m1_size = m1_size;

    while (p1 - m1 < m1_size)
    {
        uint16_t attr_type;
        uint16_t attr_len;

        _E2B(&p1, &attr_type);
        _E2B(&p1, &attr_len);

        if (ATTR_MAC_ADDR == attr_type)
        {
            if (6 != attr_len)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Incorrect length (%d) for ATTR_MAC_ADDR\n", attr_len);
                return false;
            }
            m1_info->mac_address = p1;
        }
        else if (ATTR_ENROLLEE_NONCE == attr_type)
        {
            if (16 != attr_len)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Incorrect length (%d) for ATTR_ENROLLEE_NONCE\n", attr_len);
                return false;
            }
            m1_info->nonce = p1;
        }
        else if (ATTR_PUBLIC_KEY == attr_type)
        {
            m1_info->pubkey_len = attr_len;
            m1_info->pubkey = p1;
        }
        p1 += attr_len;
    }

    return true;
}

bool wscBuildM2(struct wscM1Info *m1_info, const struct wscRegistrarInfo *wsc_info, struct wscM2Buf *m2)
{
    uint8_t  *buffer;

    uint8_t *p2;

    uint8_t  aux8;
    uint16_t aux16;
    uint32_t aux32;

    uint16_t encr_types;

    uint8_t  *local_privkey;
    uint16_t  local_privkey_len;

    uint8_t authkey   [WPS_AUTHKEY_LEN];
    uint8_t keywrapkey[WPS_KEYWRAPKEY_LEN];
    uint8_t emsk      [WPS_EMSK_LEN];

    uint8_t  registrar_nonce[16];

    if (!registrarIsLocal())
    {
        PLATFORM_PRINTF_DEBUG_WARNING("We are not a registrar. Ignoring M1 message.\n");
        return false;
    }

    if (m1_info->mac_address == NULL || m1_info->nonce == NULL || m1_info->pubkey == NULL)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("Incomplete M1 message received\n");
        return false;
    }

    /* Derive encryption from auth mode */
    switch (wsc_info->bss_info.auth_mode)
    {
    case auth_mode_open:
        encr_types = WPS_ENCR_NONE;
        break;
    case auth_mode_wpa2:
    case auth_mode_wpa2psk:
        encr_types = WPS_ENCR_AES;
        break;
    }

    // Now we can build "M2"
    //

    buffer = (uint8_t *)memalloc(sizeof(uint8_t)*1000);
    p2      = buffer;

    // VERSION
    {
        aux16 = ATTR_VERSION;                                             _I2B(&aux16,     &p2);
        aux16 = 1;                                                        _I2B(&aux16,     &p2);
        aux8  = 0x10;                                                     _I1B(&aux8,      &p2);
    }

    // MESSAGE TYPE
    {
        aux16 = ATTR_MSG_TYPE;                                            _I2B(&aux16,     &p2);
        aux16 = 1;                                                        _I2B(&aux16,     &p2);
        aux8  = WPS_M2;                                                   _I1B(&aux8,      &p2);
    }

    // ENROLLEE NONCE
    {
        aux16 = ATTR_ENROLLEE_NONCE;                                      _I2B(&aux16,     &p2);
        aux16 = 16;                                                       _I2B(&aux16,     &p2);
                                                                          _InB( m1_info->nonce,  &p2, 16);
    }

    // REGISTRAR NONCE
    {
        PLATFORM_GET_RANDOM_BYTES(registrar_nonce, 16);

        aux16 = ATTR_REGISTRAR_NONCE;                                     _I2B(&aux16,           &p2);
        aux16 = 16;                                                       _I2B(&aux16,           &p2);
                                                                          _InB( registrar_nonce, &p2, 16);
    }

    // UUID
    {
        aux16 = ATTR_UUID_R;                                              _I2B(&aux16,     &p2);
        aux16 = 16;                                                       _I2B(&aux16,     &p2);
                                                                          _InB( wsc_info->device_data.uuid,   &p2, 16);
    }

    // PUBLIC KEY
    {
        uint8_t  *priv, *pub;
        uint16_t  priv_len, pub_len;

        PLATFORM_GENERATE_DH_KEY_PAIR(&priv, &priv_len, &pub, &pub_len);
        // TODO: ZERO PAD the pub key (doesn't seem to be really needed though)

        aux16 = ATTR_PUBLIC_KEY;                                          _I2B(&aux16,       &p2);
        aux16 = pub_len;                                                  _I2B(&aux16,       &p2);
                                                                          _InB( pub,         &p2, pub_len);

        // We will use it later... save it.
        //
        local_privkey     = priv;
        local_privkey_len = priv_len;
    }

    // Key derivation (no bytes are written to the output buffer in the next
    // block of code, we just obtain three cryptographic keys that are needed
    // later
    {
        uint8_t  *shared_secret;
        uint16_t  shared_secret_len;

        const uint8_t  *addr[3];
        uint32_t  len[3];

        uint8_t   dhkey[SHA256_MAC_LEN];
        uint8_t   kdk  [SHA256_MAC_LEN];

        uint8_t keys      [WPS_AUTHKEY_LEN + WPS_KEYWRAPKEY_LEN + WPS_EMSK_LEN];

        // With the enrolle public key (which we received in M1) and our private
        // key (which we generated above), obtain the Diffie Hellman shared
        // secret (when receiving M2, the enrollee will be able to obtain this
        // same shared secret using its private key and our public key
        // -contained in M2-)
        //
        PLATFORM_COMPUTE_DH_SHARED_SECRET(&shared_secret, &shared_secret_len,
                                          m1_info->pubkey, m1_info->pubkey_len,
                                          local_privkey, local_privkey_len);
        // TODO: ZERO PAD the shared_secret (doesn't seem to be really needed
        // though)

        // Next, obtain the SHA-256 digest of this shared secret. We will call
        // this "dhkey"
        //
        addr[0] = shared_secret;
        len[0]  = shared_secret_len;

        PLATFORM_SHA256(1, addr, len, dhkey);

        // Next, concatenate three things (the enrollee nonce contained in M1,
        // the enrolle MAC address -also contained in M1-, and the nonce we just
        // generated before and calculate its HMAC (hash message authentication
        // code) using "dhkey" as the secret key.
        //
        addr[0] = m1_info->nonce;
        addr[1] = m1_info->mac_address;
        addr[2] = registrar_nonce;
        len[0]  = 16;
        len[1]  = 6;
        len[2]  = 16;

        PLATFORM_HMAC_SHA256(dhkey, SHA256_MAC_LEN, 3, addr, len, kdk);

        // Finally, take "kdk" and using a function provided in the "Wi-Fi
        // simple configuration" standard, obtain THREE KEYS that we will use
        // later ("authkey", "keywrapkey" and "emsk")
        //
        _wps_key_derivation_function(kdk, NULL, 0, "Wi-Fi Easy and Secure Key Derivation", keys, sizeof(keys));

        memcpy(authkey,    keys,                                        WPS_AUTHKEY_LEN);
        memcpy(keywrapkey, keys + WPS_AUTHKEY_LEN,                      WPS_KEYWRAPKEY_LEN);
        memcpy(emsk,       keys + WPS_AUTHKEY_LEN + WPS_KEYWRAPKEY_LEN, WPS_EMSK_LEN);

        PLATFORM_PRINTF_DEBUG_DETAIL("WPS keys: \n");
        PLATFORM_PRINTF_DEBUG_DETAIL("  Enrollee pubkey   (%3d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", m1_info->pubkey_len,  m1_info->pubkey[0], m1_info->pubkey[1], m1_info->pubkey[2], m1_info->pubkey[m1_info->pubkey_len-3], m1_info->pubkey[m1_info->pubkey_len-2], m1_info->pubkey[m1_info->pubkey_len-1]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  Registrar privkey (%3d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", local_privkey_len,  local_privkey[0], local_privkey[1], local_privkey[2], local_privkey[local_privkey_len-3], local_privkey[local_privkey_len-2], local_privkey[local_privkey_len-1]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  Shared secret     (%3d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", shared_secret_len, shared_secret[0], shared_secret[1], shared_secret[2], shared_secret[shared_secret_len-3], shared_secret[shared_secret_len-2], shared_secret[shared_secret_len-1]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  DH key            ( 32 bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", dhkey[0], dhkey[1], dhkey[2], dhkey[29], dhkey[30], dhkey[31]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  Enrollee nonce    ( 16 bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", m1_info->nonce[0], m1_info->nonce[1], m1_info->nonce[2], m1_info->nonce[13], m1_info->nonce[14], m1_info->nonce[15]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  Registrar nonce   ( 16 bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", registrar_nonce[0], registrar_nonce[1], registrar_nonce[2], registrar_nonce[13], registrar_nonce[14], registrar_nonce[15]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  KDK               ( 32 bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", kdk[0], kdk[1], kdk[2], kdk[29], kdk[30], kdk[31]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  authkey           (%3d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", WPS_AUTHKEY_LEN, authkey[0], authkey[1], authkey[2], authkey[WPS_AUTHKEY_LEN-3], authkey[WPS_AUTHKEY_LEN-2], authkey[WPS_AUTHKEY_LEN-1]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  keywrapkey        (%3d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", WPS_KEYWRAPKEY_LEN, keywrapkey[0], keywrapkey[1], keywrapkey[2], keywrapkey[WPS_KEYWRAPKEY_LEN-3], keywrapkey[WPS_KEYWRAPKEY_LEN-2], keywrapkey[WPS_KEYWRAPKEY_LEN-1]);
        PLATFORM_PRINTF_DEBUG_DETAIL("  emsk              (%3d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", WPS_EMSK_LEN, emsk[0], emsk[1], emsk[2], emsk[WPS_EMSK_LEN-3], emsk[WPS_EMSK_LEN-2], emsk[WPS_EMSK_LEN-1]);

        free(shared_secret);
    }

    aux16 = ATTR_AUTH_TYPE_FLAGS;                                     _I2B(&aux16,                &p2);
    aux16 = 2;                                                        _I2B(&aux16,                &p2);
    aux16 = wsc_info->bss_info.auth_mode;                             _I2B(&aux16,                &p2);
    aux16 = ATTR_ENCR_TYPE_FLAGS;                                     _I2B(&aux16,      &p2);
    aux16 = 2;                                                        _I2B(&aux16,      &p2);
                                                                      _I2B(&encr_types, &p2);

    // CONNECTION TYPES
    {
        // Two possible types: ESS or IBSS. In the 1905 context, registrars
        // will always be "ESS", meaning they are willing to have their
        // credentials cloned by other APs in order to end up with a network
        // which is "roaming-friendly" ("ESS": "extended service set")

        aux16 = ATTR_CONN_TYPE_FLAGS;                                     _I2B(&aux16,     &p2);
        aux16 = 1;                                                        _I2B(&aux16,     &p2);
        aux8  = WPS_CONN_ESS;                                             _I1B(&aux8,      &p2);
    }

    // CONFIGURATION METHODS
    {
        // In the 1905 context, the configuration methods the AP is willing to
        // offer will always be these two

        aux16 = ATTR_CONFIG_METHODS;                                      _I2B(&aux16,     &p2);
        aux16 = 2;                                                        _I2B(&aux16,     &p2);
        aux16 = WPS_CONFIG_PHY_PUSHBUTTON | WPS_CONFIG_VIRT_PUSHBUTTON;   _I2B(&aux16,     &p2);
    }

    // MANUFACTURER
    {
        aux16 = ATTR_MANUFACTURER;                                        _I2B(&aux16,                &p2);
        aux16 = strlen(wsc_info->device_data.manufacturer_name);          _I2B(&aux16,                &p2);
                                                                          _InB( wsc_info->device_data.manufacturer_name, &p2, strlen(wsc_info->device_data.manufacturer_name));
    }

    // MODEL NAME
    {
        aux16 = ATTR_MODEL_NAME;                                          _I2B(&aux16,         &p2);
        aux16 = strlen(wsc_info->device_data.model_name);                           _I2B(&aux16,         &p2);
                                                                          _InB( wsc_info->device_data.model_name, &p2, strlen(wsc_info->device_data.model_name));
    }

    // MODEL NUMBER
    {
        aux16 = ATTR_MODEL_NUMBER;                                        _I2B(&aux16,           &p2);
        aux16 = strlen(wsc_info->device_data.model_number);                         _I2B(&aux16,           &p2);
                                                                          _InB( wsc_info->device_data.model_number, &p2, strlen(wsc_info->device_data.model_number));
    }

    // SERIAL NUMBER
    {
        aux16 = ATTR_SERIAL_NUMBER;                                       _I2B(&aux16,            &p2);
        aux16 = strlen(wsc_info->device_data.serial_number);                        _I2B(&aux16,            &p2);
                                                                          _InB( wsc_info->device_data.serial_number, &p2, strlen(wsc_info->device_data.serial_number));
    }

    // PRIMARY DEVICE TYPE
    {
        // In the 1905 context, they node sending a M2 message will always be
        // (at least) a "network router"

        uint8_t oui[4]      = {0x00, 0x50, 0xf2, 0x00}; // Fixed value from the
                                                      // WSC spec

        aux16 = ATTR_PRIMARY_DEV_TYPE;                                    _I2B(&aux16,         &p2);
        aux16 = 8;                                                        _I2B(&aux16,         &p2);
        aux16 = WPS_DEV_NETWORK_INFRA;                                    _I2B(&aux16,         &p2);
                                                                          _InB( oui,           &p2, 4);
        aux16 = WPS_DEV_NETWORK_INFRA_ROUTER;                             _I2B(&aux16,         &p2);
    }

    // DEVICE NAME
    {
        aux16 = ATTR_DEV_NAME;                                            _I2B(&aux16,          &p2);
        aux16 = strlen(wsc_info->device_data.device_name);                          _I2B(&aux16,          &p2);
                                                                          _InB( wsc_info->device_data.device_name, &p2, strlen(wsc_info->device_data.device_name));
    }

    aux16 = ATTR_RF_BANDS;                                            _I2B(&aux16,         &p2);
    aux16 = 1;                                                        _I2B(&aux16,         &p2);
                                                                      _I1B(&wsc_info->rf_bands,      &p2);

    // ASSOCIATION STATE
    {
        aux16 = ATTR_ASSOC_STATE;                                         _I2B(&aux16,         &p2);
        aux16 = 2;                                                        _I2B(&aux16,         &p2);
        aux16 = WPS_ASSOC_CONN_SUCCESS;                                   _I2B(&aux16,         &p2);
    }

    // CONFIG ERROR
    {
        aux16 = ATTR_CONFIG_ERROR;                                        _I2B(&aux16,         &p2);
        aux16 = 2;                                                        _I2B(&aux16,         &p2);
        aux16 = WPS_CFG_NO_ERROR;                                         _I2B(&aux16,         &p2);
    }

    // DEVICE PASSWORD ID
    {
        aux16 = ATTR_DEV_PASSWORD_ID;                                     _I2B(&aux16,         &p2);
        aux16 = 2;                                                        _I2B(&aux16,         &p2);
        aux16 = DEV_PW_PUSHBUTTON;                                        _I2B(&aux16,         &p2);
    }

    // OS VERSION
    {
        // TODO: Fill with actual properties from the interface

        uint32_t os_version = 0x00000001;

        aux16 = ATTR_OS_VERSION;                                          _I2B(&aux16,         &p2);
        aux16 = 4;                                                        _I2B(&aux16,         &p2);
        aux32 = 0x80000000 | os_version;                                  _I4B(&aux32,         &p2);
    }

    // VENDOR EXTENSIONS
    {
        aux16 = ATTR_VENDOR_EXTENSION;                                    _I2B(&aux16,         &p2);
        aux16 = 9;                                                        _I2B(&aux16,         &p2);
        aux8  = WPS_VENDOR_ID_WFA_1;                                      _I1B(&aux8,          &p2);
        aux8  = WPS_VENDOR_ID_WFA_2;                                      _I1B(&aux8,          &p2);
        aux8  = WPS_VENDOR_ID_WFA_3;                                      _I1B(&aux8,          &p2);
        aux8  = WFA_ELEM_VERSION2;                                        _I1B(&aux8,          &p2);
        aux8  = 1;                                                        _I1B(&aux8,          &p2);
        aux8  = WPS_VERSION;                                              _I1B(&aux8,          &p2);
        /* Always include Multi-AP extension element, even if the enrollee is not Multi-AP - it will just ignore this.
         * @todo that's not entirely correct: a non-Multi-AP enrollee will not understand teardown etc. so it
         * will not behave correctly. */
        aux8  = WFA_ELEM_MULTI_AP_EXTENSION;                              _I1B(&aux8,          &p2);
        aux8  = 1;                                                        _I1B(&aux8,          &p2);
        /* @todo correctly set the flags */
        aux8  = MULTI_AP_FRONTHAUL_BSS | MULTI_AP_BACKHAUL_BSS;           _I1B(&aux8,          &p2);
    }

    // ENCRYPTED SETTINGS
    {
        // This is what we are going to do next:
        //
        //   1. Fill a tmp buffer ("aux") ith credential attributes (SSID,
        //      network key, etc...)
        //
        //   2. Add an HMAC to this tmp buffer (so that the enrollee, when
        //      receiving this buffer in M2, can be sure no one has tampered
        //      with these attributes)
        //
        //   3. Encryp the message + HMAC with AES (so that no one else
        //      snooping can have a look at these attributes)

        uint8_t   plain[200];
        uint8_t   hash[SHA256_MAC_LEN];
        uint8_t  *iv_start;
        uint8_t  *data_start;
        uint8_t  *r;
        uint8_t  pad_elements_nr;

        const uint8_t  *addr[1];
        uint32_t  len[1];

        r = plain;

        // SSID
        aux16 = ATTR_SSID;                                                _I2B(&aux16,         &r);
        aux16 = wsc_info->bss_info.ssid.length;                                    _I2B(&aux16,         &r);
                                                                          _InB( wsc_info->bss_info.ssid.ssid, &r, wsc_info->bss_info.ssid.length);

        // AUTH TYPE
        aux16 = ATTR_AUTH_TYPE;                                           _I2B(&aux16,         &r);
        aux16 = 2;                                                        _I2B(&aux16,         &r);
        aux16 = wsc_info->bss_info.auth_mode;                                     _I2B(&aux16,         &r);

        // ENCRYPTION TYPE
        aux16 = ATTR_ENCR_TYPE;                                           _I2B(&aux16,         &r);
        aux16 = 2;                                                        _I2B(&aux16,         &r);
        aux16 = encr_types;                                               _I2B(&aux16,         &r);

        // NETWORK KEY
        aux16 = ATTR_NETWORK_KEY;                                         _I2B(&aux16,         &r);
        aux16 = wsc_info->bss_info.key_len;                                        _I2B(&aux16,         &r);
                                                                          _InB( wsc_info->bss_info.key, &r, wsc_info->bss_info.key_len);

        // MAC ADDR
        aux16 = ATTR_MAC_ADDR;                                            _I2B(&aux16,           &r);
        aux16 = 6;                                                        _I2B(&aux16,           &r);
                                                                          _InB( wsc_info->bss_info.bssid, &r, 6);

        PLATFORM_PRINTF_DEBUG_DETAIL("AP configuration settings that we are going to send:\n");
        PLATFORM_PRINTF_DEBUG_DETAIL("  - SSID            : %.*s\n", wsc_info->bss_info.ssid.length, wsc_info->bss_info.ssid.ssid);
        PLATFORM_PRINTF_DEBUG_DETAIL("  - BSSID           : " MACSTR "\n", MAC2STR(wsc_info->bss_info.bssid));
        PLATFORM_PRINTF_DEBUG_DETAIL("  - AUTH_TYPE       : 0x%04x\n", wsc_info->bss_info.auth_mode);
        PLATFORM_PRINTF_DEBUG_DETAIL("  - ENCRYPTION_TYPE : 0x%04x\n", encr_types);
        PLATFORM_PRINTF_DEBUG_DETAIL("  - NETWORK_KEY     : %.*s\n", wsc_info->bss_info.key_len, wsc_info->bss_info.key);

        // Obtain the HMAC of the whole plain buffer using "authkey" as the
        // secret key.
        //
        addr[0] = plain;
        len[0]  = r-plain;
        PLATFORM_HMAC_SHA256(authkey, WPS_AUTHKEY_LEN, 1, addr, len, hash);

        // ...and add it to the same plain buffer (well, only the first 8 bytes
        // of the hash)
        //
        aux16 = ATTR_KEY_WRAP_AUTH;                                       _I2B(&aux16,         &r);
        aux16 = 8;                                                        _I2B(&aux16,         &r);
                                                                          _InB( hash,          &r, 8);

        // Finally, encrypt everything with AES and add the resulting blob to
        // the M2 buffer, as an "ATTR_ENCR_SETTINGS" attribute
        //
        //// Pad the length of the message to encrypt to a multiple of
        //// AES_BLOCK_SIZE. The new padded bytes must have their value equal to
        //// the amount of bytes padded (PKCS#5 v2.0 pad)
        ////
        pad_elements_nr = AES_BLOCK_SIZE - ((r-plain) % AES_BLOCK_SIZE);
        for (aux8 = 0; aux8<pad_elements_nr; aux8++)
        {
            _I1B(&pad_elements_nr, &r);
        }
        //// Add the attribute header ("type" and "lenght") to the M2 buffer,
        //// followed by the IV and the data to encrypt.
        ////
        aux16 = ATTR_ENCR_SETTINGS;                                       _I2B(&aux16,         &p2);
        aux16 = AES_BLOCK_SIZE + (r-plain);                               _I2B(&aux16,         &p2);
        iv_start   = p2; PLATFORM_GET_RANDOM_BYTES(p2, AES_BLOCK_SIZE); p2+=AES_BLOCK_SIZE;
        data_start = p2; _InB(plain, &p2, r-plain);
        //// Encrypt the data IN-PLACE. Note that the "ATTR_ENCR_SETTINGS"
        //// attribute containes both the IV and the encrypted data.
        ////
        PLATFORM_PRINTF_DEBUG_DETAIL("AP settings before encryption (%u bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", (unsigned)(r-plain), data_start[0], data_start[1], data_start[2], data_start[(r-plain)-3], data_start[(r-plain)-2], data_start[(r-plain)-1]);
        PLATFORM_PRINTF_DEBUG_DETAIL("IV (%d bytes)                           : 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", AES_BLOCK_SIZE, iv_start[0], iv_start[1], iv_start[2], iv_start[AES_BLOCK_SIZE-3], iv_start[AES_BLOCK_SIZE-2], iv_start[AES_BLOCK_SIZE-1]);
        PLATFORM_AES_ENCRYPT(keywrapkey, iv_start, data_start, r-plain);
        PLATFORM_PRINTF_DEBUG_DETAIL("AP settings after  encryption (%d bytes): 0x%02x, 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x, 0x%02x\n", (unsigned)(r-plain), data_start[0], data_start[1], data_start[2], data_start[(r-plain)-3], data_start[(r-plain)-2], data_start[(r-plain)-1]);
    }

    // AUTHENTICATOR
    {
        // This one is easy: concatenate M1 and M2 (everything in the M2 buffer
        // up to this point) and calculate the HMAC, then append it to M2 as
        // a new (and final!) attribute
        //
        uint8_t  hash[SHA256_MAC_LEN];

        const uint8_t  *addr[2];
        uint32_t  len[2];

        addr[0] = m1_info->m1;
        addr[1] = buffer;
        len[0]  = m1_info->m1_size;
        len[1]  = p2-buffer;

        PLATFORM_HMAC_SHA256(authkey, WPS_AUTHKEY_LEN, 2, addr, len, hash);

        aux16 = ATTR_AUTHENTICATOR;                                       _I2B(&aux16,         &p2);
        aux16 = 8;                                                        _I2B(&aux16,         &p2);
                                                                          _InB( hash,          &p2,  8);
    }

    m2->m2 = buffer;
    m2->m2_size = p2 - buffer;
    return true;
}

void wscFreeM2List(wscM2List m2_list)
{
    unsigned i;
    for (i = 0; i < m2_list.length; i++)
    {
        free(m2_list.data[i].m2);
    }
    PTRARRAY_CLEAR(m2_list);
}

void wscInfoFree(struct radio *radio)
{
    if (radio->wsc_info)
    {
        free(radio->wsc_info->priv_key);
    }
    free(radio->wsc_info);
    radio->wsc_info = NULL;
}

//
//////////////////////////////////////// Common functions //////////////////////
//
uint8_t wscGetType(const uint8_t *m, uint16_t m_size)
{
    const uint8_t *p;

    p = m;
    while (p - m < m_size)
    {
        uint16_t attr_type;
        uint16_t attr_len;
        uint8_t  msg_type;

        _E2B(&p, &attr_type);
        _E2B(&p, &attr_len);

        if (ATTR_MSG_TYPE == attr_type)
        {
            if (1 != attr_len)
            {
                PLATFORM_PRINTF_DEBUG_WARNING("Incorrect length (%d) for ATTR_MSG_TYPE\n", attr_len);
                return WSC_TYPE_UNKNOWN;
            }
            _E1B(&p, &msg_type);

            if (WPS_M1 == msg_type)
            {
                return WSC_TYPE_M1;
            }
            else if (WPS_M2 == msg_type)
            {
                return WSC_TYPE_M2;
            }
            else
            {
                return WSC_TYPE_UNKNOWN;
            }
        }
        else
        {
            p += attr_len;
        }
    }

    return WSC_TYPE_UNKNOWN;
}

