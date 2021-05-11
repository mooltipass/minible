/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2019 Stephan Mathieu
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*!  \file     logic_device.c
*    \brief    General logic for device
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/
#include "comms_aux_mcu_defines.h"
#include "logic_encryption.h"
#include "logic_security.h"
#include "gui_dispatcher.h"
#include "comms_aux_mcu.h"
#include "logic_aux_mcu.h"
#include "logic_device.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "logic_power.h"
#include "logic_user.h"
#include "custom_fs.h"
#include "bearssl.h"
#include "sh1122.h"
#include "utils.h"
#include "main.h"
#include "rng.h"
/* Platform wakeup reason */
volatile platform_wakeup_reason_te logic_device_wakeup_reason = WAKEUP_REASON_OTHER;
volatile BOOL logic_device_aux_mcu_wakeup_rcvd = FALSE;
/* USB timeout detected bool */
BOOL logic_device_usb_timeout_detected = FALSE;
/* Device state changed bool */
BOOL logic_device_state_changed = FALSE;
/* Settings changed bool */
BOOL logic_device_settings_changed = FALSE;
/* Time set bool */
BOOL logic_device_time_set = FALSE;


/*! \fn     logic_device_set_time_set(void)
*   \brief  Inform that the time was set
*/
void logic_device_set_time_set(void)
{
    logic_device_time_set = TRUE;
}

/*! \fn     logic_device_set_settings_changed(void)
*   \brief  Inform that the settings have changed
*/
void logic_device_set_settings_changed(void)
{
    logic_device_settings_changed = TRUE;
}

/*! \fn     logic_device_is_time_set(void)
*   \brief  Know if the time was set
*/
BOOL logic_device_is_time_set(void)
{
    return logic_device_time_set;
}

/*! \fn     logic_device_get_and_clear_settings_changed_flag(void)
*   \brief  Get and clear settings changed flag
*   \return The flag
*/
BOOL logic_device_get_and_clear_settings_changed_flag(void)
{
    BOOL return_val = logic_device_settings_changed;
    logic_device_settings_changed = FALSE;
    return return_val;
}

/*! \fn     logic_device_get_and_clear_usb_timeout_detected(void)
*   \brief  Get and clear USB timeout detected flag
*   \return The flag
*/
BOOL logic_device_get_and_clear_usb_timeout_detected(void)
{
    BOOL ret_val = logic_device_usb_timeout_detected;
    logic_device_usb_timeout_detected = FALSE;
    return ret_val;
}

/*! \fn     logic_device_set_usb_timeout_detected(void)
*   \brief  Set USB timeout detected flag
*/
void logic_device_set_usb_timeout_detected(void)
{
    logic_device_usb_timeout_detected = TRUE;
}

/*! \fn     logic_device_get_aux_wakeup_rcvd(void)
*   \brief  know if we received a wakeup event from aux mcu
*   \return the right boolean
*/
volatile BOOL logic_device_get_aux_wakeup_rcvd(void)
{
    return logic_device_aux_mcu_wakeup_rcvd;
}

/*! \fn     logic_device_clear_aux_wakeup_rcvd(void)
*   \brief  Clear aux wakeup received flag
*/
void logic_device_clear_aux_wakeup_rcvd(void)
{
    logic_device_aux_mcu_wakeup_rcvd = FALSE;
}

/*! \fn     logic_device_set_wakeup_reason(platform_wakeup_reason_te reason)
*   \brief  Set device wakeup reason
*   \param  reason  wakeup reason
*/
void logic_device_set_wakeup_reason(platform_wakeup_reason_te reason)
{
    if (logic_device_wakeup_reason == WAKEUP_REASON_NONE)
    {
        logic_device_wakeup_reason = reason;
    }
    else if (reason == WAKEUP_REASON_AUX_MCU)
    {
        logic_device_aux_mcu_wakeup_rcvd = TRUE;
    }
}

/*! \fn     logic_device_clear_wakeup_reason(void)
*   \brief  Clear current wakeup reason
*/
void logic_device_clear_wakeup_reason(void)
{
    logic_device_wakeup_reason = WAKEUP_REASON_NONE;
    logic_device_aux_mcu_wakeup_rcvd = FALSE;
}

/*! \fn     logic_device_get_wakeup_reason(void)
*   \brief  Get device wakeup reason
*   \return wakeup reason
*/
volatile platform_wakeup_reason_te logic_device_get_wakeup_reason(void)
{
    return logic_device_wakeup_reason;
}

/*! \fn     logic_device_power_off(void)
*   \brief  Switch off device
*/
void logic_device_power_off(void)
{
    logic_power_power_down_actions();           // Power down actions
    sh1122_oled_off(&plat_oled_descriptor);     // Display off command
    platform_io_power_down_oled();              // Switch off stepup
    platform_io_set_wheel_click_pull_down();    // Pull down on wheel click to slowly discharge capacitor
    timer_delay_ms(100);                        // From OLED datasheet wait before removing 3V3
    platform_io_set_wheel_click_low();          // Completely discharge cap
    timer_delay_ms(10);                         // Wait a tad
    platform_io_disable_switch_and_die();       // Die!    
}

/*! \fn     logic_device_activity_detected(void)
*   \brief  Function called whenever some activity is detected
*/
void logic_device_activity_detected(void)
{
    /* Reset timers */
    #ifndef EMULATOR_BUILD
    if (platform_io_is_usb_3v3_present() == FALSE)
    {
        timer_start_timer(TIMER_SCREEN, SCREEN_TIMEOUT_MS_BAT_PWRD);
    }
    else
    {
        timer_start_timer(TIMER_SCREEN, SCREEN_TIMEOUT_MS);        
    }    
    timer_start_timer(TIMER_USER_INTERACTION, utils_check_value_for_range(custom_fs_settings_get_device_setting(SETTING_USER_INTERACTION_TIMEOUT_ID), SETTING_MIN_USER_INTERACTION_TIMEOUT, SETTING_MAX_USER_INTERACTION_TIMOUT) << 10);
    #else
    timer_start_timer(TIMER_SCREEN, EMULATOR_SCREEN_TIMEOUT_MS);
    timer_start_timer(TIMER_USER_INTERACTION, SETTING_MAX_USER_INTERACTION_TIMOUT_EMU << 10);
    #endif
    
    /* Re-arm logoff timer if feature is enabled */
    uint16_t nb_minutes_before_lock_setting = custom_fs_settings_get_device_setting(SETTINGS_NB_MINUTES_FOR_LOCK);
    if ((nb_minutes_before_lock_setting != 0) && (logic_security_is_smc_inserted_unlocked() != FALSE))
    {
        timer_arm_inactivity_timer(nb_minutes_before_lock_setting);
    }
    
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
    
    /* Is screen saver running? */
    if (gui_dispatcher_is_screen_saver_running() != FALSE)
    {
        /* Get back to current screen */
        gui_dispatcher_get_back_to_current_screen();
        
        /* Stop screen saver if needed */
        gui_dispatcher_stop_screen_saver();
    }    
}

/*! \fn     logic_device_bundle_update_start(BOOL from_debug_messages, uint8_t* password)
*   \brief  Function called when start updating the device graphics memory
*   \param  from_debug_messages Set to TRUE if this function was called from debug messages
*   \param  password to start the bundle upload
*   \return RETURN_OK if we are allowed to start bundle update
*/
ret_type_te logic_device_bundle_update_start(BOOL from_debug_messages, uint8_t* password)
{
    logic_device_activity_detected();
    
    /* Function called from HID debug messages? */
#ifdef DEBUG_USB_COMMANDS_ENABLED
    if (from_debug_messages != FALSE)
    {
        gui_screen_te current_screen = gui_dispatcher_get_current_screen();
        
        /* Depending on the mode we're currently in */
        if (current_screen == GUI_SCREEN_INVALID)        
        {
            /* If we are in invalid screen (variable not set), it means we don't have a bundle */
            return RETURN_OK;
        }
        else if (current_screen == GUI_SCREEN_INSERTED_INVALID)
        {
            /* Card inserted invalid: allow update and display notification */
            gui_dispatcher_set_current_screen(GUI_SCREEN_FW_FILE_UPDATE, TRUE, GUI_OUTOF_MENU_TRANSITION);
            gui_dispatcher_get_back_to_current_screen();
            return RETURN_OK;
        }
        else
        {
            return RETURN_NOK;
        }
    } 
    else
#else
    (void)from_debug_messages;
#endif
    {
        /* Bruteforce delay */
        timer_delay_ms(2000 + rng_get_random_uint8_t());
        
        /* Get the current screen we're in */
        gui_screen_te current_screen = gui_dispatcher_get_current_screen();
        
        /* Bundle upload can be started in either of 2 conditions: no bundle detected at boot / invalid card inserted */
        if ((current_screen == GUI_SCREEN_INVALID) || (current_screen == GUI_SCREEN_INSERTED_INVALID))
        {
            br_aes_ct_ctrcbc_keys device_operations_aes_context;
            uint32_t password_buffer[AES_BLOCK_SIZE/8/sizeof(uint32_t)];
            uint8_t device_operations_aes_key[AES_KEY_LENGTH/8];
            uint8_t temp_ctr[AES256_CTR_LENGTH/8];
            
            /* Fetch device operations key */
            custom_fs_get_device_operations_aes_key(device_operations_aes_key);
            
            /* Bundle upload operations: we use the bundle version as counter, for the bytes 12-15 of the Big Endian CTR (byte 1 is set to the purpose value) */
            memset(temp_ctr, 0, sizeof(temp_ctr));
            temp_ctr[1] = BUNDLE_UPLOAD_CTR_B1_ID;
            utils_uint32_t_to_be_array(&temp_ctr[12], (uint32_t)custom_fs_get_platform_bundle_version());
            
            /* Initialize AES context */
            br_aes_ct_ctrcbc_init(&device_operations_aes_context, device_operations_aes_key, AES_KEY_LENGTH/8);
            
            /* Prepare what we want to encrypt: bundle version + device SN */
            _Static_assert(sizeof(password_buffer) == (AES_BLOCK_SIZE/8), "Invalid buffer size");
            memset(password_buffer, 0, sizeof(password_buffer));
            password_buffer[0] = custom_fs_get_platform_bundle_version();
            password_buffer[1] = custom_fs_get_platform_internal_serial_number();
            br_aes_ct_ctrcbc_ctr(&device_operations_aes_context, (void*)temp_ctr, password_buffer, sizeof(password_buffer));
            
            /* We have the data we want to compare to, memset everything */
            memset(&device_operations_aes_context, 0, sizeof(device_operations_aes_context));
            memset(device_operations_aes_key, 0, sizeof(device_operations_aes_key));
            memset(temp_ctr, 0, sizeof(temp_ctr));
            
            /* Check for match */
            if (utils_side_channel_safe_memcmp((uint8_t*)password_buffer, password, sizeof(password_buffer)) != 0)
            {
                memset(password_buffer, 0, sizeof(password_buffer));
                return RETURN_NOK;
            }
            
            /* Proactively set the bootloader flag to force actions at next boot */
            custom_fs_set_device_flag_value(DEVICE_WENT_THROUGH_BOOTLOADER_FLAG_ID, TRUE);

            #ifdef PLAT_V7_SETUP
            /* Reset successful update flag, used to specify that the bundle is OK */
            custom_fs_set_device_flag_value(SUCCESSFUL_UPDATE_FLAG_ID, FALSE);
            #endif
            
            /* Display notification if invalid card inserted */
            if (current_screen == GUI_SCREEN_INSERTED_INVALID)
            {
                gui_dispatcher_set_current_screen(GUI_SCREEN_FW_FILE_UPDATE, TRUE, GUI_OUTOF_MENU_TRANSITION);
                gui_dispatcher_get_back_to_current_screen();
            }
            
            return RETURN_OK;
        }
        else
        {
            return RETURN_NOK;
        }
        
        return RETURN_OK;
    }
}

/*! \fn     logic_device_bundle_update_end(BOOL from_debug_messages)
*   \brief  Function called at the end of graphics upload
*   \param  from_debug_messages Set to TRUE if this function was called from debug messages
*/
void logic_device_bundle_update_end(BOOL from_debug_messages)
{  
#ifdef DEBUG_USB_COMMANDS_ENABLED
    if (from_debug_messages != FALSE)
    {        
        /* Refresh file system and font */
        custom_fs_init();
        
        /* Go to default screen */
        gui_dispatcher_set_current_screen(GUI_SCREEN_NINSERTED, TRUE, GUI_OUTOF_MENU_TRANSITION);
        gui_dispatcher_get_back_to_current_screen();
    } 
    else
#else
    (void)from_debug_messages;
#endif
    {
        /* Go to default screen, only used to send an updated status to AUX */
        gui_dispatcher_set_current_screen(GUI_SCREEN_NINSERTED, TRUE, GUI_OUTOF_MENU_TRANSITION);
        
        /* Push the updated status to aux MCU */
        comms_aux_mcu_update_device_status_buffer();
        timer_delay_ms(10);
        
        /* Detach from USB to get a free bus */
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_DETACH_USB);
        comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_USB_DETACHED);
            
        /* Disable bluetooth if enabled */
        logic_aux_mcu_disable_ble(TRUE);
            
        /* Devices not using signed bootloader: directly flash the AUX */
        #if !defined(PLAT_V7_SETUP)
        logic_aux_mcu_flash_firmware_update(FALSE);
        #endif
            
        /* Then move on to main */
        custom_fs_set_settings_value(SETTINGS_DEVICE_TUTORIAL, TRUE);
        custom_fs_settings_set_fw_upgrade_flag();
        main_reboot();
    }
}

/*! \fn     logic_device_set_state_changed(void)
*   \brief  Function called whenever the device state has changed
*/
void logic_device_set_state_changed(void)
{
    logic_device_state_changed = TRUE;
    
    /* Invalidate preferred starting login */
    logic_user_invalidate_preferred_starting_service();
}


/*! \fn     logic_device_get_state_changed_and_reset_bool(void)
*   \brief  Fetch and reset state changed bool
*/
BOOL logic_device_get_state_changed_and_reset_bool(void)
{
    BOOL return_val = logic_device_state_changed;
    logic_device_state_changed = FALSE;
    return return_val;
}