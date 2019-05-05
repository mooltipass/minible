/*!  \file     logic_smartcard.h
*    \brief    General logic for smartcard operations
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/



#ifndef LOGIC_SMARTCARD_H_
#define LOGIC_SMARTCARD_H_

#include "defines.h"

/* Prototypes */
valid_card_det_return_te logic_smartcard_valid_card_unlock(BOOL hash_allow_flag, BOOL fast_mode);
RET_TYPE logic_smartcard_ask_for_new_pin(volatile uint16_t* new_pin, uint16_t message_id);
RET_TYPE logic_smartcard_remove_card_and_reauth_user(void);
RET_TYPE logic_smartcard_user_unlock_process(void);
RET_TYPE logic_smartcard_handle_inserted(void);
void logic_smartcard_handle_removed(void);



#endif /* LOGIC_SMARTCARD_H_ */