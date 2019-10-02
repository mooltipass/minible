#include "dma.h"

void dma_oled_init_transfer(Sercom* sercom, void* datap, uint16_t size, uint16_t dma_trigger){}
void dma_acc_init_transfer(Sercom* sercom, void* datap, uint16_t size, uint8_t* read_cmd){}
uint32_t dma_compute_crc32_from_spi(Sercom* sercom, uint32_t size){return 0;}
void dma_aux_mcu_init_tx_transfer(Sercom* sercom, void* datap, uint16_t size){}
void dma_aux_mcu_init_rx_transfer(Sercom* sercom, void* datap, uint16_t size){}
void dma_custom_fs_init_transfer(Sercom* sercom, void* datap, uint16_t size){}
void dma_aux_mcu_wait_for_current_packet_reception_and_clear_flag(void){}
uint16_t dma_aux_mcu_get_remaining_bytes_for_rx_transfer(void){ return 0;}
BOOL dma_custom_fs_check_and_clear_dma_transfer_flag(void){return TRUE;}
BOOL dma_aux_mcu_check_and_clear_dma_transfer_flag(void){return TRUE;}
BOOL dma_oled_check_and_clear_dma_transfer_flag(void){return TRUE;}
BOOL dma_acc_check_and_clear_dma_transfer_flag(void){return TRUE;}
BOOL dma_aux_mcu_check_dma_transfer_flag(void){return TRUE;}
void dma_wait_for_aux_mcu_packet_sent(void){}
void dma_aux_mcu_disable_transfer(void){}
void dma_set_custom_fs_flag_done(void){}
void dma_acc_disable_transfer(void){}
void dma_reset(void){}
void dma_init(void){}
