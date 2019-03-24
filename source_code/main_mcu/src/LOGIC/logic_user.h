/*!  \file     logic_user.h
*    \brief    General logic for user operations
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_USER_H_
#define LOGIC_USER_H_

#include "comms_hid_msgs.h"
#include "defines.h"

/* Prototypes */
RET_TYPE logic_user_store_credential(cust_char_t* service, cust_char_t* login, cust_char_t* desc, cust_char_t* third, cust_char_t* password);
int16_t logic_user_get_credential(cust_char_t* service, cust_char_t* login, hid_message_t* send_msg);
ret_type_te logic_user_create_new_user(volatile uint16_t* pin_code, uint8_t* provisioned_key);
void logic_user_init_context(uint8_t user_id);


#endif /* LOGIC_USER_H_ */