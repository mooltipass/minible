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
/*!  \file     comms_hid_msgs_debug.h
*    \brief    HID debug communications
*    Created:  06/03/2018
*    Author:   Mathieu Stephan
*/


#ifndef COMMS_HID_MSGS_DEBUG_H_
#define COMMS_HID_MSGS_DEBUG_H_

#include "comms_hid_defines.h"
#include "defines.h"

/* Prototypes */
void comms_hid_msgs_parse_debug(hid_message_t* rcv_msg, uint16_t supposed_payload_length, msg_restrict_type_te answer_restrict_type, BOOL is_message_from_usb);
#ifdef DEBUG_USB_PRINTF_ENABLED
    void comms_hid_msgs_debug_printf(const char *fmt, ...);
#else
    #define comms_hid_msgs_debug_printf(...)    ()
#endif


#endif /* COMMS_HID_MSGS_DEBUG_H_ */
