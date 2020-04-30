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
/*!  \file     logic_security.c
*    \brief    General logic for security (credentials etc)
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_SECURITY_H_
#define LOGIC_SECURITY_H_

#include "defines.h"

/* Prototypes */
BOOL logic_security_should_leave_management_mode(void);
void logic_security_set_management_mode(BOOL from_usb);
void logic_security_smartcard_unlocked_actions(void);
BOOL logic_security_is_smc_inserted_unlocked(void);
BOOL logic_security_is_management_mode_set(void);
void logic_security_clear_management_mode(void);
void logic_security_clear_security_bools(void);


#endif /* LOGIC_SECURITY_H_ */