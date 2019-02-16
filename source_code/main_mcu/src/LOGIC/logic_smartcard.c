/*!  \file     logic_smartcard.c
*    \brief    General logic for smartcard operations
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/
#include "smartcard_highlevel.h"
#include "logic_smartcard.h"
#include "gui_dispatcher.h"
#include "gui_prompts.h"
#include "platform_io.h"
#include "defines.h"


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
        if (gui_prompts_ask_for_one_line_confirmation(ID_STRING_CREATE_NEW_USER, FALSE) == MINI_INPUT_RET_YES)
        {
            volatile uint16_t pin_code;
            
            // Create a new user with his new smart card
            if ((gui_prompts_get_user_pin(&pin_code, ID_STRING_NEW_CARD_PIN) == RETURN_OK))// && (addNewUserAndNewSmartCard(&pin_code) == RETURN_OK))
            {
                gui_prompts_display_information_on_screen_and_wait(ID_STRING_NEW_USER_ADDED);
                next_screen = GUI_SCREEN_MAIN_MENU;
                //setSmartCardInsertedUnlocked();
                return_value = RETURN_OK;
            }
            else
            {
                // Something went wrong, user wasn't added
                gui_prompts_display_information_on_screen_and_wait(ID_STRING_COULDNT_ADD_USER);
            }
            pin_code = 0x0000;
        }
    }
    #ifdef blou
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
    gui_dispatcher_get_back_to_current_screen();
    return return_value;    
}