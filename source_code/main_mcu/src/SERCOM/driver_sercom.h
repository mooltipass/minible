/*!  \file     driver_sercom.h
*    \brief    Low level driver for SERCOM
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/
#ifndef DRIVER_SERCOM_H_
#define DRIVER_SERCOM_H_

#include "defines.h"
#include "asf.h"

/* Enums */
typedef enum {SPI_HSS_DISABLE = 0, SPI_HSS_ENABLE = 1} spi_hss_te;
typedef enum {SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3} spi_mode_te;
typedef enum {MISO_PAD0 = 0, MISO_PAD1 = 1, MISO_PAD2 = 2, MISO_PAD3 = 3} spi_miso_pad_te;
typedef enum {MOSI_P0_SCK_P1_SS_P2 = 0, MOSI_P2_SCK_P3_SS_P1 = 1, MOSI_P3_SCK_P1_SS_P2 = 2, MOSI_P0_SCK_P3_SS_P1 = 3} spi_mosi_sck_ss_pad_te;

/* Prototypes */
void sercom_spi_init(Sercom* sercom_pt, uint32_t sercom_baud_div, spi_mode_te mode, spi_hss_te hss, spi_miso_pad_te miso_pad, spi_mosi_sck_ss_pad_te mosi_sck_ss_pad, BOOL receiver_enabled);
void sercom_spi_send_single_byte_without_receive_wait(Sercom* sercom_pt, uint8_t data);
uint8_t sercom_spi_send_single_byte(Sercom* sercom_pt, uint8_t data);
void sercom_spi_wait_for_transmit_complete(Sercom* sercom_pt);

#endif /* DRIVER_SERCOM_H_ */