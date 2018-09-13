/*
 * logic_battery.h
 *
 * Created: 13/09/2018 21:32:38
 *  Author: limpkin
 */ 


#ifndef LOGIC_BATTERY_H_
#define LOGIC_BATTERY_H_

/* Enums */
typedef enum    {LB_IDLE, LB_CHARGE_START_RAMPING, LB_CHARGING, LB_ERROR} lb_state_machine_te;
    
/* Defines */
#define LOGIC_BATTERY_TIME_BETWEEN_ACTS     5000    // Time between actions
#define LOGIC_BATTERY_BAT_START_CHG_VOLTAGE 550     // Voltage at which we start ramping up the charge
#define LOGIC_BATTERY_BAT_START_CHG_V_INC   10      // Voltage increments for charge
#define LOGIC_BATTERY_CUR_FOR_ST_RAMP_END   10      // ADC value different between high & low cursense to stop the initial ramping (around 6mA)
#define LOGIC_BATTERY_MAX_V_FOR_ST_RAMP     1650    // Voltage at which we consider that something is wrong during initial ramp

/* Prototypes */
void logic_battery_start_charging(void);
void logic_battery_init(void);
void logic_battery_task(void);


#endif /* LOGIC_BATTERY_H_ */