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
#define COMM_PAYLOAD_SIZE       (536U)
#define COMM_PAYLOAD_VALID      (true)
#define COMM_PAYLOAD_NOT_VALID  (false)

/* Message type */
typedef struct {
    uint16_t msg_type;
    uint16_t reserved;
    uint8_t  payload[COMM_PAYLOAD_SIZE];
    uint16_t payload_len;
    uint16_t payload_valid;
} T_comm_msg;

/* Message Type enum */
typedef enum comm_msg_type{
    COMM_MSG_TO_USB = 0,
    COMM_MSG_FROM_USB = COMM_MSG_TO_USB,
    COMM_MSG_TO_BLE = 1,
    COMM_MSG_FROM_BLE = COMM_MSG_TO_BLE,
} T_comm_msg_type;

/* Indicate pkt status */
typedef struct{
    uint8_t msg_start :1;
    uint8_t msg_end   :1;
} T_comm_pkt_status;

void comm_init(void);
void comm_task(void);
void comm_usb_process_in_pkt(T_comm_pkt_status pkt_status, uint8_t* data, uint16_t data_len);
void comm_process_out_msg(T_comm_msg_type msg_type, uint8_t* buff, uint16_t buff_len);

#endif /* COMM_H_ */
