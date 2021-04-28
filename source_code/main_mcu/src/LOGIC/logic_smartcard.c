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
/*!  \file     logic_smartcard.c
*    \brief    General logic for smartcard operations
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include "smartcard_highlevel.h"
#include "logic_accelerometer.h"
#include "smartcard_lowlevel.h"
#include "logic_encryption.h"
#include "logic_smartcard.h"
#include "gui_dispatcher.h"
#include "logic_security.h"
#include "logic_aux_mcu.h"
#include "driver_timer.h"
#include "logic_device.h"
#include "gui_prompts.h"
#include "logic_power.h"
#include "platform_io.h"
#include "logic_user.h"
#include "logic_gui.h"
#include "nodemgmt.h"
#include "text_ids.h"
#include "bearssl.h"
#include "utils.h"


/*! \fn     logic_smartcard_ask_for_new_pin(uint16_t* new_pin, uint16_t message_id)
*   \brief  Ask user to enter a new PIN
*   \param  new_pin     Pointer to where to store the new pin
*   \param  message_id  Message ID
*   \return Success status, see new_pinreturn_type_te
*/
new_pinreturn_type_te logic_smartcard_ask_for_new_pin(volatile uint16_t* new_pin, uint16_t message_id)
{
    volatile uint16_t other_pin;
    
    // Ask the user twice for the new pin and compare them
    if ((gui_prompts_get_user_pin(new_pin, message_id) == RETURN_OK) && (gui_prompts_get_user_pin(&other_pin, CONFIRM_PIN_TEXT_ID) == RETURN_OK))
    {
        if (*new_pin == other_pin)
        {
            other_pin = 0;
            return RETURN_NEW_PIN_OK;
        }
        else
        {
            return RETURN_NEW_PIN_DIFF;
        }
    }
    else
    {
        return RETURN_NEW_PIN_NOK;
    }
}

/*! \fn     logic_smartcard_handle_removed(void)
*   \brief  Here is where are handled all smartcard removal logic
*   \return RETURN_OK if user is authenticated
*/
void logic_smartcard_handle_removed(void)
{
    /* Remove power and flags */
    platform_io_smc_remove_function();
    logic_security_clear_security_bools();
    
    /* Delete encryption context */
    logic_encryption_delete_context();
}

/*! \fn     logic_smartcard_handle_inserted(void)
*   \brief  Here is where are handled all smartcard insertion logic
*   \return RETURN_OK if user is authenticated
*/
RET_TYPE logic_smartcard_handle_inserted(void)
{
    // Low level routine: see what kind of card we're dealing with
    mooltipass_card_detect_return_te detection_result = smartcard_highlevel_card_detected_routine();
    // By default, return to invalid screen
    gui_screen_te next_screen = GUI_SCREEN_INSERTED_INVALID;
    // Return fail by default
    RET_TYPE return_value = RETURN_NOK;
    // User language for inserted card
    uint16_t potential_user_language;
    
    // Language: set default one
    custom_fs_set_current_language(utils_check_value_for_range(custom_fs_settings_get_device_setting(SETTING_DEVICE_DEFAULT_LANGUAGE), 0, custom_fs_get_number_of_languages()-1));
    
    if ((detection_result == RETURN_MOOLTIPASS_PB) || (detection_result == RETURN_MOOLTIPASS_INVALID))
    {
        // Either it is not a card or our Manufacturer Test Zone write/read test failed
        gui_prompts_display_information_on_screen_and_wait(PB_CARD_TEXT_ID, DISP_MSG_WARNING, FALSE);
        platform_io_smc_remove_function();
    }
    else if (detection_result == RETURN_MOOLTIPASS_BLOCKED)
    {
        // The card is blocked, no pin code tries are remaining
        gui_prompts_display_information_on_screen_and_wait(CARD_BLOCKED_TEXT_ID, DISP_MSG_WARNING, FALSE);
        platform_io_smc_remove_function();
    }
    else if (detection_result == RETURN_MOOLTIPASS_BLANK)
    {
        // This is a user free card, we can ask the user to create a new user inside the Mooltipass
        mini_input_yes_no_ret_te prompt_answer = gui_prompts_ask_for_one_line_confirmation(CREATE_NEW_USER_TEXT_ID, FALSE, FALSE, TRUE);
        
        /* Create a new user with his new smart card */
        if (prompt_answer == MINI_INPUT_RET_YES)
        {
            volatile uint16_t pin_code;
            
            /* Ask user for new PIN */
            new_pinreturn_type_te new_pin_return = logic_smartcard_ask_for_new_pin(&pin_code, NEW_CARD_PIN_TEXT_ID);
            
            if (new_pin_return == RETURN_NEW_PIN_OK)
            {
                /* Check user for simple or advanced mode */
                BOOL use_simple_mode = FALSE;
                if (gui_prompts_ask_for_one_line_confirmation(USE_SIMPLE_MODE_TEXT_ID, FALSE, FALSE, TRUE) == MINI_INPUT_RET_YES)
                {
                    use_simple_mode = TRUE;
                }

                /* Waiting screen */
                gui_prompts_display_information_on_screen(PROCESSING_TEXT_ID, DISP_MSG_INFO);
                
                /* User entered twice the same PIN, card is still there */
                if (logic_user_create_new_user(&pin_code, (uint8_t*)0, use_simple_mode) == RETURN_OK)
                {
                    /* PINs match, new user added to memories */
                    gui_prompts_display_information_on_screen_and_wait(NEW_USER_ADDED_TEXT_ID, DISP_MSG_INFO, FALSE);
                    logic_user_get_and_clear_user_to_be_logged_off_flag();
                    logic_security_smartcard_unlocked_actions();
                    next_screen = GUI_SCREEN_MAIN_MENU;
                    return_value = RETURN_OK;
                }
                else
                {
                    // Something went wrong, user wasn't added
                    gui_prompts_display_information_on_screen_and_wait(COULDNT_ADD_USER_TEXT_ID, DISP_MSG_WARNING, FALSE);                    
                }
            } 
            else if (new_pin_return == RETURN_NEW_PIN_DIFF)
            {
                gui_prompts_display_information_on_screen_and_wait(DIFFERENT_PINS_TEXT_ID, DISP_MSG_WARNING, FALSE);
            }
            
            /* Reset PIN code */
            pin_code = 0x0000;
        }
    }
    else if (detection_result == RETURN_MOOLTIPASS_USER)
    {        
        /* Check if we know this card and if we need to enable bluetooth */
        RET_TYPE bluetooth_enable_return = logic_user_is_bluetooth_enabled_for_inserted_card(&potential_user_language);
        if (bluetooth_enable_return == RETURN_OK)
        {
            /* Set user language */
            custom_fs_set_current_language(potential_user_language);
            
            /* Launch bluetooth enable in the background */
            /* The code called after will possibly send commands to the AUX. This is alright as on the AUX MCU side the enable is done in a different routine */
            logic_aux_mcu_enable_ble(FALSE);
            
            /* The packet sent above is going to keep the aux busy for a while */
            comms_aux_mcu_update_timeout_delay(10000);
        } 
        else if (bluetooth_enable_return == RETURN_NOK)
        {
            custom_fs_set_current_language(potential_user_language);
            logic_gui_disable_bluetooth(FALSE);
        }
        
        /* Call valid card detection function */
        valid_card_det_return_te temp_return = logic_smartcard_valid_card_unlock(TRUE, FALSE);
        
        /* This a valid user smart card, we call a dedicated function for the user to unlock the card */
        if (temp_return == RETURN_VCARD_OK)
        {            
            /* Unlock service feature if enabled */
            logic_user_get_and_clear_user_to_be_logged_off_flag();
            logic_user_unlocked_feature_trigger();
            next_screen = GUI_SCREEN_MAIN_MENU;
            return_value = RETURN_OK;
        }
        else if (temp_return == RETURN_VCARD_UNKNOWN)
        {
            /* Unknown card, go to dedicated screen */
            next_screen = GUI_SCREEN_INSERTED_UNKNOWN;
        }
        else if (temp_return == RETURN_VCARD_BACK)
        {
            /* User chose to not answer his PIN */
            next_screen = GUI_SCREEN_INSERTED_LCK;
            
            /* Disable Bluetooth: wait for enable then disable */
            if (bluetooth_enable_return == RETURN_OK)
            {
                /* Wait for enable notification */
                while (logic_aux_mcu_is_ble_enabled() == FALSE)
                {
                    /* Do not deal with any message, just wait for the notification */
                    comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_SN);
                }
                
                /* Now disable bluetooth if needed */
                if (custom_fs_settings_get_device_setting(SETTINGS_DISABLE_BLE_ON_LOCK) != FALSE)
                {
                    logic_gui_disable_bluetooth(FALSE);
                }
            }
        }
        else
        {
            // Default value is ok
            
            /* Disable Bluetooth: wait for enable then disable */
            if (bluetooth_enable_return == RETURN_OK)
            {
                /* Wait for enable notification */
                while (logic_aux_mcu_is_ble_enabled() == FALSE)
                {
                    /* Do not deal with any message, just wait for the notification */
                    comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_SN);
                }
                
                /* Now disable bluetooth :D */
                logic_gui_disable_bluetooth(FALSE);
            }
        }
    }
    
    /* Set specified next screen */
    gui_dispatcher_set_current_screen(next_screen, TRUE, GUI_INTO_MENU_TRANSITION);
    
    /* Transition to next screen only if the card wasn't removed */
    if (smartcard_low_level_is_smc_absent() != RETURN_OK)
    {
        gui_dispatcher_get_back_to_current_screen();
    }
    return return_value;    
}

/*! \fn     logic_smartcard_valid_card_unlock(BOOL hash_allow_flag, BOOL fast_mode)
*   \brief  Function called when a valid mooltipass card is detected
*   \param  hash_allow_flag Set to allow hash display if option is enabled
*   \return Unlock status (see valid_card_det_return_t)
*/
valid_card_det_return_te logic_smartcard_valid_card_unlock(BOOL hash_allow_flag, BOOL fast_mode)
{
    br_aes_ct_ctrcbc_keys device_operations_aes_context;
    uint32_t password_buffer[AES_BLOCK_SIZE/8/sizeof(uint32_t)];
    uint8_t device_operations_aes_key[AES_KEY_LENGTH/8];
    uint8_t temp_ctr[AES256_CTR_LENGTH/8];
    uint8_t temp_buffer[AES_KEY_LENGTH/8];
    cpz_lut_entry_t* cpz_user_entry;
    
    /* Read code protected zone to see if know this particular card */
    smartcard_highlevel_read_code_protected_zone(temp_buffer);        
    
    /* See if we know the card and if so fetch the user profile */
    if (custom_fs_get_cpz_lut_entry(temp_buffer, &cpz_user_entry) == RETURN_OK)
    {
        /* Display AESenc(CTR) if desired */
        if ((custom_fs_settings_get_device_setting(SETTINGS_HASH_DISPLAY_FEATURE) != FALSE) && (hash_allow_flag != FALSE))
        {
            /* Fetch device operations key */
            custom_fs_get_device_operations_aes_key(device_operations_aes_key);
            
            /* Use the CPZ as counter for the bytes 8-15 of the Big Endian CTR (byte 1 is set to the purpose value) */
            memset(temp_ctr, 0, sizeof(temp_ctr));
            memcpy(&temp_ctr[8], temp_buffer, SMARTCARD_CPZ_LENGTH);
            temp_ctr[1] = HASH1_CTR_B1_ID;
            
            /* Initialize AES context */
            br_aes_ct_ctrcbc_init(&device_operations_aes_context, device_operations_aes_key, AES_KEY_LENGTH/8);
            
            /* Prepare what we want to encrypt: device SN */
            _Static_assert(sizeof(temp_ctr) == 8 + SMARTCARD_CPZ_LENGTH, "Invalid encryption technique");
            _Static_assert(sizeof(password_buffer) == (AES_BLOCK_SIZE/8), "Invalid buffer size");
            memset(password_buffer, 0, sizeof(password_buffer));
            password_buffer[0] = custom_fs_get_platform_serial_number();
            br_aes_ct_ctrcbc_ctr(&device_operations_aes_context, (void*)temp_ctr, password_buffer, sizeof(password_buffer));
                        
            /* Display AESenc(CTRVAL) */
            if (gui_prompts_display_hash((uint8_t*)password_buffer, HASH_1_TEXT_ID) == FALSE)
            {
                return RETURN_VCARD_NOK;
            }
            
            /* memset everything */
            memset(&device_operations_aes_context, 0, sizeof(device_operations_aes_context));
            memset(device_operations_aes_key, 0, sizeof(device_operations_aes_key));
            memset(temp_ctr, 0, sizeof(temp_ctr));
        }
        
        /* Already initialize user context, as there's no secret there. Also sets user language */
        logic_user_init_context(cpz_user_entry->user_id);
        
        /* Check for defective card, check always done on initial unlock */
        if ((fast_mode == FALSE) && (smartcard_highlevel_check_hidden_aes_key_contents() != RETURN_OK))
        {
            gui_prompts_display_information_on_screen_and_wait(CARD_FAILING_TEXT_ID, DISP_MSG_WARNING, FALSE);
        }
        
        /* Ask the user to enter his PIN and check it */
        unlock_ret_type_te unlock_return = logic_smartcard_user_unlock_process();
        
        /* Depending on unlock process result */
        if (unlock_return == UNLOCK_OK_RET)
        {
            /* Unlocking succeeded */
            smartcard_highlevel_read_aes_key(temp_buffer);
            
            // Display AESenc(AESkey) if desired: as this check is made to make sure the device isn't compromised, it is OK to display it.
            if ((custom_fs_settings_get_device_setting(SETTINGS_HASH_DISPLAY_FEATURE) != FALSE) && (hash_allow_flag != FALSE))
            {
                /* Fetch device operations key */
                custom_fs_get_device_operations_aes_key(device_operations_aes_key);
                
                /* Use the first 8 bytes of the aes key as counter for the bytes 8-15 of the Big Endian CTR (byte 1 is set to the purpose value) */
                memset(temp_ctr, 0, sizeof(temp_ctr));
                memcpy(&temp_ctr[8], temp_buffer, 8);
                temp_ctr[1] = HASH2_CTR_B1_ID;
                
                /* Initialize AES context */
                br_aes_ct_ctrcbc_init(&device_operations_aes_context, device_operations_aes_key, AES_KEY_LENGTH/8);
                
                /* Prepare what we want to encrypt: device SN */
                _Static_assert(sizeof(password_buffer) == (AES_BLOCK_SIZE/8), "Invalid buffer size");
                _Static_assert(sizeof(temp_ctr) == 8 + 8, "Invalid encryption technique");
                memset(password_buffer, 0, sizeof(password_buffer));
                password_buffer[0] = custom_fs_get_platform_serial_number();
                br_aes_ct_ctrcbc_ctr(&device_operations_aes_context, (void*)temp_ctr, password_buffer, sizeof(password_buffer));
                
                /* Display AESenc(AESkey) */
                if (gui_prompts_display_hash((uint8_t*)password_buffer, HASH_2_TEXT_ID) == FALSE)
                {
                    return RETURN_VCARD_NOK;
                }
                
                /* memset everything */
                memset(&device_operations_aes_context, 0, sizeof(device_operations_aes_context));
                memset(device_operations_aes_key, 0, sizeof(device_operations_aes_key));
                memset(temp_ctr, 0, sizeof(temp_ctr));
            }

            /* Init encryption handling, set smartcard unlocked flag */
            logic_encryption_init_context(temp_buffer, cpz_user_entry);
            logic_security_smartcard_unlocked_actions();
            
            /* Clear temp vars */
            memset((void*)temp_buffer, 0, sizeof(temp_buffer));
            
            /* Return success */
            return RETURN_VCARD_OK;
        }
        else if (unlock_return == UNLOCK_BACK_RET)
        {
            /* User simply chose to not answer his PIN */
            return RETURN_VCARD_BACK;
        }
        else
        {
            /* Unlocking failed */
            return RETURN_VCARD_NOK;
        }
    }
    else
    {
        // Unknown card
        return RETURN_VCARD_UNKNOWN;
    }
}

/*! \fn     logic_smartcard_user_unlock_process(void)
*   \brief  Function called for the user to unlock his smartcard
*   \return success status, see enum
*/
unlock_ret_type_te logic_smartcard_user_unlock_process(void)
{
    mooltipass_card_detect_return_te temp_rettype;
    BOOL warning_displayed = FALSE;
    volatile uint16_t temp_pin;

    /* Display warning if only one security try left */
    if (smartcard_highlevel_get_nb_sec_tries_left() == 1)
    {
        gui_prompts_display_information_on_screen_and_wait(LAST_PIN_TRY_TEXT_ID, DISP_MSG_INFO, FALSE);
        warning_displayed = TRUE;
    }
    
    while (1)
    {
        if (gui_prompts_get_user_pin(&temp_pin, INSERT_PIN_TEXT_ID) == RETURN_OK)
        {
            /* Try unlocking the smartcard */
            temp_rettype = smartcard_high_level_mooltipass_card_detected_routine(&temp_pin);
            
            switch(temp_rettype)
            {
                case RETURN_MOOLTIPASS_4_TRIES_LEFT :
                {
                    // Smartcard unlocked
                    temp_pin = 0x0000;
                    return UNLOCK_OK_RET;
                }
                case RETURN_MOOLTIPASS_0_TRIES_LEFT :
                {
                    gui_prompts_display_information_on_screen_and_wait(CARD_BLOCKED_TEXT_ID, DISP_MSG_WARNING, FALSE);
                    return UNLOCK_BLOCKED_RET;
                }
                case RETURN_MOOLTIPASS_1_TRIES_LEFT :
                {
                    /* If after a wrong try there's only one try left, ask user to remove his card as a security */
                    gui_prompts_display_information_on_screen_and_wait(WRONGPIN1LEFT_TEXT_ID, DISP_MSG_INFO, FALSE);
                    if(warning_displayed == FALSE)
                    {
                        // Inform the user to remove his smart card
                        gui_prompts_display_information_on_screen(REMOVE_CARD_TEXT_ID, DISP_MSG_ACTION);
                        
                        // Wait for the user to remove his smart card
                        while(smartcard_low_level_is_smc_absent() != RETURN_OK);
                        
                        return UNLOCK_CARD_REMOVED_RET;
                    }
                }
                case RETURN_MOOLTIPASS_PB :
                {
                    gui_prompts_display_information_on_screen_and_wait(PB_CARD_TEXT_ID, DISP_MSG_WARNING, FALSE);
                    return UNLOCK_CARD_ISSUE_RET;
                }
                default :
                {
                    // Both the enum and the defines allow us to do that
                    gui_prompts_display_information_on_screen_and_wait(WRONGPIN1LEFT_TEXT_ID + temp_rettype - RETURN_MOOLTIPASS_1_TRIES_LEFT, DISP_MSG_INFO, FALSE);
                    break;
                }
            }
        }
        else
        {
            // User cancelled the request or card removed
            if (smartcard_low_level_is_smc_absent() == RETURN_OK)
            {
                return UNLOCK_CARD_REMOVED_RET;
            } 
            else
            {
                return UNLOCK_BACK_RET;
            }
        }
    }
}

/*! \fn     logic_smartcard_remove_card_and_reauth_user(BOOL display_notification)
*   \brief  Re-authentication process
*   \param  display_notification    Boolean to specify if the "authenticate yourself" notification should be shown
*   \return success or not
*/
RET_TYPE logic_smartcard_remove_card_and_reauth_user(BOOL display_notification)
{
    uint8_t temp_cpz1[SMARTCARD_CPZ_LENGTH];
    uint8_t temp_cpz2[SMARTCARD_CPZ_LENGTH];
    
    // Get current CPZ
    smartcard_highlevel_read_code_protected_zone(temp_cpz1);
    
    // Disconnect smartcard
    logic_smartcard_handle_removed();
    
    // Here we cheat: we display a prompt to hide the power off / on
    if (display_notification != FALSE)
    {
        gui_prompts_display_information_on_screen_and_wait(PLEASE_REAUTH_TEXT_ID, DISP_MSG_WARNING, FALSE);
    }
    else
    {
        timer_delay_ms(500);
    }
    
    // Launch Unlocking process
    if ((smartcard_highlevel_card_detected_routine() == RETURN_MOOLTIPASS_USER) && (logic_smartcard_valid_card_unlock(FALSE, TRUE) == RETURN_VCARD_OK))
    {
        // Read other CPZ
        smartcard_highlevel_read_code_protected_zone(temp_cpz2);
        
        // Check that they're actually the sames
        if (memcmp(temp_cpz1, temp_cpz2, SMARTCARD_CPZ_LENGTH) == 0)
        {
            return RETURN_OK;
        }
        else
        {
            logic_device_set_state_changed();
            return RETURN_NOK;
        }
    }
    else
    {
        logic_device_set_state_changed();
        return RETURN_NOK;
    }
}


/*! \fn     logic_smartcard_clone_card(uint16_t* pincode)
*   \brief  Clone a smartcard
*   \param  pincode The current pin code
*   \return success or not
*/
RET_TYPE logic_smartcard_clone_card(volatile uint16_t* pincode)
{    
    // Temp buffers to store AZ1 & AZ2
    uint8_t temp_az1[SMARTCARD_AZ_BIT_LENGTH/8];
    uint8_t temp_az2[SMARTCARD_AZ_BIT_LENGTH/8];    
    uint8_t temp_cpz[SMARTCARD_CPZ_LENGTH];
    
    // Check that the current smart card is unlocked
    if (logic_security_is_smc_inserted_unlocked() == FALSE)
    {
        return RETURN_NOK;
    }
    
    // Read code protected zone
    smartcard_highlevel_read_code_protected_zone(temp_cpz);
    
    // Extract current AZ1 & AZ2
    smartcard_highlevel_read_application_zone1(temp_az1);
    smartcard_highlevel_read_application_zone2(temp_az2);
    
    // Inform the user to remove his smart card
    gui_prompts_display_information_on_screen_and_wait(REMOVE_CARD_TEXT_ID, DISP_MSG_ACTION, FALSE);
    
    // Wait for the user to remove his smart card
    uint16_t temp_uint16 = 0;
    uint16_t current_frame_id = 0;
    while (smartcard_lowlevel_is_card_plugged() != RETURN_JRELEASED)
    {
        /* Deal with incoming messages but do not deal with them */
        comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_SN);
        
        /* Accelerometer routine for RNG stuff */
        logic_accelerometer_routine();
        
        /* Handle possible power switches */
        logic_power_check_power_switch_and_battery(FALSE);
        
        if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
        {
            /* Display new animation frame bitmap, rearm timer with provided value */
            gui_prompts_display_information_on_string_single_anim_frame(&current_frame_id, &temp_uint16, DISP_MSG_ACTION);
            timer_start_timer(TIMER_ANIMATIONS, temp_uint16);
        }
    }
    
    // Inform the user to insert a blank smart card
    gui_prompts_display_information_on_screen_and_wait(INSERT_NEW_CARD_TEXT_ID, DISP_MSG_ACTION, FALSE);
    
    // Wait for the user to insert a blank smart card
    temp_uint16 = 0;
    current_frame_id = 0;
    while (smartcard_lowlevel_is_card_plugged() != RETURN_JDETECT)
    {
        /* Deal with incoming messages but do not deal with them */
        comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_SN);
        
        /* Accelerometer routine for RNG stuff */
        logic_accelerometer_routine();
        
        /* Handle possible power switches */
        logic_power_check_power_switch_and_battery(FALSE);
        
        if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
        {
            /* Display new animation frame bitmap, rearm timer with provided value */
            gui_prompts_display_information_on_string_single_anim_frame(&current_frame_id, &temp_uint16, DISP_MSG_ACTION);
            timer_start_timer(TIMER_ANIMATIONS, temp_uint16);
        }
    }
    gui_prompts_display_information_on_screen(PROCESSING_TEXT_ID, DISP_MSG_INFO);
    
    // Check that we have a blank card
    if (smartcard_highlevel_card_detected_routine() != RETURN_MOOLTIPASS_BLANK)
    {
        /* Device state has changed */
        logic_device_set_state_changed();
        
        return RETURN_NOK;
    }
    
    // Erase AZ1 & AZ2 in the new card
    smartcard_lowlevel_erase_application_zone1_nzone2(FALSE);
    smartcard_lowlevel_erase_application_zone1_nzone2(TRUE);
    
    // Write AZ1 & AZ2 & CPZ
    smartcard_highlevel_write_application_zone1(temp_az1);
    smartcard_highlevel_write_application_zone2(temp_az2);   
    smartcard_highlevel_write_protected_zone(temp_cpz);
    
    // Write new password
    smartcard_highlevel_write_security_code(pincode);
    
    // Set the smart card inserted unlocked flag, cleared by interrupt
    logic_security_smartcard_unlocked_actions();
    
    // Inform the user that it is done
    gui_prompts_display_information_on_screen_and_wait(CARD_CLONED_TEXT_ID, DISP_MSG_INFO, FALSE);
    
    return RETURN_OK;
}