/*!  \file     gui_prompts.c
*    \brief    Code dedicated to prompts and notifications
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/


#ifndef GUI_PROMPTS_H_
#define GUI_PROMPTS_H_

#include "defines.h"

/* Defines */
// PIN prompt
#define PIN_PROMPT_MAX_TEXT_X   100
#define PIN_PROMPT_TEXT_Y       22
#define PIN_PROMPT_DIGIT_Y      20
#define PIN_PROMPT_ASTX_Y_INC   2
#define PIN_PROMPT_DIGIT_X_OFFS 140
#define PIN_PROMPT_DIGIT_X_SPC  17
#define PIN_PROMPT_DIGIT_Y_WDW  20


/* Prototypes */
void gui_prompts_render_pin_enter_screen(uint8_t* current_pin, uint16_t selected_digit, uint16_t stringID, int16_t anim_direction);
RET_TYPE gui_prompts_get_user_pin(volatile uint16_t* pin_code, uint16_t stringID);

#endif /* GUI_PROMPTS_H_ */