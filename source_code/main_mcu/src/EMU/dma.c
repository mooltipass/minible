#include "dma.h"
#include "emulator.h"

void dma_oled_init_transfer(Sercom* sercom, void* datap, uint16_t size, uint16_t dma_trigger){}
void dma_acc_init_transfer(Sercom* sercom, void* datap, uint16_t size, uint8_t* read_cmd){}
uint32_t dma_compute_crc32_from_spi(Sercom* sercom, uint32_t size){return 0;}

void dma_aux_mcu_init_tx_transfer(Sercom* sercom, void* datap, uint16_t size)
{
    emu_send_aux(datap, size);
}

static BOOL dma_aux_mcu_packet_received = FALSE;
static char *aux_rcvbuf;
static int aux_rcv_remain;

void dma_aux_mcu_init_rx_transfer(Sercom* sercom, void* datap, uint16_t size)
{
    aux_rcv_remain = size;
    aux_rcvbuf = datap;
}

void dma_aux_mcu_wait_for_current_packet_reception_and_clear_flag(void){
    while(dma_aux_mcu_get_remaining_bytes_for_rx_transfer()) {}
    dma_aux_mcu_packet_received = FALSE;
}

uint16_t dma_aux_mcu_get_remaining_bytes_for_rx_transfer(void){
    if(aux_rcvbuf) {
        int nb = emu_rcv_aux(aux_rcvbuf, aux_rcv_remain);
        aux_rcvbuf += nb;
        aux_rcv_remain -= nb;

        if(aux_rcv_remain == 0)
            dma_aux_mcu_packet_received = TRUE;
    }
    return aux_rcv_remain;
}

BOOL dma_aux_mcu_check_and_clear_dma_transfer_flag(void)
{
    // this updates the flag
    (void)dma_aux_mcu_get_remaining_bytes_for_rx_transfer();

    if (dma_aux_mcu_packet_received) { 
        dma_aux_mcu_packet_received = FALSE;
        return TRUE;
    }
    return FALSE;
}

BOOL dma_aux_mcu_check_dma_transfer_flag(void)
{
    (void)dma_aux_mcu_get_remaining_bytes_for_rx_transfer();
    return dma_aux_mcu_packet_received;
}

void dma_aux_mcu_disable_transfer(void){
    aux_rcvbuf = NULL;
    aux_rcv_remain = 0;
}

void dma_custom_fs_init_transfer(Sercom* sercom, void* datap, uint16_t size){}
BOOL dma_custom_fs_check_and_clear_dma_transfer_flag(void){return TRUE;}
BOOL dma_oled_check_and_clear_dma_transfer_flag(void){return TRUE;}
BOOL dma_acc_check_and_clear_dma_transfer_flag(void){return TRUE;}
void dma_wait_for_aux_mcu_packet_sent(void){}
void dma_set_custom_fs_flag_done(void){}
void dma_acc_disable_transfer(void){}
void dma_reset(void){}
void dma_init(void){}
