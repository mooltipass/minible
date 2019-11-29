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
/*!  \file     driver_clocks.h
*    \brief    Platform clocks related functions
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/

#ifndef CLOCKS_POWER_H_
#define CLOCKS_POWER_H_

/* Prototypes */
void clocks_map_gclk_to_peripheral_clock(uint32_t gclk_id, uint32_t peripheral_clk_id);
void clocks_start_48MDFLL(void);


#endif /* CLOCKS_POWER_H_ */