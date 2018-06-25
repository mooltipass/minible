/**
 * \file    comm_status.c
 * \author  MBorregoTrujillo
 * \date    18-June-2018
 * \brief   Aux Mcu status
 */
#include "comm_status.h"
#include "comm.h"
#include "dma.h"
#include <string.h>

#define COMM_STATUS_VERSION_MAJOR    (0u)
#define COMM_STATUS_VERSION_MINOR    (1u)

/* static var to store status of aux mcu */
static T_comm_status_msg comm_status;
uint32_t comm_status_err = 0u;


static T_comm_tx_msg comm_status_msg __attribute__ ((aligned (4)));

/**
 * \fn      comm_status_init
 * \brief   Initialize comm_status variable
 */
void comm_status_init(void){
    /* Status data */
    comm_status.fw_version_major = COMM_STATUS_VERSION_MAJOR;
    comm_status.fw_version_minor = COMM_STATUS_VERSION_MINOR;
    comm_status.mcu_did = DSU->DID.reg;
    comm_status.mcu_serial_number[0] = *(uint32_t*)0x0080A00C;
    comm_status.mcu_serial_number[1] = *(uint32_t*)0x0080A040;
    comm_status.mcu_serial_number[2] = *(uint32_t*)0x0080A044;
    comm_status.mcu_serial_number[3] = *(uint32_t*)0x0080A048;
    comm_status.ble_did = 0u;

    /* Status vars */
    comm_status_err = 0u;
}

/**
 *  \fn    comm_status_answer
 *  \brief Answer to main mcu STATUS message
 */
void comm_status_answer(void){
    comm_status_msg.msg_type = COMM_MSG_STATUS;
    comm_status_msg.payload_len1 = sizeof(comm_status);
    comm_status_msg.payload_len2 = comm_status_msg.payload_len1;
    comm_status_msg.payload_valid = COMM_PAYLOAD_VALID;
    memcpy(comm_status_msg.payload, &comm_status, sizeof(comm_status));

    /* If dma no transfer pending: */
    if(dma_aux_mcu_check_tx_dma_transfer_flag() == DMA_TX_FREE){
        dma_aux_mcu_init_tx_transfer(&comm_status_msg, sizeof(comm_status_msg));
    }else{

        /* Increase error */
        if(comm_status_err < UINT32_MAX){
            comm_status_err++;
        }
    }
}
