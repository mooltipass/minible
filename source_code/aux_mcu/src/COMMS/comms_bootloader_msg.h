/*!  \file     comms_bootloader_msg.h
*    \brief    Typedefs for bootloader messages
*    Created:  09/05/2018
*    Author:   Mathieu Stephan
*/

#ifndef COMMS_BOOTLOADER_MSG_H_
#define COMMS_BOOTLOADER_MSG_H_

#ifndef BOOTLOADER
    #include <asf.h>
#else
    #include "sam.h"
#endif
#include "comms_main_mcu.h"

/* Defines */
#define BOOTLOADER_START_PROGRAMMING_COMMAND    0x0000
#define BOOTLOADER_WRITE_COMMAND                0x0001
#define BOOTLOADER_START_APP_COMMAND            0x0002

/* Typedefs */
/* 0x0000: enter programming command */
typedef struct
{
    uint32_t image_length;
    uint32_t crc;
} aux_mcu_bootloader_programming_command_contents_t;

/* 0x0001: write command */
typedef struct
{
    uint32_t size;
    uint32_t crc;
    uint32_t address;
    union
    {
        uint8_t payload[512];
        uint16_t payload_as_uint16_t[512/2];        
    };
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