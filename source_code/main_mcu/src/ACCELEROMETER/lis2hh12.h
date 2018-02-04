/*!  \file     lis2hh12.h
*    \brief    LIS2HH12 accelerometer related functions
*    Created:  01/02/2018
*    Author:   Mathieu Stephan
*/

#ifndef LIS2HH12_H_
#define LIS2HH12_H_

#include <stdio.h>

/* Structs */
typedef struct
{
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;    
} acc_data;

/* Prototypes */
void lis2hh12_send_command(accelerometer_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length);
void lis2hh12_manual_acc_data_read(accelerometer_descriptor_t* descriptor_pt, acc_data* data_pt);
RET_TYPE lis2hh12_check_presence_and_configure(accelerometer_descriptor_t* descriptor_pt);

#endif /* LIS2HH12_H_ */