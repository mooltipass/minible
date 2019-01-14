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
    /* How many elements our menu has */
    uint16_t nb_menu_elements = sizeof(simple_menu_pic_ids)/sizeof(simple_menu_pic_ids[0]);
    if (ADVANCED_MODE)
    {
        nb_menu_elements = sizeof(advanced_menu_pic_ids)/sizeof(advanced_menu_pic_ids[0]);
    }
    
    if (wheel_action == WHEEL_ACTION_NONE)
    {
        if (ADVANCED_MODE)
        {
            gui_carousel_render(nb_menu_elements, advanced_menu_pic_ids, advanced_menu_text_ids, gui_main_menu_selected_item, 0);
        } 
        else
        {
            gui_carousel_render(nb_menu_elements, simple_menu_pic_ids, simple_menu_text_ids, gui_main_menu_selected_item, 0);
        }
    } 
    else if (wheel_action == WHEEL_ACTION_UP)
    {
        if (ADVANCED_MODE)
        {
            gui_carousel_render_animation(nb_menu_elements, advanced_menu_pic_ids, advanced_menu_text_ids, gui_main_menu_selected_item, TRUE);
            if (gui_main_menu_selected_item-- == 0)
            {
                gui_main_menu_selected_item = nb_menu_elements-1;
            }
            gui_carousel_render(nb_menu_elements, advanced_menu_pic_ids, advanced_menu_text_ids, gui_main_menu_selected_item, 0);
        }
        else
        {
            gui_carousel_render_animation(nb_menu_elements, simple_menu_pic_ids, simple_menu_text_ids, gui_main_menu_selected_item, TRUE);
            if (gui_main_menu_selected_item-- == 0)
            {
                gui_main_menu_selected_item = nb_menu_elements-1;
            }
            gui_carousel_render(nb_menu_elements, simple_menu_pic_ids, simple_menu_text_ids, gui_main_menu_selected_item, 0);
        }
    }
    else if (wheel_action == WHEEL_ACTION_DOWN)
    {        
        if (ADVANCED_MODE)
        {
            gui_carousel_render_animation(nb_menu_elements, advanced_menu_pic_ids, advanced_menu_text_ids, gui_main_menu_selected_item, FALSE);
            if (++gui_main_menu_selected_item == nb_menu_elements)
            {
                gui_main_menu_selected_item = 0;
            }
            gui_carousel_render(nb_menu_elements, advanced_menu_pic_ids, advanced_menu_text_ids, gui_main_menu_selected_item, 0);
        }
        else
        {
            gui_carousel_render_animation(nb_menu_elements, simple_menu_pic_ids, simple_menu_text_ids, gui_main_menu_selected_item, FALSE);
            if (++gui_main_menu_selected_item == nb_menu_elements)
            {
                gui_main_menu_selected_item = 0;
            }
            gui_carousel_render(nb_menu_elements, simple_menu_pic_ids, simple_menu_text_ids, gui_main_menu_selected_item, 0);
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