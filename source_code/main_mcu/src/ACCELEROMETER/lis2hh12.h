/*!  \file     lis2hh12.h
*    \brief    LIS2HH12 accelerometer related functions
*    Created:  01/02/2018
*    Author:   Mathieu Stephan
*/

#ifndef LIS2HH12_H_
#define LIS2HH12_H_

#include <stdio.h>
#include "platform_defines.h"

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
BOOL lis2hh12_check_data_received_flag_and_arm_other_transfer(accelerometer_descriptor_t* descriptor_pt);
void lis2hh12_send_command(accelerometer_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length);
void lis2hh12_manual_acc_data_read(accelerometer_descriptor_t* descriptor_pt, acc_data_t* data_pt);
RET_TYPE lis2hh12_check_presence_and_configure(accelerometer_descriptor_t* descriptor_pt);
void lis2hh12_deassert_ncs_and_go_to_sleep(accelerometer_descriptor_t* descriptor_pt);
void lis2hh12_sleep_exit_and_dma_arm(accelerometer_descriptor_t* descriptor_pt);
void lis2hh12_reset(accelerometer_descriptor_t* descriptor_pt);

#endif /* LIS2HH12_H_ */