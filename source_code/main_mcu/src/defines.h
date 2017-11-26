/*!  \file     defines.h
*    \brief    Generic type defines
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/
#ifndef DEFINES_H_
#define DEFINES_H_

#include <stdio.h>

/* Standard defines */
#define FALSE   0
#define TRUE    (!FALSE)

/* Enums */
typedef enum    {RETURN_CARD_NDET, RETURN_CARD_TEST_PB, RETURN_CARD_4_TRIES_LEFT,  RETURN_CARD_3_TRIES_LEFT,  RETURN_CARD_2_TRIES_LEFT,  RETURN_CARD_1_TRIES_LEFT, RETURN_CARD_0_TRIES_LEFT} card_detect_return_te;
typedef enum    {RETURN_REL = 0, RETURN_DET, RETURN_JDETECT, RETURN_JRELEASED} det_ret_type_te;
typedef enum    {RETURN_NOK = -1, RETURN_OK = 0} ret_type_te;

/* Typedefs */
typedef void (*void_function_ptr_type_t)(void);
typedef uint16_t cust_char_t;
typedef ret_type_te RET_TYPE;
typedef uint32_t nat_type_t;
typedef int32_t BOOL;

#endif /* DEFINES_H_ */