/*!  \file     comms_hid_msgs_debug.c
*    \brief    HID debug communications
*    Created:  06/03/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "comms_hid_msgs_debug.h"
#include "comms_hid_msgs.h"
#include "driver_sercom.h"
#include "dataflash.h"
#include "sh1122.h"
#include "main.h"


/*! \fn     comms_hid_msgs_parse_debug(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* possible_reply)
*   \brief  Parse an incoming message from USB or BLE
*   \param  rcv_msg         Received message
*   \param  msg_length      Supposed payload length
*   \return something >= 0 if an answer needs to be sent, otherwise -1
*/
int16_t comms_hid_msgs_parse_debug(hid_message_t* rcv_msg, uint16_t supposed_payload_length)
{    
    /* Check correct payload length */
    if ((supposed_payload_length != rcv_msg->payload_length) || (supposed_payload_length > sizeof(rcv_msg->payload)))
    {
        /* Silent error */
        return -1;
    }
    
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
            rcv_msg->payload[0] = HID_1BYTE_ACK;
            rcv_msg->payload_length = 1;
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
            rcv_msg->payload[0] = HID_1BYTE_ACK;
            rcv_msg->payload_length = 1;
            return 1;
        }  
        case HID_CMD_ID_CLOSE_DISP_BUFFER:
        {            
            /* Wait for spi buffer to be sent */
            sercom_spi_wait_for_transmit_complete(plat_oled_descriptor.sercom_pt);
            
            /* Stop sending data */
            sh1122_stop_data_sending(&plat_oled_descriptor);            
            
            /* Set ack, leave same command id */
            rcv_msg->payload[0] = HID_1BYTE_ACK;
            rcv_msg->payload_length = 1;
            return 1;
        }          
        case HID_CMD_ID_ERASE_DATA_FLASH:
        {
            /* Erase data flash */
            dataflash_bulk_erase_without_wait(&dataflash_descriptor);
            
            /* Set ack, leave same command id */
            rcv_msg->payload[0] = HID_1BYTE_ACK;
            rcv_msg->payload_length = 1;
            return 1;            
        }
        case HID_CMD_ID_IS_DATA_FLASH_READY:
        {
            if (dataflash_is_busy(&dataflash_descriptor) != FALSE)
            {
                /* Set nack, leave same command id */
                rcv_msg->payload[0] = HID_1BYTE_NACK;
                rcv_msg->payload_length = 1;
                return 1;                
            }
            else
            {
                /* Set ack, leave same command id */
                rcv_msg->payload[0] = HID_1BYTE_ACK;
                rcv_msg->payload_length = 1;
                return 1;                
            }
        }
        case HID_CMD_ID_DATAFLASH_WRITE_256B:
        {
            /* First 4 bytes is the write address, remaining 256 bytes is the payload */
            uint32_t* write_address = (uint32_t*)&rcv_msg->payload_as_uint32[0];
            dataflash_write_array_to_memory(&dataflash_descriptor, *write_address, &rcv_msg->payload[4], 256);
            
            /* Set ack, leave same command id */
            rcv_msg->payload[0] = HID_1BYTE_ACK;
            rcv_msg->payload_length = 1;
            return 1;
        }
        default: break;
    }
    
    return -1;
}
