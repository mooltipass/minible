/*
 * dma.c
 *
 * Created: 29/05/2017 09:17:51
 *  Author: stephan
 */ 
#include <asf.h>
#include "platform_defines.h"
#include "defines.h"
#include "dma.h"
/* DMA Descriptors for our transfers */
// Channel 0: SPI RX routine for custom fs transfers 
// Channel 1: SPI TX routine for custom fs transfers
// Channel 2: SPI TX routine for transfer to a display
// Channel 3: SPI TX routine for transfer to accelerometer
// Channel 4: SPI RX routine for transfer from accelerometer
DmacDescriptor dma_writeback_descriptors[5] __attribute__ ((aligned (16)));
DmacDescriptor dma_descriptors[5] __attribute__ ((aligned (16)));
/* Boolean to specify if the last DMA transfer for the custom_fs is done */
volatile BOOL dma_custom_fs_transfer_done = FALSE;
/* Boolean to specify if the last DMA transfer for the oled display is done */
volatile BOOL dma_oled_transfer_done = FALSE;
/* Boolean to specify if the last DMA transfer for the accelerometer is done */
volatile BOOL dma_acc_transfer_done = FALSE;


/*! \fn     DMAC_Handler(void)
*   \brief  Function called by interrupt when RX is done
*/
void DMAC_Handler(void)
{
    /* Test channel 0: RX routine */
    DMAC->CHID.reg = DMAC_CHID_ID(0);
    if ((DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) != 0)
    {
        /* Set transfer done boolean, clear interrupt */
        dma_custom_fs_transfer_done = TRUE;
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
    }
    
    /* Test channel 2: oled TX routine */
    DMAC->CHID.reg = DMAC_CHID_ID(2);
    if ((DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) != 0)
    {
        /* Set transfer done boolean, clear interrupt */
        dma_oled_transfer_done = TRUE;
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
    }
    
    /* Test channel 4: acc RX routine */
    DMAC->CHID.reg = DMAC_CHID_ID(4);
    if ((DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) != 0)
    {
        /* Set transfer done boolean, clear interrupt */
        dma_acc_transfer_done = TRUE;
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
    }
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

    /* SPI RX routine, cf user manual 26.5.4 */
    /* Using the SERCOM DMA requests, requires the DMA controller to be configured first. */

    /* Setup transfer descriptor for custom fs RX */
    dma_descriptors[0].BTCTRL.reg = DMAC_BTCTRL_VALID;                                      // Valid descriptor
    dma_descriptors[0].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;                   // 1 byte address increment
    dma_descriptors[0].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_DST_Val;                    // Step selection for destination
    dma_descriptors[0].BTCTRL.bit.DSTINC = 1;                                               // Destination Address Increment is enabled.
    dma_descriptors[0].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;                 // Byte data transfer
    dma_descriptors[0].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_INT_Val;                  // Once data block is transferred, generate interrupt
    dma_descriptors[0].DESCADDR.reg = 0;                                                    // No next descriptor address
    
    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(0);                                                       // Use channel 0
    DMAC_CHCTRLB_Type dma_chctrlb_reg;                                                      // Temp register
    dma_chctrlb_reg.reg = 0;                                                                // Clear it
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = DATAFLASH_DMA_SERCOM_RXTRIG;                              // Select RX trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;                                           // Enable channel transfer complete interrupt

    /* SPI TX routines, cf user manual 26.5.4 */
    /* Using the SERCOM DMA requests, requires the DMA controller to be configured first. */

    /* Setup transfer descriptor for custom fs TX */
    dma_descriptors[1].BTCTRL.reg = DMAC_BTCTRL_VALID;                                      // Valid descriptor
    dma_descriptors[1].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;                   // 1 byte address increment
    dma_descriptors[1].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;                    // Step selection for source
    dma_descriptors[1].BTCTRL.bit.SRCINC = 1;                                               // Destination Address Increment is enabled.
    dma_descriptors[1].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;                 // Byte data transfer
    dma_descriptors[1].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_NOACT_Val;                // Once data block is transferred, do nothing
    dma_descriptors[1].DESCADDR.reg = 0;                                                    // No next descriptor address
    
    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(1);                                                       // Use channel 1
    dma_chctrlb_reg.reg = 0;                                                                // Clear it
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = DATAFLASH_DMA_SERCOM_TXTRIG;                              // Select RX trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register

    /* Setup transfer descriptor for oled TX */
    dma_descriptors[2].BTCTRL.reg = DMAC_BTCTRL_VALID;                                      // Valid descriptor
    dma_descriptors[2].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;                   // 1 byte address increment
    dma_descriptors[2].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;                    // Step selection for source
    dma_descriptors[2].BTCTRL.bit.SRCINC = 1;                                               // Destination Address Increment is enabled.
    dma_descriptors[2].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;                 // Byte data transfer
    dma_descriptors[2].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_INT_Val;                  // Once data block is transferred, generate interrupt
    dma_descriptors[2].DESCADDR.reg = 0;                                                    // No next descriptor address

    /* Setup transfer descriptor for accelerometer TX */
    dma_descriptors[3].BTCTRL.reg = DMAC_BTCTRL_VALID;                                      // Valid descriptor
    dma_descriptors[3].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;                   // 1 byte address increment
    dma_descriptors[3].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;                    // Step selection for source
    dma_descriptors[3].BTCTRL.bit.SRCINC = 0;                                               // Destination Address Increment is disabled.
    dma_descriptors[3].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;                 // Byte data transfer
    dma_descriptors[3].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_NOACT_Val;                // Once data block is transferred, do nothing
    dma_descriptors[3].DESCADDR.reg = 0;                                                    // No next descriptor address
    
    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(3);                                                       // Use channel 3
    dma_chctrlb_reg.reg = 0;                                                                // Clear temp register
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = ACC_DMA_SERCOM_TXTRIG;                                    // Select TX trigger
    dma_chctrlb_reg.bit.EVIE = 1;                                                           // Enable event input action
    dma_chctrlb_reg.bit.EVACT = DMAC_CHCTRLB_EVACT_CBLOCK;                                  // Conditional block transfer
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register

    /* Setup transfer descriptor for accelerometer RX */
    dma_descriptors[4].BTCTRL.reg = DMAC_BTCTRL_VALID;                                      // Valid descriptor
    dma_descriptors[4].BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;                   // 1 byte address increment
    dma_descriptors[4].BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_DST_Val;                    // Step selection for destination
    dma_descriptors[4].BTCTRL.bit.DSTINC = 1;                                               // Destination Address Increment is enabled.
    dma_descriptors[4].BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;                 // Byte data transfer
    dma_descriptors[4].BTCTRL.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_INT_Val;                  // Once data block is transferred, generate interrupt
    dma_descriptors[4].DESCADDR.reg = 0;                                                    // No next descriptor address
    
    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(4);                                                       // Use channel 4
    dma_chctrlb_reg.reg = 0;                                                                // Clear temp register
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = ACC_DMA_SERCOM_RXTRIG;                                    // Select RX trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;                                           // Enable channel transfer complete interrupt

    /* Enable IRQ */
    NVIC_EnableIRQ(DMAC_IRQn);
}

/*! \fn     custom_fs_check_and_clear_dma_transfer_flag(void)
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

/*! \fn     dma_custom_fs_init_transfer(void* spi_data_p, void* datap, uint16_t size)
*   \brief  Initialize a DMA transfer from the flash bus to the array
*   \param  spi_data_p  Pointer to the SPI data register
*   \param  datap       Pointer to where to store the data
*   \param  size        Number of bytes to transfer
*/
void dma_custom_fs_init_transfer(void* spi_data_p, void* datap, uint16_t size)
{
    cpu_irq_enter_critical();
    
    /* SPI RX DMA TRANSFER */
    /* Setup transfer size */
    dma_descriptors[0].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[0].SRCADDR.reg = (uint32_t)spi_data_p;
    /* Destination address: given value */
    dma_descriptors[0].DSTADDR.reg = (uint32_t)datap + size;
    /* Resume DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(0);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;

    /* SPI TX DMA TRANSFER */
    /* Setup transfer size */
    dma_descriptors[1].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[1].DSTADDR.reg = (uint32_t)spi_data_p;
    /* Destination address: given value */
    dma_descriptors[1].SRCADDR.reg = (uint32_t)datap + size;
    /* Resume DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(1);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
    
    cpu_irq_leave_critical();
}

/*! \fn     dma_bootloader_compute_crc32_from_spi(void* spi_data_p, uint32_t size)
*   \brief  Use the DMA controller to compute a CRC32 from a spi transfer
*   \param  spi_data_p  Pointer to the SPI data register
*   \param  size        Number of bytes to transfer
*   \return the crc32
*   \note   Must only be called by the bootloader!
*/
uint32_t dma_bootloader_compute_crc32_from_spi(void* spi_data_p, uint32_t size)
{
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

/*! \fn     dma_oled_init_transfer(void* spi_data_p, void* datap, uint16_t size, uint16_t dma_trigger)
*   \brief  Initialize a DMA transfer from an array to the oled spi bus
*   \param  spi_data_p  Pointer to the SPI data register
*   \param  datap       Pointer to where to store the data
*   \param  size        Number of bytes to transfer
*   \param  dma_trigger DMA trigger ID
*/
void dma_oled_init_transfer(void* spi_data_p, void* datap, uint16_t size, uint16_t dma_trigger)
{
    cpu_irq_enter_critical();
    
    /* Setup DMA channel */
    DMAC->CHID.reg = DMAC_CHID_ID(2);                                                       // Use channel 2
    DMAC_CHCTRLB_Type dma_chctrlb_reg;                                                      // Temp register
    dma_chctrlb_reg.reg = 0;                                                                // Clear it
    dma_chctrlb_reg.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;                            // One trigger required for each beat transfer
    dma_chctrlb_reg.bit.TRIGSRC = dma_trigger;                                              // Select trigger
    DMAC->CHCTRLB = dma_chctrlb_reg;                                                        // Write register
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;                                           // Enable channel transfer complete interrupt
    
    /* SPI TX DMA TRANSFER */
    /* Setup transfer size */
    dma_descriptors[2].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[2].DSTADDR.reg = (uint32_t)spi_data_p;
    /* Destination address: given value */
    dma_descriptors[2].SRCADDR.reg = (uint32_t)datap + size;
    /* Resume DMA channel operation */
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
    
    cpu_irq_leave_critical();
}

/*! \fn     dma_acc_init_transfer(void* spi_data_p, void* datap, uint16_t size)
*   \brief  Initialize a DMA transfer from the accelerometer bus to the array
*   \param  spi_data_p  Pointer to the SPI data register
*   \param  datap       Pointer to where to store the data
*   \param  size        Number of bytes to transfer
*   \param  read_cmd    Pointer to where the read data command is stored
*/
void dma_acc_init_transfer(void* spi_data_p, void* datap, uint16_t size, uint8_t* read_cmd)
{    
    cpu_irq_enter_critical();
    
    /* SPI RX DMA TRANSFER */
    /* Setup transfer size */
    dma_descriptors[4].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[4].SRCADDR.reg = (uint32_t)spi_data_p;
    /* Destination address: given value */
    dma_descriptors[4].DSTADDR.reg = (uint32_t)datap + size;
    /* Resume DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(4);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;

    /* SPI TX DMA TRANSFER */
    /* Setup transfer size */
    dma_descriptors[3].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[3].DSTADDR.reg = (uint32_t)spi_data_p;
    /* Destination address: given value */
    dma_descriptors[3].SRCADDR.reg = (uint32_t)read_cmd;
    /* Resume DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(3);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
    
    cpu_irq_leave_critical();
}