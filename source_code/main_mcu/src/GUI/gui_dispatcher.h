/*!  \file     gui_dispatcher.h
*    \brief    GUI functions dispatcher
*    Created:  16/11/2018
*    Author:   Mathieu Stephan
*/
#include "defines.h"


#ifndef GUI_DISPATCHER_H_
#define GUI_DISPATCHER_H_

/* Defines */
#define GUI_BACK_ICON_ID        255
#define GUI_BACK_TEXT_ID        11

/* Enums */
typedef enum {  GUI_SCREEN_NINSERTED = 0,
                GUI_SCREEN_INSERTED_LCK,
                GUI_SCREEN_INSERTED_INVALID,
                GUI_SCREEN_INSERTED_UNKNOWN,
                GUI_SCREEN_MEMORY_MGMT,
                GUI_SCREEN_MAIN_MENU,
                GUI_SCREEN_BT,
                GUI_SCREEN_CATEGORIES,
                GUI_SCREEN_FAVORITES,
                GUI_SCREEN_LOGIN,
                GUI_SCREEN_LOCK,
                GUI_SCREEN_OPERATIONS,
                GUI_SCREEN_SETTINGS
             } gui_screen_te;

/* Prototypes */
void gui_dispatcher_set_current_screen(gui_screen_te screen, BOOL reset_states);
void gui_dispatcher_event_dispatch(wheel_action_ret_te wheel_action);
void gui_dispatcher_get_back_to_current_screen(void);
void gui_dispatcher_main_loop(void);


#endif /* GUI_DISPATCHER_H_ */