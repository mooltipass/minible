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

/* Prototypes */
RET_TYPE logic_smartcard_handle_inserted(void);
void logic_smartcard_handle_removed(void);



#endif /* LOGIC_SMARTCARD_H_ */