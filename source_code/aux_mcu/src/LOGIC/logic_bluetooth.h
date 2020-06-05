/*
 * logic_bluetooth.h
 *
 * Created: 02/06/2019 18:08:31
 *  Author: limpkin
 */ 


#ifndef LOGIC_BLUETOOTH_H_
#define LOGIC_BLUETOOTH_H_

#include "ble_manager.h"
#include "at_ble_api.h"
#include "defines.h"

/* Typedefs */
typedef enum    {NONE_NOTIF_SENDING = 0, KEYBOARD_NOTIF_SENDING, RAW_HID_NOTIF_SENDING, BATTERY_NOTIF_SENDING} notif_sending_te;

/* Defines */
#define BLE_KEYBOARD_HID_SERVICE_INSTANCE   0
#define BLE_RAW_HID_SERVICE_INSTANCE        1
#define BLE_MOUSE_HID_IN_REPORT_NB          1
#define BLE_KEYBOARD_HID_IN_REPORT_NB       2
#define BLE_RAW_HID_IN_REPORT_NB            3
#define BLE_RAW_HID_OUT_REPORT_NB           4
#define BLE_TOTAL_NUMBER_OF_REPORTS         3
#define BLE_MAX_REPORTS_FOR_GIVEN_SVC       2
#define HID_MAX_SERV_INST				    2
#define HID_MAX_CHARACTERISTIC              9

/** @brief APP_HID_FAST_ADV between 0x0020 and 0x4000 in 0.625 ms units (20ms to 10.24s). */
//	<o> Fast Advertisement Interval <100-1000:50>
//	<i> Defines interval of Fast advertisement in ms.
//	<i> Default: 100
//	<id> hid_fast_adv
#define APP_HID_FAST_ADV				(500) //312 ms

/** @brief APP_HID_ADV_TIMEOUT Advertising time-out between 0x0001 and 0x028F in seconds, 0x0000 disables time-out.*/
//	<o> Advertisement Timeout <1-655>
//	<i> Defines interval at which advertisement timeout in sec.
//	<i> Default: 655
//	<id> hid_adv_timeout
#define APP_HID_ADV_TIMEOUT				(0) // no timeout!

/** @brief scan_resp_len is the length of the scan response data */
//	<o> Scan Response Buffer <1-20>
//	<i> Defines size of buffer for scan response.
//	<i> Default: 10
//	<id> hid_scan_resp_len
#define SCAN_RESP_LEN					(10)

/** @brief Advertisement data appearance len */
#define ADV_DATA_APPEARANCE_LEN			(2)

/** @brief Advertisement appearance type  */
#define ADV_DATA_APPEARANCE_TYPE		(0x19)

/** @brief Advertisement data name appearance len */
#define ADV_DATA_NAME_LEN				(9)

/** @brief Advertisement data name type */
#define ADV_DATA_NAME_TYPE				(0x09)

//	<s.9>	Advertising String
//	<i>	String Descriptor describing in advertising packet.
//	<id> hid_sensor_adv_data_name_data
#define ADV_DATA_NAME_DATA				("Mooltipass_BLE")

/** @brief Advertisement data UUID length */
#define ADV_DATA_UUID_LEN				(2)

/** @brief Advertisement type length */
#define ADV_TYPE_LEN					(0x01)

/** @brief Advertisement UUID type */
#define ADV_DATA_UUID_TYPE				(0x03)


/****************************************************************************************
*							        Enumerations	                                   	*
****************************************************************************************/
typedef struct hid_gatt_serv_handler
{
    at_ble_service_t		  serv;
    at_ble_chr_t		      serv_chars[HID_MAX_CHARACTERISTIC];
    at_ble_generic_att_desc_t serv_desc[BLE_MAX_REPORTS_FOR_GIVEN_SVC];   /*Report descriptor*/
}hid_gatt_serv_handler_t;

/**@brief HID characteristic
*/
typedef enum
{
    PROTOCOL_MODE,
    REPORT_MAP,
    CHAR_REPORT,
    CHAR_REPORT_CCD,
    BOOT_MOUSE_INPUT_REPORT,
    BOOT_KEY_OUTPUT_REPORT,
    BOOT_KEY_INPUT_REPORT,
    BOOT_KEY_INPUT_CCD,
    HID_INFORMATION,
    CONTROL_POINT,
}hid_char_type_t;

/**@brief HID Profile error code
*/
typedef enum
{
    HID_PRF_SUCESS = 0,
    HID_PRF_INSTANCE_OUT_RANGE,
    HID_PRF_NO_INSTANCE,
}hid_prf_error_t;

/****************************************************************************************
*							        Structures                                     		*
****************************************************************************************/
/**@brief HID control point info, notify to user when control point change by user. */
typedef struct hid_control_mode_ntf
{
    uint8_t serv_inst;        /**< Service instance number. */
    uint16_t control_value;   /**< Suspend or exit suspend. */
}hid_control_mode_ntf_t;

/**@brief HID protocol mode info, notify to user when protocol mode change by user. */
typedef struct hid_proto_mode_ntf
{
    at_ble_handle_t conn_handle; /**< Connection Handle. */
    uint8_t serv_inst;			/**< Service instance number. */
    uint16_t mode;				/**< Protocol mode or report mode. */
}hid_proto_mode_ntf_t;

/**@brief HID boot notification or indication, notify to user. */
typedef struct hid_boot_ntf
{
    uint8_t serv_inst;				/**< Service instance number. */
    uint8_t boot_value;				/**< Keyboard or mouse. */
    uint16_t ntf_conf;				/**< Notification or Indication status. */
    at_ble_handle_t conn_handle;	/**< Connection Handle. */
}hid_boot_ntf_t;

/**@brief HID report notification or indication, notify to user. */
typedef struct hid_report_ntf
{
    uint8_t serv_inst;				/**< Service instance number. */
    uint8_t report_ID;				/**< Report ID to be notify or indicated. */
    uint16_t ntf_conf;				/**< Notification or Indication status. */
    at_ble_handle_t conn_handle;	/**< Connection Handle. */
}hid_report_ntf_t;

/**@brief HID info of device. */
typedef struct hid_info
{
    uint16_t bcd_hid;				/**< Version number. */
    uint8_t  bcountry_code;			/**< Country code. */
    uint8_t  flags;					/**< Wakeup and connectable info. */
}hid_info_t;

/**@brief HID Report Reference Descriptor. */
typedef struct hid_report_ref
{
    uint8_t  report_id;				/**< Country code. */
    uint8_t  report_type;           /**< Wakeup and connectable info. */
}hid_report_ref_t;

/**@brief HID Report Map Information. */
typedef struct hid_report_map
{
    uint8_t *report_map;			/**< Report map info. */
    uint16_t report_map_len;	    /**< Report map length. */
}hid_report_map_t;

/**@brief HID profile application info. */
typedef struct
{
    uint16_t serv_handle_info;                   /**< Service handle information. */
    uint8_t hid_serv_instance;                   /**< Number of HID service instance. */
    uint8_t hid_device;                          /**< Type of HID device. */
    uint8_t protocol_mode;						 /**< Protocol mode selected by user. */
    uint8_t num_of_report;						 /**< Number of report. */
    uint8_t report_id[BLE_MAX_REPORTS_FOR_GIVEN_SVC];       /**< Report id based on number of report. */
    uint8_t report_type[BLE_MAX_REPORTS_FOR_GIVEN_SVC];     /**< Report type based on number of report. */
    uint8_t *report_val[BLE_MAX_REPORTS_FOR_GIVEN_SVC];     /**< Report value based on number of report. */
    uint8_t report_len[BLE_MAX_REPORTS_FOR_GIVEN_SVC];		 /**< Report Length based on number of report. */
    hid_report_map_t report_map_info;            /**< Report map information based on device. */
    hid_info_t hid_device_info;					 /**< HID information. */
}hid_prf_info_t;

/**@brief Error Code for HID service operation
*/
typedef enum
{
    /// HID Service Registration Failed
    HID_SERV_FAILED = -1,    
    /// HID Service Operation Pass
    HID_SERV_SUCESS = 0,    
    /// HID Service Registration Failed
    HID_SERV_REG_FAILED,    
    /// HID Report ID Not Found
    HID_INVALID_INST = 0xFF    
}hid_serv_status;


/**@brief Protocol Mode Value
*/
typedef enum
{
    /// Enable boot protocol mode
    HID_MOUSE_MODE    = 1,    
    /// Enable report protocol mode
    HID_KEYBOARD_MODE,    
    HID_RAW_MODE
}hid_dev_type;

/**@brief Protocol Mode Value
*/
typedef enum
{
    /// Enable boot protocol mode
    HID_BOOT_PROTOCOL_MODE    = 0,    
    /// Enable report protocol mode
    HID_REPORT_PROTOCOL_MODE,
}hid_proto_mode;

/**@brief HID control point
*/
typedef enum
{
    /// Enable suspend mode
    SUSPEND    = 0,    
    /// Exit suspend mode
    EXIT_SUSPEND,
}hid_control_mode;

/**@brief HID control point
*/
typedef enum
{
    /// Input report
    INPUT_REPORT    = 1,    
    /// Input report
    OUTPUT_REPORT,    
    /// Input report
    FEATURE_REPORT,
}hid_report_type;

/****************************************************************************************
*							        Structures                                     		*
****************************************************************************************/

/**@brief HID service instance for device role. */
typedef struct hid_serv
{
    at_ble_handle_t *hid_dev_serv_handle;				/**< HID service handle. */
    at_ble_uuid_t *hid_dev_serv_uuid;					/**< HID service info (0x1812). */
    at_ble_chr_t *hid_dev_proto_mode_char;				/**< HID protocol mode (0x2A4E). */
    at_ble_chr_t *hid_dev_report_map_char;				/**< HID report map (0x2A4B). */
    at_ble_chr_t *hid_dev_report_val_char[BLE_MAX_REPORTS_FOR_GIVEN_SVC];      /**< HID report value (0x2A4D). */    
    at_ble_chr_t *hid_dev_boot_keyboard_in_report;   /**< HID boot keyboard input report (0x2A22). */
    at_ble_chr_t *hid_dev_boot_keyboard_out_report;   /**< HID boot keyboard output report (0x2A32). */    
    at_ble_chr_t *hid_dev_info;						/**< HID information (0x2A4A). */
    at_ble_chr_t *hid_control_point;				/**< HID information (0x2A4C). */
} hid_serv_t;

/**@brief HID report reference descriptor instance for device role. */
typedef struct hid_reportref_desc
{
    at_ble_handle_t reportref_desc_handle;
    uint8_t reportid;
    uint8_t reporttype;
}hid_reportref_desc_t;


/* Prototypes */
void logic_bluetooth_hid_profile_init(uint8_t servinst, uint8_t device, uint8_t* mode, uint8_t report_num, uint8_t* report_type, uint8_t** report_val, uint8_t* report_len, hid_info_t* info);
void logic_bluetooth_update_report(uint16_t conn_handle, uint8_t serv_inst, uint8_t reportid, uint8_t* report, uint16_t len, BOOL use_report_charac);
void logic_bluetooth_boot_key_report_update(at_ble_handle_t conn_handle, uint8_t serv_inst, uint8_t* bootreport, uint16_t len);
void logic_bluetooth_successfull_pairing_call(ble_connected_dev_info_t* dev_info, at_ble_connected_t* connected_info);
ret_type_te logic_bluetooth_send_modifier_and_key(uint8_t modifier, uint8_t key, uint8_t second_key);
uint8_t logic_bluetooth_get_report_characteristic(uint16_t handle, uint8_t serv, uint8_t reportid);
uint8_t logic_bluetooth_get_notif_instance(uint8_t serv_num, uint16_t char_handle);
void logic_bluetooth_gpio_set(at_ble_gpio_pin_t pin, at_ble_gpio_status_t status);
at_ble_status_t logic_bluetooth_characteristic_changed_handler(void* params);
void logic_bluetooth_store_temp_ban_connected_address(uint8_t* address);
uint8_t logic_bluetooth_get_reportid(uint8_t serv, uint16_t handle);
void logic_bluetooth_set_open_to_pairing_bool(BOOL pairing_bool);
void logic_bluetooth_start_bluetooth(uint8_t* unit_mac_address);
RET_TYPE logic_bluetooth_temporarily_ban_connected_device(void);
void logic_bluetooth_raw_send(uint8_t* data, uint16_t data_len);
uint8_t logic_bluetooth_get_hid_serv_instance(uint16_t handle);
void logic_bluetooth_encryption_changed_success(uint8_t* mac);
BOOL logic_bluetooth_is_device_temp_banned(uint8_t* mac);
at_ble_status_t ble_char_changed_app_event(void* param);
void logic_bluetooth_clear_bonding_information(void);
void logic_bluetooth_set_battery_level(uint8_t pct);
ret_type_te logic_bluetooth_stop_advertising(void);
BOOL logic_bluetooth_get_open_to_pairing(void);
void logic_bluetooth_start_advertising(void);
BOOL logic_bluetooth_can_talk_to_host(void);
void logic_bluetooth_stop_bluetooth(void);
void logic_bluetooth_routine(void);


#endif /* LOGIC_BLUETOOTH_H_ */