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

#include "openssl/dh.h"   // Diffie Hellman stuff
#include "openssl/bn.h"   // "Big numbers" stuff
#include "openssl/evp.h"  // SHA digest and AES stuff
#include "openssl/hmac.h" // HMAC stuff

#include "platform_crypto.h"

////////////////////////////////////////////////////////////////////////////////
// Private data and functions
////////////////////////////////////////////////////////////////////////////////

// Diffie Hellman group "1536-bit MODP" parameters as specified in RFC3523
// "section 2"
//
static unsigned char dh1536_p[]={
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xC9,0x0F,0xDA,0xA2,
        0x21,0x68,0xC2,0x34,0xC4,0xC6,0x62,0x8B,0x80,0xDC,0x1C,0xD1,
        0x29,0x02,0x4E,0x08,0x8A,0x67,0xCC,0x74,0x02,0x0B,0xBE,0xA6,
        0x3B,0x13,0x9B,0x22,0x51,0x4A,0x08,0x79,0x8E,0x34,0x04,0xDD,
        0xEF,0x95,0x19,0xB3,0xCD,0x3A,0x43,0x1B,0x30,0x2B,0x0A,0x6D,
        0xF2,0x5F,0x14,0x37,0x4F,0xE1,0x35,0x6D,0x6D,0x51,0xC2,0x45,
        0xE4,0x85,0xB5,0x76,0x62,0x5E,0x7E,0xC6,0xF4,0x4C,0x42,0xE9,
        0xA6,0x37,0xED,0x6B,0x0B,0xFF,0x5C,0xB6,0xF4,0x06,0xB7,0xED,
        0xEE,0x38,0x6B,0xFB,0x5A,0x89,0x9F,0xA5,0xAE,0x9F,0x24,0x11,
        0x7C,0x4B,0x1F,0xE6,0x49,0x28,0x66,0x51,0xEC,0xE4,0x5B,0x3D,
        0xC2,0x00,0x7C,0xB8,0xA1,0x63,0xBF,0x05,0x98,0xDA,0x48,0x36,
        0x1C,0x55,0xD3,0x9A,0x69,0x16,0x3F,0xA8,0xFD,0x24,0xCF,0x5F,
        0x83,0x65,0x5D,0x23,0xDC,0xA3,0xAD,0x96,0x1C,0x62,0xF3,0x56,
        0x20,0x85,0x52,0xBB,0x9E,0xD5,0x29,0x07,0x70,0x96,0x96,0x6D,
        0x67,0x0C,0x35,0x4E,0x4A,0xBC,0x98,0x04,0xF1,0x74,0x6C,0x08,
        0xCA,0x23,0x73,0x27,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    };
static unsigned char dh1536_g[]={ 0x02 };

#if OPENSSL_VERSION_NUMBER < 0x10100000L || \
        (defined(LIBRESSL_VERSION_NUMBER) && \
         LIBRESSL_VERSION_NUMBER < 0x20700000L)
/* Compatibility wrappers for older versions. */
static int DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g)
{
    dh->p = p;
    if (q != NULL)
        dh->q = q;
    dh->g = g;

    return 1;
}

static void DH_get0_key(const DH *dh,
                        const BIGNUM **pub_key, const BIGNUM **priv_key)
{
    if (pub_key != NULL)
        *pub_key = dh->pub_key;
    if (priv_key != NULL)
        *priv_key = dh->priv_key;
}

static int DH_set0_key(DH *dh, BIGNUM *pub_key, BIGNUM *priv_key)
{
    if (pub_key != NULL)
        dh->pub_key = pub_key;
    if (priv_key != NULL)
        dh->priv_key = priv_key;

    return 1;
}

#endif


////////////////////////////////////////////////////////////////////////////////
// Platform API: Interface related functions to be used by platform-independent
// files (functions declarations are  found in "../interfaces/platform.h)
////////////////////////////////////////////////////////////////////////////////

uint8_t PLATFORM_GET_RANDOM_BYTES(uint8_t *p, uint16_t len)
{
    FILE   *fd;
    uint32_t  rc;

    fd = fopen("/dev/urandom", "rb");

    if (NULL == fd)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("[PLATFORM] Cannot open /dev/urandom\n");
        return 0;
    }

    rc = fread(p, 1, len, fd);

    fclose(fd);

    if (len != rc)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("[PLATFORM] Could not obtain enough random bytes\n");
        return 0;
    }
    else
    {
        return 1;
    }
}

uint8_t PLATFORM_GENERATE_DH_KEY_PAIR(uint8_t **priv, uint16_t *priv_len, uint8_t **pub, uint16_t *pub_len)
{
    DH *dh;
    const BIGNUM *priv_key = NULL;
    const BIGNUM *pub_key = NULL;

    if (
         NULL == priv     ||
         NULL == priv_len ||
         NULL == pub      ||
         NULL == pub_len
       )
    {
        return 0;
    }

    if (NULL == (dh = DH_new()))
    {
        return 0;
    }

    // Convert binary to BIGNUM format
    //
    if (0 == DH_set0_pqg(dh,
                         BN_bin2bn(dh1536_p,sizeof(dh1536_p),NULL),
                         NULL, BN_bin2bn(dh1536_g,sizeof(dh1536_g),NULL)))
    {
        DH_free(dh);
        return 0;
    }

    // Obtain key pair
    //
    if (0 == DH_generate_key(dh))
    {
        DH_free(dh);
        return 0;
    }

    DH_get0_key(dh, &pub_key, &priv_key);
    *priv_len = BN_num_bytes(priv_key);
    *priv     = (uint8_t *)malloc(*priv_len);
    BN_bn2bin(priv_key, *priv);

    *pub_len = BN_num_bytes(pub_key);
    *pub     = (uint8_t *)malloc(*pub_len);
    BN_bn2bin(pub_key, *pub);

    DH_free(dh);
      // NOTE: This internally frees "dh->p" and "dh->q", thus no need for us
      // to do anything else.

    return 1;
}

uint8_t PLATFORM_COMPUTE_DH_SHARED_SECRET(uint8_t **shared_secret, uint16_t *shared_secret_len,
                                          const uint8_t *remote_pub, uint16_t remote_pub_len,
                                          const uint8_t *local_priv, uint8_t local_priv_len)
{
    BIGNUM *pub_key;
    BIGNUM *priv_key;

    size_t rlen;
    int    keylen;

    DH *dh;

    if (
         NULL == shared_secret     ||
         NULL == shared_secret_len ||
         NULL == remote_pub        ||
         NULL == local_priv
       )
    {
        return 0;
    }

    if (NULL == (dh = DH_new()))
    {
        return 0;
    }

    // Convert binary to BIGNUM format
    //
    if (0 == DH_set0_pqg(dh,
                         BN_bin2bn(dh1536_p,sizeof(dh1536_p),NULL),
                         NULL,
                         BN_bin2bn(dh1536_g,sizeof(dh1536_g),NULL)))
    {
        DH_free(dh);
        return 0;
    }

    if (NULL == (pub_key = BN_bin2bn(remote_pub, remote_pub_len, NULL)))
    {
        DH_free(dh);
        return 0;
    }
    if (NULL == (priv_key = BN_bin2bn(local_priv, local_priv_len, NULL)))
    {
        BN_clear_free(pub_key);
        DH_free(dh);
        return 0;
    }
    if (0 == DH_set0_key(dh, NULL, priv_key))
    {
        BN_clear_free(pub_key);
        DH_free(dh);
        return 0;
    }

    // Allocate output buffer
    //
    rlen            = DH_size(dh);
    *shared_secret  = (uint8_t*)malloc(rlen);

    // Compute the shared secret and save it in the output buffer
    //
    keylen = DH_compute_key(*shared_secret, pub_key, dh);
    if (keylen < 0)
    {
        *shared_secret_len = 0;
        free(*shared_secret);
        *shared_secret = NULL;
        BN_clear_free(pub_key);
        DH_free(dh);

        return 0;
    }
    else
    {
        *shared_secret_len = (uint16_t)keylen;
    }

    BN_clear_free(pub_key);
    DH_free(dh);

    return 1;
}


uint8_t PLATFORM_SHA256(uint8_t num_elem, const uint8_t **addr, const uint32_t *len, uint8_t *digest)
{
    uint8_t res;
    unsigned int  mac_len;
    EVP_MD_CTX   *ctx;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        return 0;
    }
#else
    EVP_MD_CTX  ctx_aux;
    ctx = &ctx_aux;

    EVP_MD_CTX_init(ctx);
#endif

    res = 1;

    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), NULL))
    {
        res = 0;
    }

    if (1 == res)
    {
        size_t i;

        for (i = 0; i < num_elem; i++)
        {
            if (!EVP_DigestUpdate(ctx, addr[i], len[i]))
            {
                res = 0;
                break;
            }
        }
    }

    if (1 == res)
    {
        if (!EVP_DigestFinal(ctx, digest, &mac_len))
        {
            res = 0;
        }
    }

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    EVP_MD_CTX_free(ctx);
#endif

    return res;
}


uint8_t PLATFORM_HMAC_SHA256(uint8_t *key, uint32_t keylen, uint8_t num_elem, const uint8_t **addr, const uint32_t *len, uint8_t *hmac)
{
    HMAC_CTX *ctx;
    size_t    i;

    unsigned int mdlen = 32;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    ctx = HMAC_CTX_new();
    if (!ctx)
    {
        return 0;
    }
#else
    HMAC_CTX  ctx_aux;
    ctx = &ctx_aux;

    HMAC_CTX_init(ctx);
#endif

    HMAC_Init_ex(ctx, key, keylen, EVP_sha256(), NULL);

    for (i = 0; i < num_elem; i++)
    {
        HMAC_Update(ctx, addr[i], len[i]);
    }

    HMAC_Final(ctx, hmac, &mdlen);

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    HMAC_CTX_free(ctx);
#else
    HMAC_CTX_cleanup(ctx);
#endif

    return 1;
}

uint8_t PLATFORM_AES_ENCRYPT(uint8_t *key, uint8_t *iv, uint8_t *data, uint32_t data_len)
{
    EVP_CIPHER_CTX *ctx;

    int clen, len;
    uint8_t buf[AES_BLOCK_SIZE];

    ctx = EVP_CIPHER_CTX_new();
    if (NULL == ctx)
    {
        return 0;
    }
    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1)
    {
        return 0;
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    clen = data_len;
    if (EVP_EncryptUpdate(ctx, data, &clen, data, data_len) != 1 || clen != (int) data_len)
    {
        return 0;
    }

    len = sizeof(buf);
    if (EVP_EncryptFinal_ex(ctx, buf, &len) != 1 || len != 0)
    {
        return 0;
    }
    EVP_CIPHER_CTX_free(ctx);


    return 1;
}

uint8_t PLATFORM_AES_DECRYPT(const uint8_t *key, const uint8_t *iv, uint8_t *data, uint32_t data_len)
{
    EVP_CIPHER_CTX *ctx;

    int plen, len;
    uint8_t buf[AES_BLOCK_SIZE];

    ctx = EVP_CIPHER_CTX_new();
    if (NULL == ctx)
    {
        return 0;
    }
    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1)
    {
        return 0;
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    plen = data_len;
    if (EVP_DecryptUpdate(ctx, data, &plen, data, data_len) != 1 || plen != (int) data_len)
    {
        return 0;
    }

    len = sizeof(buf);
    if (EVP_DecryptFinal_ex(ctx, buf, &len) != 1 || len != 0)
    {
        return 0;
    }
    EVP_CIPHER_CTX_free(ctx);

    return 1;
}
