/*!  \file     driver_sercom.c
*    \brief    Low level driver for SERCOM
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "driver_sercom.h"


/*! \fn     sercom_spi_init(Sercom* sercom_pt, uint32_t sercom_baud_div, spi_mode_te mode, spi_hss_te hss, spi_miso_pad_te miso_pad, spi_mosi_sck_ss_pad_te mosi_sck_ss_pad, BOOL receiver_enabled)
*   \brief  Initialize a SERCOM in SPI mode
*   \param  sercom_pt       Pointer to a sercom module
*   \param  sercom_baud_div value for baud generation: fbaud = sercomclk / (2 * (baud+1))
*   \param  mode            SPI mode (see enum)
*   \param  hss             Hardware SS disable/enable (see enum)
*   \param  miso_pad        MISO pad (see enum)
*   \param  mosi_sck_ss_pad MOSI/SCK/PAD (see enum)
*/
void sercom_spi_init(Sercom* sercom_pt, uint32_t sercom_baud_div, spi_mode_te mode, spi_hss_te hss, spi_miso_pad_te miso_pad, spi_mosi_sck_ss_pad_te mosi_sck_ss_pad, BOOL receiver_enabled)
{
    sercom_pt->SPI.BAUD.reg = sercom_baud_div;                              // Write baud divider
    
    SERCOM_SPI_CTRLB_Type spi_ctrlb_reg;                                    // SPI CTRLB register
    if (receiver_enabled == FALSE)
    {
        spi_ctrlb_reg.reg = 0;                                              // Disable RX
    } 
    else
    {
        spi_ctrlb_reg.reg = SERCOM_SPI_CTRLB_RXEN;                          // Enable RX
    }
    spi_ctrlb_reg.bit.MSSEN = hss;                                          // Hardware SS control
    spi_ctrlb_reg.bit.CHSIZE = 0;                                           // 8 bits character size
    while ((sercom_pt->SPI.SYNCBUSY.reg & SERCOM_SPI_SYNCBUSY_CTRLB) != 0); // Wait for sync
    sercom_pt->SPI.CTRLB = spi_ctrlb_reg;                                   // Write register
    
    SERCOM_SPI_CTRLA_Type spi_ctrla_reg;                                    // SPI CTRLA register
    spi_ctrla_reg.reg = SERCOM_SPI_CTRLA_ENABLE;                            // Enable SPI module
    spi_ctrla_reg.bit.DORD = 0;                                             // MSB transferred first
    if ((mode == SPI_MODE0) || (mode == SPI_MODE1))                         // CPOL depending on SPI mode
    {
        spi_ctrla_reg.bit.CPOL = 0;
    } 
    else
    {
        spi_ctrla_reg.bit.CPOL = 1;
    }
    if ((mode == SPI_MODE0) || (mode == SPI_MODE2))                         // CPHA depending on SPI mode
    {
        spi_ctrla_reg.bit.CPHA = 0;
    } 
    else
    {
        spi_ctrla_reg.bit.CPHA = 1;
    }
    spi_ctrla_reg.bit.FORM = 0;                                             // SPI frame format
    spi_ctrla_reg.bit.DIPO = miso_pad;                                      // Select MISO pad
    spi_ctrla_reg.bit.DOPO = mosi_sck_ss_pad;                               // MOSI SCK SS pads
    spi_ctrla_reg.bit.RUNSTDBY = 0;                                         // Do not run during standby
    spi_ctrla_reg.bit.MODE = SERCOM_SPI_CTRLA_MODE_SPI_MASTER_Val;          // SPI Master
    while ((sercom_pt->SPI.SYNCBUSY.reg & SERCOM_SPI_SYNCBUSY_ENABLE) != 0);// Wait for sync
    sercom_pt->SPI.CTRLA = spi_ctrla_reg;                                   // Write register
}

/*! \fn     sercom_spi_send_single_byte(Sercom* sercom_pt, uint8_t data)
*   \brief  Send a single byte through a given sercom
*   \param  sercom_pt       Pointer to a sercom module
*   \param  data            Byte to send
*   \return received data
*/
uint8_t sercom_spi_send_single_byte(Sercom* sercom_pt, uint8_t data)
{
    sercom_pt->SPI.INTFLAG.reg = SERCOM_SPI_INTFLAG_TXC;                    // Clear transmit complete flag
    sercom_pt->SPI.DATA.reg = data;                                         // Write data byte to transmit
    while ((sercom_pt->SPI.INTFLAG.reg & SERCOM_SPI_INTFLAG_TXC) == 0);     // Wait for received data
    return sercom_pt->SPI.DATA.reg;
}

/*! \fn     sercom_spi_send_single_byte_without_receive_wait(Sercom* sercom_pt, uint8_t data)
*   \brief  Send a single byte through a given sercom, but do not wait to receive a new byte
*   \param  sercom_pt       Pointer to a sercom module
*   \param  data            Byte to send
*/
void sercom_spi_send_single_byte_without_receive_wait(Sercom* sercom_pt, uint8_t data)
{
    while ((sercom_pt->SPI.INTFLAG.reg & SERCOM_SPI_INTFLAG_DRE) == 0);     // Wait for empty buffer
    sercom_pt->SPI.INTFLAG.reg = SERCOM_SPI_INTFLAG_TXC;                    // Clear transmit complete flag
    sercom_pt->SPI.DATA.reg = data;                                         // Write data byte to transmit    
}

/*! \fn     sercom_spi_wait_for_transmit_complete(Sercom* sercom_pt)
*   \brief  Wait for all data to be flushed out
*   \param  sercom_pt       Pointer to a sercom module
*/
void sercom_spi_wait_for_transmit_complete(Sercom* sercom_pt)
{
    while ((sercom_pt->SPI.INTFLAG.reg & SERCOM_SPI_INTFLAG_TXC) == 0);
}