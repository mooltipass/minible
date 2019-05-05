/*!  \file     logic_device.c
*    \brief    General logic for device
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/
#include "gui_dispatcher.h"
#include "logic_device.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "logic_power.h"
#include "custom_fs.h"
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

/*! \fn     logic_device_bundle_update_start(BOOL from_debug_messages)
*   \brief  Function called when start updating the device graphics memory
*   \param  from_debug_messages Set to TRUE if this function was called from debug messages
*/
void logic_device_bundle_update_start(BOOL from_debug_messages)
{
    logic_device_activity_detected();
    
    /* If we are in invalid screen, it means we don't have a bundle */
    if (gui_dispatcher_get_current_screen() != GUI_SCREEN_INVALID)
    {
        if (from_debug_messages != FALSE)
        {
            /* Go to dedicated screen */
            gui_dispatcher_set_current_screen(GUI_SCREEN_FW_FILE_UPDATE, TRUE, GUI_OUTOF_MENU_TRANSITION);
            gui_dispatcher_get_back_to_current_screen();
        }
        else
        {
            // TODO3
        }
    }
}

/*! \fn     logic_device_bundle_update_end(BOOL from_debug_messages)
*   \brief  Function called at the end of graphics upload
*   \param  from_debug_messages Set to TRUE if this function was called from debug messages
*/
void logic_device_bundle_update_end(BOOL from_debug_messages)
{
    /* If we are in invalid screen, it means we don't have a bundle */
    if (gui_dispatcher_get_current_screen() != GUI_SCREEN_INVALID)
    {
        if (from_debug_messages != FALSE)
        {        
            /* Refresh file system and font */
            custom_fs_init();
        
            /* Go to default screen */
            gui_dispatcher_set_current_screen(GUI_SCREEN_NINSERTED, TRUE, GUI_OUTOF_MENU_TRANSITION);
            gui_dispatcher_get_back_to_current_screen();
        } 
        else
        {
            // TODO3
        }
    }
}