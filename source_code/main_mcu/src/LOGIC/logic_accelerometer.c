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
#include "logic_device.h"
#include "logic_user.h"
#include "custom_fs.h"
#include "lis2hh12.h"
#include "nodemgmt.h"
#include "sh1122.h"
#include "main.h"
#include "rng.h"
// z added value
int16_t logic_accelerometer_z_added;
// z average value
int16_t logic_accelerometer_z_average;
// z counter for average
uint16_t logic_accelerometer_z_avg_counter;
// sum of the differences with the average
uint16_t logic_accelerometer_z_cum_diff_avg;
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
// accumulation for y axis
int16_t logic_accelerometer_x_cumulated;


/*! \fn     logic_accelerometer_routine(void)
*   \brief  Accelerometer routine
*   \return Any detection, see enum
*/
acc_detection_te logic_accelerometer_routine(void)
{
    /* Accelerometer interrupt */
    if (lis2hh12_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor, FALSE) != FALSE)
    {
        /* Use accelerometer data to feed our RNG */
        rng_feed_from_acc_read();
        
        /* Arm next data receive */
        lis2hh12_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor, TRUE);
        
        /* Use (potentially corrupted) accelerometer data to detect movement & knock */
        acc_detection_te return_val = logic_accelerometer_scan_for_action_in_acc_read();
        
        /* If some movement, wakeup device */
        if (return_val == ACC_DET_MOVEMENT)
        {
            logic_device_activity_detected();
        }
        
        return return_val;
    }
    else
    {
        return ACC_DET_NOTHING;
    }
}

/*! \fn     logic_accelerometer_scan_for_action_in_acc_read(void)
*   \brief  Scan for action in the raw accelerometer data we just got
*   \return Any detection, see enum
*/
acc_detection_te logic_accelerometer_scan_for_action_in_acc_read(void)
{
    acc_detection_te return_val = ACC_DET_NOTHING;
    
    /* Loop through all the received values */
    for (uint16_t i = 0; i < ARRAY_SIZE(plat_acc_descriptor.fifo_read.acc_data_array); i++)
    {
        /* Get z data acceleration value, use 8MSbs */
        int16_t z_data_val = plat_acc_descriptor.fifo_read.acc_data_array[i].acc_z >> 8;

        /* Make sure we're not getting an overflow */
        if (logic_accelerometer_z_cum_diff_avg < (UINT16_MAX - UINT8_MAX))
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
        logic_accelerometer_z_added += z_data_val;
        logic_accelerometer_x_cumulated += plat_acc_descriptor.fifo_read.acc_data_array[i].acc_x >> 8;
        
        /* Logic done every X samples */
        if (++logic_accelerometer_z_avg_counter == ACC_Z_AVG_NB_SAMPLES)
        {
            /* Check if we need to reverse the screen */
            if ((logic_accelerometer_x_cumulated > ACC_Y_TOTAL_NREVERSE) && (sh1122_is_screen_inverted(&plat_oled_descriptor) == FALSE))
            {
                /* May be overwritten after but that's alright */
                return_val = ACC_INVERT_SCREEN;
            }
            else if ((logic_accelerometer_x_cumulated < ACC_Y_TOTAL_REVERSE) && (sh1122_is_screen_inverted(&plat_oled_descriptor) != FALSE))
            {
                /* May be overwritten after but that's alright */
                return_val = ACC_NINVERT_SCREEN;
            }

            /* Compute average  */
            logic_accelerometer_z_average = logic_accelerometer_z_added / ACC_Z_AVG_NB_SAMPLES;

            /* Depending on the sum of the difference with avg, allow algo or not */
            if (logic_accelerometer_z_cum_diff_avg > ACC_Z_MAX_AVG_SUM_DIFF)
            {
                logic_accelerometer_z_tap_detect_enabled = FALSE;
            }
            else
            {
                logic_accelerometer_z_tap_detect_enabled = TRUE;
            }
            
            /* Reset vars */
            logic_accelerometer_z_added = 0;
            logic_accelerometer_x_cumulated = 0;
            logic_accelerometer_z_avg_counter = 0;
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
                    return_val = ACC_DET_KNOCK;
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

    /* If our previous loop detected a knock, it gets priority */
    if (return_val == ACC_DET_KNOCK)
    {
        return return_val;
    } 
    else
    {
        /* Depending on the threshold, return movement or nothing */
        if (logic_accelerometer_z_cum_diff_avg > ACC_Z_MOVEMENT_AVG_SUM_DIFF)
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
            else
            {
                return ACC_DET_NOTHING;
            }
        }
    }
}