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
/*!  \file     logic_accelerometer.c
*    \brief    Accelerometer logic
*    Created:  08/12/2019
*    Author:   Mathieu Stephan
*/ 
#include "logic_accelerometer.h"
#include "logic_security.h"
#include "driver_timer.h"
#include "logic_device.h"
#include "oled_wrapper.h"
#include "acc_wrapper.h"
#include "logic_power.h"
#include "logic_user.h"
#include "custom_fs.h"
#include "nodemgmt.h"
#include "main.h"
#include "rng.h"
// xyz added value
int32_t logic_accelerometer_x_added;
int32_t logic_accelerometer_y_added;
int32_t logic_accelerometer_z_added;
// xyz average value
int16_t logic_accelerometer_x_average;
int16_t logic_accelerometer_y_average;
int16_t logic_accelerometer_z_average;
// counter for average
uint16_t logic_accelerometer_avg_counter;
// sum of the differences with the average
uint32_t logic_accelerometer_x_cum_diff_avg;
uint32_t logic_accelerometer_y_cum_diff_avg;
uint32_t logic_accelerometer_z_cum_diff_avg;
// boolean to know if we should do the tap detection
BOOL logic_accelerometer_z_tap_detect_enabled = FALSE;
// knock detection sm
uint16_t logic_accelerometer_knock_detect_sm;
// knock detection internal counter
uint16_t logic_accelerometer_knock_detect_counter;
// time stamp of last detect
uint16_t logic_accelerometer_knock_last_det_counter;
// first knock width
uint16_t logic_accelerometer_first_knock_width;
// x movement detection to wakeup device only
uint16_t logic_accelerometer_x_movement_wakeup_only = FALSE;
// penalty counter for free fall / strong move detector
uint16_t logic_accelerometer_strong_move_det_penalty = 0;
uint16_t logic_accelerometer_ff_det_penalty = 0;


/*! \fn     logic_accelerometer_routine(void)
*   \brief  Accelerometer routine
*   \return Any detection, see enum
*/
acc_detection_te logic_accelerometer_routine(void)
{
    /* Accelerometer interrupt */
    if (acc_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor, FALSE) != FALSE)
    {
        /* Use accelerometer data to detect movement & knock, check for working accelerometer as well */
        acc_detection_te return_val = logic_accelerometer_scan_for_action_in_acc_read();
        
        /* Use accelerometer data to feed our RNG */
        rng_feed_from_acc_read();
        
        /* Arm next data receive */
        acc_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor, TRUE);
        
        /* If some movement, wakeup device */
        if ((return_val == ACC_DET_MOVEMENT) && (logic_power_get_power_source() == USB_POWERED))
        {
            logic_device_activity_detected();
        }
        
        /* Rearm watchdog */
        timer_start_timer(TIMER_ACC_WATCHDOG, 60000);
        
        return return_val;
    }
    else
    {
        return ACC_DET_NOTHING;
    }
}

/*! \fn     logic_accelerometer_set_x_movement_detection_wakeup(void)
*   \brief  Set X movement detection for device wakeup
*/
void logic_accelerometer_set_x_movement_detection_wakeup(void)
{
    logic_accelerometer_x_movement_wakeup_only = TRUE;
}

/*! \fn     logic_accelerometer_scan_for_action_in_acc_read(void)
*   \brief  Scan for action in the raw accelerometer data we just got
*   \return Any detection, see enum
*/
acc_detection_te logic_accelerometer_scan_for_action_in_acc_read(void)
{
    acc_detection_te return_val = ACC_DET_NOTHING;
    
    /* Totals for free fall detection */
    uint32_t acc_total_sum = 0;
    
    /* Loop through all the received values */
    for (uint16_t i = 0; i < ARRAY_SIZE(plat_acc_descriptor.fifo_read.acc_data_array); i++)
    {
        /* Get xyz data acceleration values */
        int16_t x_data_val = plat_acc_descriptor.fifo_read.acc_data_array[i].acc_x;
        int16_t y_data_val = plat_acc_descriptor.fifo_read.acc_data_array[i].acc_y;
        int16_t z_data_val = plat_acc_descriptor.fifo_read.acc_data_array[i].acc_z;
        
        /* Add to total sum : 3*32*int16_t can't get to a uint32_t */
        if (x_data_val < 0)
            acc_total_sum += (-x_data_val);
        else
            acc_total_sum += x_data_val;
        if (y_data_val < 0)
            acc_total_sum += (-y_data_val);
        else
            acc_total_sum += y_data_val;
        if (z_data_val < 0)
            acc_total_sum += (-z_data_val);
        else
            acc_total_sum += z_data_val;

        /* Make sure we're not getting an overflow */
        if (logic_accelerometer_x_cum_diff_avg < (UINT32_MAX - UINT16_MAX))
        {
            // Sum of the differences with the average
            if (x_data_val > logic_accelerometer_x_average)
            {
                logic_accelerometer_x_cum_diff_avg += (x_data_val - logic_accelerometer_x_average);
            }
            else
            {
                logic_accelerometer_x_cum_diff_avg += (logic_accelerometer_x_average - x_data_val);
            }
        }

        /* Make sure we're not getting an overflow */
        if (logic_accelerometer_y_cum_diff_avg < (UINT32_MAX - UINT16_MAX))
        {
            // Sum of the differences with the average
            if (y_data_val > logic_accelerometer_y_average)
            {
                logic_accelerometer_y_cum_diff_avg += (y_data_val - logic_accelerometer_y_average);
            }
            else
            {
                logic_accelerometer_y_cum_diff_avg += (logic_accelerometer_y_average - y_data_val);
            }
        }

        /* Make sure we're not getting an overflow */
        if (logic_accelerometer_z_cum_diff_avg < (UINT32_MAX - UINT16_MAX))
        {
            // Sum of the differences with the average
            if (z_data_val > logic_accelerometer_z_average)
            {
                logic_accelerometer_z_cum_diff_avg += (z_data_val - logic_accelerometer_z_average);
            }
            else
            {
                logic_accelerometer_z_cum_diff_avg += (logic_accelerometer_z_average - z_data_val);
            }
        }

        /* Average calculations */
        logic_accelerometer_x_added += x_data_val;
        logic_accelerometer_y_added += y_data_val;
        logic_accelerometer_z_added += z_data_val;
        
        /* Logic done every X samples */
        if (++logic_accelerometer_avg_counter == ACC_Z_AVG_NB_SAMPLES)
        {
            /* Check if we need to reverse the screen */
            if (((logic_accelerometer_x_added >> 8) > ACC_Y_TOTAL_NREVERSE) && (oled_is_screen_inverted(&plat_oled_descriptor) == FALSE))
            {
                /* May be overwritten after but that's alright */
                return_val = ACC_INVERT_SCREEN;
            }
            else if (((logic_accelerometer_x_added >> 8) < ACC_Y_TOTAL_REVERSE) && (oled_is_screen_inverted(&plat_oled_descriptor) != FALSE))
            {
                /* May be overwritten after but that's alright */
                return_val = ACC_NINVERT_SCREEN;
            }
            
            /* Check for failing accelerometer */
            if ((logic_accelerometer_x_cum_diff_avg + logic_accelerometer_y_cum_diff_avg + logic_accelerometer_z_cum_diff_avg) < ACC_AVG_SUM_DIFF_FOR_FAIL)
            {
                return_val = ACC_FAILING;
            }

            /* Compute average */
            logic_accelerometer_x_average = logic_accelerometer_x_added / ACC_Z_AVG_NB_SAMPLES;
            logic_accelerometer_y_average = logic_accelerometer_y_added / ACC_Z_AVG_NB_SAMPLES;
            logic_accelerometer_z_average = logic_accelerometer_z_added / ACC_Z_AVG_NB_SAMPLES;

            /* Depending on the sum of the difference with avg, allow algo or not */
            if ((logic_accelerometer_z_cum_diff_avg >> 8) > ACC_Z_MAX_AVG_SUM_DIFF)
            {
                logic_accelerometer_z_tap_detect_enabled = FALSE;
            }
            else
            {
                logic_accelerometer_z_tap_detect_enabled = TRUE;
            }
            
            /* Reset vars */
            logic_accelerometer_x_added = 0;
            logic_accelerometer_y_added = 0;
            logic_accelerometer_z_added = 0;
            logic_accelerometer_avg_counter = 0;
            logic_accelerometer_x_cum_diff_avg = 0;
            logic_accelerometer_y_cum_diff_avg = 0;
            logic_accelerometer_z_cum_diff_avg = 0;
        }

        /* Current z axis corrected value */
        int16_t z_cor_data_val;
        if (z_data_val > logic_accelerometer_z_average)
        {
            z_cor_data_val = z_data_val - logic_accelerometer_z_average;
        }
        else
        {
            z_cor_data_val = logic_accelerometer_z_average - z_data_val;
        }
        
        /* The algorithm below works on the Z corrected value MSB */
        z_cor_data_val >>= 8;

        /* Knock detection algo */
        if (logic_accelerometer_knock_detect_sm == 0)
        {
            if(z_cor_data_val > custom_fs_settings_get_device_setting(SETTINGS_KNOCK_DETECT_SENSITIVITY))
            {
                logic_accelerometer_knock_detect_sm++;
                logic_accelerometer_first_knock_width = 0;
                logic_accelerometer_knock_detect_counter = 0;
                logic_accelerometer_knock_last_det_counter = 0;
            }
        }
        else if (logic_accelerometer_knock_detect_sm == 1)
        {
            /* Check if second knock */
            if (z_cor_data_val > custom_fs_settings_get_device_setting(SETTINGS_KNOCK_DETECT_SENSITIVITY))
            {
                /* If silence period is respected */
                if (((logic_accelerometer_knock_detect_counter - logic_accelerometer_knock_last_det_counter) > ACC_Z_SECOND_KNOCK_MIN_NBS) && (logic_accelerometer_z_tap_detect_enabled != FALSE) && (logic_security_is_smc_inserted_unlocked() != FALSE) && ((logic_user_get_user_security_flags() & USER_SEC_FLG_KNOCK_DET_DISABLED) == 0))
                {
                    /* Return success */
                    logic_accelerometer_knock_last_det_counter = 0;
                    logic_accelerometer_knock_detect_sm++;
                    if (return_val != ACC_FAILING)
                    {
                        return_val = ACC_DET_KNOCK;
                    }
                }
                else
                {
                    logic_accelerometer_knock_last_det_counter = logic_accelerometer_knock_detect_counter;
                }

                /* Check that the time spent above the threshold isn't too long */
                if (logic_accelerometer_first_knock_width++ > ACC_Z_MAX_KNOCK_PULSE_WIDTH)
                {
                    logic_accelerometer_knock_detect_sm++;
                }
            }

            /* Second knock detection timeout */
            if (logic_accelerometer_knock_detect_counter++ > ACC_Z_SECOND_KNOCK_MAX_NBS)
            {
                logic_accelerometer_knock_detect_sm = 0;
            }
        }
        else if (logic_accelerometer_knock_detect_sm == 2)
        {
            /* Wait before retrigger */
            if (logic_accelerometer_knock_last_det_counter++ > ACC_Z_KNOCK_REARM_WAIT)
            {
                logic_accelerometer_knock_detect_sm = 0;
            }
        }
    }
    
    /* Free fall detection penalty counter */
    if (logic_accelerometer_ff_det_penalty != 0)
    {
        if (logic_accelerometer_ff_det_penalty++ > 100)
        {
            /* Reset penalty counter after 8 seconds */
            logic_accelerometer_ff_det_penalty = 0;
        }
    }
    
    /* Strong move detection penalty counter */
    if (logic_accelerometer_strong_move_det_penalty != 0)
    {
        if (logic_accelerometer_strong_move_det_penalty++ > 100)
        {
            /* Reset penalty counter after 8 seconds */
            logic_accelerometer_strong_move_det_penalty = 0;
        }
    }

    /* If our previous loop detected a knock or a failing accelerometer, it gets priority */
    if ((return_val == ACC_DET_KNOCK) || (return_val == ACC_FAILING))
    {
        return return_val;
    } 
    else
    {
        /* Depending on the threshold, return movement or nothing */
        if ((logic_accelerometer_x_movement_wakeup_only == FALSE) && ((logic_accelerometer_z_cum_diff_avg >> 8) > ACC_Z_MOVEMENT_AVG_SUM_DIFF))
        {
            return ACC_DET_MOVEMENT;
        }
        else if ((logic_accelerometer_x_movement_wakeup_only != FALSE) && ((logic_accelerometer_x_cum_diff_avg >> 8) > ACC_X_MOVEMENT_AVG_SUM_DIFF))
        {
            return ACC_DET_MOVEMENT;
        }
        else
        {
            /* Check for screen inversion */
            if (return_val != ACC_DET_NOTHING)
            {
                return return_val;
            }
            else if ((acc_total_sum < 50000) && (logic_accelerometer_ff_det_penalty == 0))
            {
                /* Free fall detection penalty */
                logic_accelerometer_ff_det_penalty = 1;
                return ACC_FREEFALL;
            }
            else if ((acc_total_sum > 3000000) && (logic_accelerometer_strong_move_det_penalty == 0))
            {
                /* Strong move detection penalty */
                logic_accelerometer_strong_move_det_penalty = 1;
                return ACC_STRONG_MOVE;
            }
            else
            {
                return ACC_DET_NOTHING;
            }
        }
    }
}