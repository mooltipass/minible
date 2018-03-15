/*
 * \file    usbhid.c
 * \author  MBorregoTrujillo
 * \date    22-February-2018
 * \brief   USB HID Protocol
 */

/* Includes */
#include <string.h>
#include "usbhid.h"
#include "dma.h"
#include "comm.h"
#include "driver_sercom.h"

/* USBHID Commands */
#define USBHID_CMD_PING         (0x0001)

/** Function like macros */
#define USBHID_new_msg_received(flip)   (flip != rx_msg_flip)
#define USBHID_check_pkt_counter(cnt)   ((cnt != rx_pkt_counter) || \
                                         (rx_pkt_counter >= USBHID_MAX_TOTAL_PKTS))

/** USBHID specific types  -------------------------------------------------  */

/** Typedef of the first two bytes of the HID packet structure */
typedef struct {
    /* Byte 0, LSB first */
    uint8_t len :6;
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
static bool usbhid_msg_process(uint8_t* buff, uint16_t buff_len);


/** Private data Declaration -----------------------------------------------  */

/** Message Buffer allocation */
static uint8_t usbhid_rx_buffer[USBHID_MAX_MSG_SIZE];
/** Stores the current message flip */
static bool rx_msg_flip;
/** Counts the number of packets to be received */
static uint8_t rx_pkt_counter;
/** Total message length */
static uint16_t rx_msg_size;



/** Public Function Implementation -----------------------------------------  */
/**
 * \fn      USBHID_init
 * \brief   Initialize USBHID internal variables
 */
void usbhid_init(void){
    rx_msg_flip = false;
    rx_pkt_counter = 0u;
    rx_msg_size = 0u;
}

/**
 * \fn          USBHID_usb_callback
 * \brief       Callback function which receives raw hid packet to be processed
 *              following minible USB HID Protocol. The callback is called from
 *              udi_hid_generic_report_out_received function and configured in
 *              conf_usb.h file.
 *
 * \param data  Pointer to raw hid packet received from USB
 */
void usbhid_usb_callback(uint8_t *data){
    T_usbhid_pkt *pkt = (T_usbhid_pkt*)data;
    bool err = false;

    /* Check new message flip bit  */
    if(USBHID_new_msg_received(pkt->control.msg_flip)){
        rx_msg_size = 0u;
        rx_pkt_counter = 0u;
        rx_msg_flip = pkt->control.msg_flip;
    }

    /* Analyze packet counter */
    if(USBHID_check_pkt_counter(pkt->control.pkt_id)){
        err = true;
    }

    /* Process incoming packet if no error has been detected */
    if( !err ){
        /* Copy from incoming pkt to buffer */
        memcpy(&usbhid_rx_buffer[rx_msg_size], pkt->payload, pkt->control.len );
        rx_msg_size+= pkt->control.len;

        /* Check if the end of packet has arrived */
        if((pkt->control.pkt_id == pkt->control.total_pkts)){
            /* Process Command */
            err = usbhid_msg_process(usbhid_rx_buffer, rx_msg_size);
			/* Consistency check */
			if(!err){
				/* Answer to Host */
				pkt->control.final_ack = true;
				udi_hid_generic_send_report_in(data, pkt->control.len+USBHID_PKT_HEADER_SIZE);
			}
            /* Reset Counters */
            rx_msg_size = 0u;
            rx_pkt_counter = 0u;
        }
    } else{
        /* Protocol Error detected, reinitialize counters */
        rx_msg_size = 0u;
        rx_pkt_counter = 0u;
        /* Send Error message ? */
    }

}


/** Private Function Implementation ----------------------------------------  */

static bool usbhid_msg_process(uint8_t* buff, uint16_t buff_len){
	bool err = false;
	T_usbhid_msg msg;
	
	msg.cmd = (buff[1] << 8) + buff[0];
	msg.len = (buff[3] << 8) + buff[2];
	
	msg.data = &buff[USBHID_MSG_HEADER_SIZE];
	
	/* Buffer Length shall be greater than Message header */
	if( buff_len < USBHID_MSG_HEADER_SIZE ){
		err = true;
	}
	/* Consistency check between msg_size and msg.len */
	else if( (buff_len-USBHID_MSG_HEADER_SIZE) != msg.len){
		err = true;
	}
	
	if(!err){
		switch(msg.cmd){
			case USBHID_CMD_PING:
			default:
				comm_process_in_msg(COMM_MSG_FROM_USB, buff, buff_len);
				break;
		}
	}
	
	return err;
}

void usbhid_send_to_usb(uint8_t* buff, uint16_t buff_len){
	uint16_t pkt_number;
	uint8_t pkt_id;
	T_usbhid_pkt pkt;
	uint16_t buff_idx = 0;
	
	/* Compute packet fragmentation */
	pkt_number = buff_len / USBHID_MAX_MSG_SIZE;

	if( ((buff_len%USBHID_MAX_MSG_SIZE) == 0) &&
		(buff_len != 0) ){
		pkt_number--;
	}
	
	for(pkt_id = 0; pkt_id <= pkt_number; pkt_id++){
		/* Compute Header */
		pkt.control.len = ((uint16_t)(buff_len - buff_idx) >= USBHID_MAX_PAYLOAD) ? (USBHID_MAX_PAYLOAD) : (uint8_t)(buff_len - buff_idx);
		pkt.control.msg_flip = rx_msg_flip;
		pkt.control.pkt_id = pkt_id;
		pkt.control.total_pkts = pkt_number;
		pkt.control.final_ack = false;
		
		/* copy data to pkt.payload */
		memcpy(pkt.payload, &buff[buff_idx], pkt.control.len);
		buff_idx += pkt.control.len;
			
		/* 
		 * After this call we can use the buffer again, as it copies to an internal 
		 * buffer to send the data through USB
		 */
		while(udi_hid_generic_send_report_in((uint8_t*)&pkt, pkt.control.len+USBHID_PKT_HEADER_SIZE));
	}
}