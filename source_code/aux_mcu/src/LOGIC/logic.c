/*!  \file     logic.c
*    \brief    General logic handling
*    Created:  06/09/2018
*    Author:   Mathieu Stephan
*/
#include "comms_main_mcu.h"
#include "defines.h"
#include "logic.h"
/* Boolean to know if BLE is enabled */
BOOL logic_ble_enabled = FALSE;
/* If no comms signal unavailable */
BOOL logic_no_comms_unavailable = FALSE;
/* Flags to enable bluetooth */
BOOL logic_bluetooth_to_be_enabled = FALSE;
uint8_t logic_bluetooth_mac_addr[6];


void logic_set_bluetooth_to_be_enabled(uint8_t* mac_addr)
{
    memcpy(logic_bluetooth_mac_addr, mac_addr, sizeof(logic_bluetooth_mac_addr));
    logic_bluetooth_to_be_enabled = TRUE;
}

BOOL logic_get_and_clear_bluetooth_to_be_enabled(uint8_t** mac_address_pt_pt)
{
    BOOL return_value = logic_bluetooth_to_be_enabled;
    *mac_address_pt_pt = logic_bluetooth_mac_addr;
    logic_bluetooth_to_be_enabled = FALSE;
    return return_value;
}

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