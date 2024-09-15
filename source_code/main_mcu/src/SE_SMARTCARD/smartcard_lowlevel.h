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
/*!  \file     smartcard_lowlevel.h
*    \brief    Low level driver for AT88SC102 smartcard
*    Created:  28/11/2017
*    Author:   Mathieu Stephan
*/
#include "platform_defines.h"

#if !defined(SMARTCARD_H_) && !defined(MINIBLE_V2)
#define SMARTCARD_H_

#include "defines.h"
#include <stdint.h>

/* Typedefs */
typedef enum    {MAN_FUSE = 0, EC2EN_FUSE = 1, ISSUER_FUSE = 2} card_fuse_type_te;
    
/* Defines */
#define CARD_DELAY_FOR_PULLUP_SWITCH    150
#define CARD_DELAY_FOR_DETECTION        350

// Prototypes
RET_TYPE smartcard_lowlevel_check_for_const_val_in_smc_array(uint16_t nb_bytes_total_read, uint16_t start_record_index, uint8_t value);
uint8_t* smartcard_lowlevel_read_smc(uint16_t nb_bytes_total_read, uint16_t start_record_index, uint8_t* data_to_receive);
void smartcard_lowlevel_write_smc(uint16_t start_index_bit, uint16_t nb_bits, uint8_t* data_to_write);
pin_check_return_te smartcard_lowlevel_validate_code(volatile uint16_t* code);
void smartcard_lowlevel_erase_application_zone1_nzone2(BOOL zone1_nzone2);
card_detect_return_te smartcard_lowlevel_first_detect_function(void);
void smartcard_lowlevel_blow_fuse(card_fuse_type_te fuse_name);
det_ret_type_te smartcard_lowlevel_is_card_plugged(void);
void smartcard_lowlevel_write_nerase(BOOL is_write);
void smartcard_lowlevel_inverted_clock_pulse(void);
void smartcard_lowlevel_clear_pgmrst_signals(void);
void smartcard_lowlevel_set_pgmrst_signals(void);
RET_TYPE smartcard_low_level_is_smc_absent(void);
void smartcard_lowlevel_hpulse_delay(void);
void smartcard_lowlevel_clock_pulse(void);
void smartcard_lowlevel_detect(void);

// Defines
#define SMARTCARD_FABRICATION_ZONE  0x0F0F
#define SMARTCARD_FACTORY_PIN       0xF0F0
#define SMARTCARD_DEFAULT_PIN       0xF0F0
#define SMARTCARD_AZ_BIT_LENGTH     512
#define SMARTCARD_AZ1_BIT_START     176
#define SMARTCARD_AZ1_BIT_RESERVED  16
#define SMARTCARD_MTP_PASS_LENGTH   (SMARTCARD_AZ_BIT_LENGTH - SMARTCARD_AZ1_BIT_RESERVED - AES_KEY_LENGTH)
#define SMARTCARD_MTP_PASS_OFFSET   (SMARTCARD_AZ1_BIT_RESERVED + AES_KEY_LENGTH)
#define SMARTCARD_AZ2_BIT_START     736
#define SMARTCARD_AZ2_BIT_RESERVED  16
#define SMARTCARD_MTP_LOGIN_LENGTH  (SMARTCARD_AZ_BIT_LENGTH - SMARTCARD_AZ2_BIT_RESERVED - AES_KEY_LENGTH)
#define SMARTCARD_MTP_LOGIN_OFFSET  (SMARTCARD_AZ2_BIT_RESERVED + AES_KEY_LENGTH)
#define SMARTCARD_ISSUER_ZONE_LGTH  8
#define SMARTCARD_CPZ_LENGTH        8

#endif /* SMARTCARD_H_ */
