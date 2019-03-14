/*!  \file     comms_hid_msgs.h
*    \brief    HID communications
*    Created:  06/03/2018
*    Author:   Mathieu Stephan
*/ 


#ifndef COMMS_HID_MSGS_H_
#define COMMS_HID_MSGS_H_

#include "comms_aux_mcu_defines.h"
#include "defines.h"

/* Defines */
#define HID_1BYTE_NACK      0x00
#define HID_1BYTE_ACK       0x01

/* Command defines */
#define HID_CMD_ID_PING         0x0001
#define HID_CMD_ID_RETRY        0x0002
#define HID_CMD_ID_PLAT_INFO    0x0003
#define HID_CMD_ID_SET_DATE     0x0004
#define HID_CMD_ID_CANCEL_REQ   0x0005
#define HID_CMD_ID_STORE_CRED   0x0006
#define HID_CMD_ID_GET_CRED     0x0007

/* Typedefs */
typedef struct
{
    uint8_t aux_mcu_infos[64];
    uint16_t main_mcu_fw_major;
    uint16_t main_mcu_fw_minor;
} hid_message_detailed_plat_info_t;

typedef struct
{
    uint16_t main_mcu_fw_major;
    uint16_t main_mcu_fw_minor;
    uint16_t aux_mcu_fw_major;
    uint16_t aux_mcu_fw_minor;
    uint32_t plat_serial_number;
    uint16_t memory_size;
} hid_message_plat_info_t;

typedef struct  
{
    uint16_t service_name_index;
    uint16_t login_name_index;
    uint16_t description_index;
    uint16_t third_field_index;
    uint16_t password_index;
    cust_char_t concatenated_strings[0];
} hid_message_store_cred_t;

typedef struct
{
    uint16_t message_type;
    uint16_t payload_length;
    union
    {
        uint8_t payload[AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t)-sizeof(uint16_t)];
        uint16_t payload_as_uint16[(AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t)-sizeof(uint16_t))/sizeof(uint16_t)];
        uint32_t payload_as_uint32[(AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t)-sizeof(uint16_t))/sizeof(uint32_t)];
        cust_char_t payload_as_cust_char_t[(AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t)-sizeof(uint16_t))/sizeof(cust_char_t)];
        hid_message_detailed_plat_info_t detailed_platform_info;
        hid_message_plat_info_t platform_info;
        hid_message_store_cred_t store_credential;
    };
} hid_message_t;

/* Prototypes */
int16_t comms_hid_msgs_parse(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* send_msg, msg_restrict_type_te answer_restrict_type);


#endif /* COMMS_HID_MSGS_H_ */