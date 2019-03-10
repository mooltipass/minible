/*!  \file     comms_hid_msgs.h
*    \brief    HID communications
*    Created:  06/03/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include <string.h>
#include "comms_hid_msgs.h" 
#include "comms_aux_mcu.h"
#include "nodemgmt.h"
#include "defines.h"
#include "dbflash.h"
#include "dma.h"


/*! \fn     comms_hid_msgs_parse(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* send_msg, msg_restrict_type_te answer_restrict_type)
*   \brief  Parse an incoming message from USB or BLE
*   \param  rcv_msg                 Received message
*   \param  supposed_payload_length Supposed payload length
*   \param  send_msg                Where to write a possible reply
*   \param  answer_restrict_type    Enum restricting which messages we can answer
*   \return something >= 0 if an answer needs to be sent, otherwise -1
*/
int16_t comms_hid_msgs_parse(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* send_msg, msg_restrict_type_te answer_restrict_type)
{    
    /* Check correct payload length */
    if ((supposed_payload_length != rcv_msg->payload_length) || (supposed_payload_length > sizeof(rcv_msg->payload)))
    {
        /* Silent error */
        return -1;
    }
    
    /* Checks based on restriction type */
    BOOL should_ignore_message = FALSE;
    if ((answer_restrict_type == MSG_RESTRICT_ALL) && (rcv_msg->message_type != HID_CMD_ID_PING))
    {
        should_ignore_message = TRUE;
    }
    if ((answer_restrict_type == MSG_RESTRICT_ALLBUT_CANCEL) && (rcv_msg->message_type != HID_CMD_ID_PING) && (rcv_msg->message_type != HID_CMD_ID_CANCEL_REQ))
    {
        should_ignore_message = TRUE;
    }
    // TODO: deal with all but bundle
    
    /* Depending on restriction, answer please retry */
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
        case HID_CMD_ID_PING:
        {
            /* Simple ping: copy the message contents */
            memcpy((void*)send_msg->payload, (void*)rcv_msg->payload, rcv_msg->payload_length);
            send_msg->payload_length = rcv_msg->payload_length;
            return send_msg->payload_length;
        }
        
        case HID_CMD_ID_PLAT_INFO:
        {
            aux_mcu_message_t* temp_rx_message;
            aux_mcu_message_t* temp_tx_message_pt;
            
            /* Generate our packet */
            comms_aux_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_PLAT_DETAILS);
            
            /* Wait for current packet reception and arm reception */
            dma_aux_mcu_wait_for_current_packet_reception_and_clear_flag();
            
            /* Send message */
            comms_aux_mcu_send_message(TRUE);
            
            /* Wait for message from aux MCU */
            while(comms_aux_mcu_active_wait(&temp_rx_message, TRUE, AUX_MCU_MSG_TYPE_PLAT_DETAILS) == RETURN_NOK){}
            
            /* Copy message contents into send packet */
            send_msg->platform_info.main_mcu_fw_major = FW_MAJOR;
            send_msg->platform_info.main_mcu_fw_minor = FW_MINOR;
            send_msg->platform_info.aux_mcu_fw_major = temp_rx_message->aux_details_message.aux_fw_ver_major;
            send_msg->platform_info.aux_mcu_fw_minor = temp_rx_message->aux_details_message.aux_fw_ver_minor;
            send_msg->platform_info.plat_serial_number = 12345678;
            send_msg->platform_info.memory_size = DBFLASH_CHIP;            
            send_msg->payload_length = sizeof(send_msg->platform_info);
            send_msg->message_type = rcv_message_type;
            return sizeof(send_msg->platform_info);            
        }
        
        case HID_CMD_ID_SET_DATE:
        {
            /* Set Date */
            nodemgmt_set_current_date(rcv_msg->payload_as_uint16[0]);
            
            /* Set ack, leave same command id */
            send_msg->payload[0] = HID_1BYTE_ACK;
            send_msg->payload_length = 1;
            return 1;
        }
        
        default: break;
    }
    
    return -1;
}
