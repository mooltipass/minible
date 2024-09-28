/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2019 Stephan Mathieu
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * dma.h
 *
 * Created: 29/05/2017 09:18:00
 *  Author: stephan
 */ 


#ifndef DMA_H_
#define DMA_H_

#include "platform_defines.h"

/* Prototypes */
void dma_oled_init_transfer(Sercom* sercom, void* datap, uint16_t size, uint16_t dma_trigger);
void dma_acc_init_transfer(Sercom* sercom, void* datap, uint16_t size, uint8_t* read_cmd);
uint32_t dma_compute_crc32_from_spi(Sercom* sercom, uint32_t size);
void dma_aux_mcu_init_tx_transfer(Sercom* sercom, void* datap, uint16_t size);
void dma_aux_mcu_init_rx_transfer(Sercom* sercom, void* datap, uint16_t size);
void dma_custom_fs_init_transfer(Sercom* sercom, void* datap, uint16_t size);
BOOL dma_aux_mcu_wait_for_current_packet_reception_and_clear_flag(void);
uint16_t dma_aux_mcu_get_remaining_bytes_for_rx_transfer(void);
BOOL dma_custom_fs_check_and_clear_dma_transfer_flag(void);
BOOL dma_aux_mcu_check_and_clear_dma_transfer_flag(void);
BOOL dma_oled_check_and_clear_dma_transfer_flag(void);
BOOL dma_acc_check_and_clear_dma_transfer_flag(void);
BOOL dma_aux_mcu_is_rx_transfer_already_init(void);
BOOL dma_aux_mcu_check_dma_transfer_flag(void);
void dma_wait_for_aux_mcu_packet_sent(void);
BOOL dma_acc_check_dma_transfer_flag(void);
void dma_aux_mcu_disable_transfer(void);
void dma_set_custom_fs_flag_done(void);
void dma_acc_disable_transfer(void);
void dma_reset(void);
void dma_init(void);


#endif /* DMA_H_ */
