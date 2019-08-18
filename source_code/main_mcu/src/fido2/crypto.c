/*!  \file     crypto.c
*    \brief    Crypto functions related to FIDO2
*    Created:  15/10/2019
*    Author:   0x0ptr@pm.me
*/
#include <stddef.h>
#include <stdint.h>
#include "rng.h"
#include "platform_defines.h"
#include "crypto.h"
#include "bearssl_hash.h"
#include "bearssl_hmac.h"
#include "bearssl_ec.h"
#include "bearssl_block.h"
#include "bearssl_rand.h"
#include "main.h"
#include "sh1122.h"

static br_sha256_context sha256_ctx;
static br_sha512_context sha512_ctx;
static br_hmac_key_context hmac_kc;
static br_hmac_context hmac_ctx;
static br_ec_impl const *br_ec_algo = &br_ec_p256_m15;
static int br_ec_algo_id = BR_EC_secp256r1;
static br_hmac_drbg_context hmac_drbg_ctx;

static br_ec_private_key _signing_key;
static uint8_t _priv_key_buf[PRIV_KEY_LEN];

void crypto_sha256_init(void)
{
    br_sha256_init(&sha256_ctx);
}

void crypto_sha512_init(void)
{
    br_sha512_init(&sha512_ctx);
}

void crypto_sha256_update(uint8_t const * data, size_t len)
{
    br_sha256_update(&sha256_ctx, data, len);
}

void crypto_sha512_update(uint8_t const *data, size_t len)
{
    br_sha512_update(&sha512_ctx, data, len);
}

void crypto_sha256_final(uint8_t *hash)
{
    br_sha256_out(&sha256_ctx, hash);
}

void crypto_sha512_final(uint8_t *hash)
{
    br_sha512_out(&sha512_ctx, hash);
}

void crypto_sha256_hmac_init(uint8_t const *key, uint32_t klen)
{
    br_hmac_key_init(&hmac_kc, &br_sha256_vtable, key, klen);
    br_hmac_init(&hmac_ctx, &hmac_kc, 0);
}

void crypto_sha256_hmac_update(uint8_t const *data, size_t len)
{
    br_hmac_update(&hmac_ctx, data, len);
}

void crypto_sha256_hmac_final(uint8_t * hmac_out)
{
    br_hmac_out(&hmac_ctx, hmac_out);
}

void crypto_ecc256_init(void)
{
    #define SEED_LEN 8
    uint8_t seed[SEED_LEN];

    br_ec_algo = &br_ec_p256_m15;
    br_ec_algo_id = BR_EC_secp256r1;

    rng_fill_array(seed, SEED_LEN);
    br_hmac_drbg_init(&hmac_drbg_ctx, &br_sha256_vtable, seed, SEED_LEN);
}

void crypto_ecc256_sign(uint8_t const *data, int len, uint8_t * sig)
{
    size_t result = br_ecdsa_i15_sign_raw(br_ec_algo, sha256_ctx.vtable, data, &_signing_key, sig);

    if (result == 0)
    {
        //TODO: Log(TAG_ERR, "error, signing failed!\n");
        while(1);
    }
    //Clear private key used to limit leaking
    memset(_priv_key_buf, 0, sizeof(_priv_key_buf));
}

static void _create_br_key(uint8_t const *priv_key, br_ec_private_key *br_key)
{
    memcpy(_priv_key_buf, priv_key, sizeof(_priv_key_buf));

    br_key->curve = br_ec_algo_id;
    br_key->xlen = PRIV_KEY_LEN;
    br_key->x = _priv_key_buf;
}

void crypto_ecc256_load_key(uint8_t const *key)
{
    _create_br_key(key, &_signing_key);
}

size_t crypto_ecc256_generate_private_key(uint8_t *priv_key)
{
    size_t result;

    result = br_ec_keygen(&hmac_drbg_ctx.vtable, br_ec_algo, NULL, priv_key, br_ec_algo_id);
    if (result == 0)
    {
        //Log("error, generating private key failed\n");
        while(1);
    }
    return result;
}

void crypto_ecc256_derive_public_key(uint8_t const *priv_key, ecc256_pub_key *pub_key)
{
    uint8_t pubkey[65];
    size_t result;
    br_ec_private_key br_priv_key;

    _create_br_key(priv_key, &br_priv_key);

    result = br_ec_compute_pub(br_ec_algo, NULL, pubkey, &br_priv_key);
    if (result == 0)
    {
        //Log("error, deriving public key failed\n");
        while(1);
    }
    memmove(pub_key->x, pubkey + 1, PUB_KEY_X_LEN);
    memmove(pub_key->y, pubkey + 1 + PUB_KEY_X_LEN, PUB_KEY_Y_LEN);

    //Clear private key used to limit leaking
    memset(_priv_key_buf, 0, sizeof(_priv_key_buf));
}

