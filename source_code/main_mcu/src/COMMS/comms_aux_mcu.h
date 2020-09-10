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
/*!  \file     comms_aux_mcu.h
*    \brief    Communications with aux MCU
*    Created:  03/03/2018
*    Author:   Mathieu Stephan
*/


#ifndef COMMS_AUX_MCU_H_
#define COMMS_AUX_MCU_H_

/* Includes */
#include "comms_aux_mcu_defines.h"
#include "defines.h"

/* Prototypes */
RET_TYPE comms_aux_mcu_active_wait(aux_mcu_message_t** rx_message_pt_pt, BOOL do_not_touch_dma_flags, uint16_t expected_packet, BOOL single_try, int16_t expected_event);
comms_msg_rcvd_te comms_aux_mcu_deal_with_ble_message(aux_mcu_message_t* received_message, msg_restrict_type_te answer_restrict_type);
aux_mcu_message_t* comms_aux_mcu_get_empty_packet_ready_to_be_sent(uint16_t message_type);
comms_msg_rcvd_te comms_aux_mcu_routine(msg_restrict_type_te answer_restrict_type);
void comms_aux_mcu_deal_with_received_event(aux_mcu_message_t* received_message);
aux_mcu_message_t* comms_aux_mcu_wait_for_aux_event(uint16_t aux_mcu_event);
aux_mcu_message_t* comms_aux_mcu_get_free_tx_message_object_pt(void);
void comms_aux_mcu_send_message(aux_mcu_message_t* message_to_send);
void comms_aux_mcu_send_simple_command_message(uint16_t command);
BOOL comms_aux_mcu_get_and_clear_rx_transfer_already_armed(void);
BOOL comms_aux_mcu_get_and_clear_invalid_message_received(void);
void comms_aux_mcu_hard_comms_reset_with_aux_mcu_reboot(void);
aux_status_return_te comms_aux_mcu_get_aux_status(void);
void comms_aux_mcu_set_invalid_message_received(void);
void comms_aux_mcu_update_device_status_buffer(void);
RET_TYPE comms_aux_mcu_send_receive_ping(void);
void comms_aux_mcu_wait_for_message_sent(void);
void comms_aux_arm_rx_and_clear_no_comms(void);
BOOL comms_aux_mcu_are_comms_disabled(void);
void comms_aux_mcu_set_comms_disabled(void);

#endif /* COMMS_AUX_MCU_H_ */
