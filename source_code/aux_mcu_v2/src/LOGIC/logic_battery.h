/*
 * logic_battery.h
 *
 * Created: 13/09/2018 21:32:38
 *  Author: limpkin
 */ 


#ifndef LOGIC_BATTERY_H_
#define LOGIC_BATTERY_H_

/* Enums */
typedef enum    {LB_IDLE, LB_CHARGE_START_RAMPING, LB_CHARGING_REACH, LB_ERROR_ST_RAMPING, LB_CUR_MAINTAIN, LB_ERROR_CUR_REACH, LB_ERROR_CUR_MAINTAIN, LB_CHARGING_DONE} lb_state_machine_te;
typedef enum    {NIMH_13C_CHARGING} lb_nimh_charge_scheme_te;

    
/* Defines */

/******************************/
/* Charging Algorithm defines */
/******************************/
// Charging start: quick ramping up until we reach the desired pre-charge current or we end up at a too high voltage
#define LOGIC_BATTERY_BAT_START_CHG_VOLTAGE 550     // Voltage at which we start ramping up the charge
#define LOGIC_BATTERY_BAT_START_CHG_V_INC   10      // Voltage increments for charge
#define LOGIC_BATTERY_CUR_FOR_ST_RAMP_END   100     // ADC value different between high & low cursense to stop the initial ramping: 1LSB = 0.4mA
#define LOGIC_BATTERY_MAX_V_FOR_ST_RAMP     1650    // Voltage at which we consider that something is wrong during initial ramp
// Charging current reaching: after quick ramp up, trying to reach the targeted charging current
#define LOGIC_BATTERY_CUR_REACH_TICK        5       // Time intervals between decisions
#define LOGIC_BATTERY_BAT_CUR_REACH_V_INC   1       // Voltage increments for charge
#define LOGIC_BATTERY_CUR_FOR_REACH_END_13C 250     // ADC value different between high & low cursense to stop current reach ramping: 1LSB = 0.4mA
#define LOGIC_BATTERY_MAX_V_FOR_CUR_REACH   1650    // Voltage at which we consider that something is wrong
/* Charging current maintaining */
#define LOGIC_BATTERY_CUR_MAINTAIN_TICK     10      // Time intervals between decisions
/* End of charge detection */
#define LOGIC_BATTERY_END_OF_CHARGE_NEG_V   10      // Decrease in ADC value during charging (around 4mV)

/* Prototypes */
void logic_battery_start_charging(lb_nimh_charge_scheme_te charging_type);
lb_state_machine_te logic_battery_get_charging_status(void);
int16_t logic_battery_get_charging_current(void);
uint16_t logic_battery_get_vbat(void);
void logic_battery_init(void);
void logic_battery_task(void);


#endif /* LOGIC_BATTERY_H_ */