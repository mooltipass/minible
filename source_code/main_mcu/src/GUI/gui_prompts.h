/*!  \file     gui_prompts.c
*    \brief    Code dedicated to prompts and notifications
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/


#ifndef GUI_PROMPTS_H_
#define GUI_PROMPTS_H_

#include "defines.h"


/* Prototypes */
void gui_prompts_render_pin_enter_screen(uint8_t* current_pin, uint16_t selected_digit, uint16_t stringID);
RET_TYPE gui_prompts_get_user_pin(volatile uint16_t* pin_code, uint16_t stringID);

#endif /* GUI_PROMPTS_H_ */