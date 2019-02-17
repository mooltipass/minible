/*!  \file     comms_hid_msgs.h
*    \brief    HID communications
*    Created:  06/03/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include <string.h>
#include "comms_hid_msgs.h" 
#include "defines.h"


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
    if ((answer_restrict_type != MSG_NO_RESTRICT) && (rcv_msg->message_type != HID_CMD_ID_PING))
    {
        should_ignore_message = TRUE;
    }
    
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
        
        default: break;
    }
    
    return -1;
}
