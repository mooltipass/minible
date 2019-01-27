/*!  \file     logic_device.c
*    \brief    General logic for device
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/
#include "logic_device.h"
#include "driver_timer.h"
#include "custom_fs.h"
#include "defines.h"
#include "utils.h"


/*! \fn     logic_device_activity_detected(void)
*   \brief  Function called whenever some activity is detected
*/
void logic_device_activity_detected(void)
{
    /* User interaction timer */
    timer_start_timer(TIMER_USER_INTERACTION, utils_check_value_for_range(custom_fs_settings_get_device_setting(SETTING_USER_INTERACTION_TIMEOUT_ID), SETTING_MIN_USER_INTERACTION_TIMEOUT, SETTING_MAX_USER_INTERACTION_TIMOUT) << 10);
}