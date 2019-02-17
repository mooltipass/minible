/*!  \file     logic_device.c
*    \brief    General logic for device
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/
#include "logic_device.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "logic_power.h"
#include "custom_fs.h"
#include "defines.h"
#include "sh1122.h"
#include "utils.h"
#include "main.h"


/*! \fn     logic_device_activity_detected(void)
*   \brief  Function called whenever some activity is detected
*/
void logic_device_activity_detected(void)
{
    /* User interaction timer */
    timer_start_timer(TIMER_SCREEN, 60000);
    timer_start_timer(TIMER_USER_INTERACTION, utils_check_value_for_range(custom_fs_settings_get_device_setting(SETTING_USER_INTERACTION_TIMEOUT_ID), SETTING_MIN_USER_INTERACTION_TIMEOUT, SETTING_MAX_USER_INTERACTION_TIMOUT) << 10);
    
    /* Check for screen off, switch it on if so */
    if (sh1122_is_oled_on(&plat_oled_descriptor) == FALSE)
    {
        if (platform_io_is_usb_3v3_present() == FALSE)
        {
            logic_power_set_power_source(BATTERY_POWERED);
            platform_io_power_up_oled(FALSE);
        }
        else
        {
            logic_power_set_power_source(USB_POWERED);
            platform_io_power_up_oled(TRUE);
        }
        
        sh1122_oled_on(&plat_oled_descriptor);
    }
}