/*!  \file     gui_bluetooth_menu.c
*    \brief    GUI for the operations menu
*    Created:  19/11/2018
*    Author:   Mathieu Stephan
*/
#include "gui_bluetooth_menu.h"
#include "gui_dispatcher.h"
#include "gui_carousel.h"
#include "defines.h"
// Menu layout
const uint16_t bluetooth_off_menu_pic_ids[] = {GUI_BT_ENABLE_ICON_ID, GUI_BT_UNPAIR_ICON_ID, GUI_NEW_PAIR_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t bluetooth_on_menu_pic_ids[] = {GUI_BT_DISABLE_ICON_ID, GUI_BT_UNPAIR_ICON_ID, GUI_NEW_PAIR_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t bluetooth_off_menu_text_ids[] = {GUI_BT_ENABLE_TEXT_ID, GUI_BT_UNPAIR_DEV_TEXT_ID, GUI_NEW_PAIR_TEXT_ID, GUI_BACK_TEXT_ID};
const uint16_t bluetooth_on_menu_text_ids[] = {GUI_BT_DISABLE_TEXT_ID, GUI_BT_UNPAIR_DEV_TEXT_ID, GUI_NEW_PAIR_TEXT_ID, GUI_BACK_TEXT_ID};
// Currently selected item
uint16_t gui_bluetooth_menu_selected_item = 0;

// TODO: use logic
#define BT_ENABLED_BOOL   FALSE


/*! \fn     gui_bluetooth_menu_event_render(wheel_action_ret_te wheel_action)
*   \brief  Render GUI depending on event received
*   \param  wheel_action    Wheel action received
*   \return TRUE if screen rendering is required
*/
BOOL gui_bluetooth_menu_event_render(wheel_action_ret_te wheel_action)
{
    if (wheel_action == WHEEL_ACTION_NONE)
    {
        if (BT_ENABLED_BOOL)
        {
            gui_carousel_render(sizeof(bluetooth_on_menu_pic_ids)/sizeof(bluetooth_on_menu_pic_ids[0]), bluetooth_on_menu_pic_ids, bluetooth_on_menu_text_ids, gui_bluetooth_menu_selected_item, 0);
        }
        else
        {
            gui_carousel_render(sizeof(bluetooth_off_menu_pic_ids)/sizeof(bluetooth_off_menu_pic_ids[0]), bluetooth_off_menu_pic_ids, bluetooth_off_menu_text_ids, gui_bluetooth_menu_selected_item, 0);
        }
    }
    else if (wheel_action == WHEEL_ACTION_UP)
    {
        if (gui_bluetooth_menu_selected_item != 0)
        {
            if (BT_ENABLED_BOOL)
            {
                gui_carousel_render_animation(sizeof(bluetooth_on_menu_pic_ids)/sizeof(bluetooth_on_menu_pic_ids[0]), bluetooth_on_menu_pic_ids, bluetooth_on_menu_text_ids, gui_bluetooth_menu_selected_item, TRUE);
                gui_bluetooth_menu_selected_item--;
                gui_carousel_render(sizeof(bluetooth_on_menu_pic_ids)/sizeof(bluetooth_on_menu_pic_ids[0]), bluetooth_on_menu_pic_ids, bluetooth_on_menu_text_ids, gui_bluetooth_menu_selected_item, 0);
            }
            else
            {
                gui_carousel_render_animation(sizeof(bluetooth_off_menu_pic_ids)/sizeof(bluetooth_off_menu_pic_ids[0]), bluetooth_off_menu_pic_ids, bluetooth_off_menu_text_ids, gui_bluetooth_menu_selected_item, TRUE);
                gui_bluetooth_menu_selected_item--;
                gui_carousel_render(sizeof(bluetooth_off_menu_pic_ids)/sizeof(bluetooth_off_menu_pic_ids[0]), bluetooth_off_menu_pic_ids, bluetooth_off_menu_text_ids, gui_bluetooth_menu_selected_item, 0);
            }
        }
    }
    else if (wheel_action == WHEEL_ACTION_DOWN)
    {        
        if (gui_bluetooth_menu_selected_item != ((sizeof(bluetooth_off_menu_pic_ids)/sizeof(bluetooth_off_menu_pic_ids[0]))-1))
        {
            if (BT_ENABLED_BOOL)
            {
                gui_carousel_render_animation(sizeof(bluetooth_on_menu_pic_ids)/sizeof(bluetooth_on_menu_pic_ids[0]), bluetooth_on_menu_pic_ids, bluetooth_on_menu_text_ids, gui_bluetooth_menu_selected_item, FALSE);
                gui_bluetooth_menu_selected_item++;
                gui_carousel_render(sizeof(bluetooth_on_menu_pic_ids)/sizeof(bluetooth_on_menu_pic_ids[0]), bluetooth_on_menu_pic_ids, bluetooth_on_menu_text_ids, gui_bluetooth_menu_selected_item, 0);
            }
            else
            {
                gui_carousel_render_animation(sizeof(bluetooth_off_menu_pic_ids)/sizeof(bluetooth_off_menu_pic_ids[0]), bluetooth_off_menu_pic_ids, bluetooth_off_menu_text_ids, gui_bluetooth_menu_selected_item, FALSE);
                gui_bluetooth_menu_selected_item++;
                gui_carousel_render(sizeof(bluetooth_off_menu_pic_ids)/sizeof(bluetooth_off_menu_pic_ids[0]), bluetooth_off_menu_pic_ids, bluetooth_off_menu_text_ids, gui_bluetooth_menu_selected_item, 0);
            }
        }
    }
    else if (wheel_action == WHEEL_ACTION_SHORT_CLICK)
    {
        /* Get selected icon */
        uint16_t selected_icon = bluetooth_off_menu_pic_ids[gui_bluetooth_menu_selected_item];
        if (BT_ENABLED_BOOL)
        {
            selected_icon = bluetooth_on_menu_pic_ids[gui_bluetooth_menu_selected_item];
        }
        
        /* Switch on the selected icon ID */
        switch (selected_icon)
        {
            case GUI_BACK_ICON_ID:   
            {
                gui_dispatcher_set_current_screen(GUI_SCREEN_MAIN_MENU, FALSE, GUI_OUTOF_MENU_TRANSITION);
                gui_bluetooth_menu_selected_item = 0;
                return TRUE;
            }                
            default: break;
        }
    }
    
    return FALSE;
}