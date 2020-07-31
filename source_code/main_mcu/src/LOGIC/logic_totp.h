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
/*!  \file     logic_totp.h
*    \brief    General logic for TOTP algorithm based on RFC 6238 (https://tools.ietf.org/html/rfc6238)
*    Created:  31/07/2020
*    Author:   dnns01
*/


#ifndef LOGIC_TOTP_H_
#define LOGIC_TOTP_H_

#include "bearssl_hash.h"
#include "bearssl_hmac.h"
#include "driver_timer.h"
#include <time.h>

/* Prototypes */
void logic_totp(char* totp_token, const char* key);

#endif /* LOGIC_TOTP_H_ */
