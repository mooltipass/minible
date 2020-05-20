/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2020 Stephan Mathieu
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
/*!  \file     comms_hid_msgs_debug_defines.h
*    \brief    Defines for aux debug HID comms
*    Created:  20/05/2020
*    Author:   Mathieu Stephan
*/
#ifndef COMMS_HID_MSGS_DEBUG_DEFINES_H_
#define COMMS_HID_MSGS_DEBUG_DEFINES_H_

/* Defines */
#define HID_MESSAGE_START_CMD_ID_DBG        0x8000

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
#define HID_CMD_ID_FLASH_AUX_AND_MAIN       0x800E

#endif /* COMMS_HID_MSGS_DEBUG_DEFINES_H_ */