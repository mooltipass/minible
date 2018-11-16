/*!  \file     gui_dispatcher.c
*    \brief    GUI functions dispatcher
*    Created:  16/11/2018
*    Author:   Mathieu Stephan
*/
#include "gui_dispatcher.h"

// Current screen
gui_screen_te gui_dispatcher_current_screen = GUI_SCREEN_NINSERTED;


/*! \fn     gui_dispatcher_set_current_screen(gui_screen_te screen)
*   \brief  Set current screen
*   \param  screen  The screen
*/
void gui_dispatcher_set_current_screen(gui_screen_te screen)
{
    gui_dispatcher_current_screen = screen;
}

/*! \fn     gui_dispatcher_get_back_to_current_screen(void)
*   \brief  Get back to the current screen
*/
void gui_dispatcher_get_back_to_current_screen(void)
{
    /* switch to let the compiler optimize instead of function pointer array */
    switch (gui_dispatcher_current_screen)
    {
        case GUI_SCREEN_NINSERTED:          break;
        case GUI_SCREEN_INSERTED_LCK:       break;
        case GUI_SCREEN_INSERTED_INVALID:   break;
        case GUI_SCREEN_INSERTED_UNKNOWN:   break;
        case GUI_SCREEN_MEMORY_MGMT:        break;
        case GUI_SCREEN_MAIN_MENU:          break;
        case GUI_SCREEN_BT:                 break;
        case GUI_SCREEN_CATEGORIES:         break;
        case GUI_SCREEN_FAVORITES:          break;
        case GUI_SCREEN_LOGIN:              break;
        case GUI_SCREEN_LOCK:               break;
        case GUI_SCREEN_OPERATIONS:         break;
        case GUI_SCREEN_SETTINGS:           break;
        default: break;
    }
}

/*! \fn     gui_dispatcher_main_loop(void)
*   \brief  GUI main loop
*/
void gui_dispatcher_main_loop(void)
{
    
}