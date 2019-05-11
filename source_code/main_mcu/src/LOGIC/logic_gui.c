/*!  \file     logic_gui.c
*    \brief    General logic for GUI
*    Created:  11/05/2019
*    Author:   Mathieu Stephan
*/
#include "smartcard_highlevel.h"
#include "smartcard_lowlevel.h"
#include "logic_smartcard.h"
#include "gui_dispatcher.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "gui_prompts.h"
#include "logic_user.h"
#include "logic_gui.h"
#include "nodemgmt.h"
#include "text_ids.h"
#include "gui_menu.h"
#include "defines.h"

/*! \fn     logic_gui_disable_bluetooth(void)
*   \brief  Disable Bluetooth
*/
void logic_gui_disable_bluetooth(void)
{
    if (logic_aux_mcu_is_ble_enabled() != FALSE)
    {
        /* Send message to the Aux MCU to enable bluetooth, don't wait for it to be enabled */
        logic_aux_mcu_disable_ble(TRUE);
        
        /* Display information message */
        gui_prompts_display_information_on_screen_and_wait(BLUETOOTH_DISABLED_TEXT_ID, DISP_MSG_INFO);
        
        /* Set BLE enabled bool */
        logic_aux_mcu_set_ble_enabled_bool(FALSE);
        
        /* Refresh menus */
        gui_menu_update_menus();
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
    if (logic_smartcard_remove_card_and_reauth_user() == RETURN_OK)
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
                gui_prompts_display_information_on_screen_and_wait(CARD_NOT_BLANK_TEXT_ID, DISP_MSG_WARNING);
            }
            pin_code = 0x0000;
        }
        else if (temp_rettype == RETURN_NEW_PIN_DIFF)
        {
            /* Display that the PINs were different */
            gui_prompts_display_information_on_screen_and_wait(PIN_DIFFERENT_TEXT_ID, DISP_MSG_WARNING);
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
}

/*! \fn     logic_gui_erase_user(void)
*   \brief  Erase user
*/
void logic_gui_erase_user(void)
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
}