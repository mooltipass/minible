/*
 * logic_battery.h
 *
 * Created: 13/09/2018 21:32:38
 *  Author: limpkin
 */ 


#ifndef LOGIC_BATTERY_H_
#define LOGIC_BATTERY_H_

/* Enums */
typedef enum    {LB_IDLE = 0, LB_CHARGE_START_RAMPING = 1, LB_CHARGING_REACH = 2, LB_ERROR_ST_RAMPING = 3, LB_CUR_MAINTAIN = 4, LB_ERROR_CUR_REACH = 5, LB_ERROR_CUR_MAINTAIN = 6, LB_CHARGING_DONE = 7} lb_state_machine_te;
typedef enum    {NIMH_12C_CHARGING} lb_nimh_charge_scheme_te;

    
/* Defines */

/******************************/
/* Charging Algorithm defines */
/******************************/
// Charging start: quick ramping up until we reach the desired pre-charge current or we end up at a too high voltage
#define LOGIC_BATTERY_BAT_START_CHG_VOLTAGE 550     // Voltage at which we start ramping up the charge
#define LOGIC_BATTERY_BAT_START_CHG_V_INC   10      // Voltage increments for charge
#define LOGIC_BATTERY_CUR_FOR_ST_RAMP_END   100     // ADC value different between high & low cursense to stop the initial ramping: 1LSB = 0.5445mA
#define LOGIC_BATTERY_MAX_V_FOR_ST_RAMP     2938    // Voltage at which we consider that something is wrong during initial ramp (around 1.6V)
#define LOGIC_BATTERY_START_CHARGE_DELAY    100     // Delay before taking the first decision in our charging algorithm
// Charging current reaching: after quick ramp up, trying to reach the targeted charging current
#define LOGIC_BATTERY_CUR_REACH_TICK        5       // Time intervals between decisions
#define LOGIC_BATTERY_BAT_CUR_REACH_V_INC   1       // Voltage increments for charge
#define LOGIC_BATTERY_CUR_FOR_REACH_END_12C 275     // ADC value different between high & low cursense to stop current reach ramping: 1LSB = 0.5445mA
#define LOGIC_BATTERY_MAX_V_FOR_CUR_REACH   2938    // Voltage at which we consider that something is wrong (around 1.6V)
/* Charging current maintaining */
#define LOGIC_BATTERY_CUR_MAINTAIN_TICK     10      // Time intervals between decisions
/* End of charge detection */
#define LOGIC_BATTERY_END_OF_CHARGE_NEG_V   5      // Decrease in ADC value during charging (around 2mV)

/* Prototypes */
void logic_battery_start_charging(lb_nimh_charge_scheme_te charging_type);
void logic_battery_debug_force_charge_voltage(uint16_t charge_voltage);
lb_state_machine_te logic_battery_get_charging_status(void);
uint16_t logic_battery_get_stepdown_voltage(void);
int16_t logic_battery_get_charging_current(void);
void logic_battery_debug_stop_charge(void);
void logic_battery_stop_charging(void);
uint16_t logic_battery_get_vbat(void);
void logic_battery_init(void);
void logic_battery_task(void);


#endif /* LOGIC_BATTERY_H_ */
