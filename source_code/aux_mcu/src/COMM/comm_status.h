/**
 * \file    comm_status.h
 * \author  MBorregoTrujillo
 * \date    18-June-2018
 * \brief   Aux Mcu status
 */


#ifndef COMM_STATUS_H_
#define COMM_STATUS_H_

#include <asf.h>

/* Status Type */
typedef struct{
    uint16_t fw_version_major;
    uint16_t fw_version_minor;
    uint32_t mcu_did;
    uint32_t mcu_serial_number[4];
    uint32_t ble_did;
}T_comm_status_msg;

/* Function Prototype */
void comm_status_init(void);
void comm_status_answer(void);

#endif /* COMM_STATUS_H_ */