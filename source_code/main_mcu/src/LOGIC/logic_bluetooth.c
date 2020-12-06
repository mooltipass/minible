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
/*!  \file     logic_bluetooth.h
*    \brief    General logic for bluetooth
*    Created:  07/03/2019
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include "logic_bluetooth.h"
#include "logic_aux_mcu.h"
#include "custom_fs.h"
#include "rng.h"
// Connected boolean
BOOL logic_bluetooth_is_connected = FALSE;
// Flag to prevent device lock after disconnect
BOOL logic_bluetooth_prevent_dev_lock_after_disconnect = FALSE;
// Our guess at the platform the device is connected to
platform_type_te logic_bluetooth_connected_to_platform;
// To indicate that too many failed ble connections happened
BOOL logic_bluetooth_too_many_failed_connections_flag = FALSE;


/*! \fn     logic_bluetooth_set_too_many_failed_connections(void)
*   \brief  Inform of too many failed BLE connections
*/
void logic_bluetooth_set_too_many_failed_connections(void)
{
    logic_bluetooth_too_many_failed_connections_flag = TRUE;
}

/*! \fn     logic_bluetooth_get_and_clear_too_many_failed_connections(void)
*   \brief  Get and clear too many failed BLE connections flag
*   \return Too many failed connections flag
*/
BOOL logic_bluetooth_get_and_clear_too_many_failed_connections(void)
{
    BOOL return_val = logic_bluetooth_too_many_failed_connections_flag;
    logic_bluetooth_too_many_failed_connections_flag = FALSE;
    return return_val;
}

/*! \fn     logic_bluetooth_set_connected_state(BOOL state)
*   \brief  Set connected state
*   \param  state   Connected state
*/
void logic_bluetooth_set_connected_state(BOOL state)
{
    logic_bluetooth_is_connected = state;
    
    /* Eventually we want to know which platform we're connected to */
    logic_bluetooth_connected_to_platform = PLAT_WIN;
}

/*! \fn     logic_bluetooth_get_connected_to_platform_type(void)
*   \brief  Get the platform type we're connected to
*   \return The connected to platform type
*/
platform_type_te logic_bluetooth_get_connected_to_platform_type(void)
{
    return logic_bluetooth_connected_to_platform;
}

/*! \fn     logic_bluetooth_set_do_not_lock_device_after_disconnect_flag(BOOL flag)
*   \brief  Set flag allowing to prevent device lock on bluetooth disconnect
*   \param  flag    Flag value
*/
void logic_bluetooth_set_do_not_lock_device_after_disconnect_flag(BOOL flag)
{
    logic_bluetooth_prevent_dev_lock_after_disconnect = flag;
}

/*! \fn     logic_bluetooth_get_do_not_lock_device_after_disconnect_flag(void)
*   \brief  Get flag allowing to prevent device lock on bluetooth disconnect
*   \return The flag
*/
BOOL logic_bluetooth_get_do_not_lock_device_after_disconnect_flag(void)
{
    return logic_bluetooth_prevent_dev_lock_after_disconnect;
}

/*! \fn     logic_bluetooth_get_state(void)
*   \brief  Get current bluetooth state
*   \return Current bluetooth state (see enum)
*/
bt_state_te logic_bluetooth_get_state(void)
{
    if (logic_aux_mcu_is_ble_enabled() == FALSE)
    {
        return BT_STATE_OFF;
    } 
    else
    {
        if (logic_bluetooth_is_connected == FALSE)
        {
            return BT_STATE_ON;
        } 
        else
        {
            return BT_STATE_CONNECTED;
        }
    }
}

/*! \fn     logic_bluetooth_get_unit_mac_address(uint8_t* buffer)
*   \brief  Get our device Bluetooth MAC address
*   \buffer 6 bytes buffer to store MAC address
*/
void logic_bluetooth_get_unit_mac_address(uint8_t* buffer)
{
    /* Try to get platform programmed MAC address */
    if (custom_fs_get_platform_ble_mac_addr(buffer) != RETURN_OK)
    {
        /* In case of fail, take the debug one */
        custom_fs_get_debug_bt_addr(buffer);
    }
}
