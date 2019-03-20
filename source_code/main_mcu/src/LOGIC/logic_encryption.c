/*!  \file     logic_encryption.c
*    \brief    Encryption related functions, calling low level stuff
*    Created:  20/03/2019
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include "logic_encryption.h"
#include "nodemgmt.h"
// Next CTR value for our AES encryption
uint8_t logic_encryption_next_ctr_val[MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr)];


/*! \fn     logic_encryption_init_context(void)
*   \brief  Init encryption context for current user
*/
void logic_encryption_init_context(void)
{
    nodemgmt_read_profile_ctr((void*)logic_encryption_next_ctr_val);    
}

/*! \fn     logic_encryption_pre_ctr_tasks(void)
*   \brief  CTR pre encryption tasks
*/
void logic_encryption_pre_ctr_tasks(void)
{
    _Static_assert(CTR_FLASH_MIN_INCR % (MEMBER_SIZE(child_cred_node_t, password)/(AES_BLOCK_SIZE/8)) == 0, "CTR increment required by password CTR encryption not a multiple of CTR_FLASH_MIN_INCR");    
    uint8_t temp_buffer[MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr)];
    uint16_t carry = CTR_FLASH_MIN_INCR;
    int16_t i;
    
    // Read CTR stored in flash
    nodemgmt_read_profile_ctr(temp_buffer);
    
    // If it is the same value, increment it by CTR_FLASH_MIN_INCR and store it in flash
    if (memcmp(temp_buffer, logic_encryption_next_ctr_val, sizeof(temp_buffer)) == 0)
    {
        for (i = sizeof(temp_buffer)-1; i >= 0; i--)
        {
            carry = (uint16_t)temp_buffer[i] + carry;
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
    for (uint16_t i = sizeof(logic_encryption_next_ctr_val)-1; i >= 0; i--)
    {
        ctr_inc = (uint16_t)logic_encryption_next_ctr_val[i] + ctr_inc;
        logic_encryption_next_ctr_val[i] = (uint8_t)(ctr_inc);
        ctr_inc = (ctr_inc >> 8) & 0x00FF;
    }    
}

/*! \fn     logic_encryption_ctr_encrypt(void)
*   \brief  Encrypt data using next available CTR value
*   \param  data        Pointer to data
*   \param  data_length Data length
*/
void logic_encryption_ctr_encrypt(uint8_t* data, uint16_t data_length)
{        
        /* Pre CTR encryption tasks */
        logic_encryption_pre_ctr_tasks();
        
        /*
        memcpy((void*)temp_buffer, (void*)current_nonce, AES256_CTR_LENGTH);
        aesXorVectors(temp_buffer + (AES256_CTR_LENGTH-USER_CTR_SIZE), nextCtrVal, USER_CTR_SIZE);
        aes256CtrSetIv(&aesctx, temp_buffer, AES256_CTR_LENGTH);
        aes256CtrEncrypt(&aesctx, data, AES_ROUTINE_ENC_SIZE);
        memcpy((void*)ctr, (void*)nextCtrVal, USER_CTR_SIZE);*/
        
        /* Post CTR encryption tasks */
        logic_encryption_post_ctr_tasks((data_length + AES256_CTR_LENGTH - 1)/AES256_CTR_LENGTH);    
}
