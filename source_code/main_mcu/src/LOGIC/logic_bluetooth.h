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
/*!  \file     logic_bluetooth.h
*    \brief    General logic for bluetooth
*    Created:  07/03/2019
*    Author:   Mathieu Stephan
*/ 


#ifndef LOGIC_BLUETOOTH_H_
#define LOGIC_BLUETOOTH_H_

#include "defines.h"

/* Enums */
typedef enum    {BT_STATE_CONNECTED = 0, BT_STATE_OFF, BT_STATE_ON} bt_state_te;

/* Prototypes */
void logic_bluetooth_set_do_not_lock_device_after_disconnect_flag(BOOL flag);
BOOL logic_bluetooth_get_do_not_lock_device_after_disconnect_flag(void);
platform_type_te logic_bluetooth_get_connected_to_platform_type(void);
BOOL logic_bluetooth_get_and_clear_too_many_failed_connections(void);
void logic_bluetooth_set_too_many_failed_connections(void);
void logic_bluetooth_get_unit_mac_address(uint8_t* buffer);
void logic_bluetooth_disconnect_from_current_device(void);
void logic_bluetooth_set_connected_state(BOOL state);
bt_state_te logic_bluetooth_get_state(void);


#endif /* LOGIC_BLUETOOTH_H_ */