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
/*!  \file     mp2710.h
*    \brief    Driver for the MP2710
*    Created:  13/10/2024
*    Author:   Mathieu Stephan
*/
#include "platform_defines.h"

#if !defined(MP2710_H_) && defined(MINIBLE_V2)
#define MP2710_H_

/* Defines */
#define MP2710_ADDR     0x08

/* Prototypes */
RET_TYPE mp2710_init(void);


#endif /* MP2710_H_ */