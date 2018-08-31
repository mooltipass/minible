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
#include "comms_aux_mcu.h"
#include "driver_sercom.h"
#include "dataflash.h"
#include "sh1122.h"
#include "main.h"


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
        comms_aux_mcu_send_message((void*)temp_message);
        comms_aux_mcu_wait_for_message_sent();
    }
    va_end(ap);
}
#pragma GCC diagnostic pop
#endif

/*! \fn     comms_hid_msgs_parse_debug(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* send_msg)
*   \brief  Parse an incoming message from USB or BLE
*   \param  rcv_msg         Received message
*   \param  msg_length      Supposed payload length
*   \param  send_msg        Where to write a possible reply
*   \return something >= 0 if an answer needs to be sent, otherwise -1
*/
int16_t comms_hid_msgs_parse_debug(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* send_msg)
{    
    /* Check correct payload length */
    if ((supposed_payload_length != rcv_msg->payload_length) || (supposed_payload_length > sizeof(rcv_msg->payload)))
    {
        /* Silent error */
        return -1;
    }
    
    /* By default: copy the same CMD identifier for TX message */
    send_msg->message_type = rcv_msg->message_type;
    
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
            /* Erase data flash */
            dataflash_bulk_erase_without_wait(&dataflash_descriptor);
            
            /* Set ack, leave same command id */
            send_msg->payload[0] = HID_1BYTE_ACK;
            send_msg->payload_length = 1;
            return 1;            
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
            /* First 4 bytes is the write address, remaining 256 bytes is the payload */
            uint32_t* write_address = (uint32_t*)&rcv_msg->payload_as_uint32[0];
            dataflash_write_array_to_memory(&dataflash_descriptor, *write_address, &rcv_msg->payload[4], 256);
            
            /* Set ack, leave same command id */
            send_msg->payload[0] = HID_1BYTE_ACK;
            send_msg->payload_length = 1;
            return 1;
        }
        case HID_CMD_ID_START_BOOTLOADER:
        {
            custom_fs_settings_set_fw_upgrade_flag();
            cpu_irq_disable();
            NVIC_SystemReset();
            while(1);            
        }
        case HID_CMD_ID_GET_ACC_32_SAMPLES:
        {
            while (lis2hh12_check_data_received_flag_and_arm_other_transfer(&acc_descriptor) == FALSE);
            memcpy((void*)send_msg->payload, (void*)acc_descriptor.fifo_read.acc_data_array, sizeof(acc_descriptor.fifo_read.acc_data_array));
            
            send_msg->payload_length = sizeof(acc_descriptor.fifo_read.acc_data_array);
            return sizeof(acc_descriptor.fifo_read.acc_data_array);
        }
        default: break;
    }
    
    return -1;
}
