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
/*!  \file     gui_menu.h
*    \brief    Standardized code to handle our menus
*    Created:  17/11/2018
*    Author:   Mathieu Stephan
*/


#ifndef GUI_MENU_H_
#define GUI_MENU_H_

#include "defines.h"

/* Back picture ID */
#define GUI_BACK_ICON_ID        255

/* Main Menu picture & texts IDs */
#define GUI_BT_ICON_ID          120
#define GUI_CAT_ICON_ID         138
#define GUI_FAV_ICON_ID         147
#define GUI_LOGIN_ICON_ID       156
#define GUI_LOCK_ICON_ID        165
#define GUI_OPR_ICON_ID         174
#define GUI_SETTINGS_ICON_ID    183

/* Operations picture IDs */
#define GUI_CLONE_ICON_ID       192
#define GUI_CHANGE_PIN_ICON_ID  201
#define GUI_ERASE_USER_ICON_ID  282
#define GUI_SIMPLE_ADV_ICON_ID  689

/* Bluetooth picture & text IDs */
#define GUI_BT_ENABLE_ICON_ID       264
#define GUI_BT_DISABLE_ICON_ID      129
#define GUI_BT_UNPAIR_ICON_ID       273
#define GUI_NEW_PAIR_ICON_ID        210

/* Settings picture & text IDs */
#define GUI_KEYB_LAYOUT_CHANGE_ICON_ID  219
#define GUI_CRED_PROMPT_CHANGE_ICON_ID  300
#define KNOCK_DETECTION_ICON_ID     228
#define GUI_WHEEL_ROT_FLIP_ICON_ID      237
#define GUI_LANGUAGE_SWITCH_ICON_ID     246
#define GUI_PIN_FOR_MMM_ICON_ID         671
#define GUI_MMM_STORAGE_CONF_ICON_ID    680

/* Enums */
typedef enum    {MAIN_MENU = 0, BT_MENU = 1, OPERATION_MENU = 2, SETTINGS_MENU = 3, NB_MENUS = 4} menu_te;

/* Prototypes */
BOOL gui_menu_event_render(wheel_action_ret_te wheel_action);
void gui_menu_reset_selected_items(BOOL reset_main_menu);
void gui_menu_set_selected_menu(menu_te menu);
void gui_menu_update_menus(void);


#endif /* GUI_MENU_H_ */