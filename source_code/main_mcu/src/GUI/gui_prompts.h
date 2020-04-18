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
/*!  \file     gui_prompts.c
*    \brief    Code dedicated to prompts and notifications
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/


#ifndef GUI_PROMPTS_H_
#define GUI_PROMPTS_H_

#include "defines.h"

/* Defines */

// Bitmap ID defines
#define BITMAP_POPUP_3LINES_ID          309
#define BITMAP_YES_PRESS_ID             338
#define BITMAP_NO_PRESS_ID              323
#define BITMAP_NY_DOWN_ID               353
#define BITMAP_NY_UP_ID                 367
#define POPUP_3LINES_ANIM_LGTH          14
#define BITMAP_POPUP_2LINES_Y           381
#define BITMAP_POPUP_2LINES_N           395
#define BITMAP_POPUP_2LINES_Y_DESEL     795
#define BITMAP_POPUP_2LINES_N_SELEC     809
#define POPUP_2LINES_ANIM_LGTH          14
#define BITMAP_2LINES_SEL_Y             409
#define BITMAP_2LINES_SEL_N             417
#define CONF_2LINES_SEL_AN_LGTH         8
#define BITMAP_2LINES_PRESS_Y           425
#define BITMAP_2LINES_PRESS_N           439
#define CONF_2LINES_IDLE_AN_LGT         12
#define BITMAP_2LINES_IDLE_Y            454
#define BITMAP_2LINES_IDLE_N            466
#define CONF_3LINES_IDLE_AN_LGT         12
#define BITMAP_3LINES_IDLE_Y            478
#define BITMAP_3LINES_IDLE_N            490
#define BITMAP_POPUP_USB                746
#define BITMAP_POPUP_BLE                698
#define BITMAP_USB_PRESS                760
#define BITMAP_BLE_PRESS                712
#define BITMAP_USB_SELECT               786
#define BITMAP_BLE_SELECT               738
#define BITMAP_USB_IDLE                 774
#define BITMAP_BLE_IDLE                 726
#define BITMAP_POPUP_USB_DESEL          837
#define BITMAP_POPUP_BLE_SEL            823

// PIN prompt
#define PIN_PROMPT_ARROW_MOV_LGTH       7
#define PIN_PROMPT_MAX_TEXT_X           100
#define PIN_PROMPT_1LTEXT_Y             23
#define PIN_PROMPT_2LTEXT_Y             12
#define PIN_PROMPT_3LTEXT_Y             6
#define PIN_PROMPT_ARROW_HOR_ANIM_STEP  4
#define PIN_PROMPT_DIGIT_X_OFFS         100
#define PIN_PROMPT_OFFSET_FOR_FOUR_DIG  34
#define PIN_PROMPT_DIGIT_X_ADJ          15
#define PIN_PROMT_DIGIT_AST_ADJ         (-1)
#define PIN_PROMPT_UP_ARROW_Y           4
#define PIN_PROMPT_ARROW_HEIGHT         16
#define PIN_PROMPT_DIGIT_HEIGHT         19
#define PIN_PROMPT_DIGIT_X_SPC          24
#define PIN_PROMPT_ASTX_Y_INC           4
#define PIN_PROMPT_DIGIT_Y_SPACING      2
#define PIN_PROMPT_POPUP_ANIM_LGTH      14
#define BITMAP_PIN_UP_ARROW_POP_ID      566
#define BITMAP_PIN_DN_ARROW_POP_ID      580
#define BITMAP_PIN_UP_ARROW_MOVE_ID     628
#define BITMAP_PIN_DN_ARROW_MOVE_ID     635
#define BITMAP_PIN_UP_ARROW_ACTIVATE_ID 642
#define BITMAP_PIN_DN_ARROW_ACTIVATE_ID 652

// Confirmation prompt
#define CONF_PROMPT_BITMAP_X            210
#define CONF_PROMPT_BITMAP_Y            1

// Information display
#define INF_DISPLAY_TEXT_Y              22
#define INFO_NOTIF_ANIM_LGTH            12
#define ACTION_NOTIF_ANIM_LGTH          11
#define WARNING_NOTIF_ANIM_LGTH         14
#define INFO_NOTIF_IDLE_ANIM_LGTH       20
#define ACTION_NOTIF_IDLE_ANIM_LGTH     21
#define WARNING_NOTIF_IDLE_ANIM_LGTH    20
#define BITMAP_INFO_NOTIF_POPUP_ID      502
#define BITMAP_INFO_NOTIF_IDLE_ID       514
#define BITMAP_ACTION_NOTIF_POPUP_ID    534
#define BITMAP_ACTION_NOTIF_IDLE_ID     545
#define BITMAP_WARNING_NOTIF_POPUP_ID   594
#define BITMAP_WARNING_NOTIF_IDLE_ID    608

// Scroll through logins display
#define LOGIN_SCROLL_Y_BAR              19
#define LOGIN_SCROLL_Y_FLINE            19
#define LOGIN_SCROLL_Y_SLINE            33
#define LOGIN_SCROLL_Y_TLINE            49
#define LOGIN_SCROLL_ANIM_DELAY         15

// Delay when scrolling a text
#define SCROLLING_DEL                   33

// Delay before starting to display hint
#define GUI_DELAY_BEFORE_HINT           5000
#define GUI_DELAY_HINT_BLINKING         5000

/* Structs */
typedef struct
{
    cust_char_t* lines[4];
} confirmationText_t;

/* Prototypes */
wheel_action_ret_te gui_prompts_render_pin_enter_screen(uint8_t* current_pin, uint16_t selected_digit, uint16_t stringID, int16_t vert_anim_direction, int16_t hor_anim_direction, BOOL six_digit_prompt);
mini_input_yes_no_ret_te gui_prompts_ask_for_confirmation(uint16_t nb_args, confirmationText_t* text_object, BOOL accept_cancel_message, BOOL parse_aux_messages, BOOL exit_on_power_change);
gui_info_display_ret_te gui_prompts_display_information_on_screen_and_wait(uint16_t string_id, display_message_te message_type, BOOL allow_scroll_or_msg_to_interrupt);
ret_type_te gui_prompts_select_language_or_keyboard_layout(BOOL layout_choice, BOOL ignore_timeout_if_usb_powered, BOOL ignore_card_removal, BOOL usb_layout_choice);
mini_input_yes_no_ret_te gui_prompts_ask_for_one_line_confirmation(uint16_t string_id, BOOL accept_cancel_message, BOOL usb_ble_prompt, BOOL first_item_selected);
void gui_prompts_display_information_on_string_single_anim_frame(uint16_t* frame_id, uint16_t* timer_timeout, display_message_te message_type);
void gui_prompts_display_information_lines_on_screen(confirmationText_t* text_lines, display_message_te message_type, uint16_t nb_lines);
void gui_prompts_display_3line_information_on_screen_and_wait(confirmationText_t* text_lines, display_message_te message_type);
mini_input_yes_no_ret_te gui_prompts_ask_for_login_select(uint16_t parent_node_addr, uint16_t* chosen_child_node_addr);
void gui_prompts_display_information_on_screen(uint16_t string_id, display_message_te message_type);
RET_TYPE gui_prompts_get_user_pin(volatile uint16_t* pin_code, uint16_t stringID);
RET_TYPE gui_prompts_get_six_digits_pin(uint8_t* pin_code, uint16_t stringID);
uint16_t gui_prompts_service_selection_screen(uint16_t start_address);
int16_t gui_prompts_favorite_selection_screen(int16_t start_favid);
gui_info_display_ret_te gui_prompts_wait_for_pairing_screen(void);
int16_t gui_prompts_select_category(void);

#endif /* GUI_PROMPTS_H_ */