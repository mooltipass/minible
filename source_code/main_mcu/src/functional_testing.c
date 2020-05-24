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
        sh1122_put_error_string(&plat_oled_descriptor, u"Nocomms error!");
        while(1);
    }
    platform_io_clear_no_comms();
    
    /* Receive pending ping */
    sh1122_put_error_string(&plat_oled_descriptor, u"Pinging Aux MCU...");
    while(comms_aux_mcu_active_wait(&temp_rx_message, FALSE, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, FALSE, AUX_MCU_EVENT_IM_HERE) != RETURN_OK);
    comms_aux_arm_rx_and_clear_no_comms();
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* First boot should be done using battery */
    if (platform_io_is_usb_3v3_present_raw() != FALSE)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Power using battery first!");
        while (platform_io_is_usb_3v3_present_raw() != FALSE);
        DELAYMS(200);
        platform_io_disable_switch_and_die();
        while(1);
    }
    
    /* Check for removed card */
    if (smartcard_low_level_is_smc_absent() != RETURN_OK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Please remove the card first!");
        while (smartcard_low_level_is_smc_absent() != RETURN_OK);
        DELAYMS(200);
        platform_io_disable_switch_and_die();
        while(1);
    }
    
    /* Internal 1V1 measurement & VOLED VIN when battery powered and screen not powered */
    platform_io_power_down_oled();
    platform_io_set_voled_vin_as_pulldown();
    DELAYMS(400);
    sh1122_oled_off(&plat_oled_descriptor);
    DELAYMS(50);
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
        while (smartcard_low_level_is_smc_absent() != RETURN_OK);
        DELAYMS(200);
        platform_io_disable_switch_and_die();
        while(1);
    }
    
    /* Wheel testing */
    sh1122_put_error_string(&plat_oled_descriptor, u"scroll up");
    while (inputs_get_wheel_action(FALSE, FALSE) != WHEEL_ACTION_UP);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    sh1122_put_error_string(&plat_oled_descriptor, u"scroll down");
    while (inputs_get_wheel_action(FALSE, FALSE) != WHEEL_ACTION_DOWN);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    sh1122_put_error_string(&plat_oled_descriptor, u"click");
    while (inputs_get_wheel_action(FALSE, FALSE) != WHEEL_ACTION_SHORT_CLICK);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    
    /* Ask to connect USB to test USB LDO + LDO 3V3 to 8V  */
    sh1122_put_error_string(&plat_oled_descriptor, u"connect USB");
    while (platform_io_is_usb_3v3_present_raw() == FALSE);
    comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_ATTACH_USB);
    
    /* Wait for enumeration */
    while(comms_aux_mcu_active_wait(&temp_rx_message, FALSE, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, FALSE, AUX_MCU_EVENT_USB_ENUMERATED) != RETURN_OK);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    comms_aux_arm_rx_and_clear_no_comms();
    
    /* Switch to LDO for voled stepup */
    logic_power_set_power_source(USB_POWERED);
    sh1122_oled_off(&plat_oled_descriptor);
    timer_delay_ms(5);
    platform_io_power_up_oled(TRUE);
    sh1122_oled_on(&plat_oled_descriptor);
    
    /* Perform bandgap measurement with OLED screen on not displaying anything */
    DELAYMS(50);
    uint16_t bandgap_measurement_usb_powered = platform_io_get_single_bandgap_measurement();
    
    /* Check that stepup voltage actually is lower than the ldo voltage (theoretical value is 103, typical value is 97) */
    if ((bandgap_measurement_bat_powered - bandgap_measurement_usb_powered) < 80)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Stepup error!");
        while (platform_io_is_usb_3v3_present_raw() != FALSE);
        DELAYMS(200);
        platform_io_disable_switch_and_die();
        while(1);
    }
    
    /* Signal the aux MCU do its functional testing */
    platform_io_enable_ble();
    sh1122_put_error_string(&plat_oled_descriptor, u"Please wait...");
    comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_FUNC_TEST);
    
    /* Wait for end of functional test */
    while(comms_aux_mcu_active_wait(&temp_rx_message, FALSE, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, FALSE, AUX_MCU_EVENT_FUNC_TEST_DONE) != RETURN_OK);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    comms_aux_arm_rx_and_clear_no_comms();
    
    /* Check functional test result */
    uint8_t func_test_result = temp_rx_message->aux_mcu_event_message.payload[0];
    if (func_test_result == 1)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"ATBTLC1000 error!");
        while (platform_io_is_usb_3v3_present_raw() != FALSE);
        DELAYMS(200);
        platform_io_disable_switch_and_die();
        while(1);
    }
    else if (func_test_result == 2)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Battery circuit error!");
        while (platform_io_is_usb_3v3_present_raw() != FALSE);
        DELAYMS(200);
        platform_io_disable_switch_and_die();
        while(1);
    }
    else if (func_test_result == 3)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Q5/Q9 error");
        while (platform_io_is_usb_3v3_present_raw() != FALSE);
        DELAYMS(200);
        platform_io_disable_switch_and_die();
        while(1);
    }
    
    /* Re-disable bluetooth */
    platform_io_disable_ble();
    
    /* Test accelerometer */
    sh1122_put_error_string(&plat_oled_descriptor, u"Testing accelerometer...");
    timer_start_timer(TIMER_TIMEOUT_FUNCTS, 2000);
    while (timer_has_timer_expired(TIMER_TIMEOUT_FUNCTS, TRUE) != TIMER_EXPIRED)
    {
        if (logic_accelerometer_routine() == ACC_FAILING)
        {
            sh1122_put_error_string(&plat_oled_descriptor, u"LIS2HH12 failed!");
            while (platform_io_is_usb_3v3_present_raw() != FALSE);
            DELAYMS(200);
            platform_io_disable_switch_and_die();
            while(1);
        }
    }
    
    /* Ask for card, tests all SMC related signals */
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    sh1122_put_error_string(&plat_oled_descriptor, u"insert card");
    while (smartcard_low_level_is_smc_absent() == RETURN_OK);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
    mooltipass_card_detect_return_te detection_result = smartcard_highlevel_card_detected_routine();
    if ((detection_result == RETURN_MOOLTIPASS_PB) || (detection_result == RETURN_MOOLTIPASS_INVALID))
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Invalid card!");
        while (platform_io_is_usb_3v3_present_raw() != FALSE);
        DELAYMS(200);
        platform_io_disable_switch_and_die();
        while(1);
    }
    
    /* Clear flag */
    if (clear_first_boot_flag != FALSE)
    {
        custom_fs_set_device_flag_value(FUNCTIONAL_TEST_PASSED_FLAG_ID, TRUE);
    }
    
    /* We're good! */
    sh1122_put_error_string(&plat_oled_descriptor, u"All Good!");    
    timer_delay_ms(4000);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    #endif
}