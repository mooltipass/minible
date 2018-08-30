/*!  \file     comms_bootloader_msg.h
*    \brief    Typedefs for bootloader messages
*    Created:  09/05/2018
*    Author:   Mathieu Stephan
*/

#ifndef COMMS_BOOTLOADER_MSG_H_
#define COMMS_BOOTLOADER_MSG_H_

#include <asf.h>
#include "comms_main_mcu.h"

/* Defines */
#define BOOTLOADER_PROGRAMMING_COMMAND  0x0000 
#define BOOTLOADER_WRITE_COMMAND        0x0001

/* Typedefs */
/* 0x0000: enter programming command */
typedef struct
{
    uint16_t reserved;
    __packed uint32_t image_length;
    __packed uint32_t crc;
} aux_mcu_bootloader_programming_command_contents_t;

/* 0x0001: write command */
typedef struct
{
    uint16_t size;
    __packed uint32_t crc;
    uint8_t payload[512];
} aux_mcu_bootloader_write_command_contents_t;

typedef struct
{
    uint16_t command;
    union
    {
        aux_mcu_bootloader_programming_command_contents_t programming_command;
        aux_mcu_bootloader_write_command_contents_t write_command;
    };
} aux_mcu_bootloader_message_t;



#endif /* COMMS_BOOTLOADER_MSG_H_ */