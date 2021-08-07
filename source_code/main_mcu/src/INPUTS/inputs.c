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
/*!  \file     inputs.c
*    \brief    Scroll wheel driver
*    Created:  20/02/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "platform_defines.h"
#include "logic_device.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "inputs.h"

#if !defined(PLAT_V5_SETUP) && !defined(PLAT_V6_SETUP) && !defined(PLAT_V7_SETUP)
// Wheel machine states
uint16_t inputs_wheel_sm_states[] = {0b011, 0b001, 0b000, 0b010};
// Boot to know if we allow next increment
volatile BOOL inputs_wheel_increment_armed_up = FALSE;
volatile BOOL inputs_wheel_increment_armed_down = FALSE;
// Last wheel state machine index
volatile uint16_t inputs_last_wheel_sm;
#else
volatile uint16_t inputs_pending_a_state_counter = 0;
volatile BOOL inputs_last_a_state_on = FALSE;
volatile BOOL inputs_last_b_state_on = FALSE;
#endif
// Wheel pressed duration counter
volatile uint16_t inputs_wheel_click_duration_counter;
// Penalty for long click
volatile uint16_t inputs_wheel_long_click_penalty;
// Current wheel click return
volatile det_ret_type_te inputs_wheel_click_return;
// Wheel click counter
volatile uint16_t inputs_wheel_click_counter;
// Wheel current increment for caller
volatile int16_t inputs_wheel_cur_increment;
// To get wheel action, discard release event
BOOL inputs_discard_release_event = FALSE;
// Wheel direction reverse bool
BOOL inputs_wheel_reverse_bool = FALSE;
// Wheel debounce ms value
volatile BOOL inputs_wheel_debounce_value = 100;
// Last detection type returned (cleared when calling cleardetections)
wheel_action_ret_te inputs_last_detection_type_ret = WHEEL_ACTION_NONE;
#ifndef BOOTLOADER
volatile uint16_t inputs_force_reboot_timer = 0;
#endif


#ifndef EMULATOR_BUILD
/*! \fn     inputs_scan(void)
*   \brief  Wheel debounce called by 1ms interrupt
*/
void inputs_scan(void)
{
    #if !defined(PLAT_V5_SETUP) && !defined(PLAT_V6_SETUP) && !defined(PLAT_V7_SETUP)
    uint16_t wheel_state, wheel_sm = 0;
    
    // Wheel encoder    
    wheel_state = ((PORT->Group[WHEEL_A_GROUP].IN.reg & WHEEL_A_MASK) >> WHEEL_A_PINID) | ((PORT->Group[WHEEL_B_GROUP].IN.reg & WHEEL_B_MASK) >> (WHEEL_B_PINID-1));

    // Find the state matching the wheel state
    for (uint16_t i = 0; i < sizeof(inputs_wheel_sm_states)/sizeof(inputs_wheel_sm_states[0]); i++)
    {
        if (wheel_state == inputs_wheel_sm_states[i])
        {
            wheel_sm = i;
        }
    }
    if (wheel_sm == ((inputs_last_wheel_sm+1)&0x03))
    {
        if (wheel_state == 0)
        {
            inputs_wheel_increment_armed_up = TRUE;
        }
        else if ((wheel_state == 0x03) && (inputs_wheel_increment_armed_up == TRUE))
        {
            if (inputs_wheel_reverse_bool != FALSE)
            {
                inputs_wheel_cur_increment++;
            } 
            else
            {
                inputs_wheel_cur_increment--;
            }
            inputs_wheel_increment_armed_up = FALSE;
            inputs_wheel_increment_armed_down = FALSE;
        }
        inputs_last_wheel_sm = wheel_sm;
    }
    else if (wheel_sm == ((inputs_last_wheel_sm-1)&0x03))
    {
        if (wheel_state == 0)
        {
            inputs_wheel_increment_armed_down = TRUE;
        }
        else if ((wheel_state == 0x03) && (inputs_wheel_increment_armed_down == TRUE))
        {
            if (inputs_wheel_reverse_bool != FALSE)
            {
                inputs_wheel_cur_increment--;
            }
            else
            {
                inputs_wheel_cur_increment++;
            }
            inputs_wheel_increment_armed_up = FALSE;
            inputs_wheel_increment_armed_down = FALSE;
        }
        inputs_last_wheel_sm = wheel_sm;
    }
    #else
        BOOL cur_switch_a_state = (((PORT->Group[WHEEL_A_GROUP].IN.reg & WHEEL_A_MASK) >> WHEEL_A_PINID) == 0)? FALSE:TRUE;
        BOOL cur_switch_b_state = (((PORT->Group[WHEEL_B_GROUP].IN.reg & WHEEL_B_MASK) >> WHEEL_B_PINID) == 0)? FALSE:TRUE;
        
        /* Detect A channel transitions */
        if (cur_switch_a_state != inputs_last_a_state_on)
        {
            /* Debouncing */
            if (inputs_pending_a_state_counter++ == 5)
            {
                if (inputs_last_a_state_on == FALSE)
                {
                    if (inputs_last_b_state_on == FALSE)
                    {
                        if (cur_switch_b_state == FALSE)
                        {
                            if (inputs_wheel_reverse_bool == FALSE)
                            {
                                inputs_wheel_cur_increment--;
                            }
                            else
                            {
                                inputs_wheel_cur_increment++;
                            }
                        } 
                        else
                        {
                            if (inputs_wheel_reverse_bool == FALSE)
                            {
                                inputs_wheel_cur_increment++;
                            }
                            else
                            {
                                inputs_wheel_cur_increment--;
                            }
                        }
                    } 
                    else
                    {
                        if (cur_switch_b_state == FALSE)
                        {
                            if (inputs_wheel_reverse_bool == FALSE)
                            {
                                inputs_wheel_cur_increment--;
                            }
                            else
                            {
                                inputs_wheel_cur_increment++;
                            }
                        }
                        else
                        {
                            if (inputs_wheel_reverse_bool == FALSE)
                            {
                                inputs_wheel_cur_increment++;
                            }
                            else
                            {
                                inputs_wheel_cur_increment--;
                            }
                        }
                    }
                } 
                else
                {
                    if (inputs_last_b_state_on == FALSE)
                    {
                        if (cur_switch_b_state == FALSE)
                        {
                            if (inputs_wheel_reverse_bool == FALSE)
                            {
                                inputs_wheel_cur_increment++;
                            }
                            else
                            {
                                inputs_wheel_cur_increment--;
                            }
                        }
                        else
                        {
                            if (inputs_wheel_reverse_bool == FALSE)
                            {
                                inputs_wheel_cur_increment--;
                            }
                            else
                            {
                                inputs_wheel_cur_increment++;
                            }
                        }
                    }
                    else
                    {
                        if (cur_switch_b_state == FALSE)
                        {
                            if (inputs_wheel_reverse_bool == FALSE)
                            {
                                inputs_wheel_cur_increment++;
                            }
                            else
                            {
                                inputs_wheel_cur_increment--;
                            }
                        }
                        else
                        {
                            if (inputs_wheel_reverse_bool == FALSE)
                            {
                                inputs_wheel_cur_increment--;
                            }
                            else
                            {
                                inputs_wheel_cur_increment++;
                            }
                        }
                    }
                }
            
                inputs_last_a_state_on = cur_switch_a_state;
                inputs_last_b_state_on = cur_switch_b_state;
                inputs_pending_a_state_counter = 0;
            }
        }
        else
        {
            inputs_pending_a_state_counter = 0;
        }
    #endif
    
    // Wheel click
    if ((PORT->Group[WHEEL_SW_GROUP].IN.reg & WHEEL_SW_MASK) == 0)
    {
        #ifndef BOOTLOADER
        /* Increment force reboot timer and reboot if it reached target value */
        if (inputs_force_reboot_timer++ == NB_MS_WHEEL_PRESS_FOR_REBOOT)
        {
            /* Switch off screen wait for user to release button */
            platform_io_power_down_oled();
            while ((PORT->Group[WHEEL_SW_GROUP].IN.reg & WHEEL_SW_MASK) == 0);
            DELAYMS(200);
            platform_io_disable_switch_and_die();
            while(1);
        }
        #endif
        
        /* Check that detection wasn't set to invalid */
        if (inputs_wheel_click_return != RETURN_INV_DET)
        {
            /* Still needed even though we have hardware debouncing */
            if ((inputs_wheel_click_counter == inputs_wheel_debounce_value) && (inputs_wheel_click_return != RETURN_JRELEASED))
            {
                inputs_wheel_click_return = RETURN_JDETECT;
            }
            if (inputs_wheel_click_counter != 0xFFFF)
            {
                inputs_wheel_click_counter++;
            }
            if ((inputs_wheel_click_return == RETURN_DET) || (inputs_wheel_click_return == RETURN_JDETECT))
            {
                inputs_wheel_click_duration_counter++;
            }
        }        
    }
    else
    {
        #ifndef BOOTLOADER
        inputs_force_reboot_timer = 0;
        #endif
        
        if (inputs_wheel_click_return == RETURN_DET)
        {
            inputs_wheel_click_return = RETURN_JRELEASED;
        }
        else if (inputs_wheel_click_return != RETURN_JRELEASED)
        {
            inputs_wheel_click_duration_counter = 0;
            inputs_wheel_click_return = RETURN_REL;
        }
        inputs_wheel_click_counter = 0;
    }
}
#endif

/*! \fn     inputs_set_wheel_debounce_value(uint16_t debounce_value)
*   \brief  Set wheel debounce value
*   \param  debounce_value  Wheel debounce value in ms
*/
void inputs_set_wheel_debounce_value(uint16_t debounce_value)
{
    inputs_wheel_debounce_value = debounce_value;
}

/*! \fn     inputs_set_inputs_invert_bool(BOOL invert_bool)
*   \brief  Set bool to invert input logic
*   \param  invert_bool     The inversion boolean
*/
void inputs_set_inputs_invert_bool(BOOL invert_bool)
{
    inputs_wheel_reverse_bool = invert_bool;
}

/*! \fn     inputs_get_wheel_increment(void)
*   \brief  Fetch the current increment/decrement for the wheel
*   \return positive or negative depending on the scrolling
*/
int16_t inputs_get_wheel_increment(void)
{
    int16_t return_val = 0;
    
    if (inputs_wheel_cur_increment != 0)
    {
        logic_device_activity_detected();
        cpu_irq_enter_critical();        
        return_val = inputs_wheel_cur_increment;
        inputs_wheel_cur_increment = 0;
        cpu_irq_leave_critical();        
    }
    
    return return_val;
}

/*! \fn     inputs_raw_is_wheel_released(void)
*   \brief  (raw access) know if the wheel is released
*   \return TRUE if the wheel is release, FALSE otherwise
*   \note   Know what you're doing if you're calling this, this is debounce-prone
*/
BOOL inputs_raw_is_wheel_released(void)
{
#ifndef EMULATOR_BUILD
    if ((PORT->Group[WHEEL_SW_GROUP].IN.reg & WHEEL_SW_MASK) == 0)
    {
        return FALSE;
    } 
    else
    {
        return TRUE;
    }
#else
    return FALSE;
#endif
}

/*! \fn     inputs_is_wheel_clicked(void)
*   \brief  Know if the wheel is clicked
*   \return just released/pressed, (non)detected
*/
det_ret_type_te inputs_is_wheel_clicked(void)
{
    // This copy is an atomic operation
    volatile det_ret_type_te return_val = inputs_wheel_click_return;

    if ((return_val != RETURN_DET) && (return_val != RETURN_REL) && (return_val != RETURN_INV_DET))
    {
        logic_device_activity_detected();
        cpu_irq_enter_critical();
        if (inputs_wheel_click_return == RETURN_JDETECT)
        {
            inputs_wheel_click_return = RETURN_DET;
        }
        else if (inputs_wheel_click_return == RETURN_JRELEASED)
        {
            inputs_wheel_click_return = RETURN_REL;
        }
        cpu_irq_leave_critical();
    }

    return return_val;
}

/*! \fn     inputs_clear_detections(void)
*   \brief  Clear current detections
*/
void inputs_clear_detections(void)
{
    cpu_irq_enter_critical();
    inputs_last_detection_type_ret = WHEEL_ACTION_NONE;
    #if !defined(PLAT_V5_SETUP) && !defined(PLAT_V6_SETUP) && !defined(PLAT_V7_SETUP)
    inputs_wheel_increment_armed_down = FALSE;
    inputs_wheel_increment_armed_up = FALSE;
    #endif
    inputs_wheel_click_return = RETURN_INV_DET;
    inputs_wheel_click_duration_counter = 0;
    inputs_discard_release_event = FALSE;
    inputs_wheel_long_click_penalty = 0;
    inputs_wheel_cur_increment = 0;
    cpu_irq_leave_critical();
}

/*! \fn     inputs_get_last_returned_action(void)
*   \brief  Get the last returned action to another call
*   \return See wheel_action_ret_t
*/
RET_TYPE inputs_get_last_returned_action(void)
{
    return inputs_last_detection_type_ret;
}

/*! \fn     inputs_get_wheel_action(void)
*   \brief  Get current wheel action
*   \param  wait_for_action     Set to TRUE to wait for an action
*   \param  ignore_incdec       Ignore actions linked to wheel scrolling
*   \return See wheel_action_ret_t
*/
wheel_action_ret_te inputs_get_wheel_action(BOOL wait_for_action, BOOL ignore_incdec)
{
    wheel_action_ret_te return_val = WHEEL_ACTION_NONE;
    int16_t inputs_wheel_cur_increment_copy = 0;

    do
    {
        // If we want to take into account wheel scrolling
        if (ignore_incdec == FALSE)
        {
            inputs_wheel_cur_increment_copy = inputs_wheel_cur_increment;
        }
        
        // Check for invalid state
        if (inputs_wheel_click_return == RETURN_INV_DET)
        {
            return WHEEL_ACTION_NONE;
        }

        if (inputs_wheel_click_return == RETURN_JDETECT)
        {
            // When checking for actions we clear the just detected state
            cpu_irq_enter_critical();
            inputs_wheel_click_return = RETURN_DET;
            cpu_irq_leave_critical();
        }
        if ((inputs_wheel_click_return == RETURN_JRELEASED) || (inputs_wheel_cur_increment_copy != 0) || (inputs_wheel_click_duration_counter > LONG_PRESS_MS + inputs_wheel_long_click_penalty))
        {
            cpu_irq_enter_critical();
            if ((inputs_wheel_click_duration_counter > LONG_PRESS_MS + inputs_wheel_long_click_penalty) && (inputs_discard_release_event == FALSE))
            {
                return_val = WHEEL_ACTION_LONG_CLICK;
            }
            else if (inputs_wheel_click_return == RETURN_JRELEASED)
            {                    
                if (inputs_wheel_cur_increment_copy == 0)
                {
                    if ((inputs_discard_release_event != FALSE) || (inputs_wheel_long_click_penalty != 0))
                    {
                        inputs_discard_release_event = FALSE;
                        inputs_wheel_long_click_penalty = 0;
                        return_val = WHEEL_ACTION_DISCARDED;
                    } 
                    else
                    {
                        return_val = WHEEL_ACTION_SHORT_CLICK;
                    }
                }
                
                /* Acknowledge */
                inputs_wheel_click_return = RETURN_REL;
            }
            else if (inputs_wheel_click_return == RETURN_DET)
            {
                if (inputs_wheel_cur_increment_copy > 0)
                {
                    return_val = WHEEL_ACTION_CLICK_DOWN;
                }
                else if (inputs_wheel_cur_increment_copy < 0)
                {
                    return_val = WHEEL_ACTION_CLICK_UP;
                }
            }
            else
            {
                if (inputs_wheel_cur_increment_copy > 0)
                {
                    return_val = WHEEL_ACTION_DOWN;
                }
                else if (inputs_wheel_cur_increment_copy < 0)
                {
                    return_val = WHEEL_ACTION_UP;
                }
            }

            // Clear detections
            inputs_wheel_click_duration_counter = 0;
            if (return_val == WHEEL_ACTION_LONG_CLICK)
            {
                inputs_discard_release_event = TRUE;
            }
            else if ((return_val == WHEEL_ACTION_CLICK_DOWN) || (return_val == WHEEL_ACTION_CLICK_UP))
            {
                inputs_wheel_long_click_penalty = LONG_PRESS_MS*2;
            }
            else if(return_val == WHEEL_ACTION_NONE)
            {
                // User has been scrolling then keeping the wheel pressed: do not do anything
            }
            if (ignore_incdec == FALSE)
            {
                inputs_wheel_cur_increment = 0;
            }                
            cpu_irq_leave_critical();
        }
    }
    while ((wait_for_action != FALSE) && (return_val == WHEEL_ACTION_NONE));

    // Don't forget to call the activity detected routine if something happened
    if (return_val != WHEEL_ACTION_NONE)
    {
        logic_device_activity_detected();
    }
    
    inputs_last_detection_type_ret = return_val;
    return return_val;
}