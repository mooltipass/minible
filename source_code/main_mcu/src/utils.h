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
int16_t utils_utf8_string_to_bmp_string(uint8_t* utf8_string, cust_char_t* bmp_string, uint16_t utf8_string_len, uint16_t bmp_string_len);
int16_t utils_bmp_string_to_utf8_string(cust_char_t* bmp_string, uint8_t* utf8_string, uint16_t utf8_string_len);
void utils_concatenate_strings_with_slash(cust_char_t* string1, cust_char_t* string2, uint16_t storage_length);
int16_t utils_custchar_strncmp(cust_char_t* f_string, cust_char_t* sec_string, uint16_t nb_chars);
int16_t utils_utf8_encode_bmp(cust_char_t codepoint, uint8_t* buf_out, uint16_t max_writes);
void utils_fill_uint16_array_with_value(uint16_t* array, uint16_t size, uint16_t value);
void utils_strncpy(cust_char_t* destination, cust_char_t* source, uint16_t max_chars);
uint8_t utils_cbor_encode_32byte_bytestring(uint8_t* source, uint8_t* destination);
void utils_surround_text_with_pointers(cust_char_t* text, uint16_t field_length);
uint16_t utils_check_value_for_range(uint16_t val, uint16_t min, uint16_t max);
uint8_t utils_get_cbor_encoded_value_for_val_btw_m24_p23(int8_t value);
uint16_t utils_strcpy(cust_char_t* destination, cust_char_t* source);
uint16_t utils_u8strnlen(uint8_t const* string, uint16_t maxlen);
void utils_ascii_to_unicode(uint8_t* string, uint16_t nb_chars);
cust_char_t* utils_get_string_next_line_pt(cust_char_t* string);
int16_t utils_utf8_to_bmp(uint8_t* input, cust_char_t* output);
uint16_t utils_strnlen(cust_char_t* string, uint16_t maxlen);
uint16_t utils_get_nb_lines(const cust_char_t* string);
uint16_t utils_strlen(cust_char_t* string);
uint16_t utils_u8strlen(uint8_t* string);
uint32_t utils_get_SP(void);
static time_t utils_switch_endiannes(time_t input);

#endif /* UTILS_H_ */
