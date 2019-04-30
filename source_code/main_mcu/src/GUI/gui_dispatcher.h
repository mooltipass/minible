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
// Strings
#define ID_STRING_REMOVE_CARD       22
#define ID_STRING_CARD_REMOVED      28
#define ID_STRING_UNKNOWN_CARD      36
#define ID_STRING_GOING_TO_SLEEP    37
#define ID_STRING_FW_FILE_UPDATE    38
#define ID_STRING_IN_MMM            47

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
gui_screen_te gui_dispatcher_get_current_screen(void);
void gui_dispatcher_get_back_to_current_screen(void);
void gui_dispatcher_idle_call(void);
void gui_dispatcher_main_loop(void);


#endif /* GUI_DISPATCHER_H_ */
