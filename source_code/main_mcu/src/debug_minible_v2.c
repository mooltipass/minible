/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2024 Stephan Mathieu
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
/*!  \file     debug_minible_v2.c
*    \brief    Debug functions for our platform
*    Created:  30/10/2024
*    Author:   Mathieu Stephan
*/
#include "platform_defines.h"
#if defined(MINIBLE_V2) && defined(DEBUG_MENU_ENABLED)

#include <string.h>
#include <asf.h>
#include "logic_accelerometer.h"
#include "debug_minible_v2.h"
#include "comms_aux_mcu.h"
#include "oled_wrapper.h"
#include "acc_wrapper.h"
#include "platform_io.h"
#include "pcf85263a.h"
#include "mp2710.h"
#include "inputs.h"
#include "main.h"


/*! \fn     debug_accelerometer_battery_display(void)
*   \brief  Accelerometer & battery debug
*/
void debug_accelerometer_battery_display(void)
{
    /* Debug variables */
    const char* ntc_fault_lut[] = {"No fault","NTC hot fault","NTC warm fault","NTC cool fault","NTC cooler fault","NTC cold fault"};
    const char* charging_status_lut[] = {"Not charging","Pre-charge","CC charge","CV charge","Charging complete"};
    const char* thermal_status_lut[] = {"No thermal regulation","In thermal regulation"};
    const char* power_status_lut[] = {"Power failure","Power good"};
    const char* wd_status_lut[] = {"WD normal","WD expired!"};
    const char* ppm_status_lut[] = {"No PPM","In PPM"};
    uint8_t mp2710_op_stat_reg = mp2710_get_operation_status_register();
    uint8_t mp2710_op_fault_reg = mp2710_get_operation_fault_register();
    uint32_t last_stat_s = driver_timer_get_rtc_timestamp_uint32t();
    pcf85263a_time_structure_t current_time = {.days = 0};
    uint32_t acc_int_nb_interrupts_latched = 0;
    uint32_t acc_int_nb_interrupts = 0;
    uint16_t bat_adc_result = 0;
    uint32_t bat_result_mv = 0;
    
    while(TRUE)
    {
        /* Clear frame buffer */
        oled_check_for_flush_and_terminate(&plat_oled_descriptor);
        oled_clear_frame_buffer(&plat_oled_descriptor);
        
        /* Accelerometer interrupt */
        if (acc_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor, TRUE) != FALSE)
        {
            acc_int_nb_interrupts++;
        }
        
        /* Battery measurement */
        if (platform_io_is_vbat_conversion_result_ready() != FALSE)
        {
            /* Real ratio is 6600/6060.6 */
            bat_adc_result = platform_io_get_vbat_conversion_result_and_trigger_conversion();
            bat_result_mv = (((uint32_t)bat_adc_result)*71369)/65536;
        }
        
        /* Check for Accelerometer SERCOM buffer overflow */
        #ifndef EMULATOR_BUILD
        if (ACC_SERCOM->SPI.STATUS.bit.BUFOVF != 0)
        {
            oled_put_error_string(&plat_oled_descriptor, u"ACC Overflow");      
        }
        #endif
        
        /* Stats latched at second changes */
        if (driver_timer_get_rtc_timestamp_uint32t() != last_stat_s)
        {
            mp2710_op_stat_reg = mp2710_get_operation_status_register();
            mp2710_op_fault_reg = mp2710_get_operation_fault_register();
            last_stat_s = driver_timer_get_rtc_timestamp_uint32t();
            acc_int_nb_interrupts_latched = acc_int_nb_interrupts;
            pcf85263a_get_time(&current_time);
            acc_int_nb_interrupts = 0;
        }
        
        /* Current time */
        oled_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, TRUE, "Time: %02d:%02d:%02d %d/%d/%d", current_time.hours, current_time.minutes, current_time.seconds, current_time.days, current_time.months, current_time.years);
        
        /* Accelerometer data */
        oled_printf_xy(&plat_oled_descriptor, 0, 12, OLED_ALIGN_LEFT, TRUE, "ACC: %uHz X: %i Y: %i Z: %i", acc_int_nb_interrupts_latched*32, plat_acc_descriptor.fifo_read.acc_data_array[0].acc_x >> 6, plat_acc_descriptor.fifo_read.acc_data_array[0].acc_y >> 6, plat_acc_descriptor.fifo_read.acc_data_array[0].acc_z >> 6);
        
        /* Battery data */
        oled_printf_xy(&plat_oled_descriptor, 0, 24, OLED_ALIGN_LEFT, TRUE, "Battery: %dlsb / %dmV", bat_adc_result, (uint16_t)bat_result_mv);
        
        /* mp2710 status */
        oled_printf_xy(&plat_oled_descriptor, 0, 36, OLED_ALIGN_LEFT, TRUE, "%s / %s / %s", charging_status_lut[(mp2710_op_stat_reg >> 3) & 0x07], ppm_status_lut[(mp2710_op_stat_reg >> 2) & 0x01], wd_status_lut[(mp2710_op_stat_reg >> 7) & 0x01]);
        oled_printf_xy(&plat_oled_descriptor, 0, 48, OLED_ALIGN_LEFT, TRUE, "%s / %s", power_status_lut[(mp2710_op_stat_reg >> 1) & 0x01], thermal_status_lut[(mp2710_op_stat_reg >> 0) & 0x01]);
        
        /* mp2710 potential faults */
        if (((mp2710_op_fault_reg >> 6) & 0x01) != 0)
            oled_printf_xy(&plat_oled_descriptor, 0, 60, OLED_ALIGN_LEFT, TRUE, "Input Fault!");
        if (((mp2710_op_fault_reg >> 5) & 0x01) != 0)
            oled_printf_xy(&plat_oled_descriptor, 0, 60, OLED_ALIGN_LEFT, TRUE, "Thermal shutdown!");
        if (((mp2710_op_fault_reg >> 4) & 0x01) != 0)
            oled_printf_xy(&plat_oled_descriptor, 0, 60, OLED_ALIGN_LEFT, TRUE, "Battery OVP!");
        if (((mp2710_op_fault_reg >> 3) & 0x01) != 0)
            oled_printf_xy(&plat_oled_descriptor, 0, 60, OLED_ALIGN_LEFT, TRUE, "Safety timer expired!");
        if (((mp2710_op_fault_reg >> 0) & 0x07) != 0)  
            oled_printf_xy(&plat_oled_descriptor, 0, 60, OLED_ALIGN_LEFT, TRUE, "%s", ntc_fault_lut[(mp2710_op_fault_reg >> 0) & 0x07]);
        
        /* Flush frame buffer */        
        oled_flush_frame_buffer(&plat_oled_descriptor);
        
        /* Get user action */
        wheel_action_ret_te wheel_user_action = inputs_get_wheel_action(FALSE, FALSE);
        
        /* Return ? */
        if ((wheel_user_action == WHEEL_ACTION_SHORT_CLICK) || (wheel_user_action == WHEEL_ACTION_LONG_CLICK))
        {
            return;
        }
    }  
}

/*! \fn     debug_debug_menu(void)
*   \brief  Debug menu
*/
void debug_debug_menu(void)
{
    wheel_action_ret_te wheel_user_action;
    int16_t selected_item = 0;
    BOOL redraw_needed = TRUE;
    
    /* Clear previous detections */
    inputs_clear_detections();
    
    /* launch routine */
    debug_accelerometer_battery_display();
    
    while(1)
    {
        /* Still deal with comms */
        comms_aux_mcu_routine(MSG_NO_RESTRICT);
        
        /* And accelerometer */
        logic_accelerometer_routine();
        
        /* Draw menu */
        if (redraw_needed != FALSE)
        {
            /* Set Emergency Font */
            oled_set_emergency_font(&plat_oled_descriptor);
            
            /* Clear screen */
            redraw_needed = FALSE;
            oled_clear_frame_buffer(&plat_oled_descriptor);
            
            /* Item selection */
            if (selected_item > 19)
            {
                selected_item = 0;
            }
            else if (selected_item < 0)
            {
                selected_item = 19;
            }
            
            oled_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Debug Menu", TRUE);
            
            /* Print items */
            if (selected_item < 4)
            {
                oled_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Accelerometer / Battery Debug", TRUE);
                
                //oled_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Time / Accelerometer / Debug", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Main and Aux MCU Info", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Aux MCU BLE Info", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"NiMH Status", TRUE);
            }
            else if (selected_item < 8)
            {
                oled_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Display All Fonts Glyphs", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Setup Developper Card", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Test Pattern Display", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"Reset Device", TRUE);
            }
            else if (selected_item < 12)
            {
                oled_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Smartcard Debug", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Smartcard Test", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Main MCU Flash", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"Aux MCU Flash", TRUE);
            }
            else if (selected_item < 16)
            {
                oled_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"BLE DTM TX Mode", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"BLE DTM RX Mode", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Functional Test", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"Switch Off", TRUE);
            }
            else
            {
                oled_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Battery Recondition", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Battery Test", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Stack Usage", TRUE);
                oled_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"Reset Settings", TRUE);
            }
            
            /* Cursor */
            oled_put_string_xy(&plat_oled_descriptor, 0, 14 + (selected_item%4)*10, OLED_ALIGN_LEFT, u"-", TRUE);
            
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            oled_flush_frame_buffer(&plat_oled_descriptor);
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
            //if (selected_item == 0)
            //{
            //    debug_debug_screen();
            //}
            //else if (selected_item == 1)
            //{
            //    debug_mcu_and_aux_info();
            //}
            //else if (selected_item == 2)
            //{
            //    debug_atbtlc_info();
            //}
            //else if (selected_item == 3)
            //{
            //    debug_nimh_status();
            //}
            //else if (selected_item == 4)
            //{
            //    debug_glyphs_show();
            //}
            //else if (selected_item == 5)
            //{
            //    debug_setup_dev_card();
            //}
            //else if (selected_item == 6)
            //{
            //    debug_test_pattern_display();
            //}
            //else if (selected_item == 7)
            //{
            //    debug_reset_device();
            //}
            //else if (selected_item == 8)
            //{
            //    debug_smartcard_info();
            //}
            //else if (selected_item == 9)
            //{
            //    debug_smartcard_test();
            //}
            //else if (selected_item == 10)
            //{
            //    custom_fs_set_device_flag_value(DEVICE_WENT_THROUGH_BOOTLOADER_FLAG_ID, TRUE);
            //    custom_fs_settings_set_fw_upgrade_flag();
            //    logic_aux_mcu_disable_ble(TRUE);
            //    main_reboot();
            //}
            //else if (selected_item == 11)
            //{
            //    logic_aux_mcu_flash_firmware_update(TRUE);
            //}
            //else if (selected_item == 12)
            //{
            //    debug_rf_freq_sweep();
            //}
            //else if (selected_item == 13)
            //{
            //    debug_rf_dtm_rx();
            //}
            //else if (selected_item == 14)
            //{
            //    oled_clear_current_screen(&plat_oled_descriptor);
            //    #ifndef EMULATOR_BUILD
            //    functional_testing_start(FALSE);
            //    #endif
            //}
            //else if (selected_item == 15)
            //{
            //    /* Check for USB power */
            //    if (platform_io_is_usb_3v3_present_raw() != FALSE)
            //    {
            //        oled_clear_current_screen(&plat_oled_descriptor);
            //        oled_put_string_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_CENTER, u"Remove USB cable", FALSE);
            //        while((platform_io_is_usb_3v3_present_raw() != FALSE));
            //    }
            //    logic_power_power_down_actions();           // Power off actions
            //    oled_off(&plat_oled_descriptor);     // Display off command
            //    platform_io_power_down_oled();              // Switch off stepup
            //    platform_io_set_wheel_click_pull_down();    // Pull down on wheel click to slowly discharge capacitor
            //    timer_delay_ms(100);                        // From OLED datasheet wait before removing 3V3
            //    platform_io_set_wheel_click_low();          // Completely discharge cap
            //    timer_delay_ms(10);                         // Wait a tad
            //    platform_io_cutoff_power();                 // Die!
            //}
            //else if (selected_item == 16)
            //{
            //    uint32_t temp_uint32;
            //    logic_power_battery_recondition(&temp_uint32);
            //}
            //else if (selected_item == 17)
            //{
            //    debug_test_battery();
            //}
            //else if (selected_item == 18)
            //{
            //    debug_stack_info();
            //}
            //else if (selected_item == 19)
            //{
            //    custom_fs_hard_reset_settings();
            //}
            redraw_needed = TRUE;
        }
    }
}

#endif
