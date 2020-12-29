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
/*!  \file     logic_power.c
*    \brief    Power logic
*    Created:  29/09/2018
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include "logic_accelerometer.h"
#include "smartcard_lowlevel.h"
#include "logic_bluetooth.h"
#include "logic_smartcard.h"
#include "gui_dispatcher.h"
#include "logic_security.h"
#include "logic_bluetooth.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "logic_device.h"
#include "logic_power.h"
#include "platform_io.h"
#include "gui_prompts.h"
#include "logic_power.h"
#include "logic_user.h"
#include "custom_fs.h"
#include "text_ids.h"
#include "sh1122.h"
#include "inputs.h"
#include "utils.h"
#include "main.h"
/* Array mapping battery levels with adc readouts */
uint16_t logic_power_battery_level_mapping[11] = {BATTERY_ADC_OUT_CUTOUT, BATTERY_ADC_10PCT_VOLTAGE, BATTERY_ADC_20PCT_VOLTAGE, BATTERY_ADC_30PCT_VOLTAGE, BATTERY_ADC_40PCT_VOLTAGE, BATTERY_ADC_50PCT_VOLTAGE, BATTERY_ADC_60PCT_VOLTAGE, BATTERY_ADC_70PCT_VOLTAGE, BATTERY_ADC_80PCT_VOLTAGE, BATTERY_ADC_90PCT_VOLTAGE, BATTERY_ADC_100PCT_VOLTAGE};
/* Current battery level in percent tenth */
uint16_t logic_power_current_battery_level = 11;
uint16_t logic_power_battery_level_to_be_acked = 0;
/* Battery level update from aux, if different than 0 */
uint16_t logic_power_aux_mcu_battery_level_update = 0;
/* Last seen voled stepup power source */
oled_stepup_pwr_source_te logic_power_last_seen_voled_stepup_pwr_source = OLED_STEPUP_SOURCE_NONE;
/* Current power source */
power_source_te logic_power_current_power_source;
/* Last Vbat measured ADC values */
uint16_t logic_power_last_vbat_measurements[LAST_VOLTAGE_CONV_BUFF_SIZE];
/* Battery charging bool */
nimh_charge_te logic_power_current_charge_type;
BOOL logic_power_battery_charging = FALSE;
/* Error with battery flag */
BOOL logic_power_error_with_battery = FALSE;
/* If the "enumerate usb" command was just sent */
BOOL logic_power_enumerate_usb_command_just_sent = FALSE;
/* Number of measurements left to discard */
uint16_t logic_power_discard_measurement_counter = 0;
/* Number of times we still can skip the queue logic when taking into account an adc measurement */
uint16_t logic_power_nb_times_to_skip_adc_measurement_queue = 0;
/* ADC measurement counter */
uint16_t logic_power_adc_measurement_counter = 0;
/* Over discharge boolean */
BOOL logic_power_over_discharge_flag = FALSE;
/* Power consumption log */
volatile power_consumption_log_t logic_power_consumption_log;


/*! \fn     logic_power_ms_tick(void)
*   \brief  Function called every ms by interrupt
*   \note   This function won't be called when the device is sleeping
*/
void logic_power_ms_tick(void)
{
    /* We're only interested when we're battery powered */
    if (logic_power_current_power_source == BATTERY_POWERED)
    {
        if (platform_io_get_voled_stepup_pwr_source() == OLED_STEPUP_SOURCE_NONE)
        {
            /* We're awake with the screen off: the AUX woke us up (or we're going to sleep) */
            logic_power_consumption_log.nb_ms_no_screen_aux_main_awake++;
        }
        else
        {
            /* Check if comms with aux are disabled, as it means we're in the periodic wakeup */
            if (comms_aux_mcu_are_comms_disabled() == FALSE)
            {
                logic_power_consumption_log.nb_ms_full_pawa++;
            }
            else
            {
                logic_power_consumption_log.nb_ms_no_screen_main_awake++;
            }
        }
    }
    
    /* Increment nb ms since last full charge counter */
    if ((logic_power_consumption_log.nb_ms_spent_since_last_full_charge != UINT32_MAX) && (logic_power_current_power_source == BATTERY_POWERED))
    {
        logic_power_consumption_log.nb_ms_spent_since_last_full_charge++;
    }
    
    /* Lifetime statistics: first boot */
    if ((logic_power_consumption_log.lifetime_nb_ms_screen_on_msb == UINT32_MAX) && (logic_power_consumption_log.lifetime_nb_ms_screen_on_lsb == UINT32_MAX))
    {
        logic_power_consumption_log.lifetime_nb_ms_screen_on_msb = 0;
        logic_power_consumption_log.lifetime_nb_ms_screen_on_lsb = 0;
    }
    
    /* Lifetime statistics: nb ms screen on */
    if (sh1122_is_oled_on(&plat_oled_descriptor) != FALSE)
    {
        if (logic_power_consumption_log.lifetime_nb_ms_screen_on_lsb++ == UINT32_MAX)
        {
            logic_power_consumption_log.lifetime_nb_ms_screen_on_msb++;
        }
    }
}

/*! \fn     logic_power_30m_tick(void)
*   \brief  Function called every 30mins by interrupt, even when the device sleeps
*   \note   As mentioned, this function doesn't lose track of time
*/
void logic_power_30m_tick(void)
{
    /* This routine is only interested in the low power background power consumption */
    if (logic_power_current_power_source == BATTERY_POWERED)
    {
        /* Lifetime statistics: first boot & timer increment */
        if (logic_power_consumption_log.lifetime_nb_30mins_bat == UINT32_MAX)
        {
            logic_power_consumption_log.lifetime_nb_30mins_bat = 0;
        }
        else
        {
            logic_power_consumption_log.lifetime_nb_30mins_bat++;
        }
        
        /* Lowest power consumption timer (no BLE, no card) */
        logic_power_consumption_log.nb_30mins_powered_on++;
        
        /* Card inserted penalty */
        if (smartcard_low_level_is_smc_absent() != RETURN_OK)
        {
            logic_power_consumption_log.nb_30mins_card_inserted++;
        }
        
        /* Get bluetooth state */
        bt_state_te current_bt_state = logic_bluetooth_get_state();
        
        /* Bluetooth enabled? */
        if (current_bt_state == BT_STATE_ON)
        {
            /* Advertising */
            logic_power_consumption_log.nb_30mins_ble_advertising++;
        }
        else if (current_bt_state == BT_STATE_CONNECTED)
        {
            /* Connected: difference power consumptions based on connected to platform */
            platform_type_te plat_type = logic_bluetooth_get_connected_to_platform_type();
            switch (plat_type)
            {
                case PLAT_IOS: logic_power_consumption_log.nb_30mins_ios_connect++; break;
                case PLAT_MACOS: logic_power_consumption_log.nb_30mins_macos_connect++; break;
                case PLAT_WIN: logic_power_consumption_log.nb_30mins_windows_connect++; break;
                case PLAT_ANDROID: logic_power_consumption_log.nb_30mins_android_connect++; break;
                default: break;
            }
        }
    } 
    else
    {
        /* Lifetime statistics: first boot & timer increment */
        if (logic_power_consumption_log.lifetime_nb_30mins_usb == UINT32_MAX)
        {
            logic_power_consumption_log.lifetime_nb_30mins_usb = 0;
        }
        else
        {
            logic_power_consumption_log.lifetime_nb_30mins_usb++;
        }
    }   
}

/*! \fn     logic_power_debug_get_last_adc_measurement(void)
*   \brief  Debug function: get last adc measurement
*   \return The raw ADC value for last measurement
*/
uint16_t logic_power_debug_get_last_adc_measurement(void)
{
    return logic_power_last_vbat_measurements[LAST_VOLTAGE_CONV_BUFF_SIZE-1];
}

/*! \fn     logic_power_get_current_charge_type(void)
*   \brief  Get current charge type
*   \return The charge type (see enum)
*/
nimh_charge_te logic_power_get_current_charge_type(void)
{
    return logic_power_current_charge_type;
}

/*! \fn     logic_power_get_power_consumption_log_pt(void)
*   \brief  Get a pointer to the current power consumption log
*   \return The power consumption log pointer
*/
power_consumption_log_t* logic_power_get_power_consumption_log_pt(void)
{
    return (power_consumption_log_t*)&logic_power_consumption_log;
}

/*! \fn     logic_power_set_power_source(power_source_te power_source)
*   \brief  Set current power source
*   \param  power_source    Power source (see enum)
*/
void logic_power_set_power_source(power_source_te power_source)
{
    logic_power_current_power_source = power_source;
}

/*! \fn     logic_power_skip_queue_logic_for_upcoming_adc_measurement(void)
*   \brief  Signal our internal logic to skip the queue logic for the upcoming measurement
*   \note   This is used when the main MCU is sleeping and only waking up every 20 minutes
*/
void logic_power_skip_queue_logic_for_upcoming_adc_measurements(void)
{
    logic_power_nb_times_to_skip_adc_measurement_queue = 3;
    logic_power_discard_measurement_counter = 1;
}

/*! \fn     logic_power_get_power_source(void)
*   \brief  Get current power source
*   \return Current power source (see enum)
*/
power_source_te logic_power_get_power_source(void)
{
    return logic_power_current_power_source;
}

/*! \fn     logic_power_init(BOOL poweredoff_due_to_battery)
*   \brief  Init power logic
*   \param  poweredoff_due_to_battery   TRUE if the device was previously switched off due to low battery
*/
void logic_power_init(BOOL poweredoff_due_to_battery)
{
    /* Recall power consumption log from flash */
    cpu_irq_enter_critical();
    custom_fs_get_power_consumption_log((power_consumption_log_t*)&logic_power_consumption_log);
    cpu_irq_leave_critical();

    /* If powered off due to battery, set initial measurement array to different values */
    if (poweredoff_due_to_battery != FALSE)
    {
        logic_power_inform_of_over_discharge();
        logic_power_register_vbat_adc_measurement(0);
    }
    else
    {
        /* First boot? */
        if (logic_power_consumption_log.nb_30mins_powered_on == UINT32_MAX)
        {
            logic_power_register_vbat_adc_measurement(0);
        } 
        else
        {
            logic_power_register_vbat_adc_measurement(UINT16_MAX);
        }
    }
    
    /* Skip next measurements for battery voltage to settle */
    logic_power_discard_measurement_counter = 5;
    
    /* Compute battery state */
    logic_power_compute_battery_state();
}

/*! \fn     logic_power_power_down_actions(void)
*   \brief  Actions to be taken before powering down
*/
void logic_power_power_down_actions(void)
{
    volatile power_consumption_log_t logic_power_consumption_log_copy;
    memcpy((void*)&logic_power_consumption_log_copy, (void*)&logic_power_consumption_log, sizeof(logic_power_consumption_log_copy));
    custom_fs_store_power_consumption_log((power_consumption_log_t*)&logic_power_consumption_log_copy);
}

/*! \fn     logic_power_get_and_ack_new_battery_level(void)
*   \brief  Acknowledge and get the new battery level
*   \return The new battery level in percent tenth
*/
uint16_t logic_power_get_and_ack_new_battery_level(void)
{
    logic_power_current_battery_level = logic_power_battery_level_to_be_acked;
    return logic_power_current_battery_level;
}

/*! \fn     logic_power_get_battery_level(void)
*   \brief  Get current battery level
*   \return The battery level in percent tenth
*/
uint16_t logic_power_get_battery_level(void)
{
    /* ADC logic didn't have time to kick in */
    if (logic_power_current_battery_level > 10)
    {
        return 10;
    }
    else
    {
        return logic_power_current_battery_level;
    }
}

/*! \fn     logic_power_set_battery_level_update_from_aux(uint8_t battery_level)
*   \brief  Called when the aux MCU reports a new battery level
*   \param  battery_level   The new battery level in percent tenth
*/
void logic_power_set_battery_level_update_from_aux(uint8_t battery_level)
{
    /* Set log value to max to force new readings */
    logic_power_register_vbat_adc_measurement(UINT16_MAX);
    
    /* Reset power consumption logs */
    cpu_irq_enter_critical();
    uint32_t nb_ms_since_full_charge_copy = logic_power_consumption_log.nb_ms_spent_since_last_full_charge;
    memset((void*)&logic_power_consumption_log, 0x00, sizeof(logic_power_consumption_log));
    logic_power_consumption_log.nb_ms_spent_since_last_full_charge = nb_ms_since_full_charge_copy;
    logic_power_consumption_log.aux_mcu_reported_pct = battery_level * 10;
    cpu_irq_leave_critical();
    
    /* Store level update */
    logic_power_aux_mcu_battery_level_update = battery_level;
}

/*! \fn     logic_power_set_battery_charging_bool(BOOL battery_charging, BOOL charge_success)
*   \brief  Set battery charging bool
*   \param  battery_charging    The boolean
*   \param  charge_success      If previous bool is FALSE, inform of charge success
*/
void logic_power_set_battery_charging_bool(BOOL battery_charging, BOOL charge_success)
{
    logic_power_battery_charging = battery_charging;
    
    /* In case of charge success, reset our counter */
    if ((battery_charging == FALSE) && (charge_success != FALSE))
    {
        cpu_irq_enter_critical();
        logic_power_consumption_log.nb_ms_spent_since_last_full_charge = 0;
        cpu_irq_leave_critical();
    }
}

/*! \fn     logic_power_is_battery_charging(void)
*   \brief  Get battery charging bool
*   \return The boolean
*/
BOOL logic_power_is_battery_charging(void)
{
    return logic_power_battery_charging;
}

/*! \fn     logic_power_signal_battery_error(void)
*   \brief  Signal an error with the battery
*/
void logic_power_signal_battery_error(void)
{
    logic_power_error_with_battery = TRUE;
}

/*! \fn     logic_power_get_battery_state(void)
*   \brief  Get current battery state
*   \return Current battery state (see enum)
*/
battery_state_te logic_power_get_battery_state(void)
{
    if (logic_power_error_with_battery == FALSE)
    {
        if (logic_power_current_battery_level < 2)
        {
            return BATTERY_0PCT;
        }
        else if (logic_power_current_battery_level < 4)
        {
            return BATTERY_25PCT;
        }
        else if (logic_power_current_battery_level < 6)
        {
            return BATTERY_50PCT;
        }
        else if (logic_power_current_battery_level < 8)
        {
            return BATTERY_75PCT;
        }
        else
        {
            return BATTERY_100PCT;
        }
    } 
    else
    {
        return BATTERY_ERROR;
    }
}

/*! \fn     logic_power_inform_of_over_discharge(void)
*   \brief  Inform the power logic that the battery is in an over discharge state
*/
void logic_power_inform_of_over_discharge(void)
{
    logic_power_over_discharge_flag = TRUE;
}

/*! \fn     logic_power_get_and_reset_over_discharge_flag(void)
*   \brief  Get and reset over discharge flag
*/
BOOL logic_power_get_and_reset_over_discharge_flag(void)
{
    BOOL return_value = logic_power_over_discharge_flag;
    logic_power_over_discharge_flag = FALSE;
    return return_value;
}

/*! \fn     logic_power_register_vbat_adc_measurement(uint16_t adc_val)
*   \brief  Register an ADC measurement of the battery voltage
*   \param  adc_val The ADC value
*/
void logic_power_register_vbat_adc_measurement(uint16_t adc_val)
{
    utils_fill_uint16_array_with_value(logic_power_last_vbat_measurements, ARRAY_SIZE(logic_power_last_vbat_measurements), adc_val);
}

/*! \fn     logic_power_usb_enumerate_just_sent(void)
*   \brief  Signal that the USB enumerate command was sent
*/
void logic_power_usb_enumerate_just_sent(void)
{
    logic_power_enumerate_usb_command_just_sent = TRUE;
}

/*! \fn     logic_power_is_usb_enumerate_sent_clear_bool(void)
*   \brief  Know if the USB enumerate command was just sent and clear the bool
*/
BOOL logic_power_is_usb_enumerate_sent_clear_bool(void)
{
    BOOL return_bool = logic_power_enumerate_usb_command_just_sent;
    logic_power_enumerate_usb_command_just_sent = FALSE;
    return return_bool;
}

/*! \fn     logic_power_compute_battery_state(void)
*   \brief  Compute battery state, if we can
*/
void logic_power_compute_battery_state(void)
{
    uint16_t possible_new_battery_level = 0;
    BOOL adc_measurement_fallback = TRUE;
    
    if ((logic_power_last_vbat_measurements[0] > BATTERY_ADC_FOR_BATTERY_STATUS_READ_STRAT_SWITCH) && (logic_power_consumption_log.aux_mcu_reported_pct <= 100))
    {
        /* Battery voltage high enough so we can try to use our power consumption log */
        volatile uint32_t nb_uah_used_total = 0;
        nb_uah_used_total += (logic_power_consumption_log.nb_30mins_powered_on * 96 / 2);
        nb_uah_used_total += (logic_power_consumption_log.nb_30mins_card_inserted * 42 / 2);
        nb_uah_used_total += (logic_power_consumption_log.nb_30mins_ble_advertising * 623 / 2);
        nb_uah_used_total += (logic_power_consumption_log.nb_30mins_ios_connect * 0 / 2);
        nb_uah_used_total += (logic_power_consumption_log.nb_30mins_macos_connect * 0 / 2);
        nb_uah_used_total += (logic_power_consumption_log.nb_30mins_android_connect * 74 / 2);
        nb_uah_used_total += (logic_power_consumption_log.nb_30mins_windows_connect * 2360 / 2);
        
        /* Below each number is divided by 60*60*1000=3600000 then multiplied by the standard power consumption in mA */
        /* FYI for a maximum of 12 hours (impossible, but who knows?) maximum allowed multiplication is 99 for an uint32_t */
        nb_uah_used_total += ((logic_power_consumption_log.nb_ms_no_screen_aux_main_awake * 57) >> 11); // 100mA
        nb_uah_used_total += ((logic_power_consumption_log.nb_ms_no_screen_main_awake * 57) >> 12);     // 50mA
        nb_uah_used_total += ((logic_power_consumption_log.nb_ms_full_pawa * 91) >> 11);                // 160mA
        
        /* Number of available uAh */
        uint32_t battery_last_reported_uah = logic_power_consumption_log.aux_mcu_reported_pct * 3000;
        
        /* Are we at least getting a positive result? */
        if (nb_uah_used_total < battery_last_reported_uah)
        {
            /* Number of pct left */
            uint32_t nb_tenth_pct_left = (battery_last_reported_uah - nb_uah_used_total)/3000/10;
            
            /* Not too low, below previous result? */
            if ((nb_tenth_pct_left >= 2) && (nb_tenth_pct_left < logic_power_current_battery_level))
            {
                logic_power_battery_level_to_be_acked = nb_tenth_pct_left;
                adc_measurement_fallback = FALSE;
            }
        }
    }
    
    if (adc_measurement_fallback != FALSE)
    {
        /* Use battery readings */
        for (uint16_t i = 0; i < ARRAY_SIZE(logic_power_battery_level_mapping); i++)
        {
            if (logic_power_last_vbat_measurements[0] > logic_power_battery_level_mapping[i])
            {
                possible_new_battery_level = i;
            }
        }
        
        /* Only allow decrease in voltage except at device boot */
        if ((possible_new_battery_level < logic_power_current_battery_level) || (logic_power_current_battery_level > 10))
        {
            logic_power_battery_level_to_be_acked = possible_new_battery_level;
        }
    }
    
    /* In case of invalid power level, force overwrite */
    if (logic_power_current_battery_level > 10)
    {
        logic_power_current_battery_level = possible_new_battery_level;
    }
}

/*! \fn     logic_power_check_power_switch_and_battery(BOOL wait_for_adc_conversion_and_dont_start_another)
*   \brief  Power handling routine
*   \param  wait_for_adc_conversion_and_dont_start_another  Set to TRUE to do what it says
*   \return An action if needed (see enum)
*/
power_action_te logic_power_check_power_switch_and_battery(BOOL wait_for_adc_conversion_and_dont_start_another)
{            
    /* Get current oled power source */
    oled_stepup_pwr_source_te current_voled_pwr_source = platform_io_get_voled_stepup_pwr_source();
    
    /* Power supply change */
    if ((logic_power_get_power_source() == BATTERY_POWERED) && (platform_io_is_usb_3v3_present() != FALSE))
    {
        /* Set inversion bool */
        plat_oled_descriptor.screen_inverted = (BOOL)custom_fs_settings_get_device_setting(SETTINGS_LEFT_HANDED_ON_USB);
        inputs_set_inputs_invert_bool(plat_oled_descriptor.screen_inverted);
        
        /* Overwrite all last battery measurements with the one from a while back in order to discard potential perturbation from ADC plugging */
        utils_fill_uint16_array_with_value(logic_power_last_vbat_measurements, ARRAY_SIZE(logic_power_last_vbat_measurements), logic_power_last_vbat_measurements[0]);
        
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_check_for_flush_and_terminate(&plat_oled_descriptor);
        #endif
        sh1122_oled_off(&plat_oled_descriptor);
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_ATTACH_USB);
        comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_ATTACH_CMD_RCVD);
        platform_io_disable_vbat_to_oled_stepup();
        logic_power_set_power_source(USB_POWERED);
        logic_power_usb_enumerate_just_sent();
        platform_io_assert_oled_reset();
        timer_delay_ms(15);
        platform_io_power_up_oled(TRUE);
        sh1122_init_display(&plat_oled_descriptor, TRUE);
        logic_device_activity_detected();
    }
    else if ((logic_power_get_power_source() == USB_POWERED) && (platform_io_is_usb_3v3_present() == FALSE))
    {        
        /* Switch off device if needed */
        if (((BOOL)custom_fs_settings_get_device_setting(SETTINGS_SWITCH_OFF_AFTER_USB_DISC) != FALSE) && (logic_bluetooth_get_state() != BT_STATE_CONNECTED))
        {
            logic_device_power_off();
        }
        
        /* Set inversion bool */
        plat_oled_descriptor.screen_inverted = (BOOL)custom_fs_settings_get_device_setting(SETTINGS_LEFT_HANDED_ON_BATTERY);
        inputs_set_inputs_invert_bool(plat_oled_descriptor.screen_inverted);
        
        /* Tell the aux MCU to detach USB, change power supply for OLED */
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_check_for_flush_and_terminate(&plat_oled_descriptor);
        #endif
        sh1122_oled_off(&plat_oled_descriptor);
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_DETACH_USB);
        comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_USB_DETACHED);
        platform_io_disable_3v3_to_oled_stepup();
        logic_power_set_power_source(BATTERY_POWERED);
        logic_power_set_battery_charging_bool(FALSE, FALSE);
        logic_user_reset_computer_locked_state(TRUE);
        logic_aux_mcu_set_usb_enumerated_bool(FALSE);
        platform_io_assert_oled_reset();
        timer_delay_ms(15);
        platform_io_power_up_oled(FALSE);
        sh1122_init_display(&plat_oled_descriptor, TRUE);
        logic_device_activity_detected();
        
        /* Discard next 10 measurement */
        logic_power_discard_measurement_counter = 10;
        logic_power_adc_measurement_counter = 0;
                
        /* If user selected to, lock device */
        if ((logic_security_is_smc_inserted_unlocked() != FALSE) && (logic_bluetooth_get_state() != BT_STATE_CONNECTED) && ((BOOL)custom_fs_settings_get_device_setting(SETTINGS_LOCK_ON_DISCONNECT) != FALSE))
        {
            logic_user_set_user_to_be_logged_off_flag();
        }
        
        /* Clear inputs in case user move the wheel not on purpose */
        inputs_clear_detections();
    }
    else if ((logic_power_last_seen_voled_stepup_pwr_source == OLED_STEPUP_SOURCE_NONE) && (current_voled_pwr_source != logic_power_last_seen_voled_stepup_pwr_source))
    {
        /* If we have been told to bypass queue logic it means we want a quick measurement */
        if (logic_power_nb_times_to_skip_adc_measurement_queue == 0)
        {
            /* Device power-on or wakeup */
            logic_power_discard_measurement_counter = 5;
            logic_power_adc_measurement_counter = 0;
        }
    }
    
    /* Store last seen voled power source */
    logic_power_last_seen_voled_stepup_pwr_source = current_voled_pwr_source;
    
    /* Battery charging start logic */
    if ((logic_power_get_power_source() == USB_POWERED) && (logic_power_is_usb_enumerate_sent_clear_bool() != FALSE) && 
        (logic_power_battery_charging == FALSE) && 
        (logic_power_get_battery_state() <= BATTERY_75PCT))
    {        
        /* Send message to start charging */
        if (logic_power_get_and_reset_over_discharge_flag() != FALSE)
        {
            comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_NIMH_RECOVERY_CHG);
            comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_CHARGE_STARTED);
            logic_power_current_charge_type = RECOVERY_CHARGE;
        }
        else if (logic_power_get_battery_state() <= BATTERY_25PCT)
        {
            comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_NIMH_CHG_SLW_STRT);
            comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_CHARGE_STARTED);   
            logic_power_current_charge_type = SLOW_START_CHARGE;
        } 
        else
        {
            comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_NIMH_CHARGE);
            comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_CHARGE_STARTED);
            logic_power_current_charge_type = NORMAL_CHARGE;
        }
        
        /* Set boolean */
        logic_power_battery_charging = TRUE;
    }
    
    /* Wait if we've been instructed to */
    if (wait_for_adc_conversion_and_dont_start_another != FALSE)
    {
        while(platform_io_is_voledin_conversion_result_ready() == FALSE);
    }
    
    /* Battery measurement */
    if (platform_io_is_voledin_conversion_result_ready() != FALSE)
    {
        uint16_t current_vbat;
        if (wait_for_adc_conversion_and_dont_start_another == FALSE)
        {
            current_vbat = platform_io_get_voledin_conversion_result_and_trigger_conversion();
        } 
        else
        {
            current_vbat = platform_io_get_voledin_conversion_result();
        }
        
        /* Take one measurement every 8, or not if we have been told to skip queue logic */
        BOOL should_deal_with_measurement = FALSE;
        logic_power_adc_measurement_counter++;
        if (((logic_power_adc_measurement_counter & 0x0007) == 0) || (logic_power_nb_times_to_skip_adc_measurement_queue != 0))
        {
            should_deal_with_measurement = TRUE;
        }
        
        /* Store current vbat only if we are battery powered, if it has been a while since we changed power sources, and if we haven't been instructed to skip next measurement */
        if ((platform_io_get_voled_stepup_pwr_source() == OLED_STEPUP_SOURCE_VBAT) && (logic_power_discard_measurement_counter == 0) && (should_deal_with_measurement != FALSE))
        {
            /* Should we skip the queue logic? */
            if (logic_power_nb_times_to_skip_adc_measurement_queue != 0)
            {
                utils_fill_uint16_array_with_value(logic_power_last_vbat_measurements, ARRAY_SIZE(logic_power_last_vbat_measurements), current_vbat);
                logic_power_nb_times_to_skip_adc_measurement_queue--;
            } 
            else
            {
                /* Shift our last measured vbat values array */
                for (uint16_t i = 0; i < ARRAY_SIZE(logic_power_last_vbat_measurements)-1; i++)
                {
                    logic_power_last_vbat_measurements[i] = logic_power_last_vbat_measurements[i+1];
                }
                
                /* Store the latest measurement at the end of this buffer */
                logic_power_last_vbat_measurements[ARRAY_SIZE(logic_power_last_vbat_measurements)-1] = current_vbat;
            }
            
            /* Logic deals with with the vbat measured a while back in order to prevent a non registered USB plug to perturbate ADC measurements */
            uint16_t vbat_measurement_from_a_bit_ago = logic_power_last_vbat_measurements[0];
            
            /* Low battery, need to power off? */
            if ((logic_power_get_power_source() == BATTERY_POWERED) && (vbat_measurement_from_a_bit_ago < BATTERY_ADC_OUT_CUTOUT) && (platform_io_is_usb_3v3_present_raw() == FALSE))
            {
                /* platform_io_is_usb_3v3_present_raw() call is here to prevent erroneous measurements */
                return POWER_ACT_POWER_OFF;
            }
            
            /* Emergency battery cutoff */
            if ((logic_power_get_power_source() == BATTERY_POWERED) && (vbat_measurement_from_a_bit_ago < BATTERY_ADC_EMGCY_CUTOUT) && (platform_io_is_usb_3v3_present_raw() == FALSE))
            {
                custom_fs_set_device_flag_value(PWR_OFF_DUE_TO_BATTERY_FLG_ID, TRUE);
                logic_device_power_off();
            }
            
            /* Check for new battery level */
            logic_power_compute_battery_state();
        }
        
        /* Counter decrement */
        if (logic_power_discard_measurement_counter != 0)
        {
            logic_power_discard_measurement_counter--;
        }
    }

    /* New battery level reported by aux MCU */
    if (logic_power_aux_mcu_battery_level_update != 0)
    {            
        /* Set new level to be acked */
        logic_power_battery_level_to_be_acked = logic_power_aux_mcu_battery_level_update;
        logic_power_aux_mcu_battery_level_update = 0;
    }
    
    /* If new level to be acked, return right value */
    if (logic_power_battery_level_to_be_acked != logic_power_current_battery_level)
    {
        return POWER_ACT_NEW_BAT_LEVEL;
    }
    else
    {
        return POWER_ACT_NONE;
    }
}

/*! \fn     logic_power_routine(void)
*   \brief  Power routine, mostly takes care of switching off device if battery too low
*/
void logic_power_routine(void)
{
    /* Power handling routine */
    power_action_te power_action = logic_power_check_power_switch_and_battery(FALSE);
    
    /* If the power routine tells us to power off, provided we are not updating */
    if ((power_action == POWER_ACT_POWER_OFF) && (gui_dispatcher_get_current_screen() != GUI_SCREEN_FW_FILE_UPDATE))
    {
        /* Set flag */
        custom_fs_set_device_flag_value(PWR_OFF_DUE_TO_BATTERY_FLG_ID, TRUE);
        logic_power_power_down_actions();
        
        /* Out of battery! */
        gui_prompts_display_information_on_screen_and_wait(BATTERY_EMPTY_TEXT_ID, DISP_MSG_WARNING, FALSE);
        sh1122_oled_off(&plat_oled_descriptor);
        platform_io_power_down_oled();
        timer_delay_ms(100);
        
        /* It may not be impossible that the user connected the device in the meantime */
        if (platform_io_is_usb_3v3_present() == FALSE)
        {
            platform_io_disable_switch_and_die();
            while(1);
        }
        else
        {
            /* Inform logic of over discharge */
            logic_power_inform_of_over_discharge();
            
            /* Call the power routine that will take care of power switch */
            logic_power_check_power_switch_and_battery(FALSE);
            gui_dispatcher_get_back_to_current_screen();
        }
    }
    else if (power_action == POWER_ACT_NEW_BAT_LEVEL)
    {
        /* New battery level, inform aux MCU */
        logic_aux_mcu_update_aux_mcu_of_new_battery_level(logic_power_get_and_ack_new_battery_level()*10);
        
        /* Update device state */
        logic_device_set_state_changed();
    }    
}

/*! \fn     logic_power_battery_recondition(void)
*   \brief  Fully discharge and charge the battery
*   \return Success status
*/
RET_TYPE logic_power_battery_recondition(void)
{
    uint16_t gui_dispatcher_current_idle_anim_frame_id = 0;
    uint16_t temp_uint16;

    /* Needs to be battery powered */
    if (platform_io_is_usb_3v3_present_raw() == FALSE)
    {
        return RETURN_NOK;
    }
    
    /* Check for charging */
    if (logic_power_is_battery_charging() != FALSE)
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_STOP_CHARGE);
        comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_CHARGE_STOPPED);
        logic_power_set_battery_charging_bool(FALSE, FALSE);
    }
    
    /* Switch to battery power for screen */
    sh1122_oled_off(&plat_oled_descriptor);
    platform_io_disable_3v3_to_oled_stepup();
    platform_io_assert_oled_reset();
    timer_delay_ms(15);
    platform_io_power_up_oled(FALSE);
    sh1122_init_display(&plat_oled_descriptor, TRUE);
    
    /* Display animation until 1V at the cell */
    uint16_t current_vbat = UINT16_MAX;
    gui_prompts_display_information_on_screen(RECOND_DISCHARGE_TEXT_ID, DISP_MSG_INFO);
    while (current_vbat > BATTERY_1V_WHEN_MEASURED_FROM_MAIN_WITH_USB)
    {        
        /* Idle animation */
        if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
        {
            /* Display new animation frame bitmap, rearm timer with provided value */
            gui_prompts_display_information_on_string_single_anim_frame(&gui_dispatcher_current_idle_anim_frame_id, &temp_uint16, DISP_MSG_INFO);
            timer_start_timer(TIMER_ANIMATIONS, temp_uint16);
        }

        /* Do not deal with incoming messages */
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        logic_accelerometer_routine();

        /* User disconnected USB? */
        if (platform_io_is_usb_3v3_present_raw() == FALSE)
        {
            gui_dispatcher_get_back_to_current_screen();
            return RETURN_NOK;
        }
        
        /* ADC value ready? */
        if (platform_io_is_voledin_conversion_result_ready() != FALSE)
        {
            current_vbat = platform_io_get_voledin_conversion_result_and_trigger_conversion();
        }
    }
    
    /* Switch back to USB power for screen */
    sh1122_oled_off(&plat_oled_descriptor);
    platform_io_disable_vbat_to_oled_stepup();
    platform_io_assert_oled_reset();
    timer_delay_ms(15);
    platform_io_power_up_oled(TRUE);
    sh1122_init_display(&plat_oled_descriptor, TRUE);

    /* Battery rest */
    uint16_t temp_timer_id = timer_get_and_start_timer(10*60*1000);
    gui_prompts_display_information_on_screen(RECOND_REST_TEXT_ID, DISP_MSG_INFO);
    while (timer_has_allocated_timer_expired(temp_timer_id, TRUE) != TIMER_EXPIRED)
    {
        /* Idle animation */
        if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
        {
            /* Display new animation frame bitmap, rearm timer with provided value */
            gui_prompts_display_information_on_string_single_anim_frame(&gui_dispatcher_current_idle_anim_frame_id, &temp_uint16, DISP_MSG_INFO);
            timer_start_timer(TIMER_ANIMATIONS, temp_uint16);
        }

        /* Do not deal with incoming messages */
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        logic_accelerometer_routine();

        /* User disconnected USB? */
        if (platform_io_is_usb_3v3_present_raw() == FALSE)
        {
            /* Switch to battery power for screen */
            sh1122_oled_off(&plat_oled_descriptor);
            platform_io_disable_3v3_to_oled_stepup();
            platform_io_assert_oled_reset();
            timer_delay_ms(15);
            platform_io_power_up_oled(FALSE);
            sh1122_init_display(&plat_oled_descriptor, TRUE);
            gui_dispatcher_get_back_to_current_screen();
            timer_deallocate_timer(temp_timer_id);
            return RETURN_NOK;
        }
    }
    
    /* Free timer */
    timer_deallocate_timer(temp_timer_id);
    
    /* Actually start charging */
    comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_NIMH_CHG_SLW_STRT);
    comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_CHARGE_STARTED);
    logic_power_set_battery_charging_bool(TRUE, FALSE);
    
    /* Wait for end of charge */
    gui_prompts_display_information_on_screen(RECOND_CHARGE_TEXT_ID, DISP_MSG_INFO);
    while (logic_power_is_battery_charging() != FALSE)
    {
        /* Idle animation */
        if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
        {
            /* Display new animation frame bitmap, rearm timer with provided value */
            gui_prompts_display_information_on_string_single_anim_frame(&gui_dispatcher_current_idle_anim_frame_id, &temp_uint16, DISP_MSG_INFO);
            timer_start_timer(TIMER_ANIMATIONS, temp_uint16);
        }
        
        /* Do not deal with incoming messages */
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        logic_accelerometer_routine();

        /* User disconnected USB? */
        if (platform_io_is_usb_3v3_present_raw() == FALSE)
        {
            /* Switch to battery power for screen */
            sh1122_oled_off(&plat_oled_descriptor);
            platform_io_disable_3v3_to_oled_stepup();
            platform_io_assert_oled_reset();
            timer_delay_ms(15);
            platform_io_power_up_oled(FALSE);
            sh1122_init_display(&plat_oled_descriptor, TRUE);
            gui_dispatcher_get_back_to_current_screen();
            return RETURN_NOK;
        }
    }

    return RETURN_OK;
}
