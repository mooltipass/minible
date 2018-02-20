/*!  \file     inputs.c
*    \brief    Scroll wheel driver
*    Created:  20/02/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "platform_defines.h"
#include "defines.h"
#include "inputs.h"

// Wheel pressed duration counter
volatile uint16_t wheel_click_duration_counter;
// Wheel click counter
volatile uint16_t wheel_click_counter;
// Wheel click return
volatile det_ret_type_te wheel_click_return;
// State machine state
uint16_t wheel_sm_states[] = {0b011, 0b001, 0b000, 0b010};
// Boot to know if we allow next increment
volatile BOOL wheel_increment_armed_up = FALSE;
volatile BOOL wheel_increment_armed_down = FALSE;
// Wheel current increment for caller
volatile int16_t wheel_cur_increment;
// Last wheel state machine index
volatile uint16_t last_wheel_sm;
// To get wheel action, discard release event
BOOL discard_release_event = FALSE;
// Wheel direction reverse bool
BOOL wheel_reverse_bool = FALSE;
// Last detection type returned (cleared when calling cleardetections)
wheel_action_ret_te last_detection_type_ret = WHEEL_ACTION_NONE;


/*! \fn     inputs_scan(void)
*   \brief  Wheel debounce called by 1ms interrupt
*/
void inputs_scan(void)
{    
    uint16_t wheel_state, wheel_sm = 0;
    
    // Wheel encoder    
    wheel_state = ((PORT->Group[WHEEL_A_GROUP].IN.reg & WHEEL_A_MASK) >> WHEEL_A_PINID) | ((PORT->Group[WHEEL_B_GROUP].IN.reg & WHEEL_B_MASK) >> WHEEL_B_PINID-1);

    // Find the state matching the wheel state
    for (uint16_t i = 0; i < sizeof(wheel_sm_states)/sizeof(wheel_sm_states[0]); i++)
    {
        if (wheel_state == wheel_sm_states[i])
        {
            wheel_sm = i;
        }
    }
    if (wheel_sm == ((last_wheel_sm+1)&0x03))
    {
        if (wheel_state == 0)
        {
            wheel_increment_armed_up = TRUE;
        }
        else if ((wheel_state == 0x03) && (wheel_increment_armed_up == TRUE))
        {
            if (wheel_reverse_bool != FALSE)
            {
                wheel_cur_increment++;
            } 
            else
            {
                wheel_cur_increment--;
            }
            wheel_increment_armed_up = FALSE;
            wheel_increment_armed_down = FALSE;
        }
        last_wheel_sm = wheel_sm;
    }
    else if (wheel_sm == ((last_wheel_sm-1)&0x03))
    {
        if (wheel_state == 0)
        {
            wheel_increment_armed_down = TRUE;
        }
        else if ((wheel_state == 0x03) && (wheel_increment_armed_down == TRUE))
        {
            if (wheel_reverse_bool != FALSE)
            {
                wheel_cur_increment--;
            }
            else
            {
                wheel_cur_increment++;
            }
            wheel_increment_armed_up = FALSE;
            wheel_increment_armed_down = FALSE;
        }
        last_wheel_sm = wheel_sm;
    }
    
    // Wheel click
    if ((PORT->Group[WHEEL_SW_GROUP].IN.reg & WHEEL_SW_MASK) == 0)
    {
        if ((wheel_click_counter == 50) && (wheel_click_return != RETURN_JRELEASED))
        {
            wheel_click_return = RETURN_JDETECT;
        }
        if (wheel_click_counter != 0xFF)
        {
            wheel_click_counter++;
        }
        if ((wheel_click_return == RETURN_DET) || (wheel_click_return == RETURN_JDETECT))
        {
            wheel_click_duration_counter++;
        }
    }
    else
    {
        if (wheel_click_return == RETURN_DET)
        {
            wheel_click_return = RETURN_JRELEASED;
        }
        else if (wheel_click_return != RETURN_JRELEASED)
        {
            wheel_click_duration_counter = 0;
            wheel_click_return = RETURN_REL;
        }
        wheel_click_counter = 0;
    }
}

/*! \fn     inputs_get_wheel_increment(void)
*   \brief  Fetch the current increment/decrement for the wheel
*   \return positive or negative depending on the scrolling
*/
int16_t inputs_get_wheel_increment(void)
{
    int16_t return_val = 0;
    
    if (wheel_cur_increment != 0)
    {
        //activityDetectedRoutine();
        cpu_irq_enter_critical();        
        return_val = wheel_cur_increment;
        wheel_cur_increment = 0;
        cpu_irq_leave_critical();        
    }
    
    return return_val;
}

/*! \fn     inputs_is_wheel_clicked(void)
*   \brief  Know if the wheel is clicked
*   \return just released/pressed, (non)detected
*/
det_ret_type_te inputs_is_wheel_clicked(void)
{
    // This copy is an atomic operation
    volatile det_ret_type_te return_val = wheel_click_return;

    if ((return_val != RETURN_DET) && (return_val != RETURN_REL))
    {
        //activityDetectedRoutine();
        cpu_irq_enter_critical();
        if (wheel_click_return == RETURN_JDETECT)
        {
            wheel_click_return = RETURN_DET;
        }
        else if (wheel_click_return == RETURN_JRELEASED)
        {
            wheel_click_return = RETURN_REL;
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
    last_detection_type_ret = WHEEL_ACTION_NONE;
    wheel_increment_armed_down = FALSE;
    wheel_increment_armed_up = FALSE;
    wheel_click_duration_counter = 0;
    wheel_click_return = RETURN_REL;
    wheel_cur_increment = 0;
    cpu_irq_leave_critical();
}

/*! \fn     inputs_get_last_returned_action(void)
*   \brief  Get the last returned action to another call
*   \return See wheel_action_ret_t
*/
RET_TYPE inputs_get_last_returned_action(void)
{
    return last_detection_type_ret;
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
    int16_t wheel_cur_increment_copy = 0;

    do
    {
        // If we want to take into account wheel scrolling
        if (ignore_incdec == FALSE)
        {
            wheel_cur_increment_copy = wheel_cur_increment;
        }

        if (wheel_click_return == RETURN_JDETECT)
        {
            // When checking for actions we clear the just detected state
            cpu_irq_enter_critical();
            wheel_click_return = RETURN_DET;
            cpu_irq_leave_critical();
        }
        if ((wheel_click_return == RETURN_JRELEASED) || (wheel_cur_increment_copy != 0) || (wheel_click_duration_counter > LONG_PRESS_MS))
        {
            cpu_irq_enter_critical();
            if ((wheel_click_duration_counter > LONG_PRESS_MS) && (discard_release_event == FALSE))
            {
                return_val = WHEEL_ACTION_LONG_CLICK;
            }
            else if (wheel_click_return == RETURN_JRELEASED)
            {                    
                if (wheel_cur_increment_copy == 0)
                {
                    if (discard_release_event != FALSE)
                    {
                        discard_release_event = FALSE;
                        return_val = WHEEL_ACTION_DISCARDED;
                    } 
                    else
                    {
                        return_val = WHEEL_ACTION_SHORT_CLICK;
                    }
                }
            }
            else if (wheel_click_return == RETURN_DET)
            {
                if (wheel_cur_increment_copy > 0)
                {
                    return_val = WHEEL_ACTION_CLICK_DOWN;
                }
                else if (wheel_cur_increment_copy < 0)
                {
                    return_val = WHEEL_ACTION_CLICK_UP;
                }
            }
            else
            {
                if (wheel_cur_increment_copy > 0)
                {
                    return_val = WHEEL_ACTION_DOWN;
                }
                else if (wheel_cur_increment_copy < 0)
                {
                    return_val = WHEEL_ACTION_UP;
                }
            }

            // Clear detections
            wheel_click_duration_counter = 0;
            if ((return_val == WHEEL_ACTION_CLICK_DOWN) || (return_val == WHEEL_ACTION_CLICK_UP))
            {
                discard_release_event = TRUE;
            }
            else if(return_val == WHEEL_ACTION_NONE)
            {
                // User has been scrolling then keeping the wheel pressed: do not do anything
            }
            else
            {
                wheel_click_return = RETURN_REL;
            }
            if (ignore_incdec == FALSE)
            {
                wheel_cur_increment = 0;
            }                
            cpu_irq_leave_critical();
        }
    }
    while ((wait_for_action != FALSE) && (return_val == WHEEL_ACTION_NONE));

    // Don't forget to call the activity detected routine if something happened
    if (return_val != WHEEL_ACTION_NONE)
    {
        //activityDetectedRoutine();
    }
    
    last_detection_type_ret = return_val;
    return return_val;
}