/*
 * comm.c
 *
 * Created: 3/13/2018 7:50:13 PM
 *  Author: Borrego
 */ 

#include <asf.h>
#include "comm.h"
#include "dma.h"
#include "driver_sercom.h"

/* buffer for RX DMA USART */
uint8_t comm_rx_buffer[COMM_MAX_MSG_SIZE];

/**
 * \fn      comm_init
 * \brief   Initialize COMM component to be prepared for USART msg reception
 *          it shall be called after dma_init 
 */
void comm_init(void){
	// 52 is  115200 at 48Mhz clock
	//  1 is  6000000 at 48Mhz clock
	sercom_usart_init(SERCOM1, 1, USART_RX_PAD1, USART_TX_P0_XCK_P1);
	
	// init reception DMA
	dma_aux_mcu_init_rx_transfer(comm_rx_buffer, COMM_MAX_MSG_SIZE);
}


/**
 * \fn      comm_task
 * \brief   Processes incoming RX packets from MAIN MCU 
 */
void comm_task(void){
	/* Check if for USART RX reception through DMA */
	if(dma_aux_mcu_check_and_clear_dma_transfer_flag()){
		/* Process Heaader */
		T_comm_msg_type type = (comm_rx_buffer[1] << 8) + comm_rx_buffer[0];
		uint16_t buff_len = (comm_rx_buffer[3] << 8) + comm_rx_buffer[2];
		/* Process message and send it to destination */
		comm_process_out_msg(type, &comm_rx_buffer[COMM_HEADER_SIZE], buff_len );
	}
}

/**
 * \fn      comm_process_out_msg
 * \brief   
 */
void comm_process_out_msg(T_comm_msg_type msg_type, uint8_t* buff, uint16_t buff_len){
	
	switch(msg_type){
		case COMM_MSG_TO_USB:
			/* Send message to USB */
			usbhid_send_to_usb(buff, buff_len);
			break;
		case COMM_MSG_TO_BLE:
			/* Send message to BLE */
			break;
		default:
			break;
	}
}
/**
 * \fn      comm_process_in_msg
 * \brief   
 */
void comm_process_in_msg(T_comm_msg_type msg_type, uint8_t* buff, uint16_t buff_len){
	
	/* Send Header to MAIN MCU */
	sercom_send_single_byte(SERCOM1, (uint8_t)(msg_type & 0xFF));
	sercom_send_single_byte(SERCOM1, (uint8_t)((msg_type >> 8) & 0xFF));	
	sercom_send_single_byte(SERCOM1, (uint8_t)(buff_len & 0xFF));
	sercom_send_single_byte(SERCOM1, (uint8_t)((buff_len >> 8) & 0xFF));
	
	/* Send Payload to MAIN MCU */
	dma_aux_mcu_init_tx_transfer(buff, COMM_PAYLOAD_SIZE);
}