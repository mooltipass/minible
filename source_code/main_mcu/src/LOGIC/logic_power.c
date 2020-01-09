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
#include "logic_smartcard.h"
#include "gui_dispatcher.h"
#include "logic_security.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "logic_device.h"
#include "logic_power.h"
#include "platform_io.h"
#include "custom_fs.h"
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
// Number of ms spent on battery since the last full charge
volatile uint32_t logic_power_nb_ms_spent_since_last_full_charge = 0;
/* Current power source */
power_source_te logic_power_current_power_source;
/* Last Vbat measured ADC values */
uint16_t logic_power_last_vbat_measurements[LAST_VOLTAGE_CONV_BUFF_SIZE];
/* Battery charging bool */
BOOL logic_power_battery_charging = FALSE;
/* Error with battery flag */
BOOL logic_power_error_with_battery = FALSE;
/* If the "enumerate usb" command was just sent */
BOOL logic_power_enumerate_usb_command_just_sent = FALSE;
/* Number of measurements left to discard */
uint16_t logic_power_discard_measurement_counter = 0;
/* Bool set for device boot */
BOOL logic_power_device_boot_flag_for_initial_battery_level_notif = TRUE;
/* Number of times we still can skip the queue logic when taking into account an adc measurement */
uint16_t logic_power_nb_times_to_skip_adc_measurement_queue = 0;
/* ADC measurement counter */
uint16_t logic_power_adc_measurement_counter = 0;


/*! \fn     logic_power_ms_tick(void)
*   \brief  Function called every ms by interrupt
*/
void logic_power_ms_tick(void)
{
    /* Increment nb ms since last full charge counter */
    if ((logic_power_nb_ms_spent_since_last_full_charge != UINT32_MAX) && (logic_power_current_power_source == BATTERY_POWERED))
    {
        logic_power_nb_ms_spent_since_last_full_charge++;
    }
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
}

/*! \fn     logic_power_get_power_source(void)
*   \brief  Get current power source
*   \return Current power source (see enum)
*/
power_source_te logic_power_get_power_source(void)
{
    return logic_power_current_power_source;
}

/*! \fn     logic_power_init(void)
*   \brief  Init power logic
*/
void logic_power_init(void)
{
    cpu_irq_enter_critical();
    logic_power_nb_ms_spent_since_last_full_charge = custom_fs_get_nb_ms_since_last_full_charge();
    cpu_irq_leave_critical();
}

/*! \fn     logic_power_power_down_actions(void)
*   \brief  Actions to be taken before powering down
*/
void logic_power_power_down_actions(void)
{
    volatile uint32_t nb_ms_since_charge_copy = logic_power_nb_ms_spent_since_last_full_charge;
    custom_fs_define_nb_ms_since_last_full_charge(nb_ms_since_charge_copy);
}

/*! \fn     logic_power_get_and_ack_new_battery_level(void)
*   \brief  Acknowledge and get the new battery level
*   \return The new battery level in percent tenth
*/
uint16_t logic_power_get_and_ack_new_battery_level(void)
{
    logic_power_current_battery_level = logic_power_battery_level_to_be_acked;
    logic_power_device_boot_flag_for_initial_battery_level_notif = FALSE;
    logic_power_aux_mcu_battery_level_update = 0;
    return logic_power_current_battery_level;
}

/*! \fn     logic_power_set_battery_level_update_from_aux(uint8_t battery_level)
*   \brief  Called when the aux MCU reports a new battery level
*   \param  battery_level   The new battery level in percent tenth
*/
void logic_power_set_battery_level_update_from_aux(uint8_t battery_level)
{
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
        logic_power_nb_ms_spent_since_last_full_charge = 0;
        cpu_irq_leave_critical();
        custom_fs_define_nb_ms_since_last_full_charge(0);
        utils_fill_uint16_array_with_value(logic_power_last_vbat_measurements, ARRAY_SIZE(logic_power_last_vbat_measurements), BATTERY_ADC_100PCT_VOLTAGE);
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
    uint16_t vbat_measurement_from_a_bit_ago = logic_power_last_vbat_measurements[0];
    
    /* In case we haven't had the time to properly select a battery level, use raw adc value (this occurs at device boot) */
    if (logic_power_current_battery_level >= ARRAY_SIZE(logic_power_battery_level_mapping))
    {
        if (vbat_measurement_from_a_bit_ago < BATTERY_ADC_20PCT_VOLTAGE)
        {
            return BATTERY_0PCT;
        }
        else if (vbat_measurement_from_a_bit_ago < BATTERY_ADC_40PCT_VOLTAGE)
        {
            return BATTERY_25PCT;
        }
        else if (vbat_measurement_from_a_bit_ago < BATTERY_ADC_60PCT_VOLTAGE)
        {
            return BATTERY_50PCT;
        }
        else if (vbat_measurement_from_a_bit_ago < BATTERY_ADC_80PCT_VOLTAGE)
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

/*! \fn     logic_power_routine(BOOL wait_for_adc_conversion_and_dont_start_another)
*   \brief  Power handling routine
*   \param  wait_for_adc_conversion_and_dont_start_another  Set to TRUE to do what it says
*   \return An action if needed (see enum)
*/
power_action_te logic_power_routine(BOOL wait_for_adc_conversion_and_dont_start_another)
{        
    volatile uint32_t nb_ms_since_full_charge_copy = logic_power_nb_ms_spent_since_last_full_charge;
    
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
        
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_ATTACH_USB);
        comms_aux_mcu_wait_for_message_sent();
        sh1122_oled_off(&plat_oled_descriptor);
        platform_io_disable_vbat_to_oled_stepup();
        logic_power_set_power_source(USB_POWERED);
        logic_power_usb_enumerate_just_sent();
        platform_io_assert_oled_reset();
        timer_delay_ms(15);
        platform_io_power_up_oled(TRUE);
        sh1122_init_display(&plat_oled_descriptor);
        gui_dispatcher_get_back_to_current_screen();
        logic_device_activity_detected();
    }
    else if ((logic_power_get_power_source() == USB_POWERED) && (platform_io_is_usb_3v3_present() == FALSE))
    {
        aux_mcu_message_t* temp_rx_message_pt;
        
        /* Set inversion bool */
        plat_oled_descriptor.screen_inverted = (BOOL)custom_fs_settings_get_device_setting(SETTINGS_LEFT_HANDED_ON_BATTERY);
        inputs_set_inputs_invert_bool(plat_oled_descriptor.screen_inverted);
        
        /* Tell the aux MCU to detach USB, change power supply for OLED */
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_DETACH_USB);
        comms_aux_mcu_wait_for_message_sent();
        sh1122_oled_off(&plat_oled_descriptor);
        platform_io_disable_3v3_to_oled_stepup();
        logic_power_set_power_source(BATTERY_POWERED);
        logic_power_set_battery_charging_bool(FALSE, FALSE);
        logic_aux_mcu_set_usb_enumerated_bool(FALSE);
        platform_io_assert_oled_reset();
        timer_delay_ms(15);
        platform_io_power_up_oled(FALSE);
        sh1122_init_display(&plat_oled_descriptor);
        gui_dispatcher_get_back_to_current_screen();
        logic_device_activity_detected();
        
        /* Discard next 10 measurement */
        logic_power_discard_measurement_counter = 10;
                
        /* If user selected to, lock device */
        if ((logic_security_is_smc_inserted_unlocked() != FALSE) && ((BOOL)custom_fs_settings_get_device_setting(SETTINGS_LOCK_ON_DISCONNECT) != FALSE))
        {
            gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_LCK, TRUE, GUI_OUTOF_MENU_TRANSITION);
            gui_dispatcher_get_back_to_current_screen();
            logic_smartcard_handle_removed();
            timer_start_timer(TIMER_WAIT_FUNCTS, 250);
        }
        
        /* Wait for detach ACK from aux MCU */
        while(comms_aux_mcu_active_wait(&temp_rx_message_pt, FALSE, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, FALSE, AUX_MCU_EVENT_USB_DETACHED) != RETURN_OK);
        comms_aux_arm_rx_and_clear_no_comms();
        
        /* Wait for timer timeout to make sure card is not powered */
        if ((logic_security_is_smc_inserted_unlocked() != FALSE) && ((BOOL)custom_fs_settings_get_device_setting(SETTINGS_LOCK_ON_DISCONNECT) != FALSE))
        {
            while (timer_has_timer_expired(TIMER_WAIT_FUNCTS, TRUE) != TIMER_EXPIRED);
        }
        
        /* Clear inputs in case user move the wheel not on purpose */
        inputs_clear_detections();
    }
    else if ((logic_power_last_seen_voled_stepup_pwr_source == OLED_STEPUP_SOURCE_NONE) && (current_voled_pwr_source != logic_power_last_seen_voled_stepup_pwr_source))
    {
        /* If we have been told to bypass queue logic it means we want a quick measurement */
        if (logic_power_nb_times_to_skip_adc_measurement_queue != 0)
        {
            /* Device quick wake up every 20 minutes for 100ms */
            logic_power_discard_measurement_counter = 1;
        } 
        else
        {
            /* Device power-on or wakeup */
            logic_power_discard_measurement_counter = 5;
        }
    }
    
    /* Store last seen voled power source */
    logic_power_last_seen_voled_stepup_pwr_source = current_voled_pwr_source;
    
    /* Battery charging start logic */
    if ((logic_power_get_power_source() == USB_POWERED) && (logic_power_is_usb_enumerate_sent_clear_bool() != FALSE) && (logic_power_battery_charging == FALSE) && (nb_ms_since_full_charge_copy >= NB_MS_BATTERY_OPERATED_BEFORE_CHARGE_ENABLE))
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_NIMH_CHARGE);
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
            
            /* Safety: if the voltage is low enough, we override logic_power_nb_ms_spent_since_last_full_charge */
            if ((vbat_measurement_from_a_bit_ago < BATTERY_ADC_60PCT_VOLTAGE) && (nb_ms_since_full_charge_copy < NB_MS_BATTERY_OPERATED_BEFORE_CHARGE_ENABLE))
            {
                cpu_irq_enter_critical();
                logic_power_nb_ms_spent_since_last_full_charge = NB_MS_BATTERY_OPERATED_BEFORE_CHARGE_ENABLE;
                cpu_irq_leave_critical();
            }
            
            /* Low battery, need to power off? */
            if ((logic_power_get_power_source() == BATTERY_POWERED) && (vbat_measurement_from_a_bit_ago < BATTERY_ADC_OUT_CUTOUT) && (platform_io_is_usb_3v3_present_raw() == FALSE))
            {
                /* platform_io_is_usb_3v3_present_raw() call is here to prevent erroneous measurements */
                return POWER_ACT_POWER_OFF;
            }
            
            /* Check for new battery level */
            uint16_t possible_new_battery_level = 0;
            for (uint16_t i = 0; i < ARRAY_SIZE(logic_power_battery_level_mapping); i++)
            {
                if (vbat_measurement_from_a_bit_ago > logic_power_battery_level_mapping[i])
                {
                    possible_new_battery_level = i;
                }
            }
            
            /* Only allow decrease in voltage or increase in case we sent a charge message */
            if (possible_new_battery_level < logic_power_current_battery_level)
            {
                logic_power_battery_level_to_be_acked = possible_new_battery_level;
                return POWER_ACT_NEW_BAT_LEVEL;
            }
        }
        
        /* Counter decrement */
        if (logic_power_discard_measurement_counter != 0)
        {
            logic_power_discard_measurement_counter--;
        }
    }
    
    /* First device boot, flag is reset in the ack function */
    if (logic_power_device_boot_flag_for_initial_battery_level_notif != FALSE)
    {        
        /* Get device battery level */
        for (uint16_t i = 0; i < ARRAY_SIZE(logic_power_battery_level_mapping); i++)
        {
            if (logic_power_last_vbat_measurements[0] > logic_power_battery_level_mapping[i])
            {
                logic_power_battery_level_to_be_acked = i;
            }
        }
        
        /* Notify */
        return POWER_ACT_NEW_BAT_LEVEL;
    }

    /* New battery level reported by aux MCU */
    if (logic_power_aux_mcu_battery_level_update != 0)
    {
        /* Only allow it if above our current one */
        if (logic_power_aux_mcu_battery_level_update >= logic_power_current_battery_level)
        {
            /* Spoof measured voltage so if the device is unplugged the reported level is still correct */
            if (logic_power_aux_mcu_battery_level_update < ARRAY_SIZE(logic_power_battery_level_mapping))
            {
                utils_fill_uint16_array_with_value(logic_power_last_vbat_measurements, ARRAY_SIZE(logic_power_last_vbat_measurements), logic_power_battery_level_mapping[logic_power_aux_mcu_battery_level_update]);
            }
            
            /* Set new level to be acked */
            logic_power_battery_level_to_be_acked = logic_power_aux_mcu_battery_level_update;
            return POWER_ACT_NEW_BAT_LEVEL;
        } 
        else
        {
            logic_power_aux_mcu_battery_level_update = 0;
        }
    }
    
    /* Nothing to do */
    return POWER_ACT_NONE;
}