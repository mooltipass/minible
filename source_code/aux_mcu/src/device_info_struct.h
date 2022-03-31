/*
 * device_info_struct.h
 *
 * Created: 31/03/2022 21:32:03
 *  Author: limpkin
 */ 


#ifndef DEVICE_INFO_STRUCT_H_
#define DEVICE_INFO_STRUCT_H_

#include <stdint.h>

typedef struct
{
    uint8_t mac_address[6];
    uint8_t fw_major;
    uint8_t fw_minor;
    uint32_t serial_number;
    uint16_t bundle_version;
    uint8_t custom_device_name[22+1];
} dis_device_information_t;

#endif /* DEVICE_INFO_STRUCT_H_ */