/*!  \file     logic_smartcard.c
*    \brief    General logic for smartcard operations
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/
#include "smartcard_highlevel.h"
#include "smartcard_lowlevel.h"
#include "logic_smartcard.h"
#include "gui_dispatcher.h"
#include "logic_security.h"
#include "gui_prompts.h"
#include "platform_io.h"
#include "logic_user.h"
#include "defines.h"


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
    // TODO    
    /*uint8_t temp_ctr_val[AES256_CTR_LENGTH];
    uint8_t temp_buffer[AES_KEY_LENGTH/8];*/

    /* Remove power and flags */
    platform_io_smc_remove_function();
    logic_security_clear_security_bools();
    
    // Clear encryption context
    /*memset((void*)temp_buffer, 0, AES_KEY_LENGTH/8);
    memset((void*)temp_ctr_val, 0, AES256_CTR_LENGTH);
    initEncryptionHandling(temp_buffer, temp_ctr_val);*/
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
    // By default, return to current screen
    BOOL return_to_current_screen = TRUE;
    // Return fail by default
    RET_TYPE return_value = RETURN_NOK;
    
    if ((detection_result == RETURN_MOOLTIPASS_PB) || (detection_result == RETURN_MOOLTIPASS_INVALID))
    {
        // Either it is not a card or our Manufacturer Test Zone write/read test failed
        gui_prompts_display_information_on_screen_and_wait(ID_STRING_PB_CARD);
        platform_io_smc_remove_function();
    }
    else if (detection_result == RETURN_MOOLTIPASS_BLOCKED)
    {
        // The card is blocked, no pin code tries are remaining
        gui_prompts_display_information_on_screen_and_wait(ID_STRING_CARD_BLOCKED);
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
                /* User entered twice the same PIN, card is still there */
                if (logic_user_create_new_user(&pin_code, FALSE, (uint8_t*)0) == RETURN_OK)
                {
                    /* PINs match, new user added to memories */
                    gui_prompts_display_information_on_screen_and_wait(ID_STRING_NEW_USER_ADDED);
                    logic_security_smartcard_unlocked_actions();
                    next_screen = GUI_SCREEN_MAIN_MENU;
                    return_value = RETURN_OK;
                }
                else
                {
                    // Something went wrong, user wasn't added
                    gui_prompts_display_information_on_screen_and_wait(ID_STRING_COULDNT_ADD_USER);                    
                }
            } 
            else
            {
                /* PIN different or card removed */
                if (smartcard_low_level_is_smc_absent() != RETURN_OK)
                {
                    gui_prompts_display_information_on_screen_and_wait(ID_STRING_DIFFERENT_PINS);                    
                }
                else
                {
                    return_to_current_screen = FALSE;
                }
            }
            
            /* Reset PIN code */
            pin_code = 0x0000;
        }
        else if (prompt_answer == MINI_INPUT_RET_CARD_REMOVED)
        {
            return_to_current_screen = FALSE;
        }
    }
    #ifdef blou
    // TODO
    else if (detection_result == RETURN_MOOLTIPASS_USER)
    {
        // Call valid card detection function
        uint8_t temp_return = validCardDetectedFunction(0, TRUE);
        
        // This a valid user smart card, we call a dedicated function for the user to unlock the card
        if (temp_return == RETURN_VCARD_OK)
        {
            unlockFeatureCheck();
            next_screen = SCREEN_DEFAULT_INSERTED_NLCK;
            return_value = RETURN_OK;
        }
        else if (temp_return == RETURN_VCARD_UNKNOWN)
        {
            // Unknown card, go to dedicated screen
            guiSetCurrentScreen(SCREEN_DEFAULT_INSERTED_UNKNOWN);
            guiGetBackToCurrentScreen();
            return return_value;
        }
        else
        {
            guiSetCurrentScreen(SCREEN_DEFAULT_INSERTED_LCK);
            guiGetBackToCurrentScreen();
            return return_value;
        }
        printSmartCardInfo();
    }
    #endif
    
    gui_dispatcher_set_current_screen(next_screen, TRUE, GUI_INTO_MENU_TRANSITION);
    if (return_to_current_screen != FALSE)
    {
        gui_dispatcher_get_back_to_current_screen();
    }
    return return_value;    
}