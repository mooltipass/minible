/*!  \file     gui_main_menu.c
*    \brief    GUI for the main menu
*    Created:  17/11/2018
*    Author:   Mathieu Stephan
*/
#include "gui_bluetooth_menu.h"
#include "gui_dispatcher.h"
#include "gui_main_menu.h"
#include "gui_carousel.h"
#include "defines.h"
// Menu layout
const uint16_t simple_menu_pic_ids[] = {GUI_BT_ICON_ID, GUI_FAV_ICON_ID, GUI_LOGIN_ICON_ID, GUI_LOCK_ICON_ID, GUI_OPR_ICON_ID};
const uint16_t advanced_menu_pic_ids[] = {GUI_BT_ICON_ID, GUI_CAT_ICON_ID, GUI_FAV_ICON_ID, GUI_LOGIN_ICON_ID, GUI_LOCK_ICON_ID, GUI_OPR_ICON_ID, GUI_SETTINGS_ICON_ID};
const uint16_t simple_menu_text_ids[] = {GUI_BT_TEXT_ID, GUI_FAV_TEXT_ID, GUI_LOGIN_TEXT_ID, GUI_LOCK_TEXT_ID, GUI_OPR_TEXT_ID};
const uint16_t advanced_menu_text_ids[] = {GUI_BT_TEXT_ID, GUI_CAT_TEXT_ID, GUI_FAV_TEXT_ID, GUI_LOGIN_TEXT_ID, GUI_LOCK_TEXT_ID, GUI_OPR_TEXT_ID, GUI_SETTINGS_TEXT_ID};
// Currently selected item
uint16_t gui_main_menu_selected_item;

// TODO: use logic
#define ADVANCED_MODE   TRUE

/*! \fn     gui_main_menu_reset_state(void)
*   \brief  Reset main menu state
*/
void gui_main_menu_reset_state(void)
{
    if (ADVANCED_MODE)
    {
        gui_main_menu_selected_item = 3;
    } 
    else
    {
        gui_main_menu_selected_item = 2;
    }
}

/*! \fn     gui_main_menu_event_render(wheel_action_ret_te wheel_action)
*   \brief  Render GUI depending on event received
*   \param  wheel_action    Wheel action received
*   \return TRUE if screen rendering is required
*/
BOOL gui_main_menu_event_render(wheel_action_ret_te wheel_action)
{
    if (wheel_action == WHEEL_ACTION_NONE)
    {
        if (ADVANCED_MODE)
        {
            gui_carousel_render(sizeof(advanced_menu_pic_ids)/sizeof(advanced_menu_pic_ids[0]), advanced_menu_pic_ids, advanced_menu_text_ids, gui_main_menu_selected_item, 0);
        } 
        else
        {
            gui_carousel_render(sizeof(simple_menu_pic_ids)/sizeof(simple_menu_pic_ids[0]), simple_menu_pic_ids, simple_menu_text_ids, gui_main_menu_selected_item, 0);
        }
    } 
    else if (wheel_action == WHEEL_ACTION_UP)
    {
        if (gui_main_menu_selected_item != 0)
        {
            if (ADVANCED_MODE)
            {
                gui_carousel_render_animation(sizeof(advanced_menu_pic_ids)/sizeof(advanced_menu_pic_ids[0]), advanced_menu_pic_ids, advanced_menu_text_ids, gui_main_menu_selected_item, TRUE);
                gui_main_menu_selected_item--;
                gui_carousel_render(sizeof(advanced_menu_pic_ids)/sizeof(advanced_menu_pic_ids[0]), advanced_menu_pic_ids, advanced_menu_text_ids, gui_main_menu_selected_item, 0);
            }
            else
            {
                gui_carousel_render_animation(sizeof(simple_menu_pic_ids)/sizeof(simple_menu_pic_ids[0]), simple_menu_pic_ids, simple_menu_text_ids, gui_main_menu_selected_item, TRUE);
                gui_main_menu_selected_item--;
                gui_carousel_render(sizeof(simple_menu_pic_ids)/sizeof(simple_menu_pic_ids[0]), simple_menu_pic_ids, simple_menu_text_ids, gui_main_menu_selected_item, 0);
            }
        }
    }
    else if (wheel_action == WHEEL_ACTION_DOWN)
    {
        uint16_t last_element = sizeof(simple_menu_pic_ids)/sizeof(simple_menu_pic_ids[0]);
        if (ADVANCED_MODE)
        {
            last_element = sizeof(advanced_menu_pic_ids)/sizeof(advanced_menu_pic_ids[0]);
        }
        
        if (gui_main_menu_selected_item != (last_element-1))
        {
            if (ADVANCED_MODE)
            {
                gui_carousel_render_animation(sizeof(advanced_menu_pic_ids)/sizeof(advanced_menu_pic_ids[0]), advanced_menu_pic_ids, advanced_menu_text_ids, gui_main_menu_selected_item, FALSE);
                gui_main_menu_selected_item++;
                gui_carousel_render(sizeof(advanced_menu_pic_ids)/sizeof(advanced_menu_pic_ids[0]), advanced_menu_pic_ids, advanced_menu_text_ids, gui_main_menu_selected_item, 0);
            }
            else
            {
                gui_carousel_render_animation(sizeof(simple_menu_pic_ids)/sizeof(simple_menu_pic_ids[0]), simple_menu_pic_ids, simple_menu_text_ids, gui_main_menu_selected_item, FALSE);
                gui_main_menu_selected_item++;
                gui_carousel_render(sizeof(simple_menu_pic_ids)/sizeof(simple_menu_pic_ids[0]), simple_menu_pic_ids, simple_menu_text_ids, gui_main_menu_selected_item, 0);
            }
        }
    }    
    else if (wheel_action == WHEEL_ACTION_SHORT_CLICK)
    {
        /* Get selected icon */
        uint16_t selected_icon = simple_menu_pic_ids[gui_main_menu_selected_item];
        if (ADVANCED_MODE)
        {
            selected_icon = advanced_menu_pic_ids[gui_main_menu_selected_item];
        }        
        
        /* Switch on the selected icon ID */
        switch (selected_icon)
        {
            case GUI_OPR_ICON_ID:           gui_dispatcher_set_current_screen(GUI_SCREEN_OPERATIONS, FALSE, GUI_INTO_MENU_TRANSITION); return TRUE;
            case GUI_BT_ICON_ID:            gui_dispatcher_set_current_screen(GUI_SCREEN_BT, FALSE, GUI_INTO_MENU_TRANSITION); return TRUE;
            case GUI_SETTINGS_ICON_ID:      gui_dispatcher_set_current_screen(GUI_SCREEN_SETTINGS, FALSE, GUI_INTO_MENU_TRANSITION); return TRUE;
            default: break;
        }
    }
        
    return FALSE;
}