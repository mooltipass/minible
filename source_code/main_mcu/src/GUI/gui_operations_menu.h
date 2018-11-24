/*!  \file     gui_operations_menu.h
*    \brief    GUI for the operations menu
*    Created:  19/11/2018
*    Author:   Mathieu Stephan
*/
#include "defines.h"

#ifndef GUI_OPERATIONS_MENU_H_
#define GUI_OPERATIONS_MENU_H_

/* Defines */
// Icon IDs
#define GUI_CLONE_ICON_ID       192
#define GUI_CHANGE_PIN_ICON_ID  201
#define GUI_ERASE_USER_ICON_ID  120
// Text IDs
#define GUI_CLONE_TEXT_ID       8
#define GUI_CHANGE_PIN_TEXT_ID  9
#define GUI_ERASE_USER_TEXT_ID  10


/* Prototypes */
BOOL gui_operations_menu_event_render(wheel_action_ret_te wheel_action);


#endif /* GUI_OPERATIONS_MENU_H_ */