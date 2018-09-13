/*!  \file     logic_battery.c
*    \brief    Battery charging related functions
*    Created:  13/09/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "logic_battery.h"
#include "driver_timer.h"
#include "platform_io.h"
/* Current state machine */
lb_state_machine_te logic_battery_state = LB_IDLE;
/* Current voltage set for charging */
uint16_t logic_battery_charge_voltage = 0;

/*! \fn     logic_battery_init(void)
*   \brief  Initialize battery logic
*/
void logic_battery_init(void)
{
    /* Arm our action timer */
    timer_start_timer(TIMER_BATTERY_TICK, LOGIC_BATTERY_TIME_BETWEEN_ACTS);
    
    /* Start current sense measurements */
    platform_io_get_cursense_conversion_result_and_trigger_conversion(); 
}

/*! \fn     logic_battery_start_charging(void)
*   \brief  Called to start battery charge
*/
void logic_battery_start_charging(void)
{
    /* Set charge voltage var */
    logic_battery_charge_voltage = LOGIC_BATTERY_BAT_START_CHG_VOLTAGE;
    
    /* State machine change */
    logic_battery_state = LB_CHARGE_START_RAMPING;
    
    /* Enable stepdown at provided voltage */
    platform_io_enable_step_down(logic_battery_charge_voltage);
    
    /* Wait a little: to characterize later on! */
    timer_delay_ms(100);
    
    /* Enable charge mosfets */
    platform_io_enable_charge_mosfets();
    
    /* Discard first measurement */
    while (platform_io_is_current_sense_conversion_result_ready() == FALSE);
    platform_io_get_cursense_conversion_result_and_trigger_conversion(); 
}

/*! \fn     logic_battery_task(void)
*   \brief  Function regularly called by main()
*/
void logic_battery_task(void)
{
    /* For charge start, we ramp as fast as possible */
    if (logic_battery_state == LB_CHARGE_START_RAMPING)
    {
        /* Did we get current measurements? */
        if (platform_io_is_current_sense_conversion_result_ready() != FALSE)
        {
            /* Extract low and high voltage sense */
            uint32_t cur_sense_vs = platform_io_get_cursense_conversion_result_and_trigger_conversion();
            volatile uint16_t high_voltage = (uint16_t)(cur_sense_vs >> 16);
            volatile uint16_t low_voltage = (uint16_t)cur_sense_vs;
            
            /* Is current starting to flow into the battery? */
            if ((high_voltage - low_voltage) > LOGIC_BATTERY_CUR_FOR_ST_RAMP_END)
            {
                /* Change state machine */
                logic_battery_state = LB_CHARGING;
            }
            else
            {
                /* Increase charge voltage */
                logic_battery_charge_voltage += LOGIC_BATTERY_BAT_START_CHG_V_INC;
                
                /* Check for over voltage */
                if (logic_battery_charge_voltage >= LOGIC_BATTERY_MAX_V_FOR_ST_RAMP)
                {
                    /* Error state */
                    logic_battery_state = LB_ERROR;
                    
                    /* Disable charging */
                    platform_io_disable_charge_mosfets();
                    
                    /* TODO: disable DAC */
                } 
                else
                {
                    /* Increment voltage */
                    platform_io_update_step_down_voltage(logic_battery_charge_voltage);
                }
            }            
        }
    } 
    else
    {
        /* Time to shine? */
        if (timer_has_timer_expired(TIMER_BATTERY_TICK, TRUE) == TIMER_EXPIRED)
        {
            /* What's our current state? */
            switch(logic_battery_state)
            {
                case LB_CHARGE_START_RAMPING:
                {
                    /* Done in the if() before */
                    break;
                }
                
                case LB_IDLE:
                {
                    /* Not much to do there.... */
                    break;
                }
            }
            
            /* Re-arm timer */
            timer_start_timer(TIMER_BATTERY_TICK, LOGIC_BATTERY_TIME_BETWEEN_ACTS);
        }
    }
}