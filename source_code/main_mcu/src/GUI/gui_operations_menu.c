/*!  \file     gui_operations_menu.c
*    \brief    GUI for the operations menu
*    Created:  19/11/2018
*    Author:   Mathieu Stephan
*/
#include "gui_operations_menu.h"
#include "gui_dispatcher.h"
#include "gui_carousel.h"
#include "defines.h"
// Menu layout
const uint16_t operations_menu_pic_ids[] = {GUI_CLONE_ICON_ID, GUI_CHANGE_PIN_ICON_ID, GUI_ERASE_USER_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t operations_menu_text_ids[] = {GUI_CLONE_TEXT_ID, GUI_CHANGE_PIN_TEXT_ID, GUI_ERASE_USER_TEXT_ID, GUI_BACK_TEXT_ID};
// Currently selected item
uint16_t gui_operations_menu_selected_item;


/*! \fn     gui_operations_menu_reset_state(void)
*   \brief  Reset operations menu state
*/
void gui_operations_menu_reset_state(void)
{
    gui_operations_menu_selected_item = 1;
}

/*! \fn     gui_operations_menu_event_render(wheel_action_ret_te wheel_action)
*   \brief  Render GUI depending on event received
*   \param  wheel_action    Wheel action received
*   \return TRUE if screen rendering is required
*/
BOOL gui_operations_menu_event_render(wheel_action_ret_te wheel_action)
{
    if (wheel_action == WHEEL_ACTION_NONE)
    {
        gui_carousel_render(sizeof(operations_menu_pic_ids)/sizeof(operations_menu_pic_ids[0]), operations_menu_pic_ids, operations_menu_text_ids, gui_operations_menu_selected_item, 0);
    }
    else if (wheel_action == WHEEL_ACTION_UP)
    {
        if (gui_operations_menu_selected_item != 0)
        {
            gui_carousel_render_animation(sizeof(operations_menu_pic_ids)/sizeof(operations_menu_pic_ids[0]), operations_menu_pic_ids, operations_menu_text_ids, gui_operations_menu_selected_item, TRUE);
            gui_operations_menu_selected_item--;
            gui_carousel_render(sizeof(operations_menu_pic_ids)/sizeof(operations_menu_pic_ids[0]), operations_menu_pic_ids, operations_menu_text_ids, gui_operations_menu_selected_item, 0);
        }
    }
    else if (wheel_action == WHEEL_ACTION_DOWN)
    {        
        if (gui_operations_menu_selected_item != ((sizeof(operations_menu_pic_ids)/sizeof(operations_menu_pic_ids[0]))-1))
        {
            gui_carousel_render_animation(sizeof(operations_menu_pic_ids)/sizeof(operations_menu_pic_ids[0]), operations_menu_pic_ids, operations_menu_text_ids, gui_operations_menu_selected_item, FALSE);
            gui_operations_menu_selected_item++;
            gui_carousel_render(sizeof(operations_menu_pic_ids)/sizeof(operations_menu_pic_ids[0]), operations_menu_pic_ids, operations_menu_text_ids, gui_operations_menu_selected_item, 0);
        }
    }
    else if (wheel_action == WHEEL_ACTION_SHORT_CLICK)
    {
        /* Get selected icon */
        uint16_t selected_icon = operations_menu_pic_ids[gui_operations_menu_selected_item];
        
        /* Switch on the selected icon ID */
        switch (selected_icon)
        {
            case GUI_BACK_ICON_ID:   gui_dispatcher_set_current_screen(GUI_SCREEN_MAIN_MENU); return TRUE;
            default: break;
        }
    }
    
    
    return FALSE;
}