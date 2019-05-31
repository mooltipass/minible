/*!  \file     logic_bluetooth.h
*    \brief    General logic for bluetooth
*    Created:  07/03/2019
*    Author:   Mathieu Stephan
*/
#include "logic_bluetooth.h"
#include "logic_aux_mcu.h"


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
        return BT_STATE_ON;
    }
}
