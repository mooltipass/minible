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
/*!  \file     functional_testing.h
*    \brief    File dedicated to the functional testing function
*    Created:  18/05/2019
*    Author:   Mathieu Stephan
*/



#ifndef FUNCTIONAL_TESTING_H_
#define FUNCTIONAL_TESTING_H_

#include "defines.h"

/* Prototypes */
void functional_testing_start(BOOL clear_first_boot_flag);
void functional_rf_testing_start(void);


#endif /* FUNCTIONAL_TESTING_H_ */