/*!  \file     gui_settings_menu.h
*    \brief    GUI for the settings menu
*    Created:  22/11/2018
*    Author:   Mathieu Stephan
*/
#include "defines.h"


#ifndef GUI_SETTINGS_MENU_H_
#define GUI_SETTINGS_MENU_H_

/* Defines */
// Icon IDs
#define GUI_KEYB_LAYOUT_CHANGE_ICON_ID  263
#define GUI_CRED_PROMPT_CHANGE_ICON_ID  120
#define GUI_PWD_DISP_CHANGE_ICON_ID     276
#define GUI_WHEEL_ROT_FLIP_ICON_ID      289
#define GUI_BAT_REFRESH_ICON_ID         120
#define GUI_LANGUAGE_SWITCH_ICON_ID     302
// Text IDs
#define GUI_KEYB_LAYOUT_CHANGE_TEXT_ID  16
#define GUI_CRED_PROMPT_CHANGE_TEXT_ID  17
#define GUI_PWD_DISP_CHANGE_TEXT_ID     18
#define GUI_WHEEL_ROT_FLIP_TEXT_ID      19
#define GUI_BAT_REFRESH_TEXT_ID         20
#define GUI_LANGUAGE_SWITCH_TEXT_ID     21


/* Prototypes */
BOOL gui_settings_menu_event_render(wheel_action_ret_te wheel_action);


#endif /* GUI_SETTINGS_MENU_H_ */