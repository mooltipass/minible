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
/*!  \file     defines.h
*    \brief    Generic type defines
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/
#ifndef DEFINES_H_
#define DEFINES_H_

#include <stdio.h>
#include <inttypes.h>

/* Defines */
#define AUX_MCU_MESSAGE_REPLY_TIMEOUT_MS    500

/* Fonts defines */
#define FONT_UBUNTU_MONO_BOLD_30_ID 0
#define FONT_UBUNTU_MEDIUM_15_ID    1
#define FONT_UBUNTU_MEDIUM_17_ID    2
#define FONT_UBUNTU_REGULAR_16_ID   3
#define FONT_UBUNTU_REGULAR_13_ID   4

/* Macros */
#define XSTR(x)                             STR(x)
#define STR(x)                              #x
#define ARRAY_SIZE(x)                       (sizeof((x)) / sizeof((x)[0]))
#define MEMBER_SIZE(type, member)           sizeof(((type*)0)->member)
#define MEMBER_ARRAY_SIZE(type, member)     (sizeof(((type*)0)->member) / sizeof(((type*)0)->member[0]))
#define MEMBER_SUB_ARRAY_SIZE(type, member) (sizeof(((type*)0)->member[0]) / sizeof(((type*)0)->member[0][0]))

/* Standard defines */
#define AES_KEY_LENGTH          256
#define AES_BLOCK_SIZE          128
#define AES256_CTR_LENGTH       AES_BLOCK_SIZE
#define FALSE                   0
#define TRUE                    (!FALSE)

/* Enums */
typedef enum    {RETURN_MOOLTIPASS_INVALID = 0, RETURN_MOOLTIPASS_PB = 1, RETURN_MOOLTIPASS_BLOCKED = 2, RETURN_MOOLTIPASS_BLANK = 3, RETURN_MOOLTIPASS_USER = 4, RETURN_MOOLTIPASS_0_TRIES_LEFT = 5, RETURN_MOOLTIPASS_1_TRIES_LEFT = 6, RETURN_MOOLTIPASS_2_TRIES_LEFT = 7, RETURN_MOOLTIPASS_3_TRIES_LEFT = 8, RETURN_MOOLTIPASS_4_TRIES_LEFT = 9} mooltipass_card_detect_return_te;
typedef enum    {WHEEL_ACTION_NONE = 0, WHEEL_ACTION_UP = 1, WHEEL_ACTION_DOWN = 2, WHEEL_ACTION_SHORT_CLICK = 3, WHEEL_ACTION_LONG_CLICK = 4, WHEEL_ACTION_CLICK_UP = 5, WHEEL_ACTION_CLICK_DOWN = 6, WHEEL_ACTION_DISCARDED = 7} wheel_action_ret_te;
typedef enum    {RETURN_CARD_NDET, RETURN_CARD_TEST_PB, RETURN_CARD_4_TRIES_LEFT,  RETURN_CARD_3_TRIES_LEFT,  RETURN_CARD_2_TRIES_LEFT,  RETURN_CARD_1_TRIES_LEFT, RETURN_CARD_0_TRIES_LEFT} card_detect_return_te;
typedef enum    {MINI_INPUT_RET_TIMEOUT = -1, MINI_INPUT_RET_NONE = 0, MINI_INPUT_RET_NO = 1, MINI_INPUT_RET_YES = 2, MINI_INPUT_RET_BACK = 3, MINI_INPUT_RET_CARD_REMOVED = 4} mini_input_yes_no_ret_te;
typedef enum    {HID_MSG_RCVD, HID_CANCEL_MSG_RCVD, BL_MSG_RCVD, EVENT_MSG_RCVD, MAIN_MCU_MSG_RCVD, RNG_MSG_RCVD, ERRONEOUS_MSG_RCVD, UNKNOW_MSG_RCVD, NO_MSG_RCVD, HID_REINDEX_BUNDLE_RCVD} comms_msg_rcvd_te;
typedef enum    {UNLOCK_OK_RET = 0, UNLOCK_BLOCKED_RET, UNLOCK_BACK_RET, UNLOCK_CARD_REMOVED_RET, UNLOCK_CARD_ISSUE_RET} unlock_ret_type_te;
typedef enum    {RETURN_VCARD_NOK = -1, RETURN_VCARD_OK = 0, RETURN_VCARD_UNKNOWN = 1, RETURN_VCARD_BACK = 2} valid_card_det_return_te;
typedef enum    {MSG_NO_RESTRICT, MSG_RESTRICT_ALL, MSG_RESTRICT_ALLBUT_BUNDLE, MSG_RESTRICT_ALLBUT_CANCEL} msg_restrict_type_te;
typedef enum    {RETURN_PIN_OK = 0, RETURN_PIN_NOK_3, RETURN_PIN_NOK_2, RETURN_PIN_NOK_1, RETURN_PIN_NOK_0} pin_check_return_te;
typedef enum    {OLED_STEPUP_SOURCE_NONE = 0, OLED_STEPUP_SOURCE_VBAT, OLED_STEPUP_SOURCE_3V3} oled_stepup_pwr_source_te;
typedef enum    {RETURN_NEW_PIN_NOK = -1, RETURN_NEW_PIN_OK = 0, RETURN_NEW_PIN_DIFF = 1} new_pinreturn_type_te;
typedef enum    {WAKEUP_REASON_NONE = 0, WAKEUP_REASON_AUX_MCU, WAKEUP_REASON_OTHER} platform_wakeup_reason_te;
typedef enum    {GUI_SEL_FAVORITE, GUI_SEL_SERVICE, GUI_SEL_DATA_SERVICE, GUI_SEL_CRED } gui_sel_item_te;
typedef enum    {DISP_MSG_INFO = 0, DISP_MSG_WARNING = 1, DISP_MSG_ACTION = 2} display_message_te;
typedef enum    {RETURN_REL = 0, RETURN_DET, RETURN_JDETECT, RETURN_JRELEASED} det_ret_type_te;
typedef enum    {CUSTOM_FS_INIT_OK = 0, CUSTOM_FS_INIT_NO_RWEE = 1} custom_fs_init_ret_type_te;
typedef enum    {COMPARE_MODE_MATCH = 0, COMPARE_MODE_COMPARE = 1} service_compare_mode_te;
typedef enum    {RETURN_BACK = -2, RETURN_NOK = -1, RETURN_OK = 0} ret_type_te;
typedef enum    {SERVICE_CRED_TYPE, SERVICE_DATA_TYPE} service_type_te;

/* Typedefs */
typedef void (*void_function_ptr_type_t)(void);
typedef uint16_t cust_char_t;
typedef ret_type_te RET_TYPE;
typedef uint32_t nat_type_t;
typedef int32_t BOOL;

#endif /* DEFINES_H_ */
