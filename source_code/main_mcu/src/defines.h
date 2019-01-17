/*!  \file     defines.h
*    \brief    Generic type defines
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/
#ifndef DEFINES_H_
#define DEFINES_H_

#include <stdio.h>

/* Defines */
#define AUX_MCU_MESSAGE_REPLY_TIMEOUT_MS    500

/* Macros */
#define XSTR(x)         STR(x)
#define STR(x)          #x
#define ARRAY_SIZE(x)   (sizeof((x)) / sizeof((x)[0]))

/* Standard defines */
#define AES_KEY_LENGTH          256
#define AES_BLOCK_SIZE          128
#define FALSE                   0
#define TRUE                    (!FALSE)

/* Enums */
typedef enum    {RETURN_MOOLTIPASS_INVALID = 0, RETURN_MOOLTIPASS_PB = 1, RETURN_MOOLTIPASS_BLOCKED = 2, RETURN_MOOLTIPASS_BLANK = 3, RETURN_MOOLTIPASS_USER = 4, RETURN_MOOLTIPASS_0_TRIES_LEFT = 5, RETURN_MOOLTIPASS_1_TRIES_LEFT = 6, RETURN_MOOLTIPASS_2_TRIES_LEFT = 7, RETURN_MOOLTIPASS_3_TRIES_LEFT = 8, RETURN_MOOLTIPASS_4_TRIES_LEFT = 9} mooltipass_card_detect_return_te;
typedef enum    {WHEEL_ACTION_NONE = 0, WHEEL_ACTION_UP = 1, WHEEL_ACTION_DOWN = 2, WHEEL_ACTION_SHORT_CLICK = 3, WHEEL_ACTION_LONG_CLICK = 4, WHEEL_ACTION_CLICK_UP = 5, WHEEL_ACTION_CLICK_DOWN = 6, WHEEL_ACTION_DISCARDED = 7} wheel_action_ret_te;
typedef enum    {RETURN_CARD_NDET, RETURN_CARD_TEST_PB, RETURN_CARD_4_TRIES_LEFT,  RETURN_CARD_3_TRIES_LEFT,  RETURN_CARD_2_TRIES_LEFT,  RETURN_CARD_1_TRIES_LEFT, RETURN_CARD_0_TRIES_LEFT} card_detect_return_te;
typedef enum    {RETURN_PIN_OK = 0, RETURN_PIN_NOK_3, RETURN_PIN_NOK_2, RETURN_PIN_NOK_1, RETURN_PIN_NOK_0} pin_check_return_te;
typedef enum    {RETURN_REL = 0, RETURN_DET, RETURN_JDETECT, RETURN_JRELEASED} det_ret_type_te;
typedef enum    {CUSTOM_FS_INIT_OK = 0, CUSTOM_FS_INIT_NO_RWEE = 1} custom_fs_init_ret_type_te;
typedef enum    {RETURN_NOK = -1, RETURN_OK = 0} ret_type_te;

/* Typedefs */
typedef void (*void_function_ptr_type_t)(void);
typedef uint16_t cust_char_t;
typedef ret_type_te RET_TYPE;
typedef uint32_t nat_type_t;
typedef int32_t BOOL;

#endif /* DEFINES_H_ */