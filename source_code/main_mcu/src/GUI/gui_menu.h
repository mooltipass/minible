/*!  \file     gui_menu.h
*    \brief    Standardized code to handle our menus
*    Created:  17/11/2018
*    Author:   Mathieu Stephan
*/


#ifndef GUI_MENU_H_
#define GUI_MENU_H_

#include "defines.h"

/* Back picture & text ID */
#define GUI_BACK_ICON_ID        255
#define GUI_BACK_TEXT_ID        11

/* Main Menu picture & texts IDs */
#define GUI_BT_ICON_ID          120
#define GUI_CAT_ICON_ID         138
#define GUI_FAV_ICON_ID         147
#define GUI_LOGIN_ICON_ID       156
#define GUI_LOCK_ICON_ID        165
#define GUI_OPR_ICON_ID         174
#define GUI_SETTINGS_ICON_ID    183
// Text IDs
#define GUI_BT_TEXT_ID          1
#define GUI_CAT_TEXT_ID         2
#define GUI_FAV_TEXT_ID         3
#define GUI_LOGIN_TEXT_ID       4
#define GUI_LOCK_TEXT_ID        5
#define GUI_OPR_TEXT_ID         6
#define GUI_SETTINGS_TEXT_ID    7

/* Operations picture & text IDs */
#define GUI_CLONE_ICON_ID       192
#define GUI_CHANGE_PIN_ICON_ID  201
#define GUI_ERASE_USER_ICON_ID  282
// Text IDs
#define GUI_CLONE_TEXT_ID       8
#define GUI_CHANGE_PIN_TEXT_ID  9
#define GUI_ERASE_USER_TEXT_ID  10

/* Bluetooth picture & text IDs */
#define GUI_BT_ENABLE_ICON_ID       264
#define GUI_BT_DISABLE_ICON_ID      129
#define GUI_BT_UNPAIR_ICON_ID       273
#define GUI_NEW_PAIR_ICON_ID        210
// Text IDs
#define GUI_BT_ENABLE_TEXT_ID       12
#define GUI_BT_DISABLE_TEXT_ID      13
#define GUI_BT_UNPAIR_DEV_TEXT_ID   14
#define GUI_NEW_PAIR_TEXT_ID        15

/* Settings picture & text IDs */
#define GUI_KEYB_LAYOUT_CHANGE_ICON_ID  219
#define GUI_CRED_PROMPT_CHANGE_ICON_ID  300
#define GUI_PWD_DISP_CHANGE_ICON_ID     228
#define GUI_WHEEL_ROT_FLIP_ICON_ID      237
#define GUI_LANGUAGE_SWITCH_ICON_ID     246
// Text IDs
#define GUI_KEYB_LAYOUT_CHANGE_TEXT_ID  16
#define GUI_CRED_PROMPT_CHANGE_TEXT_ID  17
#define GUI_PWD_DISP_CHANGE_TEXT_ID     18
#define GUI_WHEEL_ROT_FLIP_TEXT_ID      19
#define GUI_LANGUAGE_SWITCH_TEXT_ID     20

/* Enums */
typedef enum    {MAIN_MENU = 0, BT_MENU = 1, OPERATION_MENU = 2, SETTINGS_MENU = 3, NB_MENUS = 4} menu_te;

/* Prototypes */
BOOL gui_menu_event_render(wheel_action_ret_te wheel_action);
void gui_menu_reset_selected_items(BOOL reset_main_menu);
void gui_menu_set_selected_menu(menu_te menu);
void gui_menu_update_menus(void);


#endif /* GUI_MENU_H_ */