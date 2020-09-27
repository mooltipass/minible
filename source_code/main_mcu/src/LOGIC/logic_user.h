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
/*!  \file     logic_user.h
*    \brief    General logic for user operations
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_USER_H_
#define LOGIC_USER_H_

#include "comms_hid_msgs.h"
#include "logic_fido2.h"
#include "custom_fs.h"
#include "defines.h"

/* Defines */
#define CHECK_PASSWORD_TIMER_VAL    4000

/* Prototypes */
fido2_return_code_te logic_user_get_webauthn_credential_key_for_rp(cust_char_t* rp_id, uint8_t* user_handle, uint8_t *user_handle_len, uint8_t* credential_id, uint8_t* private_key, uint32_t* count, uint8_t credential_id_allow_list[FIDO2_ALLOW_LIST_MAX_SIZE][FIDO2_CREDENTIAL_ID_LENGTH], uint16_t credential_id_allow_list_length, uint8_t flags);
RET_TYPE logic_user_ask_for_credentials_keyb_output(uint16_t parent_address, uint16_t child_address, BOOL skip_login_prompt_and_int_choice, BOOL* usb_selected, lock_feature_te keys_to_send_before_login, BOOL skip_login_prompt);
fido2_return_code_te logic_user_store_webauthn_credential(cust_char_t* rp_id, uint8_t* user_handle, uint8_t user_handle_len, cust_char_t* user_name, cust_char_t* display_name, uint8_t* private_key, uint8_t* credential_id);
ret_type_te logic_user_create_new_user_for_existing_card(cpz_lut_entry_t* cpz_entry, uint16_t sec_preferences, uint16_t language_id, uint16_t usb_layout_id, uint16_t ble_layout_id, uint8_t* new_user_id);
RET_TYPE logic_user_store_credential(cust_char_t* service, cust_char_t* login, cust_char_t* desc, cust_char_t* third, cust_char_t* password);
RET_TYPE logic_user_get_data_from_service(cust_char_t* service, uint8_t* buffer, uint16_t* nb_bytes_written, BOOL is_message_from_usb);
RET_TYPE logic_user_add_data_to_current_service(hid_message_store_data_into_file_t* store_data_request, BOOL is_message_from_usb);
RET_TYPE logic_user_store_TOTP_credential(cust_char_t* service, cust_char_t* login, TOTPcredentials_t const *TOTPcreds);
ret_type_te logic_user_create_new_user(volatile uint16_t* pin_code, uint8_t* provisioned_key, BOOL simple_mode);
RET_TYPE logic_user_check_credential(cust_char_t* service, cust_char_t* login, cust_char_t* password);
void logic_user_usb_get_credential(cust_char_t* service, cust_char_t* login, BOOL send_creds_to_usb);
RET_TYPE logic_user_add_data_service(cust_char_t* service, BOOL is_message_from_usb);
void logic_user_inform_computer_locked_state(BOOL usb_interface, BOOL locked);
void logic_user_set_layout_id(uint16_t layout_id, BOOL usb_layout);
RET_TYPE logic_user_is_bluetooth_enabled_for_inserted_card(void);
BOOL logic_user_get_and_clear_user_to_be_logged_off_flag(void);
void logic_user_clear_user_security_flag(uint16_t bitmask);
void logic_user_set_user_security_flag(uint16_t bitmask);
BOOL logic_user_get_lock_unlock_shortcut_enabled(void);
void logic_user_set_user_to_be_logged_off_flag(void);
void logic_user_get_user_cards_cpz(uint8_t* buffer);
void logic_user_set_language(uint16_t language_id);
uint16_t logic_user_get_user_security_flags(void);
void logic_user_unlocked_feature_trigger(void);
void logic_user_init_context(uint8_t user_id);
void logic_user_locked_feature_trigger(void);
uint8_t logic_user_get_current_user_id(void);
void logic_user_manual_select_login(void);

#endif /* LOGIC_USER_H_ */
