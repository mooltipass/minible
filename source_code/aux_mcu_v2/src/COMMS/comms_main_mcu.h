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

/* Share vars */
extern volatile BOOL comms_main_mcu_usb_msg_answered_using_first_bytes;
extern volatile BOOL comms_main_mcu_ble_msg_answered_using_first_bytes;
extern volatile BOOL comms_main_mcu_other_msg_answered_using_first_bytes;

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
#define MAIN_MCU_COMMAND_PING           0x0003
#define MAIN_MCU_COMMAND_ENABLE_BLE     0x0004

// Aux MCU events
#define AUX_MCU_EVENT_BLE_ENABLED       0x0001

// Flags
#define TX_NO_REPLY_REQUEST_FLAG        0x0000
#define TX_REPLY_REQUEST_FLAG           0x0001

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
    uint8_t payload[];
} main_mcu_command_message_t;

typedef struct  
{
    uint16_t event_id;
    uint8_t payload[];
} aux_mcu_event_message_t;

typedef struct
{
    uint16_t message_type;
    uint16_t payload_length1;
    union
    {
        aux_mcu_bootloader_message_t bootloader_message;
        aux_plat_details_message_t aux_details_message;
        main_mcu_command_message_t main_mcu_command_message;
        aux_mcu_event_message_t aux_mcu_event_message;
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
void comms_main_mcu_send_message(aux_mcu_message_t* message, uint16_t message_length);
BOOL comms_aux_mcu_get_received_packet(aux_mcu_message_t** message, BOOL arm_new_rx);
void comms_main_mcu_deal_with_non_usb_non_ble_message(aux_mcu_message_t* message);
aux_mcu_message_t* comms_main_mcu_get_temp_tx_message_object_pt(void);
void comms_main_mcu_routine(void);
void comms_main_init_rx(void);


#endif /* COMMS_AUX_MCU_H_ */
