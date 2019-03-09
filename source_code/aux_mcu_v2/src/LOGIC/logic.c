/*!  \file     logic.c
*    \brief    General logic handling
*    Created:  06/09/2018
*    Author:   Mathieu Stephan
*/
#include "defines.h"
#include "logic.h"
/* Boolean to know if BLE is enabled */
BOOL logic_ble_enabled = FALSE;
/* If no comms signal unavailable */
BOOL logic_no_comms_unavailable = FALSE;


void logic_set_ble_enabled(void)
{
    logic_ble_enabled = TRUE;
}

void logic_set_ble_disabled(void)
{
    logic_ble_enabled = FALSE;
}

BOOL logic_is_ble_enabled(void)
{
    return logic_ble_enabled;
}

void logic_set_nocomms_unavailable(void)
{
    logic_no_comms_unavailable = TRUE;
}

BOOL logic_is_no_comms_unavailable(void)
{
    return logic_no_comms_unavailable;
}