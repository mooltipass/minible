/*!  \file     utils.c
*    \brief    Useful functions
*    Created:  02/01/2019
*    Author:   Mathieu Stephan
*/
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