#include "driver_sercom.h"

void sercom_spi_init(Sercom* sercom_pt, uint32_t sercom_baud_div, spi_mode_te mode, spi_hss_te hss, spi_miso_pad_te miso_pad, spi_mosi_sck_ss_pad_te mosi_sck_ss_pad, BOOL receiver_enabled){}
void sercom_spi_send_single_byte_without_receive_wait(Sercom* sercom_pt, uint8_t data){}
uint8_t sercom_spi_send_single_byte(Sercom* sercom_pt, uint8_t data) { return 0; }
void sercom_spi_wait_for_transmit_complete(Sercom* sercom_pt) {}
