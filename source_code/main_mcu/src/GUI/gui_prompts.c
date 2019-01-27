/*!  \file     gui_prompts.c
*    \brief    Code dedicated to prompts and notifications
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include "smartcard_lowlevel.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "logic_device.h"
#include "gui_prompts.h"
#include "custom_fs.h"
#include "defines.h"
#include "inputs.h"
#include "main.h"
#include "rng.h"


/*! \fn     gui_prompts_render_pin_enter_screen(uint8_t* current_pin, uint16_t selected_digit, uint16_t stringID)
*   \brief  Overwrite the digits on the current pin entering screen
*   \param  current_pin     Array containing the pin
*   \param  selected_digit  Currently selected digit
*   \param  stringID        String ID for text query
*/
void gui_prompts_render_pin_enter_screen(uint8_t* current_pin, uint16_t selected_digit, uint16_t stringID)
{
    cust_char_t* string_to_display;
    
    /* Try to fetch the string to display */
    if (custom_fs_get_string_from_file(stringID, &string_to_display) != RETURN_OK)
    {
        return;
    }
    
    // Display bitmap
    /*miniOledBitmapDrawFlash(0, 0, selected_digit+BITMAP_PIN_SLOT1, 0);
    miniOledSetMaxTextY(62);
    miniOledAllowTextWritingYIncrement();
    miniOledPutCenteredString(TWO_LINE_TEXT_FIRST_POS, readStoredStringToBuffer(stringID));
    miniOledPreventTextWritingYIncrement();
    miniOledResetMaxTextY();
    miniOledSetFont(FONT_PROFONT_14);*/
    
    /* Clear frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #else
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #endif
    
    /* Display bitmap */
    sh1122_allow_line_feed(&plat_oled_descriptor);
    sh1122_set_max_text_x(&plat_oled_descriptor, 100);
    sh1122_refresh_used_font(&plat_oled_descriptor, 1);
    sh1122_put_centered_string(&plat_oled_descriptor, 20, string_to_display, TRUE);
    sh1122_refresh_used_font(&plat_oled_descriptor, 1);
    sh1122_prevent_line_feed(&plat_oled_descriptor);    
    sh1122_reset_max_text_x(&plat_oled_descriptor);
    
    /* Display the 4 digits */
    for (uint16_t i = 0; i < 4; i++)
    {
        sh1122_set_xy(&plat_oled_descriptor, 164+17*i, 16);
        if (i != selected_digit)
        {
            sh1122_put_char(&plat_oled_descriptor, u'*', TRUE);
        }
        else
        {
            if (current_pin[i] >= 0x0A)
            {
                sh1122_put_char(&plat_oled_descriptor, current_pin[i]+u'A'-0x0A, TRUE);
            }
            else
            {
                sh1122_put_char(&plat_oled_descriptor, current_pin[i]+u'0', TRUE);
            }
        }
    }
    
    /* Flush */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_flush_frame_buffer(&plat_oled_descriptor);
    #endif
}


/*! \fn     gui_prompts_get_user_pin(volatile uint16_t* pin_code, uint16_t stringID)
*   \brief  Ask the user to enter a PIN
*   \param  pin_code    Pointer to where to store the pin code
*   \param  stringID    String ID
*   \return If the user approved the request
*/
RET_TYPE gui_prompts_get_user_pin(volatile uint16_t* pin_code, uint16_t stringID)
{
    // If we don't need a pin code, send default one
    #if defined(NO_PIN_CODE_REQUIRED)
        *pin_code = SMARTCARD_DEFAULT_PIN;
        return RETURN_OK;
    #endif
    
    BOOL random_pin_feature_enabled = (BOOL)custom_fs_settings_get_device_setting(SETTING_RANDOM_PIN_ID);
    wheel_action_ret_te detection_result = WHEEL_ACTION_NONE;
    RET_TYPE ret_val = RETURN_NOK;
    uint16_t selected_digit = 0;
    uint8_t current_pin[4];
    BOOL finished = FALSE;
    
    // Set current pin to 0000 or random
    if (random_pin_feature_enabled == FALSE)
    {
        memset((void*)current_pin, 0, sizeof(current_pin));
    }
    else
    {
        rng_fill_array(current_pin, sizeof(current_pin));
        for (uint16_t i = 0; i < sizeof(current_pin); i++)
        {
            current_pin[i] &= 0x0F;
        }
    }
    
    // Clear current detections
    inputs_clear_detections();
    
    // Display current pin on screen
    gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID);
    
    // While the user hasn't entered his pin
    while(!finished)
    {
        // Still process the USB commands, reply with please retries
        comms_aux_mcu_routine(TRUE);
        
        // detection result
        detection_result = inputs_get_wheel_action(FALSE, FALSE);
        
        // Position increment / decrement
        if ((detection_result == WHEEL_ACTION_UP) || (detection_result == WHEEL_ACTION_DOWN))
        {
            if ((current_pin[selected_digit] == 0x0F) && (detection_result == WHEEL_ACTION_UP))
            {
                current_pin[selected_digit] = 0xFF;
            }
            else if ((current_pin[selected_digit] == 0) && (detection_result == WHEEL_ACTION_DOWN))
            {
                current_pin[selected_digit] = 0x10;
            }
            if (detection_result == WHEEL_ACTION_UP)
            {
                current_pin[selected_digit]++;
            }
            else
            {
                current_pin[selected_digit]--;
            }
            gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID);
        }
        
        // Return if card removed or timer expired
        if ((smartcard_low_level_is_smc_absent() == RETURN_OK) || (timer_has_timer_expired(TIMER_USER_INTERACTION, TRUE) == TIMER_EXPIRED))
        {
            // Smartcard removed, no reason to continue
            ret_val = RETURN_NOK;
            finished = TRUE;
        }
        
        // Change digit position or return/proceed
        if (detection_result == WHEEL_ACTION_LONG_CLICK)
        {
            if (selected_digit > 0)
            {
                if (random_pin_feature_enabled == FALSE)
                {
                    // When going back set pin digit to 0
                    current_pin[selected_digit] = 0;
                    current_pin[--selected_digit] = 0;
                }
                else
                {
                    rng_fill_array(&current_pin[selected_digit-1], 2);
                    current_pin[selected_digit] &= 0x0F;
                    current_pin[--selected_digit] &= 0x0F;
                }
            }
            else
            {
                ret_val = RETURN_NOK;
                finished = TRUE;
            }
            gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID);
        }
        else if (detection_result == WHEEL_ACTION_SHORT_CLICK)
        {
            if (selected_digit < 3)
            {
                selected_digit++;
            }
            else
            {
                ret_val = RETURN_OK;
                finished = TRUE;
            }
            gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID);
        }
    }
    
    // Store the pin
    *pin_code = (uint16_t)(((uint16_t)(current_pin[0]) << 12) | (((uint16_t)current_pin[1]) << 8) | (current_pin[2] << 4) | current_pin[3]);
    
    // Set current pin to 0000 & set default font
    memset((void*)current_pin, 0, sizeof(pin_code));
    
    // Return success status
    return ret_val;
}