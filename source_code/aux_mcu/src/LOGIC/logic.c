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
/* Device information for bluetooth */
dis_device_information_t logic_dis_device_information;


void logic_set_bluetooth_to_be_enabled(dis_device_information_t* device_information_pt)
{
    memcpy(&logic_dis_device_information, device_information_pt, sizeof(logic_dis_device_information));
    logic_dis_device_information.custom_device_name[MEMBER_ARRAY_SIZE(dis_device_information_t,custom_device_name)-1] = 0;
    logic_bluetooth_to_be_enabled = TRUE;
}

BOOL logic_get_and_clear_bluetooth_to_be_enabled(dis_device_information_t** dis_device_info_pt_pt)
{
    BOOL return_value = logic_bluetooth_to_be_enabled;
    *dis_device_info_pt_pt = &logic_dis_device_information;
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