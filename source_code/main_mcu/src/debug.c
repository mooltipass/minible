/*!  \file     debug.c
*    \brief    Debug functions for our platform
*    Created:  21/04/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "platform_defines.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "lis2hh12.h"
#include "sh1122.h"
#include "inputs.h"
#include "debug.h"
#include "main.h"

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
