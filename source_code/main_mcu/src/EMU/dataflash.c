#include "dataflash.h"

void dataflash_write_array_to_memory(spi_flash_descriptor_t* descriptor_pt, uint32_t address, uint8_t* data, uint32_t length){}
void dataflash_read_data_array(spi_flash_descriptor_t* descriptor_pt, uint32_t address, uint8_t* data, uint32_t length) {}
void dataflash_read_bytes_from_opened_transfer(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length) {}
void dataflash_send_command(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length){}
void dataflash_send_single_byte_command(spi_flash_descriptor_t* descriptor_pt, uint8_t command){}
void dataflash_read_data_array_start(spi_flash_descriptor_t* descriptor_pt, uint32_t address){}
void dataflash_erase_64kb_block(spi_flash_descriptor_t* descriptor_pt, uint32_t address){}
void dataflash_bulk_erase_without_wait(spi_flash_descriptor_t* descriptor_pt){}
uint8_t dataflash_read_status_register(spi_flash_descriptor_t* descriptor_pt){return 0;}
void dataflash_stop_ongoing_transfer(spi_flash_descriptor_t* descriptor_pt){}
void dataflash_bulk_erase_with_wait(spi_flash_descriptor_t* descriptor_pt){}
RET_TYPE dataflash_check_presence(spi_flash_descriptor_t* descriptor_pt){return RETURN_OK;}
void dataflash_send_write_enable(spi_flash_descriptor_t* descriptor_pt){}
void dataflash_wait_for_not_busy(spi_flash_descriptor_t* descriptor_pt){}
void dataflash_exit_power_down(spi_flash_descriptor_t* descriptor_pt){}
RET_TYPE dataflash_is_busy(spi_flash_descriptor_t* descriptor_pt){ return RETURN_OK; }
void dataflash_power_down(spi_flash_descriptor_t* descriptor_pt){}
