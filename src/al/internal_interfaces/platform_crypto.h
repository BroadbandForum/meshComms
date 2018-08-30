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

#ifndef _PLATFORM_CRYPTO_H_
#define _PLATFORM_CRYPTO_H_

#define SHA256_MAC_LEN 32
#define AES_BLOCK_SIZE 16

#include "platform.h"


// Fill the buffer of length 'len' pointed by 'p' with random bytes.
//
// Return "0" if there was a problem, "1" otherwise
//
INT8U PLATFORM_GET_RANDOM_BYTES(INT8U *p, INT16U len);


// Return a Diffie Hellman pair of private and public keys (and its lengths) in
// the output arguments "priv", "priv_len", "pub" and "pub_len".
//
// Both "priv" and "pub" must be deallocated by the caller when they are no
// longer needed (using "PLATFORM_FREE()")
//
// The keys are obtained using the DH group specified in RFC3523 "section 2"
// (ie. the "1536-bit MODP Group" where "g = 2" and "p = 2^1536 - 2^1472 - 1 +
// 2^64 * { [2^1406 pi] + 741804 }")
//
// Return "0" if there was a problem, "1" otherwise
//
INT8U PLATFORM_GENERATE_DH_KEY_PAIR(INT8U **priv, INT16U *priv_len, INT8U **pub, INT16U *pub_len);

// Return the Diffie Hell shared secret (in output argument "shared_secret"
// which is "shared_secret_len" bytes long) associated to a remote public key
// ("remote_pub", which is "remote_pub_len" bytes long") and a local private
// key ("local_priv", which is "local_priv_len" bytes long).
//
// "shared_secret" must be deallocated by the caller once it is no longer needed
// (using "PLATFORM_FREE()")
//
// Return "0" if there was a problem, "1" otherwise
//
INT8U PLATFORM_COMPUTE_DH_SHARED_SECRET(INT8U **shared_secret, INT16U *shared_secret_len, INT8U *remote_pub, INT16U remote_pub_len, INT8U *local_priv, INT8U local_priv_len);

// Return the SHA256 digest of the provided input.
//
// The provided input is the result of concatenating 'num_elem' elements
// (addr[0], addr[1], ..., addr[num_elem-1] of size len[0], len[1], ...,
// len[num_elem-1])
//
// The digest is returned in the 'digest' output argument which must point to
// a preallocated buffer of "SHA256_MAC_LEN" bytes.
//
INT8U PLATFORM_SHA256(INT8U num_elem, INT8U **addr, INT32U *len, INT8U *digest);


// Return the HMAC_SHA256 digest of the provided input using the provided 'key'
// (which is 'keylen' bytes long).
//
// The provided input is the result of concatenating 'num_elem' elements
// (addr[0], addr[1], ..., addr[num_elem-1] of size len[0], len[1], ...,
// len[num_elem-1])
//
// The digest is returned in the 'hmac' output argument which must point to
// a preallocated buffer of "SHA256_MAC_LEN" bytes.
//
INT8U PLATFORM_HMAC_SHA256(INT8U *key, INT32U keylen, INT8U num_elem, INT8U **addr, INT32U *len, INT8U *hmac);

// Encrypt the provided 'data' (which is a pointer to buffer of size
// n*AES_BLOCK_SIZE) using the AES 128 CBC algorithm with the provided
// "initialization vector" ('iv', which is also a pointer to a buffer of
// AES_BLOCK_SIZE bytes).
//
// The result is written to THE SAME 'data' buffer and has the same length as
// the input (plain) data.
//
// Note that you might have to "pad" the data buffer (so that its length is a
// multiple of AES_BLOCK_SIZE) in most cases.
//
// Return "0" if there was a problem, "1" otherwise
//
INT8U PLATFORM_AES_ENCRYPT(INT8U *key, INT8U *iv, INT8U *data, INT32U data_len);

// Works exactly like "PLATFORM_AES_ENCRYPT", but now the 'data' buffer
// originally contains encrypted data and after the call it contains
// unencrypted data.
INT8U PLATFORM_AES_DECRYPT(INT8U *key, INT8U *iv, INT8U *data, INT32U data_len);

#endif
