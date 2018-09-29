/*!  \file     logic_power.h
*    \brief    Power logic
*    Created:  29/09/2018
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_POWER_H_
#define LOGIC_POWER_H_

/* Enums */
typedef enum    {USB_POWERED, BATTERY_POWERED} power_source_te;

/* Prototypes */
void logic_power_set_power_source(power_source_te power_source);
power_source_te logic_power_get_power_source(void);

#endif /* LOGIC_POWER_H_ */