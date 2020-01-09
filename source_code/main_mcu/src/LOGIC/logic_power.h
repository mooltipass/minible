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
/*!  \file     logic_power.h
*    \brief    Power logic
*    Created:  29/09/2018
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_POWER_H_
#define LOGIC_POWER_H_

/* Defines */
#ifdef EMULATOR_BUILD
/* Reduce the wait to 5 seconds on emulator */
#define NB_MS_BATTERY_OPERATED_BEFORE_CHARGE_ENABLE     (5UL * 1000UL)
#else
#define NB_MS_BATTERY_OPERATED_BEFORE_CHARGE_ENABLE     (5UL * 60UL * 1000UL)
#endif
/* Size of the buffer of last ADC conversions */
#define LAST_VOLTAGE_CONV_BUFF_SIZE         10

/* Enums */
typedef enum    {USB_POWERED, BATTERY_POWERED} power_source_te;
typedef enum    {POWER_ACT_NONE, POWER_ACT_POWER_OFF, POWER_ACT_NEW_BAT_LEVEL} power_action_te;
typedef enum    {BATTERY_0PCT = 0, BATTERY_25PCT, BATTERY_50PCT, BATTERY_75PCT, BATTERY_100PCT, BATTERY_ERROR} battery_state_te;
typedef enum    {LB_IDLE = 0, LB_CHARGE_START_RAMPING = 1, LB_CHARGING_REACH = 2, LB_ERROR_ST_RAMPING = 3, LB_CUR_MAINTAIN = 4, LB_ERROR_CUR_REACH = 5, LB_ERROR_CUR_MAINTAIN = 6, LB_CHARGING_DONE = 7} lb_state_machine_te;

/* Prototypes */
power_action_te logic_power_routine(BOOL wait_for_adc_conversion_and_dont_start_another);
void logic_power_set_battery_charging_bool(BOOL battery_charging, BOOL charge_success);
void logic_power_set_battery_level_update_from_aux(uint8_t battery_level);
void logic_power_skip_queue_logic_for_upcoming_adc_measurements(void);
void logic_power_register_vbat_adc_measurement(uint16_t adc_val);
void logic_power_set_power_source(power_source_te power_source);
uint16_t logic_power_get_and_ack_new_battery_level(void);
BOOL logic_power_is_usb_enumerate_sent_clear_bool(void);
battery_state_te logic_power_get_battery_state(void);
power_source_te logic_power_get_power_source(void);
void logic_power_usb_enumerate_just_sent(void);
void logic_power_signal_battery_error(void);
BOOL logic_power_is_battery_charging(void);
void logic_power_power_down_actions(void);
void logic_power_ms_tick(void);
void logic_power_init(void);

#endif /* LOGIC_POWER_H_ */