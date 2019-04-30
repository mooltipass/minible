/*!  \file     logic_smartcard.c
*    \brief    General logic for smartcard operations
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include "smartcard_highlevel.h"
#include "smartcard_lowlevel.h"
#include "logic_encryption.h"
#include "logic_smartcard.h"
#include "gui_dispatcher.h"
#include "logic_security.h"
#include "driver_timer.h"
#include "gui_prompts.h"
#include "platform_io.h"
#include "logic_user.h"


/*! \fn     logic_smartcard_ask_for_new_pin(uint16_t* new_pin, uint16_t message_id)
*   \brief  Ask user to enter a new PIN
*   \param  new_pin     Pointer to where to store the new pin
*   \param  message_id  Message ID
*   \return Success status, see new_pinreturn_type_t
*/
RET_TYPE logic_smartcard_ask_for_new_pin(volatile uint16_t* new_pin, uint16_t message_id)
{
    volatile uint16_t other_pin;
    
    // Ask the user twice for the new pin and compare them
    if ((gui_prompts_get_user_pin(new_pin, message_id) == RETURN_OK) && (gui_prompts_get_user_pin(&other_pin, ID_STRING_CONFIRM_PIN) == RETURN_OK))
    {
        if (*new_pin == other_pin)
        {
            other_pin = 0;
            return RETURN_OK;
        }
        else
        {
            return RETURN_NOK;
        }
    }
    else
    {
        return RETURN_NOK;
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
    
    if ((detection_result == RETURN_MOOLTIPASS_PB) || (detection_result == RETURN_MOOLTIPASS_INVALID))
    {
        // Either it is not a card or our Manufacturer Test Zone write/read test failed
        gui_prompts_display_information_on_screen_and_wait(ID_STRING_PB_CARD, DISP_MSG_WARNING);
        platform_io_smc_remove_function();
    }
    else if (detection_result == RETURN_MOOLTIPASS_BLOCKED)
    {
        // The card is blocked, no pin code tries are remaining
        gui_prompts_display_information_on_screen_and_wait(ID_STRING_CARD_BLOCKED, DISP_MSG_WARNING);
        platform_io_smc_remove_function();
    }
    else if (detection_result == RETURN_MOOLTIPASS_BLANK)
    {
        // This is a user free card, we can ask the user to create a new user inside the Mooltipass
        mini_input_yes_no_ret_te prompt_answer = gui_prompts_ask_for_one_line_confirmation(ID_STRING_CREATE_NEW_USER, FALSE);
        
        if (prompt_answer == MINI_INPUT_RET_YES)
        {
            volatile uint16_t pin_code;
            
            /* Create a new user with his new smart card */
            if (logic_smartcard_ask_for_new_pin(&pin_code, ID_STRING_NEW_CARD_PIN) == RETURN_OK)
            {
                /* Waiting screen */
                gui_prompts_display_information_on_screen(ID_STRING_PROCESSING, DISP_MSG_INFO);
                
                /* User entered twice the same PIN, card is still there */
                if (logic_user_create_new_user(&pin_code, (uint8_t*)0) == RETURN_OK)
                {
                    /* PINs match, new user added to memories */
                    gui_prompts_display_information_on_screen_and_wait(ID_STRING_NEW_USER_ADDED, DISP_MSG_INFO);
                    logic_security_smartcard_unlocked_actions();
                    next_screen = GUI_SCREEN_MAIN_MENU;
                    return_value = RETURN_OK;
                }
                else
                {
                    // Something went wrong, user wasn't added
                    gui_prompts_display_information_on_screen_and_wait(ID_STRING_COULDNT_ADD_USER, DISP_MSG_WARNING);                    
                }
            } 
            else if (smartcard_low_level_is_smc_absent() != RETURN_OK)
            {
                gui_prompts_display_information_on_screen_and_wait(ID_STRING_DIFFERENT_PINS, DISP_MSG_WARNING);
            }
            
            /* Reset PIN code */
            pin_code = 0x0000;
        }
    }
    else if (detection_result == RETURN_MOOLTIPASS_USER)
    {
        // Call valid card detection function
        valid_card_det_return_te temp_return = logic_smartcard_valid_card_unlock(TRUE);
        
        /* This a valid user smart card, we call a dedicated function for the user to unlock the card */
        if (temp_return == RETURN_VCARD_OK)
        {
            //unlockFeatureCheck();
            next_screen = GUI_SCREEN_MAIN_MENU;
            return_value = RETURN_OK;
        }
        else if (temp_return == RETURN_VCARD_UNKNOWN)
        {
            // Unknown card, go to dedicated screen
            next_screen = GUI_SCREEN_INSERTED_UNKNOWN;
        }
        else
        {
            // Default values are ok
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

/*! \fn     logic_smartcard_valid_card_unlock(BOOL hash_allow_flag)
*   \brief  Function called when a valid mooltipass card is detected
*   \param  hash_allow_flag Set to allow hash display if option is enabled
*   \return Unlock status (see valid_card_det_return_t)
*/
valid_card_det_return_te logic_smartcard_valid_card_unlock(BOOL hash_allow_flag)
{
    #ifdef MINI_VERSION
    uint8_t plateform_aes_key[AES_KEY_LENGTH/8];
    #endif
    uint8_t temp_buffer[AES_KEY_LENGTH/8];
    cpz_lut_entry_t* cpz_user_entry;
    
    /* Read code protected zone to see if know this particular card */
    smartcard_highlevel_read_code_protected_zone(temp_buffer);        
    
    /* See if we know the card and if so fetch the user profile */
    if (custom_fs_get_cpz_lut_entry(temp_buffer, &cpz_user_entry) == RETURN_OK)
    {
        // Display AESenc(CTR) if desired
        #ifdef MINI_VERSION
        if ((getMooltipassParameterInEeprom(HASH_DISPLAY_FEATURE_PARAM) != FALSE) && (hash_allow_flag != FALSE))
        {
            // Fetch AES key from eeprom: 30 bytes after the first 32bytes of EEP_BOOT_PWD, then last 2 bytes at EEP_LAST_AES_KEY2_2BYTES_ADDR
            eeprom_read_block(plateform_aes_key, (void*)(EEP_BOOT_PWD+(AES_KEY_LENGTH/8)), 30);
            eeprom_read_block(plateform_aes_key+30, (void*)EEP_LAST_AES_KEY2_2BYTES_ADDR, 2);

            // Display AESenc(CTRVAL)
            computeAndDisplayBlockSizeEncryptionResult(plateform_aes_key, temp_ctr_val, ID_STRING_HASH1);
        }
        #endif
        
        /* Check card itself */
        if (smartcard_highlevel_check_hidden_aes_key_contents() != RETURN_OK)
        {
            gui_prompts_display_information_on_screen_and_wait(ID_STRING_CARD_FAILING, DISP_MSG_WARNING);
        }
        
        /* Ask the user to enter his PIN and check it */
        if (logic_smartcard_user_unlock_process() == RETURN_OK)
        {
            /* Unlocking succeeded */
            smartcard_highlevel_read_aes_key(temp_buffer);
            
            // Display AESenc(AESkey) if desired: as this check is made to make sure the device isn't compromised, it is OK to display it.
            #ifdef MINI_VERSION
            if ((getMooltipassParameterInEeprom(HASH_DISPLAY_FEATURE_PARAM) != FALSE) && (hash_allow_flag != FALSE))
            {
                // Fetch AES key from eeprom: 30 bytes after the first 32bytes of EEP_BOOT_PWD, then last 2 bytes at EEP_LAST_AES_KEY2_2BYTES_ADDR
                eeprom_read_block(plateform_aes_key, (void*)(EEP_BOOT_PWD+(AES_KEY_LENGTH/8)), 30);
                eeprom_read_block(plateform_aes_key+30, (void*)EEP_LAST_AES_KEY2_2BYTES_ADDR, 2);

                // Display AESenc(AESkey)
                computeAndDisplayBlockSizeEncryptionResult(plateform_aes_key, temp_buffer, ID_STRING_HASH2);
            }
            #endif

            /* Init user flash context and encryption handling, set smartcard unlocked flag */             
            logic_user_init_context(cpz_user_entry->user_id);
            logic_encryption_init_context(temp_buffer, cpz_user_entry);
            logic_security_smartcard_unlocked_actions();
            
            /* Clear temp vars */
            memset((void*)temp_buffer, 0, sizeof(temp_buffer));
            
            /* Return success */
            return RETURN_VCARD_OK;
        }
        else
        {
            // Unlocking failed
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
*   \return success status
*/
RET_TYPE logic_smartcard_user_unlock_process(void)
{
    mooltipass_card_detect_return_te temp_rettype;
    BOOL warning_displayed = FALSE;
    volatile uint16_t temp_pin;

    /* Display warning if only one security try left */
    if (smartcard_highlevel_get_nb_sec_tries_left() == 1)
    {
        gui_prompts_display_information_on_screen_and_wait(ID_STRING_LAST_PIN_TRY, DISP_MSG_INFO);
        warning_displayed = TRUE;
    }
    
    while (1)
    {
        if (gui_prompts_get_user_pin(&temp_pin, ID_STRING_INSERT_PIN) == RETURN_OK)
        {
            /* Try unlocking the smartcard */
            temp_rettype = smartcard_high_level_mooltipass_card_detected_routine(&temp_pin);
            
            switch(temp_rettype)
            {
                case RETURN_MOOLTIPASS_4_TRIES_LEFT :
                {
                    // Smartcard unlocked
                    temp_pin = 0x0000;
                    return RETURN_OK;
                }
                case RETURN_MOOLTIPASS_0_TRIES_LEFT :
                {
                    gui_prompts_display_information_on_screen_and_wait(ID_STRING_CARD_BLOCKED, DISP_MSG_WARNING);
                    return RETURN_NOK;
                }
                case RETURN_MOOLTIPASS_1_TRIES_LEFT :
                {
                    /* If after a wrong try there's only one try left, ask user to remove his card as a security */
                    gui_prompts_display_information_on_screen_and_wait(ID_STRING_WRONGPIN1LEFT, DISP_MSG_INFO);
                    if(warning_displayed == FALSE)
                    {
                        // Inform the user to remove his smart card
                        gui_prompts_display_information_on_screen(ID_STRING_REMOVE_CARD, DISP_MSG_ACTION);
                        
                        // Wait for the user to remove his smart card
                        while(smartcard_low_level_is_smc_absent() != RETURN_OK);
                        
                        return RETURN_NOK;
                    }
                }
                case RETURN_MOOLTIPASS_PB :
                {
                    gui_prompts_display_information_on_screen_and_wait(ID_STRING_PB_CARD, DISP_MSG_WARNING);
                    return RETURN_NOK;
                }
                default :
                {
                    // Both the enum and the defines allow us to do that
                    gui_prompts_display_information_on_screen_and_wait(ID_STRING_WRONGPIN1LEFT + temp_rettype - RETURN_MOOLTIPASS_1_TRIES_LEFT, DISP_MSG_INFO);
                    break;
                }
            }
        }
        else
        {
            // User cancelled the request
            return RETURN_NOK;
        }
    }
}

/*! \fn     logic_smartcard_remove_card_and_reauth_user(void)
*   \brief  Re-authentication process
*   \return success or not
*/
RET_TYPE logic_smartcard_remove_card_and_reauth_user(void)
{
    uint8_t temp_cpz1[SMARTCARD_CPZ_LENGTH];
    uint8_t temp_cpz2[SMARTCARD_CPZ_LENGTH];
    
    // Get current CPZ
    smartcard_highlevel_read_code_protected_zone(temp_cpz1);
    
    // Disconnect smartcard
    logic_smartcard_handle_removed();
    
    // Wait a few ms
    timer_delay_ms(250);
    
    // Launch Unlocking process
    if ((smartcard_highlevel_card_detected_routine() == RETURN_MOOLTIPASS_USER) && (logic_smartcard_valid_card_unlock(FALSE) == RETURN_VCARD_OK))
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
            return RETURN_NOK;
        }
    }
    else
    {
        return RETURN_NOK;
    }
}
