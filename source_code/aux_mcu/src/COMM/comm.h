/**
 * \file    comm.h
 * \author  MBorregoTrujillo
 * \date    13-March-2018
 * \brief   Communications Manager
 */

#ifndef COMM_H_
#define COMM_H_

#include <asf.h>

/* Constant definition */
#define COMM_MAX_MSG_SIZE   (536U)
#define COMM_HEADER_SIZE    (4U)
#define COMM_PAYLOAD_SIZE   (COMM_MAX_MSG_SIZE-COMM_HEADER_SIZE)

/* Message Type enum */
typedef enum comm_msg_type{
    COMM_MSG_TO_USB = 0,
    COMM_MSG_FROM_USB = COMM_MSG_TO_USB,
    COMM_MSG_TO_BLE = 1,
    COMM_MSG_FROM_BLE = COMM_MSG_TO_BLE,
} T_comm_msg_type;

void comm_init(void);
void comm_task(void);
void comm_process_in_msg(T_comm_msg_type msg_type, uint8_t* buff, uint16_t buff_len);
void comm_process_out_msg(T_comm_msg_type msg_type, uint8_t* buff, uint16_t buff_len);

#endif /* COMM_H_ */
