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
#define HID_CMD_ID_PING     0x0001
#define HID_CMD_ID_RETRY    0x0002

/* Typedefs */
typedef struct
{
    uint8_t aux_mcu_infos[64];
    uint16_t main_mcu_fw_major;
    uint16_t main_mcu_fw_minor;
} hid_message_detailed_plat_info_t;

typedef struct
{
    uint16_t message_type;
    uint16_t payload_length;
    union
    {
        uint8_t payload[AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t)-sizeof(uint16_t)];
        uint16_t payload_as_uint16[(AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t)-sizeof(uint16_t))/2];
        uint32_t payload_as_uint32[(AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t)-sizeof(uint16_t))/4];
        hid_message_detailed_plat_info_t detailed_platform_info;
    };
} hid_message_t;

/* Prototypes */
int16_t comms_hid_msgs_parse(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* send_msg, BOOL do_not_deal_with);


#endif /* COMMS_HID_MSGS_H_ */