/*
 * logic_bluetooth.c
 *
 * Created: 02/06/2019 18:08:22
 *  Author: limpkin
 */ 
#include "platform_defines.h"
#include "logic_bluetooth.h"
#include "driver_timer.h"
#include "ble_manager.h"
#include "at_ble_api.h"
#include "hid_device.h"
#include "ble_utils.h"
#include "battery.h"
#include "defines.h"
#include "hid.h"
/* Control point notification structure */
hid_control_mode_ntf_t hid_control_point_value; 
/* Report notification structure */
hid_report_ntf_t report_ntf_info;
/* Boot notification structure */
hid_boot_ntf_t  boot_ntf_info;
/* Protocol mode notification structure */
hid_proto_mode_ntf_t hid_proto_mode_value;
/* HID profile structure for application */
hid_prf_info_t hid_prf_data;
/* HID profile structure for application */
hid_prf_info_t raw_hid_prf_data;
/* Keyboard character info to be displayed during demo */
uint8_t keyb_disp[12] = {0x0B, 0x08, 0x0F, 0x0F, 0x12, 0x2C, 0x04, 0x17, 0x10, 0x08, 0x0F, 0x28};
/* Keyboard character to be printed */
uint8_t keyb_id = 0;
/* Profile connection status */
BOOL logic_bluetooth_connected = FALSE;
/* Keyboard report value */
uint8_t app_keyb_report[8] = {0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
/* Raw Keyboard report value */
uint8_t raw_hid_data_in_buf[65];
uint8_t raw_hid_data_out_buf[65];
/* Keyboard key status */
volatile uint8_t key_status = 0;
/* Notification sent flag for battery char changed */
BOOL battery_notification_flag = TRUE;
/* Battery service handler */
bat_gatt_service_handler_t bas_service_handler;
/* Connection handle */
at_ble_handle_t ble_connection_handle;
/* Battery level advertised */
uint8_t ble_battery_level = 33;

static at_ble_status_t hid_custom_event(void *param)
{
    at_ble_status_t status = AT_BLE_SUCCESS;
    key_status = 1;
    return status;
}

/* Callback called during connection */
static at_ble_status_t hid_connect_cb(void *params)
{
    at_ble_handle_t *handle;
    handle = (at_ble_handle_t *)params;
    keyb_id = 0;
    logic_bluetooth_connected = TRUE;
    ALL_UNUSED(&handle);
    DBG_LOG("Connected to device");
    timer_start_timer(TIMER_BT_TESTS, 5000);

    /* From battery service */
    at_ble_connected_t *connected = (at_ble_connected_t *)params;
    ble_connection_handle = connected->handle;

    return AT_BLE_SUCCESS;
}

/* Callback called during disconnect */
static at_ble_status_t hid_disconnect_cb(void *params)
{
    at_ble_handle_t *handle;
    handle =(at_ble_handle_t *)params;
    keyb_id = 0;
    logic_bluetooth_connected = FALSE;
    ALL_UNUSED(&handle);
    DBG_LOG("Disconnected from device");

    /* From battery service */
    battery_notification_flag = TRUE;

    return AT_BLE_SUCCESS;
}

/* Callback registered for AT_BLE_PAIR_DONE event from stack */
static at_ble_status_t ble_paired_app_event(void *param)
{
	at_ble_pair_done_t *pair_done = (at_ble_pair_done_t *)param;
	if(pair_done->status == AT_BLE_SUCCESS)
	{
    	DBG_LOG("Paired to device");
		ALL_UNUSED(param);
		return pair_done->status;
	}
    else
    {
    	DBG_LOG("Failed pairing to device");
    }
	return pair_done->status;
}

/* Callback registered for AT_BLE_ENCRYPTION_STATUS_CHANGED event from stack */
static at_ble_status_t ble_enc_status_changed_app_event(void *param)
{
	at_ble_encryption_status_changed_t *encryption_status_changed = (at_ble_encryption_status_changed_t *)param;
	if(encryption_status_changed->status == AT_BLE_SUCCESS)
	{
    	DBG_LOG("Enc status changed success");
		ALL_UNUSED(param);
		return encryption_status_changed->status;
	}
    else
    {
        DBG_LOG("Enc status changed failed");
    }
	return encryption_status_changed->status;
}

/* Callback called when report send over the air */
static at_ble_status_t hid_notification_confirmed_cb(void *params)
{
    // TODO explore for what this is sent... keyboard/hid/battery
	at_ble_cmd_complete_event_t *notification_status;
	notification_status = (at_ble_cmd_complete_event_t *)params;
	DBG_LOG_DEV("Keyboard report send to host status %d", notification_status->status);

    /* From battery service */
    if(!notification_status->status)
    {
        battery_notification_flag = TRUE;
        DBG_LOG_DEV("sending notification to the peer success");
    }

	return AT_BLE_SUCCESS;
}

/* Callback registered for AT_BLE_CHARACTERISTIC_CHANGED event from stack */
static at_ble_status_t ble_char_changed_app_event(void *param)
{
    at_ble_characteristic_changed_t *char_handle = (at_ble_characteristic_changed_t *)param;
    at_ble_status_t status = bat_char_changed_event(&bas_service_handler, char_handle);
    if(status == AT_BLE_SUCCESS)
    {
        battery_notification_flag = TRUE;
    }
    else if (status == AT_BLE_PRF_NTF_DISABLED)
    {
        battery_notification_flag = FALSE;
    }

    return status;
}

static const ble_gap_event_cb_t hid_app_gap_handle = {
	.connected = hid_connect_cb,
	.disconnected = hid_disconnect_cb,
	.pair_done = ble_paired_app_event,
	.encryption_status_changed = ble_enc_status_changed_app_event
};

static const ble_gatt_server_event_cb_t hid_app_gatt_server_handle = {
	.notification_confirmed = hid_notification_confirmed_cb,
	.characteristic_changed = ble_char_changed_app_event
};

/* All BLE Manager Custom Event callback */
static const ble_custom_event_cb_t hid_custom_event_cb = {
	.custom_event = hid_custom_event
};

/* keyboard report */
static uint8_t hid_app_keyb_report_map[] =
{
   0x05, 0x01,		/* Usage Page (Generic Desktop)      */
   0x09, 0x06,		/* Usage (Keyboard)                  */
   0xA1, 0x01,		/* Collection (Application)          */
   0x85, 0x01,		/* REPORT ID (1) - MANDATORY         */ 
   0x05, 0x07,		/* Usage Page (Keyboard)             */
   0x19, 224,		/* Usage Minimum (224)               */
   0x29, 231,		/* Usage Maximum (231)               */
   0x15, 0x00,		/* Logical Minimum (0)               */
   0x25, 0x01,		/* Logical Maximum (1)               */
   0x75, 0x01,		/* Report Size (1)                   */
   0x95, 0x08,		/* Report Count (8)                  */
   0x81, 0x02,		/* Input (Data, Variable, Absolute)  */
   0x81, 0x01,		/* Input (Constant)                  */
   0x19, 0x00,		/* Usage Minimum (0)                 */
   0x29, 101,		/* Usage Maximum (101)               */
   0x15, 0x00,		/* Logical Minimum (0)               */
   0x25, 101,		/* Logical Maximum (101)             */
   0x75, 0x08,		/* Report Size (8)                   */
   0x95, 0x06,		/* Report Count (6)                  */
   0x81, 0x00,		/* Input (Data, Array)               */
   0x05, 0x08,		/* Usage Page (LED)                  */
   0x19, 0x01,		/* Usage Minimum (1)                 */
   0x29, 0x05,		/* Usage Maximum (5)                 */
   0x15, 0x00,		/* Logical Minimum (0)               */
   0x25, 0x01,		/* Logical Maximum (1)               */
   0x75, 0x01,		/* Report Size (1)                   */
   0x95, 0x05,		/* Report Count (5)                  */
   0x91, 0x02,		/* Output (Data, Variable, Absolute) */
   0x95, 0x03,		/* Report Count (3)                  */
   0x91, 0x01,		/* Output (Constant)                 */
   0xC0				/* End Collection                    */
};

/* raw keyboard report */
static uint8_t raw_hid_app_keyb_report_map[] =
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

/* Callback called when host change the control point value */
static void hid_prf_control_point_ntf_cb(hid_control_mode_ntf_t *hid_control_point_value_t)
{
	DBG_LOG_DEV("Control Point Notification Callback :: Service Instance %d Control Value %d", 
					hid_control_point_value_t->serv_inst, hid_control_point_value_t->control_value);
}

/* Callback called when host change the protocol mode value */
static void hid_prf_protocol_mode_ntf_cb(hid_proto_mode_ntf_t *protocol_mode)
{
	DBG_LOG_DEV("Protocol Mode Notification Callback :: Service Instance %d  New Protocol Mode  %d  Connection Handle %d", 
					protocol_mode->serv_inst, protocol_mode->mode, protocol_mode->conn_handle);
}

/* Callback called when host enable/disable the notification for boot report (Mouse/Keyboard)
   Mouse Boot Value 4
   Keyboard Boot Value 6
*/
static void hid_prf_boot_ntf_cb(hid_boot_ntf_t *boot_ntf_info_t)
{
	DBG_LOG_DEV("Boot Notification Callback :: Service Instance %d  Boot Value  %d  Notification(Enable/Disable) %d", 
					boot_ntf_info_t->serv_inst, boot_ntf_info_t->boot_value, boot_ntf_info_t->ntf_conf);
}

/* Callback called when host enable/disable the notification for report (Mouse/Keyboard) */
static void hid_prf_report_ntf_cb(hid_report_ntf_t *report_info)
{
	DBG_LOG_DEV("Report Notification Callback Service Instance %d  Report ID  %d  Notification(Enable/Disable) %d Connection Handle %d", 
					report_info->serv_inst, report_info->report_ID, report_info->ntf_conf, report_info->conn_handle);
    report_ntf_info.serv_inst = report_info->serv_inst;
	report_ntf_info.report_ID = report_info->report_ID;
	report_ntf_info.ntf_conf = report_info->ntf_conf;
	report_ntf_info.conn_handle = report_info->conn_handle;
}

/* Initialize the application information for HID profile*/
static void hid_mooltipass_app_init(void)
{
#ifdef ENABLE_PTS	
	uint16_t i=0;
#endif	
	hid_prf_data.hid_serv_instance = 1;
	hid_prf_data.hid_device = HID_KEYBOARD_MODE; 
	hid_prf_data.protocol_mode = HID_REPORT_PROTOCOL_MODE; 
	hid_prf_data.num_of_report = 1;
	
	/*Update the report information based on report id, User can allocate maximum HID_MAX_REPORT_NUM number of report*/
	hid_prf_data.report_id[0] = 1;  
	hid_prf_data.report_type[0] = INPUT_REPORT;  
	
	hid_prf_data.report_val[0] = &app_keyb_report[0];	
	hid_prf_data.report_len[0] = sizeof(app_keyb_report);
	hid_prf_data.report_map_info.report_map = hid_app_keyb_report_map;
	hid_prf_data.report_map_info.report_map_len = sizeof(hid_app_keyb_report_map);
	hid_prf_data.hid_device_info.bcd_hid = 0x0111;        
	hid_prf_data.hid_device_info.bcountry_code = 0x00;
	hid_prf_data.hid_device_info.flags = 0x02; 
	
#ifdef ENABLE_PTS
	DBG_LOG_PTS("Report Map Characteristic Value");
	DBG_LOG_PTS("\r\n");
	for (i=0; i<sizeof(hid_app_keyb_report_map); i++)
	{
		DBG_LOG_PTS(" 0x%02X ", hid_app_keyb_report_map[i]);
	}
	DBG_LOG_PTS("\r\n");	
	DBG_LOG_PTS("HID Information Characteristic Value");
	DBG_LOG_PTS("bcdHID 0x%02X, bCountryCode 0x%02X Flags 0x%02X", hid_prf_data.hid_device_info.bcd_hid, hid_prf_data.hid_device_info.bcountry_code, hid_prf_data.hid_device_info.flags);
#endif // _DEBUG
	
	if(hid_prf_conf(&hid_prf_data)==HID_PRF_SUCESS){
		DBG_LOG("HID Profile Configured");
	}else{
		DBG_LOG("HID Profile Configuration Failed");
	}
    //return;
    
    /* Now to the RAW HID endpoint */    
    raw_hid_prf_data.hid_serv_instance = 2;
    raw_hid_prf_data.hid_device = HID_RAW_MODE;
    //raw_hid_prf_data.hid_device = HID_KEYBOARD_MODE;
    raw_hid_prf_data.protocol_mode = HID_REPORT_PROTOCOL_MODE;
    raw_hid_prf_data.num_of_report = 2;
    
    /*Update the report information based on report id, User can allocate maximum HID_MAX_REPORT_NUM number of report*/
    raw_hid_prf_data.report_id[0] = 2;
    raw_hid_prf_data.report_type[0] = INPUT_REPORT;    
    raw_hid_prf_data.report_val[0] = &raw_hid_data_out_buf[0];
    raw_hid_prf_data.report_len[0] = sizeof(raw_hid_data_out_buf);
    raw_hid_prf_data.report_id[1] = 3;
    raw_hid_prf_data.report_type[1] = OUTPUT_REPORT;
    raw_hid_prf_data.report_val[1] = &raw_hid_data_in_buf[0];
    raw_hid_prf_data.report_len[1] = sizeof(raw_hid_data_in_buf);
    raw_hid_prf_data.report_map_info.report_map = raw_hid_app_keyb_report_map;
    raw_hid_prf_data.report_map_info.report_map_len = sizeof(raw_hid_app_keyb_report_map);
    raw_hid_prf_data.hid_device_info.bcd_hid = 0x0111;
    raw_hid_prf_data.hid_device_info.bcountry_code = 0x00;
    raw_hid_prf_data.hid_device_info.flags = 0x02;
    
    #ifdef ENABLE_PTS
    DBG_LOG_PTS("Report Map Characteristic Value");
    DBG_LOG_PTS("\r\n");
    for (i=0; i<sizeof(raw_hid_app_keyb_report_map); i++)
    {
        DBG_LOG_PTS(" 0x%02X ", hid_app_keyb_report_map[i]);
    }
    DBG_LOG_PTS("\r\n");
    DBG_LOG_PTS("HID Information Characteristic Value");
    DBG_LOG_PTS("bcdHID 0x%02X, bCountryCode 0x%02X Flags 0x%02X", raw_hid_prf_data.hid_device_info.bcd_hid, raw_hid_prf_data.hid_device_info.bcountry_code, raw_hid_prf_data.hid_device_info.flags);
    #endif // _DEBUG
    
    if(hid_prf_conf(&raw_hid_prf_data)==HID_PRF_SUCESS){
        DBG_LOG("RAW HID Profile Configured");
        }else{
        DBG_LOG("RAW HID Profile Configuration Failed");
    }
}

void logic_bluetooth_start_bluetooth(void)
{
    DBG_LOG("Initializing HID Keyboard Application");
    
    /* Initialize the profile based on user input */
    hid_mooltipass_app_init();
    
    /* initialize the ble chip  and Set the device mac address */
    ble_device_init(NULL);    
    
    /* Initialize the battery service */
    bat_init_service(&bas_service_handler, &ble_battery_level);
    
    /* Define the primary service in the GATT server database */
    if(bat_primary_service_define(&bas_service_handler)!= AT_BLE_SUCCESS)
    {
        DBG_LOG("defining battery service failed");
    }
    
    hid_prf_init(NULL);
    
    /* Register the notification handler */
    notify_report_ntf_handler(hid_prf_report_ntf_cb);
    notify_boot_ntf_handler(hid_prf_boot_ntf_cb);
    notify_protocol_mode_handler(hid_prf_protocol_mode_ntf_cb);
    notify_control_point_handler(hid_prf_control_point_ntf_cb);
    
    /* Callback registering for BLE-GAP Role */
    ble_mgr_events_callback_handler(REGISTER_CALL_BACK,
    BLE_GAP_EVENT_TYPE,
    &hid_app_gap_handle);
    
    /* Callback registering for BLE-GATT-Server Role */
    ble_mgr_events_callback_handler(REGISTER_CALL_BACK,
    BLE_GATT_SERVER_EVENT_TYPE,
    &hid_app_gatt_server_handle);
    
    /* Register callbacks for custom related events */
    ble_mgr_events_callback_handler(REGISTER_CALL_BACK,
    BLE_CUSTOM_EVENT_TYPE,
    &hid_custom_event_cb);
    
    /* Capturing the events  */
    while(FALSE)
    {
        ble_event_task();
        
        /* Check for key status */
        if(key_status && logic_bluetooth_connected){
            DBG_LOG("Key Pressed...");
            //delay_ms(KEY_PAD_DEBOUNCE_TIME);
            if((keyb_id == POSITION_ZERO) || (keyb_id == POSITION_SIX)){
                app_keyb_report[0] = CAPS_ON;
                }else{
                app_keyb_report[0] = CAPS_OFF;
            }
            
            app_keyb_report[2] = keyb_disp[keyb_id];
            hid_prf_report_update(report_ntf_info.conn_handle, report_ntf_info.serv_inst, 1, app_keyb_report, sizeof(app_keyb_report));
            //delay_ms(20);
            app_keyb_report[2] = 0x00;
            hid_prf_report_update(report_ntf_info.conn_handle, report_ntf_info.serv_inst, 1, app_keyb_report, sizeof(app_keyb_report));
            
            key_status = 0;
            
            if(keyb_id == MAX_TEXT_LEN)
            {
                keyb_id = 0;
            }
            else
            {
                ++keyb_id;
            }
        }
    }
}

void logic_bluetooth_routine(void)
{
    ble_event_task();
    
    if (logic_bluetooth_connected != FALSE)
    {
        if (timer_has_timer_expired(TIMER_BT_TESTS, TRUE) == TIMER_EXPIRED)
        {            
            timer_start_timer(TIMER_BT_TESTS, 5000);            
            app_keyb_report[2] = 17;
            hid_prf_report_update(report_ntf_info.conn_handle, report_ntf_info.serv_inst, 1, app_keyb_report, sizeof(app_keyb_report));
            timer_delay_ms(20);
            app_keyb_report[2] = 0x00;
            hid_prf_report_update(report_ntf_info.conn_handle, report_ntf_info.serv_inst, 1, app_keyb_report, sizeof(app_keyb_report));
        }
    }
}

void logic_bluetoot_set_battery_level(uint8_t pct)
{
    ble_battery_level = pct;

    /* send the notification and Update the battery level  */
    if(battery_notification_flag)
    {
        if(bat_update_char_value(ble_connection_handle,&bas_service_handler, pct, battery_notification_flag) == AT_BLE_SUCCESS)
        {
            DBG_LOG("Battery Level:%d%%", pct);
            battery_notification_flag = FALSE;
        }
    }
}