/**
 * \file    comm_bootloader.c
 * \author  MBorregoTrujillo
 * \date    29-May-2018
 * \brief   Communications Manager for Bootloader
 */

#include "comm_bootloader.h"
#include "comm.h"
#include "dma.h"
#include "bootloader.h"

/* Following structure will be shared between Bootloader and Application
 * using Non Initialized Data Section
 */
T_comm_bootloader_enter_programming comm_enter_programming __attribute__((section(".nidata")));

#ifndef BOOTLOADER
bool comm_bootloader_process_msg(T_comm_bootloader_message *msg){
    switch(msg->command){
        case BOOTLOADER_PROGRAMMING_COMMAND:
            /* Store Enter Programming Data for Bootloader */
            comm_enter_programming.reserved = BOOTLOADER_MAGIC_KEY;
            comm_enter_programming.size = msg->programming_command.size;
            comm_enter_programming.crc = msg->programming_command.crc;

            /* Disable interrupts and RESET */
            cpu_irq_disable();
            NVIC_SystemReset();
            while(1);
        default:
            break;
    }
}
#else
/*! \fn    comm_bootloader_process_msg
 *  \brief Process commands received with Message Type equal to 0x0002
 *  \param msg     Pointer to bootloader message
 *  \return true    Command Succeed
 *  \return false   Command Failed
 */
bool comm_bootloader_process_msg(T_comm_bootloader_message *msg){
    bool ret = false;

    switch(msg->command){
        case BOOTLOADER_PROGRAMMING_COMMAND:
                bootloader_enter_programming(msg->programming_command.size, msg->programming_command.crc);
                ret = true;
                break;

        case BOOTLOADER_WRITE_COMMAND:
                if( bootloader_get_state() == BOOTLOADER_PROGRAM ){
                    bootloader_write(msg->write_command.data, msg->write_command.size);
                    ret = true;
                }
                break;
        default:
        break;
    }

    return ret;
}

/*! \fn    comm_bootloader_enter_programming_required
 *  \brief
 *  \return true    Magic Key received, enter into programming and answer
 *  \return false   Enter in programming not required
 */
bool comm_bootloader_enter_programming_required(void){
    T_comm_tx_msg tx;
    T_comm_bootloader_message* msg = (T_comm_bootloader_message*) tx.payload;

    /* Check for magic Key */
    if(comm_enter_programming.reserved == BOOTLOADER_MAGIC_KEY){
        /* Cleanup MAGIC KEY */
        comm_enter_programming.reserved = 0;

        /* Enter Programming command */
        bootloader_enter_programming(comm_enter_programming.size, comm_enter_programming.crc);

        /* Create Msg for MAIN Mcu */
        tx.msg_type = COMM_MSG_FROM_BOOTLOADER;
        msg->command = BOOTLOADER_PROGRAMMING_COMMAND;
        msg->programming_command.size = comm_enter_programming.size;
        msg->programming_command.crc = comm_enter_programming.crc;
        tx.payload_len1 = 2+sizeof(T_comm_bootloader_enter_programming);
        tx.payload_len2 = 2+sizeof(T_comm_bootloader_enter_programming);
        tx.payload_valid = 1;

        /* Dma transfer to Aux Mcu*/
        dma_aux_mcu_init_tx_transfer( &tx, sizeof(tx));
        while(dma_aux_mcu_check_tx_dma_transfer_flag() == DMA_TX_BUSY);
        return true;
    } else{
        return false;
    }
}
#endif
