/*!  \file     logic_database.h
*    \brief    General logic for database operations
*    Created:  17/03/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_DATABASE_H_
#define LOGIC_DATABASE_H_

#include "comms_hid_msgs.h"
#include "defines.h"


/* Prototypes */
RET_TYPE logic_database_add_credential_for_service(uint16_t service_addr, cust_char_t* login, cust_char_t* desc, cust_char_t* third, uint8_t* password, uint8_t* ctr);
uint16_t logic_database_fill_get_cred_message_answer(uint16_t child_node_addr, hid_message_t* send_msg, uint8_t* cred_ctr, BOOL* prev_gen_credential_flag);
void logic_database_fetch_encrypted_password(uint16_t child_node_addr, uint8_t* password, uint8_t* cred_ctr, BOOL* prev_gen_credential_flag);
uint16_t logic_database_search_service(cust_char_t* name, service_compare_mode_te compare_type, BOOL cred_type, uint16_t data_category_id);
void logic_database_update_credential(uint16_t child_addr, cust_char_t* desc, cust_char_t* third, uint8_t* password, uint8_t* ctr);
uint16_t logic_database_add_service(cust_char_t* service, service_type_te cred_type, uint16_t data_category_id);
uint16_t logic_database_get_number_of_creds_for_service(uint16_t parent_addr, uint16_t* fnode_addr);
uint16_t logic_database_search_login_in_service(uint16_t parent_addr, cust_char_t* login);


#endif /* LOGIC_DATABASE_H_ */
