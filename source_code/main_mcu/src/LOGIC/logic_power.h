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
/* Size of the buffer of last ADC conversions */
#define LAST_VOLTAGE_CONV_BUFF_SIZE         10

/* Enums */
typedef enum    {USB_POWERED = 0, BATTERY_POWERED,} power_source_te;
typedef enum    {NORMAL_CHARGE, SLOW_START_CHARGE, RECOVERY_CHARGE} nimh_charge_te;
typedef enum    {POWER_ACT_NONE, POWER_ACT_POWER_OFF, POWER_ACT_NEW_BAT_LEVEL} power_action_te;
typedef enum    {BATTERY_0PCT = 0, BATTERY_25PCT = 1, BATTERY_50PCT = 2, BATTERY_75PCT = 3, BATTERY_100PCT = 4, BATTERY_ERROR = 5} battery_state_te;
typedef enum    {LB_IDLE = 0, LB_CHARGE_START_RAMPING = 1, LB_CHARGING_REACH = 2, LB_ERROR_ST_RAMPING = 3, LB_CUR_MAINTAIN = 4, LB_ERROR_CUR_REACH = 5, LB_ERROR_CUR_MAINTAIN = 6, LB_CHARGING_DONE = 7} lb_state_machine_te;

/* Typedefs */
typedef struct
{
    uint32_t nb_30mins_powered_on;              // Lowest level state: 96uA
    uint32_t nb_30mins_card_inserted;           // Adding 42uA
    uint32_t nb_30mins_ble_advertising;         // Adding 823uA to first 2
    uint32_t nb_30mins_ios_connect;             // Adding xuA to first 2
    uint32_t nb_30mins_macos_connect;           // Adding xuA to first 2
    uint32_t nb_30mins_android_connect;         // Adding 74uA to first 2
    uint32_t nb_30mins_windows_connect;         // Adding 2360uA to first 2 (!)
    uint32_t nb_ms_no_screen_aux_main_awake;    // 100mA, 30mA AUX only before MAIN wakeup then 86mA both
    uint32_t nb_ms_no_screen_main_awake;        // 50mA averaged over 163ms wakeup time, with a nice 256mA peak for 3ms
    uint32_t nb_ms_full_pawa;                   // 160mA as a guideline
    uint32_t nb_ms_spent_since_last_full_charge;// Number of ms since last full charge
    uint32_t aux_mcu_reported_pct;              // Last AUX mcu reported battery pct
    uint32_t lifetime_nb_ms_screen_on_msb;      // Total number of ms spent with screen on, msb
    uint32_t lifetime_nb_ms_screen_on_lsb;      // Total number of ms spent with screen on, lsb
    uint32_t lifetime_nb_30mins_bat;            // Total number of 30mins spent battery powered
    uint32_t lifetime_nb_30mins_usb;            // Total number of 30mins spent usb powered
} power_consumption_log_t;

/* Prototypes */
power_action_te logic_power_check_power_switch_and_battery(BOOL wait_for_adc_conversion_and_dont_start_another);
void logic_power_set_battery_charging_bool(BOOL battery_charging, BOOL charge_success);
void logic_power_set_battery_level_update_from_aux(uint8_t battery_level);
power_consumption_log_t* logic_power_get_power_consumption_log_pt(void);
void logic_power_skip_queue_logic_for_upcoming_adc_measurements(void);
RET_TYPE logic_power_battery_recondition(uint32_t* discharge_time);
void logic_power_register_vbat_adc_measurement(uint16_t adc_val);
void logic_power_set_power_source(power_source_te power_source);
uint16_t logic_power_debug_get_last_adc_measurement(void);
BOOL logic_power_get_and_reset_over_discharge_flag(void);
uint16_t logic_power_get_and_ack_new_battery_level(void);
nimh_charge_te logic_power_get_current_charge_type(void);
BOOL logic_power_is_usb_enumerate_sent_clear_bool(void);
void logic_power_switch_off_after_usb_disconnect(void);
void logic_power_init(BOOL poweredoff_due_to_battery);
battery_state_te logic_power_get_battery_state(void);
power_source_te logic_power_get_power_source(void);
void logic_power_inform_of_over_discharge(void);
void logic_power_usb_enumerate_just_sent(void);
void logic_power_compute_battery_state(void);
uint16_t logic_power_get_battery_level(void);
void logic_power_signal_battery_error(void);
BOOL logic_power_is_battery_charging(void);
void logic_power_power_down_actions(void);
void logic_power_30m_tick(void);
void logic_power_routine(void);
void logic_power_ms_tick(void);

#endif /* LOGIC_POWER_H_ */