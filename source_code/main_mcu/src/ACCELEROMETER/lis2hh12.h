/*!  \file     lis2hh12.h
*    \brief    LIS2HH12 accelerometer related functions
*    Created:  01/02/2018
*    Author:   Mathieu Stephan
*/

#ifndef LIS2HH12_H_
#define LIS2HH12_H_

/* Prototypes */
void lis2hh12_send_command(accelerometer_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length);
RET_TYPE lis2hh12_check_presence(accelerometer_descriptor_t* descriptor_pt);

#endif /* LIS2HH12_H_ */