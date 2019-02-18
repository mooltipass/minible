/*!  \file     logic_user.h
*    \brief    General logic for user operations
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_USER_H_
#define LOGIC_USER_H_

#include "defines.h"

/* Prototypes */
ret_type_te logic_user_create_new_user(volatile uint16_t* pin_code, BOOL use_provisioned_key, uint8_t* aes_key);
void logic_user_init_context(uint8_t user_id);


#endif /* LOGIC_USER_H_ */