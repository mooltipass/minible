/*!  \file     driver_sercom.c
 *   \brief    Low level driver for SERCOM
 *   Created:  01/03/2018
 *   Author:   MBorregoTrujillo
 */
#include "driver_sercom.h"

/*! \fn     sercom_usart_deinit
 *  \brief  De initialize and Reset a SERCOM in USART mode
 */
void sercom_usart_deinit(Sercom* sercom_pt){
    /* Disable SERCOM */
    sercom_pt->USART.CTRLA.reg = 0;
    while (sercom_pt->USART.SYNCBUSY.reg);
    /* Reset SERCOM */
    sercom_pt->USART.CTRLA.reg = SERCOM_USART_CTRLA_SWRST;
    while (sercom_pt->USART.SYNCBUSY.reg);
}

/*! \fn     sercom_usart_init
 *  \brief  Initialize a SERCOM in USART mode
 *  \param  sercom_pt       Pointer to a sercom module
 *  \param  sercom_baud_div Value for baud generation
 *  \param  rx_pad          RX pad (see enum)
 *  \param  usart_tx_xck_rts_cts_pad TX/XCK/RTS/CTS pad (see enum)
 */
void sercom_usart_init(Sercom* sercom_pt,
                       uint32_t sercom_baud_div,
                       usart_rx_pad_te rx_pad,
                       usart_tx_xck_rts_cts_pad_te usart_tx_xck_rts_cts_pad){

    SERCOM_USART_CTRLA_Type usart_ctrla_reg;
    usart_ctrla_reg.reg = SERCOM_USART_CTRLA_ENABLE;
    SERCOM_USART_CTRLB_Type usart_ctrlb_reg;
    /* Register configuration */
    usart_ctrla_reg.bit.MODE = 1;                                               // step1. internal clock (1)
    usart_ctrla_reg.bit.CMODE = 0;                                              // step2. either asynchronous (0)
    usart_ctrla_reg.bit.RXPO = rx_pad;                                          // step3. pin for receive data
    usart_ctrla_reg.bit.TXPO = usart_tx_xck_rts_cts_pad;                        // step4. pads for the transmitter

    usart_ctrlb_reg.bit.CHSIZE = 0;                                             // step5. 8 bits transmission

    usart_ctrla_reg.bit.DORD = 0;                                               // step6. MSB first
    usart_ctrla_reg.bit.FORM = 0;                                               // step7. No parity
    usart_ctrla_reg.bit.SAMPR = 3;                                              // 8x over-sampling using fractional baud rate generation.

    usart_ctrlb_reg.bit.SBMODE = 0;                                             // step8. 1 Stop bit
    sercom_pt->USART.BAUD.reg = sercom_baud_div;                                // step9. Write baud divider

    usart_ctrlb_reg.bit.RXEN = 1;                                               // step10. Enable RX
    usart_ctrlb_reg.bit.TXEN = 1;                                               // step10. Enable TX

    /* write CTRLB */
    while (sercom_pt->USART.SYNCBUSY.reg);                                      // Wait for sync
    sercom_pt->USART.CTRLB = usart_ctrlb_reg;

    /* perform USART enable after writing to BAUD and CTRLB */
    while (sercom_pt->USART.SYNCBUSY.reg);                                      // Wait for sync
    sercom_pt->USART.CTRLA = usart_ctrla_reg;
    while (sercom_pt->USART.SYNCBUSY.reg);                                      // Wait for sync
}

/*! \fn     sercom_send_single_byte(Sercom* sercom_pt, uint8_t data)
 *  \brief  Send a single byte through a given sercom
 *  \param  sercom_pt       Pointer to a sercom module
 *  \param  data            Byte to send
 *  \return received data
 */
uint8_t sercom_send_single_byte(Sercom* sercom_pt, uint8_t data)
{
    sercom_pt->USART.INTFLAG.reg = SERCOM_USART_INTFLAG_TXC;                    // Clear transmit complete flag
    sercom_pt->USART.DATA.reg = data;                                         // Write data byte to transmit
    while ((sercom_pt->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_TXC) == 0);     // Wait for received data
    return sercom_pt->USART.DATA.reg;
}

/*! \fn     sercom_send_single_byte_without_receive_wait(Sercom* sercom_pt, uint8_t data)
 *  \brief  Send a single byte through a given sercom, but do not wait to receive a new byte
 *  \param  sercom_pt       Pointer to a sercom module
 *  \param  data            Byte to send
 */
void sercom_send_single_byte_without_receive_wait(Sercom* sercom_pt, uint8_t data)
{
    while ((sercom_pt->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_DRE) == 0);     // Wait for empty buffer
    sercom_pt->USART.INTFLAG.reg = SERCOM_USART_INTFLAG_TXC;                    // Clear transmit complete flag
    sercom_pt->USART.DATA.reg = data;                                         // Write data byte to transmit
}

/*! \fn     sercom_wait_for_transmit_complete(Sercom* sercom_pt)
 *  \brief  Wait for all data to be flushed out
 *  \param  sercom_pt       Pointer to a sercom module
 */
void sercom_wait_for_transmit_complete(Sercom* sercom_pt)
{
    while ((sercom_pt->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_TXC) == 0);
}
