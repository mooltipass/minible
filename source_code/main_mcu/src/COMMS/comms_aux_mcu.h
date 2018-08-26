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

/* Defines */
// Aux MCU Message Type
#define AUX_MCU_MSG_TYPE_USB            0x0000
#define AUX_MCU_MSG_TYPE_BLE            0x0001
#define AUX_MCU_MSG_TYPE_BOOTLOADER     0x0002
#define AUX_MCU_MSG_TYPE_PLAT_DETAILS   0x0003
#define AUX_MCU_MSG_TYPE_MAIN_MCU_CMD   0x0004
#define AUX_MCU_MSG_TYPE_AUX_MCU_EVENT  0x0005

// Main MCU commands
#define MAIN_MCU_COMMAND_SLEEP          0x0001
#define MAIN_MCU_COMMAND_ATTACH_USB     0x0002

/* Typedefs */
typedef struct
{
    uint16_t aux_fw_ver_major;
    uint16_t aux_fw_ver_minor;
    uint32_t aux_did_register;
    uint32_t aux_uid_registers[4];
    uint32_t btle_did;
    uint32_t btle_sdk_version;
    uint32_t btlc1000_rf_version;
} aux_plat_details_message_t;

typedef struct  
{
    uint16_t command;
    uint8_t payload[];
} main_mcu_command_message_t;

typedef struct
{
    uint16_t message_type;
    uint16_t payload_length1;
    union
    {
        aux_mcu_bootloader_message_t bootloader_message;
        aux_plat_details_message_t aux_details_message;
        main_mcu_command_message_t main_mcu_command_message;
        hid_message_t hid_message;
        uint8_t payload[AUX_MCU_MSG_PAYLOAD_LENGTH];
        uint32_t payload_as_uint32[AUX_MCU_MSG_PAYLOAD_LENGTH/4];    
    };
    uint16_t payload_length2;
    union
    {
        uint16_t rx_payload_valid_flag;
        uint16_t tx_reply_request_flag;        
    };
} aux_mcu_message_t;

/* Prototypes */
void comms_aux_mcu_send_message(aux_mcu_message_t* message, uint16_t message_length);
RET_TYPE comms_aux_mcu_active_wait(aux_mcu_message_t** rx_message_pt_pt);
aux_mcu_message_t* comms_aux_mcu_get_temp_tx_message_object_pt(void);
void comms_aux_mcu_wait_for_message_sent(void);
void comms_aux_arm_rx_and_clear_no_comms(void);
void comms_aux_mcu_send_sleep_message(void);
void comms_aux_mcu_routine(void);


#endif /* COMMS_AUX_MCU_H_ */
