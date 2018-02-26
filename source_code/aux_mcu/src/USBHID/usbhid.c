/*
 * \file    usbhid.c
 * \author  MBorregoTrujillo
 * \date    22-February-2018
 * \brief   USB HID Protocol
 */

/* Includes */
#include "usbhid.h"
#include <string.h>

/* USBHID Commands */
#define USBHID_CMD_PING         (0x0001)


/** USBHID Constants **/
#define USBHID_MAX_TOTAL_PKTS   (16U)
#define USBHID_MAX_PAYLOAD      (62U)
#define USBHID_MAX_MSG_SIZE     (USBHID_MAX_PAYLOAD*USBHID_MAX_TOTAL_PKTS)
#define USBHID_MSG_HEADER_SIZE  (4U)

/** Function like macros */
#define USBHID_new_msg_received(flip)   (flip != current_msg_flip)
#define USBHID_check_pkt_counter(cnt)   ((cnt != pkt_counter) || \
                                         (pkt_counter >= USBHID_MAX_TOTAL_PKTS))

/** USBHID specific types  -------------------------------------------------  */

/** Typedef of the first two bytes of the HID packet structure */
typedef struct {
    /* Byte 0, LSB first */
    uint8_t len : 6;
    uint8_t final_ack :1;
    uint8_t msg_flip  :1;
    /* Byte 1, LSB first */
    uint8_t total_pkts    :4;
    uint8_t pkt_id        :4;
} T_usbhid_control;

/** HID Packet Structure */
typedef struct{
    T_usbhid_control control;
    uint8_t payload[USBHID_MAX_PAYLOAD];
} T_usbhid_pkt;

/** CMD Packet Structure */
typedef struct{
    /* Byte0,1 */
    uint16_t cmd;
    /* Byte2,3 */
    uint16_t len;
    /* Byte4,n */
    uint8_t* data;
} T_usbhid_msg;

/** Private function Declaration -------------------------------------------  */

/**
 * \fn          USBHID_msg_process
 * \brief       process the message received from HID and executes the command
 *              specified in the message
 *
 * \param msg   message to be processed, it contains the command, the length
 *              and pointer to data
 */
static void USBHID_msg_process(T_usbhid_msg msg);


/** Private data Declaration -----------------------------------------------  */

/** Message Buffer allocation */
static uint8_t buffer[USBHID_MAX_MSG_SIZE];
/** Stores the current message flip */
static uint8_t current_msg_flip;
/** Counts the number of packets to be received */
static uint8_t pkt_counter;
/** Total message length */
static uint16_t msg_size;



/** Public Function Implementation -----------------------------------------  */

void USBHID_init(void){
    current_msg_flip = 0u;
    pkt_counter = 0u;
    msg_size = 0u;
}

void USBHID_usb_callback(uint8_t *data){
    T_usbhid_pkt *pkt = (T_usbhid_pkt*)data;
    bool err = 0;
    T_usbhid_msg msg;

    /* Check new message flip bit  */
    if(USBHID_new_msg_received(pkt->control.msg_flip)){
        msg_size = 0;
        pkt_counter = 0u;
        current_msg_flip = pkt->control.msg_flip;
    }

    /* Analyze packet counter */
    if(USBHID_check_pkt_counter(pkt->control.pkt_id)){
        pkt_counter = 0u;
        err = 1;
    }

    /* Process incoming packet if no error has been detected */
    if( !err ){
        /* Copy from incoming pkt to buffer */
        memcpy(&buffer[msg_size], pkt->payload, pkt->control.len );
        msg_size+= pkt->control.len;

        /* Check if the end of packet has arrived */
        if((pkt->control.pkt_id == pkt->control.total_pkts)){
            /* Process Command */
            if( msg_size >= USBHID_MSG_HEADER_SIZE ){
                msg.cmd = (buffer[1] << 8) + buffer[0];
                msg.len = (buffer[3] << 8) + buffer[2];
                msg.data = &buffer[4];
                /* Consistency check between msg_size and msg.len */
                if( (msg_size-USBHID_MSG_HEADER_SIZE) == msg.len){
                    USBHID_msg_process(msg);

                    /* Answer to Host */
                    pkt->control.final_ack = 1;
                    udi_hid_generic_send_report_in(data, pkt->control.len+2);
                }
            }
        }
    } else{
        /* Protocol Error detected, reinitialize counters */
        msg_size = 0;
        pkt_counter = 0u;
        /* Send Error message ? */
    }

}


/** Private Function Implementation ----------------------------------------  */

static void USBHID_msg_process(T_usbhid_msg msg){

    switch(msg.cmd){
        case USBHID_CMD_PING:
            break;
        default:
            break;
    }
}

