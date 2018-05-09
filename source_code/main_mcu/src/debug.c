/*!  \file     debug.c
*    \brief    Debug functions for our platform
*    Created:  21/04/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "platform_defines.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "custom_fs.h"
#include "lis2hh12.h"
#include "sh1122.h"
#include "inputs.h"
#include "debug.h"
#include "main.h"


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
        comms_aux_mcu_routine();
        
        /* Draw menu */
        if (redraw_needed != FALSE)
        {
            /* Clear screen */
            redraw_needed = FALSE;
            sh1122_clear_current_screen(&plat_oled_descriptor);
            
            /* Item selection */
            if (selected_item > 3)
            {
                selected_item = 0;
            }
            else if (selected_item < 0)
            {
                selected_item = 3;
            }
            
            /* Print items */
            sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Debug Menu");
            sh1122_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Time / Accelerometer / Battery");
            sh1122_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Language Switch Test");
            sh1122_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Animation Test");
            sh1122_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"AUX MCU Flash");
            
            /* Cursor */
            sh1122_put_string_xy(&plat_oled_descriptor, 0, 14 + selected_item*10, OLED_ALIGN_LEFT, u"-");
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
        else if (wheel_user_action == WHEEL_ACTION_SHORT_CLICK)
        {
            if (selected_item == 0)
            {
                debug_debug_screen();
            }
            else if (selected_item == 1)
            {
                debug_language_test();
            }
            else if (selected_item == 2)
            {
                debug_debug_animation();
            }
            else if (selected_item == 3)
            {
                logic_aux_mcu_flash_firmware_update();
            }
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
            sh1122_refresh_used_font(&plat_oled_descriptor);
            custom_fs_get_string_from_file(0, &temp_string);
            sh1122_clear_current_screen(&plat_oled_descriptor);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, "%d/%d : ", i+1, custom_fs_flash_header.language_map_item_count);
            sh1122_put_string_xy(&plat_oled_descriptor, 30, 0, OLED_ALIGN_LEFT, custom_fs_get_current_language_text_desc());            
            sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, "String file ID: %d", custom_fs_cur_language_entry.string_file_index);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, "Start font file ID: %d", custom_fs_cur_language_entry.starting_font);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, "Start bitmap file ID: %d", custom_fs_cur_language_entry.starting_bitmap);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_LEFT, "Recommended keyboard file ID: %d", custom_fs_cur_language_entry.keyboard_layout_id);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_LEFT, "Line #0:");
            sh1122_put_string_xy(&plat_oled_descriptor, 50, 50, OLED_ALIGN_LEFT, temp_string);
            timer_delay_ms(2000);
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
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, i);
        }
    }
}

/*! \fn     debug_debug_screen(void)
*   \brief  Debug screen
*/
void debug_debug_screen(void)
{    
    /* Start bat measurement */
    platform_io_get_voledin_conversion_result_and_trigger_conversion();
    
    /* Debug variables */
    uint32_t acc_int_nb_interrupts_latched = 0;
    uint32_t acc_int_nb_interrupts = 0;
    uint16_t bat_adc_result = 0;
    calendar_t temp_calendar;
    uint32_t stat_times[6];    
    timer_get_calendar(&temp_calendar);
    uint32_t last_stat_s = temp_calendar.bit.SECOND;
    
    while(1)
    {
        /* Deal with comms */
        comms_aux_mcu_routine();
        
        /* Clear screen */
        stat_times[0] = timer_get_systick();
        sh1122_clear_current_screen(&plat_oled_descriptor);
        stat_times[1] = timer_get_systick();
        
        /* Data acq */
        stat_times[2] = timer_get_systick();
        
        /* Accelerometer interrupt */
        if (lis2hh12_check_data_received_flag_and_arm_other_transfer(&acc_descriptor) != FALSE)
        {
            acc_int_nb_interrupts++;
        }
        
        /* Battery measurement */
        if (platform_io_is_voledin_conversion_result_ready() != FALSE)
        {
            bat_adc_result = platform_io_get_voledin_conversion_result_and_trigger_conversion();
        }
        
        /* Get calendar */
        timer_get_calendar(&temp_calendar);
        
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
        if (temp_calendar.bit.SECOND != last_stat_s)
        {
            acc_int_nb_interrupts_latched = acc_int_nb_interrupts;
            last_stat_s = temp_calendar.bit.SECOND;
            acc_int_nb_interrupts = 0;
        }
         
        /* Line 2: date */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, "CURRENT TIME: %u:%u:%u %u/%u/%u", temp_calendar.bit.HOUR, temp_calendar.bit.MINUTE, temp_calendar.bit.SECOND, temp_calendar.bit.DAY, temp_calendar.bit.MONTH, temp_calendar.bit.YEAR);
        
        /* Line 3: accelerometer */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, "ACC: Freq %uHz X: %i Y: %i Z: %i", acc_int_nb_interrupts_latched*32, acc_descriptor.fifo_read.acc_data_array[0].acc_x, acc_descriptor.fifo_read.acc_data_array[0].acc_y, acc_descriptor.fifo_read.acc_data_array[0].acc_z);
        
        /* Line 4: battery */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, "BAT: ADC %u, %u mV", bat_adc_result, bat_adc_result*110/273);
        
        /* Display stats */
        stat_times[5] = timer_get_systick();
        
        /* Line 6: display stats */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_LEFT, "STATS MS: text %u, erase %u, stats %u", stat_times[5]-stat_times[4], stat_times[1]-stat_times[0], stat_times[3]-stat_times[2]);
        
        /* Get user action */
        wheel_action_ret_te wheel_user_action = inputs_get_wheel_action(FALSE, FALSE);
        
        /* Go to sleep? */
        if (wheel_user_action == WHEEL_ACTION_UP)
        {
            timer_delay_ms(2000);
            main_standby_sleep();
        }   
        
        /* Upgrade firmware? */
        if (wheel_user_action == WHEEL_ACTION_CLICK_DOWN)
        {
            custom_fs_settings_set_fw_upgrade_flag();
            cpu_irq_disable();
            NVIC_SystemReset();
        }
        
        /* Delay for display */
        //timer_delay_ms(10);
    }
}
