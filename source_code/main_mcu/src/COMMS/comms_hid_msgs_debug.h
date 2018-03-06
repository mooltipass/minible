/*!  \file     comms_hid_msgs_debug.h
*    \brief    HID debug communications
*    Created:  06/03/2018
*    Author:   Mathieu Stephan
*/


#ifndef COMMS_HID_MSGS_DEBUG_H_
#define COMMS_HID_MSGS_DEBUG_H_

#include "comms_hid_msgs.h"

/* Defines */
#define HID_MESSAGE_START_CMD_ID_DBG    0x8000
// Command IDs
#define HID_CMD_ID_DBG_MSG      0x8000

/* Prototypes */
int16_t comms_hid_msgs_parse_debug(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* possible_reply);


#endif /* COMMS_HID_MSGS_DEBUG_H_ */