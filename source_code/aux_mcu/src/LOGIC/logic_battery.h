/*
 * logic_battery.h
 *
 * Created: 13/09/2018 21:32:38
 *  Author: limpkin
 */ 


#ifndef LOGIC_BATTERY_H_
#define LOGIC_BATTERY_H_

/* Enums */
typedef enum    {LB_IDLE = 0, LB_CHARGE_START_RAMPING = 1, LB_CHARGING_REACH = 2, LB_ERROR_ST_RAMPING = 3, LB_CUR_MAINTAIN = 4, LB_ERROR_CUR_REACH = 5, LB_ERROR_CUR_MAINTAIN = 6, LB_CHARGING_DONE = 7, LB_PEAK_TIMER_TRIGGERED = 8} lb_state_machine_te;
typedef enum    {BAT_ACT_NONE = 0, BAT_ACT_NEW_BAT_LEVEL, BAT_ACT_CHARGE_FAIL, BAT_ACT_CHARGE_DONE} battery_action_te;
typedef enum    {NIMH_12C_CHARGING = 0, NIMH_23C_CHARGING, NIMH_45C_CHARGING} lb_nimh_charge_scheme_te;
    
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
#define LOGIC_BATTERY_CUR_FOR_REACH_END_23C 367     // ADC value different between high & low cursense to stop current reach ramping: 1LSB = 0.5445mA
#define LOGIC_BATTERY_CUR_FOR_REACH_END_45C 441     // ADC value different between high & low cursense to stop current reach ramping: 1LSB = 0.5445mA
#define LOGIC_BATTERY_MAX_V_FOR_CUR_REACH   3050    // Voltage at which we consider that something is wrong (around 1.66V)
/* Charging current maintaining */
#define LOGIC_BATTERY_CUR_MAINTAIN_TICK     10      // Time intervals between decisions
/* End of charge detection */
#define LOGIC_BATTERY_END_OF_CHARGE_NEG_V   2       // Decrease in ADC value during charging (around 1.5mV)
/* Safety feature */
#define LOGIC_BATTERY_NB_SECS_AFTER_PEAK    1000    // Maximum time allowed after peak voltage has been reached (17 minutes - measured at 8 minutes for standard charges)

/**************************/
/* Battery levels defines */
/**************************/
#define BATTERY_ADC_OVER_VOLTAGE    (1600*8192/3300)
#define BATTERY_ADC_100PCT_VOLTAGE  (2837)
#define BATTERY_ADC_90PCT_VOLTAGE   (2757)
#define BATTERY_ADC_80PCT_VOLTAGE   (2702)
#define BATTERY_ADC_70PCT_VOLTAGE   (2669)
#define BATTERY_ADC_60PCT_VOLTAGE   (2646)
#define BATTERY_ADC_50PCT_VOLTAGE   (2628)
#define BATTERY_ADC_40PCT_VOLTAGE   (2610)
#define BATTERY_ADC_30PCT_VOLTAGE   (2590)
#define BATTERY_ADC_20PCT_VOLTAGE   (2566)
#define BATTERY_ADC_10PCT_VOLTAGE   (2512)
#define BATTERY_ADC_OUT_CUTOUT      (2220)

/* Prototypes */
void logic_battery_start_charging(lb_nimh_charge_scheme_te charging_type);
void logic_battery_debug_force_charge_voltage(uint16_t charge_voltage);
lb_state_machine_te logic_battery_get_charging_status(void);
uint16_t logic_battery_get_stepdown_voltage(void);
int16_t logic_battery_get_charging_current(void);
void logic_battery_debug_stop_charge(void);
battery_action_te logic_battery_task(void);
void logic_battery_start_using_adc(void);
void logic_battery_stop_using_adc(void);
void logic_battery_stop_charging(void);
uint16_t logic_battery_get_vbat(void);
BOOL logic_battery_is_using_adc(void);
void logic_battery_init(void);


#endif /* LOGIC_BATTERY_H_ */
