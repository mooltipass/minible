/*
 * dma.h
 *
 * Created: 29/05/2017 09:18:00
 *  Author: stephan
 */ 


#ifndef DMA_H_
#define DMA_H_

/* Prototypes */
void dma_init(void);
bool dma_aux_mcu_check_and_clear_dma_transfer_flag(void);
void dma_aux_mcu_init_tx_transfer(void* datap, uint16_t size);
void dma_aux_mcu_init_rx_transfer(void* datap, uint16_t size);


#endif /* DMA_H_ */
