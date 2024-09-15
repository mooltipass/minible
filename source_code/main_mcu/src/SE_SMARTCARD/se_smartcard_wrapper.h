/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2014 Stephan Mathieu
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
/*!  \file     se_smartcard_wrapper.h
*    \brief    Wrapper functions for secure elements
*    Created:  15/09/2024
*    Author:   Mathieu Stephan
*/
#ifndef SE_SMARTCARD_WRAPPER_H_
#define SE_SMARTCARD_WRAPPER_H_

#include "platform_defines.h"

/* Defines */

/* Function defines */
#ifndef MINIBLE_V2
    #include "smartcard_highlevel.h"
    #include "smartcard_lowlevel.h"
    
    /* Mooltipass Mini BLE v1 */
    #define se_smartcard_is_se_absent                   smartcard_low_level_is_smc_absent
    #define se_smartcard_read_code_protected_zone       smartcard_highlevel_read_code_protected_zone
    #define se_smartcard_erase_se                       smartcard_highlevel_erase_smartcard
    #define se_smartcard_check_for_hidden_aes_key       smartcard_highlevel_check_hidden_aes_key_contents
    #define se_smartcard_read_aes_key                   smartcard_highlevel_read_aes_key
    #define se_smartcard_
    
#else
    #define SMARTCARD_CPZ_LENGTH        8
    
    /* Mooltipass Mini BLE v2 */
    #define se_smartcard_is_se_absent(...)              RETURN_OK
    #define se_smartcard_read_code_protected_zone(...)
    #define se_smartcard_erase_se(...)
    #define se_smartcard_check_for_hidden_aes_key(...)
#endif

#endif /* SE_SMARTCARD_WRAPPER_H_ */