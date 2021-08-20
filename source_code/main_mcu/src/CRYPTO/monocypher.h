// Monocypher version __git__
//
// SPDX-License-Identifier: BSD-2-Clause
//
// ------------------------------------------------------------------------
//
// Copyright (c) 2017-2019, Loup Vaillant
// All rights reserved.
//
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ------------------------------------------------------------------------

#ifndef MONOCYPHER_H
#define MONOCYPHER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////
/// Type definitions ///
////////////////////////

// Vtable for EdDSA with a custom hash.
// Instantiate it to define a custom hash.
// Its size, contents, and layout, are part of the public API.
typedef struct {
    void (*hash)(uint8_t hash[64], const uint8_t *message, size_t message_size);
    void (*init  )(void *ctx);
    void (*update)(void *ctx, const uint8_t *message, size_t message_size);
    void (*final )(void *ctx, uint8_t hash[64]);
    size_t ctx_size;
} crypto_sign_vtable;

// Do not rely on the size or contents of any of the types below,
// they may change without notice.

// Signatures (EdDSA)
typedef struct {
    const crypto_sign_vtable *hash;
    uint8_t buf[96];
    uint8_t pk [32];
} crypto_sign_ctx_abstract;
typedef crypto_sign_ctx_abstract crypto_check_ctx_abstract;

////////////////////////////
/// High level interface ///
////////////////////////////

// Constant time comparisons
// -------------------------

// Return 0 if a and b are equal, -1 otherwise
int crypto_verify16(const uint8_t a[16], const uint8_t b[16]);
int crypto_verify32(const uint8_t a[32], const uint8_t b[32]);
int crypto_verify64(const uint8_t a[64], const uint8_t b[64]);

// Erase sensitive data
// --------------------

// Please erase all copies
void crypto_wipe(void *secret, size_t size);

// Signatures (EdDSA with curve25519 + Blake2b)
// --------------------------------------------

// Generate public key
void crypto_sign_public_key(uint8_t        public_key[32],
                            const uint8_t  secret_key[32]);

// Direct interface
void crypto_sign(uint8_t        signature [64],
                 const uint8_t  secret_key[32],
                 const uint8_t  public_key[32], // optional, may be 0
                 const uint8_t *message, size_t message_size);
int crypto_check(const uint8_t  signature [64],
                 const uint8_t  public_key[32],
                 const uint8_t *message, size_t message_size);

////////////////////////////
/// Low level primitives ///
////////////////////////////

// EdDSA -- Incremental interface
// ------------------------------

// Signing (2 passes)
// Make sure the two passes hash the same message,
// else you might reveal the private key.
void crypto_sign_init_first_pass(crypto_sign_ctx_abstract *ctx,
                                 const uint8_t  secret_key[32],
                                 const uint8_t  public_key[32]);
void crypto_sign_update(crypto_sign_ctx_abstract *ctx,
                        const uint8_t *message, size_t message_size);
void crypto_sign_init_second_pass(crypto_sign_ctx_abstract *ctx);
// use crypto_sign_update() again.
void crypto_sign_final(crypto_sign_ctx_abstract *ctx, uint8_t signature[64]);

// Verification (1 pass)
// Make sure you don't use (parts of) the message
// before you're done checking it.
void crypto_check_init  (crypto_check_ctx_abstract *ctx,
                         const uint8_t signature[64],
                         const uint8_t public_key[32]);
void crypto_check_update(crypto_check_ctx_abstract *ctx,
                         const uint8_t *message, size_t message_size);
int crypto_check_final  (crypto_check_ctx_abstract *ctx);

// Custom hash interface
void crypto_sign_public_key_custom_hash(uint8_t       public_key[32],
                                        const uint8_t secret_key[32],
                                        const crypto_sign_vtable *hash);
void crypto_sign_init_first_pass_custom_hash(crypto_sign_ctx_abstract *ctx,
                                             const uint8_t secret_key[32],
                                             const uint8_t public_key[32],
                                             const crypto_sign_vtable *hash);
void crypto_check_init_custom_hash(crypto_check_ctx_abstract *ctx,
                                   const uint8_t signature[64],
                                   const uint8_t public_key[32],
                                   const crypto_sign_vtable *hash);

#ifdef __cplusplus
}
#endif

#endif // MONOCYPHER_H
