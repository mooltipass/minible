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

/*! \fn     logic_encryption_pre_ctr_task(void)
*   \brief  CTR pre encryption tasks
*/
void logic_encryption_pre_ctr_task(void)
{
    uint8_t temp_buffer[MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr)];
    uint16_t carry = CTR_FLASH_MIN_INCR;
    int16_t i;
    
    // Read CTR stored in flash
    nodemgmt_read_profile_ctr(temp_buffer);
    
    // If it is the same value, increment it by CTR_FLASH_MIN_INCR and store it in flash
    if (memcmp(temp_buffer, logic_encryption_next_ctr_val, sizeof(temp_buffer)) == 0)
    {
        for (i = sizeof(temp_buffer)-1; i > 0; i--)
        {
            carry = (uint16_t)temp_buffer[i] + carry;
            temp_buffer[i] = (uint8_t)(carry);
            carry = (carry >> 8) & 0x00FF;
        }
        nodemgmt_set_profile_ctr(temp_buffer);
    }    
}
