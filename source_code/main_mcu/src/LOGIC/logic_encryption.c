/*!  \file     logic_encryption.c
*    \brief    Encryption related functions, calling low level stuff
*    Created:  20/03/2019
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include "logic_encryption.h"
#include "bearssl_block.h"
#include "custom_fs.h"
#include "nodemgmt.h"
// Next CTR value for our AES encryption
uint8_t logic_encryption_next_ctr_val[MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr)];
// Current encryption context */
br_aes_ct_ctrcbc_keys logic_encryption_cur_aes_context;
// Current user CPZ user entry
cpz_lut_entry_t* logic_encryption_cur_cpz_entry;


/*! \fn     logic_encryption_ctr_array_to_uint32(uint8_t* array)
*   \brief  Convert CTR array to uint32_t
*   \param  array   CTR array
*   \return The uint32_t
*   \note   The reason why the CTR is stored as array is backward compatibility
*/
static inline uint32_t logic_encryption_ctr_array_to_uint32(uint8_t* array)
{
    return (((uint32_t)array[2]) << 0) | (((uint32_t)array[1]) << 8) | (((uint32_t)array[0]) << 16);
}

/*! \fn     logic_encryption_add_vector_to_other(uint8_t* destination, uint8_t* source, uint16_t vector_length)
*   \brief  Add two vectors together
*   \param  destination     Array, which will also contain the result
*   \param  source          The other array
*   \param  vector_length   Vectors length
*   \note   MSB is at [0]
*/
void logic_encryption_add_vector_to_other(uint8_t* destination, uint8_t* source, uint16_t vector_length)
{
    uint16_t carry = 0;
    
    for (int16_t i = vector_length-1; i >= 0; i--)
    {
        carry = ((uint16_t)destination[i]) + ((uint16_t)source[i]) + carry;
        destination[i] = (uint8_t)(carry);
        carry = (carry >> 8) & 0x00FF;
    }    
}

/*! \fn     logic_encryption_xor_vector_to_other(uint8_t* destination, uint8_t* source, uint16_t vector_length)
*   \brief  XOR two vectors together
*   \param  destination     Array, which will also contain the result
*   \param  source          The other array
*   \param  vector_length   Vectors length
*/
void logic_encryption_xor_vector_to_other(uint8_t* destination, uint8_t* source, uint16_t vector_length)
{    
    for (int16_t i = vector_length-1; i >= 0; i--)
    {
        destination[i] = destination[i] ^ source[i];
    }    
}

/*! \fn     logic_encryption_init_context(uint8_t* card_aes_key, cpz_lut_entry_t* cpz_user_entry)
*   \brief  Init encryption context for current user
*   \param  card_aes_key    AES key stored on user card
*   \param  cpz_user_entry  Pointer to memory persistent cpz entry
*/
void logic_encryption_init_context(uint8_t* card_aes_key, cpz_lut_entry_t* cpz_user_entry)
{
    /* Store CPZ user entry */
    logic_encryption_cur_cpz_entry = cpz_user_entry;
    
    /* Is this a fleet managed user account ? */
    if (logic_encryption_cur_cpz_entry->use_provisioned_key_flag == CUSTOM_FS_PROV_KEY_FLAG)
    {
        uint8_t user_provisioned_key[AES_KEY_LENGTH/8];
        uint8_t temp_ctr[AES256_CTR_LENGTH/8];
        
        /* Store provisioned key in our buffer */
        memcpy(user_provisioned_key, logic_encryption_cur_cpz_entry->provisioned_key, sizeof(user_provisioned_key));
        
        /* Set IV to 0 */
        memset(temp_ctr, 0, sizeof(temp_ctr));
        
        /* Use card AES key to decrypt flash-stored AES key */
        br_aes_ct_ctrcbc_init(&logic_encryption_cur_aes_context, card_aes_key, AES_KEY_LENGTH/8);        
        br_aes_ct_ctrcbc_ctr(&logic_encryption_cur_aes_context, (void*)temp_ctr, (void*)user_provisioned_key, sizeof(user_provisioned_key));
        
        /* Initialize encryption context */
        br_aes_ct_ctrcbc_init(&logic_encryption_cur_aes_context, user_provisioned_key, AES_KEY_LENGTH/8);
        nodemgmt_read_profile_ctr((void*)logic_encryption_next_ctr_val);
        
        /* Clear temp var */
        memset(user_provisioned_key, 0, sizeof(user_provisioned_key));
    } 
    else
    {
        /* Default user account: use smartcard AES key */
        br_aes_ct_ctrcbc_init(&logic_encryption_cur_aes_context, card_aes_key, AES_KEY_LENGTH/8);
        nodemgmt_read_profile_ctr((void*)logic_encryption_next_ctr_val);
    }
}

/*! \fn     logic_encryption_delete_context(void)
*   \brief  Delete encryption context
*/
void logic_encryption_delete_context(void)
{
    memset((void*)&logic_encryption_cur_aes_context, 0, sizeof(logic_encryption_cur_aes_context));
    logic_encryption_cur_cpz_entry = 0;
}

/*! \fn     logic_encryption_pre_ctr_tasks(void)
*   \brief  CTR pre encryption tasks
*   \param  ctr_inc     By how much we are planning to increment ctr value
*/
void logic_encryption_pre_ctr_tasks(uint16_t ctr_inc)
{
    uint8_t temp_buffer[MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr)];
    uint16_t carry = CTR_FLASH_MIN_INCR;
    int16_t i;
    
    // Read CTR stored in flash
    nodemgmt_read_profile_ctr(temp_buffer);
    
    /* Check if the planned increment goes over the next val in the user profile */
    if (logic_encryption_ctr_array_to_uint32(logic_encryption_next_ctr_val) + ctr_inc >= logic_encryption_ctr_array_to_uint32(temp_buffer))
    {
        for (i = sizeof(temp_buffer)-1; i >= 0; i--)
        {
            carry = ((uint16_t)temp_buffer[i]) + carry;
            temp_buffer[i] = (uint8_t)(carry);
            carry = (carry >> 8) & 0x00FF;
        }
        nodemgmt_set_profile_ctr(temp_buffer);
    }    
}

/*! \fn     logic_encryption_post_ctr_tasks(uint16_t ctr_inc)
*   \brief  CTR post encryption tasks
*   \param  ctr_inc     By how much we should increment ctr value
*/
void logic_encryption_post_ctr_tasks(uint16_t ctr_inc)
{
    for (int16_t i = sizeof(logic_encryption_next_ctr_val)-1; i >= 0; i--)
    {
        ctr_inc = ((uint16_t)logic_encryption_next_ctr_val[i]) + ctr_inc;
        logic_encryption_next_ctr_val[i] = (uint8_t)(ctr_inc);
        ctr_inc = (ctr_inc >> 8) & 0x00FF;
    }    
}

/*! \fn     logic_encryption_ctr_encrypt(uint8_t* data, uint16_t data_length)
*   \brief  Encrypt data using next available CTR value
*   \param  data        Pointer to data
*   \param  data_length Data length
*/
void logic_encryption_ctr_encrypt(uint8_t* data, uint16_t data_length)
{
        uint8_t credential_ctr[AES256_CTR_LENGTH/8];
        
        /* Pre CTR encryption tasks */
        logic_encryption_pre_ctr_tasks((data_length + AES256_CTR_LENGTH - 1)/AES256_CTR_LENGTH);
        
        /* Construct CTR for this encryption */
        memcpy(credential_ctr, logic_encryption_cur_cpz_entry->nonce, sizeof(credential_ctr));
        logic_encryption_add_vector_to_other(credential_ctr + (sizeof(credential_ctr) - sizeof(logic_encryption_next_ctr_val)), logic_encryption_next_ctr_val, sizeof(logic_encryption_next_ctr_val));
        
        /* Encrypt data */        
        br_aes_ct_ctrcbc_ctr(&logic_encryption_cur_aes_context, (void*)credential_ctr, (void*)data, data_length);
        
        /* Reset vars */
        memset(credential_ctr, 0, sizeof(credential_ctr));
        
        /* Post CTR encryption tasks */
       logic_encryption_post_ctr_tasks((data_length + AES256_CTR_LENGTH - 1)/AES256_CTR_LENGTH);    
}

/*! \fn     logic_encryption_ctr_decrypt(uint8_t* data, uint8_t* cred_ctr, uint16_t data_length, BOOL old_gen_decrypt)
*   \brief  Decrypt data using provided ctr value
*   \param  data                Pointer to data
*   \param  cred_ctr            Credential CTR
*   \param  data_length         Data length
*   \param  old_gen_decrypt     Set to TRUE when decrypting original mini password
*/
void logic_encryption_ctr_decrypt(uint8_t* data, uint8_t* cred_ctr, uint16_t data_length, BOOL old_gen_decrypt)
{
    uint8_t credential_ctr[AES256_CTR_LENGTH/8];
    
    /* Construct CTR for this encryption */
    memcpy(credential_ctr, logic_encryption_cur_cpz_entry->nonce, sizeof(credential_ctr));
    if (old_gen_decrypt == FALSE)
    {
        logic_encryption_add_vector_to_other(credential_ctr + (sizeof(credential_ctr) - sizeof(logic_encryption_next_ctr_val)), cred_ctr, sizeof(logic_encryption_next_ctr_val));
    } 
    else
    {
        logic_encryption_xor_vector_to_other(credential_ctr + (sizeof(credential_ctr) - sizeof(logic_encryption_next_ctr_val)), cred_ctr, sizeof(logic_encryption_next_ctr_val));
    }
    
    /* Decrypt data */
    br_aes_ct_ctrcbc_ctr(&logic_encryption_cur_aes_context, (void*)credential_ctr, (void*)data, data_length);
    
    /* Reset vars */
    memset(credential_ctr, 0, sizeof(credential_ctr));  
}