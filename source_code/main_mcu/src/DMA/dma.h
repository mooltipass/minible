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

/* Prototypes */
void dma_oled_init_transfer(void* spi_data_p, void* datap, uint16_t size, uint16_t dma_trigger);
void dma_acc_init_transfer(void* spi_data_p, void* datap, uint16_t size, uint8_t* read_cmd);
uint32_t dma_bootloader_compute_crc32_from_spi(void* spi_data_p, uint32_t size);
void dma_aux_mcu_init_tx_transfer(void* spi_data_p, void* datap, uint16_t size);
void dma_aux_mcu_init_rx_transfer(void* spi_data_p, void* datap, uint16_t size);
void dma_custom_fs_init_transfer(void* spi_data_p, void* datap, uint16_t size);
uint16_t dma_aux_mcu_get_remaining_bytes_for_rx_transfer(void);
BOOL dma_custom_fs_check_and_clear_dma_transfer_flag(void);
BOOL dma_aux_mcu_check_and_clear_dma_transfer_flag(void);
BOOL dma_oled_check_and_clear_dma_transfer_flag(void);
BOOL dma_acc_check_and_clear_dma_transfer_flag(void);
void dma_aux_mcu_disable_transfer(void);
void dma_set_custom_fs_flag_done(void);
void dma_acc_disable_transfer(void);
void dma_init(void);


#endif /* DMA_H_ */
