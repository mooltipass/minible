/*!  \file     dma.c
*    \brief    DMA transfers
*    Created:  03/03/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "platform_defines.h"
#include "comms_aux_mcu.h"
#include "platform_io.h"
#include "dma.h"
/* DMA Descriptors for our transfers and their DMA priority levels (highest number is higher priority, contrary to what is written in some datasheets) */
/* Beware of errata 15683 if you do want to implement linked descriptors! */
// USART RX routine for transfer from aux MCU: level 3
// USART TX routine for transfer to aux MCU: level 1
// SPI RX routine for custom fs transfers: level 0
// SPI TX routine for custom fs transfers: level 0
// SPI RX routine for transfer from accelerometer: level 2
// SPI TX routine for transfer to accelerometer: level 2
// SPI TX routine for transfer to a display: level 1
DmacDescriptor dma_writeback_descriptors[7] __attribute__ ((aligned (16)));
DmacDescriptor dma_descriptors[7] __attribute__ ((aligned (16)));
/* Boolean to specify if the last DMA transfer for the custom_fs is done */
volatile BOOL dma_custom_fs_transfer_done = FALSE;
/* Boolean to specify if the last DMA transfer for the oled display is done */
volatile BOOL dma_oled_transfer_done = FALSE;
/* Boolean to specify if the last DMA transfer for the accelerometer is done */
volatile BOOL dma_acc_transfer_done = FALSE;
/* Boolean to specify if we received a packet from aux MCU */
volatile BOOL dma_aux_mcu_packet_received = FALSE;
/* Boolean to specify if we sent a packet to aux MCU */
volatile BOOL dma_aux_mcu_packet_sent = TRUE;
/* Boolean to specify if DMA needs to be rearmed to receive an aux MCU packet (use with caution) */
volatile BOOL dma_aux_mcu_rx_transfer_to_be_rearmed = TRUE;


/*! \fn     DMAC_Handler(void)
*   \brief  Function called by interrupt when RX is done
*/
void DMAC_Handler(void)
{    
    /* AUX MCU RX routine */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_RX_COMMS);
    if ((DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) != 0)
    {
        /* Set transfer done boolean, clear interrupt */
        platform_io_set_no_comms();
        dma_aux_mcu_packet_received = TRUE;
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
        dma_aux_mcu_rx_transfer_to_be_rearmed = TRUE;
    }
    
    /* AUX MCU RX routine */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_TX_COMMS);
    if ((DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) != 0)
    {
        /* Set transfer done boolean, clear interrupt */
        dma_aux_mcu_packet_sent = TRUE;
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
    }
    
    /* RX routine for custom fs */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_RX_FS);
    if ((DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) != 0)
    {
        /* Set transfer done boolean, clear interrupt */
        dma_custom_fs_transfer_done = TRUE;
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
    }
    
    /* OLED TX routine */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_TX_OLED);
    if ((DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) != 0)
    {
        /* Set transfer done boolean, clear interrupt */
        dma_oled_transfer_done = TRUE;
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
    }
    
    /* Accelerometer RX routine */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_RX_ACC);
    if ((DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) != 0)
    {        
        /* Set nCS high to allow accelerometer to store data again */
        PORT->Group[ACC_nCS_GROUP].OUTSET.reg = ACC_nCS_MASK;
        
        /* Clear event system event detected flag */
        EVSYS->INTFLAG.reg = ((1 << ACC_EV_GEN_CHANNEL) << 8) << (16*(ACC_EV_GEN_CHANNEL/8));
        
        /* Set transfer done boolean, clear interrupt */
        dma_acc_transfer_done = TRUE;
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
    }
}

/*! \fn     dma_set_custom_fs_flag_done(void)
*   \brief  Manually set the custom fs done flag (used to simulate DMA transfers)
*/
void dma_set_custom_fs_flag_done(void)
{
    dma_custom_fs_transfer_done = TRUE;
}

/*! \fn     dma_wait_for_aux_mcu_packet_sent(void)
*   \brief  Wait for aux mcu packet to be sent
*/
void dma_wait_for_aux_mcu_packet_sent(void)
{
    while (dma_aux_mcu_packet_sent == FALSE);
}

/*! \fn     dma_reset(void)
*   \brief  Reset DMA controller
*/
void dma_reset(void)
{
    DMAC->CTRL.reg = 0;                 // Disable DMA
    while (DMAC->CTRL.reg != 0);        // Wait for DMA disabled
    DMAC->CTRL.reg = DMAC_CTRL_SWRST;   // Reset DMA
    while (DMAC->CTRL.reg != 0);        // Wait for DMA reset
}

/*! \fn     dma_init(void)
*   \brief  Initialize DMA controller that will be used later
*/
void dma_init(void)
{    
    /* Setup DMA controller */
    DMAC_CTRL_Type dmac_ctrl_reg;
    DMAC->BASEADDR.reg = (uint32_t)&dma_descriptors[0];                                     // Base descriptor
    DMAC->WRBADDR.reg = (uint32_t)&dma_writeback_descriptors[0];                            // Write back descriptor
    dmac_ctrl_reg.reg = DMAC_CTRL_DMAENABLE;                                                // Enable dma
    dmac_ctrl_reg.bit.LVLEN0 = 1;                                                           // Enable priority level 0
    dmac_ctrl_reg.bit.LVLEN1 = 1;                                                           // Enable priority level 1
    dmac_ctrl_reg.bit.LVLEN2 = 1;                                                           // Enable priority level 2
    dmac_ctrl_reg.bit.LVLEN3 = 1;                                                           // Enable priority level 3
    DMAC->CTRL = dmac_ctrl_reg;                                                             // Write DMA control register
    //DMAC->DBGCTRL.bit.DBGRUN = 1;                                                         // Normal operation during debugging
    
    /* DMA QOS: elevate to medium priority */
    DMAC_QOSCTRL_Type dma_qos_ctrl_reg;                                                     // Temporary register
    dma_qos_ctrl_reg.reg = 0;                                                               // Clear temp register
    dma_qos_ctrl_reg.bit.DQOS = DMAC_QOSCTRL_WRBQOS_MEDIUM_Val;                             // Medium QOS for writeback
    dma_qos_ctrl_reg.bit.FQOS = DMAC_QOSCTRL_WRBQOS_MEDIUM_Val;                             // Medium QOS for fetch
    dma_qos_ctrl_reg.bit.WRBQOS = DMAC_QOSCTRL_WRBQOS_MEDIUM_Val;                           // Medium QOS for data
    DMAC->QOSCTRL = dma_qos_ctrl_reg;                                                       // Write register
    
    /* Enable round robin for all levels */
    DMAC_PRICTRL0_Type dmac_prictrl_reg;                                                    // Temporary register
    dmac_prictrl_reg.reg = 0;                                                               // Clear temp register
    dmac_prictrl_reg.bit.LVLPRI0 = 1;                                                       // Enable round robin for level 0
    dmac_prictrl_reg.bit.LVLPRI1 = 1;                                                       // Enable round robin for level 1
    dmac_prictrl_reg.bit.LVLPRI2 = 1;                                                       // Enable round robin for level 2
    dmac_prictrl_reg.bit.LVLPRI3 = 1;                                                       // Enable round robin for level 3
    DMAC->PRICTRL0 = dmac_prictrl_reg;                                                      // Write register

    /* Setup transfer descriptor for custom fs RX */
    dma_descriptors[DMA_DESCID_RX_FS].BTCTRL.reg = DMAC_BTCTRL_VALID;                       // Valid descriptor
    dma_descriptors[DMA_DESCID_RX_FS].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;    // 1 byte address increment
    dma_descriptors[DMA_DESCID_RX_FS].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_DST_Val;     // Step selection for destination
    dma_descriptors[DMA_DESCID_RX_FS].BTCTRL.bit.DSTINC = 1;                                // Destination Address Increment is enabled.
    dma_descriptors[DMA_DESCID_RX_FS].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;  // Byte data transfer
    dma_descriptors[DMA_DESCID_RX_FS].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_INT_Val;   // Once data block is transferred, generate interrupt
    dma_descriptors[DMA_DESCID_RX_FS].DESCADDR.reg = 0;                                     // No next descriptor address
    
    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_RX_FS);                                        // Select channel
    DMAC_CHCTRLB_Type dma_chctrlb_reg;                                                      // Temp register
    dma_chctrlb_reg.reg = 0;                                                                // Clear it
    dma_chctrlb_reg.bit.LVL = 0;                                                            // Priority level
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = DATAFLASH_DMA_SERCOM_RXTRIG;                              // Select RX trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;                                           // Enable channel transfer complete interrupt

    /* Setup transfer descriptor for custom fs TX */
    dma_descriptors[DMA_DESCID_TX_FS].BTCTRL.reg = DMAC_BTCTRL_VALID;                       // Valid descriptor
    dma_descriptors[DMA_DESCID_TX_FS].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;    // 1 byte address increment
    dma_descriptors[DMA_DESCID_TX_FS].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;     // Step selection for source
    dma_descriptors[DMA_DESCID_TX_FS].BTCTRL.bit.SRCINC = 1;                                // Destination Address Increment is enabled.
    dma_descriptors[DMA_DESCID_TX_FS].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;  // Byte data transfer
    dma_descriptors[DMA_DESCID_TX_FS].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_NOACT_Val; // Once data block is transferred, do nothing
    dma_descriptors[DMA_DESCID_TX_FS].DESCADDR.reg = 0;                                     // No next descriptor address
    
    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_TX_FS);                                        // Select channel
    dma_chctrlb_reg.reg = 0;                                                                // Clear it
    dma_chctrlb_reg.bit.LVL = 0;                                                            // Priority level
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = DATAFLASH_DMA_SERCOM_TXTRIG;                              // Select RX trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register

    /* Setup transfer descriptor for oled TX */
    dma_descriptors[DMA_DESCID_TX_OLED].BTCTRL.reg = DMAC_BTCTRL_VALID;                     // Valid descriptor
    dma_descriptors[DMA_DESCID_TX_OLED].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;  // 1 byte address increment
    dma_descriptors[DMA_DESCID_TX_OLED].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;   // Step selection for source
    dma_descriptors[DMA_DESCID_TX_OLED].BTCTRL.bit.SRCINC = 1;                              // Destination Address Increment is enabled.
    dma_descriptors[DMA_DESCID_TX_OLED].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;// Byte data transfer
    dma_descriptors[DMA_DESCID_TX_OLED].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_INT_Val; // Once data block is transferred, generate interrupt
    dma_descriptors[DMA_DESCID_TX_OLED].DESCADDR.reg = 0;                                   // No next descriptor address
    
    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_TX_OLED);                                      // Select channel
    dma_chctrlb_reg.reg = 0;                                                                // Clear it
    dma_chctrlb_reg.bit.LVL = 1;                                                            // Priority level
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = OLED_DMA_SERCOM_TX_TRIG;                                  // Select trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;                                           // Enable channel transfer complete interrupt

    /* Setup transfer descriptor for accelerometer TX */
    dma_descriptors[DMA_DESCID_TX_ACC].BTCTRL.reg = DMAC_BTCTRL_VALID;                      // Valid descriptor
    dma_descriptors[DMA_DESCID_TX_ACC].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;   // 1 byte address increment
    dma_descriptors[DMA_DESCID_TX_ACC].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;    // Step selection for source
    dma_descriptors[DMA_DESCID_TX_ACC].BTCTRL.bit.SRCINC = 0;                               // Destination Address Increment is disabled.
    dma_descriptors[DMA_DESCID_TX_ACC].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val; // Byte data transfer
    dma_descriptors[DMA_DESCID_TX_ACC].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_NOACT_Val;// Once data block is transferred, do nothing
    dma_descriptors[DMA_DESCID_TX_ACC].DESCADDR.reg = 0;                                    // No next descriptor address
    
    /* DMA channel for TX done in the dma arm routine, because of silicon bug */

    /* Setup transfer descriptor for accelerometer RX */
    dma_descriptors[DMA_DESCID_RX_ACC].BTCTRL.reg = DMAC_BTCTRL_VALID;                      // Valid descriptor
    dma_descriptors[DMA_DESCID_RX_ACC].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;   // 1 byte address increment
    dma_descriptors[DMA_DESCID_RX_ACC].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_DST_Val;    // Step selection for destination
    dma_descriptors[DMA_DESCID_RX_ACC].BTCTRL.bit.DSTINC = 1;                               // Destination Address Increment is enabled.
    dma_descriptors[DMA_DESCID_RX_ACC].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val; // Byte data transfer
    dma_descriptors[DMA_DESCID_RX_ACC].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_INT_Val;  // Once data block is transferred, generate interrupt
    dma_descriptors[DMA_DESCID_RX_ACC].DESCADDR.reg = 0;                                    // No next descriptor address
    
    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_RX_ACC);                                       // Select channel
    dma_chctrlb_reg.reg = 0;                                                                // Clear temp register
    dma_chctrlb_reg.bit.LVL = 2;                                                            // Priority level
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = ACC_DMA_SERCOM_RXTRIG;                                    // Select RX trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;                                           // Enable channel transfer complete interrupt

    /* Setup transfer descriptor for aux comms TX */
    dma_descriptors[DMA_DESCID_TX_COMMS].BTCTRL.reg = DMAC_BTCTRL_VALID;                      // Valid descriptor
    dma_descriptors[DMA_DESCID_TX_COMMS].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;   // 1 byte address increment
    dma_descriptors[DMA_DESCID_TX_COMMS].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;    // Step selection for source
    dma_descriptors[DMA_DESCID_TX_COMMS].BTCTRL.bit.SRCINC = 1;                               // Source Address Increment is enabled.
    dma_descriptors[DMA_DESCID_TX_COMMS].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val; // Byte data transfer
    dma_descriptors[DMA_DESCID_TX_COMMS].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_INT_Val;  // Once data block is transferred, generate interrupt
    dma_descriptors[DMA_DESCID_TX_COMMS].DESCADDR.reg = 0;                                    // No next descriptor address
    
    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_TX_COMMS);                                     // Select channel
    dma_chctrlb_reg.reg = 0;                                                                // Clear temp register
    dma_chctrlb_reg.bit.LVL = 1;                                                            // Priority level
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = AUX_MCU_SERCOM_TXTRIG;                                    // Select TX trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;                                           // Enable channel transfer complete interrupt

    /* Setup transfer descriptor for aux MCU comms RX */
    dma_descriptors[DMA_DESCID_RX_COMMS].BTCTRL.reg = DMAC_BTCTRL_VALID;                     // Valid descriptor
    dma_descriptors[DMA_DESCID_RX_COMMS].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;  // 1 byte address increment
    dma_descriptors[DMA_DESCID_RX_COMMS].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_DST_Val;   // Step selection for destination
    dma_descriptors[DMA_DESCID_RX_COMMS].BTCTRL.bit.DSTINC = 1;                              // Destination Address Increment is enabled.
    dma_descriptors[DMA_DESCID_RX_COMMS].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;// Byte data transfer
    dma_descriptors[DMA_DESCID_RX_COMMS].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_INT_Val; // Once data block is transferred, generate interrupt
    dma_descriptors[DMA_DESCID_RX_COMMS].DESCADDR.reg = 0;                                   // No next descriptor
    
    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_RX_COMMS);                                     // Select channel
    dma_chctrlb_reg.reg = 0;                                                                // Clear temp register
    dma_chctrlb_reg.bit.LVL = 3;                                                            // Priority level
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = AUX_MCU_SERCOM_RXTRIG;                                    // Select RX trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;                                           // Enable channel transfer complete interrupt

    /* Enable IRQ */
    NVIC_EnableIRQ(DMAC_IRQn);
}

/*! \fn     dma_custom_fs_check_and_clear_dma_transfer_flag(void)
*   \brief  Check if a DMA transfer that we requested for flash file transfer is done
*   \note   If the flag is true, flag will be cleared to false
*   \return TRUE or FALSE
*/
BOOL dma_custom_fs_check_and_clear_dma_transfer_flag(void)
{
    /* flag can't be set twice, code is safe */
    if (dma_custom_fs_transfer_done != FALSE)
    {
        dma_custom_fs_transfer_done = FALSE;
        return TRUE;
    }
    return FALSE;
}

/*! \fn     dma_oled_check_and_clear_dma_transfer_flag(void)
*   \brief  Check if a DMA transfer that we requested for led transfer is done
*   \note   If the flag is true, flag will be cleared to false
*   \return TRUE or FALSE
*/
BOOL dma_oled_check_and_clear_dma_transfer_flag(void)
{
    /* flag can't be set twice, code is safe */
    if (dma_oled_transfer_done != FALSE)
    {
        dma_oled_transfer_done = FALSE;
        return TRUE;
    }
    return FALSE;
}

/*! \fn     dma_acc_check_and_clear_dma_transfer_flag(void)
*   \brief  Check if a DMA transfer that we requested for led transfer is done
*   \note   If the flag is true, flag will be cleared to false
*   \return TRUE or FALSE
*/
BOOL dma_acc_check_and_clear_dma_transfer_flag(void)
{
    /* flag can't be set twice, code is safe */
    if (dma_acc_transfer_done != FALSE)
    {
        dma_acc_transfer_done = FALSE;
        return TRUE;
    }
    return FALSE;
}

/*! \fn     dma_aux_mcu_get_remaining_bytes_for_rx_transfer(void)
*   \brief  Check how many bytes are remaining to be transfered for aux MCU RX message
*   \return The number of remaining bytes
*/
uint16_t dma_aux_mcu_get_remaining_bytes_for_rx_transfer(void)
{
    /* Check for active channel */
    DMAC_ACTIVE_Type active_reg_copy = DMAC->ACTIVE;
    if (active_reg_copy.bit.ID == DMA_DESCID_RX_COMMS && active_reg_copy.bit.ABUSY != 0)
    {
        return active_reg_copy.bit.BTCNT;
    } 
    else
    {
        return dma_writeback_descriptors[DMA_DESCID_RX_COMMS].BTCNT.reg;
    }
}

/*! \fn     dma_aux_mcu_check_and_clear_dma_transfer_flag(void)
*   \brief  Check if a DMA transfer from aux MCU comms has been done
*   \note   If the flag is true, flag will be cleared to false
*   \return TRUE or FALSE
*/
BOOL dma_aux_mcu_check_and_clear_dma_transfer_flag(void)
{
    /* flag can't be set twice, code is safe */
    if (dma_aux_mcu_packet_received != FALSE)
    {
        dma_aux_mcu_packet_received = FALSE;
        return TRUE;
    }
    return FALSE;
}

/*! \fn     dma_aux_mcu_check_dma_transfer_flag(void)
*   \brief  Check if a DMA transfer from aux MCU comms has been done
*   \return TRUE or FALSE
*/
BOOL dma_aux_mcu_check_dma_transfer_flag(void)
{
    return dma_aux_mcu_packet_received;
}

/*! \fn     dma_aux_mcu_wait_for_current_packet_reception_and_clear_flag(void)
*   \brief  Wait for the complete reception of current AUX MCU packet
*/
void dma_aux_mcu_wait_for_current_packet_reception_and_clear_flag(void)
{
    while (dma_aux_mcu_rx_transfer_to_be_rearmed == FALSE);
    dma_aux_mcu_packet_received = FALSE;
}

/*! \fn     dma_custom_fs_init_transfer(Sercom* sercom, void* datap, uint16_t size)
*   \brief  Initialize a DMA transfer from the flash bus to the array
*   \param  sercom      Pointer to a sercom module
*   \param  datap       Pointer to where to store the data
*   \param  size        Number of bytes to transfer
*/
void dma_custom_fs_init_transfer(Sercom* sercom, void* datap, uint16_t size)
{
    volatile void *spi_data_p = &sercom->SPI.DATA.reg;
    cpu_irq_enter_critical();
    
    /* SPI RX DMA TRANSFER */
    /* Setup transfer size */
    dma_descriptors[DMA_DESCID_RX_FS].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[DMA_DESCID_RX_FS].SRCADDR.reg = (uint32_t)spi_data_p;
    /* Destination address: given value */
    dma_descriptors[DMA_DESCID_RX_FS].DSTADDR.reg = (uint32_t)datap + size;
    
    /* Resume DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_DESCID_RX_FS);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;

    /* SPI TX DMA TRANSFER */
    /* Setup transfer size */
    dma_descriptors[DMA_DESCID_TX_FS].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[DMA_DESCID_TX_FS].DSTADDR.reg = (uint32_t)spi_data_p;
    /* Destination address: given value */
    dma_descriptors[DMA_DESCID_TX_FS].SRCADDR.reg = (uint32_t)datap + size;
    
    /* Resume DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_DESCID_TX_FS);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
    
    cpu_irq_leave_critical();
}

/*! \fn     dma_compute_crc32_from_spi(Sercom* sercom, uint32_t size)
*   \brief  Use the DMA controller to compute a CRC32 from a spi transfer
*   \param  sercom      Pointer to a sercom module
*   \param  size        Number of bytes to transfer
*   \return the crc32
*   \note   DMA controller must be disabled and reset before calling this function!
*/
uint32_t dma_compute_crc32_from_spi(Sercom* sercom, uint32_t size)
{
    volatile void *spi_data_p = &sercom->SPI.DATA.reg;
    /* The byte that will be used to read/write spi data */
    volatile uint8_t temp_src_dst_reg = 0;
    
    /* Setup CRC32 */
    DMAC_CRCCTRL_Type crc_ctrl_reg;
    crc_ctrl_reg.reg = 0;
    crc_ctrl_reg.bit.CRCSRC = 0x20;                                                         // DMA channel 0 (SPI RX)
    crc_ctrl_reg.bit.CRCPOLY = DMAC_CRCCTRL_CRCPOLY_CRC32_Val;                              // CRC32
    crc_ctrl_reg.bit.CRCBEATSIZE = DMAC_CRCCTRL_CRCBEATSIZE_BYTE_Val;                       // Beat size is one byte
    DMAC->CRCCTRL = crc_ctrl_reg;                                                           // Store register
    DMAC->CRCCHKSUM.reg = 0xFFFFFFFF;                                                       // Not sure why, it is needed
    
    /* Setup DMA controller */
    DMAC_CTRL_Type dmac_ctrl_reg;
    DMAC->BASEADDR.reg = (uint32_t)&dma_descriptors[0];                                     // Base descriptor
    DMAC->WRBADDR.reg = (uint32_t)&dma_writeback_descriptors[0];                            // Write back descriptor
    dmac_ctrl_reg.reg = DMAC_CTRL_DMAENABLE;                                                // Enable dma
    dmac_ctrl_reg.bit.LVLEN0 = 1;                                                           // Enable priority level 0
    dmac_ctrl_reg.bit.LVLEN1 = 1;                                                           // Enable priority level 1
    dmac_ctrl_reg.bit.LVLEN2 = 1;                                                           // Enable priority level 2
    dmac_ctrl_reg.bit.LVLEN3 = 1;                                                           // Enable priority level 3
    dmac_ctrl_reg.bit.CRCENABLE = 1;                                                        // Enable CRC generator
    DMAC->CTRL = dmac_ctrl_reg;                                                             // Write DMA control register
    //DMAC->DBGCTRL.bit.DBGRUN = 1;                                                         // Normal operation during debugging
        
    /* SPI RX routine, cf user manual 26.5.4 */
    /* Using the SERCOM DMA requests, requires the DMA controller to be configured first. */

    /* Setup transfer descriptor for custom fs RX */
    dma_descriptors[0].BTCTRL.reg = DMAC_BTCTRL_VALID;                                      // Valid descriptor
    dma_descriptors[0].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;                   // 1 byte address increment
    dma_descriptors[0].BTCTRL.bit.DSTINC = 0;                                               // Destination Address Increment is not enabled.
    dma_descriptors[0].BTCTRL.bit.SRCINC = 0;                                               // Source Address Increment is not enabled.
    dma_descriptors[0].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;                 // Byte data transfer
    dma_descriptors[0].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_NOACT_Val;                // Once data block is transferred, do not generate interrupt
    dma_descriptors[0].DESCADDR.reg = 0;                                                    // No next descriptor address
    
    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(0);                                                       // Use channel 0
    DMAC_CHCTRLB_Type dma_chctrlb_reg;                                                      // Temp register
    dma_chctrlb_reg.reg = 0;                                                                // Clear it
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = DATAFLASH_DMA_SERCOM_RXTRIG;                              // Select RX trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register

    /* SPI TX routines, cf user manual 26.5.4 */
    /* Using the SERCOM DMA requests, requires the DMA controller to be configured first. */

    /* Setup transfer descriptor for custom fs TX */
    dma_descriptors[1].BTCTRL.reg = DMAC_BTCTRL_VALID;                                      // Valid descriptor
    dma_descriptors[1].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;                   // 1 byte address increment
    dma_descriptors[1].BTCTRL.bit.DSTINC = 0;                                               // Destination Address Increment is not enabled.
    dma_descriptors[1].BTCTRL.bit.SRCINC = 0;                                               // Source Address Increment is not enabled.
    dma_descriptors[1].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;                 // Byte data transfer
    dma_descriptors[1].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_NOACT_Val;                // Once data block is tranferred, do nothing
    dma_descriptors[1].DESCADDR.reg = 0;                                                    // No next descriptor address
    
    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(1);                                                       // Use channel 1
    dma_chctrlb_reg.reg = 0;                                                                // Clear it
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = DATAFLASH_DMA_SERCOM_TXTRIG;                              // Select RX trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register

    uint32_t nb_bytes_to_transfer = size;
    while (size > 0)
    {
        /* Compute nb bytes to transfer */
        if (size > UINT16_MAX)
        {
            nb_bytes_to_transfer = UINT16_MAX;
        } 
        else
        {
            nb_bytes_to_transfer = size;
        }
        
        /* Arm transfers */
        /* SPI RX DMA TRANSFER */
        /* Setup transfer size */
        dma_descriptors[0].BTCNT.bit.BTCNT = (uint16_t)nb_bytes_to_transfer;
        /* Source address: DATA register from SPI */
        dma_descriptors[0].SRCADDR.reg = (uint32_t)spi_data_p;
        /* Destination address: given value */
        dma_descriptors[0].DSTADDR.reg = (uint32_t)&temp_src_dst_reg;
        /* Resume DMA channel operation */
        DMAC->CHID.reg= DMAC_CHID_ID(0);
        DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;

        /* SPI TX DMA TRANSFER */
        /* Setup transfer size */
        dma_descriptors[1].BTCNT.bit.BTCNT = (uint16_t)nb_bytes_to_transfer;
        /* Source address: DATA register from SPI */
        dma_descriptors[1].DSTADDR.reg = (uint32_t)spi_data_p;
        /* Destination address: given value */
        dma_descriptors[1].SRCADDR.reg = (uint32_t)&temp_src_dst_reg;
        /* Resume DMA channel operation */
        DMAC->CHID.reg= DMAC_CHID_ID(1);
        DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
        
        /* Wait for transfer to finish */
        DMAC->CHID.reg = DMAC_CHID_ID(0);
        while ((DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) == 0);
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
        
        /* Update size */
        size -= nb_bytes_to_transfer;
    }
    
    /* Get crc32 from dma */
    while ((DMAC->CRCSTATUS.reg & DMAC_CRCSTATUS_CRCBUSY) == DMAC_CRCSTATUS_CRCBUSY);    
    return DMAC->CRCCHKSUM.reg;
}

/*! \fn     dma_oled_init_transfer(Sercom* sercom, void* datap, uint16_t size, uint16_t dma_trigger)
*   \brief  Initialize a DMA transfer from an array to the oled spi bus
*   \param  sercom      Pointer to a sercom module
*   \param  datap       Pointer to where to store the data
*   \param  size        Number of bytes to transfer
*   \param  dma_trigger DMA trigger ID
*/
void dma_oled_init_transfer(Sercom* sercom, void* datap, uint16_t size, uint16_t dma_trigger)
{
    volatile void *spi_data_p = &sercom->SPI.DATA.reg;
    cpu_irq_enter_critical();
    
    /* SPI TX DMA TRANSFER */
    /* Setup transfer size */
    dma_descriptors[DMA_DESCID_TX_OLED].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[DMA_DESCID_TX_OLED].DSTADDR.reg = (uint32_t)spi_data_p;
    /* Destination address: given value */
    dma_descriptors[DMA_DESCID_TX_OLED].SRCADDR.reg = (uint32_t)datap + size;
    
    /* Resume DMA channel operation */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_TX_OLED);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
    
    cpu_irq_leave_critical();
}

/*! \fn     dma_acc_disable_transfer(void)
*   \brief  Disable the DMA transfer for the accelerometer
*/
void dma_acc_disable_transfer(void)
{
    cpu_irq_enter_critical();
    
    /* Stop DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_DESCID_RX_ACC);
    DMAC->CHCTRLA.reg = 0;
    
    /* Wait for bit clear */
    while(DMAC->CHCTRLA.reg != 0);
    
    /* Stop DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_DESCID_TX_ACC);
    DMAC->CHCTRLA.reg = 0;
    
    /* Wait for bit clear */
    while(DMAC->CHCTRLA.reg != 0);
    
    /* Reset bool */
    dma_acc_transfer_done = FALSE;
    
    cpu_irq_leave_critical();    
}

/*! \fn     dma_acc_init_transfer(Sercom* sercom, void* datap, uint16_t size, uint8_t* read_cmd)
*   \brief  Initialize a DMA transfer from the accelerometer bus to the array
*   \param  sercom      Pointer to a sercom module
*   \param  datap       Pointer to where to store the data
*   \param  size        Number of bytes to transfer
*   \param  read_cmd    Pointer to where the read data command is stored
*/
void dma_acc_init_transfer(Sercom *sercom, void* datap, uint16_t size, uint8_t* read_cmd)
{    
    volatile void *spi_data_p = &sercom->SPI.DATA.reg;
    /* First, make sure we don't have an ongoing transfer */
    //dma_acc_disable_transfer();
    
    cpu_irq_enter_critical();
    
    /* SPI RX DMA TRANSFER */
    /* Setup transfer size */
    dma_descriptors[DMA_DESCID_RX_ACC].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[DMA_DESCID_RX_ACC].SRCADDR.reg = (uint32_t)spi_data_p;
    /* Destination address: given value */
    dma_descriptors[DMA_DESCID_RX_ACC].DSTADDR.reg = (uint32_t)datap + size;
    
    /* Resume DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_DESCID_RX_ACC);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;

    /* SPI TX DMA TRANSFER */
    /* Setup transfer size */
    dma_descriptors[DMA_DESCID_TX_ACC].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[DMA_DESCID_TX_ACC].DSTADDR.reg = (uint32_t)spi_data_p;
    /* Destination address: given value */
    dma_descriptors[DMA_DESCID_TX_ACC].SRCADDR.reg = (uint32_t)read_cmd;
    
    /* Setup DMA channel again, due to silicon bug */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_TX_ACC);                                       // Select channel
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;                                                 // Reset channel (damn errata!)
    while (DMAC->CHCTRLA.bit.SWRST);                                                        // Wait for reset done
    DMAC_CHCTRLB_Type dma_chctrlb_reg;                                                      // Temp register
    dma_chctrlb_reg.reg = 0;                                                                // Clear temp register
    dma_chctrlb_reg.bit.LVL = 2;                                                            // Priority level
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = ACC_DMA_SERCOM_TXTRIG;                                    // Select TX trigger
    dma_chctrlb_reg.bit.EVIE = 1;                                                           // Enable event input action
    dma_chctrlb_reg.bit.EVACT = DMAC_CHCTRLB_EVACT_CBLOCK;                                  // Conditional block transfer
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register    
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;                                                // Enable channel
    
    cpu_irq_leave_critical();
}

/*! \fn     dma_aux_mcu_init_tx_transfer(Sercom* sercom, void* datap, uint16_t size)
*   \brief  Initialize a DMA transfer to the AUX MCU
*   \param  sercom      Pointer to a sercom module
*   \param  datap       Pointer to the data
*   \param  size        Number of bytes to transfer
*/
void dma_aux_mcu_init_tx_transfer(Sercom* sercom, void* datap, uint16_t size)
{
    volatile void *usart_data_p = &sercom->USART.DATA.reg;
    /* Wait for previous transfer to be done */
    while (dma_aux_mcu_packet_sent == FALSE);
    
    cpu_irq_enter_critical();
    
    /* Set bool */
    dma_aux_mcu_packet_sent = FALSE;
    
    /* Setup transfer size */
    dma_descriptors[DMA_DESCID_TX_COMMS].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[DMA_DESCID_TX_COMMS].DSTADDR.reg = (uint32_t)usart_data_p;
    /* Destination address: given value */
    dma_descriptors[DMA_DESCID_TX_COMMS].SRCADDR.reg = (uint32_t)datap + size;
    
    /* Resume DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_DESCID_TX_COMMS);
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
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_DESCID_TX_COMMS);
    DMAC->CHCTRLA.reg = 0;
    
    /* Wait for bit clear */
    while(DMAC->CHCTRLA.reg != 0);
    
    /* Stop DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_DESCID_RX_COMMS);
    DMAC->CHCTRLA.reg = 0;
    
    /* Wait for bit clear */
    while(DMAC->CHCTRLA.reg != 0);
    
    /* Reset bool */
    dma_aux_mcu_packet_received = FALSE;
    
    cpu_irq_leave_critical();    
}

/*! \fn     void dma_aux_mcu_init_rx_transfer(Sercom* sercom, void* datap, uint16_t size)
*   \brief  Initialize a DMA transfer from the AUX MCU
*   \param  sercom      Pointer to a sercom module
*   \param  datap       Pointer to where to store the data
*   \param  size        Number of bytes for transfer
*/
void dma_aux_mcu_init_rx_transfer(Sercom* sercom, void* datap, uint16_t size)
{
    volatile void *usart_data_p = &sercom->USART.DATA.reg;
    cpu_irq_enter_critical();
    
    /* Setup transfer size */
    dma_descriptors[DMA_DESCID_RX_COMMS].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[DMA_DESCID_RX_COMMS].DSTADDR.reg = (uint32_t)datap + size;
    /* Destination address: given value */
    dma_descriptors[DMA_DESCID_RX_COMMS].SRCADDR.reg = (uint32_t)usart_data_p;
    
    /* Resume DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_DESCID_RX_COMMS);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
    
    /* Set boolean */
    dma_aux_mcu_rx_transfer_to_be_rearmed = FALSE;
    
    cpu_irq_leave_critical();
}
