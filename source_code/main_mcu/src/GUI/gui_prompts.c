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
/*!  \file     gui_prompts.c
*    \brief    Code dedicated to prompts and notifications
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include "logic_accelerometer.h"
#include "smartcard_lowlevel.h"
#include "logic_database.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "logic_device.h"
#include "platform_io.h"
#include "logic_power.h"
#include "gui_prompts.h"
#include "custom_fs.h"
#include "nodemgmt.h"
#include "text_ids.h"
#include "defines.h"
#include "inputs.h"
#include "sh1122.h"
#include "utils.h"
#include "main.h"
#include "rng.h"
// Text Y positions for conf prompt
const uint8_t gui_prompts_conf_prompt_y_positions[4][4] = {
    {4, 0, 0, 0},
    {12, 36, 0, 0},
    {2, 22, 43, 0},
    {0, 13, 29, 46}
};
// Same but for the tutorial
const uint8_t gui_prompts_tutorial_3_lines_y_positions[3] = {0, 21, 42};
// Text fonts for conf prompt
const uint8_t gui_prompts_conf_prompt_fonts[4][4] = {
    {FONT_UBUNTU_MEDIUM_17_ID, 0, 0, 0},
    {FONT_UBUNTU_MEDIUM_17_ID, FONT_UBUNTU_REGULAR_16_ID, 0, 0},
    {FONT_UBUNTU_REGULAR_16_ID, FONT_UBUNTU_MEDIUM_17_ID, FONT_UBUNTU_REGULAR_16_ID, 0},
    {FONT_UBUNTU_MEDIUM_15_ID, FONT_UBUNTU_REGULAR_16_ID, FONT_UBUNTU_MEDIUM_17_ID, FONT_UBUNTU_REGULAR_16_ID}
};
// Max text X for conf prompts
const uint8_t gui_prompts_conf_prompts_max_x[4][4] = {
    {222, 0, 0, 0},
    {222, 222, 0, 0},
    {222, 210, 222, 222},
    {222, 222, 222, 222}
};
// Boxes height for each line
const uint8_t gui_prompts_conf_prompt_line_heights[4][4] = {
    {14, 14, 14, 14},
    {14, 14, 14, 14},
    {14, 14, 14, 14},
    {14, 14, 14, 14}
};
// Min display X for notifications
const uint8_t gui_prompts_notif_min_x[3] = {56, 40, 52};
const uint16_t gui_prompts_notif_popup_anim_length[3] = {INFO_NOTIF_ANIM_LGTH, WARNING_NOTIF_ANIM_LGTH, ACTION_NOTIF_ANIM_LGTH};
const uint16_t gui_prompts_notif_popup_anim_bitmap[3] = {BITMAP_INFO_NOTIF_POPUP_ID, BITMAP_WARNING_NOTIF_POPUP_ID, BITMAP_ACTION_NOTIF_POPUP_ID};
const uint16_t gui_prompts_notif_idle_anim_length[3] = {INFO_NOTIF_IDLE_ANIM_LGTH, WARNING_NOTIF_IDLE_ANIM_LGTH, ACTION_NOTIF_IDLE_ANIM_LGTH};
const uint16_t gui_prompts_notif_idle_anim_bitmap[3] = {BITMAP_INFO_NOTIF_IDLE_ID, BITMAP_WARNING_NOTIF_IDLE_ID, BITMAP_ACTION_NOTIF_IDLE_ID};


/*! \fn     gui_prompts_display_tutorial(void)
*   \brief  Display device tutorial
*/
void gui_prompts_display_tutorial(void)
{
    wheel_action_ret_te detection_result;
    uint16_t current_animation_index = 0;
    uint16_t current_tutorial_page = 0;
    cust_char_t* temp_string_pt;
    BOOL redraw_needed = TRUE;
    
    /* Activity detected */
    logic_device_activity_detected();
    
    /* Clear current detections */
    inputs_clear_detections();    
    
    /* Set timer */
    uint16_t temp_timer_id = timer_get_and_start_timer(30000);
    
    /* Scroll through the pages */
    while (current_tutorial_page < 4)
    {
        /* Transition into a new tutorial page */
        if (redraw_needed != FALSE)
        {
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            #endif
            sh1122_load_transition(&plat_oled_descriptor, OLED_IN_OUT_TRANS);
            sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_MEDIUM_17_ID);
            for (uint16_t i = 0; i < 3; i++)
            {
                uint16_t text_offset = (current_tutorial_page > 0 && current_tutorial_page < 3)? 30 : 0;
                custom_fs_get_string_from_file(TUTORIAL_FLINE_TEXT_ID + current_tutorial_page*3 + i, &temp_string_pt, TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, text_offset, gui_prompts_tutorial_3_lines_y_positions[i], OLED_ALIGN_CENTER, temp_string_pt, TRUE);
            }
            
            /* Display animation frame if needed */
            if (current_tutorial_page == 1)
            {
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_WHEEL_ROT, TRUE);
            }
            else if (current_tutorial_page == 2)
            {
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_WHEEL_PRESS, TRUE);
            }
            
            /* Flush frame buffer */
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_flush_frame_buffer(&plat_oled_descriptor);
            #endif
            
            /* Arm timer for possible animation */
            timer_start_timer(TIMER_ANIMATIONS, 50);
            current_animation_index = 0;
            inputs_clear_detections();
            redraw_needed = FALSE;
        }
        
        /* Animation */
        if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
        {
            if (current_tutorial_page == 1)
            {
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_WHEEL_ROT+current_animation_index, FALSE);
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_WHEEL_ROT+current_animation_index, TRUE);
                if (current_animation_index++ == BITMAP_WHEEL_ROT_NBFRAMES - 1)
                {
                    current_animation_index = 0;
                }
                timer_start_timer(TIMER_ANIMATIONS, 50);
            }
            else if (current_tutorial_page == 2)
            {
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_WHEEL_PRESS+current_animation_index, FALSE);
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_WHEEL_PRESS+current_animation_index, TRUE);
                
                /* Different timeout depending on the current frame */
                if ((current_animation_index == 0) || (current_animation_index == BITMAP_WHEEL_PRESS_NBFRAMES - 1))
                {
                    timer_start_timer(TIMER_ANIMATIONS, 500);
                }
                else
                {
                    timer_start_timer(TIMER_ANIMATIONS, 50);
                }
                
                /* Animation loop */
                if (current_animation_index++ == BITMAP_WHEEL_PRESS_NBFRAMES - 1)
                {
                    current_animation_index = 0;
                }
            }
        }
        
        /* Battery powered, no action, switch off */
        if ((timer_has_allocated_timer_expired(temp_timer_id, FALSE) == TIMER_EXPIRED) && (platform_io_is_usb_3v3_present() == FALSE))
        {
            /* Switch off OLED, switch off platform */
            platform_io_power_down_oled(); timer_delay_ms(200);
            platform_io_disable_switch_and_die();
        }
                
        /* Still process the USB commands, reply with please retries */
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        
        /* Deal with accelerometer data */
        logic_accelerometer_routine();
            
        /* Handle possible power switches */
        power_source_te before_power_source = logic_power_get_power_source();
        logic_power_check_power_switch_and_battery(FALSE);
        if (before_power_source != logic_power_get_power_source())
        {
            timer_rearm_allocated_timer(temp_timer_id, 30000);
        }
            
        /* Detection result */
        detection_result = inputs_get_wheel_action(FALSE, FALSE);
        
        /* Reset timer if action detected */
        if (detection_result != WHEEL_ACTION_NONE)
        {
            timer_rearm_allocated_timer(temp_timer_id, 30000);
        }
            
        /* Next page */
        if ((current_tutorial_page == 0) || (current_tutorial_page == 3))
        {
            if ((detection_result == WHEEL_ACTION_SHORT_CLICK) || (detection_result == WHEEL_ACTION_DOWN))
            {
                current_tutorial_page++;
                redraw_needed = TRUE;
            }
        }
        else if (current_tutorial_page == 1)
        {
            if (detection_result == WHEEL_ACTION_DOWN)
            {
                current_tutorial_page++;
                redraw_needed = TRUE;
            }
        }
        else if (current_tutorial_page == 2)
        {
            if (detection_result == WHEEL_ACTION_SHORT_CLICK)
            {
                current_tutorial_page++;
                redraw_needed = TRUE;
            }
        }
            
        /* Previous page */
        if ((current_tutorial_page > 0) && ((detection_result == WHEEL_ACTION_LONG_CLICK) || (detection_result == WHEEL_ACTION_UP)))
        {
            current_tutorial_page--;
            redraw_needed = TRUE;
        }        
    }  

    /* Free timer */
    timer_deallocate_timer(temp_timer_id);
}

/*! \fn     gui_prompts_display_information_on_screen(uint16_t string_id, display_message_te message_type)
*   \brief  Display text information on screen
*   \param  string_id   String ID to display
*   \param  message_type    Message type (see enum)
*   \note   Do not call power switch routines here as it may corrupt send buffers
*/
void gui_prompts_display_information_on_screen(uint16_t string_id, display_message_te message_type)
{
    cust_char_t* string_to_display;
    
    /* Activity detected routine */
    logic_device_activity_detected();
    
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
    sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_MEDIUM_15_ID);
    sh1122_set_min_text_x(&plat_oled_descriptor, gui_prompts_notif_min_x[message_type]);
    
    /* Count number of lines */
    uint16_t nb_lines_to_display = utils_get_nb_lines(string_to_display);
    
    /* Depending on number of lines */
    if (nb_lines_to_display == 1)
    {
        sh1122_put_centered_string(&plat_oled_descriptor, INF_DISPLAY_TEXT_Y, string_to_display, TRUE);
    } 
    else
    {
        sh1122_put_centered_string(&plat_oled_descriptor, INF_DISPLAY_2L_TEXT_Y1, string_to_display, TRUE);
        sh1122_put_centered_string(&plat_oled_descriptor, INF_DISPLAY_2L_TEXT_Y2, utils_get_string_next_line_pt(string_to_display), TRUE);
    }
    
    /* Reset display settings */
    sh1122_reset_min_text_x(&plat_oled_descriptor);
    
    /* Flush frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_flush_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* Clear current detections, allowing user to skip just after intro animation */
    inputs_clear_detections();
    
    /* Animation depending on message type */
    for (uint16_t i = 0; i < gui_prompts_notif_popup_anim_length[message_type]; i++)
    {
        timer_delay_ms(50);
        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, gui_prompts_notif_popup_anim_bitmap[message_type]+i, FALSE);
    }
    
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    /* Load transition (likely to be overwritten later */
    sh1122_load_transition(&plat_oled_descriptor, OLED_IN_OUT_TRANS);
    #endif
    
    /* Start timer in case the caller needs it */    
    timer_start_timer(TIMER_ANIMATIONS, 50);
}

/*! \fn     gui_prompts_display_information_lines_on_screen(confirmationText_t* text_lines, display_message_te message_type, uint16_t nb_lines)
*   \brief  Display text information on screen - multiple lines version
*   \param  confirmationText_t  Pointer to the struct containing the 3 lines
*   \param  message_type        Message type (see enum)
*   \param  nb_lines            Number of lines to display
*/
void gui_prompts_display_information_lines_on_screen(confirmationText_t* text_lines, display_message_te message_type, uint16_t nb_lines)
{
    /* Activity detected routine */
    logic_device_activity_detected();
    
    /* Check strings lengths and truncate them if necessary */
    sh1122_set_min_text_x(&plat_oled_descriptor, gui_prompts_notif_min_x[message_type]);
    for (uint16_t i = 0; i < nb_lines; i++)
    {
        /* Set correct font */
        sh1122_refresh_used_font(&plat_oled_descriptor, gui_prompts_conf_prompt_fonts[nb_lines-1][i]);

        /* Get string length */
        uint16_t string_length = utils_strlen(text_lines->lines[i]);
        
        /* Get how many characters can be printed */
        uint16_t nb_printable_chars = sh1122_get_number_of_printable_characters_for_string(&plat_oled_descriptor, gui_prompts_notif_min_x[message_type], text_lines->lines[i]);

        /* String too long, truncate it with "..." */
        if ((string_length != nb_printable_chars) && (nb_printable_chars > 2))
        {
            text_lines->lines[i][nb_printable_chars] = 0;
            text_lines->lines[i][nb_printable_chars-1] = '.';
            text_lines->lines[i][nb_printable_chars-2] = '.';
            text_lines->lines[i][nb_printable_chars-3] = '.';
        }

        /* Display string */
        sh1122_put_centered_string(&plat_oled_descriptor, gui_prompts_conf_prompt_y_positions[nb_lines-1][i], text_lines->lines[i], TRUE);
    }
    sh1122_reset_min_text_x(&plat_oled_descriptor);

    /* Clear frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_load_transition(&plat_oled_descriptor, OLED_OUT_IN_TRANS);
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #else
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #endif
    
    /* Check strings length and truncate them if necessary */
    sh1122_set_min_text_x(&plat_oled_descriptor, gui_prompts_notif_min_x[message_type]);
    for (uint16_t i = 0; i < nb_lines; i++)
    {
        /* Set correct font */
        sh1122_refresh_used_font(&plat_oled_descriptor, gui_prompts_conf_prompt_fonts[nb_lines-1][i]);

        /* Display string */
        sh1122_put_centered_string(&plat_oled_descriptor, gui_prompts_conf_prompt_y_positions[nb_lines-1][i], text_lines->lines[i], TRUE);
    }
    sh1122_reset_min_text_x(&plat_oled_descriptor);
    
    /* Flush frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_flush_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* Clear current detections, allowing user to skip just after intro animation */
    inputs_clear_detections();
    
    /* Animation depending on message type */
    for (uint16_t i = 0; i < gui_prompts_notif_popup_anim_length[message_type]; i++)
    {
        timer_delay_ms(50);
        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, gui_prompts_notif_popup_anim_bitmap[message_type]+i, FALSE);
    }
    
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    /* Load transition (likely to be overwritten later */
    sh1122_load_transition(&plat_oled_descriptor, OLED_IN_OUT_TRANS);
    #endif
}

/*! \fn     gui_prompts_display_information_on_string_single_anim_frame(uint16_t* frame_id, uint16_t* timer_timeout, display_message_te message_type)
*   \brief  Display a single frame of the animation for a given message
*   \param  frame_id        Pointer to the frame ID, function automatically increments and loops
*   \param  timer_timeout   Pointer to a future animation timer value to setup after calling function
*   \param  message_type    Message type (see enum)
*/
void gui_prompts_display_information_on_string_single_anim_frame(uint16_t* frame_id, uint16_t* timer_timeout, display_message_te message_type)
{
    /* Display correct frame */
    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, gui_prompts_notif_idle_anim_bitmap[message_type]+(*frame_id), FALSE);
    
    /* Increment frame ID */
    if ((*frame_id)++ == gui_prompts_notif_idle_anim_length[message_type]-1)
    {
        *frame_id = 0;
    }
    
    /* Store new timeout */
    *timer_timeout = 50;    
}

/*! \fn     gui_prompts_display_information_on_screen_and_wait(uint16_t string_id, display_message_te message_type, BOOL allow_scroll_or_msg_to_interrupt)
*   \brief  Display text information on screen
*   \param  string_id                           String ID to display
*   \param  message_type                        Message type (see enum)
*   \param  allow_scroll_or_msg_to_interrupt    Boolean to allow scrolling or message to interrupt the notification
*   \return What caused the function to return (see enum)
*/
gui_info_display_ret_te gui_prompts_display_information_on_screen_and_wait(uint16_t string_id, display_message_te message_type, BOOL allow_scroll_or_msg_to_interrupt)
{
    wheel_action_ret_te wheel_return;
    uint16_t i = 0;
    
    /* Store current smartcard inserted state */
    ret_type_te card_absent = smartcard_low_level_is_smc_absent();
    
    /* Display string */
    gui_prompts_display_information_on_screen(string_id, message_type);
    
    /* Optional wait */
    timer_start_timer(TIMER_ANIMATIONS, 50);
    uint16_t temp_timer_id = timer_get_and_start_timer(3000);
    while ((timer_has_allocated_timer_expired(temp_timer_id, FALSE) != TIMER_EXPIRED) || (i != gui_prompts_notif_idle_anim_length[message_type]-1))
    {
        /* Deal with incoming messages but do not deal with them */
        comms_msg_rcvd_te rcvd_message = comms_aux_mcu_routine(MSG_RESTRICT_ALL); 
        
        /* Did we receive a message worthy of stopping the animation? */
        if ((allow_scroll_or_msg_to_interrupt != FALSE) && (rcvd_message != NO_MSG_RCVD) && (rcvd_message != EVENT_MSG_RCVD) && (rcvd_message != HID_DBG_MSG_RCVD))
        {
            /* Free timer */
            timer_deallocate_timer(temp_timer_id);
            
            return GUI_INFO_DISP_RET_SCROLL_OR_MSG;
        }
        
        /* Accelerometer routine for RNG stuff */
        logic_accelerometer_routine();
        
        /* Handle possible power switches */
        logic_power_check_power_switch_and_battery(FALSE); 
        
        /* Click to interrupt */  
        wheel_return = inputs_get_wheel_action(FALSE, FALSE);
        if (wheel_return == WHEEL_ACTION_SHORT_CLICK)
        {
            /* Free timer */
            timer_deallocate_timer(temp_timer_id);
            
            return GUI_INFO_DISP_RET_CLICK;
        }
        else if (wheel_return == WHEEL_ACTION_LONG_CLICK)
        {
            /* Free timer */
            timer_deallocate_timer(temp_timer_id);
            
            return GUI_INFO_DISP_RET_LONG_CLICK;
        }
        else if ((allow_scroll_or_msg_to_interrupt != FALSE) && ((wheel_return == WHEEL_ACTION_UP) || (wheel_return == WHEEL_ACTION_DOWN)))
        {
            /* Free timer */
            timer_deallocate_timer(temp_timer_id);
            
            return GUI_INFO_DISP_RET_SCROLL_OR_MSG;
        }
        
        /* Card insertion status change */
        if (card_absent != smartcard_low_level_is_smc_absent())
        {
            /* Free timer */
            timer_deallocate_timer(temp_timer_id);
            
            return GUI_INFO_DISP_RET_CARD_CHANGE;
        }
        
        /* Animation timer */
        if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
        {
            /* Rearm timer, increment current bitmap */
            timer_start_timer(TIMER_ANIMATIONS, 50);
            if (i++ == gui_prompts_notif_idle_anim_length[message_type]-1)
            {
                i = 0;
            }
            
            /* Animation depending on message type */
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, gui_prompts_notif_idle_anim_bitmap[message_type]+i, FALSE);                 
        }
    }
    
    /* Free timer */
    timer_deallocate_timer(temp_timer_id);
    
    /* Normal animation timeout */
    return GUI_INFO_DISP_RET_OK;
}

/*! \fn     gui_prompts_wait_for_pairing_screen(void)
*   \brief  Waiting screen for pairing
*   \return What caused the function to return (see enum)
*/
gui_info_display_ret_te gui_prompts_wait_for_pairing_screen(void)
{
    wheel_action_ret_te wheel_return;
    uint16_t i = 0;
    
    /* Store current smartcard inserted state */
    ret_type_te card_absent = smartcard_low_level_is_smc_absent();
    
    /* Display string */
    gui_prompts_display_information_on_screen(PAIRING_WAIT_TEXT_ID, DISP_MSG_ACTION);
    
    /* Optional wait */
    timer_start_timer(TIMER_ANIMATIONS, 50);
    uint16_t temp_timer_id = timer_get_and_start_timer(30000);
    while (timer_has_allocated_timer_expired(temp_timer_id, TRUE) != TIMER_EXPIRED)
    {
        comms_msg_rcvd_te received_packet = comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_BOND_STORE);
        if (received_packet == BLE_BOND_STORE_RCVD)
        {
            /* Free timer */
            timer_deallocate_timer(temp_timer_id);
            
            /* We received a bonding storage message */
            return GUI_INFO_DISP_RET_BLE_PAIRED;
        }
        else if (received_packet == BLE_6PIN_REQ_RCVD)
        {
            /* Received a request to get 6 digits pin */
            gui_prompts_display_information_on_screen(PAIRING_WAIT_TEXT_ID, DISP_MSG_ACTION);     
            
            /* Add an extra 10 seconds for pairing to finish */
            timer_rearm_allocated_timer(temp_timer_id, 10000);   
        }
        
        /* Accelerometer stuff */
        logic_accelerometer_routine();
        
        /* Handle possible power switches */
        logic_power_check_power_switch_and_battery(FALSE);
        
        /* Click to interrupt */  
        wheel_return = inputs_get_wheel_action(FALSE, FALSE);
        if (wheel_return == WHEEL_ACTION_LONG_CLICK)
        {
            /* Free timer */
            timer_deallocate_timer(temp_timer_id);
            
            return GUI_INFO_DISP_RET_LONG_CLICK;
        }
        
        /* Card insertion status change */
        if (card_absent != smartcard_low_level_is_smc_absent())
        {
            /* Free timer */
            timer_deallocate_timer(temp_timer_id);
            
            return GUI_INFO_DISP_RET_CARD_CHANGE;
        }
        
        /* Animation timer */
        if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
        {
            /* Rearm timer, increment current bitmap */
            timer_start_timer(TIMER_ANIMATIONS, 50);
            if (i++ == gui_prompts_notif_idle_anim_length[DISP_MSG_ACTION]-1)
            {
                i = 0;
            }
            
            /* Animation depending on message type */
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, gui_prompts_notif_idle_anim_bitmap[DISP_MSG_ACTION]+i, FALSE);                 
        }
    }
    
    /* Free timer */
    timer_deallocate_timer(temp_timer_id);

    /* Normal animation timeout */
    return GUI_INFO_DISP_RET_OK;
}

/*! \fn     gui_prompts_display_3line_information_on_screen(confirmationText_t* text_lines, display_message_te message_type
*   \brief  Display text information on screen - 3 lines version
*   \param  confirmationText_t  Pointer to the struct containing the 3 lines
*   \param  message_type        Message type (see enum)
*/
void gui_prompts_display_3line_information_on_screen_and_wait(confirmationText_t* text_lines, display_message_te message_type)
{
    uint16_t i = 0;
    
    // Display string
    gui_prompts_display_information_lines_on_screen(text_lines, message_type, 3);
    
    /* Optional wait */
    timer_start_timer(TIMER_ANIMATIONS, 50);
    uint16_t temp_timer_id = timer_get_and_start_timer(3000);
    while ((timer_has_allocated_timer_expired(temp_timer_id, TRUE) != TIMER_EXPIRED) && (inputs_get_wheel_action(FALSE, FALSE) != WHEEL_ACTION_SHORT_CLICK))
    {
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        logic_accelerometer_routine();
        
        /* Animation timer */
        if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
        {
            /* Rearm timer, increment current bitmap */
            timer_start_timer(TIMER_ANIMATIONS, 50);
            if (i++ == gui_prompts_notif_idle_anim_length[message_type]-1)
            {
                i = 0;
            }
            
            /* Animation depending on message type */
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, gui_prompts_notif_idle_anim_bitmap[message_type]+i, FALSE);
        }
    }

    /* Free timer */
    timer_deallocate_timer(temp_timer_id);
}

/*! \fn     gui_prompts_get_char_to_display(uint8_t const * current_pin, uint8_t index)
*   \brief  Get the next PIN value to display or '*' if PIN should be hidden.
*           Called whenever a non selected PIN digit needs to be displayed.
*   \param  current_pin         Array containing the pin
*   \param  index               Index into current_pin array
*   \return Character to display
*/
static cust_char_t gui_prompts_get_char_to_display(uint8_t const * current_pin, uint8_t index)
{
    cust_char_t disp_char;

    if ((BOOL)custom_fs_settings_get_device_setting(SETTINGS_SHOW_PIN_ON_ENTRY) != FALSE)
    {
        if (current_pin[index] >= 0x0A)
        {
            disp_char = current_pin[index] + u'A' - 0x0A;
        }
        else
        {
            disp_char = current_pin[index] + u'0';
        }
    }
    else
    {
        // Display '*'
        disp_char = u'*';
    }

    return disp_char;
}

/*! \fn     gui_prompts_render_pin_enter_screen(uint8_t* current_pin, uint16_t selected_digit, uint16_t stringID, int16_t anim_direction, int16_t vert_anim_direction, int16_t hor_anim_direction, BOOL six_digit_prompt)
*   \brief  Overwrite the digits on the current pin entering screen
*   \param  current_pin         Array containing the pin
*   \param  selected_digit      Currently selected digit
*   \param  stringID            String ID for text query
*   \param  vert_anim_direction Vertical anim direction (wheel up or down)
*   \param  hor_anim_direction  Horizontal anim direction (next/previous digit)
*   \param  six_digit_prompt    TRUE to specify 6 digits prompt
*   \return A wheel action if there was one during the animation
*/
wheel_action_ret_te gui_prompts_render_pin_enter_screen(uint8_t* current_pin, uint16_t selected_digit, uint16_t stringID, int16_t vert_anim_direction, int16_t hor_anim_direction, BOOL six_digit_prompt)
{
    uint16_t x_offset_for_digits = six_digit_prompt == FALSE? PIN_PROMPT_OFFSET_FOR_FOUR_DIG:0;
    wheel_action_ret_te wheel_action_ret = WHEEL_ACTION_NONE;
    uint16_t nb_of_digits = six_digit_prompt == FALSE? 4:6;
    cust_char_t sig_digits_cust_chars[6];
    cust_char_t* string_to_display;
    
    /* Try to fetch the string to display */
    custom_fs_get_string_from_file(stringID, &string_to_display, TRUE);
    
    /* Vertical animation: get current digit and the next one */
    int16_t next_digit = current_pin[selected_digit] + vert_anim_direction;
    if (six_digit_prompt == FALSE)
    {
        if (next_digit == 0x10)
        {
            next_digit = 0;
        }
        else if (next_digit < 0)
        {
            next_digit = 0x0F;
        }
    } 
    else
    {
        if (next_digit == 10)
        {
            next_digit = 0;
        }
        else if (next_digit < 0)
        {
            next_digit = 9;
        }
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
    
    /* Six digits prompts: convert digits into chars */
    if (six_digit_prompt != FALSE)
    {
        for (uint16_t i = 0; i < nb_of_digits; i++)
        {
            sig_digits_cust_chars[i] = current_pin[i] + u'0';
        }
    }
    
    /* Number of animation steps */
    int16_t nb_animation_steps = PIN_PROMPT_DIGIT_HEIGHT + 2;
    if (vert_anim_direction == 0)
    {
        nb_animation_steps = 1;
    }
    
    /* Clear detections */
    inputs_clear_detections();
    
    /* Clear frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #else
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #endif
    
    /* Write prompt text, centered on the left part */
    sh1122_allow_line_feed(&plat_oled_descriptor);
    uint16_t nb_lines_to_display = utils_get_nb_lines(string_to_display);
    sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_MEDIUM_15_ID);
    sh1122_set_max_text_x(&plat_oled_descriptor, PIN_PROMPT_MAX_TEXT_X+x_offset_for_digits);
    if (nb_lines_to_display == 1)
    {
        sh1122_put_string_xy(&plat_oled_descriptor, PIN_PROMPT_MAX_TEXT_X+x_offset_for_digits-1, PIN_PROMPT_1LTEXT_Y, OLED_ALIGN_RIGHT, string_to_display, TRUE);
    }
    else if (nb_lines_to_display == 2)
    {
        sh1122_put_string_xy(&plat_oled_descriptor, PIN_PROMPT_MAX_TEXT_X+x_offset_for_digits-1, PIN_PROMPT_2LTEXT_Y, OLED_ALIGN_RIGHT, string_to_display, TRUE);
    }
    else
    {
        sh1122_put_string_xy(&plat_oled_descriptor, PIN_PROMPT_MAX_TEXT_X+x_offset_for_digits-1, PIN_PROMPT_3LTEXT_Y, OLED_ALIGN_RIGHT, string_to_display, TRUE);
    }    
    sh1122_prevent_line_feed(&plat_oled_descriptor);
    sh1122_reset_max_text_x(&plat_oled_descriptor);
    
    /* Horizontal animation when changing selected digit */
    if (hor_anim_direction != 0)
    {
        /* Erase digits */
        sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_MONO_BOLD_30_ID);
        sh1122_draw_rectangle(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS+x_offset_for_digits, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+PIN_PROMPT_DIGIT_Y_SPACING, SH1122_OLED_WIDTH-PIN_PROMPT_DIGIT_X_OFFS-x_offset_for_digits, PIN_PROMPT_DIGIT_HEIGHT, 0x00, TRUE);
        if (six_digit_prompt == FALSE)
        {
            for (uint16_t i = 0; i < nb_of_digits; i++)
            {
                cust_char_t disp_char = gui_prompts_get_char_to_display(current_pin, i);
                sh1122_put_centered_char(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits + PIN_PROMPT_DIGIT_X_SPC*i + PIN_PROMPT_DIGIT_X_ADJ, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+PIN_PROMPT_DIGIT_Y_SPACING+PIN_PROMPT_ASTX_Y_INC, disp_char, TRUE);
            }
        } 
        else
        {
            for (uint16_t i = 0; i < nb_of_digits; i++)
            {
                sh1122_put_centered_char(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits + PIN_PROMPT_DIGIT_X_SPC*i + PIN_PROMPT_DIGIT_X_ADJ, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+PIN_PROMPT_DIGIT_Y_SPACING, sig_digits_cust_chars[i], TRUE);
            }
        }
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_flush_frame_buffer_window(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+PIN_PROMPT_DIGIT_Y_SPACING, SH1122_OLED_WIDTH-PIN_PROMPT_DIGIT_X_OFFS-x_offset_for_digits, PIN_PROMPT_DIGIT_HEIGHT);
        #endif
        
        for (uint16_t i = 0; i < PIN_PROMPT_ARROW_MOV_LGTH; i++)
        {        
            sh1122_draw_rectangle(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits, PIN_PROMPT_UP_ARROW_Y, SH1122_OLED_WIDTH-PIN_PROMPT_DIGIT_X_OFFS - x_offset_for_digits, PIN_PROMPT_ARROW_HEIGHT, 0x00, TRUE);
            sh1122_draw_rectangle(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+2*PIN_PROMPT_DIGIT_Y_SPACING+PIN_PROMPT_DIGIT_HEIGHT, SH1122_OLED_WIDTH-PIN_PROMPT_DIGIT_X_OFFS-x_offset_for_digits, PIN_PROMPT_ARROW_HEIGHT, 0x00, TRUE);
            sh1122_display_bitmap_from_flash(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits + PIN_PROMPT_DIGIT_X_SPC*(selected_digit-hor_anim_direction) + PIN_PROMPT_ARROW_HOR_ANIM_STEP*i*hor_anim_direction, PIN_PROMPT_UP_ARROW_Y, BITMAP_PIN_UP_ARROW_MOVE_ID+i, TRUE);
            sh1122_display_bitmap_from_flash(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits + PIN_PROMPT_DIGIT_X_SPC*(selected_digit-hor_anim_direction) + PIN_PROMPT_ARROW_HOR_ANIM_STEP*i*hor_anim_direction, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+2*PIN_PROMPT_DIGIT_Y_SPACING+PIN_PROMPT_DIGIT_HEIGHT, BITMAP_PIN_DN_ARROW_MOVE_ID+i, TRUE);
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_flush_frame_buffer_window(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits, PIN_PROMPT_UP_ARROW_Y, SH1122_OLED_WIDTH-PIN_PROMPT_DIGIT_X_OFFS-x_offset_for_digits, PIN_PROMPT_ARROW_HEIGHT);
            sh1122_flush_frame_buffer_window(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+2*PIN_PROMPT_DIGIT_Y_SPACING+PIN_PROMPT_DIGIT_HEIGHT, SH1122_OLED_WIDTH-PIN_PROMPT_DIGIT_X_OFFS-x_offset_for_digits, PIN_PROMPT_ARROW_HEIGHT);
            #endif
            timer_delay_ms(20);            
        }
        
        /* Erase digits again */
        sh1122_draw_rectangle(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS+x_offset_for_digits, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+PIN_PROMPT_DIGIT_Y_SPACING, SH1122_OLED_WIDTH-PIN_PROMPT_DIGIT_X_OFFS-x_offset_for_digits, PIN_PROMPT_DIGIT_HEIGHT, 0x00, TRUE);
    }
    
    /* Prepare for digits display */
    sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_MONO_BOLD_30_ID);
    sh1122_allow_partial_text_y_draw(&plat_oled_descriptor);
    
    /* Animation code */
    for (int16_t anim_step = 0; anim_step < nb_animation_steps; anim_step+=2)
    {
        /* Wait for a possible ongoing previous flush */
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_check_for_flush_and_terminate(&plat_oled_descriptor);
        #endif
        
        /* Erase overwritten part */
        sh1122_draw_rectangle(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS+x_offset_for_digits, 0, SH1122_OLED_WIDTH-PIN_PROMPT_DIGIT_X_OFFS-x_offset_for_digits, SH1122_OLED_HEIGHT, 0, TRUE);
        
        /* Display the 4-6 digits */
        for (uint16_t i = 0; i < nb_of_digits; i++)
        {
            if (i != selected_digit)
            {
                if (six_digit_prompt == FALSE)
                {
                    cust_char_t disp_char = gui_prompts_get_char_to_display(current_pin, i);
                    sh1122_put_centered_char(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits + PIN_PROMPT_DIGIT_X_SPC*i + PIN_PROMPT_DIGIT_X_ADJ, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+PIN_PROMPT_DIGIT_Y_SPACING+PIN_PROMPT_ASTX_Y_INC, disp_char, TRUE);
                } 
                else
                {
                    sh1122_put_centered_char(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits + PIN_PROMPT_DIGIT_X_SPC*i + PIN_PROMPT_DIGIT_X_ADJ, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+PIN_PROMPT_DIGIT_Y_SPACING, sig_digits_cust_chars[i], TRUE);
                }
            }
            else
            {
                /* Arrows potential animations for scrolling digits */
                if ((hor_anim_direction != 0) || (vert_anim_direction != 0))
                {
                    if (vert_anim_direction > 0)
                    {
                        sh1122_display_bitmap_from_flash(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits + PIN_PROMPT_DIGIT_X_SPC*i, PIN_PROMPT_UP_ARROW_Y, BITMAP_PIN_UP_ARROW_ACTIVATE_ID+(anim_step-1)/2, TRUE);                        
                    }
                    else
                    {
                        sh1122_display_bitmap_from_flash(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits + PIN_PROMPT_DIGIT_X_SPC*i, PIN_PROMPT_UP_ARROW_Y, BITMAP_PIN_UP_ARROW_POP_ID+PIN_PROMPT_POPUP_ANIM_LGTH-1, TRUE);                        
                    }                    
                    if (vert_anim_direction < 0)
                    {
                        sh1122_display_bitmap_from_flash(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits + PIN_PROMPT_DIGIT_X_SPC*i, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+2*PIN_PROMPT_DIGIT_Y_SPACING+PIN_PROMPT_DIGIT_HEIGHT, BITMAP_PIN_DN_ARROW_ACTIVATE_ID+(anim_step-1)/2, TRUE);
                    }
                    else
                    {
                        sh1122_display_bitmap_from_flash(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits + PIN_PROMPT_DIGIT_X_SPC*i, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+2*PIN_PROMPT_DIGIT_Y_SPACING+PIN_PROMPT_DIGIT_HEIGHT, BITMAP_PIN_DN_ARROW_POP_ID+PIN_PROMPT_POPUP_ANIM_LGTH-1, TRUE);
                    }
                }
                
                /* Digits display with animation */
                sh1122_set_min_display_y(&plat_oled_descriptor, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+PIN_PROMPT_DIGIT_Y_SPACING);
                sh1122_set_max_display_y(&plat_oled_descriptor, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+PIN_PROMPT_DIGIT_Y_SPACING+PIN_PROMPT_DIGIT_HEIGHT);
                sh1122_put_centered_char(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits + PIN_PROMPT_DIGIT_X_SPC*i + PIN_PROMPT_DIGIT_X_ADJ, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+PIN_PROMPT_DIGIT_Y_SPACING + anim_step*vert_anim_direction,  current_char, TRUE);
                sh1122_put_centered_char(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits + PIN_PROMPT_DIGIT_X_SPC*i + PIN_PROMPT_DIGIT_X_ADJ, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+PIN_PROMPT_DIGIT_Y_SPACING + anim_step*vert_anim_direction + (PIN_PROMPT_DIGIT_HEIGHT+1)*vert_anim_direction*-1, next_char, TRUE);
                sh1122_reset_lim_display_y(&plat_oled_descriptor);
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
            sh1122_flush_frame_buffer_window(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS + x_offset_for_digits, PIN_PROMPT_UP_ARROW_Y, SH1122_OLED_WIDTH-PIN_PROMPT_DIGIT_X_OFFS-x_offset_for_digits, 2*PIN_PROMPT_ARROW_HEIGHT+2*PIN_PROMPT_DIGIT_Y_SPACING+PIN_PROMPT_DIGIT_HEIGHT);
        }
        
        /* Skip animation if desired */
        wheel_action_ret = inputs_get_wheel_action(FALSE, FALSE);
        if ((wheel_action_ret != WHEEL_ACTION_NONE) && ((hor_anim_direction != 0) || (vert_anim_direction != 0)))
        {
            memset(sig_digits_cust_chars, 0, sizeof(sig_digits_cust_chars));
            sh1122_prevent_partial_text_y_draw(&plat_oled_descriptor);
            return wheel_action_ret;
        }
        #endif
    }
        
    /* Reset temporary graphics settings */
    sh1122_prevent_partial_text_y_draw(&plat_oled_descriptor);
    
    /* Special case: first display of pin entering screen, arrows appearing */
    if ((hor_anim_direction == 0) && (vert_anim_direction == 0))
    {
        for (uint16_t i = 0; i < PIN_PROMPT_POPUP_ANIM_LGTH; i++)
        {
            sh1122_display_bitmap_from_flash(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS+x_offset_for_digits, PIN_PROMPT_UP_ARROW_Y, BITMAP_PIN_UP_ARROW_POP_ID+i, FALSE);
            sh1122_display_bitmap_from_flash(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS+x_offset_for_digits, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+2*PIN_PROMPT_DIGIT_Y_SPACING+PIN_PROMPT_DIGIT_HEIGHT, BITMAP_PIN_DN_ARROW_POP_ID+i, FALSE);
            timer_delay_ms(30);
        }
        
        /* Rewrite the last bitmaps in the frame buffer in case we have a power change */
        sh1122_display_bitmap_from_flash(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS+x_offset_for_digits, PIN_PROMPT_UP_ARROW_Y, BITMAP_PIN_UP_ARROW_POP_ID+PIN_PROMPT_POPUP_ANIM_LGTH-1, TRUE);
        sh1122_display_bitmap_from_flash(&plat_oled_descriptor, PIN_PROMPT_DIGIT_X_OFFS+x_offset_for_digits, PIN_PROMPT_UP_ARROW_Y+PIN_PROMPT_ARROW_HEIGHT+2*PIN_PROMPT_DIGIT_Y_SPACING+PIN_PROMPT_DIGIT_HEIGHT, BITMAP_PIN_DN_ARROW_POP_ID+PIN_PROMPT_POPUP_ANIM_LGTH-1, TRUE);
    }    
    
    memset(sig_digits_cust_chars, 0, sizeof(sig_digits_cust_chars));
    return wheel_action_ret;
}


/*! \fn     gui_prompts_get_six_digits_pin(uint8_t* pin_code, uint16_t stringID)
*   \brief  Ask the user to enter a six digits PIN
*   \param  pin_code    Pointer to where to store the pin code
*   \param  stringID    String ID
*   \return If the user approved the request
*/
RET_TYPE gui_prompts_get_six_digits_pin(uint8_t* pin_code, uint16_t stringID)
{
    wheel_action_ret_te detection_during_animation = WHEEL_ACTION_NONE;
    wheel_action_ret_te detection_result = WHEEL_ACTION_NONE;
    RET_TYPE ret_val = RETURN_NOK;
    uint16_t selected_digit = 0;
    BOOL finished = FALSE;
    
    // Set current pin to 000000
    memset((void*)pin_code, 0, 6);
    
    /* Activity detected */
    logic_device_activity_detected();
    
    // Clear current detections
    inputs_clear_detections();
    
    // Display current pin on screen
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_load_transition(&plat_oled_descriptor, OLED_OUT_IN_TRANS);
    #endif
    gui_prompts_render_pin_enter_screen(pin_code, selected_digit, stringID, 0, 0, TRUE);
    
    // While the user hasn't entered his pin
    while(!finished)
    {
        // Still process the USB commands, reply with please retries
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        logic_accelerometer_routine();
        
        /* Handle possible power switches */
        logic_power_check_power_switch_and_battery(FALSE);
        
        // detection result
        detection_result = inputs_get_wheel_action(FALSE, FALSE);
        
        // was there a detection during the animation?
        if (detection_during_animation != WHEEL_ACTION_NONE)
        {
            detection_result = detection_during_animation;
            detection_during_animation = WHEEL_ACTION_NONE;
        }

        /* Transform click up / click down to click */
        if ((detection_result == WHEEL_ACTION_CLICK_UP) || (detection_result == WHEEL_ACTION_CLICK_DOWN))
        {
            detection_result = WHEEL_ACTION_SHORT_CLICK;
        }
        
        // Position increment / decrement
        if ((detection_result == WHEEL_ACTION_UP) || (detection_result == WHEEL_ACTION_DOWN))
        {
            if (detection_result == WHEEL_ACTION_UP)
            {
                detection_during_animation = gui_prompts_render_pin_enter_screen(pin_code, selected_digit, stringID, 1, 0, TRUE);
                if (pin_code[selected_digit]++ == 9)
                {
                    pin_code[selected_digit] = 0;
                }
            }
            else
            {
                detection_during_animation = gui_prompts_render_pin_enter_screen(pin_code, selected_digit, stringID, -1, 0, TRUE);
                if (pin_code[selected_digit]-- == 0)
                {
                    pin_code[selected_digit] = 9;
                }
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
                detection_during_animation = gui_prompts_render_pin_enter_screen(pin_code, --selected_digit, stringID, 0, -1, TRUE);
            }
            else
            {
                ret_val = RETURN_NOK;
                finished = TRUE;
            }
        }
        else if (detection_result == WHEEL_ACTION_SHORT_CLICK)
        {
            if (selected_digit < 5)
            {
                selected_digit++;
                gui_prompts_render_pin_enter_screen(pin_code, selected_digit, stringID, 0, 1, TRUE);
            }
            else
            {
                ret_val = RETURN_OK;
                finished = TRUE;
            }
        }
    }
    
    // Return success status
    return ret_val;
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
    wheel_action_ret_te detection_during_animation = WHEEL_ACTION_NONE;
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
    
    /* Activity detected */
    logic_device_activity_detected();
    
    // Clear current detections
    inputs_clear_detections();
    
    // Display current pin on screen
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_load_transition(&plat_oled_descriptor, OLED_OUT_IN_TRANS);
    #endif
    gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID, 0, 0, FALSE);
    
    // While the user hasn't entered his pin
    while(!finished)
    {
        // Still process the USB commands, reply with please retries
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        logic_accelerometer_routine();
        
        /* Handle possible power switches */
        logic_power_check_power_switch_and_battery(FALSE);
        
        // detection result
        detection_result = inputs_get_wheel_action(FALSE, FALSE);
        
        // was there a detection during the animation?
        if (detection_during_animation != WHEEL_ACTION_NONE)
        {
            detection_result = detection_during_animation;
            detection_during_animation = WHEEL_ACTION_NONE;
        }

        /* Transform click up / click down to click */
        if ((detection_result == WHEEL_ACTION_CLICK_UP) || (detection_result == WHEEL_ACTION_CLICK_DOWN))
        {
            detection_result = WHEEL_ACTION_SHORT_CLICK;
        }
        
        // Position increment / decrement
        if ((detection_result == WHEEL_ACTION_UP) || (detection_result == WHEEL_ACTION_DOWN))
        {
            if (detection_result == WHEEL_ACTION_UP)
            {
                detection_during_animation = gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID, 1, 0, FALSE);
                if (current_pin[selected_digit]++ == 0x0F)
                {
                    current_pin[selected_digit] = 0;
                }
            }
            else
            {
                detection_during_animation = gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID, -1, 0, FALSE);
                if (current_pin[selected_digit]-- == 0)
                {
                    current_pin[selected_digit] = 0x0F;
                }
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
                if (random_pin_feature_enabled != FALSE)
                {
                    rng_fill_array(&current_pin[selected_digit-1], 2);
                    current_pin[selected_digit] &= 0x0F;
                    current_pin[--selected_digit] &= 0x0F;
                }
                else if ((BOOL)custom_fs_settings_get_device_setting(SETTINGS_PIN_SHOWN_WHEN_BACK) == FALSE)
                {
                    // If enabled, when going back set pin digit to 0
                    current_pin[selected_digit] = 0;
                    current_pin[--selected_digit] = 0;
                }
                else
                {
                    --selected_digit;
                }
                detection_during_animation = gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID, 0, -1, FALSE);
            }
            else
            {
                ret_val = RETURN_NOK;
                finished = TRUE;
            }
        }
        else if (detection_result == WHEEL_ACTION_SHORT_CLICK)
        {
            if (selected_digit < 3)
            {
                selected_digit++;
                gui_prompts_render_pin_enter_screen(current_pin, selected_digit, stringID, 0, 1, FALSE);
            }
            else
            {
                ret_val = RETURN_OK;
                finished = TRUE;
            }
        }
    }
    
    // Store the pin
    *pin_code = (uint16_t)(((uint16_t)(current_pin[0]) << 12) | (((uint16_t)current_pin[1]) << 8) | (current_pin[2] << 4) | current_pin[3]);
    
    // Set current pin to 0000 & set default font
    memset((void*)current_pin, 0, sizeof(current_pin));
    
    // Return success status
    return ret_val;
}

/*! \fn     gui_prompts_ask_for_one_line_confirmation(uint16_t string_id, BOOL accept_cancel_message, BOOL usb_ble_prompt, BOOL first_item_selected)
*   \brief  Ask for user confirmation for different things
*   \param  string_id               String ID
*   \param  accept_cancel_message   Boolean to accept the cancel message to cancel prompt
*   \param  usb_ble_prompt          Set to TRUE to get BLE/USB icons, FALSE for yes/no
*   \param  first_item_selected     Set to TRUE to select first option by default
*   \return See enum
*/
mini_input_yes_no_ret_te gui_prompts_ask_for_one_line_confirmation(uint16_t string_id, BOOL accept_cancel_message, BOOL usb_ble_prompt, BOOL first_item_selected)
{
    uint16_t bitmap_yes_no_array[10] = {BITMAP_POPUP_2LINES_Y, BITMAP_POPUP_2LINES_N, BITMAP_2LINES_PRESS_Y, BITMAP_2LINES_PRESS_N, BITMAP_2LINES_SEL_Y, BITMAP_2LINES_SEL_N, BITMAP_2LINES_IDLE_Y, BITMAP_2LINES_IDLE_N, BITMAP_POPUP_2LINES_Y_DESEL, BITMAP_POPUP_2LINES_N_SELEC};
    uint16_t bitmap_usb_ble_array[10] = {BITMAP_POPUP_USB, BITMAP_POPUP_BLE, BITMAP_USB_PRESS, BITMAP_BLE_PRESS, BITMAP_USB_SELECT, BITMAP_BLE_SELECT, BITMAP_USB_IDLE, BITMAP_BLE_IDLE, BITMAP_POPUP_USB_DESEL, BITMAP_POPUP_BLE_SEL};
    ret_type_te current_card_insertion_status = smartcard_low_level_is_smc_absent();
    BOOL approve_selected = first_item_selected;
    cust_char_t* string_to_display;
    BOOL flash_flag = FALSE;
    uint16_t flash_sm = 0;
    
    /* Set array pointer depending on prompt type */
    uint16_t* bitmap_array_pt = &bitmap_yes_no_array[0];
    if (usb_ble_prompt != FALSE)
    {
        bitmap_array_pt = &bitmap_usb_ble_array[0];
    }
    
    /* Try to fetch the string to display */
    custom_fs_get_string_from_file(string_id, &string_to_display, TRUE);
    
    /* Check the user hasn't disabled the flash screen feature */
    if ((BOOL)custom_fs_settings_get_device_setting(SETTING_FLASH_SCREEN_ID) != FALSE)
    {
        flash_flag = TRUE;
    }
    
    /* Activity detected */
    logic_device_activity_detected();
    
    /* Clear frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    if (flash_flag != FALSE)
    {
        sh1122_load_transition(&plat_oled_descriptor, OLED_OUT_IN_TRANS);
    } 
    else
    {
        sh1122_load_transition(&plat_oled_descriptor, OLED_TRANS_NONE);
    }
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
    
    /* Load transition for function exit (likely to be overwritten later */
    sh1122_load_transition(&plat_oled_descriptor, OLED_IN_OUT_TRANS);
    #endif
    
    /* Clear possible remaining detection */
    inputs_clear_detections();
    
    /* Transition the action bitmap */
    for (uint16_t i = (flash_flag == FALSE)?POPUP_2LINES_ANIM_LGTH-1:0; i < POPUP_2LINES_ANIM_LGTH; i++)
    {
        /* Write both in frame buffer and display */
        if (first_item_selected != FALSE)
        {
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[0]+i, FALSE);
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[0]+i, TRUE);
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[1]+i, FALSE);
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[1]+i, TRUE);
        } 
        else
        {
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[8]+i, FALSE);
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[8]+i, TRUE);
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[9]+i, FALSE);
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[9]+i, TRUE);
        }          
        timer_delay_ms(15);
    }
    
    /* Wait for user input */
    mini_input_yes_no_ret_te input_answer = MINI_INPUT_RET_NONE;
    wheel_action_ret_te detect_result;

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
        
        // Escape the prompt if the function was entered with a smartcard inserted and the smartcard got removed
        if ((smartcard_low_level_is_smc_absent() != current_card_insertion_status) && (smartcard_low_level_is_smc_absent() == RETURN_OK))
        {
            input_answer = MINI_INPUT_RET_CARD_REMOVED;
        }
        
        // Call accelerometer routine for (among others) RNG stuff
        logic_accelerometer_routine();
        
        /* Get power state before entering the next routine */
        power_source_te before_power_source = logic_power_get_power_source();
        
        /* Handle possible power switches */
        logic_power_check_power_switch_and_battery(FALSE);
        
        /* Power source changed and we're asking the user to change left / right hand mode, return directly to not create any confusing */
        if ((before_power_source != logic_power_get_power_source()) && ((string_id == QPROMPT_LEFT_HAND_MODE_TEXT_ID) || (string_id == QPROMPT_RIGHT_HAND_MODE_TEXT_ID)))
        {
            input_answer = MINI_INPUT_RET_POWER_SWITCH;
        }
        
        // Read usb comms as the plugin could ask to cancel the request
        if (comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_CANCEL) == HID_CANCEL_MSG_RCVD)
        {
            /* As this routine may be called by other functions in the firmware.... flash_screen is set to true for ext requests */
            if (accept_cancel_message != FALSE)
            {
                input_answer = MINI_INPUT_RET_TIMEOUT;
            }
        }
        
        // Check if something has been pressed
        detect_result = inputs_get_wheel_action(FALSE, TRUE);
        if (detect_result == WHEEL_ACTION_SHORT_CLICK)
        {
            if (approve_selected != FALSE)
            {
                input_answer = MINI_INPUT_RET_YES;
                if (flash_flag != FALSE)
                {
                    for (uint16_t i = 0; i < POPUP_2LINES_ANIM_LGTH; i++)
                    {
                        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[2]+i, FALSE);
                        timer_delay_ms(10);
                    }
                }
            }
            else
            {
                input_answer = MINI_INPUT_RET_NO;
                if (flash_flag != FALSE)
                {
                    for (uint16_t i = 0; i < POPUP_2LINES_ANIM_LGTH; i++)
                    {
                        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[3]+i, FALSE);
                        timer_delay_ms(10);
                    }
                }
            }
        }
        else if (detect_result == WHEEL_ACTION_LONG_CLICK)
        {
            input_answer = MINI_INPUT_RET_BACK;
        }
        
        // Approve / deny display change
        int16_t wheel_increments = inputs_get_wheel_increment();
        if (wheel_increments != 0)
        {
            if (approve_selected == FALSE)
            {
                for (uint16_t i = (flash_flag == FALSE)?CONF_2LINES_SEL_AN_LGTH-1:0; i < CONF_2LINES_SEL_AN_LGTH; i++)
                {
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[4]+i, FALSE);
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[5]+CONF_2LINES_SEL_AN_LGTH-1-i, FALSE);
                    timer_delay_ms(10);
                }
                
                /* In case of power switching, where the frame buffer is resent to the display */
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[5], TRUE);
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[4]+CONF_2LINES_SEL_AN_LGTH-1, TRUE);
            }
            else
            {
                for (uint16_t i = (flash_flag == FALSE)?CONF_2LINES_SEL_AN_LGTH-1:0; i < CONF_2LINES_SEL_AN_LGTH; i++)
                {
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[5]+i, FALSE);
                    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[4]+CONF_2LINES_SEL_AN_LGTH-1-i, FALSE);
                    timer_delay_ms(10);
                }
                
                /* In case of power switching, where the frame buffer is resent to the display */
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[4], TRUE);
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[5]+CONF_2LINES_SEL_AN_LGTH-1, TRUE);
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
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[7]+flash_sm, FALSE);
            }
            else
            {
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, bitmap_array_pt[6]+flash_sm, FALSE);
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

/*! \fn     gui_prompts_ask_for_confirmation(uint16_t nb_args, confirmationText_t* text_object, BOOL accept_cancel_message, BOOL parse_aux_messages, BOOL exit_on_power_change)
*   \brief  Ask for user confirmation for different things
*   \param  nb_args                 Number of text lines (2 to 4)
*   \param  text_object             Pointer to the text object
*   \param  accept_cancel_message   Boolean to accept the cancel message to cancel prompt
*   \param  parse_aux_messages      Set to TRUE to continue parsing aux messages
*   \param  exit_on_power_change    Set to TRUE to exit function on power change
*   \note   Setting parse_aux_messages to FALSE while setting accept_cancel_message to TRUE only allows to use the knock feature for approval
*   \return See enum
*/
mini_input_yes_no_ret_te gui_prompts_ask_for_confirmation(uint16_t nb_args, confirmationText_t* text_object, BOOL accept_cancel_message, BOOL parse_aux_messages, BOOL exit_on_power_change)
{
    BOOL flash_flag = FALSE;
    uint16_t flash_sm = 0;
    
    /* Check the user hasn't disabled the flash screen feature */
    if ((BOOL)custom_fs_settings_get_device_setting(SETTING_FLASH_SCREEN_ID) != FALSE)
    {
        flash_flag = TRUE;
    }

    // Variables for scrolling
    BOOL string_scrolling_left[4] = {TRUE, TRUE, TRUE, TRUE};
    int16_t string_offset_cntrs[4] = {0,0,0,0};
    BOOL string_scrolling[4];
    uint8_t max_text_x[4];
        
    /* Currently selected choice */
    BOOL approve_selected = TRUE;
    
    /* Clear frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    if (flash_flag != FALSE)
    {
        sh1122_load_transition(&plat_oled_descriptor, OLED_OUT_IN_TRANS);
    }
    else
    {
        sh1122_load_transition(&plat_oled_descriptor, OLED_TRANS_NONE);
    }
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #else
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #endif
    
    /* Compute lines lengths, display them, set scrolling bools */
    for (uint16_t i = 0; i < nb_args; i++)
    {
        /* Set correct font for given line */
        sh1122_refresh_used_font(&plat_oled_descriptor, gui_prompts_conf_prompt_fonts[nb_args-1][i]);
        
        /* Get text width in px */
        uint16_t line_width = sh1122_get_string_width(&plat_oled_descriptor, text_object->lines[i]);
        
        /* Larger than display area? */
        if (line_width < CONF_PROMPT_BITMAP_X)
        {
            string_scrolling[i] = FALSE;
            max_text_x[i] = CONF_PROMPT_BITMAP_X;
        } 
        else if (line_width < gui_prompts_conf_prompts_max_x[nb_args-1][i])
        {
            string_scrolling[i] = FALSE;
            max_text_x[i] = gui_prompts_conf_prompts_max_x[nb_args-1][i];
        }
        else
        {
            string_scrolling[i] = TRUE;
            max_text_x[i] = gui_prompts_conf_prompts_max_x[nb_args-1][i];
        }
        
        /* Set correct font and max text X */
        sh1122_allow_partial_text_x_draw(&plat_oled_descriptor);
        sh1122_set_max_text_x(&plat_oled_descriptor, max_text_x[i]);
        sh1122_put_centered_string(&plat_oled_descriptor, gui_prompts_conf_prompt_y_positions[nb_args-1][i], text_object->lines[i], TRUE);
    }
    
    /* Flush to display */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_flush_frame_buffer(&plat_oled_descriptor);
    
    /* Load transition for function exit (likely to be overwritten later */
    sh1122_load_transition(&plat_oled_descriptor, OLED_IN_OUT_TRANS);
    #endif
    
    /* Wait for user input */
    mini_input_yes_no_ret_te input_answer = MINI_INPUT_RET_NONE;
    acc_detection_te knock_detect_result;
    wheel_action_ret_te detect_result;
    
    /* Activity detected */
    logic_device_activity_detected();
    
    /* Clear possible remaining detection */
    inputs_clear_detections();
    
    /* Transition done, now transition the action bitmap */
    for (uint16_t j = (flash_flag == FALSE)?POPUP_3LINES_ANIM_LGTH-1:0; j < POPUP_3LINES_ANIM_LGTH; j++)
    {
        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_POPUP_3LINES_ID+j, TRUE);
        for (uint16_t i = 0; i < nb_args; i++)
        {
            sh1122_set_max_text_x(&plat_oled_descriptor, max_text_x[i]);
            sh1122_refresh_used_font(&plat_oled_descriptor, gui_prompts_conf_prompt_fonts[nb_args-1][i]);
            sh1122_put_centered_string(&plat_oled_descriptor, gui_prompts_conf_prompt_y_positions[nb_args-1][i], text_object->lines[i], TRUE);
        }
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_flush_frame_buffer(&plat_oled_descriptor);
        #endif
        timer_delay_ms(10);
    }
    
    /* Arm timer for scrolling */
    timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);

    /* Arm timer for flashing */
    timer_start_timer(TIMER_ANIMATIONS, 1000);
    
    /* Variables used for the up/down animation */
    uint16_t up_down_animation_bitmap_id = 0;
    int16_t up_down_animation_increment = 0;
    uint16_t up_down_animation_counter = 0;
    BOOL up_down_animation_triggered = FALSE;
    
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
        
        /* Get power state before entering the next routine */
        power_source_te before_power_source = logic_power_get_power_source();
        
        /* Handle possible power switches */
        logic_power_check_power_switch_and_battery(FALSE);
        
        /* Power source changed and we're asking the user to change left / right hand mode, return directly to not create any confusing */
        if ((before_power_source != logic_power_get_power_source()) && (exit_on_power_change != FALSE))
        {
            input_answer = MINI_INPUT_RET_POWER_SWITCH;
        }
        
        // Read usb comms as the plugin could ask to cancel the request
        if ((parse_aux_messages != FALSE) && (comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_CANCEL) == HID_CANCEL_MSG_RCVD))
        {
            /* As this routine may be called by other functions in the firmware.... flash_screen is set to true for ext requests */
            if (accept_cancel_message != FALSE)
            {
                input_answer = MINI_INPUT_RET_TIMEOUT;
            }
        }
        
        // Check if something has been pressed or for double knock
        detect_result = inputs_get_wheel_action(FALSE, TRUE);
        knock_detect_result = logic_accelerometer_routine();
        if ((detect_result == WHEEL_ACTION_SHORT_CLICK) || ((accept_cancel_message != FALSE) && (knock_detect_result == ACC_DET_KNOCK)))
        { 
            if (approve_selected != FALSE)
            {
                input_answer = MINI_INPUT_RET_YES;
                if (flash_flag != FALSE)
                {
                    for (uint16_t i = 0; i < POPUP_3LINES_ANIM_LGTH+1; i++)
                    {
                        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_YES_PRESS_ID+i, FALSE);
                        timer_delay_ms(10);
                    }
                }
            }
            else
            {
                input_answer = MINI_INPUT_RET_NO;
                if (flash_flag != FALSE)
                {
                    for (uint16_t i = 0; i < POPUP_3LINES_ANIM_LGTH+1; i++)
                    {
                        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_NO_PRESS_ID+i, FALSE);
                        timer_delay_ms(10);
                    }
                }
            }
        }
        else if (detect_result == WHEEL_ACTION_LONG_CLICK)
        {
            input_answer = MINI_INPUT_RET_BACK;
        }
        
        /* Text scrolling animation / up down animation */
        if (timer_has_timer_expired(TIMER_SCROLLING, FALSE) == TIMER_EXPIRED)
        {
            /* Clear frame buffer as we'll only push the updated parts */
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            #endif        
            
            // Idle animation if enabled
            if ((flash_flag != FALSE) && (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED))
            {
                /* Rearm timer: won't obviously fire at said timeout as this is included in the above if() */
                timer_start_timer(TIMER_ANIMATIONS, SCROLLING_DEL*2-1);
                
                /* Check for overflow */
                if (flash_sm++ == CONF_3LINES_IDLE_AN_LGT-1)
                {
                    timer_start_timer(TIMER_ANIMATIONS, 200);
                    flash_sm = 0;
                }
            }
            
            /* Display appropriate bitmap */
            if (up_down_animation_triggered == FALSE)
            {
                if (approve_selected == FALSE)
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
            
                /* Rearm timer */
                timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
            } 
            else
            {
                /* Up / Down animation */
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, up_down_animation_bitmap_id, TRUE);
                up_down_animation_bitmap_id += up_down_animation_increment;
                up_down_animation_counter++;
                
                /* Rearm timer to be as fast as possible */
                timer_start_timer(TIMER_SCROLLING, 0);
                
                /* Check for animation end */
                if (up_down_animation_counter == POPUP_3LINES_ANIM_LGTH)
                {
                    flash_sm = 0;
                    up_down_animation_triggered = FALSE;
                    
                    /* Arm timer for flashing */
                    timer_start_timer(TIMER_ANIMATIONS, 1000);
                    
                    /* Rearm scrolling timer */
                    timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
                }
            }

            /* Display all strings */
            for (uint16_t i = 0; i < nb_args; i++)
            {
                /* Set max text X, set correct font */
                sh1122_set_max_text_x(&plat_oled_descriptor, max_text_x[i]);
                sh1122_refresh_used_font(&plat_oled_descriptor, gui_prompts_conf_prompt_fonts[nb_args-1][i]);
                
                if (string_scrolling[i] != FALSE)
                {
                    /* Erase previous part */
                    #ifndef OLED_INTERNAL_FRAME_BUFFER
                    //sh1122_draw_rectangle(&plat_oled_descriptor, 0, gui_prompts_conf_prompt_y_positions[nb_args-1][i], CONF_PROMPT_MAX_TEXT_X, gui_prompts_conf_prompt_line_heights[nb_args-1][i], 0x00, TRUE);
                    #endif
                    
                    /* Display partial text */
                    int16_t displayed_text_length = sh1122_put_string_xy(&plat_oled_descriptor, string_offset_cntrs[i], gui_prompts_conf_prompt_y_positions[nb_args-1][i], OLED_ALIGN_LEFT, text_object->lines[i], TRUE);
                    
                    /* Increment or decrement X offset */
                    if (string_scrolling_left[i] == FALSE)
                    {
                        if (string_offset_cntrs[i]++ == 12)
                        {
                            string_scrolling_left[i] = TRUE;
                        }
                    } 
                    else
                    {
                        string_offset_cntrs[i]--;
                        if (displayed_text_length == max_text_x[i]-12)
                        {
                            string_scrolling_left[i] = FALSE;
                        }
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
        if ((up_down_animation_triggered == FALSE) && (wheel_increments != 0))
        {
            /* Trigger animation */
            if (flash_flag != FALSE)
            {
                up_down_animation_triggered = TRUE;
            }
            up_down_animation_counter = 0;
            
            if (wheel_increments < 0)
            {
                if (approve_selected == FALSE)
                {
                    up_down_animation_bitmap_id = BITMAP_NY_UP_ID;
                    up_down_animation_increment = 1;
                } 
                else
                {
                    up_down_animation_bitmap_id = BITMAP_NY_UP_ID+POPUP_3LINES_ANIM_LGTH-1;
                    up_down_animation_increment = -1;
                }
            } 
            else
            {
                if (approve_selected == FALSE)
                {
                    up_down_animation_bitmap_id = BITMAP_NY_DOWN_ID;
                    up_down_animation_increment = 1;
                }
                else
                {
                    up_down_animation_bitmap_id = BITMAP_NY_DOWN_ID+POPUP_3LINES_ANIM_LGTH-1;
                    up_down_animation_increment = -1;
                }
            }
                
            approve_selected = !approve_selected;
            
            /* Reset state machine, update timer */
            timer_start_timer(TIMER_SCROLLING, 1);
            flash_sm = 0;
        }
    }
    
    /* Reset text preferences */
    sh1122_reset_max_text_x(&plat_oled_descriptor);
    sh1122_prevent_partial_text_x_draw(&plat_oled_descriptor);
    
    return input_answer;
}

/*! \fn     gui_prompts_ask_for_login_select(uint16_t parent_node_addr, uint16_t* chosen_child_node_addr)
*   \brief  Ask for user login selection / approval
*   \param  parent_node_addr        Address of the parent node
*   \param  chosen_child_node_addr  Address of the selected child node by default (or NODE_ADDR_NULL), then populated with selected child node if return is MINI_INPUT_RET_YES
*   \return MINI_INPUT_RET_YES on correct selection, otherwise see enum
*/
mini_input_yes_no_ret_te gui_prompts_ask_for_login_select(uint16_t parent_node_addr, uint16_t* chosen_child_node_addr)
{
    child_cred_node_t* temp_half_cnode_pt;
    cust_char_t* select_login_string;
    parent_node_t temp_half_cnode;
    uint16_t first_child_address;
    parent_node_t temp_pnode;
    
    /* Dirty trick */
    temp_half_cnode_pt = (child_cred_node_t*)&temp_half_cnode;

    /* Check parent node address */
    if (parent_node_addr == NODE_ADDR_NULL)
    {
        return NODE_ADDR_NULL;
    }
    
    /* Activity detected */
    logic_device_activity_detected();
    
    /* Clear possible remaining detection */
    inputs_clear_detections();

    /* Read the parent node and read its first child address */
    nodemgmt_read_parent_node(parent_node_addr, &temp_pnode, TRUE);
    first_child_address = nodemgmt_check_for_logins_with_category_in_parent_node(temp_pnode.cred_parent.nextChildAddress, nodemgmt_get_current_category_flags());

    /* Check if there are stored credentials */
    if (first_child_address == NODE_ADDR_NULL)
    {
        gui_prompts_display_information_on_screen_and_wait(NO_CREDS_TEXT_ID, DISP_MSG_INFO, FALSE);
        return NODE_ADDR_NULL;
    }
    
    /* Clear frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_load_transition(&plat_oled_descriptor, OLED_IN_OUT_TRANS);
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #else
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #endif
    
    /* Temp vars for our main loop */
    uint16_t before_top_of_list_child_addr = NODE_ADDR_NULL;
    uint16_t top_of_list_child_addr = NODE_ADDR_NULL;
    uint16_t center_list_child_addr = NODE_ADDR_NULL;
    uint16_t bottom_list_child_addr = NODE_ADDR_NULL;
    uint16_t after_bottom_list_child_addr = NODE_ADDR_NULL;
    BOOL end_of_list_reached_at_center = FALSE;
    BOOL animation_just_started = TRUE;
    BOOL first_function_run = TRUE;
    int16_t text_anim_x_offset[4];
    BOOL text_anim_going_right[4];
    int16_t animation_step = 0;
    BOOL redraw_needed = TRUE;
    BOOL action_taken = FALSE;
    int16_t displayed_length;
    BOOL scrolling_needed[4];
    
    /* Lines display settings */    
    uint16_t non_addr_null_addr_tbp = NODE_ADDR_NULL+1;
    uint16_t* address_to_check_to_display[5] = {&non_addr_null_addr_tbp, &top_of_list_child_addr, &center_list_child_addr, &bottom_list_child_addr, &after_bottom_list_child_addr};
    cust_char_t* strings_to_be_displayed[4] = {temp_pnode.cred_parent.service, temp_half_cnode_pt->login, temp_half_cnode_pt->login, temp_half_cnode_pt->login};
    uint16_t fonts_to_be_used[4] = {FONT_UBUNTU_REGULAR_16_ID, FONT_UBUNTU_REGULAR_13_ID, FONT_UBUNTU_MEDIUM_15_ID, FONT_UBUNTU_REGULAR_13_ID};
    uint16_t strings_y_positions[4] = {0, LOGIN_SCROLL_Y_FLINE, LOGIN_SCROLL_Y_SLINE, LOGIN_SCROLL_Y_TLINE};
        
    /* If the selected login was specified */
    if (*chosen_child_node_addr != NODE_ADDR_NULL)
    {
        top_of_list_child_addr = nodemgmt_get_prev_child_node_for_cur_category(*chosen_child_node_addr);
        before_top_of_list_child_addr = nodemgmt_get_prev_child_node_for_cur_category(top_of_list_child_addr);
    }
    
    /* Reset temp vars */
    memset(text_anim_going_right, FALSE, sizeof(text_anim_going_right));
    memset(text_anim_x_offset, 0, sizeof(text_anim_x_offset));
    memset(scrolling_needed, FALSE, sizeof(scrolling_needed));
    
    /* "Select login" string */
    custom_fs_get_string_from_file(SELECT_LOGIN_TEXT_ID, &select_login_string, TRUE);
    
    /* Prepare first line display (<<service>>: select credential), store it in the service field. Service field is 0 terminated by previous calls */
    if (utils_strlen(temp_pnode.cred_parent.service) + utils_strlen(select_login_string) + 1 <= (uint16_t)MEMBER_ARRAY_SIZE(parent_cred_node_t, service))
    {
        utils_strcpy(&temp_pnode.cred_parent.service[utils_strlen(temp_pnode.cred_parent.service)], select_login_string);
    }
            
    /* String width to set correct underline */
    sh1122_refresh_used_font(&plat_oled_descriptor, fonts_to_be_used[0]);
    uint16_t underline_width = sh1122_get_string_width(&plat_oled_descriptor, temp_pnode.cred_parent.service);
    uint16_t underline_x_start = plat_oled_descriptor.min_text_x;
            
    /* Sanitizing */
    if ((plat_oled_descriptor.min_text_x + underline_width) < plat_oled_descriptor.max_text_x)
    {
        underline_x_start = plat_oled_descriptor.min_text_x + (plat_oled_descriptor.max_text_x - plat_oled_descriptor.min_text_x - underline_width)/2;
    }
    else
    {
        underline_width = plat_oled_descriptor.max_text_x - plat_oled_descriptor.min_text_x;
    }
    
    /* Arm timer for scrolling */
    timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
    
    /* Loop until something has been done */
    while (action_taken == FALSE)
    {
        /* User interaction timeout */
        if (timer_has_timer_expired(TIMER_USER_INTERACTION, TRUE) == TIMER_EXPIRED)
        {
            return MINI_INPUT_RET_TIMEOUT;
        }
        
        /* Card removed */
        if (smartcard_low_level_is_smc_absent() == RETURN_OK)
        {
            return MINI_INPUT_RET_CARD_REMOVED;
        }
        
        // Call accelerometer routine for (among others) RNG stuff
        logic_accelerometer_routine();
        
        /* Handle possible power switches */
        logic_power_check_power_switch_and_battery(FALSE);
        
        /* Read usb comms as the plugin could ask to cancel the request */
        if (comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_CANCEL) == HID_CANCEL_MSG_RCVD)
        {
            return MINI_INPUT_RET_CANCELED;
        }
        
        /* Check if something has been pressed */
        wheel_action_ret_te detect_result = inputs_get_wheel_action(FALSE, FALSE);
        if (detect_result == WHEEL_ACTION_SHORT_CLICK)
        { 
            *chosen_child_node_addr = center_list_child_addr;
            return MINI_INPUT_RET_YES;
        }
        else if (detect_result == WHEEL_ACTION_LONG_CLICK)
        {
            return MINI_INPUT_RET_BACK;
        }
        else if (detect_result == WHEEL_ACTION_DOWN)
        {
            if (end_of_list_reached_at_center == FALSE)
            {
                before_top_of_list_child_addr = top_of_list_child_addr;
                top_of_list_child_addr = center_list_child_addr;
                animation_step = ((LOGIN_SCROLL_Y_TLINE-LOGIN_SCROLL_Y_SLINE)/2)*2;
                animation_just_started = TRUE;
            }
        }
        else if (detect_result == WHEEL_ACTION_UP)
        {
            if (top_of_list_child_addr != NODE_ADDR_NULL)
            {
                top_of_list_child_addr = before_top_of_list_child_addr;
                animation_step = -((LOGIN_SCROLL_Y_SLINE-LOGIN_SCROLL_Y_FLINE)/2)*2;
                animation_just_started = TRUE;
            }
        }
            
        /* If animation is happening */
        if (animation_just_started != FALSE)
        {
            /* Reset scrolling states except for title */
            memset(&text_anim_going_right[1], 0, sizeof(text_anim_going_right)-sizeof(text_anim_going_right[0]));
            memset(&text_anim_x_offset[1], 0, sizeof(text_anim_x_offset)-sizeof(text_anim_x_offset[0]));
            memset(&scrolling_needed[1], FALSE, sizeof(scrolling_needed)-sizeof(scrolling_needed[0]));
            redraw_needed = TRUE;
        }
        
        /* Scrolling logic */
        if (timer_has_timer_expired(TIMER_SCROLLING, FALSE) == TIMER_EXPIRED)
        {
            /* Rearm timer */
            timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
            
            /* Scrolling logic: when enabled, going left or right... */
            for (uint16_t i = 0; i < 4; i++)
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
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            #endif
            sh1122_allow_partial_text_x_draw(&plat_oled_descriptor);
            sh1122_allow_partial_text_y_draw(&plat_oled_descriptor);
            
            /* Animation: scrolling down, keeping the first of item displayed & fading out */
            sh1122_set_min_display_y(&plat_oled_descriptor, LOGIN_SCROLL_Y_BAR+1);
            if ((animation_step > 0) && (before_top_of_list_child_addr != NODE_ADDR_NULL))
            {
                /* Fetch node */
                nodemgmt_read_cred_child_node_except_pwd(before_top_of_list_child_addr, temp_half_cnode_pt);
                
                /* Display fading out login */
                sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_REGULAR_13_ID);
                sh1122_put_centered_string(&plat_oled_descriptor, LOGIN_SCROLL_Y_FLINE-(((LOGIN_SCROLL_Y_TLINE-LOGIN_SCROLL_Y_SLINE)/2)*2)+animation_step, temp_half_cnode_pt->login, TRUE);
            }
            sh1122_reset_lim_display_y(&plat_oled_descriptor);
            
            /* Bar below the title */
            sh1122_draw_rectangle(&plat_oled_descriptor, underline_x_start, LOGIN_SCROLL_Y_BAR, underline_width, 1, 0xFF, TRUE);
            
            /* Loop for 4 always displayed texts: title then 3 list items */
            for (uint16_t i = 0; i < 4; i++)
            {
                if (i == 0)
                {
                    sh1122_reset_lim_display_y(&plat_oled_descriptor);
                }
                
                /* Check if we should display it */
                if (*(address_to_check_to_display[i]) != NODE_ADDR_NULL)
                {
                    /* Load the right font */
                    sh1122_refresh_used_font(&plat_oled_descriptor, fonts_to_be_used[i]);
                    
                    /* Fetch node if needed */
                    if (i > 0)
                    {
                        nodemgmt_read_cred_child_node_except_pwd(*(address_to_check_to_display[i]), temp_half_cnode_pt);
                    }
                    
                    /* Surround center of list item */
                    if (i == 2)
                    {
                        utils_surround_text_with_pointers(temp_half_cnode_pt->login, MEMBER_ARRAY_SIZE(child_cred_node_t, login));
                    }
                    
                    /* First address: store the "before top address */
                    if (i == 1)
                    {
                        before_top_of_list_child_addr = nodemgmt_get_prev_child_node_for_cur_category(top_of_list_child_addr);
                    }
                    
                    /* Last address: store correct bool */
                    if (i == 3)
                    {
                        end_of_list_reached_at_center = FALSE;
                    }
            
                    /* Y offset for animations */
                    int16_t yoffset = 0;
                    if (i > 0)
                    {
                        yoffset = animation_step;
                    }
                    
                    /* Display string */
                    if (scrolling_needed[i] != FALSE)
                    {                        
                        /* Scrolling required: display with the correct X offset */
                        displayed_length = sh1122_put_string_xy(&plat_oled_descriptor, text_anim_x_offset[i], strings_y_positions[i]+yoffset, OLED_ALIGN_LEFT, strings_to_be_displayed[i], TRUE);
                
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
                        displayed_length = sh1122_put_centered_string(&plat_oled_descriptor, strings_y_positions[i]+yoffset, strings_to_be_displayed[i], TRUE);
                    }
                    
                    /* First run: based on the number of chars displayed, set the scrolling needed bool */
                    if ((animation_just_started != FALSE) && (displayed_length < 0))
                    {
                        scrolling_needed[i] = TRUE;
                    }
                    
                    /* Store next item addresses */
                    if (i > 0)
                    {
                        /* Array has an extra slot */
                        *(address_to_check_to_display[i+1]) = nodemgmt_get_next_child_node_for_cur_category(*(address_to_check_to_display[i]));
                    }                    
                    
                    /* Last item & animation scrolling up: display upcoming item */
                    if (i == 3)
                    {
                        if ((animation_step < 0) && (*(address_to_check_to_display[i+1]) != NODE_ADDR_NULL))
                        {
                            /* Fetch node */
                            nodemgmt_read_cred_child_node_except_pwd(*(address_to_check_to_display[i+1]), temp_half_cnode_pt);
                            
                            /* Display fading out login */
                            sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_REGULAR_13_ID);
                            sh1122_put_centered_string(&plat_oled_descriptor, LOGIN_SCROLL_Y_TLINE+(((LOGIN_SCROLL_Y_SLINE-LOGIN_SCROLL_Y_FLINE)/2)*2)+animation_step, temp_half_cnode_pt->login, TRUE);
                        }
                    }
                }
                else
                {
                    /* Check for node to display failed */
                    if (i == 1)
                    {
                        /* Couldn't display the top of the list, means our center child is the first child */
                        center_list_child_addr = first_child_address;
                    }
                    if (i == 3)
                    {
                        /* No last item to display, end of list reached at center */
                        end_of_list_reached_at_center = TRUE;
                    }
                }
                
                if (i == 0)
                {
                    sh1122_set_min_display_y(&plat_oled_descriptor, LOGIN_SCROLL_Y_BAR+1);
                }
            }             
            
            /* Reset display settings */
            sh1122_prevent_partial_text_y_draw(&plat_oled_descriptor);
            sh1122_prevent_partial_text_x_draw(&plat_oled_descriptor);
            sh1122_reset_lim_display_y(&plat_oled_descriptor);
            
            /* Reset animation just started var */
            animation_just_started = FALSE;
            
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
            
            /* Animation processing */
            if (animation_step > 0)
            {
                animation_step-=2;
            }
            else if (animation_step < 0)
            {
                animation_step+=2;
            }
            else
            {
                redraw_needed = FALSE;                
            }
        }        
    }
    
    return MINI_INPUT_RET_NO;
}

/*! \fn     gui_prompts_service_selection_screen(uint16_t start_address)
*   \brief  Screen for manual service selection
*   \param  start_address   Address of the service we should start at
*   \return Valid parent node address or 0 otherwise
*/
uint16_t gui_prompts_service_selection_screen(uint16_t start_address)
{
    cust_char_t* select_credential_string;
    parent_node_t temp_pnode;
    
    /* Activity detected */
    logic_device_activity_detected();
    
    /* Clear possible remaining detection */
    inputs_clear_detections();

    /* Sanity check */
    if (start_address == NODE_ADDR_NULL)
    {
        return NODE_ADDR_NULL;
    }
    
    /* Clear frame buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_load_transition(&plat_oled_descriptor, OLED_IN_OUT_TRANS);
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #else
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #endif
    
    /* Temp vars for our main loop */
    uint16_t top_of_list_parent_addr = nodemgmt_get_prev_parent_node_for_cur_category(start_address, NODEMGMT_STANDARD_CRED_TYPE_ID);
    uint16_t before_top_of_list_parent_addr = NODE_ADDR_NULL;
    uint16_t center_of_list_parent_addr = start_address;
    uint16_t bottom_of_list_parent_addr = NODE_ADDR_NULL;
    uint16_t after_bottom_of_list_parent_addr = NODE_ADDR_NULL;
    BOOL end_of_list_reached_at_center = FALSE;
    BOOL displaying_service_fchars = FALSE;
    BOOL user_knows_press_scroll =  FALSE;
    BOOL animation_just_started = TRUE;
    BOOL hint_cur_displayed = FALSE;
    BOOL first_function_run = TRUE;
    BOOL displaying_hint = FALSE;
    int16_t text_anim_x_offset[4];
    BOOL text_anim_going_right[4];
    cust_char_t cur_fchar = ' ';
    cust_char_t fchar_array[3];
    int16_t animation_step = 0;
    BOOL redraw_needed = TRUE;
    BOOL action_taken = FALSE;
    int16_t displayed_length;
    BOOL scrolling_needed[4];
    
    /* Load hint string to compute aestetical elements positions */
    custom_fs_get_string_from_file(PRESS_SCROLL_HINT_TEXT_ID, &select_credential_string, TRUE);
    
    /* Lines display settings */
    uint16_t non_addr_null_addr_tbp = NODE_ADDR_NULL+1;
    uint16_t* address_to_check_to_display[5] = {&non_addr_null_addr_tbp, &top_of_list_parent_addr, &center_of_list_parent_addr, &bottom_of_list_parent_addr, &after_bottom_of_list_parent_addr};
    cust_char_t* strings_to_be_displayed[4] = {select_credential_string, temp_pnode.cred_parent.service, temp_pnode.cred_parent.service, temp_pnode.cred_parent.service};
    uint16_t fonts_to_be_used[4] = {FONT_UBUNTU_REGULAR_16_ID, FONT_UBUNTU_REGULAR_13_ID, FONT_UBUNTU_MEDIUM_15_ID, FONT_UBUNTU_REGULAR_13_ID};
    uint16_t strings_y_positions[4] = {0, LOGIN_SCROLL_Y_FLINE, LOGIN_SCROLL_Y_SLINE, LOGIN_SCROLL_Y_TLINE};
    
    /* Reset temp vars */
    memset(text_anim_going_right, FALSE, sizeof(text_anim_going_right));
    memset(text_anim_x_offset, 0, sizeof(text_anim_x_offset));
    memset(scrolling_needed, FALSE, sizeof(scrolling_needed));
    
    /* Scroll hint string width to set correct underline */
    sh1122_refresh_used_font(&plat_oled_descriptor, fonts_to_be_used[0]);
    uint16_t scroll_hint_width = sh1122_get_string_width(&plat_oled_descriptor, strings_to_be_displayed[0]);
    uint16_t scroll_hint_x_start = plat_oled_descriptor.min_text_x + (plat_oled_descriptor.max_text_x - plat_oled_descriptor.min_text_x - scroll_hint_width)/2;
    
    /* "Select credential" string */
    custom_fs_get_string_from_file(SELECT_SERVICE_TEXT_ID, &select_credential_string, TRUE);
    
    /* Select login string width to set correct underline */
    sh1122_refresh_used_font(&plat_oled_descriptor, fonts_to_be_used[0]);
    uint16_t select_login_width = sh1122_get_string_width(&plat_oled_descriptor, strings_to_be_displayed[0]);
    uint16_t select_login_x_start = plat_oled_descriptor.min_text_x + (plat_oled_descriptor.max_text_x - plat_oled_descriptor.min_text_x - select_login_width)/2;
    uint16_t underline_bar_start_x = select_login_x_start;
    uint16_t underline_bar_width = select_login_width;
    
    /* Arm timer for scrolling */
    timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
    
    /* Timestamp to know when to display the hint */
    uint32_t func_stat_ts = timer_get_systick();
    
    /* Loop until something has been done */
    while (action_taken == FALSE)
    {
        /* User interaction timeout */
        if (timer_has_timer_expired(TIMER_USER_INTERACTION, TRUE) == TIMER_EXPIRED)
        {
            return NODE_ADDR_NULL;
        }
        
        /* Card removed */
        if (smartcard_low_level_is_smc_absent() == RETURN_OK)
        {
            return NODE_ADDR_NULL;
        }
        
        /* Call accelerometer routine for (among others) RNG stuff */
        logic_accelerometer_routine();
        
        /* Handle possible power switches */
        logic_power_check_power_switch_and_battery(FALSE);
        
        /* Read usb comms as the plugin could ask to cancel the request */
        if (comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_CANCEL) == HID_CANCEL_MSG_RCVD)
        {
            return NODE_ADDR_NULL;
        }
        
        /* Check if something has been pressed */
        wheel_action_ret_te detect_result = inputs_get_wheel_action(FALSE, FALSE);
        if (detect_result == WHEEL_ACTION_SHORT_CLICK)
        {
            /* return selected address */
            return center_of_list_parent_addr;
        }
        else if (detect_result == WHEEL_ACTION_LONG_CLICK)
        {
            return NODE_ADDR_NULL;
        }
        else if (detect_result == WHEEL_ACTION_DOWN)
        {
            if (end_of_list_reached_at_center == FALSE)
            {
                before_top_of_list_parent_addr = top_of_list_parent_addr;
                top_of_list_parent_addr = center_of_list_parent_addr;
                animation_step = ((LOGIN_SCROLL_Y_TLINE-LOGIN_SCROLL_Y_SLINE)/2)*2;
                animation_just_started = TRUE;
            }
        }
        else if (detect_result == WHEEL_ACTION_UP)
        {
            if (top_of_list_parent_addr != NODE_ADDR_NULL)
            {
                center_of_list_parent_addr = top_of_list_parent_addr;
                top_of_list_parent_addr = before_top_of_list_parent_addr;
                animation_step = -((LOGIN_SCROLL_Y_SLINE-LOGIN_SCROLL_Y_FLINE)/2)*2;
                animation_just_started = TRUE;
            }
        }
        else if (detect_result == WHEEL_ACTION_CLICK_DOWN)
        {
            /* Reset hint display logic */
            if (hint_cur_displayed != FALSE)
            {
                custom_fs_get_string_from_file(SELECT_SERVICE_TEXT_ID, &select_credential_string, TRUE);
                underline_bar_start_x = select_login_x_start;
                underline_bar_width = select_login_width;
            }
            user_knows_press_scroll = TRUE;
            hint_cur_displayed = FALSE;
            displaying_hint = FALSE;
        
            uint16_t next_diff_fletter_node_addr = logic_database_get_next_2_fletters_services(center_of_list_parent_addr, cur_fchar, &fchar_array[1], NODEMGMT_STANDARD_CRED_TYPE_ID);
            if (next_diff_fletter_node_addr != NODE_ADDR_NULL)
            {
                fchar_array[0] = cur_fchar;
                displaying_service_fchars = TRUE;
                center_of_list_parent_addr = next_diff_fletter_node_addr;
                top_of_list_parent_addr = nodemgmt_get_prev_parent_node_for_cur_category(center_of_list_parent_addr, NODEMGMT_STANDARD_CRED_TYPE_ID);
                animation_just_started = TRUE;
                
                /* Only 2 letters */
                if (fchar_array[2] == ' ')
                {
                    fchar_array[2] = cur_fchar;
                }
            }     
            else
            {
                fchar_array[1] = cur_fchar;
            }        
        }
        else if (detect_result == WHEEL_ACTION_CLICK_UP)
        {
            /* Reset hint display logic */
            if (hint_cur_displayed != FALSE)
            {
                custom_fs_get_string_from_file(SELECT_SERVICE_TEXT_ID, &select_credential_string, TRUE);
                underline_bar_start_x = select_login_x_start;
                underline_bar_width = select_login_width;
            }
            user_knows_press_scroll = TRUE;
            hint_cur_displayed = FALSE;
            displaying_hint = FALSE;
            
            uint16_t prev_diff_fletter_node_addr = logic_database_get_prev_2_fletters_services(center_of_list_parent_addr, cur_fchar, fchar_array, NODEMGMT_STANDARD_CRED_TYPE_ID);
            if (prev_diff_fletter_node_addr != NODE_ADDR_NULL)
            {
                fchar_array[2] = cur_fchar;
                displaying_service_fchars = TRUE;
                center_of_list_parent_addr = prev_diff_fletter_node_addr;
                top_of_list_parent_addr = nodemgmt_get_prev_parent_node_for_cur_category(center_of_list_parent_addr, NODEMGMT_STANDARD_CRED_TYPE_ID);
                animation_just_started = TRUE;
                
                /* Only 2 letters */
                if (fchar_array[0] == ' ')
                {
                    fchar_array[0] = cur_fchar;
                }
            }
            else
            {
                fchar_array[1] = cur_fchar;
            }
        }
        else if (detect_result == WHEEL_ACTION_DISCARDED)
        {
            /* Small delay to clear detections possibly caused by scrolling */
            timer_delay_ms(50);
            inputs_clear_detections();
        }
        
        /* Check if we should start displaying the hint */
        if ((displaying_hint == FALSE) && ((timer_get_systick()-func_stat_ts) > GUI_DELAY_BEFORE_HINT) && (user_knows_press_scroll == FALSE))
        {
            displaying_hint = TRUE;
            hint_cur_displayed = TRUE;
            
            /* Animation timer used for hint blinking */
            timer_start_timer(TIMER_ANIMATIONS, GUI_DELAY_HINT_BLINKING);
            
            /* Load hint string */
            custom_fs_get_string_from_file(PRESS_SCROLL_HINT_TEXT_ID, &select_credential_string, TRUE);
            underline_bar_start_x = scroll_hint_x_start;
            underline_bar_width = scroll_hint_width;
        }

        /* We're displaying first chars but the wheel was released */
        if ((displaying_service_fchars != FALSE) && (inputs_raw_is_wheel_released() != FALSE))
        {
            displaying_service_fchars = FALSE;
        }
            
        /* If animation is happening */
        if (animation_just_started != FALSE)
        {
            /* Reset scrolling states except for title */
            memset(&text_anim_going_right[1], 0, sizeof(text_anim_going_right)-sizeof(text_anim_going_right[0]));
            memset(&text_anim_x_offset[1], 0, sizeof(text_anim_x_offset)-sizeof(text_anim_x_offset[0]));
            memset(&scrolling_needed[1], FALSE, sizeof(scrolling_needed)-sizeof(scrolling_needed[0]));
            redraw_needed = TRUE;
        }
        
        /* Scrolling logic */
        if (timer_has_timer_expired(TIMER_SCROLLING, FALSE) == TIMER_EXPIRED)
        {
            /* Rearm timer */
            timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
            
            /* Scrolling logic: when enabled, going left or right... */
            for (uint16_t i = 0; i < 4; i++)
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
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            #endif
            sh1122_allow_partial_text_x_draw(&plat_oled_descriptor);
            sh1122_allow_partial_text_y_draw(&plat_oled_descriptor);
            
            /* Animation: scrolling down, keeping the first of item displayed & fading out */
            sh1122_set_min_display_y(&plat_oled_descriptor, LOGIN_SCROLL_Y_BAR+1);
            if ((animation_step > 0) && (before_top_of_list_parent_addr != NODE_ADDR_NULL))
            {
                /* Fetch node */
                nodemgmt_read_parent_node(before_top_of_list_parent_addr, &temp_pnode, TRUE);
                
                /* Display fading out service */
                sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_REGULAR_13_ID);
                sh1122_put_centered_string(&plat_oled_descriptor, LOGIN_SCROLL_Y_FLINE-(((LOGIN_SCROLL_Y_TLINE-LOGIN_SCROLL_Y_SLINE)/2)*2)+animation_step, temp_pnode.cred_parent.service, TRUE);
            }
            sh1122_reset_lim_display_y(&plat_oled_descriptor);
            
            /* Bar below the title */
            sh1122_draw_rectangle(&plat_oled_descriptor, underline_bar_start_x, LOGIN_SCROLL_Y_BAR, underline_bar_width, 1, 0xFF, TRUE);
            
            /* Loop for 4 always displayed texts: title then 3 list items */
            for (uint16_t i = 0; i < 4; i++)
            {
                if (i == 0)
                {
                    sh1122_reset_lim_display_y(&plat_oled_descriptor);
                }
                
                /* Check if we should display it */
                if (*(address_to_check_to_display[i]) != NODE_ADDR_NULL)
                {
                    /* Load the right font */
                    sh1122_refresh_used_font(&plat_oled_descriptor, fonts_to_be_used[i]);
                    
                    /* Fetch node if needed */
                    if (i > 0)
                    {
                        nodemgmt_read_parent_node(*(address_to_check_to_display[i]), &temp_pnode, TRUE);
                    }
                    
                    /* Surround center of list item */
                    if (i == 2)
                    {
                        cur_fchar = temp_pnode.cred_parent.service[0];
                        utils_surround_text_with_pointers(temp_pnode.cred_parent.service, MEMBER_ARRAY_SIZE(parent_data_node_t, service));
                    }
                    
                    /* First address: store the "before top address */
                    if (i == 1)
                    {
                        before_top_of_list_parent_addr = nodemgmt_get_prev_parent_node_for_cur_category(top_of_list_parent_addr, NODEMGMT_STANDARD_CRED_TYPE_ID);
                    }
                    
                    /* Last address: store correct bool */
                    if (i == 3)
                    {
                        end_of_list_reached_at_center = FALSE;
                    }
                    
                    /* Y offset for animations */
                    int16_t yoffset = 0;
                    if (i > 0)
                    {
                        yoffset = animation_step;
                    }
                    
                    /* Display string */
                    if (scrolling_needed[i] != FALSE)
                    {
                        /* Scrolling required: display with the correct X offset */
                        displayed_length = sh1122_put_string_xy(&plat_oled_descriptor, text_anim_x_offset[i], strings_y_positions[i]+yoffset, OLED_ALIGN_LEFT, strings_to_be_displayed[i], TRUE);
                        
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
                        if (i == 0)
                        {
                            /* Title: either display selection text or the first chars or the hint */
                            if (displaying_service_fchars == FALSE)
                            {
                                /* Same pointer has the pointer to the string doesn't change when we reload strings */
                                displayed_length = sh1122_put_centered_string(&plat_oled_descriptor, strings_y_positions[i]+yoffset, strings_to_be_displayed[i], TRUE);
                                
                                /* Blinking timer logic */
                                if ((displaying_hint != FALSE) && (timer_has_timer_expired(TIMER_ANIMATIONS, FALSE) == TIMER_EXPIRED))
                                {
                                    /* Animation timer used for hint blinking */
                                    timer_start_timer(TIMER_ANIMATIONS, GUI_DELAY_HINT_BLINKING);
                                                         
                                    if (hint_cur_displayed == FALSE)
                                    {
                                        /* Load hint string */
                                        custom_fs_get_string_from_file(PRESS_SCROLL_HINT_TEXT_ID, &select_credential_string, TRUE);
                                        underline_bar_start_x = scroll_hint_x_start;
                                        underline_bar_width = scroll_hint_width;
                                        hint_cur_displayed = TRUE;
                                    } 
                                    else
                                    {
                                        /* "Select credential" string */
                                        custom_fs_get_string_from_file(SELECT_SERVICE_TEXT_ID, &select_credential_string, TRUE);
                                        underline_bar_start_x = select_login_x_start;
                                        underline_bar_width = select_login_width;
                                        hint_cur_displayed = FALSE;
                                    }                                
                                }
                            } 
                            else
                            {
                                /* Previous / Current / Next first service char */
                                sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_REGULAR_16_ID);
                                sh1122_put_centered_char(&plat_oled_descriptor, 103, strings_y_positions[i], fchar_array[0], TRUE);
                                sh1122_put_centered_char(&plat_oled_descriptor, 153, strings_y_positions[i], fchar_array[2], TRUE);
                                sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_MEDIUM_17_ID);
                                sh1122_put_centered_char(&plat_oled_descriptor, 128, strings_y_positions[i], fchar_array[1], TRUE);
                                displayed_length = 1;
                            }
                        } 
                        else
                        {
                            /* String not large enough or start of animation */
                            displayed_length = sh1122_put_centered_string(&plat_oled_descriptor, strings_y_positions[i]+yoffset, strings_to_be_displayed[i], TRUE);
                        }
                    }
                    
                    /* First run: based on the number of chars displayed, set the scrolling needed bool */
                    if ((animation_just_started != FALSE) && (displayed_length < 0))
                    {
                        scrolling_needed[i] = TRUE;
                    }
                    
                    /* Store next item address */
                    if (i > 0)
                    {
                        /* Array has an extra element */
                        *(address_to_check_to_display[i+1]) = nodemgmt_get_next_parent_node_for_cur_category(*(address_to_check_to_display[i]), NODEMGMT_STANDARD_CRED_TYPE_ID);
                    }
                    
                    /* Last item & animation scrolling up: display upcoming item */
                    if (i == 3)
                    {
                        if ((animation_step < 0) && (*(address_to_check_to_display[i+1]) != NODE_ADDR_NULL))
                        {
                            /* Fetch node */
                            nodemgmt_read_parent_node(*(address_to_check_to_display[i+1]), &temp_pnode, TRUE);
                            
                            /* Display fading out login */
                            sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_REGULAR_13_ID);
                            sh1122_put_centered_string(&plat_oled_descriptor, LOGIN_SCROLL_Y_TLINE+(((LOGIN_SCROLL_Y_SLINE-LOGIN_SCROLL_Y_FLINE)/2)*2)+animation_step, temp_pnode.cred_parent.service, TRUE);
                        }
                    }
                }
                else
                {
                    if (i == 3)
                    {
                        /* No last item to display, end of list reached at center */
                        end_of_list_reached_at_center = TRUE;
                    }
                }
                
                if (i == 0)
                {
                    sh1122_set_min_display_y(&plat_oled_descriptor, LOGIN_SCROLL_Y_BAR+1);
                }
            }
            
            /* Reset display settings */
            sh1122_prevent_partial_text_y_draw(&plat_oled_descriptor);
            sh1122_prevent_partial_text_x_draw(&plat_oled_descriptor);
            sh1122_reset_lim_display_y(&plat_oled_descriptor);
            
            /* Reset animation just started var */
            animation_just_started = FALSE;
            
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
            
            /* Animation processing */
            if (animation_step > 0)
            {
                animation_step-=2;
            }
            else if (animation_step < 0)
            {
                animation_step+=2;
            }
            else
            {
                redraw_needed = FALSE;
            }
        }
    }
    
    return NODE_ADDR_NULL;
}

/*! \fn     gui_prompts_select_category(void)
*   \brief  Prompt user to select a credential category 
*   \return category ID or -1 if he went back
*/
int16_t gui_prompts_select_category(void)
{
    cust_char_t temp_category_text[MEMBER_SUB_ARRAY_SIZE(nodemgmt_user_category_strings_t, category_strings) + 5];
    int16_t selected_category = nodemgmt_get_current_category();
    BOOL function_just_started = TRUE;
    BOOL first_function_run = TRUE;
    cust_char_t* string_to_display;
    int16_t text_anim_x_offset[5];
    BOOL text_anim_going_right[5];    
    BOOL redraw_needed = TRUE;
    BOOL action_taken = FALSE;
    int16_t displayed_length;
    BOOL scrolling_needed[5];
    uint16_t temp_y_offset;
    
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
    
    /* Arm timer for scrolling */
    timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
    
    /* Loop until something has been done */
    while (action_taken == FALSE)
    {
        /* User interaction timeout */
        if (timer_has_timer_expired(TIMER_USER_INTERACTION, TRUE) == TIMER_EXPIRED)
        {
            return -1;
        }
        
        /* Card removed */
        if (smartcard_low_level_is_smc_absent() == RETURN_OK)
        {
            return -1;
        }
        
        /* Call accelerometer routine for (among others) RNG stuff */
        logic_accelerometer_routine();
        
        /* Handle possible power switches */
        logic_power_check_power_switch_and_battery(FALSE);
        
        /* Deal with simple messages */
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        
        /* Check if something has been pressed */
        wheel_action_ret_te detect_result = inputs_get_wheel_action(FALSE, FALSE);
        if (detect_result == WHEEL_ACTION_SHORT_CLICK)
        {
            return selected_category;
        }
        else if (detect_result == WHEEL_ACTION_LONG_CLICK)
        {
            return -1;
        }
        else if (detect_result == WHEEL_ACTION_DOWN)
        {
            if (selected_category < 4)
            {
                selected_category++;
                function_just_started = TRUE;
            }
        }
        else if (detect_result == WHEEL_ACTION_UP)
        {
            if (selected_category != 0)
            {
                selected_category--;
                function_just_started = TRUE;
            }
        }
        
        if (function_just_started != FALSE)
        {            
            /* Reset temp vars */
            memset(text_anim_going_right, FALSE, sizeof(text_anim_going_right));
            memset(text_anim_x_offset, 0, sizeof(text_anim_x_offset));
            memset(scrolling_needed, FALSE, sizeof(scrolling_needed));
            redraw_needed = TRUE;
        }
        
        /* Scrolling logic */
        if (timer_has_timer_expired(TIMER_SCROLLING, FALSE) == TIMER_EXPIRED)
        {
            /* Rearm timer */
            timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
            
            /* Scrolling logic: when enabled, going left or right... */
            for (uint16_t i = 0; i < 5; i++)
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
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            #endif
            sh1122_allow_partial_text_x_draw(&plat_oled_descriptor);
            sh1122_allow_partial_text_y_draw(&plat_oled_descriptor);
            
            /* Loop for 5 always displayed texts */
            temp_y_offset = 0;
            for (uint16_t i = 0; i < 5; i++)
            {
                if (i == 0)
                {
                    /* First string: "default" */
                    custom_fs_get_string_from_file(DEFAULT_CATEGORY_TEXT_ID, &string_to_display, TRUE);
                }
                else
                {
                    /* Any other string: see if the user setup a string, otherwise set default one */
                    nodemgmt_get_category_string(i-1, temp_category_text);
                    
                    /* Check for empty string */
                    if ((temp_category_text[0] == 0xFFFF) || (temp_category_text[0] == 0))
                    {
                        custom_fs_get_string_from_file(DEFAULT_CATEGORY_TEXT_ID+i, &string_to_display, TRUE);
                    }
                    else
                    {
                        string_to_display = temp_category_text;
                    }
                }
                
                /* TBD */
                if (i == selected_category)
                {
                    sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_MEDIUM_15_ID);                    
                    utils_surround_text_with_pointers(string_to_display, ARRAY_SIZE(temp_category_text));
                } 
                else
                {
                    sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_REGULAR_13_ID);
                }
                    
                /* Display string */
                if (scrolling_needed[i] != FALSE)
                {
                    /* Scrolling required: display with the correct X offset */
                    displayed_length = sh1122_put_string_xy(&plat_oled_descriptor, text_anim_x_offset[i], i*12 + temp_y_offset, OLED_ALIGN_LEFT, string_to_display, TRUE);
                        
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
                    displayed_length = sh1122_put_centered_string(&plat_oled_descriptor, i*12 + temp_y_offset, string_to_display, TRUE);
                }
                
                /* Add offset if needed */
                if (i == selected_category)
                {
                    temp_y_offset += 1;
                }
                    
                /* First run: based on the number of chars displayed, set the scrolling needed bool */
                if ((function_just_started != FALSE) && (displayed_length < 0))
                {
                    scrolling_needed[i] = TRUE;
                }
            }
            
            /* Reset display settings */
            sh1122_prevent_partial_text_y_draw(&plat_oled_descriptor);
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
            
            /* Reset Bool */
            function_just_started = FALSE;
            redraw_needed = FALSE;
        }
    }
    
    return -1;
}

/*! \fn     int16_t gui_prompts_favorite_selection_screen(int16_t start_favid, int16_t start_catid)
*   \brief  Favorites selection screen
*   \param  start_favid     If different than -1, select this favID by default
*   \param  start_catid     Start category ID
*   \return -1 when no choice / no credential, otherwise favorite ID (MSB) and category ID (LSB)
*/
int32_t gui_prompts_favorite_selection_screen(int16_t start_favid, int16_t start_catid)
{
    child_cred_node_t* temp_half_cnode_pt;
    cust_char_t* select_login_string;
    parent_node_t temp_half_cnode;
    parent_node_t temp_pnode;
    
    /* Dirty trick */
    temp_half_cnode_pt = (child_cred_node_t*)&temp_half_cnode;
    
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
    
    /* Temp vars for our main loop */
    BOOL navigate_across_categories = nodemgmt_get_current_category() == 0 ? TRUE : FALSE;
    uint16_t temp_parent_addr, temp_child_addr;
    BOOL end_of_list_reached_at_center = FALSE;
    int16_t before_top_of_list_child_index = -1;
    int16_t before_top_of_list_child_cat = -1;
    int16_t top_of_list_child_index = -1;
    int16_t top_of_list_child_cat = -1;
    int16_t center_list_child_index = -1;
    int16_t center_list_child_cat = -1;
    int16_t bottom_list_child_index = -1;
    int16_t bottom_list_child_cat = -1;
    int16_t after_bottom_list_child_index = -1;
    int16_t after_bottom_list_child_cat = -1;
    BOOL animation_just_started = TRUE;
    BOOL first_function_run = TRUE;
    int16_t text_anim_x_offset[4];
    BOOL text_anim_going_right[4];
    int16_t animation_step = 0;
    BOOL redraw_needed = TRUE;
    BOOL action_taken = FALSE;
    int16_t displayed_length;
    BOOL scrolling_needed[4];
    
    /* Reset temp vars */
    memset(text_anim_going_right, FALSE, sizeof(text_anim_going_right));
    memset(text_anim_x_offset, 0, sizeof(text_anim_x_offset));
    memset(scrolling_needed, FALSE, sizeof(scrolling_needed));
    
    /* If favorite start id was specified, populate our vars */
    if (start_favid != -1)
    {
        /* Get what is the index of the previous (empty or not) favorite */
        int16_t prev_cat_index;
        int16_t prev_fav_index;
        nodemgmt_get_prev_favorite_and_category_index(start_catid, start_favid, &prev_cat_index, &prev_fav_index, navigate_across_categories);
        
        /* Each function call checks for out of bounds */
        center_list_child_index = start_favid;
        center_list_child_cat = start_catid;
        int32_t top_list_return = nodemgmt_get_next_non_null_favorite_before_index(prev_fav_index, prev_cat_index, navigate_across_categories);
        top_of_list_child_index = (top_list_return) >> 16;
        top_of_list_child_cat = (int16_t)top_list_return;
        nodemgmt_get_prev_favorite_and_category_index(top_of_list_child_cat, top_of_list_child_index, &prev_cat_index, &prev_fav_index, navigate_across_categories);
        int32_t before_top_list_return = nodemgmt_get_next_non_null_favorite_before_index(prev_fav_index, prev_cat_index, navigate_across_categories);
        before_top_of_list_child_index = (before_top_list_return) >> 16;
        before_top_of_list_child_cat = (int16_t)before_top_list_return;
    }
    
    /* "Select login" string */
    custom_fs_get_string_from_file(SELECT_CREDENTIAL_TEXT_ID, &select_login_string, TRUE);
    
    /* Lines display settings */
    int16_t non_addr_null_index_tbp = NODE_ADDR_NULL+1;
    int16_t* index_to_check_to_display[5] = {&non_addr_null_index_tbp, &top_of_list_child_index, &center_list_child_index, &bottom_list_child_index, &after_bottom_list_child_index};
    int16_t* cat_to_check_to_display[5] = {&non_addr_null_index_tbp, &top_of_list_child_cat, &center_list_child_cat, &bottom_list_child_cat, &after_bottom_list_child_cat};
    cust_char_t* strings_to_be_displayed[4] = {select_login_string, temp_pnode.cred_parent.service, temp_pnode.cred_parent.service, temp_pnode.cred_parent.service};
    uint16_t fonts_to_be_used[4] = {FONT_UBUNTU_REGULAR_16_ID, FONT_UBUNTU_REGULAR_13_ID, FONT_UBUNTU_MEDIUM_15_ID, FONT_UBUNTU_REGULAR_13_ID};
    uint16_t strings_y_positions[4] = {0, LOGIN_SCROLL_Y_FLINE, LOGIN_SCROLL_Y_SLINE, LOGIN_SCROLL_Y_TLINE};
        
    /* Select login string width to set correct underline */
    sh1122_refresh_used_font(&plat_oled_descriptor, fonts_to_be_used[0]);
    uint16_t select_login_width = sh1122_get_string_width(&plat_oled_descriptor, strings_to_be_displayed[0]);
    uint16_t select_login_x_start = plat_oled_descriptor.min_text_x + (plat_oled_descriptor.max_text_x - plat_oled_descriptor.min_text_x - select_login_width)/2;
    
    /* Arm timer for scrolling */
    timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
    
    /* Loop until something has been done */
    while (action_taken == FALSE)
    {
        /* User interaction timeout */
        if (timer_has_timer_expired(TIMER_USER_INTERACTION, TRUE) == TIMER_EXPIRED)
        {
            return -1;
        }
        
        /* Card removed */
        if (smartcard_low_level_is_smc_absent() == RETURN_OK)
        {
            return -1;
        }
        
        /* Call accelerometer routine for (among others) RNG stuff */
        logic_accelerometer_routine();
        
        /* Handle possible power switches */
        logic_power_check_power_switch_and_battery(FALSE);
        
        /* Deal with simple messages */
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        
        /* Check if something has been pressed */
        wheel_action_ret_te detect_result = inputs_get_wheel_action(FALSE, FALSE);
        if (detect_result == WHEEL_ACTION_SHORT_CLICK)
        { 
            return (((int32_t)center_list_child_index) << 16) | center_list_child_cat;
        }
        else if (detect_result == WHEEL_ACTION_LONG_CLICK)
        {
            return -1;
        }
        else if (detect_result == WHEEL_ACTION_DOWN)
        {
            if (end_of_list_reached_at_center == FALSE)
            {
                before_top_of_list_child_index = top_of_list_child_index;
                before_top_of_list_child_cat = top_of_list_child_cat;
                top_of_list_child_index = center_list_child_index;
                top_of_list_child_cat = center_list_child_cat;
                animation_step = ((LOGIN_SCROLL_Y_TLINE-LOGIN_SCROLL_Y_SLINE)/2)*2;
                animation_just_started = TRUE;
            }
        }
        else if (detect_result == WHEEL_ACTION_UP)
        {
            if (top_of_list_child_index >= 0)
            {
                top_of_list_child_index = before_top_of_list_child_index;
                top_of_list_child_cat = before_top_of_list_child_cat;
                animation_step = -((LOGIN_SCROLL_Y_SLINE-LOGIN_SCROLL_Y_FLINE)/2)*2;
                animation_just_started = TRUE;
            }
        }
            
        /* If animation is happening */
        if (animation_just_started != FALSE)
        {
            /* Reset scrolling states except for title */
            memset(&text_anim_going_right[1], 0, sizeof(text_anim_going_right)-sizeof(text_anim_going_right[0]));
            memset(&text_anim_x_offset[1], 0, sizeof(text_anim_x_offset)-sizeof(text_anim_x_offset[0]));
            memset(&scrolling_needed[1], FALSE, sizeof(scrolling_needed)-sizeof(scrolling_needed[0]));
            redraw_needed = TRUE;
        }
        
        /* Scrolling logic */
        if (timer_has_timer_expired(TIMER_SCROLLING, FALSE) == TIMER_EXPIRED)
        {
            /* Rearm timer */
            timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
            
            /* Scrolling logic: when enabled, going left or right... */
            for (uint16_t i = 0; i < 4; i++)
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
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            #endif
            sh1122_allow_partial_text_x_draw(&plat_oled_descriptor);
            sh1122_allow_partial_text_y_draw(&plat_oled_descriptor);
            
            /* Animation: scrolling down, keeping the first of item displayed & fading out */
            sh1122_set_min_display_y(&plat_oled_descriptor, LOGIN_SCROLL_Y_BAR+1);
            if ((animation_step > 0) && (before_top_of_list_child_index >= 0))
            {
                /* Fetch nodes */                
                nodemgmt_read_favorite((uint16_t)before_top_of_list_child_cat, (uint16_t)before_top_of_list_child_index, &temp_parent_addr, &temp_child_addr);
                nodemgmt_read_cred_child_node_except_pwd(temp_child_addr, temp_half_cnode_pt);
                nodemgmt_read_parent_node(temp_parent_addr, &temp_pnode, TRUE);
                
                /* Generate string */
                if (utils_strlen(temp_half_cnode_pt->login) != 0)
                {
                    utils_concatenate_strings_with_slash(temp_pnode.cred_parent.service, temp_half_cnode_pt->login, MEMBER_ARRAY_SIZE(parent_cred_node_t, service));
                }
                
                /* Display fading out login */
                sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_REGULAR_13_ID);
                sh1122_put_centered_string(&plat_oled_descriptor, LOGIN_SCROLL_Y_FLINE-(((LOGIN_SCROLL_Y_TLINE-LOGIN_SCROLL_Y_SLINE)/2)*2)+animation_step, temp_pnode.cred_parent.service, TRUE);
            }
            sh1122_reset_lim_display_y(&plat_oled_descriptor);
            
            /* Bar below the title */
            sh1122_draw_rectangle(&plat_oled_descriptor, select_login_x_start, LOGIN_SCROLL_Y_BAR, select_login_width, 1, 0xFF, TRUE);
            
            /* Loop for 4 always displayed texts: title then 3 list items */
            for (uint16_t i = 0; i < 4; i++)
            {
                if (i == 0)
                {
                    sh1122_reset_lim_display_y(&plat_oled_descriptor);
                }
                
                /* Check if we should display it */
                if (*(index_to_check_to_display[i]) >= 0)
                {
                    /* Load the right font */
                    sh1122_refresh_used_font(&plat_oled_descriptor, fonts_to_be_used[i]);
                    
                    /* Fetch node if needed */
                    if (i > 0)
                    {
                        /* Fetch nodes */
                        nodemgmt_read_favorite((uint16_t)*(cat_to_check_to_display[i]), (uint16_t)*(index_to_check_to_display[i]), &temp_parent_addr, &temp_child_addr);
                        nodemgmt_read_cred_child_node_except_pwd(temp_child_addr, temp_half_cnode_pt);
                        nodemgmt_read_parent_node(temp_parent_addr, &temp_pnode, TRUE);
                        
                        /* Generate string */
                        if (utils_strlen(temp_half_cnode_pt->login) != 0)
                        {
                            utils_concatenate_strings_with_slash(temp_pnode.cred_parent.service, temp_half_cnode_pt->login, MEMBER_ARRAY_SIZE(parent_cred_node_t, service));
                        }                            
                    }
                    
                    /* Surround center of list item */
                    if (i == 2)
                    {
                        utils_surround_text_with_pointers(temp_pnode.cred_parent.service, MEMBER_ARRAY_SIZE(parent_cred_node_t, service));
                    }
                    
                    /* First address: store the "before top address */
                    if (i == 1)
                    {
                        if (top_of_list_child_index != -1)
                        {
                            /* Get what is the index of the previous (empty or not) favorite */
                            int16_t prev_cat_index;
                            int16_t prev_fav_index;
                            nodemgmt_get_prev_favorite_and_category_index(top_of_list_child_cat, top_of_list_child_index, &prev_cat_index, &prev_fav_index, navigate_across_categories);
                            
                            /* Get before top address */
                            int32_t before_top_list_return = nodemgmt_get_next_non_null_favorite_before_index(prev_fav_index, prev_cat_index, navigate_across_categories);
                            before_top_of_list_child_index = (before_top_list_return) >> 16;
                            before_top_of_list_child_cat = (int16_t)before_top_list_return;
                        }
                        else
                        {
                            before_top_of_list_child_index = -1;
                        }
                    }
                    
                    /* Last address: store correct bool */
                    if (i == 3)
                    {
                        end_of_list_reached_at_center = FALSE;
                    }
            
                    /* Y offset for animations */
                    int16_t yoffset = 0;
                    if (i > 0)
                    {
                        yoffset = animation_step;
                    }
                    
                    /* Display string */
                    if (scrolling_needed[i] != FALSE)
                    {                        
                        /* Scrolling required: display with the correct X offset */
                        displayed_length = sh1122_put_string_xy(&plat_oled_descriptor, text_anim_x_offset[i], strings_y_positions[i]+yoffset, OLED_ALIGN_LEFT, strings_to_be_displayed[i], TRUE);
                
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
                        displayed_length = sh1122_put_centered_string(&plat_oled_descriptor, strings_y_positions[i]+yoffset, strings_to_be_displayed[i], TRUE);
                    }
                    
                    /* First run: based on the number of chars displayed, set the scrolling needed bool */
                    if ((animation_just_started != FALSE) && (displayed_length < 0))
                    {
                        scrolling_needed[i] = TRUE;
                    }
                    
                    /* Search & store next favorite address (our array has an extra element) */
                    if (i > 0)
                    {
                        /* Get what is the index of the next (empty or not) favorite */
                        int16_t next_cat_index;
                        int16_t next_fav_index;
                        nodemgmt_get_next_favorite_and_category_index(*(cat_to_check_to_display[i]), *(index_to_check_to_display[i]), &next_cat_index, &next_fav_index, navigate_across_categories);
                        
                        int32_t next_address_return = nodemgmt_get_next_non_null_favorite_after_index(next_fav_index, next_cat_index, navigate_across_categories);
                        *(index_to_check_to_display[i+1]) = (next_address_return) >> 16;
                        *(cat_to_check_to_display[i+1]) = (int16_t)next_address_return;  
                    }
                    
                    /* Last item & animation scrolling up: display upcoming item */
                    if (i == 3)
                    {
                        if ((animation_step < 0) && (*(index_to_check_to_display[i+1]) >= 0))
                        {
                            /* Fetch nodes */
                            nodemgmt_read_favorite((uint16_t)after_bottom_list_child_cat, (uint16_t)after_bottom_list_child_index, &temp_parent_addr, &temp_child_addr);
                            nodemgmt_read_cred_child_node_except_pwd(temp_child_addr, temp_half_cnode_pt);
                            nodemgmt_read_parent_node(temp_parent_addr, &temp_pnode, TRUE);
                            
                            /* Display fading out login */
                            sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_REGULAR_13_ID);
                            sh1122_put_centered_string(&plat_oled_descriptor, LOGIN_SCROLL_Y_TLINE+(((LOGIN_SCROLL_Y_SLINE-LOGIN_SCROLL_Y_FLINE)/2)*2)+animation_step, temp_pnode.cred_parent.service, TRUE);
                        }
                    }
                }
                else
                {
                    /* Check for node to display failed */
                    if (i == 1)
                    {
                        /* Couldn't display the top of the list, means our center child is the first child */
                        int32_t center_list_return = nodemgmt_get_next_non_null_favorite_after_index(0, nodemgmt_get_current_category(), navigate_across_categories);
                        center_list_child_index = (center_list_return) >> 16;
                        center_list_child_cat = (int16_t)center_list_return;
                        
                        /* Check for no favorites */
                        if (center_list_child_index < 0)
                        {
                            sh1122_prevent_partial_text_y_draw(&plat_oled_descriptor);
                            sh1122_prevent_partial_text_x_draw(&plat_oled_descriptor);
                            sh1122_reset_lim_display_y(&plat_oled_descriptor);
                            gui_prompts_display_information_on_screen_and_wait(NO_FAVORITES_TEXT_ID, DISP_MSG_INFO, FALSE);
                            return -1;
                        }
                    }
                    if (i == 3)
                    {
                        /* No last item to display, end of list reached at center */
                        end_of_list_reached_at_center = TRUE;
                    }
                }
                
                if (i == 0)
                {
                    sh1122_set_min_display_y(&plat_oled_descriptor, LOGIN_SCROLL_Y_BAR+1);
                }
            }             
            
            /* Reset display settings */
            sh1122_prevent_partial_text_y_draw(&plat_oled_descriptor);
            sh1122_prevent_partial_text_x_draw(&plat_oled_descriptor);
            sh1122_reset_lim_display_y(&plat_oled_descriptor);
            
            /* Reset animation just started var */
            animation_just_started = FALSE;
            
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
            
            /* Animation processing */
            if (animation_step > 0)
            {
                animation_step-=2;
            }
            else if (animation_step < 0)
            {
                animation_step+=2;
            }
            else
            {
                redraw_needed = FALSE;                
            }
        }        
    }
    
    return -1;
}

/*! \fn     gui_prompts_select_language_or_keyboard_layout(BOOL layout_choice, BOOL ignore_timeout_if_usb_powered, BOOL ignore_card_removal, BOOL usb_layout_choice)
*   \brief  select language or keyboard layout, depending on the bool
*   \param  layout_choice                   TRUE to choose layout, FALSE for language
*   \param  ignore_timeout_if_usb_powered   TRUE to discard timeout if we are usb powered
*   \param  ignore_card_removal             TRUE to ignore card removal
*   \param  usb_layout_choice               If layout_choice is TRUE, selects either usb or ble layout choice
*   \return RETURN_OK if it was a choice, NOK if timeout or card removal
*/
ret_type_te gui_prompts_select_language_or_keyboard_layout(BOOL layout_choice, BOOL ignore_timeout_if_usb_powered, BOOL ignore_card_removal, BOOL usb_layout_choice)
{    
    _Static_assert(CUSTOM_FS_KEYBOARD_DESC_LGTH >= MEMBER_ARRAY_SIZE(language_map_entry_t,language_descr), "Incorrect buffer length");
    cust_char_t string_buffer[CUSTOM_FS_KEYBOARD_DESC_LGTH + 4];
    uint8_t cur_item_id;
    uint16_t nb_items;
    
    /* Language / layout logic */
    if (layout_choice == FALSE)
    {
        cur_item_id = custom_fs_get_current_language_id();
        nb_items = custom_fs_get_number_of_languages();
    }
    else
    {
        cur_item_id = custom_fs_get_current_layout_id(usb_layout_choice);
        nb_items = custom_fs_get_number_of_keyb_layouts();
    }
    
    /* Sanity checks */
    if (cur_item_id >= nb_items)
    {
        cur_item_id = 0;
    }
    
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
    
    /* Temp vars for our main loop */
    int16_t first_item_id_in_list = ((int16_t)cur_item_id) - 1;
    cust_char_t* select_language_string;
    int16_t last_item_id_in_list = -1;
    BOOL first_function_run = TRUE;
    int16_t animation_step = 0;
    BOOL redraw_needed = TRUE;
    
    /* Lines display settings */
    uint16_t fonts_to_be_used[4] = {FONT_UBUNTU_REGULAR_16_ID, FONT_UBUNTU_REGULAR_13_ID, FONT_UBUNTU_MEDIUM_15_ID, FONT_UBUNTU_REGULAR_13_ID};
    uint16_t strings_y_positions[4] = {0, LOGIN_SCROLL_Y_FLINE, LOGIN_SCROLL_Y_SLINE, LOGIN_SCROLL_Y_TLINE};
    
    /* Arm timer for scrolling */
    timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
    
    /* Loop until something has been done */
    while (TRUE)
    {
        /* User interaction timeout */
        if (timer_has_timer_expired(TIMER_USER_INTERACTION, TRUE) == TIMER_EXPIRED)
        {
            if ((ignore_timeout_if_usb_powered != FALSE) && (platform_io_is_usb_3v3_present() != FALSE))
            {
                /* Just rearm timer */
                logic_device_activity_detected();
            }
            else
            {
                return RETURN_NOK;
            }                
        }
        
        /* Card removed */
        if ((smartcard_low_level_is_smc_absent() == RETURN_OK) && (ignore_card_removal == FALSE))
        {
            return RETURN_NOK;
        }
        
        /* Call accelerometer routine for (among others) RNG stuff */
        logic_accelerometer_routine();
        
        /* Handle possible power switches */
        logic_power_check_power_switch_and_battery(FALSE);
        
        /* Deal with simple messages */
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        
        /* Check if something has been pressed */
        wheel_action_ret_te detect_result = inputs_get_wheel_action(FALSE, FALSE);
        if (detect_result == WHEEL_ACTION_SHORT_CLICK)
        {
            if (layout_choice == FALSE)
            {
                custom_fs_set_current_language(first_item_id_in_list+1);
            }
            else
            {
                custom_fs_set_current_keyboard_id(first_item_id_in_list+1, usb_layout_choice);
            }
            return RETURN_OK;
        }
        else if (detect_result == WHEEL_ACTION_LONG_CLICK)
        {
            return RETURN_NOK;
        }
        else if (detect_result == WHEEL_ACTION_DOWN)
        {
            if ((last_item_id_in_list != -1) && (first_item_id_in_list + 1 != nb_items-1))
            {
                animation_step = ((LOGIN_SCROLL_Y_SLINE-LOGIN_SCROLL_Y_FLINE)/2)*2;
                last_item_id_in_list = -1;
                first_item_id_in_list++;
            }
        }
        else if (detect_result == WHEEL_ACTION_UP)
        {
            if (first_item_id_in_list != -1)
            {
                animation_step = -((LOGIN_SCROLL_Y_SLINE-LOGIN_SCROLL_Y_FLINE)/2)*2;
                last_item_id_in_list = -1;
                first_item_id_in_list--;
            }
        }
        
        /* Scrolling logic */
        if (timer_has_timer_expired(TIMER_SCROLLING, FALSE) == TIMER_EXPIRED)
        {
            /* Rearm timer */
            timer_start_timer(TIMER_SCROLLING, SCROLLING_DEL);
            redraw_needed = TRUE;
        }
        
        /* Redraw if needed */
        if (redraw_needed != FALSE)
        {
            /* Clear frame buffer, set display settings */
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            #endif
            sh1122_allow_partial_text_x_draw(&plat_oled_descriptor);
            sh1122_allow_partial_text_y_draw(&plat_oled_descriptor);
            
            /* Animation: scrolling down, keeping the first of item displayed & fading out */
            sh1122_set_min_display_y(&plat_oled_descriptor, LOGIN_SCROLL_Y_BAR+1);
            if ((animation_step > 0) && (first_item_id_in_list > 0))
            {         
                /* Display fading out language */
                if (layout_choice == FALSE)
                {
                    custom_fs_get_language_description((uint8_t)(first_item_id_in_list-1), string_buffer);
                }         
                else
                {
                    custom_fs_get_keyboard_descriptor_string((uint8_t)(first_item_id_in_list-1), string_buffer);
                }         
                sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_REGULAR_13_ID);
                sh1122_put_centered_string(&plat_oled_descriptor, LOGIN_SCROLL_Y_FLINE-(((LOGIN_SCROLL_Y_TLINE-LOGIN_SCROLL_Y_SLINE)/2)*2)+animation_step, string_buffer, TRUE);
            }
            sh1122_reset_lim_display_y(&plat_oled_descriptor);
            
            /* Main frame title */
            sh1122_reset_lim_display_y(&plat_oled_descriptor);
            if (layout_choice == FALSE)
            {      
                custom_fs_set_current_language(first_item_id_in_list+1);
            }                
            sh1122_refresh_used_font(&plat_oled_descriptor, fonts_to_be_used[0]);
            if (layout_choice == FALSE)
            {
                custom_fs_get_string_from_file(SELECT_LANGUAGE_TEXT_ID, &select_language_string, TRUE);
            }
            else
            {
                if (usb_layout_choice == FALSE)
                {
                    custom_fs_get_string_from_file(SELECT_BLE_LAYOUT_TEXT_ID, &select_language_string, TRUE);
                } 
                else
                {
                    custom_fs_get_string_from_file(SELECT_USB_LAYOUT_TEXT_ID, &select_language_string, TRUE);
                }             
            }
            sh1122_put_centered_string(&plat_oled_descriptor, strings_y_positions[0], select_language_string, TRUE);
            sh1122_set_min_display_y(&plat_oled_descriptor, LOGIN_SCROLL_Y_BAR+1);
            if (layout_choice == FALSE)
            {
                custom_fs_set_current_language(cur_item_id);
            }
            
            /* String width to set correct underline */
            uint16_t string_width = sh1122_get_string_width(&plat_oled_descriptor, select_language_string);
            uint16_t underline_x_start = plat_oled_descriptor.min_text_x + (plat_oled_descriptor.max_text_x - plat_oled_descriptor.min_text_x - string_width)/2;
            
            /* Bar below the title */
            sh1122_draw_rectangle(&plat_oled_descriptor, underline_x_start, LOGIN_SCROLL_Y_BAR, string_width, 1, 0xFF, TRUE);
            
            /* Loop for next 3 languages */
            for (uint16_t i = 0; i < 3; i++)
            {                
                /* Check if we can display it */
                if (((first_item_id_in_list + i) < nb_items) && ((first_item_id_in_list + i) >= 0))
                {
                    last_item_id_in_list = i + first_item_id_in_list;
                    if (layout_choice == FALSE)
                    {
                        custom_fs_get_language_description((uint8_t)(first_item_id_in_list+i), string_buffer);
                    }     
                    else
                    {
                        custom_fs_get_keyboard_descriptor_string((uint8_t)(first_item_id_in_list+i), string_buffer);                
                    }                   
                    sh1122_refresh_used_font(&plat_oled_descriptor, fonts_to_be_used[i+1]);
                    
                    /* Surround center of list item */
                    if (i == 1)
                    {
                        utils_surround_text_with_pointers(string_buffer, ARRAY_SIZE(string_buffer));
                    }
                    
                    /* Display language string */
                    sh1122_put_centered_string(&plat_oled_descriptor, strings_y_positions[i+1]+animation_step, string_buffer, TRUE);
                    
                    /* Last item & animation scrolling up: display upcoming item */
                    if (i == 2)
                    {
                        if ((animation_step < 0) && ((last_item_id_in_list + 1) < nb_items))
                        {                            
                            /* Display fading out language */
                            if (layout_choice == FALSE)
                            {
                                custom_fs_get_language_description((uint8_t)(last_item_id_in_list+1), string_buffer);
                            }
                            else
                            {
                                custom_fs_get_keyboard_descriptor_string((uint8_t)(last_item_id_in_list+1), string_buffer);                     
                            }
                            sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_REGULAR_13_ID);                            
                            sh1122_put_centered_string(&plat_oled_descriptor, LOGIN_SCROLL_Y_TLINE+(((LOGIN_SCROLL_Y_SLINE-LOGIN_SCROLL_Y_FLINE)/2)*2)+animation_step, string_buffer, TRUE);
                        }
                    }
                }
            }
            
            /* Reset display settings */
            sh1122_prevent_partial_text_y_draw(&plat_oled_descriptor);
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
            
            /* Animation processing */
            if (animation_step > 0)
            {
                animation_step-=2;
            }
            else if (animation_step < 0)
            {
                animation_step+=2;
            }
            else
            {
                redraw_needed = FALSE;
            }
        }
    }
}
