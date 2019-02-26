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
#include "sh1122.h"
#include "utils.h"
#include "main.h"
#include "rng.h"
// Text Y positions for conf prompt
const uint8_t gui_prompts_conf_prompt_y_positions[4][4] = {
    {ONE_LINE_TEXT_FIRST_POS, 0, 0, 0},
    {TWO_LINE_TEXT_FIRST_POS, TWO_LINE_TEXT_SECOND_POS, 0, 0},
    {THREE_LINE_TEXT_FIRST_POS, THREE_LINE_TEXT_SECOND_POS, THREE_LINE_TEXT_THIRD_POS, 0},
    {FOUR_LINE_TEXT_FIRST_POS, FOUR_LINE_TEXT_SECOND_POS, FOUR_LINE_TEXT_THIRD_POS, FOUR_LINE_TEXT_FOURTH_POS}
};
// Text fonts for conf prompt
const uint8_t gui_prompts_conf_prompt_fonts[4][4] = {
    {3, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
};
// Boxes height for each line
const uint8_t gui_prompts_conf_prompt_line_heights[4][4] = {
    {14, 14, 14, 14},
    {14, 14, 14, 14},
    {14, 14, 14, 14},
    {14, 14, 14, 14}
};


/*! \fn     gui_prompts_display_information_on_screen(uint16_t string_id)
*   \brief  Display text information on screen
*   \param  string_id   String ID to display
*/
void gui_prompts_display_information_on_screen(uint16_t string_id)
{
    cust_char_t* string_to_display;
    
    /* Clear frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_load_transition(&plat_oled_descriptor, OLED_OUT_IN_TRANS);
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #else
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #endif
    
    /* Try to fetch the string to display */
    custom_fs_get_string_from_file(string_id, &string_to_display, TRUE);
    
    /* Display string */
    sh1122_refresh_used_font(&plat_oled_descriptor, 1);
    sh1122_put_centered_string(&plat_oled_descriptor, INF_DISPLAY_TEXT_Y, string_to_display, TRUE);
    
    /* Flush frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_flush_frame_buffer(&plat_oled_descriptor);
    #endif
}

/*! \fn     gui_prompts_display_information_on_screen_and_wait(uint16_t string_id)
*   \brief  Display text information on screen
*   \param  string_id   String ID to display
*/
void gui_prompts_display_information_on_screen_and_wait(uint16_t string_id)
{
    // Display string 
    gui_prompts_display_information_on_screen(string_id);
    
    // Clear current detections
    inputs_clear_detections();
    
    /* Mandatory wait for information display */
    timer_start_timer(TIMER_WAIT_FUNCTS, 1500);
    while (timer_has_timer_expired(TIMER_WAIT_FUNCTS, TRUE) != TIMER_EXPIRED)
    {
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);        
    }
    
    /* Optional wait */
    timer_start_timer(TIMER_WAIT_FUNCTS, 2000);
    while ((timer_has_timer_expired(TIMER_WAIT_FUNCTS, TRUE) != TIMER_EXPIRED) && (inputs_get_wheel_action(FALSE, FALSE) != WHEEL_ACTION_SHORT_CLICK))
    {
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
    }
}

/*! \fn     gui_prompts_render_pin_enter_screen(uint8_t* current_pin, uint16_t selected_digit, uint16_t stringID, int16_t anim_direction)
*   \brief  Overwrite the digits on the current pin entering screen
*   \param  current_pin     Array containing the pin
*   \param  selected_digit  Currently selected digit
*   \param  stringID        String ID for text query
*/
void gui_prompts_render_pin_enter_screen(uint8_t* current_pin, uint16_t selected_digit, uint16_t stringID, int16_t anim_direction)
{
    cust_char_t* string_to_display;
    uint16_t cur_glyph_height;
    
    /* Try to fetch the string to display */
    custom_fs_get_string_from_file(stringID, &string_to_display, TRUE);
    
    /* Animation: get current digit and the next one */
    int16_t next_digit = current_pin[selected_digit] + anim_direction;
    if (next_digit == 0x10)
    {
        next_digit = 0;
    }
    else if (next_digit  < 0)
    {
        next_digit = 0x0F;
    }
    
    /* Convert current digit and next one into chars */
    cust_char_t current_char = current_pin[selected_digit] + u'0';
    cust_char_t next_char = next_digit + u'0';
    if (current_pin[selected_digit] >= 0x0A)
    {
        current_char = current_pin[selected_digit] + u'A' - 0x0A;
    }
    if (next_digit >= 0x0A)
    {
        next_char = next_digit + u'A' - 0x0A;
    }
    
    /* Animation: get current digit height to know the display boundaries */
    sh1122_refresh_used_font(&plat_oled_descriptor, 1);
    sh1122_get_glyph_width(&plat_oled_descriptor, current_char, &cur_glyph_height);
    
    /* Number of animation steps */
    int16_t nb_animation_steps = cur_glyph_height + 2;
    if (anim_direction == 0)
    {
        nb_animation_steps = 1;
    }
    
    /* Clear frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #else
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #endif
    
    /* Write fixed bitmaps & text */
    sh1122_allow_line_feed(&plat_oled_descriptor);
    sh1122_refresh_used_font(&plat_oled_descriptor, 1);
    sh1122_set_max_text_x(&plat_oled_descriptor, PIN_PROMPT_MAX_TEXT_X);
    sh1122_put_centered_string(&plat_oled_descriptor, PIN_PROMPT_TEXT_Y, string_to_display, TRUE);
    sh1122_prevent_line_feed(&plat_oled_descriptor);
    sh1122_reset_max_text_x(&plat_oled_descriptor);
    
    /* Prepare for digits display */
    sh1122_refresh_used_font(&plat_oled_descriptor, 1);
    sh1122_allow_partial_text_y_draw(&plat_oled_descriptor);
    sh1122_set_min_display_y(&plat_oled_descriptor, PIN_PROMPT_DIGIT_Y);
    sh1122_set_max_display_y(&plat_oled_descriptor, PIN_PROMPT_DIGIT_Y+cur_glyph_height);
    
    /* Animation code */
    for (int16_t anim_step = 0; anim_step < nb_animation_steps; anim_step++)
    {        
        /* Why erase a small box when you can erase the screen? */
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        if (anim_step != 0)
        {
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
        }
        #endif
        
        /* Display the 4 digits */
        for (uint16_t i = 0; i < 4; i++)
        {
            if (i != selected_digit)
            {
                sh1122_set_xy(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + PIN_PROMPT_DIGIT_X_SPC*i, PIN_PROMPT_DIGIT_Y+PIN_PROMPT_ASTX_Y_INC);
                sh1122_put_char(&plat_oled_descriptor, u'*', TRUE);
            }
            else
            {
                sh1122_set_xy(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + PIN_PROMPT_DIGIT_X_SPC*i, PIN_PROMPT_DIGIT_Y + anim_step*anim_direction);
                sh1122_put_char(&plat_oled_descriptor, current_char, TRUE);
                sh1122_set_xy(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + PIN_PROMPT_DIGIT_X_SPC*i, PIN_PROMPT_DIGIT_Y + anim_step*anim_direction + (cur_glyph_height+1)*anim_direction*-1);
                sh1122_put_char(&plat_oled_descriptor, next_char, TRUE);
            }
        }
        
        /* Flush updated part */
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        if (anim_step == 0)
        {
            sh1122_flush_frame_buffer(&plat_oled_descriptor);
        } 
        else
        {
            sh1122_flush_frame_buffer_window(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS, PIN_PROMPT_DIGIT_Y, PIN_PROMPT_DIGIT_X_SPC*4, PIN_PROMPT_DIGIT_Y_WDW);
        }
        #endif
        
        /* Small delay for animation */
        timer_delay_ms(3);
    }
        
    /* Reset temporary graphics settings */
    sh1122_reset_lim_display_y(&plat_oled_descriptor);
    sh1122_prevent_partial_text_y_draw(&plat_oled_descriptor);
}


/*! \fn     gui_prompts_get_user_pin(volatile uint16_t* pin_code, uint16_t stringID)
*   \brief  Ask the user to enter a PIN
*   \param  pin_code    Pointer to where to store the pin code
*   \param  stringID    String ID
*   \return If the user approved the request
*/
RET_TYPE gui_prompts_get_user_pin(volatile uint16_t* pin_code, uint16_t stringID)
{
    #ifdef SPECIAL_DEVELOPER_CARD_FEATURE
    if (special_dev_card_inserted != FALSE)
    {
        *pin_code = SMARTCARD_DEFAULT_PIN;
        return RETURN_OK;
    }
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
    gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID, 0);
    
    // While the user hasn't entered his pin
    while(!finished)
    {
        // Still process the USB commands, reply with please retries
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        
        // detection result
        detection_result = inputs_get_wheel_action(FALSE, FALSE);
        
        // Position increment / decrement
        if ((detection_result == WHEEL_ACTION_UP) || (detection_result == WHEEL_ACTION_DOWN))
        {
            if (detection_result == WHEEL_ACTION_UP)
            {
                gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID, 1);
                if (current_pin[selected_digit]++ == 0x0F)
                {
                    current_pin[selected_digit] = 0;
                }
                gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID, 0);
            }
            else
            {
                gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID, -1);
                if (current_pin[selected_digit]-- == 0)
                {
                    current_pin[selected_digit] = 0x0F;
                }
                gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID, 0);
            }
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
            gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID, 0);
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
            gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID, 0);
        }
    }
    
    // Store the pin
    *pin_code = (uint16_t)(((uint16_t)(current_pin[0]) << 12) | (((uint16_t)current_pin[1]) << 8) | (current_pin[2] << 4) | current_pin[3]);
    
    // Set current pin to 0000 & set default font
    memset((void*)current_pin, 0, sizeof(pin_code));
    
    // Return success status
    return ret_val;
}

/*! \fn     gui_prompts_ask_for_one_line_confirmation(uint16_t string_id, BOOL flash_screen)
*   \brief  Ask for user confirmation for different things
*   \param  string_id       String ID
*   \param  flash_screen    Boolean to flash screen
*   \return See enum
*/
mini_input_yes_no_ret_te gui_prompts_ask_for_one_line_confirmation(uint16_t string_id, BOOL flash_screen)
{
    cust_char_t* string_to_display;
    BOOL approve_selected = TRUE;
    BOOL flash_flag = FALSE;
    uint16_t flash_sm = 0;
    
    /* Try to fetch the string to display */
    custom_fs_get_string_from_file(string_id, &string_to_display, TRUE);
    
    /* Check the user hasn't disabled the flash screen feature */
    if ((flash_screen != FALSE) && ((BOOL)custom_fs_settings_get_device_setting(SETTING_FLASH_SCREEN_ID) != FALSE))
    {
        flash_flag = TRUE;
    }
    
    /* Clear frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_load_transition(&plat_oled_descriptor, OLED_OUT_IN_TRANS);
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #else
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #endif
    
    /* Display single line */
    sh1122_refresh_used_font(&plat_oled_descriptor, gui_prompts_conf_prompt_fonts[0][0]);
    sh1122_put_centered_string(&plat_oled_descriptor, gui_prompts_conf_prompt_y_positions[0][0], (cust_char_t*)string_to_display, TRUE);
    
    /* Flush to display */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_flush_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* Transition the action bitmap */
    for (uint16_t i = 0; i < POPUP_2LINES_ANIM_LGTH; i++)
    {
        /* Write both in frame buffer and display */
        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_POPUP_2LINES_Y+i, FALSE);
        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_POPUP_2LINES_Y+i, TRUE);
        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_POPUP_2LINES_N+i, FALSE);
        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_POPUP_2LINES_N+i, TRUE);            
        timer_delay_ms(15);
    }
    
    /* Wait for user input */
    mini_input_yes_no_ret_te input_answer = MINI_INPUT_RET_NONE;
    wheel_action_ret_te detect_result;
    
    /* Activity detected */
    logic_device_activity_detected();
    
    /* Clear possible remaining detection */
    inputs_clear_detections();

    /* Arm timer for flashing */
    timer_start_timer(TIMER_ANIMATIONS, 1000);
    
    // Loop while no timeout occurs or no button is pressed
    while (input_answer == MINI_INPUT_RET_NONE)
    {
        // User interaction timeout
        if (timer_has_timer_expired(TIMER_USER_INTERACTION, TRUE) == TIMER_EXPIRED)
        {
            input_answer = MINI_INPUT_RET_TIMEOUT;
        }
        
        // Card removed
        if (smartcard_low_level_is_smc_absent() == RETURN_OK)
        {
            input_answer = MINI_INPUT_RET_CARD_REMOVED;
        }
        
        // Read usb comms as the plugin could ask to cancel the request
        comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_CANCEL);
        /*if (usbCancelRequestReceived() == RETURN_OK)
        {
            input_answer = MINI_INPUT_RET_TIMEOUT;
        }*/
        
        // Check if something has been pressed
        detect_result = inputs_get_wheel_action(FALSE, TRUE);
        if (detect_result == WHEEL_ACTION_SHORT_CLICK)
        {
            if (approve_selected != FALSE)
            {
                input_answer = MINI_INPUT_RET_YES;
                for (uint16_t i = 0; i < POPUP_2LINES_ANIM_LGTH; i++)
                {
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_2LINES_PRESS_Y+i, FALSE);
                    timer_delay_ms(10);
                }
            }
            else
            {
                input_answer = MINI_INPUT_RET_NO;
                for (uint16_t i = 0; i < POPUP_2LINES_ANIM_LGTH; i++)
                {
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_2LINES_PRESS_N+i, FALSE);
                    timer_delay_ms(10);
                }
            }
        }
        else if (detect_result == WHEEL_ACTION_LONG_CLICK)
        {
            input_answer = MINI_INPUT_RET_BACK;
        }

        // TODO: Knock to approve
        #if defined(HARDWARE_MINI_CLICK_V2) && !defined(NO_ACCELEROMETER_FUNCTIONALITIES)
        if ((scanAndGetDoubleZTap(FALSE) == ACC_RET_KNOCK) && (flash_flag_set != FALSE))
        {
            input_answer = MINI_INPUT_RET_YES;
        }
        #endif
        
        // Approve / deny display change
        int16_t wheel_increments = inputs_get_wheel_increment();
        if (wheel_increments != 0)
        {
            if (approve_selected == FALSE)
            {
                for (uint16_t i = 0; i < CONF_2LINES_SEL_AN_LGTH; i++)
                {
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_2LINES_SEL_Y+i, FALSE);
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_2LINES_SEL_N+CONF_2LINES_SEL_AN_LGTH-1-i, FALSE);
                    timer_delay_ms(10);
                }
            }
            else
            {
                for (uint16_t i = 0; i < CONF_2LINES_SEL_AN_LGTH; i++)
                {
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_2LINES_SEL_N+i, FALSE);
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_2LINES_SEL_Y+CONF_2LINES_SEL_AN_LGTH-1-i, FALSE);
                    timer_delay_ms(10);
                }
            }                
            approve_selected = !approve_selected;
            
            /* Reset state machine and wait before displaying idle animation again */
            timer_start_timer(TIMER_ANIMATIONS, 3000);
            flash_sm = 0;
        }
        
        // Idle animation if enabled
        if ((flash_flag != FALSE) && (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED))
        {
            if (approve_selected == FALSE)
            {
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_2LINES_IDLE_N+flash_sm, FALSE);
            }
            else
            {
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_2LINES_IDLE_Y+flash_sm, FALSE);
            }
            
            /* Rearm timer */
            timer_start_timer(TIMER_ANIMATIONS, 50);
            
            /* Check for overflow */
            if (flash_sm++ == CONF_2LINES_IDLE_AN_LGT-1)
            {
                timer_start_timer(TIMER_ANIMATIONS, 200);
                flash_sm = 0;
            }
        }
    }
    
    return input_answer;    
}

/*! \fn     gui_prompts_ask_for_confirmation(uint16_t nb_args, confirmationText_t* text_object, BOOL flash_screen)
*   \brief  Ask for user confirmation for different things
*   \param  nb_args         Number of text lines (2 to 4)
*   \param  text_object     Pointer to the text object
*   \param  flash_screen    Boolean to flash screen
*   \return See enum
*/
mini_input_yes_no_ret_te gui_prompts_ask_for_confirmation(uint16_t nb_args, confirmationText_t* text_object, BOOL flash_screen)
{
    BOOL flash_flag = FALSE;
    uint16_t flash_sm = 0;
    
    /* Check the user hasn't disabled the flash screen feature */
    if ((flash_screen != FALSE) && ((BOOL)custom_fs_settings_get_device_setting(SETTING_FLASH_SCREEN_ID) != FALSE))
    {
        flash_flag = TRUE;
    }
    flash_flag = TRUE;

    // Variables for scrolling
    BOOL string_scrolling[4];
    uint16_t string_lengths[4];
    int16_t string_offset_cntrs[4] = {0,0,0,0};
    BOOL string_scrolling_left[4] = {TRUE, TRUE, TRUE, TRUE};
        
    /* Currently selected choice */
    BOOL approve_selected = TRUE;
    
    /* Clear frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_load_transition(&plat_oled_descriptor, OLED_OUT_IN_TRANS);
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #else
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #endif
    
    /* Set text display preferences */
    sh1122_set_max_text_x(&plat_oled_descriptor, CONF_PROMPT_MAX_TEXT_X);   
    
    /* Compute lines lengths, display them, set scrolling bools */
    for (uint16_t i = 0; i < nb_args; i++)
    {
        /* Set correct font */
        sh1122_refresh_used_font(&plat_oled_descriptor, gui_prompts_conf_prompt_fonts[nb_args-1][i]);
            
        /* Is scrolling needed for that string? */
        string_lengths[i] = utils_strlen(text_object->lines[i]);
        sh1122_prevent_partial_text_x_draw(&plat_oled_descriptor);
        if (string_lengths[i] != sh1122_put_centered_string(&plat_oled_descriptor, gui_prompts_conf_prompt_y_positions[nb_args-1][i], text_object->lines[i], TRUE))
        {
            string_scrolling[i] = TRUE;
        }
        else
        {
            string_scrolling[i] = FALSE;
        }
            
        /* Now allow partial X text display and display partial text if there's any */
        sh1122_allow_partial_text_x_draw(&plat_oled_descriptor);
        sh1122_put_centered_string(&plat_oled_descriptor, gui_prompts_conf_prompt_y_positions[nb_args-1][i], text_object->lines[i], TRUE);
    }
    
    /* Flush to display */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_flush_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* Transition done, now transition the action bitmap */
    for (uint16_t i = 0; i < POPUP_3LINES_ANIM_LGTH; i++)
    {
        /* Write both in frame buffer and display */
        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_POPUP_3LINES_ID+i, FALSE);
        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_POPUP_3LINES_ID+i, TRUE);
        timer_delay_ms(30);
    }
    
    /* Wait for user input */
    mini_input_yes_no_ret_te input_answer = MINI_INPUT_RET_NONE;
    wheel_action_ret_te detect_result;
    
    /* Activity detected */
    logic_device_activity_detected();
    
    /* Clear possible remaining detection */
    inputs_clear_detections();
    
    /* Arm timer for scrolling */
    timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);

    /* Arm timer for flashing */
    timer_start_timer(TIMER_ANIMATIONS, 1000);
    
    // Loop while no timeout occurs or no button is pressed
    while (input_answer == MINI_INPUT_RET_NONE)
    {        
        // User interaction timeout
        if (timer_has_timer_expired(TIMER_USER_INTERACTION, TRUE) == TIMER_EXPIRED)
        {
            input_answer = MINI_INPUT_RET_TIMEOUT;
        }
        
        // Card removed
        if (smartcard_low_level_is_smc_absent() == RETURN_OK)
        {
            input_answer = MINI_INPUT_RET_CARD_REMOVED;
        }
        
        // Read usb comms as the plugin could ask to cancel the request
        comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_CANCEL);
        /*if (usbCancelRequestReceived() == RETURN_OK)
        {
            input_answer = MINI_INPUT_RET_TIMEOUT;
        }*/
        
        // Check if something has been pressed
        detect_result = inputs_get_wheel_action(FALSE, TRUE);
        if (detect_result == WHEEL_ACTION_SHORT_CLICK)
        { 
            if (approve_selected != FALSE)
            {
                input_answer = MINI_INPUT_RET_YES;
                for (uint16_t i = 0; i < POPUP_3LINES_ANIM_LGTH+1; i++)
                {
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_YES_PRESS_ID+i, FALSE);
                    timer_delay_ms(10);
                }
            }
            else
            {
                input_answer = MINI_INPUT_RET_NO;
                for (uint16_t i = 0; i < POPUP_3LINES_ANIM_LGTH+1; i++)
                {
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_NO_PRESS_ID+i, FALSE);
                    timer_delay_ms(10);
                }
            }
        }
        else if (detect_result == WHEEL_ACTION_LONG_CLICK)
        {
            input_answer = MINI_INPUT_RET_BACK;
        }

        // Knock to approve
        #if defined(HARDWARE_MINI_CLICK_V2) && !defined(NO_ACCELEROMETER_FUNCTIONALITIES)
        if ((scanAndGetDoubleZTap(FALSE) == ACC_RET_KNOCK) && (flash_flag_set != FALSE))
        {
            input_answer = MINI_INPUT_RET_YES;
        }
        #endif
        
        /* Text scrolling animation */
        if (timer_has_timer_expired(TIMER_SCROLLING, FALSE) == TIMER_EXPIRED)
        {
            /* Clear frame buffer as we'll only push the updated parts */
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            #endif        
            
            /* Rearm timer */
            timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
            
            // Idle animation if enabled
            if ((flash_flag != FALSE) && (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED))
            {
                /* Rearm timer: won't obviously fire at said timeout as this is included in the above if() */
                timer_start_timer(TIMER_ANIMATIONS, 50);
                
                /* Check for overflow */
                if (flash_sm++ == CONF_3LINES_IDLE_AN_LGT-1)
                {
                    timer_start_timer(TIMER_ANIMATIONS, 200);
                    flash_sm = 0;
                }
            }
            
            /* Display bitmap */
            if(approve_selected == FALSE)
            {
                if (flash_sm != 0)
                {
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_3LINES_IDLE_N+flash_sm, TRUE);
                }
                else
                {
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_NY_DOWN_ID, TRUE);
                }
            }
            else
            {
                if (flash_sm != 0)
                {
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_3LINES_IDLE_Y+flash_sm, TRUE);
                }
                else
                {
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_NY_DOWN_ID+POPUP_3LINES_ANIM_LGTH-1, TRUE);
                }
            }

            /* Display all strings */
            for (uint16_t i = 0; i < nb_args; i++)
            {
                if (string_scrolling[i] != FALSE)
                {
                    /* Erase previous part */
                    #ifndef OLED_INTERNAL_FRAME_BUFFER
                    sh1122_draw_rectangle(&plat_oled_descriptor, 0, gui_prompts_conf_prompt_y_positions[nb_args-1][i], CONF_PROMPT_MAX_TEXT_X, gui_prompts_conf_prompt_line_heights[nb_args-1][i], 0x00, TRUE);
                    #endif
                    
                    /* Set correct font */
                    sh1122_refresh_used_font(&plat_oled_descriptor, gui_prompts_conf_prompt_fonts[nb_args-1][i]);
                    
                    /* Check if scrolling is not needed anymore */
                    sh1122_prevent_partial_text_x_draw(&plat_oled_descriptor);
                    if (sh1122_put_string_xy(&plat_oled_descriptor, string_offset_cntrs[i], gui_prompts_conf_prompt_y_positions[nb_args-1][i], OLED_ALIGN_LEFT, text_object->lines[i], TRUE) == string_lengths[i])
                    {
                        string_scrolling_left[i] = FALSE;
                    } 
                    if (string_offset_cntrs[i] == 0)
                    {
                        string_scrolling_left[i] = TRUE;
                    }
                    
                    /* Now allow partial X text display and display partial text if there's any */
                    sh1122_allow_partial_text_x_draw(&plat_oled_descriptor);
                    sh1122_put_string_xy(&plat_oled_descriptor, string_offset_cntrs[i], gui_prompts_conf_prompt_y_positions[nb_args-1][i], OLED_ALIGN_LEFT, text_object->lines[i], TRUE);
                    
                    /* Increment or decrement X offset */
                    if (string_scrolling_left[i] == FALSE)
                    {
                        string_offset_cntrs[i]++;
                    } 
                    else
                    {
                        string_offset_cntrs[i]--;
                    }
                }
                else
                {
                    sh1122_put_centered_string(&plat_oled_descriptor, gui_prompts_conf_prompt_y_positions[nb_args-1][i], text_object->lines[i], TRUE);                    
                }
            }
            
            /* Flush to display */
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_flush_frame_buffer(&plat_oled_descriptor);
            #endif
        }
        
        // Approve / deny display change
        int16_t wheel_increments = inputs_get_wheel_increment();
        if (wheel_increments != 0)
        {
            if (wheel_increments > 0)
            {
                if (approve_selected == FALSE)
                {
                    for (uint16_t i = 0; i < POPUP_3LINES_ANIM_LGTH; i++)
                    {
                        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_NY_UP_ID+i, FALSE);
                        timer_delay_ms(10);
                    }
                } 
                else
                {
                    for (int16_t i = POPUP_3LINES_ANIM_LGTH-1; i >= 0; i--)
                    {
                        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_NY_UP_ID+i, FALSE);
                        timer_delay_ms(10);
                    }
                }
            } 
            else
            {
                if (approve_selected == FALSE)
                {
                    for (uint16_t i = 0; i < POPUP_3LINES_ANIM_LGTH; i++)
                    {
                        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_NY_DOWN_ID+i, FALSE);
                        timer_delay_ms(10);
                    }
                }
                else
                {
                    for (int16_t i = POPUP_3LINES_ANIM_LGTH-1; i >= 0; i--)
                    {
                        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_NY_DOWN_ID+i, FALSE);
                        timer_delay_ms(10);
                    }
                }
            }
                
            approve_selected = !approve_selected;
            
            /* Reset state machine and wait before displaying idle animation again */
            timer_start_timer(TIMER_ANIMATIONS, 3000);
            flash_sm = 0;
        }
    }
    
    /* Reset text preferences */
    sh1122_reset_max_text_x(&plat_oled_descriptor);
    sh1122_prevent_partial_text_x_draw(&plat_oled_descriptor);
    
    return input_answer;
}
