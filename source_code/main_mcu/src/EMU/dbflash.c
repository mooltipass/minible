#include "dbflash.h"
void dbflash_write_data_pattern_to_flash(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber, uint16_t offset, uint16_t dataSize, uint8_t pattern){}
void dbflash_send_data_with_four_bytes_opcode_no_readback(spi_flash_descriptor_t* descriptor_pt, uint8_t* opcode, uint8_t* buffer, uint16_t buffer_size){}
void dbflash_send_pattern_data_with_four_bytes_opcode(spi_flash_descriptor_t* descriptor_pt, uint8_t* opcode, uint8_t pattern, uint16_t nb_bytes){}
void dbflash_read_data_from_flash(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber, uint16_t offset, uint16_t dataSize, void *data){}
void dbflash_send_data_with_four_bytes_opcode(spi_flash_descriptor_t* descriptor_pt, uint8_t* opcode, uint8_t* buffer, uint16_t buffer_size){}
void dbflash_write_data_to_flash(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber, uint16_t offset, uint16_t dataSize, void *data){}
void dbflash_write_buffer(spi_flash_descriptor_t* descriptor_pt, uint8_t* datap, uint16_t offset, uint16_t size){}
void dbflash_raw_read(spi_flash_descriptor_t* descriptor_pt, uint8_t* datap, uint16_t addr, uint16_t size){}
void dbflash_load_page_to_internal_buffer(spi_flash_descriptor_t* descriptor_pt, uint16_t page_number){}
void dbflash_send_command(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length){}
void dbflash_flash_write_buffer_to_page(spi_flash_descriptor_t* descriptor_pt, uint16_t page){}
void dbflash_sector_zero_erase(spi_flash_descriptor_t* descriptor_pt, uint8_t sectorNumber){}
void dbflash_sector_erase(spi_flash_descriptor_t* descriptor_pt, uint8_t sectorNumber){}
void dbflash_block_erase(spi_flash_descriptor_t* descriptor_pt, uint16_t blockNumber){}
void dbflash_page_erase(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber){}
void dbflash_enter_ultra_deep_power_down(spi_flash_descriptor_t* descriptor_pt){}
RET_TYPE dbflash_check_presence(spi_flash_descriptor_t* descriptor_pt){ return RETURN_OK; }
void dbflash_wait_for_not_busy(spi_flash_descriptor_t* descriptor_pt){}
void dbflash_format_flash(spi_flash_descriptor_t* descriptor_pt) {}
void dbflash_chip_erase(spi_flash_descriptor_t* descriptor_pt) {}
void dbflash_memory_boundary_error_callblack(void){}
