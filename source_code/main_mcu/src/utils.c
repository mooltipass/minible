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

/*! \fn     utils_u8strlen(uint8_t* string)
*   \brief  strlen for u8 strings
*   \param  string      The string
*   \return The string length
*/
uint16_t utils_u8strlen(uint8_t* string)
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

/*! \fn     utils_get_number_of_given_char(cust_char_t* string, cust_char_t needed_char)
*   \brief  Get the number of occurences of a given char in a 0 terminated string
*   \param  string      The string
*   \param  needed_char The char we're looking for
*   \return Number of occurences
*/
uint16_t utils_get_number_of_given_char(cust_char_t* string, cust_char_t needed_char)
{
    uint16_t ret_val = 0;
    while (*string)
    {
        if (*string == needed_char)
        {
            ret_val++;
        }
        string++;
    }
    return ret_val;
}

/*! \fn     utils_get_string_next_line_pt(cust_char_t* string)
*   \brief  Get a pointer to the next line in the current string
*   \param  string      The string
*   \return Pointer to the next line or 0 if no new line
*   \note   If the string is not terminated this function will freeze
*/
cust_char_t* utils_get_string_next_line_pt(cust_char_t* string)
{
    BOOL new_char_found = FALSE;
    while (*string)
    {
        if ((*string == '\r') ||  (*string == '\n'))
        {
            new_char_found = TRUE;
        }
        else if (new_char_found != FALSE)
        {
            return string;
        }
        string++;
    }
    return (cust_char_t*)0;
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

/*! \fn     utils_strcpy(cust_char_t* destination, cust_char_t const* source)
*   \brief  Our own version of strcpy
*   \param  destination     Where to store
*   \param  source          The source
*   \return Number of chars copied
*/
uint16_t utils_strcpy(cust_char_t* destination, cust_char_t const* source)
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

/*! \fn     utils_u8strnlen(uint8_t* string, uint16_t maxlen)
*   \brief  Our own custom strnlen for uint8_t strings
*   \param  string      The string
*   \param  maxlen      Max Length
*   \return The string length
*/
uint16_t utils_u8strnlen(uint8_t const* string, uint16_t maxlen)
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
        string[i*2] = string[i];
        string[i*2 + 1] = 0;
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

/*! \fn     utils_utf8_to_bmp(uint8_t* input, cust_char_t* output)
*   \brief  Decode a utf8 point to a unicode BMP codepoint
*   \param  input       Input buffer
*   \param  output      Where to store the result
*   \return How many bytes were read at the input, -1 if the parsing was erroneous
*/
int16_t utils_utf8_to_bmp(uint8_t* input, cust_char_t* output)
{
    uint8_t lead;
    uint8_t mask;
    uint16_t remunits;
    uint16_t temp_index = 0;
    uint8_t nxt = input[temp_index++];

    if ((nxt & 0x80) == 0) 
    {
        /* ascii char */
        *output = (cust_char_t)nxt;
        return 1;
    } 
    else if ((nxt & 0xC0) == 0x80) 
    {
        /* invalid value */
        return -1;
    }
    else 
    {
        /* Initial value for lead and mask to match utf8 2 bytes */
        lead = 0xC0;
        mask = 0xE0;

        /* Bit shift until we match the lead */
        for (remunits = 1; (nxt & mask) != lead; ++remunits) 
        {
            lead = mask;
            mask >>= 1;
            mask |= 0x80;
        }

        /* Check for valid number of remunits for fit in unicode BMP */
        if (remunits >= 3)
        {
            return -1;
        }

        /* Remove the lead */
        *output = (cust_char_t)(nxt ^ lead);

        /* Add the remaining data */
        while (remunits-- > 0) 
        {
            *output <<= 6;
            *output |= (input[temp_index++] & 0x3F);
        }

        return temp_index;
    }
}

/*! \fn     utils_utf8_encode_bmp(cust_char_t codepoint, uint8_t* buf_out, uint16_t max_writes)
*   \brief  Encode a BMP code point into unicode string
*   \param  codepoint   The code point
*   \param  buf_out     Output buffer
*   \param  max_writes  Maximum writes we're allowed
*   \return How many bytes were written in the destination, not including terminating 0 (max 3), -1 if couldn't write
*/
int16_t utils_utf8_encode_bmp(cust_char_t codepoint, uint8_t* buf_out, uint16_t max_writes)
{
    if (codepoint <= 0x7F) 
    {
        if (max_writes >= 2)
        {
            // Plain ASCII
            buf_out[0] = (uint8_t)codepoint;
            buf_out[1] = 0;
            return 1;
        } 
        else
        {
            return -1;
        }
    }
    else if (codepoint <= 0x07FF) 
    {
        if (max_writes >= 3)
        {
            // 2-byte unicode
            buf_out[0] = (uint8_t)(((codepoint >> 6) & 0x1F) | 0xC0);
            buf_out[1] = (uint8_t)(((codepoint >> 0) & 0x3F) | 0x80);
            buf_out[2] = 0;
            return 2;
        } 
        else
        {
            return -1;
        }
    }
    else if (codepoint <= 0xFFFF) 
    {
        if (max_writes >= 4)
        {
            // 3-byte unicode
            buf_out[0] = (uint8_t)(((codepoint >> 12) & 0x0F) | 0xE0);
            buf_out[1] = (uint8_t)(((codepoint >>  6) & 0x3F) | 0x80);
            buf_out[2] = (uint8_t)(((codepoint >>  0) & 0x3F) | 0x80);
            buf_out[3] = 0;
            return 3;
        } 
        else
        {
            return -1;
        }
    }
    else
    {
        // Not supported
        return -1;
    }
}

/*! \fn     utils_bmp_string_to_utf8_string(cust_char_t* bmp_string, uint8_t* utf8_string, uint16_t utf8_string_len)
*   \brief  Encode a BMP string into a 0 terminated utf8 string
*   \param  bmp_string          Pointer to bmp string
*   \param  utf8_string         Pointer to utf8 string
*   \param  utf8_string_len     Maximum bytes we can write in the utf8 string
*   \return Number of bytes written excluding terminating 0, or something < 0 if we couldn't
*/
int16_t utils_bmp_string_to_utf8_string(cust_char_t* bmp_string, uint8_t* utf8_string, uint16_t utf8_string_len)
{
    int16_t nb_bytes_written_in_unicode_string;
    int16_t total_bytes_written = 0;

    while (*bmp_string)
    {
        /* Try to write into string, returns nb bytes written + 1 for terminating 0 */
        nb_bytes_written_in_unicode_string = utils_utf8_encode_bmp(*bmp_string, utf8_string, utf8_string_len);

        if (nb_bytes_written_in_unicode_string < 0)
        {
            /* Not enough space */
            return -1;
        }
        else
        {
            /* Increment pointed codepoint, decrease available space */
            total_bytes_written += nb_bytes_written_in_unicode_string;
            utf8_string_len -= nb_bytes_written_in_unicode_string;
            utf8_string += nb_bytes_written_in_unicode_string;
            bmp_string++;
        }
    }

    return total_bytes_written;
}

/*! \fn     utils_utf8_string_to_bmp_string(uint8_t* utf8_string, cust_char_t* bmp_string, uint16_t utf8_string_len, uint16_t bmp_string_len)
*   \brief  Encode a utf8 string into a BMP string
*   \param  utf8_string         Pointer to a 0 terminated utf8 string
*   \param  bmp_string          Pointer to bmp string
*   \param  utf8_string_len     Maximum bytes we can read in the utf8 string
*   \return Number of code points written excluding terminating 0, or something < 0 if we couldn't do the conversion (invalid utf8 code points for BMP)
*   \note   BMP string should be at least the same size as the utf8 string
*/
int16_t utils_utf8_string_to_bmp_string(uint8_t* utf8_string, cust_char_t* bmp_string, uint16_t utf8_string_len, uint16_t bmp_string_len)
{
    int16_t nb_bytes_read_in_unicode_string_for_a_cp;
    int16_t total_bytes_read = 0;
    int16_t nb_bmp_written = 0;

    do
    {
        /* Check if we still have space to write... */
        if (bmp_string_len == nb_bmp_written)
        {
            /* Who would be dumb enough to call this function with len set to 0? */
            *(bmp_string-1) = 0;
            return -1;
        }
        
        /* Try to get one bmp codepoint */
        nb_bytes_read_in_unicode_string_for_a_cp = utils_utf8_to_bmp(utf8_string, bmp_string);

        /* Parsing error? */
        if (nb_bytes_read_in_unicode_string_for_a_cp < 0)
        {
            /* Either invalid char or goes over bmp limit */
            return -1;
        } 
        else if ((nb_bytes_read_in_unicode_string_for_a_cp + total_bytes_read) > utf8_string_len)
        {
            /* Maliciously formed string to read above boundaries */
            *bmp_string = 0;
            return -1;
        }
        else
        {
            total_bytes_read += nb_bytes_read_in_unicode_string_for_a_cp;
            utf8_string += nb_bytes_read_in_unicode_string_for_a_cp;
            nb_bmp_written++;
        }
    } while(*bmp_string++);

    return nb_bmp_written-1;
}

/*! \fn     utils_get_SP(void)
*   \brief  Get current active Stack Pointer (SP/r13)
*   \return Stack Pointer
*/
__attribute__( ( always_inline ) ) inline uint32_t utils_get_SP(void)
{
#ifdef EMULATOR_BUILD
    return 0;
#else
    register uint32_t sp;

    asm volatile ("MOV %0, SP\n" : "=r" (sp) );
    return(sp);
#endif
}

/*! \fn     utils_itoa(int32_t value, cust_char_t *str, uint8_t str_len)
*   \brief  Convert integer to string prefixed with 0 padding
            Convert an unsigned integer to a string. Prefix string with 0s
            if the number of digits in integer is less than num_digits.
*   \param  value       Integer to convert to string
*   \param  num_digits  Number of digits including 0 padding
*   \param  str         Output string
*   \param  str_len     Length of str buffer
*/
void utils_itoa(uint32_t value, uint8_t num_digits, cust_char_t *str, uint8_t str_len)
{
    if (num_digits < str_len)
    {
        str[num_digits] = '\0';
        for(int8_t digit = num_digits - 1; digit >= 0; --digit)
        {
            str[digit] = value % 10 + '0';
            value /= 10;
        }
    }
}

