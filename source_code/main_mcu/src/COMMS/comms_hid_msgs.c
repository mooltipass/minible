/*!  \file     comms_hid_msgs.h
*    \brief    HID communications
*    Created:  06/03/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include <string.h>
#include "comms_hid_msgs.h" 


/*! \fn     comms_hid_msgs_parse(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* possible_reply)
*   \brief  Parse an incoming message from USB or BLE
*   \param  rcv_msg         Received message
*   \param  msg_length      Supposed payload length
*   \param  possible_reply  Pointer to where to store a possible reply (see struct for max size)
*   \return something >= 0 if an answer needs to be sent, otherwise -1
*/
int16_t comms_hid_msgs_parse(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* possible_reply)
{    
    /* Check correct payload length */
    if (supposed_payload_length != rcv_msg->payload_length)
    {
        /* Silent error */
        return -1;
    }
    
    /* By default all answers have the same command ID */
    possible_reply->message_type = rcv_msg->message_type;
    
    /* Switch on command id */
    switch (rcv_msg->message_type)
    {
        case HID_CMD_ID_PING:
        {
            /* Simple ping: copy contents */
            possible_reply->payload_length = rcv_msg->payload_length;
            memcpy((void*)possible_reply->payload, rcv_msg->payload, rcv_msg->payload_length);
            return possible_reply->payload_length;
        }
        
        default: break;
    }
    
    return -1;
}
