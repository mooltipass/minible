/*!  \file     logic_power.h
*    \brief    Power logic
*    Created:  29/09/2018
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_POWER_H_
#define LOGIC_POWER_H_

/* Defines */
#define NB_MS_BATTERY_OPERATED_BEFORE_CHARGE_ENABLE     (5UL * 60UL * 1000UL)

/* Enums */
typedef enum    {USB_POWERED, BATTERY_POWERED} power_source_te;
typedef enum    {POWER_ACT_NONE, POWER_ACT_POWER_OFF} power_action_te;
typedef enum    {BATTERY_0PCT = 0, BATTERY_25PCT, BATTERY_50PCT, BATTERY_75PCT, BATTERY_100PCT, BATTERY_ERROR} battery_state_te;
typedef enum    {LB_IDLE = 0, LB_CHARGE_START_RAMPING = 1, LB_CHARGING_REACH = 2, LB_ERROR_ST_RAMPING = 3, LB_CUR_MAINTAIN = 4, LB_ERROR_CUR_REACH = 5, LB_ERROR_CUR_MAINTAIN = 6, LB_CHARGING_DONE = 7} lb_state_machine_te;

/* Prototypes */
void logic_power_set_battery_charging_bool(BOOL battery_charging, BOOL charge_success);
void logic_power_register_vbat_adc_measurement(uint16_t adc_val);
void logic_power_set_power_source(power_source_te power_source);
battery_state_te logic_power_get_battery_state(void);
power_source_te logic_power_get_power_source(void);
BOOL logic_power_is_battery_charging(void);
power_action_te logic_power_routine(void);
void logic_power_ms_tick(void);

#endif /* LOGIC_POWER_H_ */