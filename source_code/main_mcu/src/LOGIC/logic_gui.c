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
/*!  \file     logic_gui.c
*    \brief    General logic for GUI
*    Created:  11/05/2019
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include "smartcard_highlevel.h"
#include "logic_accelerometer.h"
#include "smartcard_lowlevel.h"
#include "logic_encryption.h"
#include "logic_smartcard.h"
#include "gui_dispatcher.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "logic_device.h"
#include "gui_prompts.h"
#include "logic_power.h"
#include "logic_user.h"
#include "logic_gui.h"
#include "nodemgmt.h"
#include "text_ids.h"
#include "gui_menu.h"
#include "defines.h"
#include "inputs.h"
#include "utils.h"
#include "main.h"

/*! \fn     logic_gui_disable_bluetooth(void)
*   \brief  Disable Bluetooth
*/
void logic_gui_disable_bluetooth(void)
{
    if (logic_aux_mcu_is_ble_enabled() != FALSE)
    {
        /* Send message to the Aux MCU to disable bluetooth, wait for the answer */
        logic_aux_mcu_disable_ble(TRUE);
        
        /* Display information message */
        gui_prompts_display_information_on_screen_and_wait(BLUETOOTH_DISABLED_TEXT_ID, DISP_MSG_INFO, FALSE);
        
        /* Set BLE enabled bool */
        logic_aux_mcu_set_ble_enabled_bool(FALSE);
    }
}

/*! \fn     logic_gui_enable_bluetooth(void)
*   \brief  Enable Bluetooth
*/
void logic_gui_enable_bluetooth(void)
{
    if (logic_aux_mcu_is_ble_enabled() == FALSE)
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
            func_return = comms_aux_mcu_active_wait(&temp_rx_message, FALSE, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, TRUE, AUX_MCU_EVENT_BLE_ENABLED);
            
            /* Idle animation */
            if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
            {
                /* Display new animation frame bitmap, rearm timer with provided value */
                gui_prompts_display_information_on_string_single_anim_frame(&gui_dispatcher_current_idle_anim_frame_id, &temp_uint16, DISP_MSG_INFO);
                timer_start_timer(TIMER_ANIMATIONS, temp_uint16);
            }
            
            /* Handle possible power switches */
            logic_power_check_power_switch_and_battery(FALSE);
        }
        
        /* Re-arm communication system */
        comms_aux_arm_rx_and_clear_no_comms();
        
        /* Display information message */
        gui_prompts_display_information_on_screen_and_wait(BLUETOOTH_ENABLED_TEXT_ID, DISP_MSG_INFO, FALSE);
        
        /* Set BLE enabled bool */
        logic_aux_mcu_set_ble_enabled_bool(TRUE);
    }
}

/*! \fn     logic_gui_clone_card(void)
*   \brief  Clone Card
*/
void logic_gui_clone_card(void)
{
    /* User wants to clone his smartcard */
    new_pinreturn_type_te temp_rettype;
    volatile uint16_t pin_code;
    
    /* Reauth user */
    if (logic_smartcard_remove_card_and_reauth_user(TRUE) == RETURN_OK)
    {
        /* Ask for new pin */
        temp_rettype = logic_smartcard_ask_for_new_pin(&pin_code, NEW_CARD_PIN_TEXT_ID);
        if (temp_rettype == RETURN_NEW_PIN_OK)
        {
            // Start the cloning process
            if (logic_smartcard_clone_card(&pin_code) == RETURN_OK)
            {
                /* Well it worked... */
                return;
            }
            else
            {
                /* Card wasn't blank */
                gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_INVALID, TRUE, GUI_OUTOF_MENU_TRANSITION);
                gui_prompts_display_information_on_screen_and_wait(CARD_NOT_BLANK_TEXT_ID, DISP_MSG_WARNING, FALSE);
            }
            pin_code = 0x0000;
        }
        else if (temp_rettype == RETURN_NEW_PIN_DIFF)
        {
            /* Display that the PINs were different */
            gui_prompts_display_information_on_screen_and_wait(PIN_DIFFERENT_TEXT_ID, DISP_MSG_WARNING, FALSE);
        }
        else
        {
            /* User went back */
            return;
        }
    }
    else
    {
        /* Couldn't reauth user */
        gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_LCK, TRUE, GUI_OUTOF_MENU_TRANSITION);
    }
}    

/*! \fn     logic_gui_change_pin(void)
*   \brief  Change card PIN
*/
void logic_gui_change_pin(void)
{
    /* User wants to change PIN */
    if (logic_smartcard_remove_card_and_reauth_user(TRUE) == RETURN_OK)
    {
        /* User approved his pin, ask his new one */
        volatile uint16_t pin_code;
        
        if (logic_smartcard_ask_for_new_pin(&pin_code, ENTER_NEW_PIN_TEXT_ID) == RETURN_NEW_PIN_OK)
        {
            // User successfully entered a new pin
            smartcard_highlevel_write_security_code(&pin_code);

            /* Inform user */
            gui_prompts_display_information_on_screen_and_wait(PIN_CHANGED_TEXT_ID, DISP_MSG_INFO, FALSE);
        }
        else
        {
            /* Inform user */
            gui_prompts_display_information_on_screen_and_wait(PIN_NOT_CHANGED_TEXT_ID, DISP_MSG_WARNING, FALSE);
        }
        pin_code = 0x0000;
    }
    else
    {
        gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_LCK, TRUE, GUI_OUTOF_MENU_TRANSITION);
    }
}

/*! \fn     logic_gui_erase_user(void)
*   \brief  Erase user
*/
void logic_gui_erase_user(void)
{
    /* Device state is going to change... */
    logic_device_set_state_changed();
    
    /* Delete user profile */
    if ((gui_prompts_ask_for_one_line_confirmation(DELETE_USER_TEXT_ID, FALSE, FALSE, TRUE) == MINI_INPUT_RET_YES) && (logic_smartcard_remove_card_and_reauth_user(TRUE) == RETURN_OK) && (gui_prompts_ask_for_one_line_confirmation(DELETE_USER_CONF_TEXT_ID, FALSE, FALSE, TRUE) == MINI_INPUT_RET_YES))
    {
        uint8_t current_user_id = logic_user_get_current_user_id();
        gui_prompts_display_information_on_screen(PROCESSING_TEXT_ID, DISP_MSG_INFO);
        
        /* Delete user profile and credentials from external DB flash */
        nodemgmt_delete_current_user_from_flash();
        
        /* Ask if user wants to erase the card */
        if (gui_prompts_ask_for_one_line_confirmation(ERASE_CARD_TEXT_ID, FALSE, FALSE, TRUE) == MINI_INPUT_RET_YES)
        {
            /* Erase smartcard */
            gui_prompts_display_information_on_screen(PROCESSING_TEXT_ID, DISP_MSG_INFO);
            smartcard_highlevel_erase_smartcard();
            
            /* Erase other smartcards */
            while (gui_prompts_ask_for_one_line_confirmation(ERASE_OTHER_CARD_TEXT_ID, FALSE, FALSE, TRUE) == MINI_INPUT_RET_YES)
            {
                // Ask the user to insert other smartcards
                gui_prompts_display_information_on_screen_and_wait(REMOVE_CARD_TEXT_ID, DISP_MSG_ACTION, FALSE);
                
                // Wait for the user to remove and enter another smartcard
                while (smartcard_lowlevel_is_card_plugged() != RETURN_JRELEASED);
                
                // Ask the user to insert other smartcards
                gui_prompts_display_information_on_screen_and_wait(INSERT_OTHER_CARD_TEXT_ID, DISP_MSG_ACTION, FALSE);
                
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
}

/*! \fn     logic_gui_display_login_password(child_cred_node_t* child_node)
*   \brief  Display login and password on the screen
*   \param  child_node  Pointer to the child node
*/
void logic_gui_display_login_password(child_cred_node_t* child_node)
{
    /* Activity detected */
    logic_device_activity_detected();
    
    /* Clear possible remaining detection */
    inputs_clear_detections();    
    
    /* Clear frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_load_transition(&plat_oled_descriptor, OLED_IN_OUT_TRANS);
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #else
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #endif
    
    /* Set prev gen bool */
    BOOL prev_gen_credential_flag = FALSE;
    if ((child_node->flags & NODEMGMT_PREVGEN_BIT_BITMASK) != 0)
    {
        prev_gen_credential_flag = TRUE;
    }
    
    /* User approved, decrypt password */
    logic_encryption_ctr_decrypt(child_node->password, child_node->ctr, MEMBER_SIZE(child_cred_node_t, password), prev_gen_credential_flag);
    
    /* 0 terminate password */
    child_node->cust_char_password[MEMBER_ARRAY_SIZE(child_cred_node_t, cust_char_password) - 1] = 0;
    
    /* Check for valid password */
    if (child_node->passwordBlankFlag != FALSE)
    {
        child_node->cust_char_password[0] = 0;
    }
    
    /* If old generation password, convert it to unicode */
    if (prev_gen_credential_flag != FALSE)
    {
        _Static_assert(MEMBER_SIZE(child_cred_node_t, password) >= NODEMGMT_OLD_GEN_ASCII_PWD_LENGTH*2 + 2, "Backward compatibility problem");
        utils_ascii_to_unicode(child_node->password, NODEMGMT_OLD_GEN_ASCII_PWD_LENGTH);
    }    
    
    /* Temp vars for our main loop */
    BOOL first_function_run = TRUE;
    int16_t text_anim_x_offset[2];
    BOOL text_anim_going_right[2];
    BOOL redraw_needed = TRUE;
    int16_t displayed_length;
    BOOL scrolling_needed[2];
    
    /* Reset temp vars */
    memset(text_anim_going_right, FALSE, sizeof(text_anim_going_right));
    memset(text_anim_x_offset, 0, sizeof(text_anim_x_offset));
    memset(scrolling_needed, FALSE, sizeof(scrolling_needed));
    
    /* Lines display settings */
    cust_char_t* strings_to_be_displayed[2] = {child_node->login, child_node->cust_char_password};
    uint16_t strings_y_positions[4] = {12, 36};
    
    /* Arm timer for scrolling */
    timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
    
    /* Loop until something has been done */
    while (TRUE)
    {
        /* Deal with ping messages only */
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        
        /* Call accelerometer routine for (among others) RNG stuff */
        logic_accelerometer_routine();
        
        /* User interaction timeout */
        if (timer_has_timer_expired(TIMER_USER_INTERACTION, TRUE) == TIMER_EXPIRED)
        {
            return;
        }
        
        /* Card removed */
        if (smartcard_low_level_is_smc_absent() == RETURN_OK)
        {
            return;
        }
        
        /* Click to exit */
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
        
        /* Scrolling logic */
        if (timer_has_timer_expired(TIMER_SCROLLING, FALSE) == TIMER_EXPIRED)
        {
            /* Rearm timer */
            timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
            
            /* Scrolling logic: when enabled, going left or right... */
            for (uint16_t i = 0; i < 2; i++)
            {
                if (scrolling_needed[i] != FALSE)
                {
                    if (text_anim_going_right[i] == FALSE)
                    {
                        text_anim_x_offset[i]--;
                    }
                    else
                    {
                        text_anim_x_offset[i]++;
                    }
                }
            }
            redraw_needed = TRUE;
        }
        
        /* Redraw if needed */
        if (redraw_needed != FALSE)
        {
            /* Clear frame buffer, set display settings */
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            sh1122_allow_partial_text_x_draw(&plat_oled_descriptor);
            
            /* Loop for 2 always displayed texts: login & password */
            for (uint16_t i = 0; i < 2; i++)
            {          
                /* Load the right font */
                sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_MEDIUM_15_ID);
                    
                /* Display string */
                if (scrolling_needed[i] != FALSE)
                {
                    /* Scrolling required: display with the correct X offset */
                    displayed_length = sh1122_put_string_xy(&plat_oled_descriptor, text_anim_x_offset[i], strings_y_positions[i], OLED_ALIGN_LEFT, strings_to_be_displayed[i], TRUE);
                        
                    /* Scrolling: go change direction if we went too far */
                    if (text_anim_going_right[i] == FALSE)
                    {
                        if (displayed_length == SH1122_OLED_WIDTH-12)
                        {
                            text_anim_going_right[i] = TRUE;
                        }
                    }
                    else
                    {
                        if (text_anim_x_offset[i] == 12)
                        {
                            text_anim_going_right[i] = FALSE;
                        }
                    }
                }
                else
                {
                    /* String not large enough or start of animation */
                    displayed_length = sh1122_put_centered_string(&plat_oled_descriptor, strings_y_positions[i], strings_to_be_displayed[i], TRUE);
                }
                    
                /* First run: based on the number of chars displayed, set the scrolling needed bool */
                if ((first_function_run != FALSE) && (displayed_length < 0))
                {
                    scrolling_needed[i] = TRUE;
                }
            }
            
            /* Reset display settings */
            sh1122_prevent_partial_text_x_draw(&plat_oled_descriptor);
            sh1122_reset_lim_display_y(&plat_oled_descriptor);
            
            /* Flush to display */
            if (first_function_run != FALSE)
            {
                first_function_run = FALSE;
            }
            else
            {
                sh1122_load_transition(&plat_oled_descriptor, OLED_TRANS_NONE);
            }
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_flush_frame_buffer(&plat_oled_descriptor);
            #endif
            
            /* Load function exit transition */
            sh1122_load_transition(&plat_oled_descriptor, OLED_IN_OUT_TRANS);
            
            /* Reset bool */
            redraw_needed = FALSE;
        }
    }
}