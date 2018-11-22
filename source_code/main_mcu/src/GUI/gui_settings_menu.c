/*!  \file     gui_settings_menu.c
*    \brief    GUI for the settings menu
*    Created:  22/11/2018
*    Author:   Mathieu Stephan
*/
#include "gui_settings_menu.h"
#include "gui_dispatcher.h"
#include "gui_carousel.h"
#include "defines.h"
// Menu layout
const uint16_t operations_settings_pic_ids[] = {GUI_KEYB_LAYOUT_CHANGE_ICON_ID, GUI_CRED_PROMPT_CHANGE_ICON_ID, GUI_PWD_DISP_CHANGE_ICON_ID, GUI_WHEEL_ROT_FLIP_ICON_ID, GUI_BAT_REFRESH_ICON_ID, GUI_LANGUAGE_SWITCH_ICON_ID};
const uint16_t operations_settings_text_ids[] = {GUI_KEYB_LAYOUT_CHANGE_TEXT_ID, GUI_CRED_PROMPT_CHANGE_TEXT_ID, GUI_PWD_DISP_CHANGE_TEXT_ID, GUI_WHEEL_ROT_FLIP_TEXT_ID, GUI_BAT_REFRESH_TEXT_ID, GUI_LANGUAGE_SWITCH_TEXT_ID};
// Currently selected item
uint16_t gui_settings_menu_selected_item = 0;


/*! \fn     gui_settings_menu_event_render(wheel_action_ret_te wheel_action)
*   \brief  Render GUI depending on event received
*   \param  wheel_action    Wheel action received
*   \return TRUE if screen rendering is required
*/
BOOL gui_settings_menu_event_render(wheel_action_ret_te wheel_action)
{
    if (wheel_action == WHEEL_ACTION_NONE)
    {
        gui_carousel_render(sizeof(operations_settings_pic_ids)/sizeof(operations_settings_pic_ids[0]), operations_settings_pic_ids, operations_settings_text_ids, gui_settings_menu_selected_item, 0);
    }
    else if (wheel_action == WHEEL_ACTION_UP)
    {
        if (gui_settings_menu_selected_item != 0)
        {
            gui_carousel_render_animation(sizeof(operations_settings_pic_ids)/sizeof(operations_settings_pic_ids[0]), operations_settings_pic_ids, operations_settings_text_ids, gui_settings_menu_selected_item, TRUE);
            gui_settings_menu_selected_item--;
            gui_carousel_render(sizeof(operations_settings_pic_ids)/sizeof(operations_settings_pic_ids[0]), operations_settings_pic_ids, operations_settings_text_ids, gui_settings_menu_selected_item, 0);
        }
    }
    else if (wheel_action == WHEEL_ACTION_DOWN)
    {
        if (gui_settings_menu_selected_item != ((sizeof(operations_settings_pic_ids)/sizeof(operations_settings_pic_ids[0]))-1))
        {
            gui_carousel_render_animation(sizeof(operations_settings_pic_ids)/sizeof(operations_settings_pic_ids[0]), operations_settings_pic_ids, operations_settings_text_ids, gui_settings_menu_selected_item, FALSE);
            gui_settings_menu_selected_item++;
            gui_carousel_render(sizeof(operations_settings_pic_ids)/sizeof(operations_settings_pic_ids[0]), operations_settings_pic_ids, operations_settings_text_ids, gui_settings_menu_selected_item, 0);
        }
    }
    else if (wheel_action == WHEEL_ACTION_SHORT_CLICK)
    {
        /* Get selected icon */
        uint16_t selected_icon = operations_settings_pic_ids[gui_settings_menu_selected_item];
        
        /* Switch on the selected icon ID */
        switch (selected_icon)
        {
            case GUI_BACK_ICON_ID:
            {
                gui_dispatcher_set_current_screen(GUI_SCREEN_MAIN_MENU, FALSE);
                gui_settings_menu_selected_item = 0;
                return TRUE;
            }
            default: break;
        }
    }
    
    
    return FALSE;
}