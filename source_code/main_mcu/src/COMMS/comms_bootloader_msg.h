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