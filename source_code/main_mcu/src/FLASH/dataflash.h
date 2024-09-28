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
/*!  \file     dataflash.h
*    \brief    Low level driver for W25Q16 flash
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/
#ifndef W25Q16_H_
#define W25Q16_H_

#include "platform_defines.h"

/* Defines */
#define W25Q16_PAGE_SIZE    256
#define W25Q16_FLASH_SIZE   2097152UL

/* Prototypes */
void dataflash_write_array_to_memory(spi_flash_descriptor_t* descriptor_pt, uint32_t address, uint8_t* data, uint32_t length);
void dataflash_read_data_array(spi_flash_descriptor_t* descriptor_pt, uint32_t address, uint8_t* data, uint32_t length);
void dataflash_read_bytes_from_opened_transfer(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length);
void dataflash_send_command(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length);
void dataflash_send_single_byte_command(spi_flash_descriptor_t* descriptor_pt, uint8_t command);
void dataflash_read_data_array_start(spi_flash_descriptor_t* descriptor_pt, uint32_t address);
void dataflash_erase_64kb_block(spi_flash_descriptor_t* descriptor_pt, uint32_t address);
void dataflash_bulk_erase_without_wait(spi_flash_descriptor_t* descriptor_pt);
uint8_t dataflash_read_status_register(spi_flash_descriptor_t* descriptor_pt);
void dataflash_stop_ongoing_transfer(spi_flash_descriptor_t* descriptor_pt);
void dataflash_bulk_erase_with_wait(spi_flash_descriptor_t* descriptor_pt);
RET_TYPE dataflash_check_presence(spi_flash_descriptor_t* descriptor_pt);
void dataflash_send_write_enable(spi_flash_descriptor_t* descriptor_pt);
void dataflash_wait_for_not_busy(spi_flash_descriptor_t* descriptor_pt);
void dataflash_exit_power_down(spi_flash_descriptor_t* descriptor_pt);
RET_TYPE dataflash_is_busy(spi_flash_descriptor_t* descriptor_pt);
void dataflash_power_down(spi_flash_descriptor_t* descriptor_pt);

#endif /* W25Q16_H_ */