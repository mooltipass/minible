/**
 * \file
 *
 * \brief Device Information Service
 *
 * Copyright (c) 2017-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Atmel
 *Support</a>
 */

/**
 * \mainpage
 * \section preface Preface
 * This is the reference manual for the Device Information service
 */
/*- Includes ---------------------------------------------------------------*/

#include "device_info.h"

/** characteristics of the device information service */
device_info_char_value_t char_value;

bool volatile dis_notification_flag[DIS_TOTAL_CHARATERISTIC_NUM] = {false};
uint16_t device_info_char_counter = 0;

/**@brief Initialize the dis service related information. */
void dis_init_service(dis_gatt_service_handler_t *device_info_serv, dis_device_information_t* device_info)
{    
    device_info_char_counter = 0;
    
	device_info_serv->serv_handle = 0;
	device_info_serv->serv_uuid.type = AT_BLE_UUID_16;
	device_info_serv->serv_uuid.uuid[0] = (uint8_t) DIS_SERVICE_UUID;
	device_info_serv->serv_uuid.uuid[1] = (uint8_t) (DIS_SERVICE_UUID >> 8);
	
	//Characteristic Info for Manufacturer Name String
	device_info_serv->serv_chars[device_info_char_counter].char_val_handle = 0;          /* handle stored here */
	device_info_serv->serv_chars[device_info_char_counter].uuid.type = AT_BLE_UUID_16;
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[0] = (uint8_t) DIS_CHAR_MANUFACTURER_NAME_UUID;          /* UUID : Manufacturer Name String */
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[1] = (uint8_t) (DIS_CHAR_MANUFACTURER_NAME_UUID >> 8);   /* UUID : Manufacturer Name String */
	device_info_serv->serv_chars[device_info_char_counter].properties = AT_BLE_CHAR_READ; /* Properties */
	
	memcpy(char_value.manufacturer_name,DEFAULT_MANUFACTURER_NAME,DIS_CHAR_MANUFACTURER_NAME_INIT_LEN);
	device_info_serv->serv_chars[device_info_char_counter].init_value = char_value.manufacturer_name;
	
	device_info_serv->serv_chars[device_info_char_counter].value_init_len = DIS_CHAR_MANUFACTURER_NAME_INIT_LEN;
	device_info_serv->serv_chars[device_info_char_counter].value_max_len = DIS_CHAR_MANUFACTURER_NAME_MAX_LEN;
    // Allow anyone to read the device information
	device_info_serv->serv_chars[device_info_char_counter].value_permissions = AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR;   /* permissions */
	device_info_serv->serv_chars[device_info_char_counter].user_desc = NULL;           /* user defined name */
	device_info_serv->serv_chars[device_info_char_counter].user_desc_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_max_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_permissions = AT_BLE_ATTR_NO_PERMISSIONS;             /*user description permissions*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*client config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*server config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].user_desc_handle = 0;             /*user desc handles*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_handle = 0;         /*client config handles*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_handle = 0;         /*server config handles*/
	device_info_serv->serv_chars[device_info_char_counter].presentation_format = NULL;       /* presentation format */
	device_info_char_counter++;
	
	//Characterisitc Info for Model Number String
	device_info_serv->serv_chars[device_info_char_counter].char_val_handle = 0;          /* handle stored here */
	device_info_serv->serv_chars[device_info_char_counter].uuid.type = AT_BLE_UUID_16;
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[0] = (uint8_t) DIS_CHAR_MODEL_NUMBER_UUID;          /* UUID : Serial Number String*/
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[1] = (uint8_t) (DIS_CHAR_MODEL_NUMBER_UUID >> 8);          /* UUID : Serial Number String*/
	device_info_serv->serv_chars[device_info_char_counter].properties = AT_BLE_CHAR_READ; /* Properties */
	
	memcpy(char_value.default_model_number,DEFAULT_MODEL_NUMBER,DIS_CHAR_MODEL_NUMBER_INIT_LEN);
	device_info_serv->serv_chars[device_info_char_counter].init_value = char_value.default_model_number;
	
	device_info_serv->serv_chars[device_info_char_counter].value_init_len = DIS_CHAR_MODEL_NUMBER_INIT_LEN;
	device_info_serv->serv_chars[device_info_char_counter].value_max_len = DIS_CHAR_MODEL_NUMBER_MAX_LEN;
    // Allow anyone to read the device information
	device_info_serv->serv_chars[device_info_char_counter].value_permissions = AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR;   /* permissions */
	device_info_serv->serv_chars[device_info_char_counter].user_desc = NULL;           /* user defined name */
	device_info_serv->serv_chars[device_info_char_counter].user_desc_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_max_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_permissions = AT_BLE_ATTR_NO_PERMISSIONS;             /*user description permissions*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*client config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*server config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].user_desc_handle = 0;             /*user desc handles*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_handle = 0;         /*client config handles*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_handle = 0;         /*server config handles*/
	device_info_serv->serv_chars[device_info_char_counter].presentation_format = NULL;       /* presentation format */
	device_info_char_counter++;


	//Characteristic Info for Serial String
	device_info_serv->serv_chars[device_info_char_counter].char_val_handle = 0;          /* handle stored here */
	device_info_serv->serv_chars[device_info_char_counter].uuid.type = AT_BLE_UUID_16;
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[0] = (uint8_t) DIS_CHAR_SERIAL_NUMBER_UUID;          /* UUID : Hardware Revision String*/
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[1] = (uint8_t) (DIS_CHAR_SERIAL_NUMBER_UUID >> 8);          /* UUID : Hardware Revision String*/
	device_info_serv->serv_chars[device_info_char_counter].properties = AT_BLE_CHAR_READ; /* Properties */
	
    itoa(device_info->serial_number, (char*)char_value.default_serial_number, 10);
	device_info_serv->serv_chars[device_info_char_counter].init_value = char_value.default_serial_number;	
	device_info_serv->serv_chars[device_info_char_counter].value_init_len = strlen((char*)char_value.default_serial_number);
	device_info_serv->serv_chars[device_info_char_counter].value_max_len = DIS_CHAR_SERIAL_NUMBER_MAX_LEN;
    // Allow anyone to read the device information
	device_info_serv->serv_chars[device_info_char_counter].value_permissions = AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR;   /* permissions */
	device_info_serv->serv_chars[device_info_char_counter].user_desc = NULL;           /* user defined name */
	device_info_serv->serv_chars[device_info_char_counter].user_desc_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_max_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_permissions = AT_BLE_ATTR_NO_PERMISSIONS;             /*user description permissions*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*client config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*server config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].user_desc_handle = 0;             /*user desc handles*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_handle = 0;         /*client config handles*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_handle = 0;         /*server config handles*/
	device_info_serv->serv_chars[device_info_char_counter].presentation_format = NULL;       /* presentation format */
	device_info_char_counter++;

    #if false
	//Characteristic Info for Hardware Revision String
	device_info_serv->serv_chars[device_info_char_counter].char_val_handle = 0;          /* handle stored here */
	device_info_serv->serv_chars[device_info_char_counter].uuid.type = AT_BLE_UUID_16;
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[0] = (uint8_t) DIS_CHAR_HARDWARE_REVISION_UUID;          /* UUID : Firmware Revision String*/
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[1] = (uint8_t) (DIS_CHAR_HARDWARE_REVISION_UUID >> 8);          /* UUID : Firmware Revision String*/
	device_info_serv->serv_chars[device_info_char_counter].properties = AT_BLE_CHAR_READ; /* Properties */
	
	memcpy(char_value.default_hardware_revision,DEFAULT_HARDWARE_REVISION,DIS_CHAR_HARDWARE_REVISION_INIT_LEN);
	device_info_serv->serv_chars[device_info_char_counter].init_value = char_value.default_hardware_revision;
	
	device_info_serv->serv_chars[device_info_char_counter].value_init_len = DIS_CHAR_HARDWARE_REVISION_INIT_LEN;
	device_info_serv->serv_chars[device_info_char_counter].value_max_len = DIS_CHAR_HARDWARE_REVISION_MAX_LEN;
    // Allow anyone to read the device information
	device_info_serv->serv_chars[device_info_char_counter].value_permissions = AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR;   /* permissions */
	device_info_serv->serv_chars[device_info_char_counter].user_desc = NULL;           /* user defined name */
	device_info_serv->serv_chars[device_info_char_counter].user_desc_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_max_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_permissions = AT_BLE_ATTR_NO_PERMISSIONS;             /*user description permissions*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*client config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*server config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].user_desc_handle = 0;             /*user desc handles*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_handle = 0;         /*client config handles*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_handle = 0;         /*server config handles*/
	device_info_serv->serv_chars[device_info_char_counter].presentation_format = NULL;       /* presentation format */
	device_info_char_counter++;
	#endif
	
	//Characteristic Info for Firmware  Revision
	device_info_serv->serv_chars[device_info_char_counter].char_val_handle = 0;          /* handle stored here */
	device_info_serv->serv_chars[device_info_char_counter].uuid.type = AT_BLE_UUID_16;
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[0] = (uint8_t) DIS_CHAR_FIRMWARE_REIVSION_UUID;          /* UUID : Software Revision */
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[1] = (uint8_t) (DIS_CHAR_FIRMWARE_REIVSION_UUID >> 8);          /* UUID : Software Revision */
	device_info_serv->serv_chars[device_info_char_counter].properties = AT_BLE_CHAR_READ; /* Properties */
	
    
    itoa(device_info->fw_major, (char*)char_value.default_firmware_revision, 10);
    char_value.default_firmware_revision[1] = '.';
    itoa(device_info->fw_minor, (char*)&char_value.default_firmware_revision[2], 10);
	device_info_serv->serv_chars[device_info_char_counter].init_value = char_value.default_firmware_revision;	
	device_info_serv->serv_chars[device_info_char_counter].value_init_len = strlen((char*)char_value.default_firmware_revision);
	device_info_serv->serv_chars[device_info_char_counter].value_max_len = DIS_CHAR_FIRMWARE_REIVSION_MAX_LEN;
    // Allow anyone to read the device information
	device_info_serv->serv_chars[device_info_char_counter].value_permissions = AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR;   /* permissions */
	device_info_serv->serv_chars[device_info_char_counter].user_desc = NULL;           /* user defined name */
	device_info_serv->serv_chars[device_info_char_counter].user_desc_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_max_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_permissions = AT_BLE_ATTR_NO_PERMISSIONS;             /*user description permissions*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*client config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*server config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].user_desc_handle = 0;             /*user desc handles*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_handle = 0;         /*client config handles*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_handle = 0;         /*server config handles*/
	device_info_serv->serv_chars[device_info_char_counter].presentation_format = NULL;       /* presentation format */
	device_info_char_counter++;
	
	//Characteristic Info for Software  Revision
	device_info_serv->serv_chars[device_info_char_counter].char_val_handle = 0;          /* handle stored here */
	device_info_serv->serv_chars[device_info_char_counter].uuid.type = AT_BLE_UUID_16;
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[0] = (uint8_t) DIS_CHAR_SOFTWARE_REVISION_UUID;          /* uuid : software revision */
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[1] = (uint8_t) (DIS_CHAR_SOFTWARE_REVISION_UUID >> 8);          /* uuid : software revision */
	device_info_serv->serv_chars[device_info_char_counter].properties = AT_BLE_CHAR_READ; /* properties */
	
    
    
    itoa(device_info->bundle_version, (char*)char_value.default_software_revision, 10);
	device_info_serv->serv_chars[device_info_char_counter].init_value = char_value.default_software_revision;	
	device_info_serv->serv_chars[device_info_char_counter].value_init_len = strlen((char*)char_value.default_software_revision);
	device_info_serv->serv_chars[device_info_char_counter].value_max_len = DIS_CHAR_SOFTWARE_REVISION_MAX_LEN;
    // Allow anyone to read the device information
	device_info_serv->serv_chars[device_info_char_counter].value_permissions = AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR;   /* permissions */
	device_info_serv->serv_chars[device_info_char_counter].user_desc = NULL;           /* user defined name */
	device_info_serv->serv_chars[device_info_char_counter].user_desc_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_max_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_permissions = AT_BLE_ATTR_NO_PERMISSIONS;             /*user description permissions*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*client config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*server config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].user_desc_handle = 0;             /*user desc handles*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_handle = 0;         /*client config handles*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_handle = 0;         /*server config handles*/
	device_info_serv->serv_chars[device_info_char_counter].presentation_format = NULL;       /* presentation format */
	device_info_char_counter++;
	
    #if false
	//Characteristic Info for SystemID  Number
	device_info_serv->serv_chars[device_info_char_counter].char_val_handle = 0;          /* handle stored here */
	device_info_serv->serv_chars[device_info_char_counter].uuid.type = AT_BLE_UUID_16;
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[0] = (uint8_t) DIS_CHAR_SYSTEM_ID_UUID;          /* UUID : Software Revision */
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[1] = (uint8_t) (DIS_CHAR_SYSTEM_ID_UUID >> 8);          /* UUID : Software Revision */
	device_info_serv->serv_chars[device_info_char_counter].properties = AT_BLE_CHAR_READ; /* Properties */
	
	memcpy(char_value.default_system_id.manufacturer_id, SYSTEM_ID_MANUFACTURER_ID, SYSTEM_ID_MANUFACTURER_ID_LEN);
	memcpy(char_value.default_system_id.org_unique_id, SYSTEM_ID_ORG_UNIQUE_ID, SYSTEM_ID_ORG_UNIQUE_ID_LEN);
	device_info_serv->serv_chars[device_info_char_counter].init_value = (uint8_t *) &char_value.default_system_id;					/*Initial Value*/
	
	device_info_serv->serv_chars[device_info_char_counter].value_init_len = DIS_CHAR_SYSTEM_ID_INIT_LEN;
	device_info_serv->serv_chars[device_info_char_counter].value_max_len = DIS_CHAR_SYSTEM_ID_MAX_LEN;
    // Allow anyone to read the device information
	device_info_serv->serv_chars[device_info_char_counter].value_permissions = AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR;   /* permissions */
	device_info_serv->serv_chars[device_info_char_counter].user_desc = NULL;           /* user defined name */
	device_info_serv->serv_chars[device_info_char_counter].user_desc_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_max_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_permissions = AT_BLE_ATTR_NO_PERMISSIONS;             /*user description permissions*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*client config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*server config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].user_desc_handle = 0;             /*user desc handles*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_handle = 0;         /*client config handles*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_handle = 0;         /*server config handles*/
	device_info_serv->serv_chars[device_info_char_counter].presentation_format = NULL;       /* presentation format */
	device_info_char_counter++;
	#endif
    
	//Characteristic Info for PnP ID 
	device_info_serv->serv_chars[device_info_char_counter].char_val_handle = 0;          /* handle stored here */
	device_info_serv->serv_chars[device_info_char_counter].uuid.type = AT_BLE_UUID_16;
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[0] = (uint8_t) DIS_CHAR_PNP_ID_UUID;          /* UUID : Software Revision */
	device_info_serv->serv_chars[device_info_char_counter].uuid.uuid[1] = (uint8_t) (DIS_CHAR_PNP_ID_UUID >> 8);          /* UUID : Software Revision */
	device_info_serv->serv_chars[device_info_char_counter].properties = AT_BLE_CHAR_READ; /* Properties */
	
	char_value.default_pnp_id.vendor_id_source = PNP_ID_VENDOR_ID_SOURCE;					/*characteristic value initialization */
	char_value.default_pnp_id.vendor_id = PNP_ID_VENDOR_ID;
	char_value.default_pnp_id.product_id= PNP_ID_PRODUCT_ID;
	char_value.default_pnp_id.product_version= PNP_ID_PRODUCT_VERSION;
	device_info_serv->serv_chars[device_info_char_counter].init_value = (uint8_t *) &char_value.default_pnp_id;					/*Initial Value*/
	
	device_info_serv->serv_chars[device_info_char_counter].value_init_len = DIS_CHAR_PNP_ID_INIT_LEN;
	device_info_serv->serv_chars[device_info_char_counter].value_max_len = DIS_CHAR_PNP_ID_MAX_LEN;
    // Allow anyone to read the device information
	device_info_serv->serv_chars[device_info_char_counter].value_permissions = AT_BLE_ATTR_READABLE_NO_AUTHN_NO_AUTHR;   /* permissions */
	device_info_serv->serv_chars[device_info_char_counter].user_desc = NULL;           /* user defined name */
	device_info_serv->serv_chars[device_info_char_counter].user_desc_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_max_len = 0;
	device_info_serv->serv_chars[device_info_char_counter].user_desc_permissions = AT_BLE_ATTR_NO_PERMISSIONS;             /*user description permissions*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*client config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_permissions = AT_BLE_ATTR_NO_PERMISSIONS;         /*server config permissions*/
	device_info_serv->serv_chars[device_info_char_counter].user_desc_handle = 0;             /*user desc handles*/
	device_info_serv->serv_chars[device_info_char_counter].client_config_handle = 0;         /*client config handles*/
	device_info_serv->serv_chars[device_info_char_counter].server_config_handle = 0;         /*server config handles*/
	device_info_serv->serv_chars[device_info_char_counter].presentation_format = NULL;       /* presentation format */
	device_info_char_counter++;
}

/**@brief Register a dis service instance inside stack. */
at_ble_status_t dis_primary_service_define(dis_gatt_service_handler_t *dis_primary_service)
{
	
	return(at_ble_primary_service_define(&dis_primary_service->serv_uuid,
	&dis_primary_service->serv_handle,
	NULL, 0,
	dis_primary_service->serv_chars,device_info_char_counter));
}

/**@brief  Update the DIS characteristic value after defining the services using dis_primary_service_define*/
at_ble_status_t dis_info_update(dis_gatt_service_handler_t *dis_serv , dis_info_type info_type, dis_info_data* info_data, at_ble_handle_t conn_handle)
{
	if (info_data->data_len > dis_serv->serv_chars[info_type].value_max_len)
	{
		DBG_LOG("invalid length parameter");
		return AT_BLE_FAILURE;
	}
	
	//updating application data
	memcpy(&(dis_serv->serv_chars[info_type].init_value), info_data->info_data,info_data->data_len);
	
	//updating the characteristic value
	if ((at_ble_characteristic_value_set(dis_serv->serv_chars[info_type].char_val_handle, info_data->info_data, info_data->data_len)) != AT_BLE_SUCCESS){
		DBG_LOG("updating the characteristic failed\r\n");
	} else {
		return AT_BLE_SUCCESS;
	}
        ALL_UNUSED(conn_handle);
	return AT_BLE_FAILURE;
}
