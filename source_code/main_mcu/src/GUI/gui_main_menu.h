/*!  \file     gui_main_menu.h
*    \brief    GUI for the main menu
*    Created:  17/11/2018
*    Author:   Mathieu Stephan
*/
#include "defines.h"

#ifndef GUI_MAIN_MENU_H_
#define GUI_MAIN_MENU_H_

/* Defines */
// Icon IDs
#define GUI_BT_ICON_ID          120
#define GUI_CAT_ICON_ID         120
#define GUI_FAV_ICON_ID         120
#define GUI_LOGIN_ICON_ID       120
#define GUI_LOCK_ICON_ID        120
#define GUI_OPR_ICON_ID         120
#define GUI_SETTINGS_ICON_ID    120
// Text IDs
#define GUI_BT_TEXT_ID          0
#define GUI_CAT_TEXT_ID         0
#define GUI_FAV_TEXT_ID         0
#define GUI_LOGIN_TEXT_ID       0
#define GUI_LOCK_TEXT_ID        0
#define GUI_OPR_TEXT_ID         0
#define GUI_SETTINGS_TEXT_ID    0


/* Prototypes */
void gui_main_menu_event_render(wheel_action_ret_te wheel_action);
void gui_main_menu_reset_state(void);


#endif /* GUI_MAIN_MENU_H_ */