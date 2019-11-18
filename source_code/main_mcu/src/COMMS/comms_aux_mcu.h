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

#include "comms_aux_mcu_defines.h"
#include "comms_bootloader_msg.h"
#include "comms_hid_msgs.h"
#include "defines.h"

/* Defines */
// Aux MCU Message Type
#define AUX_MCU_MSG_TYPE_USB            0x0000
#define AUX_MCU_MSG_TYPE_BLE            0x0001
#define AUX_MCU_MSG_TYPE_BOOTLOADER     0x0002
#define AUX_MCU_MSG_TYPE_PLAT_DETAILS   0x0003
#define AUX_MCU_MSG_TYPE_MAIN_MCU_CMD   0x0004
#define AUX_MCU_MSG_TYPE_AUX_MCU_EVENT  0x0005
#define AUX_MCU_MSG_TYPE_NIMH_CHARGE    0x0006
#define AUX_MCU_MSG_TYPE_PING_WITH_INFO 0x0007
#define AUX_MCU_MSG_TYPE_KEYBOARD_TYPE  0x0008

#define AUX_MCU_MSG_TYPE_RNG_TRANSFER   0x000A

// Main MCU commands
#define MAIN_MCU_COMMAND_SLEEP          0x0001
#define MAIN_MCU_COMMAND_ATTACH_USB     0x0002
#define MAIN_MCU_COMMAND_PING           0x0003
#define MAIN_MCU_COMMAND_ENABLE_BLE     0x0004
#define MAIN_MCU_COMMAND_NIMH_CHARGE    0x0005
#define MAIN_MCU_COMMAND_NO_COMMS_UNAV  0x0006
#define MAIN_MCU_COMMAND_DISABLE_BLE    0x0007
#define MAIN_MCU_COMMAND_DETACH_USB     0x0008
#define MAIN_MCU_COMMAND_FUNC_TEST      0x0009
#define MAIN_MCU_COMMAND_UPDT_DEV_STAT  0x000A

// Debug MCU commands
#define MAIN_MCU_COMMAND_TX_SWEEP_SGL       0x1000
#define MAIN_MCU_COMMAND_TX_TONE_CONT       0x1001
#define MAIN_MCU_COMMAND_TX_TONE_CONT_STOP  0x1002

// Aux MCU events
#define AUX_MCU_EVENT_BLE_ENABLED       0x0001
#define AUX_MCU_EVENT_BLE_DISABLED      0x0002
#define AUX_MCU_EVENT_TX_SWEEP_DONE     0x0003
#define AUX_MCU_EVENT_FUNC_TEST_DONE    0x0004
#define AUX_MCU_EVENT_USB_ENUMERATED    0x0005
#define AUX_MCU_EVENT_CHARGE_DONE       0x0006
#define AUX_MCU_EVENT_CHARGE_FAIL       0x0007
#define AUX_MCU_EVENT_SLEEP_RECEIVED    0x0008
#define AUX_MCU_EVENT_IM_HERE           0x0009
#define AUX_MCU_EVENT_BLE_CONNECTED     0x000A
#define AUX_MCU_EVENT_BLE_DISCONNECTED  0x000B

/* Typedefs */
typedef struct
{
    uint16_t aux_fw_ver_major;
    uint16_t aux_fw_ver_minor;
    uint32_t aux_did_register;
    uint32_t aux_uid_registers[4];
    uint16_t blusdk_lib_maj;
    uint16_t blusdk_lib_min;
    uint16_t blusdk_fw_maj;
    uint16_t blusdk_fw_min;
    uint16_t blusdk_fw_build;
    uint32_t atbtlc_rf_ver;
    uint32_t atbtlc_chip_id;
    uint8_t atbtlc_address[6];
} aux_plat_details_message_t;

typedef struct  
{
    uint16_t command;
    union
    {
        uint8_t payload[AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t)];
        uint16_t payload_as_uint16[(AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t))/2];
    };
} main_mcu_command_message_t;

typedef struct  
{
    uint16_t event_id;
    uint8_t payload[];
} aux_mcu_event_message_t;

typedef struct  
{
    uint16_t charge_status;
    uint16_t battery_voltage;
    int16_t charge_current;
} nimh_charge_message_t;

typedef struct  
{
    uint16_t initial_device_status_value[2];
} ping_with_info_message_t;

typedef struct
{
    uint16_t interface_identifier;
    uint16_t delay_between_types;
    uint16_t keyboard_symbols[(AUX_MCU_MSG_PAYLOAD_LENGTH/2)-sizeof(uint16_t)-sizeof(uint16_t)];
} keyboard_type_message_t;

typedef struct
{
    uint16_t message_type;
    uint16_t payload_length1;
    union
    {
        aux_mcu_bootloader_message_t bootloader_message;
        aux_plat_details_message_t aux_details_message;
        main_mcu_command_message_t main_mcu_command_message;
        ping_with_info_message_t ping_with_info_message;
        aux_mcu_event_message_t aux_mcu_event_message;
        keyboard_type_message_t keyboard_type_message;
        nimh_charge_message_t nimh_charge_message;
        hid_message_t hid_message;
        uint8_t payload[AUX_MCU_MSG_PAYLOAD_LENGTH];
        uint16_t payload_as_uint16[AUX_MCU_MSG_PAYLOAD_LENGTH/2];
        uint32_t payload_as_uint32[AUX_MCU_MSG_PAYLOAD_LENGTH/4];    
    };
    uint16_t payload_length2;
    union
    {
        uint16_t rx_payload_valid_flag;
        uint16_t tx_not_used;        
    };
} aux_mcu_message_t;


/* Prototypes */
RET_TYPE comms_aux_mcu_active_wait(aux_mcu_message_t** rx_message_pt_pt, BOOL do_not_touch_dma_flags, uint16_t expected_packet, BOOL single_try, int16_t expected_event);
void comms_aux_mcu_get_empty_packet_ready_to_be_sent(aux_mcu_message_t** message_pt_pt, uint16_t message_type);
comms_msg_rcvd_te comms_aux_mcu_routine(msg_restrict_type_te answer_restrict_type);
void comms_aux_mcu_deal_with_received_event(aux_mcu_message_t* received_message);
aux_mcu_message_t* comms_aux_mcu_get_temp_tx_message_object_pt(void);
void comms_aux_mcu_send_simple_command_message(uint16_t command);
void comms_aux_mcu_hard_comms_reset_with_aux_mcu_reboot(void);
void comms_aux_mcu_update_device_status_buffer(void);
void comms_aux_mcu_send_message(BOOL wait_for_send);
RET_TYPE comms_aux_mcu_send_receive_ping(void);
void comms_aux_mcu_wait_for_message_sent(void);
void comms_aux_arm_rx_and_clear_no_comms(void);


#endif /* COMMS_AUX_MCU_H_ */
