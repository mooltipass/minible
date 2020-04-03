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
/*!  \file     logic_security.c
*    \brief    General logic for security (credentials etc)
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "logic_bluetooth.h"
#include "logic_security.h"
#include "logic_aux_mcu.h"
/* Inserted card unlocked */
volatile BOOL logic_security_smartcard_inserted_unlocked = FALSE;
/* Memory management mode */
volatile BOOL logic_security_management_mode = FALSE;
BOOL logic_security_management_usb_con_on_enter = FALSE;
bt_state_te logic_security_management_ble_con_on_enter = FALSE;


/*! \fn     logic_security_clear_security_bools(void)
*   \brief  Clear all security booleans
*/
void logic_security_clear_security_bools(void)
{
    logic_security_smartcard_inserted_unlocked = FALSE;
    logic_security_management_mode = FALSE;
    // TODO2
    /*
    context_valid_flag = FALSE;
    selected_login_flag = FALSE;
    login_just_added_flag = FALSE;
    leaveMemoryManagementMode();
    data_context_valid_flag = FALSE;
    current_adding_data_flag = FALSE;
    activateTimer(TIMER_CREDENTIALS, 0);
    smartcard_inserted_unlocked = FALSE;
    currently_writing_first_block = FALSE;
    */
}

/*! \fn     logic_security_smartcard_unlocked_actions(void)
*   \brief  Actions when smartcard is unlocked
*/
void logic_security_smartcard_unlocked_actions(void)
{    
    cpu_irq_enter_critical();
    logic_security_smartcard_inserted_unlocked = TRUE;
    logic_security_management_mode = FALSE;
    cpu_irq_leave_critical();
}

/*! \fn     logic_security_is_smc_inserted_unlocked(void)
*   \brief  Know if inserted smartcard is unlocked
*   \return The boolean
*/
BOOL logic_security_is_smc_inserted_unlocked(void)
{
    return logic_security_smartcard_inserted_unlocked;
}

/*! \fn     logic_security_set_management_mode(void)
*   \brief  Set device into management mode
*/
void logic_security_set_management_mode(void)
{
    logic_security_management_mode = TRUE;
    logic_security_management_usb_con_on_enter = logic_aux_mcu_is_usb_enumerated();
    logic_security_management_ble_con_on_enter = logic_bluetooth_get_state();
}

/*! \fn     logic_security_should_leave_management_mode(void)
*   \brief  Know if we should leave management mode
*/
BOOL logic_security_should_leave_management_mode(void)
{
    if ((logic_security_management_mode != FALSE) && ((logic_security_management_usb_con_on_enter != logic_aux_mcu_is_usb_enumerated()) || (logic_security_management_ble_con_on_enter != logic_bluetooth_get_state())))
    {
        return TRUE;
    } 
    else
    {
        return FALSE;
    }
}

/*! \fn     logic_security_clear_management_mode(void)
*   \brief  Exit management mode
*/
void logic_security_clear_management_mode(void)
{
    logic_security_management_mode = FALSE;
}

/*! \fn     logic_security_is_management_mode_set(void)
*   \brief  Check if device is in MMM
*   \return The boolean
*/
BOOL logic_security_is_management_mode_set(void)
{
    return logic_security_management_mode;
}
