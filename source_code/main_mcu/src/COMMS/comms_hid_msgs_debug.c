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
/*!  \file     comms_hid_msgs_debug.c
*    \brief    HID debug communications
*    Created:  06/03/2018
*    Author:   Mathieu Stephan
*/
#include <stdarg.h>
#include <string.h>
#include <asf.h>
#include "comms_hid_msgs_debug.h"
#include "comms_hid_msgs.h"
#include "gui_dispatcher.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_sercom.h"
#include "logic_device.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "logic_power.h"
#include "dataflash.h"
#include "sh1122.h"
#include "main.h"
#include "dma.h"
/* Variable to know if we're allowing bundle upload */
BOOL comms_hid_msgs_debug_upload_allowed = FALSE;


#ifdef DEBUG_USB_PRINTF_ENABLED
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
/*! \fn     comms_hid_msgs_debug_printf(const char *fmt, ...) 
*   \brief  Output debug string to USB
*/
void comms_hid_msgs_debug_printf(const char *fmt, ...) 
{
    char buf[64];    
    va_list ap;    
    va_start(ap, fmt);

    /* Print into our temporary buffer */
    int16_t hypothetical_nb_chars = vsnprintf(buf, sizeof(buf), fmt, ap);
    
    /* No error? */
    if (hypothetical_nb_chars > 0)
    {
        /* Compute number of chars printed to our buffer */
        uint16_t actual_printed_chars = (uint16_t)hypothetical_nb_chars < sizeof(buf)-1? (uint16_t)hypothetical_nb_chars : sizeof(buf)-1;
        
        /* Use message to send to aux mcu as temporary buffer */        
        comms_aux_mcu_wait_for_message_sent();
        aux_mcu_message_t* temp_message = comms_aux_mcu_get_temp_tx_message_object_pt();
        memset((void*)temp_message, 0, sizeof(aux_mcu_message_t));
        temp_message->message_type = AUX_MCU_MSG_TYPE_USB;
        temp_message->hid_message.message_type = HID_CMD_ID_DBG_MSG;
        temp_message->hid_message.payload_length = actual_printed_chars*2 + 2;
        temp_message->payload_length1 = temp_message->hid_message.payload_length + sizeof(temp_message->hid_message.payload_length) + sizeof(temp_message->hid_message.message_type);
        
        /* Copy to message payload */
        for (uint16_t i = 0; i < actual_printed_chars; i++)
        {
            temp_message->hid_message.payload_as_uint16[i] = buf[i];
        }
        
        /* Send message */
        comms_aux_mcu_send_message(TRUE);
    }
    va_end(ap);
}
#pragma GCC diagnostic pop
#endif

/*! \fn     comms_hid_msgs_parse_debug(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* send_msg, msg_restrict_type_te answer_restrict_type)
*   \brief  Parse an incoming message from USB or BLE
*   \param  rcv_msg                 Received message
*   \param  supposed_payload_length Supposed payload length
*   \param  send_msg                Where to write a possible reply
*   \param  answer_restrict_type    Enum restricting which messages we can answer
*   \return something >= 0 if an answer needs to be sent, otherwise -1
*/
int16_t comms_hid_msgs_parse_debug(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* send_msg, msg_restrict_type_te answer_restrict_type)
{    
    /* Check correct payload length */
    if ((supposed_payload_length != rcv_msg->payload_length) || (supposed_payload_length > sizeof(rcv_msg->payload)))
    {
        /* Silent error */
        return -1;
    }
    
    /* Checks based on restriction type */
    BOOL should_ignore_message = FALSE;
    if (answer_restrict_type == MSG_RESTRICT_ALL)
    {
        should_ignore_message = TRUE;
    }
    
    /* If do_not_deal_with is set, send please retry...  */
    if (should_ignore_message != FALSE)
    {
        /* Send please retry */
        send_msg->message_type = HID_CMD_ID_RETRY;
        send_msg->payload_length = 0;
        return 0;
    }
    
    /* By default: copy the same CMD identifier for TX message */
    send_msg->message_type = rcv_msg->message_type;
    uint16_t rcv_message_type = rcv_msg->message_type;
    
    /* Switch on command id */
    switch (rcv_msg->message_type)
    {
        case HID_CMD_ID_OPEN_DISP_BUFFER:
        {
            /* Set pixel write window */
            sh1122_set_row_address(&plat_oled_descriptor, 0);
            sh1122_set_column_address(&plat_oled_descriptor, 0);
            
            /* Start filling the SSD1322 RAM */
            sh1122_start_data_sending(&plat_oled_descriptor);
            
            /* Set ack, leave same command id */
            send_msg->payload[0] = HID_1BYTE_ACK;
            send_msg->payload_length = 1;
            return 1;
        }    
        case HID_CMD_ID_SEND_TO_DISP_BUFFER:
        {            
            /* Send all pixels */
            for (uint16_t i = 0; i < supposed_payload_length; i++)
            {
                sercom_spi_send_single_byte_without_receive_wait(plat_oled_descriptor.sercom_pt, rcv_msg->payload[i]);
            }
            
            /* Set ack, leave same command id */
            send_msg->payload[0] = HID_1BYTE_ACK;
            send_msg->payload_length = 1;
            return 1;
        }  
        case HID_CMD_ID_CLOSE_DISP_BUFFER:
        {            
            /* Wait for spi buffer to be sent */
            sercom_spi_wait_for_transmit_complete(plat_oled_descriptor.sercom_pt);
            
            /* Stop sending data */
            sh1122_stop_data_sending(&plat_oled_descriptor);            
            
            /* Set ack, leave same command id */
            send_msg->payload[0] = HID_1BYTE_ACK;
            send_msg->payload_length = 1;
            return 1;
        }          
        case HID_CMD_ID_ERASE_DATA_FLASH:
        {
            /* Required actions when we start dealing with graphics memory */
            if (logic_device_bundle_update_start(TRUE) == RETURN_OK)
            {
                /* Set upload allowed boolean */
                comms_hid_msgs_debug_upload_allowed = TRUE;
                
                /* Erase data flash */
                dataflash_bulk_erase_without_wait(&dataflash_descriptor);
                
                /* Set ack, leave same command id */
                send_msg->payload[0] = HID_1BYTE_ACK;
                send_msg->payload_length = 1;
                return 1;
            }
            else
            {
                /* Set ack, leave same command id */
                send_msg->payload[0] = HID_1BYTE_NACK;
                send_msg->payload_length = 1;
                return 1;               
            }                     
        }
        case HID_CMD_ID_IS_DATA_FLASH_READY:
        {
            if (dataflash_is_busy(&dataflash_descriptor) != FALSE)
            {
                /* Set nack, leave same command id */
                send_msg->payload[0] = HID_1BYTE_NACK;
                send_msg->payload_length = 1;
                return 1;                
            }
            else
            {
                /* Set ack, leave same command id */
                send_msg->payload[0] = HID_1BYTE_ACK;
                send_msg->payload_length = 1;
                return 1;                
            }
        }
        case HID_CMD_ID_DATAFLASH_WRITE_256B:
        {
            if (comms_hid_msgs_debug_upload_allowed != FALSE)
            {
                /* First 4 bytes is the write address, remaining 256 bytes is the payload */
                uint32_t* write_address = (uint32_t*)&rcv_msg->payload_as_uint32[0];
                dataflash_write_array_to_memory(&dataflash_descriptor, *write_address, &rcv_msg->payload[4], 256);
                
                /* Set ack, leave same command id */
                send_msg->payload[0] = HID_1BYTE_ACK;
                send_msg->payload_length = 1;
                return 1;
            } 
            else
            {
                /* Set nack, leave same command id */
                send_msg->payload[0] = HID_1BYTE_NACK;
                send_msg->payload_length = 1;
                return 1;
            }
        }
        case HID_CMD_ID_REINDEX_BUNDLE:
        {        
            if (comms_hid_msgs_debug_upload_allowed != FALSE)
            {
                /* Do required actions */
                logic_device_bundle_update_end(TRUE);
                
                /* Call activity detected to prevent going to sleep directly after */
                logic_device_activity_detected();
                
                /* Reset boolean */
                comms_hid_msgs_debug_upload_allowed = FALSE;
                
                /* Set ack, leave same command id */
                send_msg->payload[0] = HID_1BYTE_ACK;
                send_msg->payload_length = 1;
                return 1;
            } 
            else
            {
                /* Set nack, leave same command id */
                send_msg->payload[0] = HID_1BYTE_NACK;
                send_msg->payload_length = 1;
                return 1;
            }                
        }
        case HID_CMD_ID_SET_OLED_PARAMS:
        {
            /* vcomh change requires oled on / off */
            uint8_t vcomh = rcv_msg->payload[1];
            
            /* Do required actions */
            if (vcomh != 0xFF)
            {
                sh1122_oled_off(&plat_oled_descriptor);
            }
            sh1122_set_contrast_current(&plat_oled_descriptor, rcv_msg->payload[0]);
            if (vcomh != 0xFF)
            {
                sh1122_set_vcomh_level(&plat_oled_descriptor, vcomh);
            }
            sh1122_set_vsegm_level(&plat_oled_descriptor, rcv_msg->payload[2]);
            sh1122_set_discharge_charge_periods(&plat_oled_descriptor, rcv_msg->payload[3] | (rcv_msg->payload[4] << 4));
            sh1122_set_discharge_vsl_level(&plat_oled_descriptor, rcv_msg->payload[5]);
            if (vcomh != 0xFF)
            {
                sh1122_oled_on(&plat_oled_descriptor);
            }
            
            /* Set ack, leave same command id */
            send_msg->payload[0] = HID_1BYTE_ACK;
            send_msg->payload_length = 1;
            return 1;
        }
        case HID_CMD_ID_START_BOOTLOADER:
        {
            custom_fs_set_device_flag_value(DEVICE_WENT_THROUGH_BOOTLOADER_FLAG_ID, TRUE);
            custom_fs_settings_set_fw_upgrade_flag();
            main_reboot();      
        }
        case HID_CMD_ID_GET_ACC_32_SAMPLES:
        {
            while (lis2hh12_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor) == FALSE);
            memcpy((void*)send_msg->payload, (void*)plat_acc_descriptor.fifo_read.acc_data_array, sizeof(plat_acc_descriptor.fifo_read.acc_data_array));            
            send_msg->payload_length = sizeof(plat_acc_descriptor.fifo_read.acc_data_array);
            return sizeof(plat_acc_descriptor.fifo_read.acc_data_array);
        }
        case HID_CMD_ID_FLASH_AUX_MCU:
        {            
            /* Wait for current packet reception and arm reception */
            dma_aux_mcu_wait_for_current_packet_reception_and_clear_flag();
            comms_aux_arm_rx_and_clear_no_comms();            
            logic_aux_mcu_flash_firmware_update();            
            return -1;
        }
        case HID_CMD_ID_GET_DBG_PLAT_INFO:
        {
            aux_mcu_message_t* temp_rx_message;
            aux_mcu_message_t* temp_tx_message_pt;
            
            /* Generate our packet */
            comms_aux_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_PLAT_DETAILS);
            
            /* Wait for current packet reception and arm reception */
            dma_aux_mcu_wait_for_current_packet_reception_and_clear_flag();
            comms_aux_arm_rx_and_clear_no_comms();
            
            /* Send message */
            comms_aux_mcu_send_message(TRUE);
            
            /* Wait for message from aux MCU */
            while(comms_aux_mcu_active_wait(&temp_rx_message, TRUE, AUX_MCU_MSG_TYPE_PLAT_DETAILS, FALSE, -1) != RETURN_OK){}
                
            /* Copy message contents into send packet */
            memcpy((void*)send_msg->detailed_platform_info.aux_mcu_infos, (void*)&temp_rx_message->aux_details_message, sizeof(temp_rx_message->aux_details_message));
            send_msg->detailed_platform_info.main_mcu_fw_major = FW_MAJOR;
            send_msg->detailed_platform_info.main_mcu_fw_minor = FW_MINOR;
            send_msg->payload_length = sizeof(send_msg->detailed_platform_info);
            send_msg->message_type = rcv_message_type;
            
            /* Rearm receive */
            comms_aux_arm_rx_and_clear_no_comms();
            
            return sizeof(send_msg->detailed_platform_info);
        }
        case HID_CMD_ID_GET_BATTERY_STATUS:
        {
            aux_mcu_message_t* temp_tx_message_pt;
            aux_mcu_message_t* temp_rx_message;
            uint16_t bat_adc_result;
            
            /* Keep screen on in case we're testing power consumption */
            logic_device_activity_detected();        
            
            /* Wait for current packet reception and arm reception */
            dma_aux_mcu_wait_for_current_packet_reception_and_clear_flag();
            comms_aux_arm_rx_and_clear_no_comms();
            
            /* Start charging? */
            if (rcv_msg->payload[0] != 0)
            {
                comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_NIMH_CHARGE);
                logic_power_set_battery_charging_bool(TRUE, FALSE);
                comms_aux_mcu_wait_for_message_sent();
            }
            
            /* Stop charging? */
            if (rcv_msg->payload[1] != 0)
            {
                comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_STOP_CHARGE);
                logic_power_set_battery_charging_bool(FALSE, FALSE);
                comms_aux_mcu_wait_for_message_sent();
            }
            
            /* Use USB to power the screen? */
            if (rcv_msg->payload[2] != 0)
            {
                sh1122_oled_off(&plat_oled_descriptor);
                platform_io_disable_vbat_to_oled_stepup();
                platform_io_assert_oled_reset();
                timer_delay_ms(15);
                platform_io_power_up_oled(TRUE);
                sh1122_init_display(&plat_oled_descriptor);
                gui_dispatcher_get_back_to_current_screen();
            }
            
            /* Use battery to power the screen? */
            if (rcv_msg->payload[3] != 0)
            {
                sh1122_oled_off(&plat_oled_descriptor);
                platform_io_disable_3v3_to_oled_stepup();
                platform_io_assert_oled_reset();
                timer_delay_ms(15);
                platform_io_power_up_oled(FALSE);
                sh1122_init_display(&plat_oled_descriptor);
                gui_dispatcher_get_back_to_current_screen();
            }   
            
            /* Generate our get battery charge status packet */
            comms_aux_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_NIMH_CHARGE);
            
            /* Send message */
            comms_aux_mcu_send_message(TRUE);
            
            /* Get ADC value for current conversion */
            while (platform_io_is_voledin_conversion_result_ready() == FALSE);
            bat_adc_result = platform_io_get_voledin_conversion_result_and_trigger_conversion();
            
            /* Wait for message from aux MCU */
            while(comms_aux_mcu_active_wait(&temp_rx_message, TRUE, AUX_MCU_MSG_TYPE_NIMH_CHARGE, FALSE, -1) != RETURN_OK){}
                
            /* Prepare packet to send back */
            memset(send_msg, 0x00, sizeof(hid_message_t));
            send_msg->battery_status.power_source = logic_power_get_power_source();
            send_msg->battery_status.platform_charging = logic_power_is_battery_charging();
            send_msg->battery_status.main_adc_battery_value = bat_adc_result;
            send_msg->battery_status.aux_charge_status = temp_rx_message->nimh_charge_message.charge_status;
            send_msg->battery_status.aux_battery_voltage = temp_rx_message->nimh_charge_message.battery_voltage;
            send_msg->battery_status.aux_charge_current = temp_rx_message->nimh_charge_message.charge_current;
            send_msg->payload_length = sizeof(send_msg->battery_status);
            send_msg->message_type = rcv_message_type;
            
            /* Rearm receive */
            comms_aux_arm_rx_and_clear_no_comms();
            
            return sizeof(send_msg->battery_status);
        }
        default: break;
    }
    
    return -1;
}
