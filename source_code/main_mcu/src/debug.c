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
/*!  \file     debug.c
*    \brief    Debug functions for our platform
*    Created:  21/04/2018
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include <asf.h>
#include "comms_hid_msgs_debug.h"
#include "logic_accelerometer.h"
#include "smartcard_highlevel.h"
#include "smartcard_lowlevel.h"
#include "functional_testing.h"
#include "platform_defines.h"
#include "logic_smartcard.h"
#include "gui_dispatcher.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "gui_prompts.h"
#include "platform_io.h"
#include "logic_power.h"
#include "dataflash.h"
#include "custom_fs.h"
#include "nodemgmt.h"
#include "lis2hh12.h"
#include "text_ids.h"
#include "sh1122.h"
#include "inputs.h"
#include "debug.h"
#include "main.h"
#include "dma.h"
#include "rng.h"


/*! \fn     debug_array_to_hex_u8string(uint8_t* array, uint8_t* string, uint16_t length)
*   \brief  Generate a string to display an array contents
*   \param  array   Pointer to the array
*   \param  string  Pointer to the u8 string
*   \param  length  Length of the array
*   \note   string length should be twice the size of the array
*/
void debug_array_to_hex_u8string(uint8_t* array, uint8_t* string, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++)
    {
        sprintf((char*)&string[i*2], "%02x", array[i]);
    }
}

/*! \fn     debug_test_prompts(void)
*   \brief  Display several prompts or notifications to check code
*/
void debug_test_prompts(void)
{
    volatile uint16_t pinpin;
    
    /* Set language */    
    custom_fs_set_current_language(0);
    
    /* One line prompt */
    gui_prompts_ask_for_one_line_confirmation(CREATE_NEW_USER_TEXT_ID, TRUE, FALSE, TRUE);
        
    /* 2 lines prompt */
    cust_char_t* two_line_prompt_1 = (cust_char_t*)u"Delete Service?";
    cust_char_t* two_line_prompt_2 = (cust_char_t*)u"myreallysuperextralongwebsite.com";
    confirmationText_t conf_text_2_lines = {.lines[0]=two_line_prompt_1, .lines[1]=two_line_prompt_2};
    gui_prompts_ask_for_confirmation(2, &conf_text_2_lines, TRUE, TRUE, FALSE);
    
    /* 3 lines prompt */
    cust_char_t* three_line_prompt_1 = (cust_char_t*)u"themooltipass.com";
    cust_char_t* three_line_prompt_2 = (cust_char_t*)u"Login With:";
    cust_char_t* three_line_prompt_3 = (cust_char_t*)u"mysuperextralonglogin@mydomain.com";
    confirmationText_t conf_text_3_lines = {.lines[0]=three_line_prompt_1, .lines[1]=three_line_prompt_2, .lines[2]=three_line_prompt_3};
    gui_prompts_ask_for_confirmation(3, &conf_text_3_lines, TRUE, TRUE, FALSE);
    
    /* 4 lines prompt */
    cust_char_t* four_line_prompt_1 = (cust_char_t*)u"SSH Daemon";
    cust_char_t* four_line_prompt_2 = (cust_char_t*)u"myserver.com";
    cust_char_t* four_line_prompt_3 = (cust_char_t*)u"Login With:";
    cust_char_t* four_line_prompt_4 = (cust_char_t*)u"mysuperextralonglogin@mydomain.com";
    confirmationText_t conf_text_4_lines = {.lines[0]=four_line_prompt_1, .lines[1]=four_line_prompt_2, .lines[2]=four_line_prompt_3, .lines[3]=four_line_prompt_4};
    gui_prompts_ask_for_confirmation(4, &conf_text_4_lines, TRUE, TRUE, FALSE);
    
    /* Notifications */
    gui_prompts_display_information_on_screen_and_wait(35, DISP_MSG_INFO, FALSE);
    gui_prompts_display_information_on_screen_and_wait(35, DISP_MSG_ACTION, FALSE);
    gui_prompts_display_information_on_screen_and_wait(35, DISP_MSG_WARNING, FALSE);
    
    /* Pin get */
    gui_prompts_get_user_pin(&pinpin, 32);    
}

/*! \fn     debug_debug_menu(void)
*   \brief  Debug menu
*/
void debug_debug_menu(void)
{
    wheel_action_ret_te wheel_user_action;
    int16_t selected_item = 0;
    BOOL redraw_needed = TRUE;
    
    while(1)
    {
        /* Still deal with comms */
        comms_aux_mcu_routine(MSG_NO_RESTRICT);
        
        /* Draw menu */
        if (redraw_needed != FALSE)
        {
            /* Set Emergency Font */
            sh1122_set_emergency_font(&plat_oled_descriptor);
            
            /* Clear screen */
            redraw_needed = FALSE;
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            #else
            sh1122_clear_current_screen(&plat_oled_descriptor);
            #endif
            
            /* Item selection */
            if (selected_item > 19)
            {
                selected_item = 0;
            }
            else if (selected_item < 0)
            {
                selected_item = 19;
            }
            
            sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Debug Menu", TRUE);
            
            /* Print items */
            if (selected_item < 4)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Time / Accelerometer / Battery", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Main and Aux MCU Info", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Aux MCU BLE Info", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"NiMH Status", TRUE);
            }
            else if (selected_item < 8)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Display All Fonts Glyphs", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Setup Developper Card", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Test Pattern Display", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"Reset Device", TRUE);            
            }
            else if (selected_item < 12)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Smartcard Debug", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Smartcard Test", TRUE);                
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Main MCU Flash", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"Aux MCU Flash", TRUE);
            }
            else if (selected_item < 16)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"RF Frequency Sweep", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Functional Test", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Switch Off", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"Sleep", TRUE);
            }
            else
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Battery Recondition", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Battery Test", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Stack Usage", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"Reset Settings", TRUE);
            }
            
            /* Cursor */
            sh1122_put_string_xy(&plat_oled_descriptor, 0, 14 + (selected_item%4)*10, OLED_ALIGN_LEFT, u"-", TRUE);
            
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_flush_frame_buffer(&plat_oled_descriptor);
            #endif
        }
        
        /* Get user action */
        wheel_user_action = inputs_get_wheel_action(FALSE, FALSE);
        
        /* action depending on scroll */
        if (wheel_user_action == WHEEL_ACTION_UP)
        {
            redraw_needed = TRUE;
            selected_item--;
        }
        else if (wheel_user_action == WHEEL_ACTION_DOWN)
        {
            redraw_needed = TRUE;
            selected_item++;
        }
        else if (wheel_user_action == WHEEL_ACTION_LONG_CLICK)
        {
            return;
        }
        else if (wheel_user_action == WHEEL_ACTION_SHORT_CLICK)
        {
            if (selected_item == 0)
            {
                debug_debug_screen();
            }
            else if (selected_item == 1)
            {
                debug_mcu_and_aux_info();
            }
            else if (selected_item == 2)
            {
                debug_atbtlc_info();
            }
            else if (selected_item == 3)
            {
                debug_nimh_status();
            }
            else if (selected_item == 4)
            {
                debug_glyphs_show();
            }
            else if (selected_item == 5)
            {
                debug_setup_dev_card();
            }
            else if (selected_item == 6)
            {
                debug_test_pattern_display();
            }
            else if (selected_item == 7)
            {
                debug_reset_device();
            }
            else if (selected_item == 8)
            {
                debug_smartcard_info();
            }
            else if (selected_item == 9)
            {
                debug_smartcard_test();
            }
            else if (selected_item == 10)
            {
                custom_fs_set_device_flag_value(DEVICE_WENT_THROUGH_BOOTLOADER_FLAG_ID, TRUE);
                custom_fs_settings_set_fw_upgrade_flag();
                logic_aux_mcu_disable_ble(TRUE);
                main_reboot();
            }
            else if (selected_item == 11)
            {
                logic_aux_mcu_flash_firmware_update(TRUE);
            }
            else if (selected_item == 12)
            {
                debug_rf_freq_sweep();
            }
            else if (selected_item == 13)
            {
                sh1122_clear_current_screen(&plat_oled_descriptor);
                functional_testing_start(FALSE);
            }
            else if (selected_item == 14)
            {
                /* Check for USB power */
                if (platform_io_is_usb_3v3_present_raw() != FALSE)
                {
                    sh1122_clear_current_screen(&plat_oled_descriptor);
                    sh1122_put_string_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_CENTER, u"Remove USB cable", FALSE);
                    while((platform_io_is_usb_3v3_present_raw() != FALSE));
                }
                logic_power_power_down_actions();           // Power off actions
                sh1122_oled_off(&plat_oled_descriptor);     // Display off command    
                platform_io_power_down_oled();              // Switch off stepup            
                platform_io_set_wheel_click_pull_down();    // Pull down on wheel click to slowly discharge capacitor
                timer_delay_ms(100);                        // From OLED datasheet wait before removing 3V3
                platform_io_set_wheel_click_low();          // Completely discharge cap
                timer_delay_ms(10);                         // Wait a tad
                platform_io_disable_switch_and_die();       // Die!
            }
            else if (selected_item == 15)
            {
                main_standby_sleep();
            }
            else if (selected_item == 16)
            {
                logic_power_battery_recondition();
            }
            else if (selected_item == 17)
            {
                debug_test_battery();
            }
            else if (selected_item == 18)
            {
                debug_stack_info();
            }
            else if (selected_item == 19)
            {
                custom_fs_hard_reset_settings();
            }
            redraw_needed = TRUE;
        }
    }
}

/*! \fn     debug_kickstarter_video(void)
*   \brief  Kickstarter video: animation, then sleep, click to wakeup, screen off, animation when movement detection
*/
void debug_kickstarter_video(void)
{
    logic_accelerometer_set_x_movement_detection_wakeup();
    inputs_clear_detections();
    
    uint16_t sleep_counter = 0;
    while (TRUE)
    {
        /* One full animation */
        for (uint16_t i = GUI_ANIMATION_FFRAME_ID; i < GUI_ANIMATION_NBFRAMES; i++)
        {
            timer_start_timer(TIMER_ANIMATIONS, 88);
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, i, FALSE);
            while(timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_RUNNING)
            {
                logic_accelerometer_routine();
                
                /* click during animation to switch off */
                if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
                {
                    logic_power_power_down_actions();           // Power off actions
                    sh1122_oled_off(&plat_oled_descriptor);     // Display off command
                    platform_io_power_down_oled();              // Switch off stepup
                    platform_io_set_wheel_click_pull_down();    // Pull down on wheel click to slowly discharge capacitor
                    timer_delay_ms(100);                        // From OLED datasheet wait before removing 3V3
                    platform_io_set_wheel_click_low();          // Completely discharge cap
                    timer_delay_ms(10);                         // Wait a tad
                    platform_io_disable_switch_and_die();       // Die!
                }
            }
        }
        sh1122_clear_current_screen(&plat_oled_descriptor);
        
        /* Wait for end of x axis movement */
        while(logic_accelerometer_routine() == ACC_DET_MOVEMENT);
        
        /* Go to sleep */
        if ((platform_io_is_usb_3v3_present_raw() == FALSE) && (sleep_counter > 7))
        {
            sleep_counter = 0;
            sh1122_oled_off(&plat_oled_descriptor);
            platform_io_power_down_oled();
            logic_power_check_power_switch_and_battery(TRUE);
            main_standby_sleep();
            platform_io_get_voledin_conversion_result_and_trigger_conversion();
        }
        else
        {
            sleep_counter++;
        }
        
        /* Wait for x axis movement */
        while(logic_accelerometer_routine() != ACC_DET_MOVEMENT);      
    }    
}

/*! \fn     debug_always_bluetooth_enable_and_click_to_send_cred(void)
*   \brief  A routine to click to send a test string through ble
*/
void debug_always_bluetooth_enable_and_click_to_send_cred(void)
{
    aux_mcu_message_t* temp_tx_message_pt;
    
    /* Set undefined settings in case this function is called early during device init */
    custom_fs_set_undefined_settings();
    
    /* Clear all bonding information */
    nodemgmt_delete_all_bluetooth_bonding_information();
    
    inputs_clear_detections();
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_put_error_string(&plat_oled_descriptor, u"Click to upload bundle, otherwise scroll");
    if (inputs_get_wheel_action(TRUE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
    {
        BOOL bundle_uploaded = FALSE;
        while (bundle_uploaded == FALSE)
        {            
            /* Check for reindex bundle message */
            if (comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_BUNDLE) == HID_REINDEX_BUNDLE_RCVD)
            {
                /* Try to init our file system */
                custom_fs_init();
                bundle_uploaded = TRUE;
            }
        }
    }        
    
    sh1122_clear_current_screen(&plat_oled_descriptor);    
    sh1122_put_error_string(&plat_oled_descriptor, u"Enabling bluetooth...");
    logic_aux_mcu_enable_ble(TRUE);
    
    /* Send command to enable pairing to aux MCU */
    temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_BLE_CMD);
    temp_tx_message_pt->ble_message.message_id = BLE_MESSAGE_ENABLE_PAIRING;
    temp_tx_message_pt->payload_length1 = sizeof(temp_tx_message_pt->ble_message.message_id);
    comms_aux_mcu_send_message(temp_tx_message_pt);
    
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_put_error_string(&plat_oled_descriptor, u"Waiting for pairing");    
    BOOL pairing_done = FALSE;
    while (pairing_done == FALSE)
    {
        comms_msg_rcvd_te received_packet = comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_BOND_STORE);
        if (received_packet == BLE_BOND_STORE_RCVD)
        {
            /* We received a bonding storage message */
            pairing_done = TRUE;
        }
    }
    
    /* Reset emergency font */
    sh1122_set_emergency_font(&plat_oled_descriptor);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_put_error_string(&plat_oled_descriptor, u"Click to send creds");
    
    /* Main loop */
    while(TRUE)
    {
        if (inputs_get_wheel_action(FALSE, TRUE) == WHEEL_ACTION_SHORT_CLICK)
        {
            aux_mcu_message_t* typing_message_to_be_sent;
            aux_mcu_message_t* temp_rx_message;
            
            /* Type "test" */
            typing_message_to_be_sent = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_KEYBOARD_TYPE);
            typing_message_to_be_sent->payload_length1 = 16;
            typing_message_to_be_sent->keyboard_type_message.keyboard_symbols[0] = 23;
            typing_message_to_be_sent->keyboard_type_message.keyboard_symbols[1] = 8;
            typing_message_to_be_sent->keyboard_type_message.keyboard_symbols[2] = 22;
            typing_message_to_be_sent->keyboard_type_message.keyboard_symbols[3] = 23;
            typing_message_to_be_sent->keyboard_type_message.delay_between_types = 25;
            typing_message_to_be_sent->keyboard_type_message.interface_identifier = 1;
            comms_aux_mcu_send_message(typing_message_to_be_sent);
            
            /* Wait for typing status */
            while(comms_aux_mcu_active_wait(&temp_rx_message, AUX_MCU_MSG_TYPE_KEYBOARD_TYPE, FALSE, -1) != RETURN_OK){}
            
            /* Rearm DMA RX */
            comms_aux_arm_rx_and_clear_no_comms();
        }
        
        /* Deal with comms */
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
    }
}

/*! \fn     debug_battery_repair(void)
*   \brief  Our custom sauce to repair damaged NiMH battery
*   \note   Completely based on our experience
*/
void debug_battery_repair(void)
{
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* Needs to be battery powered */
    if (platform_io_is_usb_3v3_present_raw() == FALSE)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Device must be USB powered");
        DELAYMS(2000);
        return;
    }
    
    /* Check for charging */
    if (logic_power_is_battery_charging() != FALSE)
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_STOP_CHARGE);
        comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_CHARGE_STOPPED);
        logic_power_set_battery_charging_bool(FALSE, FALSE);
    }

    for (uint16_t charge_discharge_cycles = 0; charge_discharge_cycles < 2; charge_discharge_cycles++)
    {
        /* From previous loop iteration: wait for little charge */
        if (charge_discharge_cycles != 0)
        {
            timer_delay_ms(30000);
            comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_STOP_CHARGE);
            comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_CHARGE_STOPPED);
            logic_power_set_battery_charging_bool(FALSE, FALSE);
        }

        /* Switch on & off power to the oled stepup in a very abrupt manner to create current transcients on battery */
        for (uint16_t i = 0; i < 20; i++)
        {
            platform_io_disable_3v3_to_oled_stepup();
            sh1122_oled_off(&plat_oled_descriptor);
            platform_io_assert_oled_reset();
            timer_delay_ms(15);
            platform_io_transcienty_battery_oled_power_up();
            sh1122_init_display(&plat_oled_descriptor, TRUE);
            sh1122_put_error_string(&plat_oled_descriptor, u"Blinking");
            timer_delay_ms(15);
        }

        /* Switch back to USB supply */
        sh1122_oled_off(&plat_oled_descriptor);
        platform_io_disable_vbat_to_oled_stepup();
        platform_io_assert_oled_reset();
        timer_delay_ms(15);
        platform_io_power_up_oled(TRUE);
        sh1122_init_display(&plat_oled_descriptor, TRUE);
        sh1122_put_error_string(&plat_oled_descriptor, u"Charging");

        /* Start danger change */
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_NIMH_DANGER_CHARGE);
        comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_CHARGE_STARTED);
        logic_power_set_battery_charging_bool(TRUE, FALSE);
    }
}

/*! \fn     debug_battery_recondition(void)
*   \brief  Fully discharge and charge the battery
*/
void debug_battery_recondition(void)
{
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* Needs to be battery powered */
    if (platform_io_is_usb_3v3_present_raw() == FALSE)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Device must be USB powered");
        DELAYMS(2000);
        return;
    }
    
    /* Check for charging */
    if (logic_power_is_battery_charging() != FALSE)
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_STOP_CHARGE);
        comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_CHARGE_STOPPED);
        logic_power_set_battery_charging_bool(FALSE, FALSE);
    }
    
    /* Switch to battery power for screen */
    sh1122_oled_off(&plat_oled_descriptor);
    platform_io_disable_3v3_to_oled_stepup();
    platform_io_assert_oled_reset();
    timer_delay_ms(15);
    platform_io_power_up_oled(FALSE);
    sh1122_init_display(&plat_oled_descriptor, TRUE);
    
    /* Display animation until 1V cell */
    uint16_t current_vbat = 0xFFFF;
    while (current_vbat > (1000*8192/3300))
    {
        #ifdef OLED_INTERNAL_FRAME_BUFFER
            uint8_t* frame_buffer_pt = (uint8_t*)&plat_oled_descriptor.frame_buffer[0][0];
            sh1122_check_for_flush_and_terminate(&plat_oled_descriptor);
            for (uint16_t i = 0; i < sizeof(plat_oled_descriptor.frame_buffer)/8; i++)
            {
                uint16_t rng_byte = rng_get_random_uint8_t();
                for (uint16_t j = 0; j < 8; j++)
                {
                    if ((rng_byte & (1 << j)) != 0)
                    {
                        frame_buffer_pt[i*8+j] = 0x08;
                    }
                    else
                    {
                        frame_buffer_pt[i*8+j] = 0x00;                        
                    }
                }
            }
            sh1122_flush_frame_buffer(&plat_oled_descriptor);
        #else
            for (uint16_t i = GUI_ANIMATION_FFRAME_ID; i < GUI_ANIMATION_NBFRAMES; i++)
            {
                timer_start_timer(TIMER_WAIT_FUNCTS, 28);
                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, i, FALSE);
                while(timer_has_timer_expired(TIMER_WAIT_FUNCTS, TRUE) == TIMER_RUNNING);
            }
            sh1122_clear_current_screen(&plat_oled_descriptor);
        #endif
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        logic_accelerometer_routine();
        
        /* ADC value ready? */
        if (platform_io_is_voledin_conversion_result_ready() != FALSE)
        {
            current_vbat = platform_io_get_voledin_conversion_result_and_trigger_conversion();
        }            
    }
    
    /* Switch to USB power for screen */
    sh1122_oled_off(&plat_oled_descriptor);
    platform_io_disable_vbat_to_oled_stepup();
    platform_io_assert_oled_reset();
    timer_delay_ms(15);
    platform_io_power_up_oled(TRUE);
    sh1122_init_display(&plat_oled_descriptor, TRUE);
    
    /* Display info */
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_put_error_string(&plat_oled_descriptor, u"Battery rest...");
    timer_delay_ms(360000);
    
    /* Display info */
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_put_error_string(&plat_oled_descriptor, u"Topping up battery...");
    
    /* Actually start charging */
    comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_NIMH_CHG_SLW_STRT);
    comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_CHARGE_STARTED);
    logic_power_set_battery_charging_bool(TRUE, FALSE);
    
    /* Wait for end of charge */
    while (logic_power_is_battery_charging() != FALSE)
    {
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        logic_accelerometer_routine();
    }
}

/*! \fn     debug_test_battery(void)
*   \brief  Test battery capacity
*/
void debug_test_battery(void)
{
    sh1122_clear_current_screen(&plat_oled_descriptor);
    
    if (platform_io_is_usb_3v3_present_raw() != FALSE)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Device must be battery powered");
        DELAYMS(2000);
        return;
    }
    
    battery_state_te bat_state = logic_power_get_battery_state();
    if (bat_state <= BATTERY_75PCT)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Charge battery first!");
        DELAYMS(2000);
        return;
    }
    
    uint32_t animation_counter = 0;
    
    /* Count number of full animations between 75% and 25% */
    while (bat_state > BATTERY_25PCT)
    {
        /* One full animation */
        for (uint16_t i = GUI_ANIMATION_FFRAME_ID; i < GUI_ANIMATION_NBFRAMES; i++)
        {
            timer_start_timer(TIMER_ANIMATIONS, 28);
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, i, FALSE);
            while(timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_RUNNING)
            {
                /* power routines, new battery level acking in case we get a new one */
                if (logic_power_check_power_switch_and_battery(FALSE) == POWER_ACT_NEW_BAT_LEVEL)
                {
                    logic_power_get_and_ack_new_battery_level();
                }
            }
        }
        sh1122_clear_current_screen(&plat_oled_descriptor);
        
        /* Updated battery status */
        bat_state = logic_power_get_battery_state();
        
        /* Animation counter */
        if (bat_state <= BATTERY_75PCT)
        {
            animation_counter++;
        }
    }
    
    /* Finished! display result */
    inputs_clear_detections();
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_printf_xy(&plat_oled_descriptor, 40, 5, OLED_ALIGN_LEFT, FALSE, "Result: %d", animation_counter);
    sh1122_printf_xy(&plat_oled_descriptor, 40, 30, OLED_ALIGN_LEFT, FALSE, "Click to exit");
    while (inputs_get_wheel_action(FALSE, FALSE) != WHEEL_ACTION_SHORT_CLICK);
}

/*! \fn     debug_test_pattern_display(void)
*   \brief  Display test pattern
*/
void debug_test_pattern_display(void)
{
    wheel_action_ret_te action_ret = WHEEL_ACTION_NONE;
    uint8_t master_current = custom_fs_settings_get_device_setting(SETTINGS_MASTER_CURRENT);
    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, TEST_PATTERN__BITMAP_ID, FALSE);
    
    while (action_ret != WHEEL_ACTION_SHORT_CLICK)
    {
        action_ret = inputs_get_wheel_action(FALSE, FALSE);
        comms_aux_mcu_routine(MSG_NO_RESTRICT);
        if (action_ret == WHEEL_ACTION_UP)
        {
            master_current++;
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, TEST_PATTERN__BITMAP_ID, FALSE);
            sh1122_printf_xy(&plat_oled_descriptor, 40, 5, OLED_ALIGN_LEFT, FALSE, "0x%02x", master_current);
            sh1122_set_contrast_current(&plat_oled_descriptor, master_current);
        }
        else if (action_ret == WHEEL_ACTION_DOWN)
        {
            master_current--;
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, TEST_PATTERN__BITMAP_ID, FALSE);
            sh1122_printf_xy(&plat_oled_descriptor, 40, 5, OLED_ALIGN_LEFT, FALSE, "0x%02x", master_current);
            sh1122_set_contrast_current(&plat_oled_descriptor, master_current);
        }
    }
}

/*! \fn     debug_reset_device(void)
*   \brief  Reset device: erase all flash memories
*/
void debug_reset_device(void)
{
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Please wait...", FALSE);
    dataflash_bulk_erase_with_wait(&dataflash_descriptor);
    dbflash_format_flash(&dbflash_descriptor);
    for (uint16_t i = 0; i < 4096/NVMCTRL_ROW_SIZE; i++)
    {
        custom_fs_erase_256B_at_internal_custom_storage_slot(i);
    }
    custom_fs_hard_reset_settings();
    nodemgmt_format_user_profile(100, 0xFFFF & (~USER_SEC_FLG_BLE_ENABLED), 0, 0, 0);
    cpu_irq_disable();
    NVIC_SystemReset();
}

/*! \fn     debug_setup_dev_card(void)
*   \brief  Setup a smartcard to ease developer's work
*/
void debug_setup_dev_card(void)
{
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Please insert a blank card", FALSE);
    
    while(1)
    {
        /* Return ? */
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
        
        if (smartcard_lowlevel_is_card_plugged() == RETURN_JDETECT)
        {
            /* Get detection result */
            mooltipass_card_detect_return_te detection_result = smartcard_highlevel_card_detected_routine();
            
            /* Card debug info */
            if (detection_result == RETURN_MOOLTIPASS_BLANK)
            {
                volatile uint16_t pin_code = SMARTCARD_DEFAULT_PIN;
                uint8_t temp_buffer[AES_KEY_LENGTH/8];
                
                /* all arrays set to 0, pin code set to default smartcard pin code */
                memset(temp_buffer, 0, sizeof(temp_buffer));                
                
                /* Special user for special card has ID 100 */
                nodemgmt_format_user_profile(100, 0xFFFF & (~USER_SEC_FLG_BLE_ENABLED), 0, 0, 0);
                
                /* Setup smartcard */
                smartcard_highlevel_write_protected_zone(temp_buffer);
                smartcard_highlevel_write_aes_key(temp_buffer);
                smartcard_highlevel_write_security_code(&pin_code);

                /* Special card has 0000 CPZ, set 0000 as nonce */
                cpz_lut_entry_t special_user_profile;
                memset(&special_user_profile, 0, sizeof(special_user_profile));
                special_user_profile.user_id = 100;
                custom_fs_store_cpz_entry(&special_user_profile, special_user_profile.user_id);             
            }
            else
            {
                return;
            }
            
            /* Remove power to the card */
            platform_io_smc_remove_function();
            
            /* Inform user */
            sh1122_put_string_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_CENTER, u"Done! Please reboot device!", FALSE);
            timer_delay_ms(2000);
            return;
        }
    }    
}

/*! \fn     debug_smartcard_test(void)
*   \brief  Smartcard Read/Write test
*/
void debug_smartcard_test(void)
{
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Please insert a blank card", FALSE);
    
    while(1)
    {
        /* Return ? */
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
        
        if (smartcard_lowlevel_is_card_plugged() == RETURN_JDETECT)
        {            
            /* Get detection result */
            mooltipass_card_detect_return_te detection_result = smartcard_highlevel_card_detected_routine();
            
            /* Card debug info */
            if (detection_result == RETURN_MOOLTIPASS_BLANK)
            {
                uint8_t random_data[256/8];
                uint16_t success_count = 0;
                
                /* Perform a 100 read / write tests */
                for (uint16_t i = 0; i < 100; i++)
                {
                    /* Fill buffer with accelerometer data */
                    while(lis2hh12_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor, TRUE) == FALSE);
                    memcpy((void*)random_data, plat_acc_descriptor.fifo_read.acc_data_array, sizeof(random_data));
                    
                    /* Stats show */
                    sh1122_clear_current_screen(&plat_oled_descriptor);
                    sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Testing...", FALSE);
                    sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, FALSE, "%d/%d OK", success_count, i);
                    
                    /* Reset card */
                    smartcard_lowlevel_erase_application_zone1_nzone2(FALSE);
                    smartcard_lowlevel_erase_application_zone1_nzone2(TRUE);
                    smartcard_highlevel_set_authenticated_readwrite_to_zone1and2();
                    
                    /* Fill random bytes */
                    if (smartcard_highlevel_write_aes_key(random_data) != RETURN_NOK)
                    {
                        success_count++;
                    }                   
                }
            }
            else
            {
                return;
            }
            
            /* Remove power to the card */
            platform_io_smc_remove_function();
        }
    }
}    

/*! \fn     debug_smartcard_info(void)
*   \brief  Smartcard Debug Info
*/
void debug_smartcard_info(void)
{
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Please insert card", FALSE);
    
    while(1)
    {
        /* Return ? */
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
        
        if (smartcard_lowlevel_is_card_plugged() == RETURN_JDETECT)
        {
            /* Erase screen */
            sh1122_clear_current_screen(&plat_oled_descriptor);
            
            /* Get detection result */
            mooltipass_card_detect_return_te detection_result = smartcard_highlevel_card_detected_routine();
            
            /* Inform what the card is */
            if (detection_result == RETURN_MOOLTIPASS_INVALID)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Invalid Card", FALSE);
            }
            else if (detection_result == RETURN_MOOLTIPASS_PB)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Problem with card", FALSE);
            }
            else if (detection_result == RETURN_MOOLTIPASS_BLANK)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Blank card", FALSE);
            }
            else if (detection_result == RETURN_MOOLTIPASS_USER)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"User card", FALSE);
            }    
            else if (detection_result == RETURN_MOOLTIPASS_BLOCKED)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Blocked card", FALSE);
            }    
            
            /* Card debug info */
            if (detection_result != RETURN_MOOLTIPASS_INVALID)
            {                
                uint8_t temp_string[40];
                uint8_t data_buffer[20];
                
                /* Security mode */
                sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, FALSE, "Security mode: %c", (smartcard_highlevel_check_security_mode2() == RETURN_OK) ? '2' : '1');
                
                /* Fabrication zone / Memory test zone / Manufacturer zone */
                smartcard_highlevel_read_fab_zone(data_buffer);
                uint16_t fz = (uint16_t)data_buffer[0] | (uint16_t)data_buffer[1]<<8;
                smartcard_highlevel_read_mem_test_zone(data_buffer);
                uint16_t mtz = (uint16_t)data_buffer[0] | (uint16_t)data_buffer[1]<<8;
                smartcard_highlevel_read_manufacturer_zone(data_buffer);
                uint16_t mz = (uint16_t)data_buffer[1] | (uint16_t)data_buffer[0]<<8;
                sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, FALSE, "FZ: %04X / MTZ: %04X / MZ: %04X", fz, mtz, mz);
                
                /* Issuer zone */
                strcpy((char*)temp_string, "IZ: ");
                smartcard_highlevel_read_issuer_zone(data_buffer);
                debug_array_to_hex_u8string(data_buffer, temp_string + 4, SMARTCARD_ISSUER_ZONE_LGTH);
                sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, FALSE, (const char*)temp_string);
                
                /* Code protected zone */
                strcpy((char*)temp_string, "CPZ: ");
                smartcard_highlevel_read_code_protected_zone(data_buffer);
                debug_array_to_hex_u8string(data_buffer, temp_string + 5, SMARTCARD_CPZ_LENGTH);
                sh1122_printf_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_LEFT, FALSE, (const char*)temp_string);
                
                /* First bytes of AZ1 */
                strcpy((char*)temp_string, "AZ1: ");
                smartcard_lowlevel_read_smc((SMARTCARD_AZ1_BIT_START + 16*8)/8, (SMARTCARD_AZ1_BIT_START)/8, data_buffer);
                debug_array_to_hex_u8string(data_buffer, temp_string + 5, 16);
                sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_LEFT, FALSE, (const char*)temp_string);
            }            
            
            /* Remove power to the card */
            platform_io_smc_remove_function();         
        }              
    }
}

/*! \fn     debug_language_test(void)
*   \brief  Language test
*/
void debug_language_test(void)
{
    /* Language feature test */
    while(1)
    {
        cust_char_t* temp_string;
        for (uint16_t i = 0; i < custom_fs_get_number_of_languages(); i++)
        {
            custom_fs_set_current_language(i);
            custom_fs_get_string_from_file(0, &temp_string, FALSE);
            sh1122_set_emergency_font(&plat_oled_descriptor);
            sh1122_clear_current_screen(&plat_oled_descriptor);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "%d/%d : ", i+1, custom_fs_flash_header.language_map_item_count);
            sh1122_put_string_xy(&plat_oled_descriptor, 40, 0, OLED_ALIGN_LEFT, custom_fs_get_current_language_text_desc(), FALSE);            
            sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, FALSE, "String file ID: %d", custom_fs_cur_language_entry.string_file_index);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, FALSE, "Start font file ID: %d", custom_fs_cur_language_entry.starting_font);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, FALSE, "Start bitmap file ID: %d", custom_fs_cur_language_entry.starting_bitmap);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_LEFT, FALSE, "Recommended keyboard file ID: %d", custom_fs_cur_language_entry.keyboard_layout_id);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_LEFT, FALSE, "Line #0:");
            sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_MEDIUM_15_ID);
            sh1122_put_string_xy(&plat_oled_descriptor, 50, 50, OLED_ALIGN_LEFT, temp_string, FALSE);
            
            /* Return ? */
            timer_start_timer(TIMER_ANIMATIONS, 2000);
            while (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) != TIMER_EXPIRED)
            {
                if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
                {
                    sh1122_set_emergency_font(&plat_oled_descriptor);
                    return;
                }                
            }
        }
    }
}

/*! \fn     debug_debug_animation(void)
*   \brief  Debug animation
*/
void debug_debug_animation(void)
{
    while (1)
    {
        for (uint16_t i = 0; i < 120; i++)
        {
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, i, FALSE);
            
            /* Return ? */
            if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
            {
                return;
            }
        }
    }
}

/*! \fn     debug_debug_screen(void)
*   \brief  Debug screen
*/
void debug_debug_screen(void)
{        
    /* Debug variables */
    uint32_t acc_int_nb_interrupts_latched = 0;
    uint32_t acc_int_nb_interrupts = 0;
    uint16_t bat_adc_result = 0;
    uint32_t stat_times[6];    
    uint32_t last_stat_s = driver_timer_get_rtc_timestamp_uint32t();
    uint16_t default_osculp_calib_val = SYSCTRL->OSCULP32K.bit.CALIB;
    
    while(1)
    {
        /* Deal with comms */
        comms_aux_mcu_routine(MSG_NO_RESTRICT);
        
        /* Clear screen */
        stat_times[0] = timer_get_systick();
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_check_for_flush_and_terminate(&plat_oled_descriptor);
        sh1122_clear_frame_buffer(&plat_oled_descriptor);
        #else
        sh1122_clear_current_screen(&plat_oled_descriptor);
        #endif
        stat_times[1] = timer_get_systick();
        
        /* Data acq */
        stat_times[2] = timer_get_systick();
        
        /* Accelerometer interrupt */
        if (lis2hh12_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor, TRUE) != FALSE)
        {
            acc_int_nb_interrupts++;
        }
        
        /* Battery measurement */
        if (platform_io_is_voledin_conversion_result_ready() != FALSE)
        {
            bat_adc_result = platform_io_get_voledin_conversion_result_and_trigger_conversion();
        }
        
        /* Check for Accelerometer SERCOM buffer overflow */   
        if (ACC_SERCOM->SPI.STATUS.bit.BUFOVF != 0)
        {
            sh1122_put_error_string(&plat_oled_descriptor, u"ACC Overflow");      
        }
        
        /* Check for aux comms SERCOM buffer overflow */
        if (AUXMCU_SERCOM->SPI.STATUS.bit.BUFOVF != 0)
        {
            sh1122_put_error_string(&plat_oled_descriptor, u"AUX COM Overflow");      
        }
        
        /* End data acq */
        stat_times[3] = timer_get_systick();
        
        /* Display stats */
        stat_times[4] = timer_get_systick();
        
        /* Stats latched at second changes */        
        if (driver_timer_get_rtc_timestamp_uint32t() != last_stat_s)
        {
            acc_int_nb_interrupts_latched = acc_int_nb_interrupts;
            last_stat_s = driver_timer_get_rtc_timestamp_uint32t();
            acc_int_nb_interrupts = 0;
        }
         
        /* Line 2: date */
        uint32_t timestamp;
        int32_t fine_adjust_val;
        int32_t counter_correct;
        int32_t cumulative_correct;
        timer_get_timestamp_debug_data(&timestamp, &counter_correct, &cumulative_correct, &fine_adjust_val);
        sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, TRUE, "TS: %u, adj %d, %d + %d", timestamp, fine_adjust_val, cumulative_correct, counter_correct);
        
        /* Line 3: accelerometer */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, TRUE, "ACC: %uHz X: %i Y: %i Z: %i", acc_int_nb_interrupts_latched*32, plat_acc_descriptor.fifo_read.acc_data_array[0].acc_x, plat_acc_descriptor.fifo_read.acc_data_array[0].acc_y, plat_acc_descriptor.fifo_read.acc_data_array[0].acc_z);
        
        /* Line 4: battery */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, TRUE, "BAT: ADC %u, %u mV, OSC: %u", bat_adc_result, (((uint32_t)bat_adc_result)*199)>>9, SYSCTRL->OSCULP32K.bit.CALIB);
        
        /* Line 5: Unit SN & MAC */
        uint8_t mac[6];
        custom_fs_get_debug_bt_addr(mac);
        sh1122_printf_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_LEFT, TRUE, "Unit SN: %u, MAC: %02x%02x%02x%02x%02x%02x", custom_fs_get_platform_serial_number(), mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
        
        /* Display stats */
        stat_times[5] = timer_get_systick();
        
        /* Line 6: display stats */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_LEFT, TRUE, "STATS MS: text %u, erase %u, stats %u", stat_times[5]-stat_times[4], stat_times[1]-stat_times[0], stat_times[3]-stat_times[2]);

        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_flush_frame_buffer(&plat_oled_descriptor);
        #endif
        
        /* Get user action */
        wheel_action_ret_te wheel_user_action = inputs_get_wheel_action(FALSE, FALSE);  
        
        /* Return ? */
        if ((wheel_user_action == WHEEL_ACTION_SHORT_CLICK) || (wheel_user_action == WHEEL_ACTION_LONG_CLICK))
        {
            return;
        }
        else if (wheel_user_action == WHEEL_ACTION_UP)
        {
            if (default_osculp_calib_val != 31)
            {
                default_osculp_calib_val++;
            }
            //SYSCTRL->OSCULP32K.bit.CALIB = default_osculp_calib_val;
        }
        else if (wheel_user_action == WHEEL_ACTION_DOWN)
        {
            if (default_osculp_calib_val != 0)
            {
                default_osculp_calib_val--;
            }
            //SYSCTRL->OSCULP32K.bit.CALIB = default_osculp_calib_val;
        }
    }
}

/*! \fn     debug_mcu_and_aux_info(void)
*   \brief  Print info about main & aux MCU
*/
void debug_mcu_and_aux_info(void)
{
    aux_mcu_message_t* temp_rx_message;
    aux_mcu_message_t* temp_tx_message_pt;
    
    /* Get part number */
    char part_number[20] = "unknown";
    if (DSU->DID.bit.DEVSEL == 0x05)
    {
        strcpy(part_number, "ATSAMD21G18A");
    }
    
    /* Print info */
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "Main MCU, fw %d.%d", FW_MAJOR, FW_MINOR);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, FALSE, "DID 0x%08x (%s), rev %c", DSU->DID.reg, part_number, 'A' + DSU->DID.bit.REVISION);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, FALSE, "UID: 0x%08x%08x%08x%08x", *(uint32_t*)0x0080A00C, *(uint32_t*)0x0080A040, *(uint32_t*)0x0080A044, *(uint32_t*)0x0080A048);
    
    /* Prepare status message request */
    temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_PLAT_DETAILS);
    
    /* Send message */
    comms_aux_mcu_send_message(temp_tx_message_pt);
    
    /* Wait for message from aux MCU */
    while(comms_aux_mcu_active_wait(&temp_rx_message, AUX_MCU_MSG_TYPE_PLAT_DETAILS, FALSE, -1) != RETURN_OK){}
        
    /* Cast aux MCU DID */
    DSU_DID_Type aux_mcu_did;
    aux_mcu_did.reg = temp_rx_message->aux_details_message.aux_did_register;
    
    /* From DID, get part number */
    if (aux_mcu_did.bit.DEVSEL == 0x0B)
    {
        strcpy(part_number, "ATSAMD21E17A");
    }
    if (aux_mcu_did.bit.DEVSEL == 0x0A)
    {
        strcpy(part_number, "ATSAMD21E18A");
    }
    else
    {
        strcpy(part_number, "unknown");
    }    
        
    /* This is debug, no need to check if it is the correct received message */
    sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, FALSE, "Aux MCU fw %d.%d", temp_rx_message->aux_details_message.aux_fw_ver_major, temp_rx_message->aux_details_message.aux_fw_ver_minor);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_LEFT, FALSE, "DID 0x%08x (%s), rev %c", aux_mcu_did.reg, part_number, 'A' + aux_mcu_did.bit.REVISION);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_LEFT, FALSE, "UID: 0x%08x%08x%08x%08x", temp_rx_message->aux_details_message.aux_uid_registers[0], temp_rx_message->aux_details_message.aux_uid_registers[1], temp_rx_message->aux_details_message.aux_uid_registers[2], temp_rx_message->aux_details_message.aux_uid_registers[3]);
    
    /* Info printed, rearm DMA RX */
    comms_aux_arm_rx_and_clear_no_comms();
    
    /* Check for click to return */
    while(1)
    {
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
    }
}

/*! \fn     debug_rf_freq_sweep(void)
*   \brief  Use ATBTLC1000 test mode to do a tx frequency sweep
*/
void debug_rf_freq_sweep(void)
{
    aux_mcu_message_t* sweep_message_to_be_sent;
    
    /* Parameters to be sent to aux mcu */
    int16_t payload_length = 36;
    int16_t frequency_index = -1;
    int16_t screen_contents = 0;
    int16_t payload_type = 0;
    int16_t nb_loops = -2;
    
    /* Logic */
    int16_t* values_pts[] = {&frequency_index, &payload_type, &payload_length, &nb_loops, &screen_contents};
    int16_t upper_bounds[] = {39, 7, 36, 100, 3};
    int16_t lower_bounds[] = {-1, 0, 0, -2, 0};
    uint16_t selected_item = 0;
    BOOL redraw_needed = TRUE;
    uint16_t run_number = 0;
    
    /* Enable BLE */
    logic_aux_mcu_enable_ble(TRUE);
    
    /* Frequency index choice */
    while(TRUE)
    {        
        if (redraw_needed != FALSE)
        {
            sh1122_clear_current_screen(&plat_oled_descriptor);
            
            /* Line 1: frequency index */
            if (frequency_index < 0)
                sh1122_printf_xy(&plat_oled_descriptor, 10, 0, OLED_ALIGN_LEFT, FALSE, "Frequency Index: Sweep");
            else
                sh1122_printf_xy(&plat_oled_descriptor, 10, 0, OLED_ALIGN_LEFT, FALSE, "Frequency Index: %d", frequency_index);
                
            /* Line 2: payload type */
            switch (payload_type)
            {
                case 0: sh1122_printf_xy(&plat_oled_descriptor, 10, 11, OLED_ALIGN_LEFT, FALSE, "Payload: PAYL_PSEUDO_RAND_9"); break;
                case 1: sh1122_printf_xy(&plat_oled_descriptor, 10, 11, OLED_ALIGN_LEFT, FALSE, "Payload: PAYL_11110000"); break;
                case 2: sh1122_printf_xy(&plat_oled_descriptor, 10, 11, OLED_ALIGN_LEFT, FALSE, "Payload: PAYL_10101010"); break;
                case 3: sh1122_printf_xy(&plat_oled_descriptor, 10, 11, OLED_ALIGN_LEFT, FALSE, "Payload: PAYL_PSEUDO_RAND_15"); break;
                case 4: sh1122_printf_xy(&plat_oled_descriptor, 10, 11, OLED_ALIGN_LEFT, FALSE, "Payload: PAYL_ALL_1"); break;
                case 5: sh1122_printf_xy(&plat_oled_descriptor, 10, 11, OLED_ALIGN_LEFT, FALSE, "Payload: PAYL_ALL_0"); break;
                case 6: sh1122_printf_xy(&plat_oled_descriptor, 10, 11, OLED_ALIGN_LEFT, FALSE, "Payload: PAYL_00001111"); break;
                case 7: sh1122_printf_xy(&plat_oled_descriptor, 10, 11, OLED_ALIGN_LEFT, FALSE, "Payload: PAYL_01010101"); break;
                default: break;
            }
            
            /* Line 3: payload length */
            sh1122_printf_xy(&plat_oled_descriptor, 10, 22, OLED_ALIGN_LEFT, FALSE, "Payload length: %d", payload_length);
            
            /* Line 4: number of loops */
            if (nb_loops < -1)
                sh1122_printf_xy(&plat_oled_descriptor, 10, 33, OLED_ALIGN_LEFT, FALSE, "NB loops: continuous");
            else if (nb_loops < 0)
                sh1122_printf_xy(&plat_oled_descriptor, 10, 33, OLED_ALIGN_LEFT, FALSE, "NB loops: continuous (on AUX)");
            else
                sh1122_printf_xy(&plat_oled_descriptor, 10, 33, OLED_ALIGN_LEFT, FALSE, "NB loops: %d", nb_loops);
            
            /* Line 5: screen contents */
            switch (screen_contents)
            {
                case 0: sh1122_printf_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, FALSE, "Screen contents: static"); break;
                case 1: sh1122_printf_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, FALSE, "Screen contents: empty"); break;
                case 2: sh1122_printf_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, FALSE, "Screen contents: off"); break;
                case 3: sh1122_printf_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, FALSE, "Screen contents: dynamic"); break;
                default: break;
            }            
            
            /* Selected line */
            sh1122_printf_xy(&plat_oled_descriptor, 0, selected_item*11, OLED_ALIGN_LEFT, FALSE, "-");
            
            redraw_needed = FALSE;
        }
        
        wheel_action_ret_te action_ret = inputs_get_wheel_action(TRUE, FALSE);
        if (action_ret == WHEEL_ACTION_UP)
        {
            if ((*values_pts[selected_item])++ == upper_bounds[selected_item])
                *values_pts[selected_item] = lower_bounds[selected_item];
            redraw_needed = TRUE;
        }
        else if (action_ret == WHEEL_ACTION_DOWN)
        {
            if ((*values_pts[selected_item])-- == lower_bounds[selected_item])
                *values_pts[selected_item] = upper_bounds[selected_item];
            redraw_needed = TRUE;
        }
        else if (action_ret == WHEEL_ACTION_SHORT_CLICK)
        {
            if (selected_item++ == 4)
                break;
            redraw_needed = TRUE;
        }
        else if (action_ret == WHEEL_ACTION_LONG_CLICK)
        {
            if (selected_item-- == 0)
                return;
            redraw_needed = TRUE;
        }
    }
    
    /* Current frequency index beeing sent */
    uint16_t cur_frequency_index = (uint16_t)frequency_index;
    if (frequency_index < 0)
    {
        cur_frequency_index = 0;
    }
    
    /* Depending on display options */
    if (screen_contents == 0)
    {
        sh1122_clear_current_screen(&plat_oled_descriptor);
        sh1122_printf_xy(&plat_oled_descriptor, 0, 25, OLED_ALIGN_CENTER, FALSE, "Currently sweeping...");
    } 
    else if (screen_contents == 1)
    {
        sh1122_clear_current_screen(&plat_oled_descriptor);
        sh1122_oled_off(&plat_oled_descriptor);
    }
    else if (screen_contents == 2)
    {
        sh1122_clear_current_screen(&plat_oled_descriptor);
        sh1122_oled_off(&plat_oled_descriptor);
        platform_io_power_down_oled();
    }
    
    /* Check for continuous sweep on aux MCU */
    if (nb_loops == -1)
    {
        sweep_message_to_be_sent = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_MAIN_MCU_CMD);
        sweep_message_to_be_sent->payload_length1 = MEMBER_SIZE(main_mcu_command_message_t, command) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t);
        sweep_message_to_be_sent->main_mcu_command_message.command = MAIN_MCU_COMMAND_TX_TONE_CONT;
        sweep_message_to_be_sent->main_mcu_command_message.payload_as_uint16[0] = cur_frequency_index;  // Frequency index, up to 39
        sweep_message_to_be_sent->main_mcu_command_message.payload_as_uint16[1] = payload_type;         // Payload type, up to 7
        sweep_message_to_be_sent->main_mcu_command_message.payload_as_uint16[2] = payload_length;       // Payload length, up to 36
        comms_aux_mcu_send_message(sweep_message_to_be_sent);
    }
 
    while (TRUE)
    {
        /* Dynamic screen contents */
        if (screen_contents == 3)
        {
            sh1122_clear_current_screen(&plat_oled_descriptor);
        }
        
        if (nb_loops != -1)
        {
            /* Start single sweep */
            sweep_message_to_be_sent = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_MAIN_MCU_CMD);
            sweep_message_to_be_sent->payload_length1 = MEMBER_SIZE(main_mcu_command_message_t, command) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t);
            sweep_message_to_be_sent->main_mcu_command_message.command = MAIN_MCU_COMMAND_TX_SWEEP_SGL;
            sweep_message_to_be_sent->main_mcu_command_message.payload_as_uint16[0] = cur_frequency_index;  // Frequency index, up to 39
            sweep_message_to_be_sent->main_mcu_command_message.payload_as_uint16[1] = payload_type;         // Payload type, up to 7
            sweep_message_to_be_sent->main_mcu_command_message.payload_as_uint16[2] = payload_length;       // Payload length, up to 36
            comms_aux_mcu_send_message(sweep_message_to_be_sent);
        }
        
        /* Dynamic screen contents */
        if (screen_contents == 3)
        {
            sh1122_printf_xy(&plat_oled_descriptor, 0, 25, OLED_ALIGN_CENTER, FALSE, "Run #%d, freq #%d, payload #%d", run_number, cur_frequency_index, payload_type);
        }        
        
        if (nb_loops != -1)
        {
            /* Wait for end of sweep */
            comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_TX_SWEEP_DONE);
        }        
        
        /* Sweep if required */
        if (frequency_index < 0)
        {
            if (cur_frequency_index++ == 39)
            {
                cur_frequency_index = 0;
            }
        }
        
        /* Click for return */
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            if (screen_contents == 1)
            {
                sh1122_oled_on(&plat_oled_descriptor);
            }
            else if (screen_contents == 2)
            {
                platform_io_power_up_oled(platform_io_is_usb_3v3_present_raw());
                sh1122_oled_on(&plat_oled_descriptor);
            }
            break;
        }
        
        /* Increment run number */
        run_number++;
        
        /* Exit condition */
        if ((nb_loops > 0) && (run_number >= nb_loops))
        {
            break;
        }
    }   
    
    /* Stop continuous sweeping on aux side if selected */
    if (nb_loops == -1)
    {
        /* Send message to stop */
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_TX_TONE_CONT_STOP);
        
        /* Wait for callback */
        comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_TX_SWEEP_DONE);
    }
}

/*! \fn     debug_atbtlc_info(void)
*   \brief  Print info about aux MCU ATBTLC1000
*/
void debug_atbtlc_info(void)
{    
    aux_mcu_message_t* temp_rx_message;   
    aux_mcu_message_t* temp_tx_message_pt;
    
    /* Enable BLE */
    logic_aux_mcu_enable_ble(TRUE);
    
    /* Generate our packet */
    temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_PLAT_DETAILS);
    
    /* Send message */
    comms_aux_mcu_send_message(temp_tx_message_pt);
    
    /* Wait for message from aux MCU */
    while(comms_aux_mcu_active_wait(&temp_rx_message, AUX_MCU_MSG_TYPE_PLAT_DETAILS, FALSE, -1) != RETURN_OK){}
        
    /* Output debug info */
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 00, OLED_ALIGN_LEFT, FALSE, "BluSDK Lib: %X.%X", temp_rx_message->aux_details_message.blusdk_lib_maj, temp_rx_message->aux_details_message.blusdk_lib_min);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, FALSE, "ATBTLC Fw: %X.%X, Build %X", temp_rx_message->aux_details_message.blusdk_fw_maj, temp_rx_message->aux_details_message.blusdk_fw_min, temp_rx_message->aux_details_message.blusdk_fw_build);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, FALSE, "ATBTLC RF Ver: 0x%8X", (unsigned int)temp_rx_message->aux_details_message.atbtlc_rf_ver);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, FALSE, "ATBTLC Chip ID: 0x%6X", (unsigned int)temp_rx_message->aux_details_message.atbtlc_chip_id);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_LEFT, FALSE, "ATBTLC Addr: 0x%02X%02X%02X%02X%02X%02X", temp_rx_message->aux_details_message.atbtlc_address[5], temp_rx_message->aux_details_message.atbtlc_address[4], temp_rx_message->aux_details_message.atbtlc_address[3], temp_rx_message->aux_details_message.atbtlc_address[2], temp_rx_message->aux_details_message.atbtlc_address[1], temp_rx_message->aux_details_message.atbtlc_address[0]);

    /* Info printed, rearm DMA RX */
    comms_aux_arm_rx_and_clear_no_comms();
    
    /* Check for click to return */
    while(1)
    {
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
    }
}

/*! \fn     debug_glyphs_show(void)
*   \brief  Show all glyphs for a given font
*/
void debug_glyphs_show(void)
{
    wheel_action_ret_te action_ret = WHEEL_ACTION_NONE;
    uint16_t current_char = ' ';
    uint8_t current_font = 0;

    /* Allow going to next line */
    plat_oled_descriptor.line_feed_allowed = TRUE;
    
    while (TRUE)
    {
        /* Clear screen */
        sh1122_clear_current_screen(&plat_oled_descriptor);

        /* Set Emergency Font */
        sh1122_set_emergency_font(&plat_oled_descriptor);

        /* Show current font ID */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "Font #%d: ", current_font);

        /* Set current font */
        sh1122_refresh_used_font(&plat_oled_descriptor, current_font);
        
        /* Dirty hack: set '?' support to FALSE so non supported chars aren't replaced with it */
        plat_oled_descriptor.question_mark_support_described = FALSE;

        /* Set XY for following chars */
        plat_oled_descriptor.cur_text_x = 50;
        plat_oled_descriptor.cur_text_y = 0;

        /* Display all chars */
        while(sh1122_put_char(&plat_oled_descriptor, current_char++, FALSE) >= 0);
        
        /* Get action */
        action_ret = inputs_get_wheel_action(TRUE, FALSE);
        
        /* Exit ? */
        if (action_ret == WHEEL_ACTION_LONG_CLICK)
        {
            plat_oled_descriptor.question_mark_support_described = TRUE;
            sh1122_set_emergency_font(&plat_oled_descriptor);
            plat_oled_descriptor.line_feed_allowed = FALSE;
            return;
        }
        else if (action_ret == WHEEL_ACTION_UP)
        {
            while(sh1122_refresh_used_font(&plat_oled_descriptor, --current_font) != RETURN_OK);
            current_char = ' ';
        }
        else if (action_ret == WHEEL_ACTION_DOWN)
        {
            while(sh1122_refresh_used_font(&plat_oled_descriptor, ++current_font) != RETURN_OK);
            current_char = ' ';
        }
    }
}

/*! \fn     debug_glyph_scroll(void)
*   \brief  Scroll through the glyphs
*/
void debug_glyph_scroll(void)
{
    wheel_action_ret_te action_ret = WHEEL_ACTION_UP;
    uint8_t current_font = 0;
    uint16_t temp_uint16 = 0;
    uint16_t cur_glyph = ' ';
    
    /* Dirty hack: set '?' support to FALSE so non supported chars aren't replaced with it */
    plat_oled_descriptor.question_mark_support_described = FALSE;
    
    while (TRUE)
    {        
        /* Find a glyph to print */
        do
        {
            if (action_ret == WHEEL_ACTION_DOWN)
            {
                cur_glyph--;
            }
            else
            {
                cur_glyph++;
            }
        }
        while(sh1122_get_glyph_width(&plat_oled_descriptor, (cust_char_t)cur_glyph, &temp_uint16) == 0);
        
        /* Clear screen */
        sh1122_clear_current_screen(&plat_oled_descriptor);
        
        /* Set Emergency Font */
        sh1122_set_emergency_font(&plat_oled_descriptor);
        
        /* Print glyph */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "Font %d, glyph %d: ", current_font, cur_glyph);

        /* Set current font */
        sh1122_refresh_used_font(&plat_oled_descriptor, current_font);
                
        /* Print glyph */
        sh1122_put_char(&plat_oled_descriptor, cur_glyph, FALSE);
        
        /* Get action */
        action_ret = inputs_get_wheel_action(TRUE, FALSE);
        
        /* Exit ? */
        if (action_ret == WHEEL_ACTION_LONG_CLICK)
        {
            plat_oled_descriptor.question_mark_support_described = TRUE;
            sh1122_set_emergency_font(&plat_oled_descriptor);
            return;
        }
        else if (action_ret == WHEEL_ACTION_SHORT_CLICK)
        {
            while(sh1122_refresh_used_font(&plat_oled_descriptor, ++current_font) != RETURN_OK);
            cur_glyph = ' ';
        }
    }
}

/*! \fn     debug_nimh_status(void)
*   \brief  NiMH status debug
*/
void debug_nimh_status(void)
{
    power_consumption_log_t* consumption_log_pt = logic_power_get_power_consumption_log_pt();
    aux_mcu_message_t* temp_tx_message_pt;
    aux_mcu_message_t* temp_rx_message_pt;
        
    while (TRUE)
    {        
        /* Keep calling the routines */
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        logic_power_routine();
        
        /* Clear screen */
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_check_for_flush_and_terminate(&plat_oled_descriptor);
        sh1122_clear_frame_buffer(&plat_oled_descriptor);
        #else
        sh1122_clear_current_screen(&plat_oled_descriptor);
        #endif
        
        /* Line 1 & 2: Discharge / Charge type with information */
        if (logic_power_is_battery_charging() == FALSE)
        {
            if (logic_power_get_power_source() == BATTERY_POWERED)
            {
                /* Line 1: battery level */
                sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, TRUE, "Discharging, %dpcts, ADC: %u", logic_power_get_battery_level()*10, logic_power_debug_get_last_adc_measurement());
            } 
            else
            {
                /* Line 1: battery level */
                sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, TRUE, "Not charging, %dpcts, ADC: %u", logic_power_get_battery_level()*10, logic_power_debug_get_last_adc_measurement());
            }
        }
        else
        {
            /* Line 1: charge type */
            nimh_charge_te charge_type = logic_power_get_current_charge_type();
            if (charge_type == NORMAL_CHARGE)
            {
                sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, TRUE, "Standard Charge");
            }
            else if (charge_type == SLOW_START_CHARGE)
            {
                sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, TRUE, "Slow Start Charge");
            }
            else if (charge_type == RECOVERY_CHARGE)
            {
                sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, TRUE, "Recovery Charge");
            }
        }
            
        /* Get charge information from AUX MCU */
        temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_NIMH_CHARGE);
        comms_aux_mcu_send_message(temp_tx_message_pt);
        while(comms_aux_mcu_active_wait(&temp_rx_message_pt, AUX_MCU_MSG_TYPE_NIMH_CHARGE, FALSE, -1) != RETURN_OK){}
            
        /* Line 2: charge information from AUX */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, TRUE, "AUX state %d, ADC: %u, cur: %u", temp_rx_message_pt->nimh_charge_message.charge_status, temp_rx_message_pt->nimh_charge_message.battery_voltage, temp_rx_message_pt->nimh_charge_message.charge_current);
            
        /* Rearm aux communications */
        comms_aux_arm_rx_and_clear_no_comms();
        
        /* Line 3: nb 30mins counters */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, TRUE, "30mns %don %dcard %dadv %dcon", consumption_log_pt->nb_30mins_powered_on, consumption_log_pt->nb_30mins_card_inserted, consumption_log_pt->nb_30mins_ble_advertising, consumption_log_pt->nb_30mins_windows_connect);
        
        /* Line 4: ms counters */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, TRUE, "s %unos %dmain %dfull", consumption_log_pt->nb_ms_no_screen_aux_main_awake >> 10, consumption_log_pt->nb_ms_no_screen_main_awake >> 10, consumption_log_pt->nb_ms_full_pawa >> 10);
        
        /* Line 5: others */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_LEFT, TRUE, "s %usince full %upct aux", consumption_log_pt->nb_ms_spent_since_last_full_charge, consumption_log_pt->aux_mcu_reported_pct);
        
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_flush_frame_buffer(&plat_oled_descriptor);
        #endif
        
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
    }
}

/*! \fn     debug_nimh_charging(void)
*   \brief  NiMH charge with debug info
*/
void debug_nimh_charging(void)
{
    int16_t ystarts[256];
    uint16_t sec_counter = 59;
    uint16_t current_index = 0;
    aux_mcu_message_t* temp_rx_message;
    aux_mcu_message_t* temp_tx_message_pt;
    
    /* Clear screen */
    sh1122_clear_current_screen(&plat_oled_descriptor);
    memset((void*)ystarts, 63, sizeof(ystarts));
    
    /* Send battery charge message */
    if (logic_power_get_power_source() == USB_POWERED)
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_NIMH_CHARGE);
        comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_CHARGE_STARTED);
        logic_power_set_battery_charging_bool(TRUE, FALSE);
    }
    
    /* Arm charge status request timer */
    uint16_t temp_timer_id = timer_get_and_start_timer(1000);
    
    /* Local vars */
    uint16_t bat_mv = 0;
    
    while(TRUE)
    {   
        /* Battery measurement */
        if (platform_io_is_voledin_conversion_result_ready() != FALSE)
        {
            bat_mv = platform_io_get_voledinmv_conversion_result_and_trigger_conversion();
        }
        
        /* Send a packet to aux MCU? */
        if (timer_has_allocated_timer_expired(temp_timer_id, TRUE) == TIMER_EXPIRED)
        {            
            /* Generate our packet */
            temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_NIMH_CHARGE);
            
            /* Send message */
            comms_aux_mcu_send_message(temp_tx_message_pt);
            
            /* Wait for message from aux MCU */
            while(comms_aux_mcu_active_wait(&temp_rx_message, AUX_MCU_MSG_TYPE_NIMH_CHARGE, FALSE, -1) != RETURN_OK){}
            
            /* Clear screen */
            sh1122_clear_current_screen(&plat_oled_descriptor);
            
            /* Debug info */
            if (logic_power_get_power_source() == BATTERY_POWERED)
            {
                sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "MCU Vbat: %umV", bat_mv);
            } 
            else
            {
                if (temp_rx_message->nimh_charge_message.charge_status == LB_CHARGING_DONE)
                {
                    sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "Charging done");
                } 
                else
                {
                    /* AUX MCU is using Vcc/1.48 reference and we are 3V3 powered */
                    uint32_t scaled_down_value = ((uint32_t)temp_rx_message->nimh_charge_message.battery_voltage*8919)>>14;
                    sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "%u - VBat: %umV - Cur %i", temp_rx_message->nimh_charge_message.charge_status, scaled_down_value, temp_rx_message->nimh_charge_message.charge_current);
                }
            }
                
            /* Info printed, rearm DMA RX */
            comms_aux_arm_rx_and_clear_no_comms();
            
            /* Time to update our graph? */
            if (sec_counter++ == 59)
            {
                sec_counter = 0;
                
                if (current_index == 255)
                {
                    /* Shift bars */
                    for (uint16_t i = 0; i < 255; i++)
                    {
                        ystarts[i] = ystarts[i+1];
                    }
                } 
                
                /* Store data */
                if (logic_power_get_power_source() == BATTERY_POWERED)
                {
                    ystarts[current_index] = 63-((bat_mv-1100)>>3);
                }
                else
                {
                    ystarts[current_index] = 63-((((temp_rx_message->nimh_charge_message.battery_voltage*103)>>8)-1100)>>3);
                }                
                
                /* Increment index */
                if (current_index != 255)
                {
                    current_index++;
                }                    
            }
            
            /* Draw graph */
            for (uint16_t x = 0; x < 256; x++)
            {
                sh1122_draw_vertical_line(&plat_oled_descriptor, x, ystarts[x], 63, 0x04, FALSE);
            }
            
            if (temp_rx_message->nimh_charge_message.charge_status == LB_CHARGING_DONE)
            {
                while (TRUE)
                {
                    if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
                    {
                        /* Free timer */
                        timer_deallocate_timer(temp_timer_id);
                        return;
                    }
                }
            }
            
            /* Arm charge status request timer */
            timer_rearm_allocated_timer(temp_timer_id, 1000);       
        }
        
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            /* Free timer */
            timer_deallocate_timer(temp_timer_id);
            return;
        }
    }
}

/*! \fn     debug_stack_info(void)
*   \brief  Print info about stack usage
*/
void debug_stack_info(void)
{
    aux_mcu_message_t* temp_rx_message;
    aux_mcu_message_t* temp_tx_message_pt;
    uint32_t main_mcu_stack_low_watermark = main_check_stack_usage();
    uint32_t aux_mcu_stack_low_watermark = 0;

    /* Print info */
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "Stack Usage Low Watermarks");

    /* Prepare status message request */
    temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_PLAT_DETAILS);

    /* Send message */
    comms_aux_mcu_send_message(temp_tx_message_pt);

    /* Wait for message from aux MCU */
    while(comms_aux_mcu_active_wait(&temp_rx_message, AUX_MCU_MSG_TYPE_PLAT_DETAILS, FALSE, -1) != RETURN_OK){}

    aux_mcu_stack_low_watermark = temp_rx_message->aux_details_message.aux_stack_low_watermark;

    sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, FALSE, "Main MCU: %u bytes", main_mcu_stack_low_watermark);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, FALSE, "Aux MCU: %u bytes", aux_mcu_stack_low_watermark);

    /* Info printed, rearm DMA RX */
    comms_aux_arm_rx_and_clear_no_comms();

    /* Check for click to return */
    while(1)
    {
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
    }
}
