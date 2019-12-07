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
#include "logic_bluetooth.h"
#include "logic_aux_mcu.h"
// Connected boolean
BOOL logic_bluetooth_is_connected = FALSE;


/*! \fn     logic_bluetooth_set_connected_state(BOOL state)
*   \brief  Set connected state
*   \param  state   Connected state
*/
void logic_bluetooth_set_connected_state(BOOL state)
{
    logic_bluetooth_is_connected = state;
}

/*! \fn     logic_bluetooth_get_state(void)
*   \brief  Get current bluetooth state
*   \return Current bluetooth state (see enum)
*/
bt_state_te logic_bluetooth_get_state(void)
{
    return BT_STATE_CONNECTED;
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
