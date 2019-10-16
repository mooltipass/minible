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
#include "ble_utils.h"
#include "platform.h"
#include "battery.h"
#include "defines.h"
// Appearance: 0x03C1 (keyboard)
// Dev name perm: disable writing device name
// Device name: Mooltipass Mini BLE
// Minimum connection interval N (Value Time = N *1.25 ms): 6 = 7.5ms (taken from logitech craft)
// Maximum connection interval N (Value Time = N *1.25 ms): 9 = 11.25ms (taken from logitech craft)
// Slave preferred Connection latency (number of events): 44 (taken from logitech craft)
// Slave preferred Link supervision time-out (Value Time = N * 10 ms): 216 (taken from logitech craft)
at_ble_gap_deviceinfo_t logic_bluetooth_advanced_info = {.appearance=0x03C1, .dev_name_perm=AT_BLE_WRITE_DISABLE, .name.length=20, .name.value="Mooltipass Mini BLE", .slv_params.con_intv_min=6, .slv_params.con_intv_max=9, .slv_params.con_latency=44, .slv_params.superv_to=216};
/* Control point notification structure for HID services*/
hid_control_mode_ntf_t logic_bluetooth_hid_control_point_value[HID_MAX_SERV_INST];
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
uint8_t logic_bluetooth_boot_keyb_out_report[1];
uint8_t logic_bluetooth_boot_keyb_in_report[8];
uint8_t logic_bluetooth_keyboard_in_report[8];
uint8_t logic_bluetooth_ctrl_point[1];
/* Bluetooth connection bools */
BOOL logic_bluetooth_advertising = FALSE;
BOOL logic_bluetooth_just_paired = FALSE;
BOOL logic_bluetooth_connected = FALSE;
BOOL logic_bluetooth_paired = FALSE;


static at_ble_status_t hid_custom_event(void *param)
{
    at_ble_status_t status = AT_BLE_SUCCESS;
    return status;
}

/*! \fn     logic_bluetooth_hid_connected_callback(void* params)
*   \brief  Called during device connection
*/
static at_ble_status_t logic_bluetooth_hid_connected_callback(void* params)
{
    DBG_LOG("Connected to device");
    
    /* Set booleans */
    logic_bluetooth_connected = TRUE;
    
    /* Store connection handle */
    at_ble_connected_t* connected = (at_ble_connected_t*)params;
    logic_bluetooth_ble_connection_handle = connected->handle;

    return AT_BLE_SUCCESS;
}

/*! \fn     logic_bluetooth_hid_disconnected_callback(void* params)
*   \brief  Called during device disconnection
*/
static at_ble_status_t logic_bluetooth_hid_disconnected_callback(void *params)
{
    DBG_LOG("Disconnected from device");
    
    /* Set booleans */
    logic_bluetooth_just_paired = FALSE;
    logic_bluetooth_connected = FALSE;
    logic_bluetooth_paired = FALSE;

    /* From battery service */
    logic_bluetooth_battery_notification_flag = TRUE;    
    
    /* Start advertising again */
    logic_bluetooth_start_advertising();

    return AT_BLE_SUCCESS;
}

/*! \fn     logic_bluetooth_hid_paired_callback(void* params)
*   \brief  Called during device pairing
*/
static at_ble_status_t logic_bluetooth_hid_paired_callback(void* param)
{
    at_ble_pair_done_t* pair_done = (at_ble_pair_done_t*)param;
    
    if(pair_done->status == AT_BLE_SUCCESS)
    {
        DBG_LOG("Paired to device");
        
        /* Set booleans */
        logic_bluetooth_just_paired = TRUE;
        logic_bluetooth_paired = TRUE;
    }
    else
    {
        DBG_LOG("ERROR: Failed pairing to device");
    }
    
    return pair_done->status;
}

/*! \fn     logic_bluetooth_encryption_changed_callback(void* params)
*   \brief  Called during encryption status change
*/
static at_ble_status_t logic_bluetooth_encryption_changed_callback(void *param)
{
    at_ble_encryption_status_changed_t *encryption_status_changed = (at_ble_encryption_status_changed_t *)param;
    if(encryption_status_changed->status == AT_BLE_SUCCESS)
    {
        DBG_LOG("Enc status changed success");
        return encryption_status_changed->status;
    }
    else
    {
        DBG_LOG("ERROR: Enc status changed failed");
    }
    return encryption_status_changed->status;
}

/* Callbacks for GAP */
static const ble_gap_event_cb_t hid_app_gap_handle = 
{
    .connected = logic_bluetooth_hid_connected_callback,
    .disconnected = logic_bluetooth_hid_disconnected_callback,
    .pair_done = logic_bluetooth_hid_paired_callback,
    .encryption_status_changed = logic_bluetooth_encryption_changed_callback
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
            DBG_LOG("%02x %02x %02x %02x %02x", change_params.char_new_value[0], change_params.char_new_value[1], change_params.char_new_value[2], change_params.char_new_value[3], change_params.char_new_value[4]);
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
            logic_bluetooth_report_ntf_info[report_id].serv_inst = serv_inst;
            logic_bluetooth_report_ntf_info[report_id].report_ID = report_id;
            logic_bluetooth_report_ntf_info[report_id].conn_handle = change_params.conn_handle;
            logic_bluetooth_report_ntf_info[report_id].ntf_conf = (change_params.char_new_value[1]<<8 | change_params.char_new_value[0]);
            DBG_LOG("Report Notification Callback Service Instance %d  Report ID  %d  Notification(Enable/Disable) %d Connection Handle %d", serv_inst, report_id, logic_bluetooth_report_ntf_info[report_id].ntf_conf, change_params.conn_handle);
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
    0x05, 0x01,                         // Usage Page (Generic Desktop),
    0x09, 0x06,                         // Usage (Keyboard),
    0xA1, 0x01,                         // Collection (Application),
    0x85, 0x01,                         // REPORT ID (1) - MANDATORY
    0x75, 0x01,                         //   Report Size (1),
    0x95, 0x08,                         //   Report Count (8),
    0x05, 0x07,                         //   Usage Page (Key Codes),
    0x19, 0xE0,                         //   Usage Minimum (224),
    0x29, 0xE7,                         //   Usage Maximum (231),
    0x15, 0x00,                         //   Logical Minimum (0),
    0x25, 0x01,                         //   Logical Maximum (1),
    0x81, 0x02,                         //   Input (Data, Variable, Absolute), ;Modifier byte
    0x95, 0x01,                         //   Report Count (1),
    0x75, 0x08,                         //   Report Size (8),
    0x81, 0x03,                         //   Input (Constant),                 ;Reserved byte
    0x95, 0x05,                         //   Report Count (5),
    0x75, 0x01,                         //   Report Size (1),
    0x05, 0x08,                         //   Usage Page (LEDs),
    0x19, 0x01,                         //   Usage Minimum (1),
    0x29, 0x05,                         //   Usage Maximum (5),
    0x91, 0x02,                         //   Output (Data, Variable, Absolute), ;LED report
    0x95, 0x01,                         //   Report Count (1),
    0x75, 0x03,                         //   Report Size (3),
    0x91, 0x03,                         //   Output (Constant),                 ;LED report padding
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
    0x85, 0x02,                         // Report ID 2
    0x75, 0x08,                         // report size = 8 bits
    0x15, 0x00,                         // logical minimum = 0
    0x26, 0xFF, 0x00,                   // logical maximum = 255
    0x95, USB_RAWHID_TX_SIZE,           // report count
    0x09, 0x01,                         // usage
    0x81, 0x02,                         // Input (array)
    0xC0,                               // end collection (logical)    
    0xA1, 0x02,                         // Collection (logical)
    0x85, 0x03,                         // Report ID 3
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

/*! \fn     logic_bluetooth_update_report(uint16_t conn_handle, uint8_t serv_inst, uint8_t reportid, uint8_t *report, uint16_t len)
*   \brief  Function to update the report
*   \param  conn_handle Connection handle
*   \param  serv_inst   Service Instance
*   \param  report_id   Report ID
*   \param  report      Report to be send
*   \param  len         Length of report
*/
void logic_bluetooth_update_report(uint16_t conn_handle, uint8_t serv_inst, uint8_t reportid, uint8_t* report, uint16_t len)
{
    // TODO: should we check for notification subscription?
    uint16_t status = 0;
    uint8_t id;
    
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
    
    DBG_LOG_LOGIC_BT_AD("Report map characteristic: %d", cur_characteristic_index);
    /*Configure HID Report Map Characteristic : Value related info*/
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.handle = 0;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.type = AT_BLE_UUID_16;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[0] =(uint8_t) HID_UUID_CHAR_REPORT_MAP;
    logic_bluetooth_hid_gatt_instances[servinst].serv_chars[cur_characteristic_index].char_val.uuid.uuid[1] = (uint8_t)(HID_UUID_CHAR_REPORT_MAP >> 8);
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
    
    if(device == HID_KEYBOARD_MODE)
    {
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
    }
 
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
    /* Configure the HID characteristic value permission */    
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

/*! \fn     logic_bluetooth_start_bluetooth(void)
*   \brief  Start bluetooth
*/
void logic_bluetooth_start_bluetooth(void)
{
    DBG_LOG("Starting Bluetooth...");
    
    /* Setup Keyboard HID profile data */
    logic_bluetooth_hid_prf_data.hid_serv_instance = BLE_KEYBOARD_HID_SERVICE_INSTANCE;
    logic_bluetooth_hid_prf_data.hid_device = HID_KEYBOARD_MODE;
    logic_bluetooth_hid_prf_data.protocol_mode = HID_REPORT_PROTOCOL_MODE;
    logic_bluetooth_hid_prf_data.num_of_report = 1;
    logic_bluetooth_hid_prf_data.report_id[0] = BLE_KEYBOARD_HID_IN_REPORT_NB;
    logic_bluetooth_hid_prf_data.report_type[0] = INPUT_REPORT;
    logic_bluetooth_hid_prf_data.report_val[0] = &logic_bluetooth_keyboard_in_report[0];
    logic_bluetooth_hid_prf_data.report_len[0] = sizeof(logic_bluetooth_keyboard_in_report);
    logic_bluetooth_hid_prf_data.report_map_info.report_map = logic_bluetooth_keyb_report_map;
    logic_bluetooth_hid_prf_data.report_map_info.report_map_len = sizeof(logic_bluetooth_keyb_report_map);
    logic_bluetooth_hid_prf_data.hid_device_info.bcd_hid = 0x0111;
    logic_bluetooth_hid_prf_data.hid_device_info.bcountry_code = 0x00;
    logic_bluetooth_hid_prf_data.hid_device_info.flags = 0x02;
    
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
    logic_bluetooth_raw_hid_prf_data.hid_device_info.flags = 0x02;
    
    /* Update HID profile data reference */
    hid_prf_dataref[BLE_RAW_HID_SERVICE_INSTANCE] = &logic_bluetooth_raw_hid_prf_data;
    
    /* IO inits for BLE enabling */
    platform_io_ble_enabled_inits();
    
    /* Initialize the ble chip and set the device mac address */
    ble_device_init(NULL, &logic_bluetooth_advanced_info);    
    
    /* Initialize the battery service */
    bat_init_service(&logic_bluetooth_bas_service_handler, &logic_bluetooth_ble_battery_level);
    
    /* Define the primary service in the GATT server database */
    if(bat_primary_service_define(&logic_bluetooth_bas_service_handler) != AT_BLE_SUCCESS)
    {
        DBG_LOG("ERROR: Defining battery service failed");
    }
    
    /* Initialize HID services */
    for(uint16_t serv_num = 0; serv_num <= BLE_RAW_HID_SERVICE_INSTANCE; serv_num++)
    {
        /* Clear service */
        memset(&logic_bluetooth_hid_serv_instances[serv_num], 0, sizeof(logic_bluetooth_hid_serv_instances[0]));
        
        /* Initialize HID profile based on previously provided input */
        logic_bluetooth_hid_profile_init(serv_num, hid_prf_dataref[serv_num]->hid_device, &hid_prf_dataref[serv_num]->protocol_mode, hid_prf_dataref[serv_num]->num_of_report, (uint8_t *)&hid_prf_dataref[serv_num]->report_type, &(hid_prf_dataref[serv_num]->report_val[0]), (uint8_t *)&hid_prf_dataref[serv_num]->report_len, &hid_prf_dataref[serv_num]->hid_device_info);
                
        /* Map report map with correct characteristic */
        if (serv_num == BLE_KEYBOARD_HID_SERVICE_INSTANCE)
        {
            logic_bluetooth_hid_gatt_instances[serv_num].serv_chars[1].char_val.init_value = hid_prf_dataref[serv_num]->report_map_info.report_map;
            logic_bluetooth_hid_gatt_instances[serv_num].serv_chars[1].char_val.max_len = hid_prf_dataref[serv_num]->report_map_info.report_map_len;
            logic_bluetooth_hid_gatt_instances[serv_num].serv_chars[1].char_val.len = hid_prf_dataref[serv_num]->report_map_info.report_map_len;
        }
        else
        {
            logic_bluetooth_hid_gatt_instances[serv_num].serv_chars[0].char_val.init_value = hid_prf_dataref[serv_num]->report_map_info.report_map;
            logic_bluetooth_hid_gatt_instances[serv_num].serv_chars[0].char_val.max_len = hid_prf_dataref[serv_num]->report_map_info.report_map_len;
            logic_bluetooth_hid_gatt_instances[serv_num].serv_chars[0].char_val.len = hid_prf_dataref[serv_num]->report_map_info.report_map_len;
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
    
    /* Initialize the device information service */    
    dis_gatt_service_handler_t device_info_serv;
    dis_init_service(&device_info_serv);
    
    /* Define the primary service in the GATT server database */
    if(dis_primary_service_define(&device_info_serv) != AT_BLE_SUCCESS)
    {
        DBG_LOG("ERROR: Defining device information service failed");
    }
    
    /*  Set advertisement data from ble_manager */
    if(!(ble_advertisement_data_set() == AT_BLE_SUCCESS))
    {
        DBG_LOG("ERROR: Fail to set Advertisement data");
    }
    
    /* Callback registering for BLE-GAP Role */
    ble_mgr_events_callback_handler(REGISTER_CALL_BACK, BLE_GAP_EVENT_TYPE, &hid_app_gap_handle);
    
    /* Callback registering for BLE-GATT-Server Role */
    ble_mgr_events_callback_handler(REGISTER_CALL_BACK, BLE_GATT_SERVER_EVENT_TYPE, &hid_app_gatt_server_handle);   
    
    /* Register callbacks for custom related events */
    ble_mgr_events_callback_handler(REGISTER_CALL_BACK, BLE_CUSTOM_EVENT_TYPE, &hid_custom_event_cb);
    
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
    logic_bluetooth_ble_battery_level = pct;

    /* send the notification and Update the battery level  */
    if(logic_bluetooth_battery_notification_flag)
    {
        if(bat_update_char_value(logic_bluetooth_ble_connection_handle,&logic_bluetooth_bas_service_handler, pct, logic_bluetooth_battery_notification_flag) == AT_BLE_SUCCESS)
        {
            DBG_LOG("Battery Level:%d%%", pct);
            logic_bluetooth_battery_notification_flag = FALSE;
            logic_bluetooth_notif_being_sent = BATTERY_NOTIF_SENDING;
        }
        else
        {
            logic_bluetooth_notif_being_sent = NONE_NOTIF_SENDING;            
        }
    }
}

/*! \fn     logic_bluetooth_raw_send(uint8_t* data, uint16_t data_len)
*   \brief  Send raw data through bluetooth
*   \param  data        Pointer to the data
*   \param  data_len    Data length
*/
void logic_bluetooth_raw_send(uint8_t* data, uint16_t data_len)
{
    DBG_LOG_LOGIC_BT_AD("Call to raw send");
    
    /* Check that we're actually paired */
    if (logic_bluetooth_paired != FALSE)
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
        DBG_LOG("BLE send: %02x %02x %02x%02x %02x%02x", logic_bluetooth_raw_hid_data_out_buf[0], logic_bluetooth_raw_hid_data_out_buf[1], logic_bluetooth_raw_hid_data_out_buf[3], logic_bluetooth_raw_hid_data_out_buf[2], logic_bluetooth_raw_hid_data_out_buf[5], logic_bluetooth_raw_hid_data_out_buf[4]);
        
        /* Send data */
        logic_bluetooth_notif_being_sent = RAW_HID_NOTIF_SENDING;
        logic_bluetooth_update_report(logic_bluetooth_ble_connection_handle, BLE_RAW_HID_SERVICE_INSTANCE, BLE_RAW_HID_IN_REPORT_NB, logic_bluetooth_raw_hid_data_out_buf, sizeof(logic_bluetooth_raw_hid_data_out_buf));
    }
}

/*! \fn     logic_bluetooth_routine(void)
*   \brief  Our bluetooth routine
*/
void logic_bluetooth_routine(void)
{
    ble_event_task();
    logic_sleep_routine_ble_call();
    
    if (logic_bluetooth_just_paired != FALSE)
    {
        logic_bluetooth_just_paired = FALSE;
        timer_start_timer(TIMER_BT_TESTS, 5000);
    }
    
    if (logic_bluetooth_connected != FALSE)
    {
        if ((timer_has_timer_expired(TIMER_BT_TESTS, TRUE) == TIMER_EXPIRED) && false)
        {            
            timer_start_timer(TIMER_BT_TESTS, 5000);            
            logic_bluetooth_keyboard_in_report[2] = 17;
            logic_bluetooth_update_report(logic_bluetooth_ble_connection_handle, BLE_KEYBOARD_HID_SERVICE_INSTANCE, BLE_KEYBOARD_HID_IN_REPORT_NB, logic_bluetooth_keyboard_in_report, sizeof(logic_bluetooth_keyboard_in_report));
            timer_delay_ms(20);
            logic_bluetooth_keyboard_in_report[2] = 0x00;
            logic_bluetooth_update_report(logic_bluetooth_ble_connection_handle, BLE_KEYBOARD_HID_SERVICE_INSTANCE, BLE_KEYBOARD_HID_IN_REPORT_NB, logic_bluetooth_keyboard_in_report, sizeof(logic_bluetooth_keyboard_in_report));
        }
    }
}