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
/*!  \file     comms_hid_msgs.h
*    \brief    HID communications
*    Created:  06/03/2018
*    Author:   Mathieu Stephan
*/ 


#ifndef COMMS_HID_MSGS_H_
#define COMMS_HID_MSGS_H_

#include "comms_aux_mcu_defines.h"
#include "defines.h"

/* Prototypes */
void comms_hid_msgs_parse(hid_message_t* rcv_msg, uint16_t supposed_payload_length, msg_restrict_type_te answer_restrict_type, BOOL is_message_from_usb);
void comms_hid_msgs_update_message_fields(aux_mcu_message_t* message_pt, BOOL usb_hid_message, uint16_t message_type, uint16_t hid_payload_size);
aux_mcu_message_t* comms_hid_msgs_get_empty_hid_packet(BOOL usb_hid_message, uint16_t message_type, uint16_t hid_payload_size);
void comms_hid_msgs_update_message_payload_length_fields(aux_mcu_message_t* message_pt, uint16_t hid_payload_size);
void comms_hid_msgs_send_ack_nack_message(BOOL usb_hid_message, uint16_t message_type, BOOL ack_message);
uint16_t comms_hid_msgs_fill_get_status_message_answer(uint16_t* msg_array_uint16);

#endif /* COMMS_HID_MSGS_H_ */