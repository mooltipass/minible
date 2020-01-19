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
/*!  \file     utils.c
*    \brief    Useful functions
*    Created:  02/01/2019
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include "utils.h"

/*! \fn     utils_strlen(cust_char_t* string)
*   \brief  Our own custom strlen
*   \param  string      The string
*   \return The string length
*/
uint16_t utils_strlen(cust_char_t* string)
{
    uint16_t i;
    for (i = 0; string[i] != 0; i++);
    return i;
}

/*! \fn     utils_get_nb_lines(const cust_char_t* string)
*   \brief  Count number of lines in string
*   \param  string      The string
*   \return The number of lines
*/
uint16_t utils_get_nb_lines(const cust_char_t* string)
{
    uint16_t return_val = 1;
    while (*string)
    {
        if (*string == '\r')
        {
            return_val++;
        }
        string++;
    }
    return return_val;
}

/*! \fn     utils_strncpy(cust_char_t* destination, cust_char_t* source, uint16_t max_chars)
*   \brief  Our own version of strncpy
*   \param  destination     Where to store
*   \param  source          The source
*   \param  max_chars       Maximum number of chars to copy
*/
void utils_strncpy(cust_char_t* destination, cust_char_t* source, uint16_t max_chars)
{
    uint16_t i;
    for (i = 0; (i < max_chars) && (source[i] != 0); i++)
    {
        destination[i] = source[i];
    }
    
    /* Terminating 0 */
    if (i < max_chars)
    {
        destination[i] = 0;
    }
}

/*! \fn     utils_strcpy(cust_char_t* destination, cust_char_t* source)
*   \brief  Our own version of strcpy
*   \param  destination     Where to store
*   \param  source          The source
*   \return Number of chars copied
*/
uint16_t utils_strcpy(cust_char_t* destination, cust_char_t* source)
{
    uint16_t return_val = 0;
    
    while (*source != 0)
    {
        *destination = *source;
        destination++;
        return_val++;
        source++;
    }
    *destination = 0;
    
    return return_val;
}

/*! \fn     utils_strnlen(cust_char_t* string, uint16_t maxlen)
*   \brief  Our own custom strnlen
*   \param  string      The string
*   \param  maxlen      Max Length
*   \return The string length
*/
uint16_t utils_strnlen(cust_char_t* string, uint16_t maxlen)
{
    uint16_t i;
    for (i = 0; (string[i] != 0) && (i < maxlen); i++);
    return i;
}

/*! \fn     utils_custchar_strncmp(cust_char_t* f_string, cust_char_t* sec_string, uint16_t nb_chars)
*   \brief  Implementation of strncmp
*   \param  f_string    First string
*   \param  sec_string  Second string
*   \param  nb_chars    Maximum number of chars
*   \return positive if f_string comes after sec_string, negative if not, 0 if equal strings
*/
int16_t utils_custchar_strncmp(cust_char_t* f_string, cust_char_t* sec_string, uint16_t nb_chars)
{
    for (uint16_t i = 0; (i < nb_chars); i++)
    {
        if (f_string[i] < sec_string[i])
        {
            return -1;
        }
        else if (sec_string[i] < f_string[i])
        {
            return 1;
        }
        else if ((f_string[i] == 0) && (sec_string[i] == 0))
        {
            return 0;
        }
    }
    
    return 0;
}

/*! \fn     utils_ascii_to_unicode(uint8_t* string, uint16_t nb_chars)
*   \brief  Convert ascii string to unicode
*   \param  string      The string
*   \param  nb_chars    String length
*/
void utils_ascii_to_unicode(uint8_t* string, uint16_t nb_chars)
{
    for (int16_t i = nb_chars - 1; i >= 0; i--)
    {
        string[i*2] = 0;
        string[i*2 + 1] = string[i];
    }
}

/*! \fn     utils_check_value_for_range(uint16_t val, uint16_t min, uint16_t max)
*   \brief  Make sure a given value is within a range
*   \param  val     The value to check
*   \param  min     Minimum value
*   \param  max     Maximum value
*   \return Controlled value
*/
uint16_t utils_check_value_for_range(uint16_t val, uint16_t min, uint16_t max)
{
    if (val < min)
    {
        return min;
    }
    else if (val > max)
    {
        return max;
    }
    else
    {
        return val;
    }
}

/*! \fn     utils_surround_text_with_pointers(cust_char_t* text, uint16_t field_length)
*   \brief  Surround a text with "> <"
*   \param  text            Pointer to the text
*   \param  field_length    Text max field length
*/
void utils_surround_text_with_pointers(cust_char_t* text, uint16_t field_length)
{
    uint16_t text_length = utils_strnlen(text, field_length);
    
    /* Do we have enough space to do what we want? */
    if (text_length + 4 + 1 <= field_length)
    {
        /* Shift */
        for (int16_t i = text_length; i >= 0; i--)
        {
            text[i+2] = text[i];
        }
        
        /* Add pointers */
        text[0] = '>';
        text[1] = ' ';
        text[text_length+2] = ' ';
        text[text_length+3] = '<';
        text[text_length+4] = 0;
    }
}

/*! \fn     utils_concatenate_strings_with_slash(cust_char_t* string1, cust_char_t* string2, uint16_t storage_length)
*   \brief  Generate a string like "string1/string2"
*   \param  string1         First string, where the result will be stored
*   \param  string2         Second string
*   \param  storage_length  How many chars we can put in string 1
*   \note   both strings should be 0 terminated
*/
void utils_concatenate_strings_with_slash(cust_char_t* string1, cust_char_t* string2, uint16_t storage_length)
{
    uint16_t string1_length = utils_strlen(string1);
    uint16_t string2_length = utils_strlen(string2);
    
    /* Do we have enough space to do what we want? */
    if (string1_length + string2_length + 1 + 1 <= storage_length)
    {
        string1[string1_length] = '/';
        utils_strcpy(&string1[string1_length+1], string2);
    }    
}

/*! \fn     utils_fill_uint16_array_with_value(uint16_t* array, uint16_t size, uint16_t value)
*   \brief  Fill a uint16_t array with a given value
*   \param  array           Pointer to array
*   \param  size            Size of array
*   \param  value           Value to fill the array with
*/
void utils_fill_uint16_array_with_value(uint16_t* array, uint16_t size, uint16_t value)
{
    for (uint16_t i = 0; i < size; i++)
    {
        array[i] = value;
    }
}

/*! \fn     utils_get_cbor_encoded_value_for_val_btw_m24_p23(int8_t value)
*   \brief  Get the cbor encoded value for a value between -24 and 23 included
*   \param  value   The value to encode
*   \return Encoded value
*/
uint8_t utils_get_cbor_encoded_value_for_val_btw_m24_p23(int8_t value)
{
    if (value >= 0)
    {
        /* Positive/unsigned integer major type value is 0 */
        return (uint8_t)value;
    }
    else
    {
        /* Major type value 1 */
        uint8_t return_val = 0x20;

        /* Reverse sign and remove one */
        return_val |= ~value;

        return return_val;
    }
}

/*! \fn     utils_cbor_encode_32byte_bytestring(uint8_t* source, uint8_t* destination)
*   \brief  CBOR encode a 32bytes long bytestring
*   \param  source      source bytestring
*   \param  destination Where to store the result (should be 2 bytes longer than source!)
*   \return How many bytes were written in the destination (34)
*/
uint8_t utils_cbor_encode_32byte_bytestring(uint8_t* source, uint8_t* destination)
{
    destination[0] = 0x40 + 0x18;   // Bytestring type + next byte is uint8_t payload length
    destination[1] = 32;            // Payload length
    memcpy(&(destination[2]), source, 32);
    return 34;
}



