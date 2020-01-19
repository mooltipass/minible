/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2019 Stephan Mathieu
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*!  \file     logic_encryption.h
*    \brief    Encryption related functions, calling low level stuff
*    Created:  20/03/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_ENCRYPTION_H_
#define LOGIC_ENCRYPTION_H_

#include "comms_aux_mcu_defines.h"
#include "custom_fs.h"
#include "defines.h"

/* Defines */
#define CTR_FLASH_MIN_INCR  32
#define ECC256_SEED_LENGTH 8

/* Prototypes */
void logic_encryption_ctr_decrypt(uint8_t* data, uint8_t* cred_ctr, uint16_t data_length, BOOL old_gen_decrypt);
void logic_encryption_add_vector_to_other(uint8_t* destination, uint8_t* source, uint16_t vector_length);
void logic_encryption_xor_vector_to_other(uint8_t* destination, uint8_t* source, uint16_t vector_length);
void logic_encryption_ctr_encrypt(uint8_t* data, uint16_t data_length, uint8_t* ctr_val_used);
void logic_encryption_init_context(uint8_t* card_aes_key, cpz_lut_entry_t* cpz_user_entry);
cpz_lut_entry_t* logic_encryption_get_cur_cpz_lut_entry(void);
void logic_encryption_get_cpz_ctr_entry(uint8_t* buffer);
void logic_encryption_post_ctr_tasks(uint16_t ctr_inc);
void logic_encryption_pre_ctr_tasks(uint16_t ctr_inc);
void logic_encryption_delete_context(void);

typedef struct
{
    uint8_t x[FIDO2_PUB_KEY_X_LEN];
    uint8_t y[FIDO2_PUB_KEY_Y_LEN];
} ecc256_pub_key;

void logic_encryption_sha256_init(void);
void logic_encryption_sha256_update(uint8_t const *data, size_t len);
void logic_encryption_sha256_final(uint8_t *hash);

void logic_encryption_sha256_hmac_init(uint8_t const *key, uint32_t klen);
void logic_encryption_sha256_hmac_update(uint8_t const * data, size_t len);
void logic_encryption_sha256_hmac_final(uint8_t *hmac_out);

void logic_encryption_sha512_init(void);
void logic_encryption_sha512_update(uint8_t const * data, size_t len);
void logic_encryption_sha512_final(uint8_t *hash);

void logic_encryption_ecc256_init(void);
size_t logic_encryption_ecc256_generate_private_key(uint8_t *priv_key);
void logic_encryption_ecc256_derive_public_key(uint8_t const *priv_key, ecc256_pub_key *pub_key);

void logic_encryption_ecc256_load_key(uint8_t const *key);
void logic_encryption_ecc256_sign(uint8_t const *data, int len, uint8_t * sig);

#endif /* LOGIC_ENCRYPTION_H_ */
