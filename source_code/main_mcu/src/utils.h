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
/*!  \file     utils.h
*    \brief    Useful functions
*    Created:  02/01/2019
*    Author:   Mathieu Stephan
*/


#ifndef UTILS_H_
#define UTILS_H_

#include "defines.h"


/* Prototypes */
void utils_concatenate_strings_with_slash(cust_char_t* string1, cust_char_t* string2, uint16_t storage_length);
int16_t utils_custchar_strncmp(cust_char_t* f_string, cust_char_t* sec_string, uint16_t nb_chars);
void utils_strncpy(cust_char_t* destination, cust_char_t* source, uint16_t max_chars);
void utils_surround_text_with_pointers(cust_char_t* text, uint16_t field_length);
uint16_t utils_check_value_for_range(uint16_t val, uint16_t min, uint16_t max);
uint16_t utils_strcpy(cust_char_t* destination, cust_char_t* source);
void utils_ascii_to_unicode(uint8_t* string, uint16_t nb_chars);
uint16_t utils_strnlen(cust_char_t* string, uint16_t maxlen);
uint16_t utils_get_nb_lines(const cust_char_t* string);
uint16_t utils_strlen(cust_char_t* string);

#endif /* UTILS_H_ */