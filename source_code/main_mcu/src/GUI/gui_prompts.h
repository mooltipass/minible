/*!  \file     gui_prompts.c
*    \brief    Code dedicated to prompts and notifications
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/


#ifndef GUI_PROMPTS_H_
#define GUI_PROMPTS_H_

#include "defines.h"

/* Defines */

// Text defines
#define ADD_CRED_TEXT_ID        39
#define CHANGE_PWD_TEXT_ID      40
#define SEND_CREDS_FOR_TEXT_ID  41

// Bitmap ID defines
#define BITMAP_POPUP_3LINES_ID  309
#define BITMAP_YES_PRESS_ID     338
#define BITMAP_NO_PRESS_ID      323
#define BITMAP_NY_DOWN_ID       353
#define BITMAP_NY_UP_ID         367
#define POPUP_3LINES_ANIM_LGTH  14
#define BITMAP_POPUP_2LINES_Y   381
#define BITMAP_POPUP_2LINES_N   395
#define POPUP_2LINES_ANIM_LGTH  14
#define BITMAP_2LINES_SEL_Y     409
#define BITMAP_2LINES_SEL_N     417
#define CONF_2LINES_SEL_AN_LGTH 8
#define BITMAP_2LINES_PRESS_Y   425
#define BITMAP_2LINES_PRESS_N   439
#define CONF_2LINES_IDLE_AN_LGT 12
#define BITMAP_2LINES_IDLE_Y    454
#define BITMAP_2LINES_IDLE_N    466
#define CONF_3LINES_IDLE_AN_LGT 12
#define BITMAP_3LINES_IDLE_Y    478
#define BITMAP_3LINES_IDLE_N    490

// PIN prompt
#define PIN_PROMPT_ARROW_MOV_LGTH       7
#define PIN_PROMPT_MAX_TEXT_X           134
#define PIN_PROMPT_1LTEXT_Y             23
#define PIN_PROMPT_2LTEXT_Y             12
#define PIN_PROMPT_ARROW_HOR_ANIM_STEP  4
#define PIN_PROMPT_DIGIT_X_OFFS         134
#define PIN_PROMPT_DIGIT_X_ADJ          7
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

// Delay when scrolling a text
#define SCROLLING_DEL                   33

/* Structs */
typedef struct
{
    cust_char_t* lines[4];
} confirmationText_t;

/* Prototypes */
void gui_prompts_render_pin_enter_screen(uint8_t* current_pin, uint16_t selected_digit, uint16_t stringID, int16_t vert_anim_direction, int16_t hor_anim_direction);
mini_input_yes_no_ret_te gui_prompts_ask_for_confirmation(uint16_t nb_args, confirmationText_t* text_object, BOOL flash_screen);
void gui_prompts_display_information_on_screen_and_wait(uint16_t string_id, display_message_te message_type);
mini_input_yes_no_ret_te gui_prompts_ask_for_one_line_confirmation(uint16_t string_id, BOOL flash_screen);
void gui_prompts_display_information_on_screen(uint16_t string_id, display_message_te message_type);
RET_TYPE gui_prompts_get_user_pin(volatile uint16_t* pin_code, uint16_t stringID);

#endif /* GUI_PROMPTS_H_ */