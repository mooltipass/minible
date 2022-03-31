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
/*!  \file     logic_aux_mcu.c
*    \brief    Aux MCU logic
*    Created:  09/05/2018
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include <asf.h>
#include "logic_accelerometer.h"
#include "logic_bluetooth.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "custom_fs.h"
#include "dma.h"
/* BLE enabled bool */
BOOL logic_aux_mcu_ble_enabled = FALSE;
/* USB enumerated bool */
BOOL logic_aux_mcu_usb_just_enumerated = FALSE;
BOOL logic_aux_mcu_usb_enumerated = FALSE;


/*! \fn     logic_aux_mcu_set_ble_enabled_bool(BOOL ble_enabled)
*   \brief  Set BLE enabled boolean
*   \param  ble_enabled     BLE state
*/
void logic_aux_mcu_set_ble_enabled_bool(BOOL ble_enabled)
{
    logic_aux_mcu_ble_enabled = ble_enabled;
}

/*! \fn     logic_aux_mcu_set_usb_enumerated_bool(BOOL usb_enumerated)
*   \brief  Set USB enumerated
*   \param  usb_enumerated  USB enumerated state
*/
void logic_aux_mcu_set_usb_enumerated_bool(BOOL usb_enumerated)
{
    logic_aux_mcu_usb_just_enumerated = usb_enumerated;
    logic_aux_mcu_usb_enumerated = usb_enumerated;
}

/*! \fn     logic_aux_mcu_is_ble_enabled(void)
*   \brief  Check if the BLE is enabled
*   \return BLE enabled boolean
*/
BOOL logic_aux_mcu_is_ble_enabled(void)
{
    return logic_aux_mcu_ble_enabled;
}

/*! \fn     logic_aux_mcu_is_usb_just_enumerated(void)
*   \brief  Check if USB was just enumerated and clear the bool
*   \return USB enumerated boolean
*/
BOOL logic_aux_mcu_is_usb_just_enumerated(void)
{
    BOOL return_val = logic_aux_mcu_usb_just_enumerated;
    logic_aux_mcu_usb_just_enumerated = FALSE;
    return return_val;
}

/*! \fn     logic_aux_mcu_is_usb_enumerated(void)
*   \brief  Check if USB enumerated
*   \return USB enumerated boolean
*/
BOOL logic_aux_mcu_is_usb_enumerated(void)
{
    return logic_aux_mcu_usb_enumerated;
}

/*! \fn     logic_aux_mcu_enable_ble(BOOL wait_for_enabled)
*   \brief  Enable bluetooth
*   \param  wait_for_enabled    Set to true to wait for BLE enabled
*/
void logic_aux_mcu_enable_ble(BOOL wait_for_enabled)
{
    _Static_assert(MEMBER_SIZE(dis_device_information_t, custom_device_name)==MEMBER_SIZE(custom_platform_settings_t, custom_ble_name), "Non matching bluetooth name fields lengths");
    
    if (logic_aux_mcu_ble_enabled == FALSE)
    {
        aux_mcu_message_t* temp_tx_message_pt;
        aux_mcu_message_t* temp_rx_message;
        
        /* Enable BLE line */
        platform_io_enable_ble();
        timer_delay_ms(10);
        
        /* Prepare packet to send to AUX, specify bluetooth mac */
        temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_BLE_CMD);
        temp_tx_message_pt->ble_message.message_id = BLE_MESSAGE_CMD_ENABLE;        
        logic_bluetooth_get_unit_mac_address(temp_tx_message_pt->ble_message.dis_device_information.mac_address);
        temp_tx_message_pt->ble_message.dis_device_information.fw_major = FW_MAJOR;
        temp_tx_message_pt->ble_message.dis_device_information.fw_minor = FW_MINOR;
        temp_tx_message_pt->ble_message.dis_device_information.serial_number = (custom_fs_get_platform_programmed_serial_number() == UINT32_MAX)? custom_fs_get_platform_internal_serial_number():custom_fs_get_platform_programmed_serial_number();
        temp_tx_message_pt->ble_message.dis_device_information.bundle_version = custom_fs_get_platform_bundle_version();
        memcpy(temp_tx_message_pt->ble_message.dis_device_information.custom_device_name, (void*)custom_fs_get_custom_ble_name(), MEMBER_ARRAY_SIZE(dis_device_information_t, custom_device_name)-1);
        temp_tx_message_pt->ble_message.dis_device_information.custom_device_name[MEMBER_ARRAY_SIZE(dis_device_information_t, custom_device_name)-1] = 0;        
        temp_tx_message_pt->payload_length1 = sizeof(temp_tx_message_pt->ble_message.message_id) + sizeof(temp_tx_message_pt->ble_message.uint32_t_padding) + sizeof(dis_device_information_t);
        
        /* Send message */
        comms_aux_mcu_send_message(temp_tx_message_pt);
        
        if (wait_for_enabled != FALSE)
        {
            /* wait for BLE to bootup */
            while(comms_aux_mcu_active_wait(&temp_rx_message, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, FALSE, AUX_MCU_EVENT_BLE_ENABLED) != RETURN_OK)
            {
                logic_accelerometer_routine();
            }
            
            /* Rearm DMA RX */
            comms_aux_arm_rx_and_clear_no_comms();
            
            /* Set boolean */
            logic_aux_mcu_ble_enabled = TRUE;
        }
    }
}

/*! \fn     logic_aux_mcu_disable_ble(BOOL wait_for_disabled)
*   \brief  Enable bluetooth
*   \param  wait_for_enabled    Set to true to wait for BLE enabled
*/
void logic_aux_mcu_disable_ble(BOOL wait_for_disabled)
{
    if (logic_aux_mcu_ble_enabled != FALSE)
    {
        /* Send command to aux MCU */
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_DISABLE_BLE);
        
        if (wait_for_disabled != FALSE)
        {
            /* Wait for BLE to switch off */
            comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_BLE_DISABLED);
            
            /* Set boolean */
            logic_aux_mcu_ble_enabled = FALSE;
        }
        
        /* Disable BLE */
        platform_io_disable_ble();
    }
}

/*! \fn     logic_power_update_aux_mcu_of_new_battery_level(uint16_t battery_level_pct)
*   \brief  Let the aux mcu know about the new battery level
*   \param  battery_level_pct   Battery level in pcts
*/
void logic_aux_mcu_update_aux_mcu_of_new_battery_level(uint16_t battery_level_pct)
{
    aux_mcu_message_t* temp_tx_message_pt;
    
    /* Prepare message */
    temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_MAIN_MCU_CMD);
    temp_tx_message_pt->main_mcu_command_message.command = MAIN_MCU_COMMAND_SET_BATTERYLVL;
    temp_tx_message_pt->main_mcu_command_message.payload[0] = (uint8_t)battery_level_pct;
    temp_tx_message_pt->payload_length1 = sizeof(temp_tx_message_pt->main_mcu_command_message.command) + sizeof(temp_tx_message_pt->main_mcu_command_message.payload[0]);
    
    /* Send message */
    comms_aux_mcu_send_message(temp_tx_message_pt);
    comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_NEW_BATTERY_LVL_RCVD);
}

/*! \fn     comms_aux_mcu_get_ble_chip_id(void)
*   \brief  Get ATBTLC1000 chip ID
*   \return uint32_t of chipID, 0 if error
*   \note   BLE must be enabled before calling this
*/
uint32_t logic_aux_mcu_get_ble_chip_id(void)
{
    if (logic_aux_mcu_ble_enabled == FALSE)
    {
        return 0;
    }
    else
    {
        aux_mcu_message_t* temp_tx_message_pt;
        aux_mcu_message_t* temp_rx_message;
        uint32_t return_val;
        
        /* Get an empty packet ready to be sent */
        temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_PLAT_DETAILS);
        
        /* Send message */
        comms_aux_mcu_send_message(temp_tx_message_pt);
        
        /* Wait for message from aux MCU */
        while(comms_aux_mcu_active_wait(&temp_rx_message, AUX_MCU_MSG_TYPE_PLAT_DETAILS, FALSE, -1) != RETURN_OK){}
        
        /* Output debug info */
        return_val = temp_rx_message->aux_details_message.atbtlc_chip_id;

        /* Info gotten, rearm DMA RX */
        comms_aux_arm_rx_and_clear_no_comms();
        
        return return_val;
    }
}

/*! \fn     logic_aux_mcu_flash_firmware_update(BOOL connect_to_usb_if_needed)
*   \brief  Flash update firmware for aux MCU
*   \param  connect_to_usb_if_needed    Connect to USB after flash procedure if needed
*   \return something >= 0 if an answer needs to be sent, otherwise -1
*/
RET_TYPE logic_aux_mcu_flash_firmware_update(BOOL connect_to_usb_if_needed)
{
    aux_mcu_message_t* temp_rx_message;
    aux_mcu_message_t* temp_tx_message_pt;
    
    /* Look for update file address */
    custom_fs_address_t fw_file_address;
    custom_fs_binfile_size_t fw_file_size;
    if (custom_fs_get_file_address(1, &fw_file_address, CUSTOM_FS_FW_UPDATE_TYPE) != RETURN_OK)
    {
        /* We couldn't find the update file */
        return RETURN_NOK;
    }
    
    /* Read file size and compute binary data address */
    custom_fs_read_from_flash((uint8_t*)&fw_file_size, fw_file_address, sizeof(fw_file_size));
    fw_file_address += sizeof(fw_file_size);
    
    /* Prepare programming command */
    temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_BOOTLOADER);
    temp_tx_message_pt->bootloader_message.command = BOOTLOADER_START_PROGRAMMING_COMMAND;
    temp_tx_message_pt->bootloader_message.programming_command.image_length = fw_file_size;
    temp_tx_message_pt->payload_length1 = sizeof(temp_tx_message_pt->bootloader_message.command) + sizeof(temp_tx_message_pt->bootloader_message.programming_command);
    
    /* Send message */
    comms_aux_mcu_send_message(temp_tx_message_pt);
    
    /* Wait for message from aux MCU */
    while(comms_aux_mcu_active_wait(&temp_rx_message, AUX_MCU_MSG_TYPE_BOOTLOADER, FALSE, -1) != RETURN_OK){}
    
    /* Answer checked, rearm RX */    
    comms_aux_arm_rx_and_clear_no_comms();
    
    /* Send bytes by blocks of 512 */
    uint32_t nb_bytes_sent = 0;
    uint32_t nb_bytes_to_read;
    while (fw_file_size > 0)
    {
        /* Compute number of bytes to send */
        nb_bytes_to_read = fw_file_size;
        if (nb_bytes_to_read > 512)
        {
            nb_bytes_to_read = 512;
        }
        
        /* Prepare write command */
        temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_BOOTLOADER);
        temp_tx_message_pt->bootloader_message.command = BOOTLOADER_WRITE_COMMAND;
        temp_tx_message_pt->bootloader_message.write_command.size = 512;
        temp_tx_message_pt->bootloader_message.write_command.address = nb_bytes_sent;
        temp_tx_message_pt->payload_length1 = sizeof(temp_tx_message_pt->bootloader_message.command) + sizeof(temp_tx_message_pt->bootloader_message.write_command);
        
        /* Fill payload */
        custom_fs_read_from_flash((uint8_t*)temp_tx_message_pt->bootloader_message.write_command.payload, fw_file_address, nb_bytes_to_read);
        
        /* Update vars */
        fw_file_address += nb_bytes_to_read;
        fw_file_size -= nb_bytes_to_read;   
        nb_bytes_sent += nb_bytes_to_read;
        
        /* Padding for last packet */
        if ((fw_file_size == 0) && (nb_bytes_to_read != 512))
        {
            memset((void*)&temp_tx_message_pt->bootloader_message.write_command.payload[nb_bytes_to_read], 0, 512-nb_bytes_to_read);
        }
        
        /* Send message */
        comms_aux_mcu_send_message(temp_tx_message_pt);
        
        /* Wait for message from aux MCU */
        while(comms_aux_mcu_active_wait(&temp_rx_message, AUX_MCU_MSG_TYPE_BOOTLOADER, FALSE, -1) != RETURN_OK){}
        
        /* Answer checked, rearm RX */
        comms_aux_arm_rx_and_clear_no_comms();
    }      
        
    /* Prepare start application command, no answers */
    temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_BOOTLOADER);
    temp_tx_message_pt->bootloader_message.command = BOOTLOADER_START_APP_COMMAND;
    temp_tx_message_pt->payload_length1 = sizeof(temp_tx_message_pt->bootloader_message.command);
    comms_aux_mcu_send_message(temp_tx_message_pt);
    
    /* Let the aux MCU boot */
    timer_delay_ms(1000);
    
    /* If USB present, send USB attach message */
    if ((platform_io_is_usb_3v3_present() != FALSE) && (connect_to_usb_if_needed != FALSE))
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_ATTACH_USB);
        comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_ATTACH_CMD_RCVD);
    }
        
    return RETURN_OK;
}