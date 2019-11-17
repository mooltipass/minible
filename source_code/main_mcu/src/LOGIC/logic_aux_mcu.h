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
/*!  \file     logic_aux_mcu.h
*    \brief    Aux MCU logic
*    Created:  09/05/2018
*    Author:   Mathieu Stephan
*/

#ifndef LOGIC_AUX_MCU_H_
#define LOGIC_AUX_MCU_H_

#include "defines.h"

/* Prototypes */
void logic_aux_mcu_set_usb_enumerated_bool(BOOL usb_enumerated);
void logic_aux_mcu_set_ble_enabled_bool(BOOL ble_enabled);
void logic_aux_mcu_disable_ble(BOOL wait_for_disabled);
void logic_aux_mcu_enable_ble(BOOL wait_for_enabled);
RET_TYPE logic_aux_mcu_flash_firmware_update(void);
BOOL logic_aux_mcu_is_usb_just_enumerated(void);
uint32_t logic_aux_mcu_get_ble_chip_id(void);
BOOL logic_aux_mcu_is_usb_enumerated(void);
BOOL logic_aux_mcu_is_ble_enabled(void);

#endif /* LOGIC_AUX_MCU_H_ */