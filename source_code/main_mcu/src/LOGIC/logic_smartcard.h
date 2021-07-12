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
/*!  \file     logic_smartcard.h
*    \brief    General logic for smartcard operations
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/



#ifndef LOGIC_SMARTCARD_H_
#define LOGIC_SMARTCARD_H_

#include "defines.h"

/* Prototypes */
new_pinreturn_type_te logic_smartcard_ask_for_new_pin(volatile uint16_t* new_pin, uint16_t message_id);
valid_card_det_return_te logic_smartcard_valid_card_unlock(BOOL hash_allow_flag, BOOL fast_mode);
RET_TYPE logic_smartcard_remove_card_and_reauth_user(BOOL display_notification);
cloning_ret_te logic_smartcard_clone_card(volatile uint16_t* pincode);
unlock_ret_type_te logic_smartcard_user_unlock_process(void);
RET_TYPE logic_smartcard_handle_inserted(void);
void logic_smartcard_handle_removed(void);



#endif /* LOGIC_SMARTCARD_H_ */