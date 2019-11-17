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
/*!  \file     rng.h
*    \brief    Random number generator
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/


#ifndef RNG_H_
#define RNG_H_

#include "defines.h"

/* Prototypes */
void rng_fill_array(uint8_t* array, uint16_t nb_bytes);
uint16_t rng_get_random_uint16_t(void);
uint8_t rng_get_random_uint8_t(void);
void rng_feed_from_acc_read(void);


#endif /* RNG_H_ */