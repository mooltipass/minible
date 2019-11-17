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
/*!  \file     fuses.h
*    \brief    Functions dedicated to fuse checks
*    Created:  20/06/2018
*    Author:   Mathieu Stephan
*/
#ifndef FUSES_H_
#define FUSES_H_

#include "defines.h"

/* Prototypes */
RET_TYPE fuses_check_program(BOOL flash_fuses);



#endif /* FUSES_H_ */