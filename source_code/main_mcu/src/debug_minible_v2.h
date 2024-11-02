/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2024 Stephan Mathieu
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
/*!  \file     debug_minible_v2.h
*    \brief    Debug functions for our platform
*    Created:  30/10/2024
*    Author:   Mathieu Stephan
*/


#ifndef DEBUG_MINIBLE_V2_H_
#define DEBUG_MINIBLE_V2_H_

#include "platform_defines.h"
#ifdef MINIBLE_V2

/* Prototypes */
void debug_accelerometer_battery_display(void);
void debug_debug_menu(void);


#endif /* MINIBLE_V2 */
#endif /* DEBUG_MINIBLE_V2_H_ */