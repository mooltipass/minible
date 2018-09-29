/*!  \file     logic_power.c
*    \brief    Power logic
*    Created:  29/09/2018
*    Author:   Mathieu Stephan
*/
#include "logic_power.h"
/* Current power source */
power_source_te logic_power_current_power_source;


/*! \fn     logic_power_set_power_source(power_source_te power_source)
*   \brief  Set current power source
*   \param  power_source    Power source (see enum)
*/
void logic_power_set_power_source(power_source_te power_source)
{
    logic_power_current_power_source = power_source;
}

/*! \fn     logic_power_get_power_source(void)
*   \brief  Get current power source
*   \return Current power source (see enum)
*/
power_source_te logic_power_get_power_source(void)
{
    return logic_power_current_power_source;
}