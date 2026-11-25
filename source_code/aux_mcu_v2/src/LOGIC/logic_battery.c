/*!  \file     logic_battery.c
*    \brief    Battery charging related functions
*    Created:  13/09/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "comms_main_mcu.h"
#include "logic_battery.h"
#include "driver_timer.h"
#include "platform_io.h"
/* Array mapping battery levels with adc readouts */
uint16_t logic_battery_battery_level_mapping[11] = {BATTERY_ADC_OUT_CUTOUT, BATTERY_ADC_10PCT_VOLTAGE, BATTERY_ADC_20PCT_VOLTAGE, BATTERY_ADC_30PCT_VOLTAGE, BATTERY_ADC_40PCT_VOLTAGE, BATTERY_ADC_50PCT_VOLTAGE, BATTERY_ADC_60PCT_VOLTAGE, BATTERY_ADC_70PCT_VOLTAGE, BATTERY_ADC_80PCT_VOLTAGE, BATTERY_ADC_90PCT_VOLTAGE, BATTERY_ADC_100PCT_VOLTAGE};
/* Current battery level in percent tenth */
uint16_t logic_battery_current_battery_level = 0;
/* Current charging type / speed */
lb_nimh_charge_scheme_te logic_battery_charging_type = NIMH_RECOVERY_23C_CHARGING;
/* Counter when wanting to keep battery charge to a low current */
uint32_t logic_battery_low_charge_current_counter = 0;
/* Current state machine */
lb_state_machine_te logic_battery_state = LB_IDLE;
/* Current goal for ramping start */
uint32_t logic_battery_ramping_current_goal = 0;
/* Current voltage set for charging */
uint16_t logic_battery_charge_voltage = 0;
/* Peak voltage at the battery */
uint16_t logic_battery_peak_voltage = 0;
/* Boolean to skip recovery charge initial logic */
BOOL logic_battery_skip_initial_recovery_logic = FALSE;
/* Number of ticks since we saw the peak voltage */
uint16_t logic_battery_nb_end_condition_counter = 0;
uint32_t logic_battery_nb_secs_in_cur_maintain = 0;
uint32_t logic_battery_nb_secs_since_peak = 0;
uint16_t logic_battery_last_second_seen = 0;
uint16_t logic_battery_nb_abnormal_eoc = 0;
uint32_t logic_battery_diag_estimat_chg = 0;
uint32_t logic_battery_diag_last_ts = 0;
/* Flag to start/stop using the ADC */
BOOL logic_battery_start_using_adc_flag = FALSE;
BOOL logic_battery_stop_using_adc_flag = FALSE;
BOOL logic_battery_using_adc_flag = FALSE;
/* Counter for how many ADC measurements should be discarded */
BOOL logic_battery_discard_next_adc_measurement_counter = 0;
/* Diagnostic values */
BOOL logic_battery_diag_charging_forced = FALSE;
uint16_t logic_battery_diag_current_vbat = 0;
int16_t logic_battery_diag_current_cur = 0;


/*! \fn     logic_battery_init(void)
*   \brief  Initialize battery logic
*/
void logic_battery_init(void)
{    
}

/*! \fn     logic_battery_get_charging_status(void)
*   \brief  Get current battery charging status
*   \return Battery charging status
*/
lb_state_machine_te logic_battery_get_charging_status(void)
{
    return logic_battery_state;
}

/*! \fn     logic_battery_get_vbat(void)
*   \brief  Get current battery voltage
*   \return Battery voltage (ADC value)
*/
uint16_t logic_battery_get_vbat(void)
{
    return logic_battery_diag_current_vbat;
}

/*! \fn     logic_battery_get_charging_current(void)
*   \brief  Get current battery charging current (if any)
*   \return Charging current
*/
int16_t logic_battery_get_charging_current(void)
{
    return logic_battery_diag_current_cur;
}

/*! \fn     logic_battery_get_stepdown_voltage(void)
*   \brief  Get current stepdown voltage set
*   \return The stepdown voltage set
*/
uint16_t logic_battery_get_stepdown_voltage(void)
{
    return logic_battery_charge_voltage;
}

/*! \fn     logic_battery_debug_force_charge_voltage(uint16_t charge_voltage)
*   \brief  Debug function to force a given charge voltage - use with care!!!!
*   \param  charge_voltage  Charge voltage to be set
*/
void logic_battery_debug_force_charge_voltage(uint16_t charge_voltage)
{
    if (logic_battery_diag_charging_forced == FALSE)
    {
        /* Enable stepdown at provided voltage */
        platform_io_enable_step_down(charge_voltage);
        
        /* Leave a little time for step down voltage to stabilize */
        timer_delay_ms(1);
        
        /* Enable charge mosfets */
        platform_io_enable_charge_mosfets();

        /* Set boolean */
        logic_battery_diag_charging_forced = TRUE;
    }
    else
    {
        /* Increment voltage */
        platform_io_update_step_down_voltage(charge_voltage);
    }
}

/*! \fn     logic_battery_debug_stop_charge(void)
*   \brief  Debug function to stop the charge
*/
void logic_battery_debug_stop_charge(void)
{
    /* Disable charging */
    platform_io_disable_charge_mosfets();
    timer_delay_ms(1);
    
    /* Disable step-down */
    platform_io_disable_step_down();

    /* Reset bool */
    logic_battery_diag_charging_forced = FALSE;
}

/*! \fn     logic_battery_start_charging(lb_nimh_charge_scheme_te charging_type)
*   \brief  Called to start battery charge
*   \param  charging_type Charging scheme
*/
void logic_battery_start_charging(lb_nimh_charge_scheme_te charging_type)
{
    /* Set charge voltage var */
    logic_battery_charge_voltage = LOGIC_BATTERY_BAT_START_CHG_VOLTAGE;
    logic_battery_diag_estimat_chg = 0;
    
    /* Initial state machine: for recovery & slow starts, do an initial battery test as these 2 methods have a pause of some sorts, making it long to detect disconnected batteries) */
    if ((charging_type == NIMH_RECOVERY_23C_CHARGING) || (charging_type == NIMH_SLOWSTART_23C_CHARGING))
    {
        logic_battery_state = LB_INIT_BAT_TEST;
    } 
    else
    {
        logic_battery_state = LB_CHARGE_START_RAMPING;
        logic_battery_diag_last_ts = timer_get_systick();
    }
    
    /* Ramping current goal */
    if (charging_type == NIMH_TRICKLE_CHARGING)
    {
        logic_battery_ramping_current_goal = LOGIC_BATTERY_CUR_FOR_TRICKLE;
    }
    else if (charging_type == NIMH_RECOVERY_23C_CHARGING)
    {
        logic_battery_ramping_current_goal = LOGIC_BATTERY_CUR_FOR_RECOVERY;
    }
    else
    {
        logic_battery_ramping_current_goal = LOGIC_BATTERY_CUR_FOR_ST_RAMP_END;
    }
    
    /* Type of charging storage */
    logic_battery_charging_type = charging_type;

    /* Allow battery status updates */
    logic_battery_current_battery_level = 0;
    
    /* Enable stepdown at provided voltage */
    platform_io_enable_step_down(logic_battery_charge_voltage);
    
    /* Leave a little time for step down voltage to stabilize */
    timer_delay_ms(1);
    
    /* Enable charge mosfets */
    platform_io_enable_charge_mosfets();

    /* Reset vars */
    logic_battery_skip_initial_recovery_logic = FALSE;
    logic_battery_low_charge_current_counter = 0;
    logic_battery_nb_end_condition_counter = 0;
    logic_battery_nb_secs_in_cur_maintain = 0;
    logic_battery_nb_secs_since_peak = 0;
    logic_battery_nb_abnormal_eoc = 0;
    logic_battery_peak_voltage = 0;
}

/*! \fn     logic_battery_stop_charging(void)
*   \brief  Stop charging if we are
*/
void logic_battery_stop_charging(void)
{
    if ((logic_battery_state == LB_INIT_BAT_TEST) || (logic_battery_state == LB_CHARGE_START_RAMPING) || (logic_battery_state == LB_CHARGING_REACH) || (logic_battery_state == LB_CUR_MAINTAIN))
    {        
        /* Disable charging */
        platform_io_disable_charge_mosfets();
        timer_delay_ms(1);
        
        /* Disable step-down */
        platform_io_disable_step_down();
    }
        
    /* Reset state machine */
    logic_battery_state = LB_IDLE;
}

/*! \fn     logic_battery_stop_using_adc(void)
*   \brief  Stop using the ADC
*/
void logic_battery_stop_using_adc(void)
{
    logic_battery_stop_using_adc_flag = TRUE;
}

/*! \fn     logic_battery_start_using_adc(void)
*   \brief  Start using the ADC
*/
void logic_battery_start_using_adc(void)
{
    logic_battery_start_using_adc_flag = TRUE;
}

/*! \fn     logic_battery_is_using_adc(void)
*   \brief  Know if we're using ADC
*   \return The boolean in question
*/
BOOL logic_battery_is_using_adc(void)
{
    return logic_battery_using_adc_flag;
}

/*! \fn     logic_battery_task(void)
*   \brief  Deal with battery related tasks such as charging
*   \return Any action that may be needed
*/
battery_action_te logic_battery_task(void)
{
    battery_action_te return_value = BAT_ACT_NONE;
    
    /* Very odd case that might happen if logic_battery_task is never called */
    if ((logic_battery_stop_using_adc_flag != FALSE) && (logic_battery_start_using_adc_flag != FALSE))
    {
        logic_battery_stop_using_adc_flag = FALSE;
        logic_battery_start_using_adc_flag = FALSE;
    }
    
    /* Wait if we've been instructed to */
    if (logic_battery_stop_using_adc_flag != FALSE)
    {
        if (logic_battery_using_adc_flag != FALSE)
        {
            while(platform_io_is_current_sense_conversion_result_ready() == FALSE);
        }
        logic_battery_using_adc_flag = FALSE;
    }
    
    /* If we've been told to, arm ADC */
    if (logic_battery_start_using_adc_flag != FALSE)
    {
        platform_io_get_cursense_conversion_result(TRUE);
        logic_battery_using_adc_flag = TRUE;
    }

    /* Did we get current measurements? */
    if (platform_io_is_current_sense_conversion_result_ready() != FALSE)
    {
        /* Extract low and high voltage sense */
        uint32_t cur_sense_vs = platform_io_get_cursense_conversion_result(FALSE);
        uint32_t high_voltage = (cur_sense_vs >> 16) & 0x00000FFFF;
        uint32_t low_voltage = cur_sense_vs & 0x00000FFFF;
        
        /* Sanity checks on measured voltages (due to slow interrupt) */
        if (high_voltage < low_voltage)
        {
            high_voltage = low_voltage;
        }
        
        /* Boolean to know if we already sent a status message to the main MCU */
        BOOL status_message_sent_to_main_mcu = FALSE;
        
        /* Possible new battery level */
        uint16_t possible_new_battery_level = 0;
        
        /* Diagnostic values */
        logic_battery_diag_current_cur = (high_voltage - low_voltage);
        logic_battery_diag_current_vbat = low_voltage;
        
        /* If we're not told to skip next measurement due to DAC value change */
        if (logic_battery_discard_next_adc_measurement_counter == 0)
        {
            /* What's our current state? */
            switch(logic_battery_state)
            {
                /* Rest in between charging states */
                case LB_CHARGE_REST:
                {
                    if ((logic_battery_charging_type == NIMH_RECOVERY_23C_CHARGING) && (logic_battery_low_charge_current_counter > (LOGIC_BATTERY_NB_MIN_RECOV_REST*60UL*1000UL)/LOGIC_BATTERY_AVG_TIME_BTW_ADC_INT))
                    {
                        /* Rest after first constant small current charge: go full speed! */
                        logic_battery_low_charge_current_counter = 0;
                        logic_battery_state = LB_CHARGE_START_RAMPING;
                        logic_battery_diag_last_ts = timer_get_systick();
                        logic_battery_skip_initial_recovery_logic = TRUE;
                        logic_battery_discard_next_adc_measurement_counter = 30;
                        logic_battery_charge_voltage = LOGIC_BATTERY_BAT_START_CHG_VOLTAGE;
                        
                        /* Enable stepdown at provided voltage */
                        platform_io_enable_step_down(logic_battery_charge_voltage);
                        
                        /* Leave a little time for step down voltage to stabilize */
                        timer_delay_ms(1);
                        
                        /* Enable charge mosfets */
                        platform_io_enable_charge_mosfets();
                    }
                
                    /* Increment counter */
                    logic_battery_low_charge_current_counter++;
                    break;
                }
                
                /* Start of recovery or slow start charge: we ramp up the voltage to check if a battery is actually there */
                case LB_INIT_BAT_TEST:
                {
                    /* Is enough current flowing into the battery? */
                    if (((high_voltage - low_voltage) > LOGIC_BATTERY_CUR_FOR_ST_RAMP_END) && (low_voltage > LOGIC_BATTERY_MIN_V_FOR_CUR_MES))
                    {
                        /* Start the initial charging procedure */
                        logic_battery_state = LB_CHARGE_START_RAMPING;
                        logic_battery_diag_last_ts = timer_get_systick();
                        logic_battery_discard_next_adc_measurement_counter = 30;
                        logic_battery_charge_voltage = LOGIC_BATTERY_BAT_START_CHG_VOLTAGE;
                        platform_io_update_step_down_voltage(logic_battery_charge_voltage);
                    }
                    else
                    {
                        /* Increase charge voltage */
                        logic_battery_charge_voltage += 10;
                        
                        /* Check for over voltage - may be caused by disconnected discharge path */
                        if ((low_voltage >= LOGIC_BATTERY_MAX_V_FOR_ST_RAMP) || (logic_battery_charge_voltage > 1700))
                        {
                            /* Error state */
                            logic_battery_state = LB_ERROR_BAT_TEST;
                            
                            /* Disable charging */
                            platform_io_disable_charge_mosfets();
                            timer_delay_ms(1);
                            
                            /* Disable step-down */
                            platform_io_disable_step_down();
                            
                            /* Inform main MCU */
                            comms_main_mcu_send_simple_event(AUX_MCU_EVENT_CHARGE_FAIL);
                            return_value = BAT_ACT_CHARGE_FAIL;
                        }
                        else
                        {
                            /* Increment voltage */
                            platform_io_update_step_down_voltage(logic_battery_charge_voltage);
                            logic_battery_discard_next_adc_measurement_counter = 1;
                        }
                    }
                    
                    break;
                }
            
                /* Quick current ramp */
                case LB_CHARGE_START_RAMPING:
                {
                    /* Is enough current flowing into the battery? */
                    if (((high_voltage - low_voltage) > logic_battery_ramping_current_goal) && (low_voltage > LOGIC_BATTERY_MIN_V_FOR_CUR_MES))
                    {
                        if (logic_battery_charging_type == NIMH_TRICKLE_CHARGING)
                        {
                            /* Trickle charging: do nothing */
                        }
                        else if ((logic_battery_charging_type == NIMH_RECOVERY_23C_CHARGING) && (logic_battery_skip_initial_recovery_logic == FALSE) && (low_voltage <= LOGIC_BATTERY_MAX_V_FOR_RECOVERY_CG))
                        {
                            /* Recovery: 0.1C until battery reaches given voltage */
                        }
                        else if ((logic_battery_charging_type == NIMH_SLOWSTART_23C_CHARGING) && (logic_battery_low_charge_current_counter <= (LOGIC_BATTERY_NB_MIN_SLOW_START*60UL*1000UL)/LOGIC_BATTERY_AVG_TIME_BTW_ADC_INT))
                        {
                            /* Slow start: keep current at low value for a fixed time before increasing it */
                        }
                        else
                        {
                            if ((logic_battery_charging_type == NIMH_RECOVERY_23C_CHARGING) && (logic_battery_skip_initial_recovery_logic == FALSE))
                            {
                                /* Recovery charge: let the battery rest before charging it at full speed */
                                logic_battery_low_charge_current_counter = 0;
                                logic_battery_state = LB_CHARGE_REST;
                                
                                /* Update diagnostics */
                                logic_battery_diag_estimat_chg += logic_battery_ramping_current_goal * ((timer_get_systick() - logic_battery_diag_last_ts) >> 9);
                                
                                /* Disable charging */
                                platform_io_disable_charge_mosfets();
                                timer_delay_ms(1);
                                
                                /* Disable step-down */
                                platform_io_disable_step_down();
                            }
                            else
                            {
                                /* Move to the next state */
                                logic_battery_skip_initial_recovery_logic = FALSE;
                                logic_battery_state = LB_CHARGING_REACH;
                                
                                /* Update diagnostics */
                                logic_battery_diag_estimat_chg += logic_battery_ramping_current_goal * ((timer_get_systick() - logic_battery_diag_last_ts) >> 9);                             
                            }                  
                        }                        
                    }
                    else
                    {
                        /* Increase charge voltage */
                        if (logic_battery_charge_voltage < UINT16_MAX - LOGIC_BATTERY_BAT_START_CHG_V_INC)
                        {
                            /* Trickle charging: increment slowly */
                            if (logic_battery_charging_type == NIMH_TRICKLE_CHARGING)
                            {
                                logic_battery_charge_voltage += 1;
                            } 
                            else
                            {
                                logic_battery_charge_voltage += LOGIC_BATTERY_BAT_START_CHG_V_INC;
                            }
                        }
                        
                        /* Check for over voltage - may be caused by disconnected discharge path */
                        if (low_voltage >= LOGIC_BATTERY_MAX_V_FOR_ST_RAMP)
                        {
                            /* Error state */
                            logic_battery_state = LB_ERROR_ST_RAMPING;
                            
                            /* Disable charging */
                            platform_io_disable_charge_mosfets();
                            timer_delay_ms(1);
                            
                            /* Disable step-down */
                            platform_io_disable_step_down();
                            
                            /* Inform main MCU */
                            comms_main_mcu_send_simple_event(AUX_MCU_EVENT_CHARGE_FAIL);
                            return_value = BAT_ACT_CHARGE_FAIL;
                        }
                        else
                        {
                            /* Increment voltage */
                            platform_io_update_step_down_voltage(logic_battery_charge_voltage);
                            logic_battery_discard_next_adc_measurement_counter = 1;
                        }
                    }
                    
                    /* Increment counter */
                    logic_battery_low_charge_current_counter++;  
                    break;
                }
            
                /* Slower current ramp to reach target charge current */
                case LB_CHARGING_REACH:
                {
                    /* Set voltage difference depending on charging scheme */
                    uint16_t voltage_diff_goal = LOGIC_BATTERY_CUR_FOR_REACH_END_23C;
                    if (logic_battery_charging_type == NIMH_23C_CHARGING)
                    {
                        voltage_diff_goal = LOGIC_BATTERY_CUR_FOR_REACH_END_23C;
                    }
                    else if (logic_battery_charging_type == NIMH_SLOWSTART_23C_CHARGING)
                    {
                        voltage_diff_goal = LOGIC_BATTERY_CUR_FOR_REACH_END_23C;                        
                    }
                    else if (logic_battery_charging_type == NIMH_RECOVERY_23C_CHARGING)
                    {
                        voltage_diff_goal = LOGIC_BATTERY_CUR_FOR_REACH_END_23C;                        
                    }
                    
                    /* Is enough current flowing into the battery? */
                    if ((high_voltage - low_voltage) > voltage_diff_goal)
                    {
                        /* Change state machine */
                        logic_battery_state = LB_CUR_MAINTAIN;
                        logic_battery_nb_secs_since_peak = 0;
                        logic_battery_nb_secs_in_cur_maintain = 0;
                        logic_battery_nb_end_condition_counter = 0;
                        logic_battery_peak_voltage = low_voltage - 5;
                        logic_battery_diag_last_ts = timer_get_systick();
                    }
                    else
                    {
                        /* Increase charge voltage */
                        logic_battery_charge_voltage += LOGIC_BATTERY_BAT_CUR_REACH_V_INC;
                        
                        /* Check for over voltage - may be caused by disconnected discharge path */
                        if (low_voltage >= LOGIC_BATTERY_MAX_V_FOR_CUR_REACH)
                        {
                            /* Error state */
                            logic_battery_state = LB_ERROR_ST_RAMPING;
                            
                            /* Disable charging */
                            platform_io_disable_charge_mosfets();
                            timer_delay_ms(1);
                            
                            /* Disable step-down */
                            platform_io_disable_step_down();
                            
                            /* Inform main MCU */
                            comms_main_mcu_send_simple_event(AUX_MCU_EVENT_CHARGE_FAIL);
                            return_value = BAT_ACT_CHARGE_FAIL;
                        }
                        else
                        {
                            /* Increment voltage */
                            platform_io_update_step_down_voltage(logic_battery_charge_voltage);
                            logic_battery_discard_next_adc_measurement_counter = 1;
                        }
                    }
                    break;
                }
            
                /* Current maintaining */
                case LB_CUR_MAINTAIN:
                {
                    /* Get current calendar */
                    calendar_t current_calendar;
                    timer_get_calendar(&current_calendar);                      
                    
                    /* Update peak vars if required */
                    if (low_voltage > logic_battery_peak_voltage)
                    {
                        logic_battery_nb_end_condition_counter = 0;
                        logic_battery_peak_voltage = low_voltage;
                        logic_battery_nb_secs_since_peak = 0;
                    }
                    else if (logic_battery_last_second_seen != current_calendar.bit.SECOND)
                    {
                        logic_battery_nb_secs_since_peak++;
                    }
                    
                    /* Timing counter */
                    if (logic_battery_last_second_seen != current_calendar.bit.SECOND)
                    {
                        logic_battery_last_second_seen = current_calendar.bit.SECOND;
                        logic_battery_nb_secs_in_cur_maintain++;
                    }  
                    
                    /* Set voltage difference depending on charging scheme */
                    uint32_t voltage_diff_goal = LOGIC_BATTERY_CUR_FOR_REACH_END_23C;
                    if (logic_battery_charging_type == NIMH_23C_CHARGING)
                    {
                        voltage_diff_goal = LOGIC_BATTERY_CUR_FOR_REACH_END_23C;
                    }
                    else if (logic_battery_charging_type == NIMH_SLOWSTART_23C_CHARGING)
                    {
                        voltage_diff_goal = LOGIC_BATTERY_CUR_FOR_REACH_END_23C;                        
                    }
                    else if (logic_battery_charging_type == NIMH_RECOVERY_23C_CHARGING)
                    {
                        voltage_diff_goal = LOGIC_BATTERY_CUR_FOR_REACH_END_23C;                        
                    }
                    
                    /* End of charge detection here */
                    if (((logic_battery_peak_voltage - low_voltage) > LOGIC_BATTERY_END_OF_CHARGE_NEG_V) && (logic_battery_nb_end_condition_counter++ > 30))
                    {
                        /* Abnormal EOC? */
                        if ((logic_battery_nb_abnormal_eoc < LOGIC_BATTERY_MAX_NB_ABN_EOC_RETR) && (logic_battery_nb_secs_in_cur_maintain < LOGIC_BATTERY_ABNORMAL_EOC_SECS) && (logic_battery_charging_type == NIMH_RECOVERY_23C_CHARGING))
                        {
                            /* Update diagnostics */
                            logic_battery_diag_estimat_chg += voltage_diff_goal * ((timer_get_systick() - logic_battery_diag_last_ts) >> 9);

                            /* Start the initial charging procedure */
                            logic_battery_state = LB_CHARGE_START_RAMPING;
                            logic_battery_diag_last_ts = timer_get_systick();
                            logic_battery_discard_next_adc_measurement_counter = 30;
                            logic_battery_charge_voltage = LOGIC_BATTERY_BAT_START_CHG_VOLTAGE;
                            platform_io_update_step_down_voltage(logic_battery_charge_voltage);
                            logic_battery_nb_abnormal_eoc++;
                        } 
                        else
                        {
                            if ((logic_battery_nb_abnormal_eoc == LOGIC_BATTERY_MAX_NB_ABN_EOC_RETR) && (logic_battery_charging_type == NIMH_RECOVERY_23C_CHARGING))
                            {
                                /* Error state */
                                logic_battery_state = LB_ERROR_RECOVERY_CHG;
                                
                                /* Disable charging */
                                platform_io_disable_charge_mosfets();
                                timer_delay_ms(1);
                                
                                /* Disable step-down */
                                platform_io_disable_step_down();
                                
                                /* Inform main MCU */
                                comms_main_mcu_send_simple_event(AUX_MCU_EVENT_CHARGE_FAIL);
                                status_message_sent_to_main_mcu = TRUE;
                                return_value = BAT_ACT_CHARGE_FAIL;
                            } 
                            else
                            {
                                /* Done state */
                                logic_battery_state = LB_CHARGING_DONE;
                                
                                /* Disable charging */
                                platform_io_disable_charge_mosfets();
                                timer_delay_ms(1);
                                
                                /* Disable step-down */
                                platform_io_disable_step_down();

                                /* Update diagnostics */
                                logic_battery_diag_estimat_chg += voltage_diff_goal * ((timer_get_systick() - logic_battery_diag_last_ts) >> 9);
                                
                                /* Inform main MCU */
                                logic_battery_inform_main_of_charge_done(logic_battery_peak_voltage, logic_battery_diag_estimat_chg, logic_battery_nb_abnormal_eoc);
                                status_message_sent_to_main_mcu = TRUE;
                                return_value = BAT_ACT_CHARGE_DONE;
                            }
                        }
                    }
                    else if ((high_voltage - low_voltage) > voltage_diff_goal + 4)
                    {
                        /* Decrease charge voltage */
                        logic_battery_charge_voltage -= LOGIC_BATTERY_BAT_CUR_REACH_V_INC;
                        platform_io_update_step_down_voltage(logic_battery_charge_voltage);
                        logic_battery_discard_next_adc_measurement_counter = 1;
                    }                        
                    else if ((high_voltage - low_voltage) < voltage_diff_goal - 2)
                    {
                        /* Not used anymore to avoid big oscillations on old batteries: Do some math to compute by how much we need to raise the charge voltage */
                        //uint16_t current_to_compensate = voltage_diff_goal - (high_voltage - low_voltage);
                        
                        /* Not used anymore to avoid big oscillations on old batteries: 1LSB is 0.5445mA, R shunt is 1R */
                        //if (logic_battery_charge_voltage < 3000)
                        //{
                        //    logic_battery_charge_voltage += (current_to_compensate * 70) >> 7;
                        //}
                            
                        /* Always increase charging voltage by 1 DAC LSb */
                        logic_battery_charge_voltage += LOGIC_BATTERY_BAT_CUR_REACH_V_INC;
                        
                        /* Check for over voltage - may be caused by disconnected discharge path */
                        if (low_voltage >= LOGIC_BATTERY_MAX_V_FOR_CUR_REACH)
                        {
                            /* Error state */
                            logic_battery_state = LB_ERROR_CUR_MAINTAIN;
                            
                            /* Disable charging */
                            platform_io_disable_charge_mosfets();
                            timer_delay_ms(1);
                            
                            /* Disable step-down */
                            platform_io_disable_step_down();
                            
                            /* Inform main MCU */
                            comms_main_mcu_send_simple_event(AUX_MCU_EVENT_CHARGE_FAIL);
                            status_message_sent_to_main_mcu = TRUE;
                            return_value = BAT_ACT_CHARGE_FAIL;
                        }
                        else
                        {
                            /* Increment voltage */
                            platform_io_update_step_down_voltage(logic_battery_charge_voltage);
                            logic_battery_discard_next_adc_measurement_counter = 3;
                        }
                    }
                    else if (logic_battery_nb_secs_since_peak >= LOGIC_BATTERY_NB_SECS_AFTER_PEAK)
                    {
                        /* Update state */
                        logic_battery_state = LB_PEAK_TIMER_TRIGGERED;

                        /* Update diagnostics */
                        logic_battery_diag_estimat_chg += voltage_diff_goal * ((timer_get_systick() - logic_battery_diag_last_ts) >> 9);
                        
                        /* Disable charging */
                        platform_io_disable_charge_mosfets();
                        timer_delay_ms(1);
                        
                        /* Disable step-down */
                        platform_io_disable_step_down();
                        
                        /* Inform main MCU */
                        logic_battery_inform_main_of_charge_done(logic_battery_peak_voltage, logic_battery_diag_estimat_chg, logic_battery_nb_abnormal_eoc);
                        status_message_sent_to_main_mcu = TRUE;
                        return_value = BAT_ACT_CHARGE_DONE;
                    }

                    /* Check for new battery level */
                    for (uint16_t i = 0; i < ARRAY_SIZE(logic_battery_battery_level_mapping); i++)
                    {
                        if (low_voltage > logic_battery_battery_level_mapping[i])
                        {
                            possible_new_battery_level = i;
                        }
                    }
                    break;
                }
            
                default: break;
            }
        }        
            
        /* Trigger new conversion if not specified otherwise */
        if (logic_battery_stop_using_adc_flag == FALSE)
        {
            platform_io_get_cursense_conversion_result(TRUE);
        }
        
        /* Leave outside the switch to allow fast actions on current: check if we need to send a notification to the main */
        if ((logic_battery_state == LB_CUR_MAINTAIN) && (possible_new_battery_level > logic_battery_current_battery_level) && (status_message_sent_to_main_mcu == FALSE))
        {
            aux_mcu_message_t* temp_tx_message_pt;

            /* Update main MCU with new battery level */
            comms_main_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT);
            temp_tx_message_pt->aux_mcu_event_message.event_id = AUX_MCU_EVENT_CHARGE_LVL_UPDATE;
            temp_tx_message_pt->aux_mcu_event_message.payload[0] = possible_new_battery_level;
            temp_tx_message_pt->payload_length1 = sizeof(temp_tx_message_pt->aux_mcu_event_message.event_id) + sizeof(uint8_t);
            comms_main_mcu_send_message((void*)temp_tx_message_pt, (uint16_t)sizeof(aux_mcu_message_t));

            /* Store new level */
            logic_battery_current_battery_level = possible_new_battery_level;
            return_value = BAT_ACT_NEW_BAT_LEVEL;
        }
        
        /* Clear flags & decrease counters */
        if (logic_battery_discard_next_adc_measurement_counter != 0)
        {
            logic_battery_discard_next_adc_measurement_counter--;
        }
    }
    
    logic_battery_start_using_adc_flag = FALSE;
    logic_battery_stop_using_adc_flag = FALSE;
    return return_value;
}

/*! \fn     logic_battery_inform_main_of_charge_done(uint16_t peak_voltage_level, uint32_t estimated_charge, uint16_t nb_abnormal_eoc)
*   \brief  Inform main MCU of charge done
*   \param  peak_voltage_level The ADC value for peak voltage level
*   \param  estimated_charge   Estimated charge that went into the battery
*   \param  nb_abnormal_eoc    Number of abnormal EoC events
*/
void logic_battery_inform_main_of_charge_done(uint16_t peak_voltage_level, uint32_t estimated_charge, uint16_t nb_abnormal_eoc)
{
    aux_mcu_message_t* temp_tx_message_pt;
    comms_main_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT);
    temp_tx_message_pt->aux_mcu_event_message.event_id = AUX_MCU_EVENT_CHARGE_DONE;
    temp_tx_message_pt->aux_mcu_event_message.payload_as_uint16[0] = peak_voltage_level;
    temp_tx_message_pt->aux_mcu_event_message.payload_as_uint16[1] = nb_abnormal_eoc;
    temp_tx_message_pt->aux_mcu_event_message.payload_as_uint16[2] = (uint16_t)estimated_charge;
    temp_tx_message_pt->aux_mcu_event_message.payload_as_uint16[3] = (uint16_t)(estimated_charge >> 16);
    temp_tx_message_pt->payload_length1 = sizeof(temp_tx_message_pt->aux_mcu_event_message.event_id) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t);
    comms_main_mcu_send_message((void*)temp_tx_message_pt, (uint16_t)sizeof(aux_mcu_message_t));
}
