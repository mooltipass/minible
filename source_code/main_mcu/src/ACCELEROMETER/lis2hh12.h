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
/*!  \file     lis2hh12.h
*    \brief    LIS2HH12 accelerometer related functions
*    Created:  01/02/2018
*    Author:   Mathieu Stephan
*/

#ifndef LIS2HH12_H_
#define LIS2HH12_H_

#include "platform_defines.h"
#ifndef MINIBLE_V2

#include <stdio.h>

/* Structs */
typedef struct
{
    /* Each data actually is 16 bits long */
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
} acc_data_t;

typedef struct __attribute__((packed))
{
    uint8_t wasted_byte_for_read_cmd;
    acc_data_t acc_data_array[32];
} acc_single_fifo_read_t;    

typedef struct
{
    Sercom* sercom_pt;
    pin_group_te cs_pin_group;
    PIN_MASK_T cs_pin_mask;
    pin_group_te int_pin_group;
    PIN_MASK_T int_pin_mask;
    uint16_t evgen_sel;
    uint16_t evgen_channel;
    uint16_t dma_channel;
    uint8_t read_cmd;
    acc_single_fifo_read_t fifo_read;
} accelerometer_descriptor_t;

/* Prototypes */
BOOL lis2hh12_check_data_received_flag_and_arm_other_transfer(accelerometer_descriptor_t* descriptor_pt, BOOL arm_other_transfer);
void lis2hh12_send_command(accelerometer_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length);
void lis2hh12_manual_acc_data_read(accelerometer_descriptor_t* descriptor_pt, acc_data_t* data_pt);
RET_TYPE lis2hh12_check_presence_and_configure(accelerometer_descriptor_t* descriptor_pt);
void lis2hh12_deassert_ncs_and_go_to_sleep(accelerometer_descriptor_t* descriptor_pt);
void lis2hh12_sleep_exit_and_dma_arm(accelerometer_descriptor_t* descriptor_pt);
int16_t lis2hh12_get_temperature(accelerometer_descriptor_t* descriptor_pt);
void lis2hh12_dma_arm(accelerometer_descriptor_t* descriptor_pt);
void lis2hh12_reset(accelerometer_descriptor_t* descriptor_pt);


#endif /* MINIBLE_V2 */
#endif /* LIS2HH12_H_ */