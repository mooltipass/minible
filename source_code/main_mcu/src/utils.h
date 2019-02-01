/*!  \file     utils.h
*    \brief    Useful functions
*    Created:  02/01/2019
*    Author:   Mathieu Stephan
*/


#ifndef UTILS_H_
#define UTILS_H_

#include "defines.h"


/* Prototypes */
int16_t utils_custchar_strncmp(cust_char_t* f_string, cust_char_t* sec_string, uint16_t nb_chars);
uint16_t utils_check_value_for_range(uint16_t val, uint16_t min, uint16_t max);
uint16_t utils_strlen(cust_char_t* string);

#endif /* UTILS_H_ */