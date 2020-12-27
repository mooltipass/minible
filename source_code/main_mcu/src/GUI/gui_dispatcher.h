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
/*!  \file     gui_dispatcher.h
*    \brief    GUI functions dispatcher
*    Created:  16/11/2018
*    Author:   Mathieu Stephan
*/

#ifndef GUI_DISPATCHER_H_
#define GUI_DISPATCHER_H_

#include "defines.h"
#include "sh1122.h"

/* Defines */
// Bitmaps
#define GUI_ANIMATION_FFRAME_ID     0
#define GUI_ANIMATION_NBFRAMES      120
#define GUI_LOCKED_MINI_BITMAP_ID   453
#define GUI_NYANCAT_FFRAME_ID       899
#define GUI_NYANCAT_LFRAME_ID       910

// Animations
#define GUI_BATTERY_ANIM_DELAY_MS   500
#define GUI_NYANCAT_ANIM_DELAY_MS   66

/* Enums */
typedef enum {  GUI_SCREEN_INVALID = 0,
                GUI_SCREEN_NINSERTED,
                GUI_SCREEN_INSERTED_LCK,
                GUI_SCREEN_INSERTED_INVALID,
                GUI_SCREEN_INSERTED_UNKNOWN,
                GUI_SCREEN_MEMORY_MGMT,
                GUI_SCREEN_FW_FILE_UPDATE,
                GUI_SCREEN_LOGIN_NOTIF,
                GUI_SCREEN_CATEGORIES,
                GUI_SCREEN_FAVORITES,
                GUI_SCREEN_LOGIN,
                GUI_SCREEN_LOCK,
                GUI_SCREEN_MAIN_MENU,
                GUI_SCREEN_BT,
                GUI_SCREEN_OPERATIONS,
                GUI_SCREEN_SETTINGS
             } gui_screen_te;
             
/* Transitions */
#define     GUI_INTO_MENU_TRANSITION    OLED_IN_OUT_TRANS
#define     GUI_OUTOF_MENU_TRANSITION   OLED_OUT_IN_TRANS

/* Prototypes */
void gui_dispatcher_set_current_screen(gui_screen_te screen, BOOL reset_states, oled_transition_te transition);
void gui_dispatcher_event_dispatch(wheel_action_ret_te wheel_action);
void gui_dispatcher_display_battery_bt_overlay(BOOL write_to_buffer);
void gui_dispatcher_main_loop(wheel_action_ret_te wheel_action);
gui_screen_te gui_dispatcher_get_current_screen(void);
void gui_dispatcher_get_back_to_current_screen(void);
BOOL gui_dispatcher_is_screen_saver_running(void);
void gui_dispatcher_stop_screen_saver(void);
void gui_dispatcher_idle_call(void);


#endif /* GUI_DISPATCHER_H_ */