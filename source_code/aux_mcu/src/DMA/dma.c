/**
 * \file    dma.c
 * \author  MBorregoTrujillo
 * \date    08-March-2018
 * \brief   Direct Memory Access driver and util functions
 */
#include "dma.h"

/* Dma Trigger sources */
#define DMA_TRIGSRC_SERCOM1_RX  (0x03)
#define DMA_TRIGSRC_SERCOM1_TX  (0x04)

/**
 * DMA Descriptors
 * Channel 0: UART TX
 * Channel 1: UART RX
 */
enum {
    DMA_UART_TX_CH = 0,
    DMA_UART_RX_CH = 1,
    DMA_NUM_OF_CH
};
DmacDescriptor dma_writeback_descriptors[DMA_NUM_OF_CH] __attribute__ ((aligned (16)));
DmacDescriptor dma_descriptors[DMA_NUM_OF_CH] __attribute__ ((aligned (16)));

/*! \fn     dma_init
 *  \brief  Initialize DMA controller that will be used later
 */
void dma_init(void)
{
    DMAC_CHCTRLB_Type dma_chctrlb_reg;                                                      // Temp register
    DMAC_CTRL_Type dmac_ctrl_reg;                                                           // Temp register

    /* Setup DMA controller */
    DMAC->BASEADDR.reg = (uint32_t)&dma_descriptors[0];                                     // Base descriptor
    DMAC->WRBADDR.reg = (uint32_t)&dma_writeback_descriptors[0];                            // Write back descriptor
    dmac_ctrl_reg.reg = DMAC_CTRL_DMAENABLE;                                                // Enable dma
    dmac_ctrl_reg.bit.LVLEN0 = 1;                                                           // Enable priority level 0
    dmac_ctrl_reg.bit.LVLEN1 = 1;                                                           // Enable priority level 1
    dmac_ctrl_reg.bit.LVLEN2 = 1;                                                           // Enable priority level 2
    dmac_ctrl_reg.bit.LVLEN3 = 1;                                                           // Enable priority level 3
    DMAC->CTRL = dmac_ctrl_reg;                                                             // Write DMA control register
    //DMAC->DBGCTRL.bit.DBGRUN = 1;                                                         // Normal operation during debugging

    /* Setup transfer descriptor for aux comms TX */
    dma_descriptors[DMA_UART_TX_CH].BTCTRL.reg = DMAC_BTCTRL_VALID;                         // Valid descriptor
    dma_descriptors[DMA_UART_TX_CH].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;      // 1 byte address increment
    dma_descriptors[DMA_UART_TX_CH].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;       // Step selection for source
    dma_descriptors[DMA_UART_TX_CH].BTCTRL.bit.SRCINC = 1;                                  // Source Address Increment is enabled.
    dma_descriptors[DMA_UART_TX_CH].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;    // Byte data transfer
    dma_descriptors[DMA_UART_TX_CH].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_NOACT_Val;   // Once data block is transferred, do nothing
    dma_descriptors[DMA_UART_TX_CH].DESCADDR.reg = 0;                                       // No next descriptor address

    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_UART_TX_CH);                                                       // Use channel 5
    dma_chctrlb_reg.reg = 0;                                                                // Clear temp register
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = DMA_TRIGSRC_SERCOM1_TX;                                    // Select TX trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;                                           // Enable channel transfer complete interrupt

    /* Setup transfer descriptor for USART RX */
    dma_descriptors[DMA_UART_RX_CH].BTCTRL.reg = DMAC_BTCTRL_VALID;                         // Valid descriptor
    dma_descriptors[DMA_UART_RX_CH].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;      // 1 byte address increment
    dma_descriptors[DMA_UART_RX_CH].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_DST_Val;       // Step selection for destination
    dma_descriptors[DMA_UART_RX_CH].BTCTRL.bit.DSTINC = 1;                                  // Destination Address Increment is enabled.
    dma_descriptors[DMA_UART_RX_CH].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;    // Byte data transfer
    dma_descriptors[DMA_UART_RX_CH].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_INT_Val;     // Once data block is transferred, generate interrupt
    dma_descriptors[DMA_UART_RX_CH].DESCADDR.reg = 0;                                       // No next descriptor address

    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_UART_RX_CH);                                                       // Use channel 6
    dma_chctrlb_reg.reg = 0;                                                                // Clear temp register
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = DMA_TRIGSRC_SERCOM1_RX;                                    // Select RX trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;                                           // Enable channel transfer complete interrupt
}

/*! \fn     dma_aux_mcu_check_and_clear_dma_transfer_flag
 *  \brief  Check if a DMA transfer from main MCU comms has been done
 *  \note   If the flag is true, flag will be cleared to false
 *  \return true or false
 */
bool dma_aux_mcu_check_and_clear_dma_transfer_flag(void)
{
    /* Test channel DMA_UART_RX_CH: aux MCU RX routine */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_UART_RX_CH);
    if ((DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) != 0)
    {
        /* Set transfer done boolean, clear interrupt */
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
        return true;
    } else{
        return false;
    }
}

/*! \fn     dma_aux_mcu_check_tx_dma_transfer_flag
 *  \brief  Check if a DMA transfer to main MCU has been completed
 *  \note   If the flag is true, flag will be cleared to false
 *  \return true or false
 */
bool dma_aux_mcu_check_tx_dma_transfer_flag(void)
{
    /* Test channel DMA_UART_TX_CH: aux MCU TX routine */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_UART_TX_CH);
    if ((DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) != 0)
    {
        /* Set transfer done boolean, clear interrupt */
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
        return true;
    } else{
        return false;
    }
}

/*! \fn     dma_aux_mcu_init_tx_transfer
 *  \brief  Initialize a DMA transfer to the AUX MCU
 *  \param  datap   Pointer to the data
 *  \param  size    Number of bytes to transfer
 */
void dma_aux_mcu_init_tx_transfer(void* datap, uint16_t size)
{
    cpu_irq_enter_critical();

    /* Setup transfer size */
    dma_descriptors[DMA_UART_TX_CH].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[DMA_UART_TX_CH].DSTADDR.reg = (uint32_t)&SERCOM1->USART.DATA.reg;
    /* Destination address: given value */
    dma_descriptors[DMA_UART_TX_CH].SRCADDR.reg = (uint32_t)datap + size;
    /* Select TX DMA channel */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_UART_TX_CH);
    /* Clear Transmit complete interrupt flag */
    DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
    /* Resume channel operation */
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;

    cpu_irq_leave_critical();
}

/*! \fn     dma_aux_mcu_init_rx_transfer
 *  \brief  Initialize a DMA transfer from the AUX MCU
 *  \param  datap   Pointer to where to store the data
 *  \param  size    Number of bytes to transfer
 */
void dma_aux_mcu_init_rx_transfer(void* datap, uint16_t size)
{
    cpu_irq_enter_critical();

    /* Setup transfer size */
    dma_descriptors[DMA_UART_RX_CH].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[DMA_UART_RX_CH].DSTADDR.reg = (uint32_t)datap + size;
    /* Destination address: given value */
    dma_descriptors[DMA_UART_RX_CH].SRCADDR.reg = (uint32_t)&SERCOM1->USART.DATA.reg;
    /* Resume DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_UART_RX_CH);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;

    cpu_irq_leave_critical();
}

/*! \fn     dma_aux_mcu_disable_transfer(void)
*   \brief  Disable the DMA transfer for the aux MCU comms
*/
void dma_aux_mcu_disable_transfer(void)
{
    cpu_irq_enter_critical();

    /* Stop DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_UART_TX_CH);
    DMAC->CHCTRLA.reg = 0;
    /* Wait for bit clear */
    while(DMAC->CHCTRLA.reg != 0);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
    /* Wait for bit clear */
    while(DMAC->CHCTRLA.reg != 0);

    /* Stop DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_UART_TX_CH);
    DMAC->CHCTRLA.reg = 0;
    /* Wait for bit clear */
    while(DMAC->CHCTRLA.reg != 0);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
    /* Wait for bit clear */
    while(DMAC->CHCTRLA.reg != 0);

    /* Disable Main DMA */
    DMAC->CTRL.reg = 0;
    while(DMAC->CTRL.reg != 0);

    cpu_irq_leave_critical();
}
