// Copyright 2019 SoloKeys Developers
//
// Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
// http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
// http://opensource.org/licenses/MIT>, at your option. This file may not be
// copied, modified, or distributed except according to those terms.
//
// MiniBLE modificatins
// -Removed unused functions and added new ones
#ifndef _CRYPTO_H
#define _CRYPTO_H

#include <stddef.h>
#include "comms_aux_mcu_defines.h"

typedef struct
{
    uint8_t x[PUB_KEY_X_LEN];
    uint8_t y[PUB_KEY_Y_LEN];
} ecc256_pub_key;

void crypto_sha256_init(void);
void crypto_sha256_update(uint8_t const *data, size_t len);
void crypto_sha256_final(uint8_t *hash);

void crypto_sha256_hmac_init(uint8_t const *key, uint32_t klen);
void crypto_sha256_hmac_update(uint8_t const * data, size_t len);
void crypto_sha256_hmac_final(uint8_t *hmac_out);

void crypto_sha512_init(void);
void crypto_sha512_update(uint8_t const * data, size_t len);
void crypto_sha512_final(uint8_t *hash);

void crypto_ecc256_init(void);
size_t crypto_ecc256_generate_private_key(uint8_t *priv_key);
void crypto_ecc256_derive_public_key(uint8_t const *priv_key, ecc256_pub_key *pub_key);

void crypto_ecc256_load_key(uint8_t const *key);
void crypto_ecc256_sign(uint8_t const *data, int len, uint8_t * sig);

#endif
