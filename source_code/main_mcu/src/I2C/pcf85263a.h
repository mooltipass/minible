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
/*!  \file     pcf85263a.h
*    \brief    Driver for the pcf85263a
*    Created:  05/11/2024
*    Author:   Mathieu Stephan
*/
#include "platform_defines.h"

#if !defined(PCF85263A_H_) && defined(MINIBLE_V2)
#define PCF85263A_H_

/* Defines */
#define PCF85263A_ADDR  0x51

/* Macros */
#define DDIGITS_TO_U8(X)    (((X) >> 4)*10 + ((X) & 0x0F))

/* Structures */
typedef struct
{
    uint8_t hundredth_seconds;
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t days;
    uint8_t week_days;
    uint8_t months;
    uint8_t years;
} pcf85263a_time_structure_t;

/* Prototypes */
void pcf85263a_get_time(pcf85263a_time_structure_t* time_struct_pt);
RET_TYPE pcf85263a_init(void);


#endif /* PCF85263A_H_ */