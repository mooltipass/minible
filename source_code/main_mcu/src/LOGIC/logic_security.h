/*!  \file     logic_security.c
*    \brief    General logic for security (credentials etc)
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_SECURITY_H_
#define LOGIC_SECURITY_H_

#include "defines.h"

/* Prototypes */
void logic_security_smartcard_unlocked_actions(void);
BOOL logic_security_is_smc_inserted_unlocked(void);
BOOL logic_security_is_management_mode_set(void);
void logic_security_clear_management_mode(void);
void logic_security_clear_security_bools(void);
void logic_security_set_management_mode(void);


#endif /* LOGIC_SECURITY_H_ */