/*!  \file     gui_menu.c
*    \brief    Standardized code to handle our menus
*    Created:  17/11/2018
*    Author:   Mathieu Stephan
*/
#include "smartcard_highlevel.h"
#include "logic_smartcard.h"
#include "gui_dispatcher.h"
#include "comms_aux_mcu.h"
#include "logic_aux_mcu.h"
#include "gui_carousel.h"
#include "driver_timer.h"
#include "gui_prompts.h"
#include "logic_user.h"
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
const uint16_t operations_menu_pic_ids[] = {GUI_CLONE_ICON_ID, GUI_CHANGE_PIN_ICON_ID, GUI_ERASE_USER_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t operations_menu_text_ids[] = {CLONE_TEXT_ID, CHANGE_PIN_TEXT_ID, ERASE_USER_TEXT_ID, BACK_TEXT_ID};
/* Settings Menu */
const uint16_t operations_settings_pic_ids[] = {GUI_KEYB_LAYOUT_CHANGE_ICON_ID, GUI_CRED_PROMPT_CHANGE_ICON_ID, GUI_PWD_DISP_CHANGE_ICON_ID, GUI_WHEEL_ROT_FLIP_ICON_ID, GUI_LANGUAGE_SWITCH_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t operations_settings_text_ids[] = {KEYB_LAYOUT_CHANGE_TEXT_ID, CRED_PROMPT_CHANGE_TEXT_ID, PWD_DISP_CHANGE_TEXT_ID, WHEEL_ROT_FLIP_TEXT_ID, LANGUAGE_SWITCH_TEXT_ID, BACK_TEXT_ID};
/* Array of pointers to the menus pics & texts */
const uint16_t* gui_menu_menus_pics_ids[NB_MENUS] = {simple_menu_pic_ids, bluetooth_off_menu_pic_ids, operations_menu_pic_ids, operations_settings_pic_ids};
const uint16_t* gui_menu_menus_text_ids[NB_MENUS] = {simple_menu_text_ids, bluetooth_off_menu_text_ids, operations_menu_text_ids, operations_settings_text_ids};
/* Number of menu items */
uint16_t gui_menu_menus_nb_items[NB_MENUS] = {ARRAY_SIZE(simple_menu_pic_ids), ARRAY_SIZE(bluetooth_off_menu_pic_ids), ARRAY_SIZE(operations_menu_text_ids), ARRAY_SIZE(operations_settings_pic_ids)};
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
    }
    else
    {
        gui_menu_menus_pics_ids[MAIN_MENU] = simple_menu_pic_ids;
        gui_menu_menus_text_ids[MAIN_MENU] = simple_menu_text_ids;
        gui_menu_menus_nb_items[MAIN_MENU] = ARRAY_SIZE(simple_menu_pic_ids);
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
            /* Main menu, long click to switch between simple / advanced menu */            
            if ((logic_user_get_user_security_flags() & USER_SEC_FLG_ADVANCED_MENU) != 0)
            {
                logic_user_clear_user_security_flag(USER_SEC_FLG_ADVANCED_MENU);
                gui_prompts_display_information_on_screen_and_wait(SIMPLE_MENU_ENABLED_TEXT_ID, DISP_MSG_INFO);
            }                
            else
            {
                logic_user_set_user_security_flag(USER_SEC_FLG_ADVANCED_MENU);
                gui_prompts_display_information_on_screen_and_wait(ADVANCED_MENU_ENABLED_TEXT_ID, DISP_MSG_INFO);
            }
            
            /* Update menu */
            gui_menu_update_menus();
            gui_menu_reset_selected_items(TRUE);
            
            /* Re-render menu */
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
                if (nodemgmt_get_starting_parent_addr() != NODE_ADDR_NULL)
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
            case GUI_OPR_ICON_ID:           gui_dispatcher_set_current_screen(GUI_SCREEN_OPERATIONS, FALSE, GUI_INTO_MENU_TRANSITION); return TRUE;
            case GUI_SETTINGS_ICON_ID:      gui_dispatcher_set_current_screen(GUI_SCREEN_SETTINGS, FALSE, GUI_INTO_MENU_TRANSITION); return TRUE;
            case GUI_BT_ICON_ID:            gui_dispatcher_set_current_screen(GUI_SCREEN_BT, FALSE, GUI_INTO_MENU_TRANSITION); return TRUE;
            
            /* Bluetooth Menu */
            case GUI_BT_ENABLE_ICON_ID:
            {
                uint16_t temp_uint16;
                aux_mcu_message_t* temp_rx_message;
                ret_type_te func_return = RETURN_NOK;
                uint16_t gui_dispatcher_current_idle_anim_frame_id = 0;
                
                /* Send message to the Aux MCU to enable bluetooth, don't wait for it to be enabled */
                logic_aux_mcu_enable_ble(FALSE);
                
                /* Display information message */
                gui_prompts_display_information_on_screen(ENABLING_BLUETOOTH_TEXT_ID, DISP_MSG_INFO);
                
                /* Wait for message to be sent before going to the logic below */
                comms_aux_mcu_wait_for_message_sent();
                
                /* Wait for BT activation while displaying an animation */
                while (func_return != RETURN_OK)
                {
                    func_return = comms_aux_mcu_active_wait(&temp_rx_message, FALSE, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, TRUE);
                    
                    /* Idle animation */
                    if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
                    {
                        /* Display new animation frame bitmap, rearm timer with provided value */
                        gui_prompts_display_information_on_string_single_anim_frame(&gui_dispatcher_current_idle_anim_frame_id, &temp_uint16, DISP_MSG_INFO);
                        timer_start_timer(TIMER_ANIMATIONS, temp_uint16);
                    }
                }
                
                /* Re-arm communication system */
                comms_aux_arm_rx_and_clear_no_comms();
                
                /* Display information message */
                gui_prompts_display_information_on_screen_and_wait(BLUETOOTH_ENABLED_TEXT_ID, DISP_MSG_INFO); 
                
                /* Set BLE enabled bool */
                logic_aux_mcu_set_ble_enabled_bool(TRUE);
                
                /* Refresh menus */
                gui_menu_update_menus();
                
                return TRUE;       
            }
            
            case GUI_BT_DISABLE_ICON_ID:
            {                
                /* Send message to the Aux MCU to enable bluetooth, don't wait for it to be enabled */
                logic_aux_mcu_disable_ble(TRUE);
                
                /* Display information message */
                gui_prompts_display_information_on_screen_and_wait(BLUETOOTH_DISABLED_TEXT_ID, DISP_MSG_INFO);
                
                /* Set BLE enabled bool */
                logic_aux_mcu_set_ble_enabled_bool(FALSE);
                
                /* Refresh menus */
                gui_menu_update_menus();
                
                return TRUE;       
            }
            
            /* Operations menu */
            case GUI_CLONE_ICON_ID:
            {
                /* User wants to clone his smartcard */
                new_pinreturn_type_te temp_rettype;
                volatile uint16_t pin_code;
                
                /* Reauth user */
                if (logic_smartcard_remove_card_and_reauth_user() == RETURN_OK)
                {
                    /* Ask for new pin */
                    temp_rettype = logic_smartcard_ask_for_new_pin(&pin_code, NEW_CARD_PIN_TEXT_ID);
                    if (temp_rettype == RETURN_NEW_PIN_OK)
                    {
                        // Start the cloning process
                        if (logic_smartcard_clone_card(&pin_code) == RETURN_OK)
                        {
                            // Well it worked....
                        }
                        else
                        {
                            gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_INVALID, TRUE, GUI_OUTOF_MENU_TRANSITION);
                            gui_prompts_display_information_on_screen_and_wait(CARD_NOT_BLANK_TEXT_ID, DISP_MSG_WARNING);
                        }
                        pin_code = 0x0000;
                    }
                    else if (temp_rettype == RETURN_NEW_PIN_DIFF)
                    {
                        gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_LCK, TRUE, GUI_OUTOF_MENU_TRANSITION);
                        gui_prompts_display_information_on_screen_and_wait(PIN_DIFFERENT_TEXT_ID, DISP_MSG_WARNING);
                        logic_smartcard_handle_removed();
                    }
                    else
                    {
                        return TRUE;
                    }
                }
                else
                {
                    gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_LCK, TRUE, GUI_OUTOF_MENU_TRANSITION);
                }

                return TRUE;
            }

            case GUI_CHANGE_PIN_ICON_ID:
            {
                /* User wants to change PIN */
                if (logic_smartcard_remove_card_and_reauth_user() == RETURN_OK)
                {
                    /* User approved his pin, ask his new one */
                    volatile uint16_t pin_code;
                    
                    if (logic_smartcard_ask_for_new_pin(&pin_code, ENTER_NEW_PIN_TEXT_ID) == RETURN_NEW_PIN_OK)
                    {
                        // User successfully entered a new pin
                        smartcard_highlevel_write_security_code(&pin_code);

                        /* Inform user */
                        gui_prompts_display_information_on_screen_and_wait(PIN_CHANGED_TEXT_ID, DISP_MSG_INFO);
                    }
                    else
                    {
                        /* Inform user */
                        gui_prompts_display_information_on_screen_and_wait(PIN_NOT_CHANGED_TEXT_ID, DISP_MSG_WARNING);
                    }
                    pin_code = 0x0000;
                }
                else
                {
                    gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_LCK, TRUE, GUI_OUTOF_MENU_TRANSITION);
                }
                return TRUE;
            }

            case GUI_ERASE_USER_ICON_ID:
            {
                /* Delete user profile */
                if ((gui_prompts_ask_for_one_line_confirmation(DELETE_USER_TEXT_ID, FALSE) == MINI_INPUT_RET_YES) && (logic_smartcard_remove_card_and_reauth_user() == RETURN_OK) && (gui_prompts_ask_for_one_line_confirmation(DELETE_USER_CONF_TEXT_ID, FALSE) == MINI_INPUT_RET_YES))
                {
                    uint8_t current_user_id = logic_user_get_current_user_id();
                    gui_prompts_display_information_on_screen(PROCESSING_TEXT_ID, DISP_MSG_INFO);
                    
                    /* Delete user profile and credentials from external DB flash */
                    nodemgmt_delete_current_user_from_flash();
                    
                    /* Ask if user wants to erase the card */
                    if (gui_prompts_ask_for_one_line_confirmation(ERASE_CARD_TEXT_ID, FALSE) == MINI_INPUT_RET_YES)
                    {
                        /* Erase smartcard */
                        gui_prompts_display_information_on_screen(PROCESSING_TEXT_ID, DISP_MSG_INFO);
                        smartcard_highlevel_erase_smartcard();
                        
                        /* Erase other smartcards */
                        while (gui_prompts_ask_for_one_line_confirmation(ERASE_OTHER_CARD_TEXT_ID, FALSE) == MINI_INPUT_RET_YES)
                        {
                            // Ask the user to insert other smartcards
                            gui_prompts_display_information_on_screen_and_wait(REMOVE_CARD_TEXT_ID, DISP_MSG_ACTION);
                            
                            // Wait for the user to remove and enter another smartcard
                            while (smartcard_lowlevel_is_card_plugged() != RETURN_JRELEASED);
                            
                            // Ask the user to insert other smartcards
                            gui_prompts_display_information_on_screen_and_wait(INSERT_OTHER_CARD_TEXT_ID, DISP_MSG_ACTION);
                            
                            // Wait for the user to insert a new smart card
                            while (smartcard_lowlevel_is_card_plugged() != RETURN_JDETECT);
                            gui_prompts_display_information_on_screen(PROCESSING_TEXT_ID, DISP_MSG_INFO);
                            
                            // Check the card type & ask user to enter his pin, check that the new user id loaded by validCardDetectedFunction is still the same
                            if ((smartcard_highlevel_card_detected_routine() == RETURN_MOOLTIPASS_USER) && (logic_smartcard_valid_card_unlock(FALSE, TRUE) == RETURN_VCARD_OK) && (current_user_id == logic_user_get_current_user_id()))
                            {
                                smartcard_highlevel_erase_smartcard();
                            }
                        }
                    }
                    
                    /* Delete LUT entry in our internal flash */
                    gui_prompts_display_information_on_screen(PROCESSING_TEXT_ID, DISP_MSG_INFO);
                    custom_fs_detele_user_cpz_lut_entry(current_user_id);
                    
                    /* Go to invalid screen */
                    gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_INVALID, TRUE, GUI_OUTOF_MENU_TRANSITION);
                }
                else
                {
                    gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_LCK, TRUE, GUI_OUTOF_MENU_TRANSITION);                    
                }
                
                /* We're done! */
                logic_smartcard_handle_removed();
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