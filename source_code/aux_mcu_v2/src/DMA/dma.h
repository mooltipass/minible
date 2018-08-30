/*
 * dma.h
 *
 * Created: 29/05/2017 09:18:00
 *  Author: stephan
 */ 


#ifndef DMA_H_
#define DMA_H_

#include "platform_defines.h"
#include "defines.h"

/* Global vars */
extern volatile aux_mcu_message_t dma_main_mcu_temp_rcv_message;
extern volatile aux_mcu_message_t dma_main_mcu_usb_rcv_message;
extern volatile aux_mcu_message_t dma_main_mcu_ble_rcv_message;
extern volatile aux_mcu_message_t dma_main_mcu_other_message;
extern volatile BOOL dma_main_mcu_other_msg_received;
extern volatile BOOL dma_main_mcu_usb_msg_received;
extern volatile BOOL dma_main_mcu_ble_msg_received;

/* Prototypes */
void dma_main_mcu_init_tx_transfer(void* spi_data_p, void* datap, uint16_t size);
uint16_t dma_main_mcu_get_remaining_bytes_for_rx_transfer(void);
BOOL dma_main_mcu_check_and_clear_dma_transfer_flag(void);
void dma_wait_for_main_mcu_packet_sent(void);
void dma_main_mcu_init_rx_transfer(void);
void dma_main_mcu_disable_transfer(void);
void dma_init(void);


#endif /* DMA_H_ */
