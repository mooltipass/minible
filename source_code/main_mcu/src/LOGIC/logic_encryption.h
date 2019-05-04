/*!  \file     logic_encryption.h
*    \brief    Encryption related functions, calling low level stuff
*    Created:  20/03/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_ENCRYPTION_H_
#define LOGIC_ENCRYPTION_H_

#include "custom_fs.h"
#include "defines.h"

/* Defines */
#define CTR_FLASH_MIN_INCR  32

/* Prototypes */
void logic_encryption_ctr_decrypt(uint8_t* data, uint8_t* cred_ctr, uint16_t data_length, BOOL old_gen_decrypt);
void logic_encryption_add_vector_to_other(uint8_t* destination, uint8_t* source, uint16_t vector_length);
void logic_encryption_xor_vector_to_other(uint8_t* destination, uint8_t* source, uint16_t vector_length);
void logic_encryption_ctr_encrypt(uint8_t* data, uint16_t data_length, uint8_t* ctr_val_used);
void logic_encryption_init_context(uint8_t* card_aes_key, cpz_lut_entry_t* cpz_user_entry);
void logic_encryption_get_cpz_ctr_entry(uint8_t* buffer);
uint8_t logic_encryption_get_user_security_flags(void);
void logic_encryption_post_ctr_tasks(uint16_t ctr_inc);
void logic_encryption_pre_ctr_tasks(uint16_t ctr_inc);
void logic_encryption_delete_context(void);


#endif /* LOGIC_ENCRYPTION_H_ */