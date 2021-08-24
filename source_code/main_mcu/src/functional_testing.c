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
/*!  \file     functional_testing.c
*    \brief    File dedicated to the functional testing function
*    Created:  18/05/2019
*    Author:   Mathieu Stephan
*/
#include "smartcard_highlevel.h"
#include "logic_accelerometer.h"
#include "functional_testing.h"
#include "smartcard_lowlevel.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "logic_power.h"
#include "platform_io.h"
#include "custom_fs.h"
#include "sh1122.h"
#include "inputs.h"
#include "main.h"


/*! \fn     functional_rf_testing_start(void)
*   \brief  RF functional testing start
*/
void functional_rf_testing_start(void)
{
    aux_mcu_message_t* sweep_message_to_be_sent;

    /* Debug */
    sh1122_put_error_string(&plat_oled_descriptor, u"Starting RF testing...");

    /* Enable BLE */
    logic_aux_mcu_enable_ble(TRUE);
    
    /* Sweep message send */
    sweep_message_to_be_sent = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_MAIN_MCU_CMD);
    sweep_message_to_be_sent->payload_length1 = MEMBER_SIZE(main_mcu_command_message_t, command) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t);
    sweep_message_to_be_sent->main_mcu_command_message.command = MAIN_MCU_COMMAND_TX_TONE_CONT;
    sweep_message_to_be_sent->main_mcu_command_message.payload_as_uint16[0] = 0;        // Frequency index, up to 39
    sweep_message_to_be_sent->main_mcu_command_message.payload_as_uint16[1] = 7;        // Payload type, up to 7
    sweep_message_to_be_sent->main_mcu_command_message.payload_as_uint16[2] = 36;       // Payload length, up to 36
    comms_aux_mcu_send_message(sweep_message_to_be_sent);

    /* Debug */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_put_error_string(&plat_oled_descriptor, u"Check for power ONLY between bands");
}

/*! \fn     functional_testing_start(BOOL clear_first_boot_flag)
*   \brief  Functional testing function
*   \param  clear_first_boot_flag   Set to TRUE to clear first boot flag on successful test
*/
void functional_testing_start(BOOL clear_first_boot_flag)
{
    aux_mcu_message_t* temp_rx_message;
    
    /* Test no comms signal */
    platform_io_set_no_comms();
    if (comms_aux_mcu_send_receive_ping() == RETURN_OK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Nocomms error!"); while(1);
    }
    comms_aux_mcu_clear_rx_already_armed_error();
    platform_io_clear_no_comms();
    
    /* Receive pending ping */
    sh1122_put_error_string(&plat_oled_descriptor, u"Pinging Aux MCU...");
    comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_IM_HERE);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* First boot should be done using battery */
    if (platform_io_is_usb_3v3_present_raw() != FALSE)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Power using battery first!");
        while (platform_io_is_usb_3v3_present_raw() != FALSE);
        timer_delay_ms(200); platform_io_disable_switch_and_die(); while(1);
    }
    
    /* Check for removed card */
    if (smartcard_low_level_is_smc_absent() != RETURN_OK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Please remove the card first!");
        while (smartcard_low_level_is_smc_absent() != RETURN_OK);
        timer_delay_ms(200); platform_io_disable_switch_and_die(); while(1);
    }
    
    /* Internal 1V1 measurement & VOLED VIN when battery powered and screen not powered */
    platform_io_power_down_oled();
    platform_io_set_voled_vin_as_pulldown();
    timer_delay_ms(400); sh1122_oled_off(&plat_oled_descriptor); timer_delay_ms(50);
    uint16_t bandgap_measurement_bat_powered = platform_io_get_single_bandgap_measurement();
    
    /* Use Voled_vin back as adc input, get the voltage when all power sources are off (falls to 800mV instantly than slowly drifts down... check for 800mV) */
    while(platform_io_is_voledin_conversion_result_ready() == FALSE);
    uint32_t battery_voltage = platform_io_get_voledin_conversion_result_and_trigger_conversion();
    while(platform_io_is_voledin_conversion_result_ready() == FALSE);
    battery_voltage = platform_io_get_voledin_conversion_result_and_trigger_conversion();
    
    /* Switch the screen back on */
    logic_power_set_power_source(BATTERY_POWERED);
    platform_io_power_up_oled(FALSE);
    sh1122_oled_on(&plat_oled_descriptor);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* Check for above 800mV */
    if (battery_voltage > BATTERY_ADC_800MV_VALUE)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Q2/Q8/U1 issue");
        timer_delay_ms(5000); platform_io_disable_switch_and_die(); while(1);
    }
    
    /* Wheel testing */
    timer_delay_ms(500);
    inputs_clear_detections();
    uint16_t temp_timer_id = timer_get_and_start_timer(20000);
    sh1122_put_error_string(&plat_oled_descriptor, u"scroll up");
    while (inputs_get_wheel_action(FALSE, FALSE) != WHEEL_ACTION_UP)
    {
        /* Timer timeout, switch off platform */
        if (timer_has_allocated_timer_expired(temp_timer_id, FALSE) == TIMER_EXPIRED)
        {
            sh1122_oled_off(&plat_oled_descriptor);     // Display off command
            platform_io_power_down_oled();              // Switch off stepup
            platform_io_set_wheel_click_pull_down();    // Pull down on wheel click to slowly discharge capacitor
            timer_delay_ms(100);                        // From OLED datasheet wait before removing 3V3
            platform_io_set_wheel_click_low();          // Completely discharge cap
            timer_delay_ms(10);                         // Wait a tad
            platform_io_disable_switch_and_die();       // Die!
        }            
    }
    timer_deallocate_timer(temp_timer_id);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    timer_delay_ms(500);
    inputs_clear_detections();
    sh1122_put_error_string(&plat_oled_descriptor, u"scroll down");
    while (inputs_get_wheel_action(FALSE, FALSE) != WHEEL_ACTION_DOWN);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* Annoying click test due to bad experiences: 9 quick clicks */
    timer_delay_ms(500);
    inputs_clear_detections();
    uint16_t click_counter = 0;
    inputs_set_wheel_debounce_value(100);
    temp_timer_id = timer_get_and_start_timer(10000);
    cust_char_t quick_click_string[] = u"Quick click 9 times QUICKLY";
    sh1122_put_error_string(&plat_oled_descriptor, (const cust_char_t*)quick_click_string);
    while ((click_counter < 9) && (timer_has_allocated_timer_expired(temp_timer_id, FALSE) != TIMER_EXPIRED))
    {
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            sh1122_clear_current_screen(&plat_oled_descriptor);
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            #endif
            click_counter++;
            quick_click_string[12] = '0' + 9 - click_counter;
            if (click_counter != 9)
            {
                sh1122_put_error_string(&plat_oled_descriptor, quick_click_string);
            }
        }            
    }
    timer_deallocate_timer(temp_timer_id);
    if (click_counter < 9)
    {
        sh1122_clear_current_screen(&plat_oled_descriptor);
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_clear_frame_buffer(&plat_oled_descriptor);
        #endif
        sh1122_put_error_string(&plat_oled_descriptor, u"wheel issue");
        timer_delay_ms(5000); platform_io_disable_switch_and_die(); while(1);
    }
    
    /* Annoying click test due to bad experiences: 3 long clicks with a short debounce value (there's HW debouncing!) */
    click_counter = 0;
    timer_delay_ms(500);
    inputs_clear_detections();
    inputs_set_wheel_debounce_value(40);
    temp_timer_id = timer_get_and_start_timer(10000);
    cust_char_t long_click_string[] = u"Long click 3 times QUICKLY";
    sh1122_put_error_string(&plat_oled_descriptor, (const cust_char_t*)long_click_string);
    while ((click_counter < 3) && (timer_has_allocated_timer_expired(temp_timer_id, FALSE) != TIMER_EXPIRED))
    {
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_LONG_CLICK)
        {
            sh1122_clear_current_screen(&plat_oled_descriptor);
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            #endif
            click_counter++;
            long_click_string[11] = '0' + 3 - click_counter;
            if (click_counter != 3)
            {
                sh1122_put_error_string(&plat_oled_descriptor, long_click_string);
            }
        }
    }
    timer_deallocate_timer(temp_timer_id);
    if (click_counter < 3)
    {
        sh1122_clear_current_screen(&plat_oled_descriptor);
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_clear_frame_buffer(&plat_oled_descriptor);
        #endif
        sh1122_put_error_string(&plat_oled_descriptor, u"wheel issue");
        timer_delay_ms(5000); platform_io_disable_switch_and_die(); while(1);
    }
       
    /* Ask to connect USB to test USB LDO + LDO 3V3 to 8V  */
    sh1122_put_error_string(&plat_oled_descriptor, u"connect USB");
    while (platform_io_is_usb_3v3_present_raw() == FALSE);
    comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_ATTACH_USB);
    comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_ATTACH_CMD_RCVD);
    
    /* Wait for enumeration */
    comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_USB_ENUMERATED);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* Switch to LDO for voled stepup */
    logic_power_set_power_source(USB_POWERED);
    sh1122_oled_off(&plat_oled_descriptor);
    timer_delay_ms(5);
    platform_io_power_up_oled(TRUE);
    sh1122_oled_on(&plat_oled_descriptor);
    
    /* Perform bandgap measurement with OLED screen on not displaying anything */
    timer_delay_ms(50);
    uint16_t bandgap_measurement_usb_powered = platform_io_get_single_bandgap_measurement();
    
    /* Check that stepup voltage actually is lower than the ldo voltage (theoretical value is 93) */
    if ((bandgap_measurement_bat_powered - bandgap_measurement_usb_powered) < 70)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Stepup error!");
        while (platform_io_is_usb_3v3_present_raw() != FALSE);
        timer_delay_ms(200); platform_io_disable_switch_and_die(); while(1);
    }
    
    /* Signal the aux MCU do its functional testing */
    platform_io_enable_ble();
    sh1122_put_error_string(&plat_oled_descriptor, u"Please wait...");
    comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_FUNC_TEST);
    
    /* Wait for end of functional test */
    temp_rx_message = comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_FUNC_TEST_DONE);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* Check functional test result */
    uint8_t func_test_result = temp_rx_message->aux_mcu_event_message.payload[0];
    if (func_test_result == 1)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"ATBTLC1000 error!");
        while (platform_io_is_usb_3v3_present_raw() != FALSE);
        timer_delay_ms(200); platform_io_disable_switch_and_die(); while(1);
    }
    else if (func_test_result == 2)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Battery circuit error!");
        while (platform_io_is_usb_3v3_present_raw() != FALSE);
        timer_delay_ms(200); platform_io_disable_switch_and_die(); while(1);
    }
    else if (func_test_result == 3)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Q5/Q9 error");
        while (platform_io_is_usb_3v3_present_raw() != FALSE);
        timer_delay_ms(200); platform_io_disable_switch_and_die(); while(1);
    }
    
    /* Wait for DTM RX message from aux MCU */
    sh1122_put_error_string(&plat_oled_descriptor, u"Waiting DTM RX...");
    while(comms_aux_mcu_active_wait(&temp_rx_message, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, FALSE, AUX_MCU_EVENT_RX_DTM_DONE) != RETURN_OK){}
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
        
    /* Check for received messages: average count is 1565 */
    if (temp_rx_message->aux_mcu_event_message.payload_as_uint16[0] < 1000)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"ATBTLC1000 error / NO RX!");
        while (platform_io_is_usb_3v3_present_raw() != FALSE);
        timer_delay_ms(200); platform_io_disable_switch_and_die(); while(1);
    }
    
    /* Rearm DMA RX */
    comms_aux_arm_rx_and_clear_no_comms();
        
    /* Re-disable bluetooth */
    platform_io_disable_ble();
    
    /* Ask to disconnect USB to test... well I don't really know, but very few devices have this issue  */
    sh1122_put_error_string(&plat_oled_descriptor, u"disconnect USB");
    while (platform_io_is_usb_3v3_present_raw() != FALSE);
    sh1122_oled_off(&plat_oled_descriptor);
    comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_DETACH_USB);
    comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_USB_DETACHED);
    platform_io_disable_3v3_to_oled_stepup();
    logic_power_set_power_source(BATTERY_POWERED);
    logic_aux_mcu_set_usb_enumerated_bool(FALSE);
    platform_io_assert_oled_reset();
    timer_delay_ms(15);
    platform_io_power_up_oled(FALSE);
    sh1122_init_display(&plat_oled_descriptor, TRUE);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* Test accelerometer */
    sh1122_put_error_string(&plat_oled_descriptor, u"Testing accelerometer...");
    temp_timer_id = timer_get_and_start_timer(3000);
    while (timer_has_allocated_timer_expired(temp_timer_id, TRUE) != TIMER_EXPIRED)
    {
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        if (logic_accelerometer_routine() == ACC_FAILING)
        {
            sh1122_put_error_string(&plat_oled_descriptor, u"LIS2HH12 failed!");
            while (platform_io_is_usb_3v3_present_raw() != FALSE);
            timer_delay_ms(200); platform_io_disable_switch_and_die(); while(1);
        }
    }
    
    /* Free timer */
    timer_deallocate_timer(temp_timer_id);
    
    /* Ask for card, tests all SMC related signals */
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    sh1122_put_error_string(&plat_oled_descriptor, u"insert card");
    while (smartcard_low_level_is_smc_absent() == RETURN_OK)
    {
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
    }
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    mooltipass_card_detect_return_te detection_result = smartcard_highlevel_card_detected_routine();
    if ((detection_result == RETURN_MOOLTIPASS_PB) || (detection_result == RETURN_MOOLTIPASS_INVALID))
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Invalid card!");
        while (platform_io_is_usb_3v3_present_raw() != FALSE)
        {
            comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        }
        timer_delay_ms(200); platform_io_disable_switch_and_die(); while(1);
    }
    
    /* Clear flag */
    if (clear_first_boot_flag != FALSE)
    {
        custom_fs_set_device_flag_value(FUNCTIONAL_TEST_PASSED_FLAG_ID, TRUE);
    }
    
    /* We're good! */
    sh1122_put_error_string(&plat_oled_descriptor, u"All Good!");
    temp_timer_id = timer_get_and_start_timer(5000);
    while (timer_has_allocated_timer_expired(temp_timer_id, TRUE) != TIMER_EXPIRED)
    {
        /* Now all SN query to print label */
        comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_SN);
    }
    timer_deallocate_timer(temp_timer_id);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
}
