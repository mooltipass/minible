/*!  \file     logic_power.c
*    \brief    Power logic
*    Created:  29/09/2018
*    Author:   Mathieu Stephan
*/
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "logic_device.h"
#include "logic_power.h"
#include "platform_io.h"
#include "custom_fs.h"
#include "sh1122.h"
#include "main.h"
// Number of ms spent on battery since the last full charge
volatile uint32_t logic_power_nb_ms_spent_since_last_full_charge = 0;
/* Current power source */
power_source_te logic_power_current_power_source;
/* Last Vbat measured ADC value */
uint16_t logic_power_last_vbat_measurement;
/* Battery charging bool */
BOOL logic_power_battery_charging = FALSE;
// Number of ADC conversions since last power change
uint16_t logic_power_nb_adc_conv_since_last_power_change = 0;


/*! \fn     logic_power_ms_tick(void)
*   \brief  Function called every ms by interrupt
*/
void logic_power_ms_tick(void)
{
    /* Increment nb ms since last full charge counter */
    if ((logic_power_nb_ms_spent_since_last_full_charge != UINT32_MAX) && (logic_power_current_power_source == BATTERY_POWERED))
    {
        logic_power_nb_ms_spent_since_last_full_charge++;
    }
}

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

/*! \fn     logic_power_init(void)
*   \brief  Init power logic
*/
void logic_power_init(void)
{
    logic_power_nb_ms_spent_since_last_full_charge = custom_fs_get_nb_ms_since_last_full_charge();
}

/*! \fn     logic_power_power_down_actions(void)
*   \brief  Actions to be taken before powering down
*/
void logic_power_power_down_actions(void)
{
    custom_fs_define_nb_ms_since_last_full_charge(logic_power_nb_ms_spent_since_last_full_charge);
}

/*! \fn     logic_power_set_battery_charging_bool(BOOL battery_charging, BOOL charge_success)
*   \brief  Set battery charging bool
*   \param  battery_charging    The boolean
*   \param  charge_success      If previous bool is FALSE, inform of charge success
*/
void logic_power_set_battery_charging_bool(BOOL battery_charging, BOOL charge_success)
{
    logic_power_battery_charging = battery_charging;
    
    /* In case of charge success, reset our counter */
    if ((battery_charging == FALSE) && (charge_success != FALSE))
    {
        logic_power_nb_ms_spent_since_last_full_charge = 0;
        custom_fs_define_nb_ms_since_last_full_charge(logic_power_nb_ms_spent_since_last_full_charge);
    }
}

/*! \fn     logic_power_is_battery_charging(void)
*   \brief  Get battery charging bool
*   \return The boolean
*/
BOOL logic_power_is_battery_charging(void)
{
    return logic_power_battery_charging;
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
        timer_delay_ms(15);
        platform_io_power_up_oled(TRUE);
        sh1122_oled_on(&plat_oled_descriptor);
        logic_device_activity_detected();
        logic_power_nb_adc_conv_since_last_power_change = 0;
    }
    else if ((logic_power_get_power_source() == USB_POWERED) && (platform_io_is_usb_3v3_present() == FALSE))
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_DETACH_USB);
        logic_power_set_battery_charging_bool(FALSE, FALSE);
        logic_power_set_power_source(BATTERY_POWERED);
        logic_aux_mcu_set_usb_enumerated_bool(FALSE);
        sh1122_oled_off(&plat_oled_descriptor);
        timer_delay_ms(15);
        platform_io_power_up_oled(FALSE);
        sh1122_oled_on(&plat_oled_descriptor);
        logic_device_activity_detected();
        logic_power_nb_adc_conv_since_last_power_change = 0;
    }
    
    /* Battery charging */
    if ((logic_power_get_power_source() == USB_POWERED) && (logic_aux_mcu_is_usb_just_enumerated() != FALSE) && (logic_power_battery_charging == FALSE) && (logic_power_nb_ms_spent_since_last_full_charge > NB_MS_BATTERY_OPERATED_BEFORE_CHARGE_ENABLE))
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_NIMH_CHARGE);
        logic_power_battery_charging = TRUE;
    }
    
    /* Battery measurement */
    if (platform_io_is_voledin_conversion_result_ready() != FALSE)
    {
        uint16_t current_vbat = platform_io_get_voledin_conversion_result_and_trigger_conversion();
        
        /* Var increment */
        if (logic_power_nb_adc_conv_since_last_power_change != UINT16_MAX)
        {
            logic_power_nb_adc_conv_since_last_power_change++;
        }
        
        /* Store current vbat only if we are battery powered */
        if ((platform_io_is_usb_3v3_present() == FALSE) && (logic_power_nb_adc_conv_since_last_power_change > 5))
        {
            logic_power_last_vbat_measurement = current_vbat;
        }
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