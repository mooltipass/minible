/*!  \file     driver_sercom.h
 *   \brief    Low level driver for SERCOM
 *   Created:  01/03/2018
 *   Author:   MBorregoTrujillo
 */
#ifndef DRIVER_SERCOM_H_
#define DRIVER_SERCOM_H_

#include <asf.h>

/** USART RX pad allocation */
typedef enum {
    USART_RX_PAD0 = 0,
    USART_RX_PAD1 = 1,
    USART_RX_PAD2 = 2,
    USART_RX_PAD3 = 3
} usart_rx_pad_te;

/** USART TX,XCK or TX,RTS,CTS pad allocation */
typedef enum {
    USART_TX_P0_XCK_P1 = 0,
    USART_TX_P2_XCK_P3 = 1,
    USART_TX_P0_RTS_P2_CTS_P3 = 2
} usart_tx_xck_rts_cts_pad_te;

/* Prototypes */
void sercom_usart_deinit(Sercom* sercom_pt);
void sercom_usart_init(Sercom* sercom_pt,
                       uint32_t sercom_baud_div,
                       usart_rx_pad_te rx_pad,
                       usart_tx_xck_rts_cts_pad_te usart_tx_xck_rts_cts_pad);
void sercom_send_single_byte_without_receive_wait(Sercom* sercom_pt, uint8_t data);
uint8_t sercom_send_single_byte(Sercom* sercom_pt, uint8_t data);
void sercom_wait_for_transmit_complete(Sercom* sercom_pt);

#endif /* DRIVER_SERCOM_H_ */
