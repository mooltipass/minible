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
/*!  \file     inputs.h
*    \brief    Scroll wheel driver
*    Created:  20/02/2018
*    Author:   Mathieu Stephan
*/


#ifndef INPUTS_H_
#define INPUTS_H_
#include "defines.h"

/* Defines */
// How many ms is considered as a long press
#define LONG_PRESS_MS               1000

/* Prototypes */
wheel_action_ret_te inputs_get_wheel_action(BOOL wait_for_action, BOOL ignore_incdec);
void inputs_set_inputs_invert_bool(BOOL invert_bool);
RET_TYPE inputs_get_last_returned_action(void);
det_ret_type_te inputs_is_wheel_clicked(void);
int16_t inputs_get_wheel_increment(void);
BOOL inputs_raw_is_wheel_released(void);
void inputs_clear_detections(void);
void inputs_scan(void);


#endif /* INPUTS_H_ */