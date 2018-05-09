/*!  \file     comms_bootloader_msg.h
*    \brief    Typedefs for bootloader messages
*    Created:  09/05/2018
*    Author:   Mathieu Stephan
*/

#ifndef COMMS_BOOTLOADER_MSG_H_
#define COMMS_BOOTLOADER_MSG_H_

#include <asf.h>
#include "comms_aux_mcu.h"

/* Defines */
#define BOOTLOADER_PROGRAMMING_COMMAND  0x0000 

/* Typedefs */
typedef struct
{
    uint16_t reserved;
    __packed uint32_t image_length;
    __packed uint32_t crc;
} aux_mcu_bootloader_programming_command_contents_t;

typedef struct
{
    uint16_t command;
    union
    {
        aux_mcu_bootloader_programming_command_contents_t programming_command;
    };
} aux_mcu_bootloader_message_t;



#endif /* COMMS_BOOTLOADER_MSG_H_ */