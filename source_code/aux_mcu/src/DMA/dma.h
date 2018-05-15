/**
 * \file    dma.h
 * \author  MBorregoTrujillo
 * \date    08-March-2018
 * \brief   Direct Memory Access driver and util functions
 */
#ifndef DMA_H_
#define DMA_H_

#include <asf.h>

/* Prototypes */
void dma_init(void);
bool dma_aux_mcu_check_and_clear_dma_transfer_flag(void);
bool dma_aux_mcu_check_tx_dma_transfer_flag(void);
void dma_aux_mcu_init_tx_transfer(void* datap, uint16_t size);
void dma_aux_mcu_init_rx_transfer(void* datap, uint16_t size);
void dma_aux_mcu_disable_transfer(void);


#endif /* DMA_H_ */
