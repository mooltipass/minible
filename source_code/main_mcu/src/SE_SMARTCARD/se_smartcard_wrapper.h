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
    #define se_smartcard_mooltipass_se_det_routine      smartcard_high_level_mooltipass_card_detected_routine
    #define se_smartcard_check_for_hidden_aes_key       smartcard_highlevel_check_hidden_aes_key_contents
    #define se_smartcard_read_code_protected_zone       smartcard_highlevel_read_code_protected_zone
    #define se_smartcard_write_protected_zone           smartcard_highlevel_write_protected_zone
    #define se_smartcard_se_detect_routine              smartcard_highlevel_card_detected_routine
    #define se_smartcard_get_nb_tries_left              smartcard_highlevel_get_nb_sec_tries_left
    #define se_smartcard_write_pin_code                 smartcard_highlevel_write_security_code
    #define se_smartcard_write_aes_key                  smartcard_highlevel_write_aes_key
    #define se_smartcard_is_se_plugged                  smartcard_lowlevel_is_card_plugged
    #define se_smartcard_is_se_absent                   smartcard_low_level_is_smc_absent
    #define se_smartcard_read_aes_key                   smartcard_highlevel_read_aes_key
    #define se_smartcard_detect_scan                    smartcard_lowlevel_detect
    #define se_smartcard_erase_se                       smartcard_highlevel_erase_smartcard
#else
    #define SMARTCARD_CPZ_LENGTH        8
    #define SMARTCARD_DEFAULT_PIN       0xF0F0
    
    /* Mooltipass Mini BLE v2 */
    #define se_smartcard_mooltipass_se_det_routine(...) RETURN_MOOLTIPASS_PB
    #define se_smartcard_check_for_hidden_aes_key(...)  RETURN_OK
    #define se_smartcard_read_code_protected_zone(...)
    #define se_smartcard_write_protected_zone(...)
    #define se_smartcard_se_detect_routine(...)         RETURN_MOOLTIPASS_INVALID
    #define se_smartcard_get_nb_tries_left(...)         4
    #define se_smartcard_write_pin_code(...)
    #define se_smartcard_write_aes_key(...)             RETURN_OK
    #define se_smartcard_is_se_plugged(...)             RETURN_REL
    #define se_smartcard_is_se_absent(...)              RETURN_OK
    #define se_smartcard_read_aes_key(...)
    #define se_smartcard_detect_scan(...)
    #define se_smartcard_erase_se(...)
#endif

#endif /* SE_SMARTCARD_WRAPPER_H_ */