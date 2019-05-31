/*!  \file     logic_power.c
*    \brief    Power logic
*    Created:  29/09/2018
*    Author:   Mathieu Stephan
*/
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "logic_power.h"
#include "platform_io.h"
#include "sh1122.h"
#include "main.h"
/* Current power source */
power_source_te logic_power_current_power_source;
/* Last Vbat measured ADC value */
uint16_t logic_power_last_vbat_measurement;


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

/*! \fn     logic_power_get_battery_state(void)
*   \brief  Get current battery state
*   \return Current battery state (see enum)
*/
battery_state_te logic_power_get_battery_state(void)
{    
    /* I'm sure some more fancy logic will be required here in the future */
    if (logic_power_last_vbat_measurement < BATTERY_ADC_20PCT_VOLTAGE)
    {
        return BATTERY_0PCT;
    }
    else if (logic_power_last_vbat_measurement < BATTERY_ADC_40PCT_VOLTAGE)
    {
        return BATTERY_25PCT;
    }
    else if (logic_power_last_vbat_measurement < BATTERY_ADC_60PCT_VOLTAGE)
    {
        return BATTERY_50PCT;
    }
    else if (logic_power_last_vbat_measurement < BATTERY_ADC_80PCT_VOLTAGE)
    {
        return BATTERY_75PCT;
    }
    else
    {
        return BATTERY_100PCT;
    }
}

/*! \fn     logic_power_register_vbat_adc_measurement(uint16_t adc_val)
*   \brief  Register an ADC measurement of the battery voltage
*   \param  adc_val The ADC value
*/
void logic_power_register_vbat_adc_measurement(uint16_t adc_val)
{
    logic_power_last_vbat_measurement = adc_val;
}

/*! \fn     logic_power_routine(void)
*   \brief  Power handling routine
*   \return An action if needed (see enum)
*/
power_action_te logic_power_routine(void)
{    
    /* Power supply change */
    if ((logic_power_get_power_source() == BATTERY_POWERED) && (platform_io_is_usb_3v3_present() != FALSE))
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_ATTACH_USB);
        logic_power_set_power_source(USB_POWERED);
        sh1122_oled_off(&plat_oled_descriptor);
        timer_delay_ms(5);
        platform_io_power_up_oled(TRUE);
        sh1122_oled_on(&plat_oled_descriptor);
    }
    else if ((logic_power_get_power_source() == USB_POWERED) && (platform_io_is_usb_3v3_present() == FALSE))
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_DETACH_USB);
        logic_power_set_power_source(BATTERY_POWERED);
        sh1122_oled_off(&plat_oled_descriptor);
        timer_delay_ms(5);
        platform_io_power_up_oled(FALSE);
        sh1122_oled_on(&plat_oled_descriptor);
    }
    
    /* Battery measurement */
    if (platform_io_is_voledin_conversion_result_ready() != FALSE)
    {
        logic_power_last_vbat_measurement = platform_io_get_voledin_conversion_result_and_trigger_conversion();
    }
    
    /* Action based on battery measurement */
    if ((logic_power_get_power_source() == BATTERY_POWERED) && (logic_power_last_vbat_measurement < BATTERY_ADC_OUT_CUTOUT))
    {
        return POWER_ACT_POWER_OFF;
    }
    else
    {
        return POWER_ACT_NONE;
    }
}