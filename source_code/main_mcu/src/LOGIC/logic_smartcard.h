/*!  \file     logic_smartcard.h
*    \brief    General logic for smartcard operations
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/



#ifndef LOGIC_SMARTCARD_H_
#define LOGIC_SMARTCARD_H_

#include "defines.h"

/* Defines */
#define ID_STRING_PB_CARD           21
#define ID_STRING_CARD_BLOCKED      23
#define ID_STRING_CREATE_NEW_USER   24
#define ID_STRING_NEW_CARD_PIN      25
#define ID_STRING_NEW_USER_ADDED    26
#define ID_STRING_COULDNT_ADD_USER  27
#define ID_STRING_CONFIRM_PIN       29
#define ID_STRING_DIFFERENT_PINS    30
#define ID_STRING_LAST_PIN_TRY      31
#define ID_STRING_INSERT_PIN        32
#define ID_STRING_WRONGPIN1LEFT     33
#define ID_STRING_CARD_FAILING      42
#define ID_STRING_PROCESSING        43
#define ID_STRING_ENTER_MMM         46

/* Prototypes */
RET_TYPE logic_smartcard_ask_for_new_pin(volatile uint16_t* new_pin, uint16_t message_id);
valid_card_det_return_te logic_smartcard_valid_card_unlock(BOOL hash_allow_flag);
RET_TYPE logic_smartcard_user_unlock_process(void);
RET_TYPE logic_smartcard_handle_inserted(void);
void logic_smartcard_handle_removed(void);



#endif /* LOGIC_SMARTCARD_H_ */