/*
 * logic_bluetooth.c
 *
 * Created: 02/06/2019 18:08:22
 *  Author: limpkin
 */ 
#include "platform_defines.h"
#include "logic_bluetooth.h"
#include "conf_serialdrv.h"
#include "driver_timer.h"
#include "device_info.h"
#include "ble_manager.h"
#include "platform_io.h"
#include "logic_sleep.h"
#include "at_ble_api.h"
#include "logic_rng.h"
#include "ble_utils.h"
#include "platform.h"
#include "battery.h"
#include "defines.h"
#include "logic.h"
// Appearance: 0x03C1 (keyboard)
// Dev name perm: disable writing device name
// Device name: Mooltipass Mini BLE
// Minimum connection interval N (Value Time = N *1.25 ms): 9 = 11.25ms
// Maximum connection interval N (Value Time = N *1.25 ms): 24 = 30ms
// Slave preferred Connection latency (number of events): 30 (taken from ios guidelines)
// Slave preferred Link supervision time-out (Value Time = N * 10 ms): 300 (taken from ios guidelines)
at_ble_gap_deviceinfo_t logic_bluetooth_advanced_info = {.appearance=0x03C1, .dev_name_perm=AT_BLE_WRITE_DISABLE, .name.length=20, .name.value="Mooltipass Mini BLE", .slv_params.con_intv_min=9, .slv_params.con_intv_max=24, .slv_params.con_latency=30, .slv_params.superv_to=300};
/* Control point notification structure for HID services*/
hid_control_mode_ntf_t logic_bluetooth_hid_control_point_value[HID_MAX_SERV_INST];
/* Characteristic numbers for the report CCD notifications */
uint8_t logic_bluetooth_report_notif_characteristic_numbers_init_values[BLE_TOTAL_NUMBER_OF_REPORTS];
uint16_t* logic_bluetooth_report_notif_characteristic_numbers[BLE_TOTAL_NUMBER_OF_REPORTS];
uint16_t logic_bluetooth_report_notif_characteristic_numbers_counter = 0;
/* Report notification structure for HID services */
hid_report_ntf_t logic_bluetooth_report_ntf_info[BLE_TOTAL_NUMBER_OF_REPORTS];
/* HID GATT services instances */
hid_gatt_serv_handler_t logic_bluetooth_hid_gatt_instances[HID_MAX_SERV_INST];
/* Notification we're sending */
notif_sending_te logic_bluetooth_notif_being_sent = NONE_NOTIF_SENDING;
/* HID service instances */
hid_serv_t logic_bluetooth_hid_serv_instances[HID_MAX_SERV_INST];
/* Boot notification structure for keyboard service in boot protocol */
hid_boot_ntf_t logic_bluetooth_boot_keyboard_report_ntf_info;
/* Protocol mode notification structure for keyboard service */
hid_proto_mode_ntf_t logic_bluetooth_protocol_mode_value;
/* Battery service handler */
bat_gatt_service_handler_t logic_bluetooth_bas_service_handler;
/* Notification sent flag for battery char changed */
BOOL logic_bluetooth_battery_notification_flag = TRUE;
/* Bluetooth connection handle */
at_ble_handle_t logic_bluetooth_ble_connection_handle;
/* Battery level advertised */
uint8_t logic_bluetooth_pending_battery_level = UINT8_MAX;
uint8_t logic_bluetooth_ble_battery_notif_char = 1;
uint8_t logic_bluetooth_ble_battery_level = 33;
/* Array of pointers to keyboard & raw hid profiles */
hid_prf_info_t* hid_prf_dataref[HID_MAX_SERV_INST];
/* HID profile structure for raw hid profile */
hid_prf_info_t logic_bluetooth_raw_hid_prf_data;
/* HID profile structure for keyboard profile */
hid_prf_info_t logic_bluetooth_hid_prf_data;
/* HID reports for RAW HID interface */
uint8_t logic_bluetooth_raw_hid_data_out_buf[64];
uint8_t logic_bluetooth_raw_hid_data_in_buf[64];
/* Control points and reports for HID keyboard */
uint8_t logic_bluetooth_boot_mouse_in_report[1];
uint8_t logic_bluetooth_boot_keyb_out_report[1];
uint8_t logic_bluetooth_boot_keyb_in_report[1];
uint8_t logic_bluetooth_keyboard_in_report[8];
uint8_t logic_bluetooth_mouse_in_report[4];
uint8_t logic_bluetooth_ctrl_point[1];
BOOL logic_bluetooth_typed_report_sent = FALSE;
/* Bluetooth connection bools */
BOOL logic_bluetooth_can_communicate_with_host_prev = FALSE;
BOOL logic_bluetooth_notifications_just_enabled = FALSE;
BOOL logic_bluetooth_can_communicate_with_host = FALSE;
BOOL logic_bluetooth_open_to_pairing = FALSE;
BOOL logic_bluetooth_advertising = FALSE;
BOOL logic_bluetooth_just_connected = FALSE;
BOOL logic_bluetooth_just_paired = FALSE;
BOOL logic_bluetooth_connected = FALSE;
BOOL logic_bluetooth_paired = FALSE;
BOOL logic_bluetooth_disable_flag = FALSE;
/* Bool to specify if we setup the callbacks */
BOOL logic_bluetooth_callbacks_set = FALSE;
/* Banning timeouts */
uint16_t logic_bluetooth_last_temp_banned_mac_entered_index;
uint8_t logic_bluetooth_temp_banned_mac[3][6];
uint16_t logic_bluetooth_banning_timeouts[3];
/* Logic to know if some device is trying to connect to us too often */
BOOL logic_bluetooth_too_many_invalid_connect_notif_sent = FALSE;
uint8_t logic_bluetooth_invalid_connect_counter = 0;


static at_ble_status_t hid_custom_event(void *param)
{
    at_ble_status_t status = AT_BLE_SUCCESS;
    return status;
}

/*! \fn     logic_bluetooth_ms_tick(void)
*   \brief  Function called every ms
*/
void logic_bluetooth_ms_tick(void)
{
    /* Decrease banning timeouts */
    for (uint16_t i = 0; i < ARRAY_SIZE(logic_bluetooth_banning_timeouts); i++)
    {
        if (logic_bluetooth_banning_timeouts[i] != 0)
        {
            logic_bluetooth_banning_timeouts[i]--;
        }
    }
}

/*! \fn     logic_bluetooth_denied_connection_trigger(void)
*   \brief  Called whenever a device tried to connect to the mini BLE and we didn't have the keys
*/
void logic_bluetooth_denied_connection_trigger(void)
{
    if (++logic_bluetooth_invalid_connect_counter == 0)
    {
        /* Use wrapover to our advantage */
        logic_bluetooth_too_many_invalid_connect_notif_sent = FALSE;
    }
}

/*! \fn     logic_bluetooth_set_disable_flag(void)
*   \brief  Set flag to disable bluetooth
*/
void logic_bluetooth_set_disable_flag(void)
{
    logic_bluetooth_disable_flag = TRUE;
}

/*! \fn     logic_bluetooth_store_temp_ban_connected_address(uint8_t* address)
*   \brief  Store provided address for potential upcoming ban
*/
void logic_bluetooth_store_temp_ban_connected_address(uint8_t* address)
{
    cpu_irq_enter_critical();
    /* Find an empty slot to store the mac address */
    for (uint16_t i = 0; i < ARRAY_SIZE(logic_bluetooth_banning_timeouts); i++)
    {
        if (logic_bluetooth_banning_timeouts[i] == 0)
        {
            /* Store last temp banned index */
            logic_bluetooth_last_temp_banned_mac_entered_index = i;
            
            /* Copy MAC into temp ban buffer */
            memcpy(logic_bluetooth_temp_banned_mac[i], address, sizeof(logic_bluetooth_temp_banned_mac[0]));
        }
    }  
    cpu_irq_leave_critical();
}

/*! \fn     logic_bluetooth_temporarily_ban_connected_device(void)
*   \brief  Temporarily ban currently connected device
*   \return RETURN_OK if we were actually connected to a device and if we setup the temp ban
*/
RET_TYPE logic_bluetooth_temporarily_ban_connected_device(void)
{
    /* Mac is filled when device is connected */
    if ((logic_is_ble_enabled() != FALSE) && (logic_bluetooth_can_communicate_with_host != FALSE) && (logic_bluetooth_last_temp_banned_mac_entered_index < ARRAY_SIZE(logic_bluetooth_banning_timeouts)))
    {
        /* Temp ban for 34 seconds */
        logic_bluetooth_banning_timeouts[logic_bluetooth_last_temp_banned_mac_entered_index] = 45678;
        ble_disconnect_all_devices();
        return RETURN_OK;
    }
    else
    {
        return RETURN_NOK;
    }
}

/*! \fn     logic_bluetooth_is_device_temp_banned(uint8_t* mac)
*   \brief  Know if a given mac is temporarily banned
*   \param  mac     MAC address for checks
*   \return TRUE or false
*/
BOOL logic_bluetooth_is_device_temp_banned(uint8_t* mac)
{
    BOOL return_val = FALSE;
    
    /* Check all slots */
    cpu_irq_enter_critical();
    for (uint16_t i = 0; i < ARRAY_SIZE(logic_bluetooth_banning_timeouts); i++)
    {
        if ((logic_bluetooth_banning_timeouts[i] != 0) && (memcmp(logic_bluetooth_temp_banned_mac[i], mac, sizeof(logic_bluetooth_temp_banned_mac[0])) == 0))
        {
            return_val = TRUE;
        }
    }
    cpu_irq_leave_critical();
    
    return return_val;
}

/*! \fn     logic_bluetooth_get_open_to_pairing(void)
*   \brief  Know if we are open for pairing
*   \return The boolean
*/
BOOL logic_bluetooth_get_open_to_pairing(void)
{
    return logic_bluetooth_open_to_pairing;
}

/*! \fn     logic_bluetooth_set_open_to_pairing_bool(BOOL pairing_bool)
*   \brief  Specify if we are open for pairing
*   \param  pairing_bool The boolean
*/
void logic_bluetooth_set_open_to_pairing_bool(BOOL pairing_bool)
{
    logic_bluetooth_open_to_pairing = pairing_bool;
}

/*! \fn     logic_bluetooth_hid_connected_callback(void* params)
*   \brief  Called during device connection
*/
static at_ble_status_t logic_bluetooth_hid_connected_callback(void* params)
{
    DBG_LOG("Connected to device");
    
    /* Set booleans */
    logic_bluetooth_just_connected = TRUE;
    logic_bluetooth_advertising = FALSE;
    logic_bluetooth_connected = TRUE;
    
    /* Store connection handle */
    at_ble_connected_t* connected = (at_ble_connected_t*)params;
    logic_bluetooth_ble_connection_handle = connected->handle;

    return AT_BLE_SUCCESS;
}

/*! \fn     logic_bluetooth_check_and_wait_for_notif_sent(void)
*   \brief  Check if a notification if being sent and wait for end
*/
void logic_bluetooth_check_and_wait_for_notif_sent(void)
{
    while (logic_bluetooth_notif_being_sent != NONE_NOTIF_SENDING)
    {
        ble_event_task();
    }
}

/*! \fn     logic_bluetooth_hid_disconnected_callback(void* params)
*   \brief  Called during device disconnection
*/
static at_ble_status_t logic_bluetooth_hid_disconnected_callback(void *params)
{
    DBG_LOG("Disconnected from device");
    
    /* Callback in case we're waiting for a packet send */
    comms_raw_hid_send_callback(BLE_INTERFACE);

    /* From battery service */
    logic_bluetooth_battery_notification_flag = TRUE;
    
    /* Inform main MCU */
    if (logic_bluetooth_can_communicate_with_host != FALSE)
    {
        comms_main_mcu_send_simple_event(AUX_MCU_EVENT_BLE_DISCONNECTED);
    }
    
    /* Reset booleans */
    logic_bluetooth_notif_being_sent = NONE_NOTIF_SENDING;
    logic_bluetooth_can_communicate_with_host = FALSE;
    logic_bluetooth_just_connected = FALSE;
    logic_bluetooth_just_paired = FALSE;
    logic_bluetooth_connected = FALSE;
    logic_bluetooth_paired = FALSE;
    
    /* Start advertising again */
    logic_bluetooth_start_advertising();

    return AT_BLE_SUCCESS;
}

/*! \fn     logic_bluetooth_clear_bonding_information(void)
*   \brief  Called to clear all bonding information and disconnect any potential devices
*/
void logic_bluetooth_clear_bonding_information(void)
{
    if (logic_is_ble_enabled() == FALSE)
    {
        ble_clear_bond_info();
    } 
    else
    {
        ble_disconnect_all_devices();
        ble_clear_bond_info();
    }
}

/*! \fn     logic_bluetooth_successfull_pairing_call(ble_connected_dev_info_t* dev_info, at_ble_connected_t* connected_info)
*   \brief  Called during device pairing
*   \param  dev_info        Device information pointer
*   \param  connected_info  Connection information pointer
*/
void logic_bluetooth_successfull_pairing_call(ble_connected_dev_info_t* dev_info, at_ble_connected_t* connected_info)
{
    aux_mcu_message_t* temp_tx_message_pt;
    DBG_LOG("Paired to device");
        
    /* Set booleans */
    logic_bluetooth_too_many_invalid_connect_notif_sent = FALSE;
    logic_bluetooth_notif_being_sent = NONE_NOTIF_SENDING;
    logic_bluetooth_can_communicate_with_host = TRUE;
    logic_bluetooth_invalid_connect_counter = 0;
    logic_bluetooth_just_paired = TRUE;
    logic_bluetooth_paired = TRUE;
    
    /* Store ban info if IRK address */
    if (connected_info->peer_addr.type != AT_BLE_ADDRESS_PUBLIC)
    {
        logic_bluetooth_store_temp_ban_connected_address(dev_info->bond_info.peer_irk.addr.addr);
    }
        
    /* Inform main MCU */
    comms_main_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_BLE_CMD);
        
    /* Set payload size */
    temp_tx_message_pt->payload_length1 = sizeof(temp_tx_message_pt->ble_message.message_id) + sizeof(temp_tx_message_pt->ble_message.bonding_information_to_store_message);
        
    /* Message ID */
    temp_tx_message_pt->ble_message.message_id = BLE_MESSAGE_STORE_BOND_INFO;
        
    /***********************/
    /* Bonding information */
    /***********************/
        
    /* General stuff */
    temp_tx_message_pt->ble_message.bonding_information_to_store_message.zero_to_be_valid = 0;
    temp_tx_message_pt->ble_message.bonding_information_to_store_message.address_resolv_type = dev_info->conn_info.peer_addr.type;
    memcpy(temp_tx_message_pt->ble_message.bonding_information_to_store_message.mac_address, dev_info->conn_info.peer_addr.addr, sizeof(dev_info->conn_info.peer_addr.addr));
    temp_tx_message_pt->ble_message.bonding_information_to_store_message.auth_type = dev_info->bond_info.auth;
        
    /* Peer LTK */
    memcpy(temp_tx_message_pt->ble_message.bonding_information_to_store_message.peer_ltk_key, dev_info->bond_info.peer_ltk.key, sizeof(dev_info->bond_info.peer_ltk.key));
    temp_tx_message_pt->ble_message.bonding_information_to_store_message.peer_ltk_ediv = dev_info->bond_info.peer_ltk.ediv;
    memcpy(temp_tx_message_pt->ble_message.bonding_information_to_store_message.peer_ltk_random_nb, dev_info->bond_info.peer_ltk.nb, sizeof(dev_info->bond_info.peer_ltk.nb));
    temp_tx_message_pt->ble_message.bonding_information_to_store_message.peer_ltk_key_size = dev_info->bond_info.peer_ltk.key_size;
        
    /* CSRK */
    memcpy(temp_tx_message_pt->ble_message.bonding_information_to_store_message.peer_csrk_key, dev_info->bond_info.peer_csrk.key, sizeof(dev_info->bond_info.peer_csrk.key));
        
    /* IRK */
    memcpy(temp_tx_message_pt->ble_message.bonding_information_to_store_message.peer_irk_key, dev_info->bond_info.peer_irk.key, sizeof(dev_info->bond_info.peer_irk.key));
    temp_tx_message_pt->ble_message.bonding_information_to_store_message.peer_irk_resolv_type = dev_info->bond_info.peer_irk.addr.type;
    memcpy(temp_tx_message_pt->ble_message.bonding_information_to_store_message.peer_irk_address, dev_info->bond_info.peer_irk.addr.addr, sizeof(dev_info->bond_info.peer_irk.addr.addr));
    
    /* Host LTK & CSRK */
    memcpy(temp_tx_message_pt->ble_message.bonding_information_to_store_message.host_ltk_key, dev_info->host_ltk.key, sizeof(dev_info->host_ltk.key));
    temp_tx_message_pt->ble_message.bonding_information_to_store_message.host_ltk_ediv = dev_info->host_ltk.ediv;
    memcpy(temp_tx_message_pt->ble_message.bonding_information_to_store_message.host_ltk_random_nb, dev_info->host_ltk.nb, sizeof(dev_info->host_ltk.nb));
    temp_tx_message_pt->ble_message.bonding_information_to_store_message.host_ltk_key_size = dev_info->host_ltk.key_size;
    memcpy(temp_tx_message_pt->ble_message.bonding_information_to_store_message.host_csrk_key, dev_info->host_csrk.key, sizeof(dev_info->host_csrk.key));
    
    /* Zero stuff */
    memset(temp_tx_message_pt->ble_message.bonding_information_to_store_message.reserved, 0, sizeof(temp_tx_message_pt->ble_message.bonding_information_to_store_message.reserved));
        
    /* Send packet */
    comms_main_mcu_send_message((void*)temp_tx_message_pt, (uint16_t)sizeof(aux_mcu_message_t));
    
    switch (dev_info->bond_info.auth)
    {
        case AT_BLE_AUTH_NO_MITM_NO_BOND: DBG_LOG("No Man In The Middle protection(MITM) , No Bonding");break;
        case AT_BLE_AUTH_NO_MITM_BOND: DBG_LOG("No MITM, Bonding");break;
        case AT_BLE_AUTH_MITM_NO_BOND: DBG_LOG("MITM, No Bonding");break;
        case AT_BLE_AUTH_MITM_BOND: DBG_LOG("MITM and Bonding");break;
    	default: break;
    }
    
    #ifndef DEBUG_LOG_DISABLED
    DBG_LOG("Host LTK Key (%dB): %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", dev_info->host_ltk.key_size,dev_info->host_ltk.key[0],dev_info->host_ltk.key[1],dev_info->host_ltk.key[2],dev_info->host_ltk.key[3],dev_info->host_ltk.key[4],dev_info->host_ltk.key[5],dev_info->host_ltk.key[6],dev_info->host_ltk.key[7],dev_info->host_ltk.key[8],dev_info->host_ltk.key[9],dev_info->host_ltk.key[10],dev_info->host_ltk.key[11],dev_info->host_ltk.key[12],dev_info->host_ltk.key[13],dev_info->host_ltk.key[14],dev_info->host_ltk.key[15]);
    DBG_LOG("LTK ediv: %04x, RNG: %02x%02x%02x%02x%02x%02x%02x%02x", dev_info->host_ltk.ediv, dev_info->host_ltk.nb[0],dev_info->host_ltk.nb[1],dev_info->host_ltk.nb[2],dev_info->host_ltk.nb[3],dev_info->host_ltk.nb[4],dev_info->host_ltk.nb[5],dev_info->host_ltk.nb[6],dev_info->host_ltk.nb[7]);
    DBG_LOG("Peer LTK Key (%dB): %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", dev_info->bond_info.peer_ltk.key_size,dev_info->bond_info.peer_ltk.key[0],dev_info->bond_info.peer_ltk.key[1],dev_info->bond_info.peer_ltk.key[2],dev_info->bond_info.peer_ltk.key[3],dev_info->bond_info.peer_ltk.key[4],dev_info->bond_info.peer_ltk.key[5],dev_info->bond_info.peer_ltk.key[6],dev_info->bond_info.peer_ltk.key[7],dev_info->bond_info.peer_ltk.key[8],dev_info->bond_info.peer_ltk.key[9],dev_info->bond_info.peer_ltk.key[10],dev_info->bond_info.peer_ltk.key[11],dev_info->bond_info.peer_ltk.key[12],dev_info->bond_info.peer_ltk.key[13],dev_info->bond_info.peer_ltk.key[14],dev_info->bond_info.peer_ltk.key[15]);
    DBG_LOG("LTK ediv: %04x, RNG: %02x%02x%02x%02x%02x%02x%02x%02x", dev_info->bond_info.peer_ltk.ediv, dev_info->bond_info.peer_ltk.nb[0],dev_info->bond_info.peer_ltk.nb[1],dev_info->bond_info.peer_ltk.nb[2],dev_info->bond_info.peer_ltk.nb[3],dev_info->bond_info.peer_ltk.nb[4],dev_info->bond_info.peer_ltk.nb[5],dev_info->bond_info.peer_ltk.nb[6],dev_info->bond_info.peer_ltk.nb[7]);
    DBG_LOG("CSRK: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",dev_info->bond_info.peer_csrk.key[0],dev_info->bond_info.peer_csrk.key[1],dev_info->bond_info.peer_csrk.key[2],dev_info->bond_info.peer_csrk.key[3],dev_info->bond_info.peer_csrk.key[4],dev_info->bond_info.peer_csrk.key[5],dev_info->bond_info.peer_csrk.key[6],dev_info->bond_info.peer_csrk.key[7],dev_info->bond_info.peer_csrk.key[8],dev_info->bond_info.peer_csrk.key[9],dev_info->bond_info.peer_csrk.key[10],dev_info->bond_info.peer_csrk.key[11],dev_info->bond_info.peer_csrk.key[12],dev_info->bond_info.peer_csrk.key[13],dev_info->bond_info.peer_csrk.key[14],dev_info->bond_info.peer_csrk.key[15]);
    DBG_LOG("IRK: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",dev_info->bond_info.peer_irk.key[0],dev_info->bond_info.peer_irk.key[1],dev_info->bond_info.peer_irk.key[2],dev_info->bond_info.peer_irk.key[3],dev_info->bond_info.peer_irk.key[4],dev_info->bond_info.peer_irk.key[5],dev_info->bond_info.peer_irk.key[6],dev_info->bond_info.peer_irk.key[7],dev_info->bond_info.peer_irk.key[8],dev_info->bond_info.peer_irk.key[9],dev_info->bond_info.peer_irk.key[10],dev_info->bond_info.peer_irk.key[11],dev_info->bond_info.peer_irk.key[12],dev_info->bond_info.peer_irk.key[13],dev_info->bond_info.peer_irk.key[14],dev_info->bond_info.peer_irk.key[15]);
    #endif
    
    switch(dev_info->bond_info.peer_irk.addr.type)
    {
        case(AT_BLE_ADDRESS_PUBLIC): DBG_LOG("Public IRK addr");break;
        case(AT_BLE_ADDRESS_RANDOM_STATIC): DBG_LOG("Random static IRK addr");break;
        case(AT_BLE_ADDRESS_RANDOM_PRIVATE_RESOLVABLE): DBG_LOG("Random private resolvable IRK addr");break;
        case(AT_BLE_ADDRESS_RANDOM_PRIVATE_NON_RESOLVABLE): DBG_LOG("Random private non resolvable IRK addr");break;
        default: break;
    }
    DBG_LOG("IRK addr: %02x%02x%02x%02x%02x%02x",dev_info->bond_info.peer_irk.addr.addr[0],dev_info->bond_info.peer_irk.addr.addr[1],dev_info->bond_info.peer_irk.addr.addr[2],dev_info->bond_info.peer_irk.addr.addr[3],dev_info->bond_info.peer_irk.addr.addr[4],dev_info->bond_info.peer_irk.addr.addr[5]);
}

/*! \fn     logic_bluetooth_encryption_changed_success(uint8_t* mac)
*   \brief  Called after encryption changed success
*   \param  mac     Host MAC address (public or IRK)
*/
void logic_bluetooth_encryption_changed_success(uint8_t* mac)
{        
    /* Set boolean */
    logic_bluetooth_too_many_invalid_connect_notif_sent = FALSE;
    logic_bluetooth_notif_being_sent = NONE_NOTIF_SENDING;
    logic_bluetooth_can_communicate_with_host = TRUE;
    logic_bluetooth_invalid_connect_counter = 0;
}

/* Callbacks for GAP */
static const ble_gap_event_cb_t hid_app_gap_handle = 
{
    .connected = logic_bluetooth_hid_connected_callback,
    .disconnected = logic_bluetooth_hid_disconnected_callback,
    //.pair_done = logic_bluetooth_hid_paired_callback,
    //.encryption_status_changed = logic_bluetooth_encryption_changed_callback
};

/*! \fn     logic_bluetooth_notification_confirmed_callback(void* params)
*   \brief  Called when the notification has been confirmed by client
*/
static at_ble_status_t logic_bluetooth_notification_confirmed_callback(void* params)
{
    at_ble_cmd_complete_event_t* notification_status;
    notification_status = (at_ble_cmd_complete_event_t*) params;
    
    /* Debug */
    if(notification_status->status == AT_BLE_SUCCESS)
    {
        DBG_LOG("sending notification to the peer success");
    }
    else
    {
        DBG_LOG("ERROR: failed sending notification to peer");
    }
    
    if (logic_bluetooth_notif_being_sent == RAW_HID_NOTIF_SENDING)
    {
        comms_raw_hid_send_callback(BLE_INTERFACE);
    }
    else if (logic_bluetooth_notif_being_sent == KEYBOARD_NOTIF_SENDING)
    {
        logic_bluetooth_typed_report_sent = TRUE;
    }
    else if (logic_bluetooth_notif_being_sent == BATTERY_NOTIF_SENDING)
    {
        /* From battery service */
        if(notification_status->status == AT_BLE_SUCCESS)
        {
            logic_bluetooth_battery_notification_flag = TRUE;
        }
    }
    
    /* Reset flag */
    logic_bluetooth_notif_being_sent = NONE_NOTIF_SENDING;

    return AT_BLE_SUCCESS;
}

/*! \fn     logic_bluetooth_characteristic_changed_handler(void* params)
*   \brief  Service characteristic change handler function
*/
at_ble_status_t logic_bluetooth_characteristic_changed_handler(void* params)
{
    at_ble_characteristic_changed_t change_params;

    /* Copy locally the change params */
    memcpy((uint8_t*)&change_params, params, sizeof(at_ble_characteristic_changed_t));
    DBG_LOG("Characteristic %d changed", change_params.char_handle);
    
    /* Check for battery service */
    if(logic_bluetooth_bas_service_handler.serv_chars.client_config_handle == change_params.char_handle)
    {
        DBG_LOG("Characteristic belongs to battery service");
        at_ble_status_t status = bat_char_changed_event(&logic_bluetooth_bas_service_handler, (at_ble_characteristic_changed_t*)params);
        if(status == AT_BLE_SUCCESS)
        {
            logic_bluetooth_battery_notification_flag = TRUE;
            DBG_LOG("Battery service: notifications enabled");
        }
        else if (status == AT_BLE_PRF_NTF_DISABLED)
        {
            logic_bluetooth_battery_notification_flag = FALSE;
            DBG_LOG("Battery service: notifications disabled");
        }
        return status;
    }
    
    /* See if this is for the hid service, and if so get the service instance */
    uint8_t serv_inst = logic_bluetooth_get_hid_serv_instance(change_params.char_handle);
    
    /* If above function returns 0xFF... it means we don't know which service this characteristics belongs to */
    if (serv_inst == 0xFF)
    {
        DBG_LOG("ERROR: couldn't find service for this characteristic!!!!");
    }
    else
    {
        DBG_LOG("Characteristic belongs to HID service instance %d", serv_inst);
    }
    
    /* Get the kind of notification based on the characteristic handle */
    uint8_t ntf_op = logic_bluetooth_get_notif_instance(serv_inst, change_params.char_handle);
    
    switch(ntf_op)
    {
        case PROTOCOL_MODE:
        {
            logic_bluetooth_protocol_mode_value.serv_inst = serv_inst;
            logic_bluetooth_protocol_mode_value.mode = change_params.char_new_value[0];
            logic_bluetooth_protocol_mode_value.conn_handle = change_params.conn_handle;
            hid_prf_dataref[serv_inst]->protocol_mode = change_params.char_new_value[0];
            DBG_LOG("Protocol Mode Notification Callback : Service Instance %d  New Protocol Mode  %d  Connection Handle %d", serv_inst, logic_bluetooth_protocol_mode_value.mode, change_params.conn_handle);
        }
        break;
        
        case CHAR_REPORT:
        {
            DBG_LOG("Report Callback : Service Instance %d Bytes Received %d Connection Handle %d", serv_inst, change_params.char_len, change_params.conn_handle);
            DBG_LOG("BLE receive: %02x %02x %02x%02x %02x%02x", change_params.char_new_value[0], change_params.char_new_value[1], change_params.char_new_value[2], change_params.char_new_value[3], change_params.char_new_value[4], change_params.char_new_value[5]);
            if (serv_inst == BLE_RAW_HID_SERVICE_INSTANCE)
            {
                uint8_t* recv_buf = comms_raw_hid_get_recv_buffer(BLE_INTERFACE);
                memcpy(recv_buf, change_params.char_new_value, change_params.char_len);
                comms_raw_hid_recv_callback(BLE_INTERFACE, change_params.char_len);
            }
        }
        break;
        
        case CHAR_REPORT_CCD:
        {
            uint8_t report_id = logic_bluetooth_get_reportid(serv_inst, change_params.char_handle);
            if ((report_id-1) < BLE_TOTAL_NUMBER_OF_REPORTS)
            {
                logic_bluetooth_report_ntf_info[report_id-1].serv_inst = serv_inst;
                logic_bluetooth_report_ntf_info[report_id-1].report_ID = report_id;
                logic_bluetooth_report_ntf_info[report_id-1].conn_handle = change_params.conn_handle;
                logic_bluetooth_report_ntf_info[report_id-1].ntf_conf = (change_params.char_new_value[1]<<8 | change_params.char_new_value[0]);
                if (logic_bluetooth_report_ntf_info[report_id-1].ntf_conf != 0)
                {
                    logic_bluetooth_notifications_just_enabled = TRUE;
                }
                DBG_LOG("Report Notification Callback Service Instance %d  Report ID  %d  Notification(Enable/Disable) %d Connection Handle %d", serv_inst, report_id, logic_bluetooth_report_ntf_info[report_id-1].ntf_conf, change_params.conn_handle);
            }
        }
        break;
        
        case (BOOT_KEY_INPUT_REPORT):
        {
            DBG_LOG("BOOT_KEY_INPUT_REPORT callback... why is this here?");
        }
        break;

        case (BOOT_KEY_INPUT_CCD):
        {
            logic_bluetooth_boot_keyboard_report_ntf_info.serv_inst = serv_inst;
            logic_bluetooth_boot_keyboard_report_ntf_info.boot_value = ntf_op;
            logic_bluetooth_boot_keyboard_report_ntf_info.conn_handle = change_params.conn_handle;
            logic_bluetooth_boot_keyboard_report_ntf_info.ntf_conf = (change_params.char_new_value[1]<<8 | change_params.char_new_value[0]);
            DBG_LOG("Boot Notification Callback :: Service Instance %d  Boot Value  %d  Notification(Enable/Disable) %d", serv_inst, ntf_op, logic_bluetooth_boot_keyboard_report_ntf_info.ntf_conf);
        }
        break;

        case CONTROL_POINT:
        {
            logic_bluetooth_hid_control_point_value[serv_inst].serv_inst = serv_inst;
            logic_bluetooth_hid_control_point_value[serv_inst].control_value = (change_params.char_new_value[1]<<8 | change_params.char_new_value[0]);
            DBG_LOG("Control Point Notification Callback :: Service Instance %d Control Value %d", logic_bluetooth_hid_control_point_value[serv_inst].serv_inst, logic_bluetooth_hid_control_point_value[serv_inst].control_value);
        }
        break;

        default:
        {
            DBG_LOG("Unhandled Notification");
        }
        break;
    }
    return AT_BLE_SUCCESS;
}

/* GATT callbacks */
static const ble_gatt_server_event_cb_t hid_app_gatt_server_handle = 
{
    .notification_confirmed = logic_bluetooth_notification_confirmed_callback,
    .characteristic_changed = logic_bluetooth_characteristic_changed_handler
};

/* All BLE Manager Custom Event callback */
static const ble_custom_event_cb_t hid_custom_event_cb = 
{
    .custom_event = hid_custom_event
};

/* keyboard report */
static uint8_t logic_bluetooth_keyb_report_map[] =
{
    /* Want to have your keyboard working on iOS? you better include that mouse in it... because that makes sense... right? */
    0x05, 0x01,                         // Usage Page (Generic Desktop)
    0x09, 0x02,                         // Usage (Mouse)
    0xA1, 0x01,                         // Collection (Application)
    0x85, 0x01,                         // Report Id (1)
    0x09, 0x01,                         //   Usage (Pointer)
    0xA1, 0x00,                         //   Collection (Physical)
    0x05, 0x09,                         //     Usage Page (Buttons)
    0x19, 0x01,                         //     Usage Minimum (01) - Button 1
    0x29, 0x03,                         //     Usage Maximum (03) - Button 3
    0x15, 0x00,                         //     Logical Minimum (0)
    0x25, 0x01,                         //     Logical Maximum (1)
    0x75, 0x01,                         //     Report Size (1)
    0x95, 0x03,                         //     Report Count (3)
    0x81, 0x02,                         //     Input (Data, Variable, Absolute) - Button states
    0x75, 0x05,                         //     Report Size (5)
    0x95, 0x01,                         //     Report Count (1)
    0x81, 0x01,                         //     Input (Constant) - Padding or Reserved bits
    0x05, 0x01,                         //     Usage Page (Generic Desktop)
    0x09, 0x30,                         //     Usage (X)
    0x09, 0x31,                         //     Usage (Y)
    0x09, 0x38,                         //     Usage (Wheel)
    0x15, 0x81,                         //     Logical Minimum (-127)
    0x25, 0x7F,                         //     Logical Maximum (127)
    0x75, 0x08,                         //     Report Size (8)
    0x95, 0x03,                         //     Report Count (3)
    0x81, 0x06,                         //     Input (Data, Variable, Relative) - X & Y coordinate
    0xC0,                               //   End Collection
    0xC0,                               // End Collection
    0x05, 0x01,                         // Usage Page (Generic Desktop),
    0x09, 0x06,                         // Usage (Keyboard),
    0xA1, 0x01,                         // Collection (Application),
    0x85, 0x02,                         // REPORT ID (2) - MANDATORY
    0x05, 0x07,                         //   Usage Page (Key Codes),
    0x19, 0xE0,                         //   Usage Minimum (224, keyboard left control),
    0x29, 0xE7,                         //   Usage Maximum (231, keyboard right gui),
    0x15, 0x00,                         //   Logical Minimum (0),
    0x25, 0x01,                         //   Logical Maximum (1),
    0x75, 0x01,                         //   Report Size (1),
    0x95, 0x08,                         //   Report Count (8),
    0x81, 0x02,                         //   Input (Data, Variable, Absolute), ;Modifier byte
    0x95, 0x01,                         //   Report Count (1)
    0x75, 0x08,                         //   Report Size (8)
    0x81, 0x01,                         //   Input: (constant) ; Reserved byte
    0x95, 0x05,                         //   Report Count (5),
    0x75, 0x01,                         //   Report Size (1),
    0x05, 0x08,                         //   Usage Page (LEDs),
    0x19, 0x01,                         //   Usage Minimum (1),
    0x29, 0x05,                         //   Usage Maximum (5),
    0x91, 0x02,                         //   Output (Data, Variable, Absolute), ;LED report
    0x95, 0x01,                         //   Report Count (1),
    0x75, 0x03,                         //   Report Size (3),
    0x91, 0x01,                         //   Output (Constant),                 ;LED report padding
    0x95, 0x06,                         //   Report Count (6),
    0x75, 0x08,                         //   Report Size (8),
    0x15, 0x00,                         //   Logical Minimum (0),
    0x25, 0xff,                         //   Logical Maximum(255), was 0x68 (104) before
    0x05, 0x07,                         //   Usage Page (Key Codes),
    0x19, 0x00,                         //   Usage Minimum (0),
    0x29, 0xe7,                         //   Usage Maximum (231), was 0x68 (104) before
    0x81, 0x00,                         //   Input (Data, Array),
    0xc0                                // End Collection
};

/* raw keyboard report */
static uint8_t logic_bluetooth_raw_report_map[] =
{
    0x06, USB_RAWHID_USAGE_PAGE & 0xFF, (USB_RAWHID_USAGE_PAGE >> 8) & 0xFF,
    0x0A, USB_RAWHID_USAGE & 0xFF, (USB_RAWHID_USAGE >> 8) & 0xFF,
    0xA1, 0x01,                         // Collection (application)
    0xA1, 0x02,                         // Collection (logical)
    0x85, 0x03,                         // Report ID 3
    0x75, 0x08,                         // report size = 8 bits
    0x15, 0x00,                         // logical minimum = 0
    0x26, 0xFF, 0x00,                   // logical maximum = 255
    0x95, USB_RAWHID_TX_SIZE,           // report count
    0x09, 0x01,                         // usage
    0x81, 0x02,                         // Input (array)
    0xC0,                               // end collection (logical)    
    0xA1, 0x02,                         // Collection (logical)
    0x85, 0x04,                         // Report ID 4
    0x75, 0x08,                         // report size = 8 bits
    0x15, 0x00,                         // logical minimum = 0
    0x26, 0xFF, 0x00,                   // logical maximum = 255
    0x95, USB_RAWHID_RX_SIZE,           // report count
    0x09, 0x02,                         // usage
    0x91, 0x02,                         // Output (array)
    0xC0,                               // end collection (logical)
    0xC0                                // end collection (application)
};

/*! \fn     logic_bluetooth_get_hid_serv_instance(uint16_t handle)
*   \brief  Function to get service instance
*   \param  Connection handle
*/
uint8_t logic_bluetooth_get_hid_serv_instance(uint16_t handle)
{
    DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_hid_serv_instance :: Get service Instance Handle %d", handle);
    for(uint8_t id = 0; id<HID_MAX_SERV_INST; id++)
    {
        DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_hid_serv_instance :: Service Handle %d  Characteristic Handle %d", *(logic_bluetooth_hid_serv_instances[id].hid_dev_serv_handle), logic_bluetooth_hid_serv_instances[id].hid_control_point->char_val.handle);
        if((handle > *(logic_bluetooth_hid_serv_instances[id].hid_dev_serv_handle)) && (handle <= logic_bluetooth_hid_serv_instances[id].hid_control_point->char_val.handle))
        {
            DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_hid_serv_instance :: Service Instance %d", id);
            return id;
        }
    }
    return  0xff;
}

/*! \fn     logic_bluetooth_get_notif_instance(uint8_t serv_num, uint16_t char_handle)
*   \brief  Function to get the notification instance.
*   \param  serv_num        Service Instance
*   \param  char_handle     Character Handle
*   \return see enum
*/
uint8_t logic_bluetooth_get_notif_instance(uint8_t serv_num, uint16_t char_handle)
{
    DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_notif_instance :: Search Handle %d", char_handle);
    DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_notif_instance :: Protocol Mode Handle %d", logic_bluetooth_hid_serv_instances[serv_num].hid_dev_proto_mode_char->char_val.handle);
    
    if(logic_bluetooth_hid_serv_instances[serv_num].hid_dev_proto_mode_char->char_val.handle == char_handle)
    {
        return PROTOCOL_MODE;
    }

    for(uint8_t id = 0; id < hid_prf_dataref[serv_num]->num_of_report; id++)
    {
        DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_notif_instance :: Report Handle %d", logic_bluetooth_hid_serv_instances[serv_num].hid_dev_report_val_char[id]->char_val.handle);
        if(char_handle == logic_bluetooth_hid_serv_instances[serv_num].hid_dev_report_val_char[id]->char_val.handle)
        {
            return CHAR_REPORT;
        }
        DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_notif_instance :: Report CCD Handle %d",logic_bluetooth_hid_serv_instances[serv_num].hid_dev_report_val_char[id]->client_config_desc.handle);
        if(char_handle == logic_bluetooth_hid_serv_instances[serv_num].hid_dev_report_val_char[id]->client_config_desc.handle)
        {
            return CHAR_REPORT_CCD;
        }
    }
    
    DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_notif_instance :: Boot Keyboard Handle %d", logic_bluetooth_hid_serv_instances[serv_num].hid_dev_boot_keyboard_in_report->char_val.handle);
    if(logic_bluetooth_hid_serv_instances[serv_num].hid_dev_boot_keyboard_in_report->char_val.handle == char_handle)
    {
        return BOOT_KEY_INPUT_REPORT;
    }
    
    DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_notif_instance :: Boot Keyboard CCD Handle %d", logic_bluetooth_hid_serv_instances[serv_num].hid_dev_boot_keyboard_in_report->client_config_desc.handle);
    if(logic_bluetooth_hid_serv_instances[serv_num].hid_dev_boot_keyboard_in_report->client_config_desc.handle == char_handle)
    {
        return BOOT_KEY_INPUT_CCD;
    }

    DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_notif_instance :: Control Point Handle %d", logic_bluetooth_hid_serv_instances[serv_num].hid_control_point->char_val.handle);
    if(logic_bluetooth_hid_serv_instances[serv_num].hid_control_point->char_val.handle == char_handle)
    {
        return CONTROL_POINT;
    }
    
    return 0xFF;
}

/*! \fn     logic_bluetooth_boot_key_report_update(at_ble_handle_t conn_handle, uint8_t serv_inst, uint8_t* bootreport, uint16_t len)
*   \brief  Function to update the boot keyboard report
*   \param  conn_handle     Connection handle
*   \param  serv_inst       Service Instance
*   \param  bootreport      Report to be send
*   \param  len             Length of report
*/
void logic_bluetooth_boot_key_report_update(at_ble_handle_t conn_handle, uint8_t serv_inst, uint8_t* bootreport, uint16_t len)
{
    at_ble_status_t status;
    uint16_t length = 2;
    uint8_t value = 0;
    
    status = at_ble_characteristic_value_get(logic_bluetooth_hid_serv_instances[serv_inst].hid_dev_boot_keyboard_in_report->client_config_desc.handle, &value, &length);
    if (status != AT_BLE_SUCCESS)
    {
        DBG_LOG("logic_bluetooth_boot_key_report_update :: Characteristic value get fail Reason %d", status);
    }
    
    //If Notification Enabled
    if(value == 1)
    {
        status = at_ble_characteristic_value_set(logic_bluetooth_hid_serv_instances[serv_inst].hid_dev_boot_keyboard_in_report->char_val.handle, bootreport, len);
        if (status != AT_BLE_SUCCESS)
        {
            DBG_LOG("logic_bluetooth_boot_key_report_update :: Characteristic value set fail Reason %d", status);
        }
        //Need to check for connection handle
        status = at_ble_notification_send(conn_handle, logic_bluetooth_hid_serv_instances[serv_inst].hid_dev_boot_keyboard_in_report->char_val.handle);
        if (status != AT_BLE_SUCCESS)
        {
            DBG_LOG("logic_bluetooth_boot_key_report_update :: Notification send failed %d", status);
        }
    }
}

/*! \fn     logic_bluetooth_get_reportid(uint8_t serv, uint16_t handle)
*   \brief  Function to get the report ID
*   \param  serv        HID service instance
*   \param  handle      Connection handle
  */
uint8_t logic_bluetooth_get_reportid(uint8_t serv, uint16_t handle)
{
    uint8_t descval[2] = {0, 0};
    uint16_t len = 2;
    uint8_t id = 0;
    uint8_t status;
    
    DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_reportid :: Report Number %d", hid_prf_dataref[serv]->num_of_report);
    for(id = 0; id < hid_prf_dataref[serv]->num_of_report; id++)
    {
        DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_reportid :: id %d Search Handle %d, CCD Handle %d", id, handle, logic_bluetooth_hid_serv_instances[serv].hid_dev_report_val_char[id]->client_config_desc.handle);
        if(handle == logic_bluetooth_hid_serv_instances[serv].hid_dev_report_val_char[id]->client_config_desc.handle)
        {
            DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_reportid :: Report ID Descriptor Handle %d :: id %d :: serv %d", logic_bluetooth_hid_serv_instances[serv].hid_dev_report_val_char[id]->additional_desc_list->handle, id, serv);
            status = at_ble_descriptor_value_get(logic_bluetooth_hid_serv_instances[serv].hid_dev_report_val_char[id]->additional_desc_list->handle, &descval[0], &len);
            if (status != AT_BLE_SUCCESS)
            {
                DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_reportid :: Decriptor value get failed");
            }
            else
            {
                DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_reportid :: Descriptor value Report ID %d Type %d", descval[0], descval[1]);   
            }
            return descval[0];
        }
    }
    return HID_INVALID_INST;
}

/*! \fn     logic_bluetooth_get_report_characteristic(uint16_t handle, uint8_t serv, uint8_t reportid)
*   \brief  Function to get report characteristic id.
*   \param  handle      Connection handle
*   \param  serv        Service Instance
*   \param  reportid    Report id
*/
uint8_t logic_bluetooth_get_report_characteristic(uint16_t handle, uint8_t serv, uint8_t reportid)
{
    uint8_t descval[2] = {0, 0};
    at_ble_status_t status;
    uint16_t len = 2;
    uint8_t id = 0;

    DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_report_characteristic :: Handle %d Service Instance %d Report ID %d", handle, serv, reportid); 
    for(id = 0; id < hid_prf_dataref[serv]->num_of_report; id++)
    {
        DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_report_characteristic :: Report ID Descriptor Handle %d :: id %d :: serv %d", logic_bluetooth_hid_serv_instances[serv].hid_dev_report_val_char[id]->additional_desc_list->handle, id, serv);
        status = at_ble_descriptor_value_get(logic_bluetooth_hid_serv_instances[serv].hid_dev_report_val_char[id]->additional_desc_list->handle, &descval[0], &len);
        if (status != AT_BLE_SUCCESS)
        {
            DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_report_characteristic :: Decriptor value get failed Reason %d", status);
        }
        else
        {
            DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_report_characteristic :: Report Value ID %d Type %d", descval[0], descval[1]); 
        }
        if(descval[0] == reportid)
        {
            DBG_LOG_LOGIC_BT_AD("logic_bluetooth_get_report_characteristic :: Report Characteristic ID %d", id);
            return id;
        }
    }
    
    DBG_LOG_LOGIC_BT_AD("Couldn't find report char!");
    return HID_INVALID_INST;
}

/*! \fn     logic_bluetooth_update_report(uint16_t conn_handle, uint8_t serv_inst, uint8_t reportid, uint8_t *report, uint16_t len, BOOL use_report_charac)
*   \brief  Function to update the report
*   \param  conn_handle         Connection handle
*   \param  serv_inst           Service Instance
*   \param  report_id           Report ID
*   \param  report              Report to be send
*   \param  len                 Length of report
*   \param  use_report_charac   Bool to indicate if we should use the report characteristic instead of the boot keyboard
*/
void logic_bluetooth_update_report(uint16_t conn_handle, uint8_t serv_inst, uint8_t reportid, uint8_t* report, uint16_t len, BOOL use_report_charac)
{
    // TODO: should we check for notification subscription?
    uint16_t status = 0;
    uint8_t id;
    
    /* Standard report? */
    if (use_report_charac != FALSE)
    {
        /* Get report characteristic id */
        id = logic_bluetooth_get_report_characteristic(conn_handle, serv_inst, reportid);
        
        if(id != HID_INVALID_INST)
        {
            if((status = at_ble_characteristic_value_set(logic_bluetooth_hid_serv_instances[serv_inst].hid_dev_report_val_char[id]->char_val.handle, report, len)) == AT_BLE_SUCCESS)
            {
                DBG_LOG("Updated characteristic %d for hid instance %d and report id %d: sending %d bytes", id, serv_inst, reportid, len);
                status = at_ble_notification_send(conn_handle, logic_bluetooth_hid_serv_instances[serv_inst].hid_dev_report_val_char[id]->char_val.handle);
                if (status != AT_BLE_SUCCESS)
                {
                    DBG_LOG("ERROR: Couldn't send notification, reason %d", status);
                }
            }
            else
            {
                DBG_LOG("ERROR: couldn't update characteristic %d for hid instance %d and report id %d: sending %d bytes", id, serv_inst, reportid, len);
            }
        }
    } 
    else
    {
        /* Boot keyboard: big hack */
        if((status = at_ble_characteristic_value_set(logic_bluetooth_hid_serv_instances[BLE_KEYBOARD_HID_SERVICE_INSTANCE].hid_dev_boot_keyboard_in_report->char_val.handle, report, len)) == AT_BLE_SUCCESS)
        {
            DBG_LOG("Updated boot keyboard characteristic");
            status = at_ble_notification_send(conn_handle, logic_bluetooth_hid_serv_instances[BLE_KEYBOARD_HID_SERVICE_INSTANCE].hid_dev_boot_keyboard_in_report->char_val.handle);
            if (status != AT_BLE_SUCCESS)
            {
                DBG_LOG("ERROR: Couldn't send notification, reason %d", status);
            }
        }
        else
        {
            DBG_LOG("ERROR: couldn't update boot keyboard characteristic");
        }
    }
}

/*! \fn     logic_bluetooth_hid_profile_init(uint8_t servinst, uint8_t device, uint8_t *mode, uint8_t report_num, uint8_t *report_type, uint8_t **report_val, uint8_t *report_len, hid_info_t *info)
*   \brief  Initialize the hid profile based on user input
*   \param  servinst    HID service instance 
*   \param  device      HID device
*   \param  mode        HID mode
*   \param  report_num  Number of report
*   \param  report_type Report Type
*   \param  report_val  Report Value
*   \param  report_len  Report Length
*   \param  info        HID device info 
*/
void logic_bluetooth_hid_profile_init(uint8_t servinst, uint8_t device, uint8_t* mode, uint8_t report_num, uint8_t* report_type, uint8_t** report_val, uint8_t* report_len, hid_info_t* info)
{
    uint16_t cur_characteristic_index = 0;
    
    /* Configure the HID service type */
    logic_bluetooth_hid_gatt_instances[servinst].serv.type = PRIMARY_SERVICE;

    /* Configure the HID service permission */
    if(BLE_PAIR_ENABLE)
    {
        if(BLE_MITM_REQ)
        {
            logic_bluetooth_hid_gatt_instances[servinst].serv.perm = (AT_BLE_ATTR_READABLE_REQ_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_REQ_AUTHN_NO_AUTHR);
        }
        else
        {
            logic_bluetooth_hid_gatt_instances[servinst].serv.perm = (AT_BLE_ATTR_READABLE_REQ_ENC_NO_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_REQ_ENC_NO_AUTHN_NO_AUTHR);
        }
    }
    else
    {
        logic_bluetooth_hid_gatt_instances[servinst].serv.perm = (AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_NO_AUTHN_NO_AUTHR);
    }
    /* Configure the HID service handle */
    logic_bluetooth_hid_gatt_instances[servinst].serv.handle = 0;   
    /* Configure HID Service */
    logic_bluetooth_hid_gatt_instances[servinst].serv.uuid.type = AT_BLE_UUID_16;
    logic_bluetooth_hid_gatt_instances[servinst].serv.uuid.uuid[0] = (uint8_t) HID_SERV_UUID;
    logic_bluetooth_hid_gatt_instances[servinst].serv.uuid.uuid[1] = (uint8_t) (HID_SERV_UUID>>8);      
    /* Configure HID include service list */
    logic_bluetooth_hid_gatt_instances[servinst].serv.inc_list = NULL;  
    /* Configure the HID include service count */
    logic_bluetooth_hid_gatt_instances[servinst].serv.included_count = 0;   
    /*Map the service UUID instance*/
    logic_bluetooth_hid_serv_instances[servinst].hid_dev_serv_uuid = &logic_bluetooth_hid_gatt_instances[servinst].serv.uuid;   
    /*Map the service UUID instance*/
    logic_bluetooth_hid_serv_instances[servinst].hid_dev_serv_handle = &logic_bluetooth_hid_gatt_instances[servinst].serv.handle;
    
    DBG_LOG_LOGIC_BT_AD("hid information characteristic: %d", cur_characteristic_index);
    /*Configure HID Information Characteristic : Value related info*/
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.handle = 0;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.type = AT_BLE_UUID_16;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[0] =(uint8_t) HID_UUID_CHAR_HID_INFORMATION;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[1] = (uint8_t)(HID_UUID_CHAR_HID_INFORMATION >> 8);
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.init_value = (uint8_t*)info;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.len = sizeof(hid_info_t);
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.max_len = sizeof(hid_info_t);
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.properties = AT_BLE_CHAR_READ;
    /* Configure the HID characteristic value permission */
    if(BLE_PAIR_ENABLE)
    {
        if(BLE_MITM_REQ)
        {
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_REQ_AUTHN_NO_AUTHR;
        }
        else
        {
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_REQ_ENC_NO_AUTHN_NO_AUTHR;
        }
    }
    else
    {
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR;
    }
    /*Configure HID Information Characteristic : user descriptor related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.user_description = NULL;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.len = 0;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.handle = 0;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.permissions = AT_BLE_ATTR_NO_PERMISSIONS;
    /*Configure HID Information Characteristic : presentation format related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].presentation_format = NULL;
    /*Configure HID Information Characteristic : client config descriptor related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.handle = 0;
    /*Configure HID Information Characteristic : server config descriptor related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.handle = 0;
    /*Configure HID Information Characteristic : generic descriptor related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_list = NULL;
    /*Configure HID Information Characteristic : count of generic descriptor */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_count = 0;
    /*Map the HID Information characteristic*/
    logic_bluetooth_hid_serv_instances[servinst].hid_dev_info = &logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index];
    
    /* Increment current index */
    cur_characteristic_index++;
    
    if(device == HID_KEYBOARD_MODE)
    {
        DBG_LOG_LOGIC_BT_AD("boot mouse input characteristic: %d", cur_characteristic_index);
        /*Configure HID Boot Keyboard Input Report Characteristic : Value related info*/
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.handle = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.type = AT_BLE_UUID_16;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[0] =(uint8_t) HID_UUID_CHAR_BOOT_MOUSE_INPUT_REPORT;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[1] = (uint8_t)(HID_UUID_CHAR_BOOT_MOUSE_INPUT_REPORT >> 8);
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.init_value = (uint8_t*)logic_bluetooth_boot_mouse_in_report;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.len = sizeof(logic_bluetooth_boot_mouse_in_report);
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.max_len = sizeof(logic_bluetooth_boot_mouse_in_report);
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.properties = (AT_BLE_CHAR_READ|AT_BLE_CHAR_NOTIFY);
        /* Configure the HID characteristic value permission */
        if(BLE_PAIR_ENABLE)
        {
            if(BLE_MITM_REQ)
            {
                logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_REQ_AUTHN_NO_AUTHR;
            }
            else
            {
                logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_REQ_ENC_NO_AUTHN_NO_AUTHR;
            }
        }
        else
        {
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR;
        }
        /*Configure HID Boot Keyboard Input Report Characteristic : user descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.user_description = NULL;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.len = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.handle = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.permissions = AT_BLE_ATTR_NO_PERMISSIONS;
        /*Configure HID Boot Keyboard Input Report Characteristic : presentation format related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].presentation_format = NULL;
        /*Configure HID Boot Keyboard Input Report Characteristic : client config descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.handle = 0;
        /*Configure HID Boot Keyboard Input Report Characteristic : server config descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.handle = 0;
        /*Configure HID Boot Keyboard Input Report Characteristic : generic descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_list = NULL;
        /*Configure HID Boot Keyboard Input Report Characteristic : count of generic descriptor */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_count = 0;
        /*Map the keyboard input report characteristic*/
        logic_bluetooth_hid_serv_instances[servinst].hid_dev_boot_keyboard_in_report = &logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index];
        
        /* Increment current index */
        cur_characteristic_index++;
        
        DBG_LOG_LOGIC_BT_AD("boot keyboard input characteristic: %d", cur_characteristic_index);
        /*Configure HID Boot Keyboard Input Report Characteristic : Value related info*/
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.handle = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.type = AT_BLE_UUID_16;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[0] =(uint8_t) HID_UUID_CHAR_BOOT_KEY_INPUT_REPORT;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[1] = (uint8_t)(HID_UUID_CHAR_BOOT_KEY_INPUT_REPORT >> 8);
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.init_value = (uint8_t*)logic_bluetooth_boot_keyb_in_report;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.len = sizeof(logic_bluetooth_boot_keyb_in_report);
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.max_len = sizeof(logic_bluetooth_boot_keyb_in_report);
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.properties = (AT_BLE_CHAR_READ|AT_BLE_CHAR_NOTIFY);
        /* Configure the HID characteristic value permission */
        if(BLE_PAIR_ENABLE)
        {
            if(BLE_MITM_REQ)
            {
                logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_REQ_AUTHN_NO_AUTHR;
            }
            else
            {
                logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_REQ_ENC_NO_AUTHN_NO_AUTHR;
            }
        }
        else
        {
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR;
        }
        /*Configure HID Boot Keyboard Input Report Characteristic : user descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.user_description = NULL;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.len = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.handle = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.permissions = AT_BLE_ATTR_NO_PERMISSIONS;
        /*Configure HID Boot Keyboard Input Report Characteristic : presentation format related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].presentation_format = NULL;
        /*Configure HID Boot Keyboard Input Report Characteristic : client config descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.handle = 0;
        /*Configure HID Boot Keyboard Input Report Characteristic : server config descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.handle = 0;
        /*Configure HID Boot Keyboard Input Report Characteristic : generic descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_list = NULL;
        /*Configure HID Boot Keyboard Input Report Characteristic : count of generic descriptor */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_count = 0;
        /*Map the keyboard input report characteristic*/
        logic_bluetooth_hid_serv_instances[servinst].hid_dev_boot_keyboard_in_report = &logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index];
        
        /* Increment current index */
        cur_characteristic_index++;
        
        DBG_LOG_LOGIC_BT_AD("boot keyboard output characteristic: %d", cur_characteristic_index);
        /*Configure HID Boot Keyboard Output Report Characteristic : Value related info*/
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.handle = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.type = AT_BLE_UUID_16;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[0] =(uint8_t) HID_UUID_CHAR_BOOT_KEY_OUTPUT_REPORT;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[1] = (uint8_t)(HID_UUID_CHAR_BOOT_KEY_OUTPUT_REPORT >> 8);
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.init_value = (uint8_t*)logic_bluetooth_boot_keyb_out_report;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.len = sizeof(uint8_t);
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.max_len = sizeof(uint8_t);
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.properties = (AT_BLE_CHAR_READ|AT_BLE_CHAR_WRITE_WITHOUT_RESPONSE|AT_BLE_CHAR_WRITE);
        /* Configure the HID characteristic value permission */
        if(BLE_PAIR_ENABLE)
        {
            if(BLE_MITM_REQ)
            {
                logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_READABLE_REQ_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_REQ_AUTHN_NO_AUTHR);
            }
            else
            {
                logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_READABLE_REQ_ENC_NO_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_REQ_ENC_NO_AUTHN_NO_AUTHR);
            }
        }
        else
        {
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_NO_AUTHN_NO_AUTHR);
        }
        /*Configure HID Boot Keyboard Output Report Characteristic : user descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.user_description = NULL;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.len = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.handle = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.permissions = AT_BLE_ATTR_NO_PERMISSIONS;
        /*Configure HID Boot Keyboard Output Report Characteristic : presentation format related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].presentation_format = NULL;
        /*Configure HID Boot Keyboard Output Report Characteristic : client config descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.handle = 0;
        /*Configure HID Boot Keyboard Output Report Characteristic : server config descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.handle = 0;
        /*Configure HID Boot Keyboard Output Report Characteristic : generic descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_list = NULL;
        /*Configure HID Boot Keyboard Output Report Characteristic : count of generic descriptor */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_count = 0;
        /*Map the keyboard output report characteristic*/
        logic_bluetooth_hid_serv_instances[servinst].hid_dev_boot_keyboard_out_report = &logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index];
        
        /* Increment current index */
        cur_characteristic_index++;
    }
    
    DBG_LOG_LOGIC_BT_AD("Report map characteristic: %d", cur_characteristic_index);
    /*Configure HID Report Map Characteristic : Value related info*/
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.handle = 0;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.type = AT_BLE_UUID_16;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[0] =(uint8_t) HID_UUID_CHAR_REPORT_MAP;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[1] = (uint8_t)(HID_UUID_CHAR_REPORT_MAP >> 8);
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.properties = AT_BLE_CHAR_READ;   
    if(BLE_PAIR_ENABLE)
    {
        if(BLE_MITM_REQ)
        {
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_REQ_AUTHN_NO_AUTHR;
        }
        else
        {
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_REQ_ENC_NO_AUTHN_NO_AUTHR;
        }
    }
    else
    {
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR;
    }
    /*Configure HID Report Map Characteristic : user descriptor related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.user_description = NULL;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.len = 0;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.handle = 0;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.permissions = AT_BLE_ATTR_NO_PERMISSIONS;   
    /*Configure HID Report Map Characteristic : presentation format related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].presentation_format = NULL;   
    /*Configure HID Report Map Characteristic : client config descriptor related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.handle = 0;    
    /*Configure HID Report Map Characteristic : server config descriptor related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.handle = 0;    
    /*Configure HID Report Map Characteristic : generic descriptor related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_list = NULL;  
    /*Configure HID Report Map Characteristic : count of generic descriptor */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_count = 0;    
    /*Map the device Report Map characteristic*/
    logic_bluetooth_hid_serv_instances[servinst].hid_dev_report_map_char = &logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index];
    
    /* Increment current index */
    cur_characteristic_index++;

    /*Configure number of HID report*/
    for(uint16_t id = 0; id < report_num; ++id)
    {
        /*Configure HID Report Characteristic : Value related info*/
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.handle = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.uuid.type = AT_BLE_UUID_16;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.uuid.uuid[0] =(uint8_t) HID_UUID_CHAR_REPORT;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.uuid.uuid[1] = (uint8_t)(HID_UUID_CHAR_REPORT >> 8);
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.init_value = ((report_val[id]));
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.len = report_len[id];
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.max_len = report_len[id];

        if(report_type[id] == INPUT_REPORT)
        {
            DBG_LOG_LOGIC_BT_AD("input report characteristic: %d", id + cur_characteristic_index);              
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.properties = (AT_BLE_CHAR_READ|AT_BLE_CHAR_NOTIFY);         
            /* Configure the HID characteristic value permission */
            if(BLE_PAIR_ENABLE)
            {
                if(BLE_MITM_REQ)
                {
                    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_REQ_AUTHN_NO_AUTHR;
                }
                else
                {
                    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_REQ_ENC_NO_AUTHN_NO_AUTHR;
                }
            }
            else
            {
                logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.permissions = AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR;
            }
            /*Configure HID Report Characteristic : client config descriptor related info (leave permissions as is!!!) */
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].client_config_desc.perm = (AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_NO_AUTHN_NO_AUTHR);
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].client_config_desc.handle = 0;
            
            /* Store pointer to handle */
            logic_bluetooth_report_notif_characteristic_numbers[logic_bluetooth_report_notif_characteristic_numbers_counter++] = &logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].client_config_desc.handle;
        }
        else if(report_type[id] == OUTPUT_REPORT)
        {
            DBG_LOG_LOGIC_BT_AD("output report characteristic: %d", id + cur_characteristic_index);
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.properties = (AT_BLE_CHAR_READ|AT_BLE_CHAR_WRITE_WITHOUT_RESPONSE|AT_BLE_CHAR_WRITE);           
            /* Configure the HID characteristic value permission */
            if(BLE_PAIR_ENABLE)
            {
                if(BLE_MITM_REQ)
                {
                    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_READABLE_REQ_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_REQ_AUTHN_NO_AUTHR);
                }
                else
                {
                    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_READABLE_REQ_ENC_NO_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_REQ_ENC_NO_AUTHN_NO_AUTHR);
                }
            }
            else
            {
                logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_NO_AUTHN_NO_AUTHR);
            }           
            /*Configure HID Report Characteristic : client config descriptor related info */
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].client_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].client_config_desc.handle = 0;
        }
        else if(report_type[id] == FEATURE_REPORT)
        {
            DBG_LOG_LOGIC_BT_AD("feature report characteristic: %d", id + cur_characteristic_index);
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.properties = (AT_BLE_CHAR_READ|AT_BLE_CHAR_WRITE);          
            /* Configure the HID characteristic permission */
            if(BLE_PAIR_ENABLE)
            {
                if(BLE_MITM_REQ)
                {
                    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_READABLE_REQ_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_REQ_AUTHN_NO_AUTHR);
                }
                else
                {
                    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_READABLE_REQ_ENC_NO_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_REQ_ENC_NO_AUTHN_NO_AUTHR);
                }
            }
            else
            {
                logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_NO_AUTHN_NO_AUTHR);
            }           
            /*Configure HID Report Characteristic : client config descriptor related info */
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].client_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].client_config_desc.handle = 0; 
        }
 
        /*Configure HID Report Characteristic : user descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].user_desc.user_description = NULL;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].user_desc.len = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].user_desc.handle = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].user_desc.permissions = AT_BLE_ATTR_NO_PERMISSIONS;              
        /*Configure HID Report Characteristic : presentation format related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].presentation_format = NULL;          
        /*Configure HID Report Map Characteristic : server config descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].server_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].server_config_desc.handle = 0;
        
        /* Configure HID Report Reference Descriptor*/
        logic_bluetooth_hid_gatt_instances[servinst].serv_desc[id].desc_val_length = (sizeof(uint8_t)*2);
        logic_bluetooth_hid_gatt_instances[servinst].serv_desc[id].desc_val_max_length = (sizeof(uint8_t)*2);      
        if(BLE_PAIR_ENABLE)
        {
            if(BLE_MITM_REQ)
            {
                logic_bluetooth_hid_gatt_instances[servinst].serv_desc[id].perm = (AT_BLE_ATTR_READABLE_REQ_AUTHN_NO_AUTHR);
            }
            else
            {
                logic_bluetooth_hid_gatt_instances[servinst].serv_desc[id].perm = (AT_BLE_ATTR_READABLE_REQ_ENC_NO_AUTHN_NO_AUTHR);
            }
        }
        else
        {
            logic_bluetooth_hid_gatt_instances[servinst].serv_desc[id].perm = (AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR);
        }
        logic_bluetooth_hid_gatt_instances[servinst].serv_desc[id].uuid.type = AT_BLE_UUID_16;
        logic_bluetooth_hid_gatt_instances[servinst].serv_desc[id].uuid.uuid[0] =(uint8_t) HID_REPORT_REF_DESC;
        logic_bluetooth_hid_gatt_instances[servinst].serv_desc[id].uuid.uuid[1] = (uint8_t)(HID_REPORT_REF_DESC >> 8);
        logic_bluetooth_hid_gatt_instances[servinst].serv_desc[id].handle +=  0;                
        
        /*Configure HID Report Characteristic : generic descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].additional_desc_list = &logic_bluetooth_hid_gatt_instances[servinst].serv_desc[id];
        
        /*Configure HID Report Characteristic : count of generic descriptor */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index].additional_desc_count = 1;
            
        /*Map the report characteristic*/
        logic_bluetooth_hid_serv_instances[servinst].hid_dev_report_val_char[id] = &logic_bluetooth_hid_gatt_instances[servinst].serv_chars[id + cur_characteristic_index];     
    }//End of for loop
    
    /* Increment current index */
    cur_characteristic_index+= report_num;
    
    DBG_LOG_LOGIC_BT_AD("hid control characteristic: %d", cur_characteristic_index);
    /*Configure HID Control Point Characteristic : Value related info*/
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.handle = 0;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.type = AT_BLE_UUID_16;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[0] = (uint8_t) HID_UUID_CHAR_HID_CONTROL_POINT;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[1] = (uint8_t)(HID_UUID_CHAR_HID_CONTROL_POINT >> 8);
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.init_value = (uint8_t*)logic_bluetooth_ctrl_point;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.len = sizeof(uint8_t);
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.max_len = sizeof(uint8_t);
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.properties = AT_BLE_CHAR_WRITE_WITHOUT_RESPONSE; 
    if(BLE_PAIR_ENABLE)
    {
        if(BLE_MITM_REQ)
        {
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_WRITABLE_REQ_AUTHN_NO_AUTHR);
        }
        else
        {
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_WRITABLE_REQ_ENC_NO_AUTHN_NO_AUTHR);
        }
    }
    else
    {
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_WRITABLE_NO_AUTHN_NO_AUTHR);
    }
    /*Configure HID Control Point Characteristic : user descriptor related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.user_description = NULL;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.len = 0;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.handle = 0;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.permissions = AT_BLE_ATTR_NO_PERMISSIONS;   
    /*Configure HID Control Point Characteristic : presentation format related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].presentation_format = NULL;   
    /*Configure HID Control Point Characteristic : client config descriptor related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.handle = 0;    
    /*Configure HID Control Point Characteristic : server config descriptor related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.handle = 0;    
    /*Configure HID Control Point Characteristic : generic descriptor related info */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_list = NULL;  
    /*Configure HID Control Point Characteristic : count of generic descriptor */
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_count = 0;
    /*Map the HID Information characteristic*/
    logic_bluetooth_hid_serv_instances[servinst].hid_control_point = &logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index];    
    /* Configure HID characteristic list */
    logic_bluetooth_hid_gatt_instances[servinst].serv.char_list =  logic_bluetooth_hid_gatt_instances[servinst].serv_chars;
    
    /* Increment current index */
    cur_characteristic_index++;
    
    /* Keyboard / raw hid switch */
    if (device == HID_KEYBOARD_MODE)
    {
        DBG_LOG_LOGIC_BT_AD("Protocol mode characteristic: %d", cur_characteristic_index);
        /*Configure HID Protocol Mode Characteristic : Value related info*/
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.handle = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.type = AT_BLE_UUID_16;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[0] =(uint8_t) HID_UUID_CHAR_PROTOCOL_MODE;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[1] = (uint8_t)(HID_UUID_CHAR_PROTOCOL_MODE >> 8);
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.properties = (AT_BLE_CHAR_READ|AT_BLE_CHAR_WRITE_WITHOUT_RESPONSE);
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.init_value = mode;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.len = sizeof(uint8_t);
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.max_len = sizeof(uint8_t);
        /* Configure the HID characteristic value permission */
        if(BLE_PAIR_ENABLE)
        {
            if(BLE_MITM_REQ)
            {
                logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_READABLE_REQ_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_REQ_AUTHN_NO_AUTHR);
            }
            else
            {
                logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_READABLE_REQ_ENC_NO_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_REQ_ENC_NO_AUTHN_NO_AUTHR);
            }
        }
        else
        {
            logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.permissions = (AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR | AT_BLE_ATTR_WRITABLE_NO_AUTHN_NO_AUTHR);
        }
        /*Configure HID Protocol Mode Characteristic : user descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.user_description = NULL;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.len = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.handle = 0;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].user_desc.permissions = AT_BLE_ATTR_NO_PERMISSIONS;
        /*Configure HID Protocol Mode Characteristic : presentation format related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].presentation_format = NULL;
        /*Configure HID Protocol Mode Characteristic : client config descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].client_config_desc.handle = 0;
        /*Configure HID Protocol Mode Characteristic : server config descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.perm = AT_BLE_ATTR_NO_PERMISSIONS;
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].server_config_desc.handle = 0;
        /*Configure HID Protocol Mode Characteristic : generic descriptor related info */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_list = NULL;
        /*Configure HID Protocol Mode Characteristic : count of generic descriptor */
        logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].additional_desc_count = 0;
        /*Map the device protocol mode characteristic*/
        logic_bluetooth_hid_serv_instances[servinst].hid_dev_proto_mode_char = &logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index];
        
        /* Increment current index */
        cur_characteristic_index++;
    }
    
    /* Configure the HID characteristic count */
    logic_bluetooth_hid_gatt_instances[servinst].serv.char_count = cur_characteristic_index;    
}

/*! \fn     logic_bluetooth_gpio_set(at_ble_gpio_pin_t pin, at_ble_gpio_status_t status)
*   \brief  Function called by library to set GPIOs
*   \param  pin     The PIN (see enum)
*   \param  status  high or low (see enum)
*/
void logic_bluetooth_gpio_set(at_ble_gpio_pin_t pin, at_ble_gpio_status_t status)
{
    if (pin == AT_BLE_CHIP_ENABLE)
    {
        if (status == AT_BLE_HIGH)
        {
            platform_io_assert_ble_enable();
        }
        else
        {
            platform_io_deassert_ble_enable();
        }
    }
    else if (pin == AT_BLE_EXTERNAL_WAKEUP)
    {
        if (status == AT_BLE_HIGH)
        {
            platform_io_assert_ble_wakeup();
        }
        else
        {
            /* Sleep between events only if there is no data to be processed */
            if (platform_io_is_wakeup_in_pin_low() == FALSE)
            {
                platform_io_deassert_ble_wakeup();
            }
        }
    }
}

/*! \fn     logic_bluetooth_stop_bluetooth(void)
*   \brief  Stop bluetooth
*/
void logic_bluetooth_stop_bluetooth(void)
{
    /* Should we stop advertising? */
    if (logic_bluetooth_advertising != FALSE)
    {
        logic_bluetooth_stop_advertising();
    }
    
    /* Set booleans */
    logic_bluetooth_notif_being_sent = NONE_NOTIF_SENDING;
    logic_bluetooth_can_communicate_with_host = FALSE;
    logic_bluetooth_open_to_pairing = FALSE;
    logic_bluetooth_just_connected = FALSE;
    logic_bluetooth_advertising = FALSE;
    logic_bluetooth_just_paired = FALSE;
    logic_bluetooth_connected = FALSE;
    logic_bluetooth_paired = FALSE;
    
    /* Reset UARTs */
    platform_io_reset_ble_uarts();
    
    /* Reset IOs */
    platform_io_init_ble_ports_for_disabled();
}

/*! \fn     logic_bluetooth_enable_atbtlc_32k_output(void)
*   \brief  Enable ATBTLC1000 32k output
*/
void logic_bluetooth_enable_atbtlc_32k_output(void)
{
    uint32_t val;
    
    //read_32_from_BTLC1000(0X4000F404, &val);
    //val |= (0b0<<20); // Bits 20-23 control the value of internal tuning capacitors. Valid value 0b0000 to 0b1111
    //write_32_to_BTLC1000(0X4000F404,val);

    // 32.768kHz RTC XO clock output = 8
    write_32_to_BTLC1000(0x40020250, 8);
    
    /* Set pinmux_sel_2 (base address 0x4000B04C) bits 2:0 (Pinmux select for LP_GPIO_16) to value 7 (TEST OUT 16) */
    read_32_from_BTLC1000(0x4000B04C, &val);
    val |= 0x07;
    write_32_to_BTLC1000(0x4000B04C, val);
    
    // Enable test MUX output
    write_32_to_BTLC1000(0x400201A0, 1);

    // Block BTLC1000 from entering ULP
    logic_bluetooth_gpio_set(AT_BLE_EXTERNAL_WAKEUP, AT_BLE_HIGH);
}

/*! \fn     logic_bluetooth_start_bluetooth(uint8_t* unit_mac_address)
*   \brief  Start bluetooth
*   \param  unit_mac_address    6 bytes unit mac address
*/
void logic_bluetooth_start_bluetooth(uint8_t* unit_mac_address)
{
    DBG_LOG("Starting Bluetooth...");
    
    /* Setup Keyboard HID profile data */
    logic_bluetooth_hid_prf_data.hid_serv_instance = BLE_KEYBOARD_HID_SERVICE_INSTANCE;
    logic_bluetooth_hid_prf_data.hid_device = HID_KEYBOARD_MODE;
    logic_bluetooth_hid_prf_data.protocol_mode = HID_REPORT_PROTOCOL_MODE;
    logic_bluetooth_hid_prf_data.num_of_report = 2;
    logic_bluetooth_hid_prf_data.report_id[0] = BLE_MOUSE_HID_IN_REPORT_NB;
    logic_bluetooth_hid_prf_data.report_type[0] = INPUT_REPORT;
    logic_bluetooth_hid_prf_data.report_val[0] = &logic_bluetooth_mouse_in_report[0];
    logic_bluetooth_hid_prf_data.report_len[0] = sizeof(logic_bluetooth_mouse_in_report);
    logic_bluetooth_hid_prf_data.report_id[1] = BLE_KEYBOARD_HID_IN_REPORT_NB;
    logic_bluetooth_hid_prf_data.report_type[1] = INPUT_REPORT;
    logic_bluetooth_hid_prf_data.report_val[1] = &logic_bluetooth_keyboard_in_report[0];
    logic_bluetooth_hid_prf_data.report_len[1] = sizeof(logic_bluetooth_keyboard_in_report);
    logic_bluetooth_hid_prf_data.report_map_info.report_map = logic_bluetooth_keyb_report_map;
    logic_bluetooth_hid_prf_data.report_map_info.report_map_len = sizeof(logic_bluetooth_keyb_report_map);
    logic_bluetooth_hid_prf_data.hid_device_info.bcd_hid = 0x0111;
    logic_bluetooth_hid_prf_data.hid_device_info.bcountry_code = 0x00;
    //logic_bluetooth_raw_hid_prf_data.hid_device_info.flags = 0x02;
    logic_bluetooth_hid_prf_data.hid_device_info.flags = 0x03;
    
    /* Update HID profile data reference */
    hid_prf_dataref[BLE_KEYBOARD_HID_SERVICE_INSTANCE] = &logic_bluetooth_hid_prf_data;
    
    /* Setup Raw HID profile data */
    logic_bluetooth_raw_hid_prf_data.hid_serv_instance = BLE_RAW_HID_SERVICE_INSTANCE;
    logic_bluetooth_raw_hid_prf_data.hid_device = HID_RAW_MODE;
    logic_bluetooth_raw_hid_prf_data.protocol_mode = HID_REPORT_PROTOCOL_MODE;
    logic_bluetooth_raw_hid_prf_data.num_of_report = 2;
    logic_bluetooth_raw_hid_prf_data.report_id[0] = BLE_RAW_HID_IN_REPORT_NB;
    logic_bluetooth_raw_hid_prf_data.report_type[0] = INPUT_REPORT;
    logic_bluetooth_raw_hid_prf_data.report_val[0] = &logic_bluetooth_raw_hid_data_out_buf[0];
    logic_bluetooth_raw_hid_prf_data.report_len[0] = sizeof(logic_bluetooth_raw_hid_data_out_buf);
    logic_bluetooth_raw_hid_prf_data.report_id[1] = BLE_RAW_HID_OUT_REPORT_NB;
    logic_bluetooth_raw_hid_prf_data.report_type[1] = OUTPUT_REPORT;
    logic_bluetooth_raw_hid_prf_data.report_val[1] = &logic_bluetooth_raw_hid_data_in_buf[0];
    logic_bluetooth_raw_hid_prf_data.report_len[1] = sizeof(logic_bluetooth_raw_hid_data_in_buf);
    logic_bluetooth_raw_hid_prf_data.report_map_info.report_map = logic_bluetooth_raw_report_map;
    logic_bluetooth_raw_hid_prf_data.report_map_info.report_map_len = sizeof(logic_bluetooth_raw_report_map);
    logic_bluetooth_raw_hid_prf_data.hid_device_info.bcd_hid = 0x0111;
    logic_bluetooth_raw_hid_prf_data.hid_device_info.bcountry_code = 0x00;
    //logic_bluetooth_raw_hid_prf_data.hid_device_info.flags = 0x02;
    logic_bluetooth_raw_hid_prf_data.hid_device_info.flags = 0x03;
    
    /* Update HID profile data reference */
    hid_prf_dataref[BLE_RAW_HID_SERVICE_INSTANCE] = &logic_bluetooth_raw_hid_prf_data;
    
    /* IO inits for BLE enabling */
    platform_io_ble_enabled_inits();
    
    /* Generate random mac address :D */
    at_ble_addr_t mac_address;
    mac_address.type = AT_BLE_ADDRESS_PUBLIC;
    memcpy(mac_address.addr, unit_mac_address, AT_BLE_ADDR_LEN);
    
    /* Initialize the ble chip and set the device mac address */
    ble_device_init(&mac_address, &logic_bluetooth_advanced_info);
    
    /* Initialize the device information service */
    dis_gatt_service_handler_t device_info_serv;
    dis_init_service(&device_info_serv);
    
    /* Define the primary service in the GATT server database */
    if(dis_primary_service_define(&device_info_serv) != AT_BLE_SUCCESS)
    {
        DBG_LOG("ERROR: Defining device information service failed");
    }
    
    /* Initialize the battery service */
    bat_init_service(&logic_bluetooth_bas_service_handler, &logic_bluetooth_ble_battery_level);
    
    /* Define the primary service in the GATT server database */
    if(bat_primary_service_define(&logic_bluetooth_bas_service_handler) != AT_BLE_SUCCESS)
    {
        DBG_LOG("ERROR: Defining battery service failed");
    }
    
    /* Enable notification for battery service */
    if (at_ble_characteristic_value_set(logic_bluetooth_bas_service_handler.serv_chars.client_config_handle, &logic_bluetooth_ble_battery_notif_char, sizeof(uint8_t)) != AT_BLE_SUCCESS)
    {
        DBG_LOG("Couldn't set battery notification characteristic");
    }
    
    /* Reset counter */
    logic_bluetooth_report_notif_characteristic_numbers_counter = 0;
    
    /* Initialize HID services */
    #ifdef ONE_BLE_HID_INTERFACE_DBG
    for(uint16_t serv_num = 0; serv_num <= 0; serv_num++)
    #else
    for(uint16_t serv_num = 0; serv_num <= BLE_RAW_HID_SERVICE_INSTANCE; serv_num++)
    #endif
    {
        /* Clear service */
        memset(&logic_bluetooth_hid_serv_instances[serv_num], 0, sizeof(logic_bluetooth_hid_serv_instances[0]));
        
        /* Initialize HID profile based on previously provided input */
        logic_bluetooth_hid_profile_init(serv_num, hid_prf_dataref[serv_num]->hid_device, &hid_prf_dataref[serv_num]->protocol_mode, hid_prf_dataref[serv_num]->num_of_report, (uint8_t *)&hid_prf_dataref[serv_num]->report_type, &(hid_prf_dataref[serv_num]->report_val[0]), (uint8_t *)&hid_prf_dataref[serv_num]->report_len, &hid_prf_dataref[serv_num]->hid_device_info);
                
        /* Map report map with correct characteristic */
        if (serv_num == BLE_KEYBOARD_HID_SERVICE_INSTANCE)
        {
            logic_bluetooth_hid_gatt_instances[serv_num].serv_chars[4].char_val.init_value = hid_prf_dataref[serv_num]->report_map_info.report_map;
            logic_bluetooth_hid_gatt_instances[serv_num].serv_chars[4].char_val.max_len = hid_prf_dataref[serv_num]->report_map_info.report_map_len;
            logic_bluetooth_hid_gatt_instances[serv_num].serv_chars[4].char_val.len = hid_prf_dataref[serv_num]->report_map_info.report_map_len;
        }
        else
        {
            logic_bluetooth_hid_gatt_instances[serv_num].serv_chars[1].char_val.init_value = hid_prf_dataref[serv_num]->report_map_info.report_map;
            logic_bluetooth_hid_gatt_instances[serv_num].serv_chars[1].char_val.max_len = hid_prf_dataref[serv_num]->report_map_info.report_map_len;
            logic_bluetooth_hid_gatt_instances[serv_num].serv_chars[1].char_val.len = hid_prf_dataref[serv_num]->report_map_info.report_map_len;
        }
        logic_bluetooth_hid_serv_instances[serv_num].hid_dev_report_map_char->char_val.init_value = hid_prf_dataref[serv_num]->report_map_info.report_map;
        logic_bluetooth_hid_serv_instances[serv_num].hid_dev_report_map_char->char_val.max_len = hid_prf_dataref[serv_num]->report_map_info.report_map_len;
        logic_bluetooth_hid_serv_instances[serv_num].hid_dev_report_map_char->char_val.len = hid_prf_dataref[serv_num]->report_map_info.report_map_len;        
        timer_delay_ms(1);
        
        /* HID service database registration */
        if(at_ble_service_define(&logic_bluetooth_hid_gatt_instances[serv_num].serv) == AT_BLE_SUCCESS)
        {
            DBG_LOG_LOGIC_BT_AD("HID Service Handle Value %d", logic_bluetooth_hid_gatt_instances[serv_num].serv.handle);            
            /* Set report reference descriptor values */
            for(uint16_t id = 0; id < hid_prf_dataref[serv_num]->num_of_report; id++)
            {
                uint8_t descval[2];
                descval[0] = hid_prf_dataref[serv_num]->report_id[id];
                descval[1] = hid_prf_dataref[serv_num]->report_type[id];                
                /* Call to BluSDK */
                at_ble_status_t status;
                if((status = at_ble_descriptor_value_set(logic_bluetooth_hid_serv_instances[serv_num].hid_dev_report_val_char[id]->additional_desc_list->handle, &descval[0], 2)) == AT_BLE_SUCCESS)
                {
                    DBG_LOG_LOGIC_BT_AD("Report Reference Descriptor Value Set");
                }
                else
                {
                    DBG_LOG_LOGIC_BT_AD("Report Reference Descriptor Value Set Fail Reason %d",status);
                }
            }            
            /* Store service handle */
            hid_prf_dataref[serv_num]->serv_handle_info = logic_bluetooth_hid_gatt_instances[serv_num].serv.handle;
        }
        else
        {
            DBG_LOG("ERROR: HID Service Registration Fail");
        }
    }
    
    /* Enable notifications by default */
    for (uint16_t i = 0; i < logic_bluetooth_report_notif_characteristic_numbers_counter; i++)
    {
        /* Set the notification value to 1, as we pass it as a pointer */
        logic_bluetooth_report_notif_characteristic_numbers_init_values[i] = 1;
        if (at_ble_characteristic_value_set(*logic_bluetooth_report_notif_characteristic_numbers[i], &logic_bluetooth_report_notif_characteristic_numbers_init_values[i], sizeof(uint8_t)) != AT_BLE_SUCCESS)
        {
            DBG_LOG("Couldn't set notification characteristic");
        }
    }
    
    /*  Set advertisement data from ble_manager */
    if(!(ble_advertisement_data_set() == AT_BLE_SUCCESS))
    {
        DBG_LOG("ERROR: Fail to set Advertisement data");
    }
    
    /* Set callbacks if we haven't already */
    if (logic_bluetooth_callbacks_set == FALSE)
    {
        /* Callback registering for BLE-GAP Role */
        ble_mgr_events_callback_handler(REGISTER_CALL_BACK, BLE_GAP_EVENT_TYPE, &hid_app_gap_handle);
        
        /* Callback registering for BLE-GATT-Server Role */
        ble_mgr_events_callback_handler(REGISTER_CALL_BACK, BLE_GATT_SERVER_EVENT_TYPE, &hid_app_gatt_server_handle);
        
        /* Register callbacks for custom related events */
        ble_mgr_events_callback_handler(REGISTER_CALL_BACK, BLE_CUSTOM_EVENT_TYPE, &hid_custom_event_cb);
        
        logic_bluetooth_callbacks_set = TRUE;
    }
    
    /* Start advertising */
    logic_bluetooth_start_advertising();
}

/*! \fn     logic_bluetooth_start_advertising(void)
*   \brief  Start advertising
*/
void logic_bluetooth_start_advertising(void)
{
    /* Start of advertisement */
    if(at_ble_adv_start(AT_BLE_ADV_TYPE_UNDIRECTED, AT_BLE_ADV_GEN_DISCOVERABLE, NULL, AT_BLE_ADV_FP_ANY, APP_HID_FAST_ADV, APP_HID_ADV_TIMEOUT, 0) == AT_BLE_SUCCESS)
    {
        DBG_LOG("Device Started Advertisement");
        logic_bluetooth_advertising = TRUE;
    }
    else
    {
        DBG_LOG("ERROR: Device Advertisement Failed");
        logic_bluetooth_advertising = FALSE;
    }    
}

/*! \fn     logic_bluetooth_stop_advertising(void)
*   \brief  Stop advertising
*/
ret_type_te logic_bluetooth_stop_advertising(void)
{
    if (logic_bluetooth_advertising != FALSE)
    {
        if(at_ble_adv_stop() == AT_BLE_SUCCESS)
        {
            logic_bluetooth_advertising = FALSE;
            DBG_LOG("Advertising stopped");
            return RETURN_OK;
        }
        else
        {
            DBG_LOG("ERROR: Couldn't stop advertising");
            return RETURN_NOK;
        }
    } 
    else
    {
        return RETURN_OK;
    }
}

/*! \fn     logic_bluetooth_set_battery_level(uint8_t pct)
*   \brief  Update battery profile advertised battery level
*   \param  pct Battery percentage
*/
void logic_bluetooth_set_battery_level(uint8_t pct)
{
    DBG_LOG("battery level:%d%%", pct);
    if (logic_bluetooth_can_communicate_with_host == FALSE)
    {
        logic_bluetooth_ble_battery_level = pct;
    } 
    else
    {
        logic_bluetooth_pending_battery_level = pct;
    }
}

/*! \fn     logic_bluetooth_can_talk_to_host(void)
*   \brief  Know if we are paired
*   \return Paired status
*/
BOOL logic_bluetooth_can_talk_to_host(void)
{
    return logic_bluetooth_can_communicate_with_host;
}

/*! \fn     logic_bluetooth_raw_send(uint8_t* data, uint16_t data_len)
*   \brief  Send raw data through bluetooth
*   \param  data        Pointer to the data
*   \param  data_len    Data length
*/
void logic_bluetooth_raw_send(uint8_t* data, uint16_t data_len)
{
    #ifdef ONE_BLE_HID_INTERFACE_DBG
        comms_raw_hid_send_callback(BLE_INTERFACE);
        return;
    #endif
    
    /* Check that we're actually paired */
    if (logic_bluetooth_can_communicate_with_host != FALSE)
    {
        /* Check for overflow */
        if (data_len > sizeof(logic_bluetooth_raw_hid_data_out_buf))
        {
            data_len = sizeof(logic_bluetooth_raw_hid_data_out_buf);
        }
        
        /* Copy data to buffer */
        memset(logic_bluetooth_raw_hid_data_out_buf, 0, sizeof(logic_bluetooth_raw_hid_data_out_buf));
        memcpy(logic_bluetooth_raw_hid_data_out_buf, data, data_len);
        
        /* Debug */
        DBG_LOG("BLE send: %02x %02x %02x%02x %02x%02x", logic_bluetooth_raw_hid_data_out_buf[0], logic_bluetooth_raw_hid_data_out_buf[1], logic_bluetooth_raw_hid_data_out_buf[2], logic_bluetooth_raw_hid_data_out_buf[3], logic_bluetooth_raw_hid_data_out_buf[4], logic_bluetooth_raw_hid_data_out_buf[5]);
        
        /* Send data */
        logic_bluetooth_check_and_wait_for_notif_sent();
        logic_bluetooth_notif_being_sent = RAW_HID_NOTIF_SENDING;
        logic_bluetooth_update_report(logic_bluetooth_ble_connection_handle, BLE_RAW_HID_SERVICE_INSTANCE, BLE_RAW_HID_IN_REPORT_NB, logic_bluetooth_raw_hid_data_out_buf, sizeof(logic_bluetooth_raw_hid_data_out_buf), TRUE);
    }
    else
    {
        DBG_LOG("BLE Call to raw send but device not connected");
        comms_raw_hid_send_callback(BLE_INTERFACE);
    }
}

/*! \fn     logic_bluetooth_send_modifier_and_key(uint8_t modifier, uint8_t key, uint8_t second_key)
*   \brief  Send modifier and key through keyboard link
*   \param  modifier    HID modifier
*   \param  key         HID key
*   \param  second_key  Another HID key
*   \return If we were able to correctly type
*/
ret_type_te logic_bluetooth_send_modifier_and_key(uint8_t modifier, uint8_t key, uint8_t second_key)
{
    if (logic_bluetooth_can_communicate_with_host != FALSE)
    {
        if (TRUE)
        {
            logic_bluetooth_check_and_wait_for_notif_sent();
            logic_bluetooth_notif_being_sent = KEYBOARD_NOTIF_SENDING;
            logic_bluetooth_keyboard_in_report[0] = modifier;
            logic_bluetooth_keyboard_in_report[2] = key;
            logic_bluetooth_keyboard_in_report[3] = second_key;
            logic_bluetooth_typed_report_sent = FALSE;
            logic_bluetooth_update_report(logic_bluetooth_ble_connection_handle, BLE_KEYBOARD_HID_SERVICE_INSTANCE, BLE_KEYBOARD_HID_IN_REPORT_NB, logic_bluetooth_keyboard_in_report, sizeof(logic_bluetooth_keyboard_in_report), TRUE);
        } 
        else
        {
            logic_bluetooth_check_and_wait_for_notif_sent();
            logic_bluetooth_notif_being_sent = KEYBOARD_NOTIF_SENDING;
            logic_bluetooth_boot_keyb_in_report[0] = modifier;
            logic_bluetooth_boot_keyb_in_report[2] = key;
            logic_bluetooth_boot_keyb_in_report[3] = second_key;
            logic_bluetooth_typed_report_sent = FALSE;
            logic_bluetooth_update_report(logic_bluetooth_ble_connection_handle, BLE_KEYBOARD_HID_SERVICE_INSTANCE, BLE_KEYBOARD_HID_IN_REPORT_NB, logic_bluetooth_boot_keyb_in_report, sizeof(logic_bluetooth_boot_keyb_in_report), FALSE);
        }
        
        /* OK I'm still not sure about this one... but I think it should be OK. Stack trace is main > comms_main_mcu_routine > comms_main_mcu_deal_with_non_usb_non_ble_message > logic_keyboard_type_symbol > logic_keyboard_type_key_with_modifier to here */
        timer_start_timer(TIMER_BT_TYPING_TIMEOUT, 1000);
        while ((timer_has_timer_expired(TIMER_BT_TYPING_TIMEOUT, FALSE) == TIMER_RUNNING) && (logic_bluetooth_typed_report_sent == FALSE))
        {
            ble_event_task();
        }
        
        /* Report sent? */
        if (logic_bluetooth_typed_report_sent == FALSE)
        {
            DBG_LOG("Couldn't send modifier in key as notification in time!");
            return RETURN_NOK;
        } 
        else
        {
            return RETURN_OK;
        }
    }
    else
    {
        return RETURN_NOK;
    }
}

/*! \fn     logic_bluetooth_routine(void)
*   \brief  Our bluetooth routine
*/
void logic_bluetooth_routine(void)
{
    /* Update battery pct if needed */
    if (logic_bluetooth_pending_battery_level != UINT8_MAX)
    {
        logic_bluetooth_ble_battery_level = logic_bluetooth_pending_battery_level;
        logic_bluetooth_pending_battery_level = UINT8_MAX;

        /* send the notification and Update the battery level  */
        if ((logic_bluetooth_battery_notification_flag != FALSE) && (logic_bluetooth_can_communicate_with_host != FALSE))
        {
            logic_bluetooth_check_and_wait_for_notif_sent();
            if(bat_update_char_value(logic_bluetooth_ble_connection_handle, &logic_bluetooth_bas_service_handler, logic_bluetooth_ble_battery_level, logic_bluetooth_battery_notification_flag) == AT_BLE_SUCCESS)
            {
                DBG_LOG("Notif battery level:%d%%", logic_bluetooth_ble_battery_level);
                logic_bluetooth_battery_notification_flag = FALSE;
                logic_bluetooth_notif_being_sent = BATTERY_NOTIF_SENDING;
            }
            else
            {
                logic_bluetooth_notif_being_sent = NONE_NOTIF_SENDING;
            }
        }
    }
    
    /* If we just connected */
    if ((logic_bluetooth_can_communicate_with_host != FALSE) && (logic_bluetooth_can_communicate_with_host_prev == FALSE))
    {
        /* Inform main MCU */
        comms_main_mcu_send_simple_event(AUX_MCU_EVENT_BLE_CONNECTED);
    }
    
    /* If something is spamming us with connections */
    if ((logic_bluetooth_invalid_connect_counter > 10) && (logic_bluetooth_too_many_invalid_connect_notif_sent == FALSE))
    {
        /* Inform main MCU */
        comms_main_mcu_send_simple_event(AUX_MCU_EVENT_BLE_CON_SPAM);        
        logic_bluetooth_too_many_invalid_connect_notif_sent = TRUE;
    }
    
    /* Notifications just enabled */
    if (logic_bluetooth_notifications_just_enabled != FALSE)
    {
        logic_bluetooth_notifications_just_enabled = FALSE;
        
        /* Send empty report */
        logic_bluetooth_send_modifier_and_key(0, 0, 0);
    }
    
    /* Store previous state */
    logic_bluetooth_can_communicate_with_host_prev = logic_bluetooth_can_communicate_with_host;
    
    ble_event_task();
    logic_sleep_routine_ble_call();
    
    /* If we were asked to disable bluetooth */
    if (logic_bluetooth_disable_flag != FALSE)
    {
        logic_bluetooth_disable_flag = FALSE;
        logic_bluetooth_stop_bluetooth();
        logic_set_ble_disabled();
        timer_reset_callback_timers();
        comms_main_mcu_send_simple_event(AUX_MCU_EVENT_BLE_DISABLED);
    }
}