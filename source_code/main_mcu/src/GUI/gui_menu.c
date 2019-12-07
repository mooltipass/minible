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
/*!  \file     gui_menu.c
*    \brief    Standardized code to handle our menus
*    Created:  17/11/2018
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include "smartcard_highlevel.h"
#include "logic_smartcard.h"
#include "logic_bluetooth.h"
#include "gui_dispatcher.h"
#include "comms_aux_mcu.h"
#include "logic_aux_mcu.h"
#include "gui_carousel.h"
#include "driver_timer.h"
#include "gui_prompts.h"
#include "logic_user.h"
#include "logic_gui.h"
#include "nodemgmt.h"
#include "gui_menu.h"
#include "text_ids.h"
#include "main.h"

/* Main Menu */
const uint16_t simple_menu_pic_ids[] = {GUI_BT_ICON_ID, GUI_FAV_ICON_ID, GUI_LOGIN_ICON_ID, GUI_LOCK_ICON_ID, GUI_OPR_ICON_ID};
const uint16_t advanced_menu_pic_ids[] = {GUI_BT_ICON_ID, GUI_CAT_ICON_ID, GUI_FAV_ICON_ID, GUI_LOGIN_ICON_ID, GUI_LOCK_ICON_ID, GUI_OPR_ICON_ID, GUI_SETTINGS_ICON_ID};
const uint16_t simple_menu_text_ids[] = {BT_TEXT_ID, FAV_TEXT_ID, LOGIN_TEXT_ID, LOCK_TEXT_ID, OPR_TEXT_ID};
const uint16_t advanced_menu_text_ids[] = {BT_TEXT_ID, CAT_TEXT_ID, FAV_TEXT_ID, LOGIN_TEXT_ID, LOCK_TEXT_ID, OPR_TEXT_ID, SETTINGS_TEXT_ID};
/* Bluetooth Menu */
const uint16_t bluetooth_off_menu_pic_ids[] = {GUI_BT_ENABLE_ICON_ID, GUI_BT_UNPAIR_ICON_ID, GUI_NEW_PAIR_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t bluetooth_on_menu_pic_ids[] = {GUI_BT_DISABLE_ICON_ID, GUI_BT_UNPAIR_ICON_ID, GUI_NEW_PAIR_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t bluetooth_off_menu_text_ids[] = {BT_ENABLE_TEXT_ID, BT_UNPAIR_DEV_TEXT_ID, BT_NEW_PAIR_TEXT_ID, BACK_TEXT_ID};
const uint16_t bluetooth_on_menu_text_ids[] = {BT_DISABLE_TEXT_ID, BT_UNPAIR_DEV_TEXT_ID, BT_NEW_PAIR_TEXT_ID, BACK_TEXT_ID};
/* Operations Menu */
const uint16_t operations_menu_pic_ids[] = {GUI_ERASE_USER_ICON_ID, GUI_CHANGE_PIN_ICON_ID, GUI_CLONE_ICON_ID, GUI_SIMPLE_ADV_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t operations_simple_menu_text_ids[] = {ERASE_USER_TEXT_ID, CHANGE_PIN_TEXT_ID, CLONE_TEXT_ID, ENABLE_ADV_MENU_TEXT_ID, BACK_TEXT_ID};
const uint16_t operations_advanced_menu_text_ids[] = {ERASE_USER_TEXT_ID, CHANGE_PIN_TEXT_ID, CLONE_TEXT_ID, ENABLE_SIMPLE_MENU_TEXT_ID, BACK_TEXT_ID};
/* Settings Menu */
const uint16_t operations_settings_pic_ids[] = {GUI_LANGUAGE_SWITCH_ICON_ID, GUI_MMM_STORAGE_CONF_ICON_ID, GUI_PIN_FOR_MMM_ICON_ID, GUI_KEYB_LAYOUT_CHANGE_ICON_ID, GUI_CRED_PROMPT_CHANGE_ICON_ID, GUI_PWD_DISP_CHANGE_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t operations_settings_text_ids[] = {LANGUAGE_SWITCH_TEXT_ID, CONF_FOR_MMM_STORAGE_TEXT_ID, PIN_FOR_MMM_TEXT_ID, KEYB_LAYOUT_CHANGE_TEXT_ID, CRED_PROMPT_CHANGE_TEXT_ID, PWD_DISP_CHANGE_TEXT_ID, BACK_TEXT_ID};
/* Array of pointers to the menus pics & texts */
const uint16_t* gui_menu_menus_pics_ids[NB_MENUS] = {simple_menu_pic_ids, bluetooth_off_menu_pic_ids, operations_menu_pic_ids, operations_settings_pic_ids};
const uint16_t* gui_menu_menus_text_ids[NB_MENUS] = {simple_menu_text_ids, bluetooth_off_menu_text_ids, operations_simple_menu_text_ids, operations_settings_text_ids};
/* Number of menu items */
uint16_t gui_menu_menus_nb_items[NB_MENUS] = {ARRAY_SIZE(simple_menu_pic_ids), ARRAY_SIZE(bluetooth_off_menu_pic_ids), ARRAY_SIZE(operations_simple_menu_text_ids), ARRAY_SIZE(operations_settings_pic_ids)};
/* Selected items in menus */
uint16_t gui_menu_selected_menu_items[NB_MENUS] = {0,0,0,0};
/* Selected Menu */
uint16_t gui_menu_selected_menu = MAIN_MENU;


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
        if ((logic_user_get_user_security_flags() & USER_SEC_FLG_ADVANCED_MENU) != 0)
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
    if ((logic_user_get_user_security_flags() & USER_SEC_FLG_ADVANCED_MENU) != 0)
    {
        gui_menu_menus_pics_ids[MAIN_MENU] = advanced_menu_pic_ids;
        gui_menu_menus_text_ids[MAIN_MENU] = advanced_menu_text_ids;
        gui_menu_menus_nb_items[MAIN_MENU] = ARRAY_SIZE(advanced_menu_pic_ids);
        gui_menu_menus_text_ids[OPERATION_MENU] = operations_advanced_menu_text_ids;       
    }
    else
    {
        gui_menu_menus_pics_ids[MAIN_MENU] = simple_menu_pic_ids;
        gui_menu_menus_text_ids[MAIN_MENU] = simple_menu_text_ids;
        gui_menu_menus_nb_items[MAIN_MENU] = ARRAY_SIZE(simple_menu_pic_ids);
        gui_menu_menus_text_ids[OPERATION_MENU] = operations_simple_menu_text_ids;
    }

    /* Bluetooth menu: BT enabled or not? */
    if (logic_aux_mcu_is_ble_enabled() != FALSE)
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
    else if (wheel_action == WHEEL_ACTION_LONG_CLICK)
    {
        if (gui_menu_selected_menu == MAIN_MENU)
        {
            /* Lock device */
            gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_LCK, TRUE, GUI_OUTOF_MENU_TRANSITION);
            logic_smartcard_handle_removed();
            
            /* Re-render menu */
            return TRUE;
        }
        else
        {
            /* Any other menu, long click to go back */
            gui_dispatcher_set_current_screen(GUI_SCREEN_MAIN_MENU, FALSE, GUI_OUTOF_MENU_TRANSITION);
            return TRUE;
        }
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
                if (nodemgmt_get_starting_parent_addr_for_category() != NODE_ADDR_NULL)
                {
                    logic_user_manual_select_login();
                    #ifdef OLED_INTERNAL_FRAME_BUFFER
                    sh1122_load_transition(&plat_oled_descriptor, OLED_OUT_IN_TRANS);
                    #endif
                } 
                else
                {
                    gui_prompts_display_information_on_screen_and_wait(NO_CREDS_TEXT_ID, DISP_MSG_INFO);
                }
                return TRUE;
            }
            case GUI_LOCK_ICON_ID:
            {
                gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_LCK, TRUE, GUI_OUTOF_MENU_TRANSITION);
                logic_smartcard_handle_removed(); 
                return TRUE;
            }                
            case GUI_CAT_ICON_ID:
            {
                int16_t selected_category = gui_prompts_select_category();
                if (selected_category >= 0)
                {
                    nodemgmt_set_current_category_id(selected_category);
                }
                return TRUE;
            }
            case GUI_FAV_ICON_ID:
            {
                uint16_t chosen_service_addr, chosen_login_addr;
                int16_t chosen_favorite_index = -1;
                BOOL only_password_prompt = FALSE;
                BOOL usb_interface_output = TRUE;
                uint16_t state_machine_index = 0;
                child_node_t temp_cnode;

                while (TRUE)
                {
                    if (state_machine_index == 0)
                    {
                        chosen_favorite_index = gui_prompts_favorite_selection_screen(chosen_favorite_index);

                        /* No login was chosen */
                        if (chosen_favorite_index < 0)
                        {
                            return TRUE;
                        }
                    
                        /* Load address */
                        nodemgmt_read_favorite_for_current_category(chosen_favorite_index, &chosen_service_addr, &chosen_login_addr);
                        
                        /* Next state */
                        state_machine_index++;
                    }
                    else if (state_machine_index == 1)
                    {
                        /* Ask the user permission to enter login / password, check for back action */
                        ret_type_te user_prompt_return = logic_user_ask_for_credentials_keyb_output(chosen_service_addr, chosen_login_addr, only_password_prompt, &usb_interface_output);
                        
                        /* Check for back, and deny of both prompts */
                        if (user_prompt_return == RETURN_BACK)
                        {
                            only_password_prompt = FALSE;
                            state_machine_index--;
                        }
                        else if (user_prompt_return == RETURN_NOK)
                        {
                            /* If user enabled that setting or if we're not connected to anything, ask to display credential */
                            if (((logic_user_get_user_security_flags() & USER_SEC_FLG_PWD_DISPLAY_PROMPT) != 0) || ((logic_bluetooth_get_state() != BT_STATE_CONNECTED) && (logic_aux_mcu_is_usb_enumerated() == FALSE)))
                            {
                                state_machine_index++;
                            }
                            else
                            {
                                return TRUE;
                            }
                        }
                        else
                        {
                            return TRUE;
                        }
                    }
                    else if (state_machine_index == 2)
                    {
                        // Fetch parent node to prepare prompt text
                        _Static_assert(sizeof(child_node_t) >= sizeof(parent_node_t), "Invalid buffer reuse");
                        parent_node_t* temp_pnode_pt = (parent_node_t*)&temp_cnode;
                        nodemgmt_read_parent_node(chosen_service_addr, temp_pnode_pt, TRUE);
                        
                        // Ask the user if he wants to display credentials on screen
                        cust_char_t* display_cred_prompt_text;
                        custom_fs_get_string_from_file(QPROMPT_SNGL_DISP_CRED_TEXT_ID, &display_cred_prompt_text, TRUE);
                        confirmationText_t prompt_object = {.lines[0] = temp_pnode_pt->cred_parent.service, .lines[1] = display_cred_prompt_text};
                        mini_input_yes_no_ret_te display_prompt_return = gui_prompts_ask_for_confirmation(2, &prompt_object, FALSE, TRUE);
                        
                        if (display_prompt_return == MINI_INPUT_RET_BACK)
                        {
                            only_password_prompt = TRUE;
                            
                            if ((logic_bluetooth_get_state() != BT_STATE_CONNECTED) && (logic_aux_mcu_is_usb_enumerated() == FALSE))
                            {
                                state_machine_index = 0;
                            }
                            else
                            {
                                state_machine_index--;
                            }
                        }
                        else if (display_prompt_return == MINI_INPUT_RET_YES)
                        {
                            nodemgmt_read_cred_child_node(chosen_login_addr, (child_cred_node_t*)&temp_cnode);
                            logic_gui_display_login_password((child_cred_node_t*)&temp_cnode);
                            memset(&temp_cnode, 0, sizeof(temp_cnode));
                            return TRUE;
                        }
                        else
                        {
                            return TRUE;
                        }
                    }
                }
            }
            case GUI_OPR_ICON_ID:           gui_dispatcher_set_current_screen(GUI_SCREEN_OPERATIONS, FALSE, GUI_INTO_MENU_TRANSITION); return TRUE;
            case GUI_SETTINGS_ICON_ID:      gui_dispatcher_set_current_screen(GUI_SCREEN_SETTINGS, FALSE, GUI_INTO_MENU_TRANSITION); return TRUE;
            case GUI_BT_ICON_ID:            gui_dispatcher_set_current_screen(GUI_SCREEN_BT, FALSE, GUI_INTO_MENU_TRANSITION); return TRUE;
            
            /* Bluetooth Menu */
            case GUI_BT_ENABLE_ICON_ID:
            {
                logic_gui_enable_bluetooth();
                logic_user_set_user_security_flag(USER_SEC_FLG_BLE_ENABLED);
                return TRUE;       
            }            
            case GUI_BT_DISABLE_ICON_ID:
            {                
                logic_gui_disable_bluetooth();
                logic_user_clear_user_security_flag(USER_SEC_FLG_BLE_ENABLED);
                return TRUE;       
            }
            
            /* Operations menu */
            case GUI_SIMPLE_ADV_ICON_ID:
            {
                /* Main menu, long click to switch between simple / advanced menu */
                if ((logic_user_get_user_security_flags() & USER_SEC_FLG_ADVANCED_MENU) != 0)
                {
                    logic_user_clear_user_security_flag(0xFFFF & (~USER_SEC_FLG_BLE_ENABLED));
                    gui_prompts_display_information_on_screen_and_wait(SIMPLE_MENU_ENABLED_TEXT_ID, DISP_MSG_INFO);
                }
                else
                {
                    logic_user_set_user_security_flag(0xFFFF & (~USER_SEC_FLG_BLE_ENABLED));
                    gui_prompts_display_information_on_screen_and_wait(ADVANCED_MENU_ENABLED_TEXT_ID, DISP_MSG_INFO);
                }
                
                /* Update menu */
                gui_menu_update_menus();
                gui_dispatcher_set_current_screen(GUI_SCREEN_MAIN_MENU, TRUE, GUI_OUTOF_MENU_TRANSITION);
                
                return TRUE;
            }
            case GUI_CLONE_ICON_ID:
            {
                logic_gui_clone_card();
                return TRUE;
            }
            case GUI_CHANGE_PIN_ICON_ID:
            {
                logic_gui_change_pin();
                return TRUE;
            }
            case GUI_ERASE_USER_ICON_ID:
            {
                logic_gui_erase_user();
                return TRUE;
            }
            
            /* Settings menu */
            case GUI_LANGUAGE_SWITCH_ICON_ID:
            {
                gui_prompts_select_language_or_keyboard_layout(FALSE, FALSE, FALSE, FALSE);
                logic_user_set_language(custom_fs_get_current_language_id());
                return TRUE;
            }
            case GUI_MMM_STORAGE_CONF_ICON_ID:
            {
                mini_input_yes_no_ret_te user_input = gui_prompts_ask_for_one_line_confirmation(QCONF_FOR_MMM_STORAGE_TEXT_ID, FALSE, FALSE, (logic_user_get_user_security_flags() & USER_SEC_FLG_CRED_SAVE_PROMPT_MMM) != 0);
                if (user_input == MINI_INPUT_RET_YES)
                {
                    logic_user_set_user_security_flag(USER_SEC_FLG_CRED_SAVE_PROMPT_MMM);
                } 
                else if (user_input == MINI_INPUT_RET_NO)
                {
                    logic_user_clear_user_security_flag(USER_SEC_FLG_CRED_SAVE_PROMPT_MMM);
                }
                return TRUE;
            }
            case GUI_PIN_FOR_MMM_ICON_ID:
            {
                mini_input_yes_no_ret_te user_input = gui_prompts_ask_for_one_line_confirmation(QPIN_FOR_MMM_TEXT_ID, FALSE, FALSE, (logic_user_get_user_security_flags() & USER_SEC_FLG_PIN_FOR_MMM) != 0);
                if (user_input == MINI_INPUT_RET_YES)
                {
                    logic_user_set_user_security_flag(USER_SEC_FLG_PIN_FOR_MMM);
                }
                else if (user_input == MINI_INPUT_RET_NO)
                {
                    logic_user_clear_user_security_flag(USER_SEC_FLG_PIN_FOR_MMM);
                }
                return TRUE;
            }
            case GUI_KEYB_LAYOUT_CHANGE_ICON_ID:
            {
                /* Ask for keyboard type selection */
                mini_input_yes_no_ret_te select_inteface_prompt_return = gui_prompts_ask_for_one_line_confirmation(SELECT_KEYBOARD_TYPE_TEXT_ID, FALSE, TRUE, TRUE);
                BOOL usb_interface_selected = FALSE;
                
                if (select_inteface_prompt_return == MINI_INPUT_RET_BACK)
                {
                    return TRUE;
                }
                else if (select_inteface_prompt_return == MINI_INPUT_RET_YES)
                {
                    usb_interface_selected = TRUE;
                }
                else if (select_inteface_prompt_return == MINI_INPUT_RET_NO)
                {
                    usb_interface_selected = FALSE;
                }
                else
                {
                    return TRUE;
                }
                
                /* Keyboard layout selection */
                gui_prompts_select_language_or_keyboard_layout(TRUE, FALSE, FALSE, usb_interface_selected);
                
                /* Keyboard layout storage */
                logic_user_set_layout_id(custom_fs_get_current_layout_id(usb_interface_selected), usb_interface_selected);
                return TRUE;
            }
            case GUI_CRED_PROMPT_CHANGE_ICON_ID:
            {
                mini_input_yes_no_ret_te user_input = gui_prompts_ask_for_one_line_confirmation(QPROMPT_FOR_LOGIN_TEXT_ID, FALSE, FALSE, (logic_user_get_user_security_flags() & USER_SEC_FLG_LOGIN_CONF) != 0);
                if (user_input == MINI_INPUT_RET_YES)
                {
                    logic_user_set_user_security_flag(USER_SEC_FLG_LOGIN_CONF);
                }
                else if (user_input == MINI_INPUT_RET_NO)
                {
                    logic_user_clear_user_security_flag(USER_SEC_FLG_LOGIN_CONF);
                }
                return TRUE;                
            }
            case GUI_PWD_DISP_CHANGE_ICON_ID:
            {
                mini_input_yes_no_ret_te user_input = gui_prompts_ask_for_one_line_confirmation(QPROMPT_PWD_DISPLAY_TEXT_ID, FALSE, FALSE, (logic_user_get_user_security_flags() & USER_SEC_FLG_PWD_DISPLAY_PROMPT) != 0);
                if (user_input == MINI_INPUT_RET_YES)
                {
                    logic_user_set_user_security_flag(USER_SEC_FLG_PWD_DISPLAY_PROMPT);
                }
                else if (user_input == MINI_INPUT_RET_NO)
                {
                    logic_user_clear_user_security_flag(USER_SEC_FLG_PWD_DISPLAY_PROMPT);
                }
                return TRUE;
            }
            
            
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