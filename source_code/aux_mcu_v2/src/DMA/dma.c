/*!  \file     dma.c
*    \brief    DMA transfers
*    Created:  03/03/2018
*    Author:   Mathieu Stephan
*/
#include <string.h>
#ifndef BOOTLOADER
    #include <asf.h>
#else
    #include "sam.h"
#endif
#include "platform_defines.h"
#include "comms_main_mcu.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "defines.h"
#include "logic.h"
#include "dma.h"
/* DMA Descriptors for our transfers and their DMA priority levels (highest number is higher priority, contrary to what is written in some datasheets) */
/* Beware of errata 15683 if you do want to implement linked descriptors! */
// MAIN MCU USART RX routine for transfer from aux MCU: level 3
// USART TX routine for transfer to aux MCU: level 1
DmacDescriptor dma_writeback_descriptors[7] __attribute__ ((aligned (16)));
DmacDescriptor dma_descriptors[7] __attribute__ ((aligned (16)));
/* Boolean to specify if we received a packet from aux MCU */
volatile BOOL dma_aux_mcu_packet_received = FALSE;
/* Boolean to specify if we sent a packet to main MCU */
volatile BOOL dma_main_mcu_packet_sent = TRUE;
/* Message we're currently receiving through DMA */
volatile aux_mcu_message_t dma_main_mcu_temp_rcv_message;
volatile aux_mcu_message_t dma_main_mcu_usb_rcv_message;
volatile aux_mcu_message_t dma_main_mcu_ble_rcv_message;
volatile aux_mcu_message_t dma_main_mcu_other_message;
/* Message received flags */
volatile BOOL dma_main_mcu_usb_msg_received = FALSE;
volatile BOOL dma_main_mcu_ble_msg_received = FALSE;
volatile BOOL dma_main_mcu_other_msg_received = FALSE;
/* MCU systick value when the main MCU message sent interrupt happened */
volatile uint32_t dma_main_mcu_message_sent_mcu_systick_val;
volatile uint32_t dma_main_mcu_message_sent_systick_val;

/*! \fn     DMAC_Handler(void)
*   \brief  Function called by interrupt when RX is done
*/
void DMAC_Handler(void)
{    
    /* MAIN MCU RX routine */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_RX_COMMS);
    if ((DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) != 0)
    {
        /* Set transfer done boolean, clear interrupt */
        dma_aux_mcu_packet_received = TRUE;
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
        
        /* Arm next transfer: leave this here! */
        dma_main_mcu_init_rx_transfer();        
        
        #ifndef BOOTLOADER        
        /* Depending on message received, copy to the right rcv buffer and set flag */
        if (dma_main_mcu_temp_rcv_message.message_type == AUX_MCU_MSG_TYPE_USB)
        {
            memcpy((void*)&dma_main_mcu_usb_rcv_message, (void*)&dma_main_mcu_temp_rcv_message, sizeof(dma_main_mcu_temp_rcv_message));
            /* Check if received message has already been dealt with, do not set received flag if so */
            if (comms_main_mcu_usb_msg_answered_using_first_bytes != FALSE)
            {
                comms_main_mcu_usb_msg_answered_using_first_bytes = FALSE;
            } 
            else
            {
                dma_main_mcu_usb_msg_received = TRUE;
            }
        }
        else if (dma_main_mcu_temp_rcv_message.message_type == AUX_MCU_MSG_TYPE_BLE)
        {
            memcpy((void*)&dma_main_mcu_ble_rcv_message, (void*)&dma_main_mcu_temp_rcv_message, sizeof(dma_main_mcu_temp_rcv_message));
            /* Check if received message has already been dealt with, do not set received flag if so */
            if (comms_main_mcu_ble_msg_answered_using_first_bytes != FALSE)
            {
                comms_main_mcu_ble_msg_answered_using_first_bytes = FALSE;
            }
            else
            {
                dma_main_mcu_ble_msg_received = TRUE;
            }
        }
        else
        {
            memcpy((void*)&dma_main_mcu_other_message, (void*)&dma_main_mcu_temp_rcv_message, sizeof(dma_main_mcu_temp_rcv_message));
            /* Check if received message has already been dealt with, do not set received flag if so */
            if (comms_main_mcu_other_msg_answered_using_first_bytes != FALSE)
            {
                comms_main_mcu_other_msg_answered_using_first_bytes = FALSE;
            }
            else
            {
                dma_main_mcu_other_msg_received = TRUE;
            }    
        }
        #else
            /* Bootloader: we're only receiving other messages :D */
            dma_main_mcu_other_msg_received = TRUE;
        #endif
    }
    
    /* MAIN MCU TX routine */
    DMAC->CHID.reg = DMAC_CHID_ID(DMA_DESCID_TX_COMMS);
    if ((DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) != 0)
    {
        /* Set transfer done boolean, clear interrupt */
        dma_main_mcu_packet_sent = TRUE;
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL;
        
        /* Store timestamp of message sent */
        #ifndef BOOTLOADER
        timer_get_mcu_systick((uint32_t*)&dma_main_mcu_message_sent_mcu_systick_val);
        dma_main_mcu_message_sent_systick_val = timer_get_systick();
        #endif
    }
}

/*! \fn     dma_wait_for_main_mcu_packet_sent(void)
*   \brief  Wait for aux mcu packet to be sent
*/
void dma_wait_for_main_mcu_packet_sent(void)
{
    while (dma_main_mcu_packet_sent == FALSE);
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
    DMAC_CHCTRLB_Type dma_chctrlb_reg;                                                      // Temp register
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

/*! \fn     dma_main_mcu_get_remaining_bytes_for_rx_transfer(void)
*   \brief  Check how many bytes are remaining to be transfered for main MCU RX message
*   \return The number of remaining bytes
*/
uint16_t dma_main_mcu_get_remaining_bytes_for_rx_transfer(void)
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

/*! \fn     dma_main_mcu_check_and_clear_dma_transfer_flag(void)
*   \brief  Check if a DMA transfer from main MCU comms has been done
*   \note   If the flag is true, flag will be cleared to false
*   \return TRUE or FALSE
*/
BOOL dma_main_mcu_check_and_clear_dma_transfer_flag(void)
{
    /* flag can't be set twice, code is safe */
    if (dma_aux_mcu_packet_received != FALSE)
    {
        dma_aux_mcu_packet_received = FALSE;
        return TRUE;
    }
    return FALSE;
}

/*! \fn     dma_main_mcu_init_tx_transfer(void* spi_data_p, void* datap, uint16_t size)
*   \brief  Initialize a DMA transfer to the AUX MCU
*   \param  spi_data_p  Pointer to the SPI data register
*   \param  datap       Pointer to the data
*   \param  size        Number of bytes to transfer
*/
void dma_main_mcu_init_tx_transfer(void* spi_data_p, void* datap, uint16_t size)
{
    /* Wait for previous transfer to be done */
    BOOL went_through_loop_below = FALSE;
    while (dma_main_mcu_packet_sent == FALSE)
    {
        went_through_loop_below = TRUE;
    }
    
    #ifndef BOOTLOADER
    /* We just waited for previous message to be sent, leave a little time for MCU to raise no_comms */
    /* Maths: 6Mbit/s baud rate -> 3.3us for 20 bits + added interrupt latency */
    if (went_through_loop_below != FALSE)
    {
        DELAYUS(10);
    }
    else
    {
        /* Edge case: we didn't go through the loop, the flag just got cleared but the main MCU hasn't raised the no comms signal yet */
        uint32_t mcu_systick;
        timer_get_mcu_systick(&mcu_systick);
        if (dma_main_mcu_message_sent_mcu_systick_val < mcu_systick)
        {
            /* Main MCU systick is decreasing, we've detected a wrapover */
            if((timer_get_systick() - dma_main_mcu_message_sent_systick_val) < 20)
            {
                /* This is a real wrapover as main MCU wrapover is ~300ms */
                uint32_t t_delay = MCU_SYSTICK_MAX_PERIOD - mcu_systick + dma_main_mcu_message_sent_mcu_systick_val;
                if (t_delay < 480)
                {
                    /* Less than 10us since message sent interrupt and now */
                    DELAYUS(10);
                }
            }
        }
        else
        {
            uint32_t t_delay = dma_main_mcu_message_sent_mcu_systick_val - mcu_systick;
            if ((t_delay < 480) && ((timer_get_systick() - dma_main_mcu_message_sent_systick_val) < 20))
            {
                /* Less than 10us since message sent interrupt and now */
                DELAYUS(10);
            }
        }        
    }
    
    /* Check for no comms */
    if (logic_is_no_comms_unavailable() == FALSE)
    {
        while(platform_io_is_no_comms_asserted() == RETURN_OK);
    }
    else
    {
        /* No access to no comms signal, add non negotiable delay */
        DELAYMS(1);
    }
    #else
    (void)went_through_loop_below;
    #endif
    
    /* Disable IRQs */
    __disable_irq();
    __DMB();
    
    /* Set bool */
    dma_main_mcu_packet_sent = FALSE;
    
    /* Setup transfer size */
    dma_descriptors[DMA_DESCID_TX_COMMS].BTCNT.bit.BTCNT = (uint16_t)size;
    /* Source address: DATA register from SPI */
    dma_descriptors[DMA_DESCID_TX_COMMS].DSTADDR.reg = (uint32_t)spi_data_p;
    /* Destination address: given value */
    dma_descriptors[DMA_DESCID_TX_COMMS].SRCADDR.reg = (uint32_t)datap + size;
    
    /* Resume DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_DESCID_TX_COMMS);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
    
    /* Re-enable IRQs */
    __DMB();
    __enable_irq();
}

/*! \fn     dma_main_mcu_disable_transfer(void)
*   \brief  Disable the DMA transfer for the main MCU comms
*/
void dma_main_mcu_disable_transfer(void)
{
    /* Disable IRQs */
    __disable_irq();
    __DMB();
    
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
    
    /* Re-enable IRQs */
    __DMB();
    __enable_irq();
}

/*! \fn     dma_main_mcu_init_rx_transfer(void)
*   \brief  Initialize a DMA transfer from the main MCU
*   \note   We are not disabling IRQs as this is called from an IRQ
*/
void dma_main_mcu_init_rx_transfer(void)
{
    /* Setup transfer size */
    dma_descriptors[DMA_DESCID_RX_COMMS].BTCNT.bit.BTCNT = (uint16_t)sizeof(dma_main_mcu_temp_rcv_message);
    /* Source address: DATA register from SPI */
    dma_descriptors[DMA_DESCID_RX_COMMS].DSTADDR.reg = (uint32_t)(&dma_main_mcu_temp_rcv_message) + sizeof(dma_main_mcu_temp_rcv_message);
    /* Destination address: given value */
    dma_descriptors[DMA_DESCID_RX_COMMS].SRCADDR.reg = (uint32_t)((void*)&AUXMCU_SERCOM->USART.DATA.reg);
    
    /* Resume DMA channel operation */
    DMAC->CHID.reg= DMAC_CHID_ID(DMA_DESCID_RX_COMMS);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
}
