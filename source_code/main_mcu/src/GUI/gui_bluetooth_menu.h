/*!  \file     gui_bluetooth_menu.h
*    \brief    GUI for the bluetooth menu
*    Created:  19/11/2018
*    Author:   Mathieu Stephan
*/
#include "defines.h"

#ifndef GUI_BLUETOOTH_MENU_H_
#define GUI_BLUETOOTH_MENU_H_

/* Defines */
// Icon IDs
#define GUI_BT_ENABLE_ICON_ID       264
#define GUI_BT_DISABLE_ICON_ID      129
#define GUI_BT_UNPAIR_ICON_ID       273
#define GUI_NEW_PAIR_ICON_ID        210
// Text IDs
#define GUI_BT_ENABLE_TEXT_ID       12
#define GUI_BT_DISABLE_TEXT_ID      13
#define GUI_BT_UNPAIR_DEV_TEXT_ID   14
#define GUI_NEW_PAIR_TEXT_ID        15


/* Prototypes */
BOOL gui_bluetooth_menu_event_render(wheel_action_ret_te wheel_action);



#endif /* GUI_BLUETOOTH_MENU_H_ */