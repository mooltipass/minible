/*!  \file     gui_menu.c
*    \brief    Standardized code to handle our menus
*    Created:  17/11/2018
*    Author:   Mathieu Stephan
*/
#include "gui_dispatcher.h"
#include "gui_carousel.h"
#include "gui_prompts.h"
#include "logic_user.h"
#include "nodemgmt.h"
#include "gui_menu.h"

/* Main Menu */
const uint16_t simple_menu_pic_ids[] = {GUI_BT_ICON_ID, GUI_FAV_ICON_ID, GUI_LOGIN_ICON_ID, GUI_LOCK_ICON_ID, GUI_OPR_ICON_ID};
const uint16_t advanced_menu_pic_ids[] = {GUI_BT_ICON_ID, GUI_CAT_ICON_ID, GUI_FAV_ICON_ID, GUI_LOGIN_ICON_ID, GUI_LOCK_ICON_ID, GUI_OPR_ICON_ID, GUI_SETTINGS_ICON_ID};
const uint16_t simple_menu_text_ids[] = {GUI_BT_TEXT_ID, GUI_FAV_TEXT_ID, GUI_LOGIN_TEXT_ID, GUI_LOCK_TEXT_ID, GUI_OPR_TEXT_ID};
const uint16_t advanced_menu_text_ids[] = {GUI_BT_TEXT_ID, GUI_CAT_TEXT_ID, GUI_FAV_TEXT_ID, GUI_LOGIN_TEXT_ID, GUI_LOCK_TEXT_ID, GUI_OPR_TEXT_ID, GUI_SETTINGS_TEXT_ID};
/* Bluetooth Menu */
const uint16_t bluetooth_off_menu_pic_ids[] = {GUI_BT_ENABLE_ICON_ID, GUI_BT_UNPAIR_ICON_ID, GUI_NEW_PAIR_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t bluetooth_on_menu_pic_ids[] = {GUI_BT_DISABLE_ICON_ID, GUI_BT_UNPAIR_ICON_ID, GUI_NEW_PAIR_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t bluetooth_off_menu_text_ids[] = {GUI_BT_ENABLE_TEXT_ID, GUI_BT_UNPAIR_DEV_TEXT_ID, GUI_NEW_PAIR_TEXT_ID, GUI_BACK_TEXT_ID};
const uint16_t bluetooth_on_menu_text_ids[] = {GUI_BT_DISABLE_TEXT_ID, GUI_BT_UNPAIR_DEV_TEXT_ID, GUI_NEW_PAIR_TEXT_ID, GUI_BACK_TEXT_ID};
/* Operations Menu */
const uint16_t operations_menu_pic_ids[] = {GUI_CLONE_ICON_ID, GUI_CHANGE_PIN_ICON_ID, GUI_ERASE_USER_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t operations_menu_text_ids[] = {GUI_CLONE_TEXT_ID, GUI_CHANGE_PIN_TEXT_ID, GUI_ERASE_USER_TEXT_ID, GUI_BACK_TEXT_ID};
/* Settings Menu */
const uint16_t operations_settings_pic_ids[] = {GUI_KEYB_LAYOUT_CHANGE_ICON_ID, GUI_CRED_PROMPT_CHANGE_ICON_ID, GUI_PWD_DISP_CHANGE_ICON_ID, GUI_WHEEL_ROT_FLIP_ICON_ID, GUI_LANGUAGE_SWITCH_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t operations_settings_text_ids[] = {GUI_KEYB_LAYOUT_CHANGE_TEXT_ID, GUI_CRED_PROMPT_CHANGE_TEXT_ID, GUI_PWD_DISP_CHANGE_TEXT_ID, GUI_WHEEL_ROT_FLIP_TEXT_ID, GUI_LANGUAGE_SWITCH_TEXT_ID, GUI_BACK_TEXT_ID};
/* Array of pointers to the menus pics & texts */
const uint16_t* gui_menu_menus_pics_ids[NB_MENUS] = {simple_menu_pic_ids, bluetooth_off_menu_pic_ids, operations_menu_pic_ids, operations_settings_pic_ids};
const uint16_t* gui_menu_menus_text_ids[NB_MENUS] = {simple_menu_text_ids, bluetooth_off_menu_text_ids, operations_menu_text_ids, operations_settings_text_ids};
/* Number of menu items */
uint16_t gui_menu_menus_nb_items[NB_MENUS] = {ARRAY_SIZE(simple_menu_pic_ids), ARRAY_SIZE(bluetooth_off_menu_pic_ids), ARRAY_SIZE(operations_menu_text_ids), ARRAY_SIZE(operations_settings_pic_ids)};
/* Selected items in menus */
uint16_t gui_menu_selected_menu_items[NB_MENUS] = {0,0,0,0};
/* Selected Menu */
uint16_t gui_menu_selected_menu = MAIN_MENU;
#define ADVANCED_MODE   TRUE
#define BT_ENABLED_BOOL   FALSE


/*! \fn     gui_menu_set_selected_menu(menu_te menu)
*   \brief  Set selected menu
*   \param  menu    Currently selected menu
*/
void gui_menu_set_selected_menu(menu_te menu)
{
    gui_menu_selected_menu = menu;
}

/*! \fn     gui_menu_reset_selected_items(BOOL reset_main_menu)
*   \brief  Reset selected items
*   \param  reset_main_menu     Set to TRUE to reset main menu selected item
*/
void gui_menu_reset_selected_items(BOOL reset_main_menu)
{
    if (reset_main_menu != FALSE)
    {
        if (ADVANCED_MODE)
        {
            gui_menu_selected_menu_items[MAIN_MENU] = 3;
        }
        else
        {
            gui_menu_selected_menu_items[MAIN_MENU] = 2;
        }
    }
    gui_menu_selected_menu_items[BT_MENU] = 2;
    gui_menu_selected_menu_items[OPERATION_MENU] = 2;
    gui_menu_selected_menu_items[SETTINGS_MENU] = 3;
}

/*! \fn     gui_menu_update_menus(void)
*   \brief  Update our menus
*/
void gui_menu_update_menus(void)
{
    /* Main menu: advanced mode or not? */
    if (ADVANCED_MODE)
    {
        gui_menu_menus_pics_ids[MAIN_MENU] = advanced_menu_pic_ids;
        gui_menu_menus_text_ids[MAIN_MENU] = advanced_menu_text_ids;
        gui_menu_menus_nb_items[MAIN_MENU] = ARRAY_SIZE(advanced_menu_pic_ids);
    }
    else
    {
        gui_menu_menus_pics_ids[MAIN_MENU] = simple_menu_pic_ids;
        gui_menu_menus_text_ids[MAIN_MENU] = simple_menu_text_ids;
        gui_menu_menus_nb_items[MAIN_MENU] = ARRAY_SIZE(simple_menu_pic_ids);
    }

    /* Bluetooth menu: BT enabled or not? */
    if (BT_ENABLED_BOOL)
    {
        gui_menu_menus_pics_ids[BT_MENU] = bluetooth_on_menu_pic_ids;
        gui_menu_menus_text_ids[BT_MENU] = bluetooth_on_menu_text_ids;
    }
    else
    {
        gui_menu_menus_pics_ids[BT_MENU] = bluetooth_off_menu_pic_ids;
        gui_menu_menus_text_ids[BT_MENU] = bluetooth_off_menu_text_ids;
    }
}

/*! \fn     gui_menu_event_render(wheel_action_ret_te wheel_action)
*   \brief  Render GUI depending on event received
*   \param  wheel_action    Wheel action received
*   \return TRUE if screen rendering is required
*/
BOOL gui_menu_event_render(wheel_action_ret_te wheel_action)
{
    uint16_t* menu_selected_item = &gui_menu_selected_menu_items[gui_menu_selected_menu];
    const uint16_t* menu_texts = gui_menu_menus_text_ids[gui_menu_selected_menu];
    const uint16_t* menu_pics = gui_menu_menus_pics_ids[gui_menu_selected_menu];
    uint16_t menu_nb_items = gui_menu_menus_nb_items[gui_menu_selected_menu];
    
    if (wheel_action == WHEEL_ACTION_NONE)
    {
        gui_carousel_render(menu_nb_items, menu_pics, menu_texts, *menu_selected_item, 0);
    }
    else if (wheel_action == WHEEL_ACTION_UP)
    {
        gui_carousel_render_animation(menu_nb_items, menu_pics, menu_texts, *menu_selected_item, TRUE);
        if ((*menu_selected_item)-- == 0)
        {
            *menu_selected_item = menu_nb_items-1;
        }
        gui_carousel_render(menu_nb_items, menu_pics, menu_texts, *menu_selected_item, 0);
    }
    else if (wheel_action == WHEEL_ACTION_DOWN)
    {
        gui_carousel_render_animation(menu_nb_items, menu_pics, menu_texts, *menu_selected_item, FALSE);
        if (++(*menu_selected_item) == menu_nb_items)
        {
            *menu_selected_item = 0;
        }
        gui_carousel_render(menu_nb_items, menu_pics, menu_texts, *menu_selected_item, 0);
    }
    else if (wheel_action == WHEEL_ACTION_SHORT_CLICK)
    {
        /* Get selected icon */
        uint16_t selected_icon = menu_pics[*menu_selected_item];

        /* Switch on the selected icon ID */
        switch (selected_icon)
        {
            /* Main Menu */
            case GUI_LOGIN_ICON_ID:         
            {
                /* Do we actually have credentials to show? */
                if (nodemgmt_get_starting_parent_addr() != NODE_ADDR_NULL)
                {
                    logic_user_manual_select_login();
                } 
                else
                {
                    gui_prompts_display_information_on_screen_and_wait(NO_CREDS_TEXT_ID, DISP_MSG_INFO);
                }
                gui_dispatcher_get_back_to_current_screen();
            }
            case GUI_OPR_ICON_ID:           gui_dispatcher_set_current_screen(GUI_SCREEN_OPERATIONS, FALSE, GUI_INTO_MENU_TRANSITION); return TRUE;
            case GUI_SETTINGS_ICON_ID:      gui_dispatcher_set_current_screen(GUI_SCREEN_SETTINGS, FALSE, GUI_INTO_MENU_TRANSITION); return TRUE;
            case GUI_BT_ICON_ID:            gui_dispatcher_set_current_screen(GUI_SCREEN_BT, FALSE, GUI_INTO_MENU_TRANSITION); return TRUE;
            
            /* Common to all sub-menus */
            case GUI_BACK_ICON_ID:
            {
                gui_dispatcher_set_current_screen(GUI_SCREEN_MAIN_MENU, FALSE, GUI_OUTOF_MENU_TRANSITION);
                return TRUE;
            }
            
            default: break;
        }
    }


    return FALSE;
}