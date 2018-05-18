/**
 * \file    comm.c
 * \author  MBorregoTrujillo
 * \date    13-March-2018
 * \brief   Communications Manager
 */

#include "comm.h"
#include "dma.h"
#include "driver_sercom.h"
#include <string.h>
#ifdef BOOTLOADER
    #include "bootloader.h"
#else
    #include "usbhid.h"
#endif

/* buffer for RX/TX DMA USART */
T_comm_rx_msg comm_rx __attribute__ ((aligned (4)));
T_comm_tx_msg comm_tx __attribute__ ((aligned (4)));

/* counter of the number of bytes transfered through DMA */
uint16_t comm_tx_dma_index;

/**
 * \fn      comm_deinit
 * \brief   De initialize COMM component
 */
void comm_deinit(void){
    sercom_usart_deinit(SERCOM1);
}

/**
 * \fn      comm_init
 * \brief   Initialize COMM component to be prepared for USART msg reception
 *          it shall be called after dma_init
 */
void comm_init(void){
    // 52 is  115200 at 48Mhz clock
    //  6 is  1000000 at 48Mhz clock
    //  1 is  6000000 at 48Mhz clock
    sercom_usart_init(SERCOM1, 1, USART_RX_PAD1, USART_TX_P0_XCK_P1);

    // init reception DMA
    dma_aux_mcu_init_rx_transfer(&comm_rx, sizeof(comm_rx));
}


/**
 * \fn      comm_task
 * \brief   Processes incoming RX packets from MAIN MCU
 */
void comm_task(void){
    /* Check if for USART RX reception through DMA */
    if(dma_aux_mcu_check_and_clear_dma_transfer_flag()){
        /* Process message and send it to destination */
        comm_process_out_msg((T_comm_msg_type)comm_rx.msg_type, comm_rx.payload, comm_rx.payload_len2);
        /* Reconfigure RX DMA, comm_rx is free at this point */
        dma_aux_mcu_init_rx_transfer(&comm_rx, sizeof(comm_rx));
    }
}

/**
 * \fn      comm_process_out_msg
 * \brief   Process outgoing messages from MAIN MCU and forward them to BLE or USB
 * \param   msg_type    Destination of the message, BLE or USB
 * \param   buff        Message pointer
 * \param   buff_len    Length of the message
 */
void comm_process_out_msg(T_comm_msg_type msg_type, uint8_t* buff, uint16_t buff_len){

    switch(msg_type){
#ifndef BOOTLOADER
        case COMM_MSG_TO_USB:
            /* Send message to USB */
            usbhid_send_to_usb(buff, buff_len);
            break;
        case COMM_MSG_TO_BLE:
            /* Send message to BLE */
            break;
#else
        case COMM_MSG_TO_BOOTLOADER:
            if(bootloader_process_msg(buff, buff_len) == true){
                memcpy(&comm_tx, &comm_rx, sizeof(T_comm_tx_msg));
                dma_aux_mcu_init_tx_transfer( &comm_tx, sizeof(comm_rx));
            } else{
                memcpy(&comm_tx, &comm_rx, sizeof(T_comm_tx_msg));
                memset(comm_tx.payload, 0xFF, COMM_PAYLOAD_SIZE);
                dma_aux_mcu_init_tx_transfer( &comm_tx, sizeof(comm_rx));
            }
            break;
#endif
        default:
            break;
    }
}

/**
 * \fn    comm_usb_process_in_pkt
 * \brief Process incoming packets from USB and forward them to MAIN MCU
 *
 * \param pkt_status Indicates if start and end of packet
 * \param data       Buffer to the data to transmit
 * \param data_len   Length of the data to transmit
 *
 * \return void
 */
void comm_usb_process_in_pkt(T_comm_pkt_status pkt_status, uint8_t* data, uint16_t data_len){

    /* Number of bytes to transfer through dma */
    uint16_t dma_bytes = data_len;
    /* err var */
    bool err = false;

    /* Actions to do if start of pkt */
    if( pkt_status.msg_start ){
        /* Reset vars */
        comm_tx_dma_index = 0u;
        comm_tx.payload_len2 = 0u;
        /* Write Header */
        comm_tx.msg_type = COMM_MSG_FROM_USB;
        comm_tx.payload_len1 = 0u;
        /* Increment the number bytes to be sent */
        dma_bytes += sizeof(comm_tx.msg_type);
        dma_bytes += sizeof(comm_tx.payload_len1);
    }

    /* Payload */
    if( (comm_tx.payload_len2+data_len) <= COMM_PAYLOAD_SIZE ){
        /* copy data into comm_tx payload */
        memcpy(&comm_tx.payload[comm_tx.payload_len2], data, data_len );
        comm_tx.payload_len2 += data_len;
    } else{
        err = true;
    }

    /* Actions to do if end of pkt */
    if( pkt_status.msg_end ){
        /* Increment the bytes to be sent */
        dma_bytes += (COMM_PAYLOAD_SIZE - comm_tx.payload_len2);
        dma_bytes += sizeof(comm_tx.payload_len2);
        dma_bytes += sizeof(comm_tx.payload_valid);
        /* */
        if(data_len == comm_tx.payload_len2){
            comm_tx.payload_len1 = comm_tx.payload_len2;
        }

        /* valid message */
        comm_tx.payload_valid = COMM_PAYLOAD_VALID;
    }

    /* Data transfer through DMA if no error */
    if(!err){
        dma_aux_mcu_init_tx_transfer( ((uint8_t*)&comm_tx)+comm_tx_dma_index, dma_bytes);
        comm_tx_dma_index += dma_bytes;
    }
}
