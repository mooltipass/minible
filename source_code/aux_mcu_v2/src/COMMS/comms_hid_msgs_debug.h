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
#define HID_CMD_ID_DBG_MSG                  0x8000
#define HID_CMD_ID_OPEN_DISP_BUFFER         0x8001
#define HID_CMD_ID_SEND_TO_DISP_BUFFER      0x8002
#define HID_CMD_ID_CLOSE_DISP_BUFFER        0x8003
#define HID_CMD_ID_ERASE_DATA_FLASH         0x8004
#define HID_CMD_ID_IS_DATA_FLASH_READY      0x8005
#define HID_CMD_ID_DATAFLASH_WRITE_256B     0x8006
#define HID_CMD_ID_START_BOOTLOADER         0x8007
#define HID_CMD_ID_GET_ACC_32_SAMPLES       0x8008
#define HID_CMD_ID_FLASH_AUX_MCU            0x8009
#define HID_CMD_ID_GET_DBG_PLAT_INFO        0x800A
#define HID_CMD_ID_REINDEX_BUNDLE           0x800B
#define HID_CMD_ID_SET_OLED_PARAMS          0x800C
#define HID_CMD_ID_GET_BATTERY_STATUS       0x800D

/* Prototypes */
int16_t comms_hid_msgs_parse_debug(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* send_msg);


#endif /* COMMS_HID_MSGS_DEBUG_H_ */