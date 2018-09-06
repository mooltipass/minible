/*!  \file     platform_io.c
*    \brief    Platform IO related functions
*    Created:  22/08/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "platform_defines.h"
//#include "driver_sercom.h"
#include "driver_clocks.h"
#include "driver_timer.h"
#include "platform_io.h"


/*! \fn     EIC_Handler(void)
*   \brief  Routine called for an extint
*/
void EIC_Handler2(void)
{
    /* No comms interrupt: ACK and disable interrupt */
    if ((EIC->INTFLAG.reg & (1 << NOCOMMS_EXTINT_NUM)) != 0)
    {
        EIC->INTFLAG.reg = (1 << NOCOMMS_EXTINT_NUM);
        EIC->INTENCLR.reg = (1 << NOCOMMS_EXTINT_NUM);
    }
}

/*! \fn     platform_io_init_aux_comms_ports(void)
*   \brief  Initialize the ports used for communication with aux MCU
*/
void platform_io_init_aux_comms(void)
{
    /* Port init */
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].DIRCLR.reg = AUX_MCU_NOCOMMS_MASK;                                   // No comms as input
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.INEN = 1;                          // No comms as input
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.PMUXEN = 0;                        // No comms as input
    PORT->Group[AUX_MCU_RX_GROUP].PINCFG[AUX_MCU_RX_PINID].bit.PMUXEN = 1;                                  // Enable peripheral multiplexer
    PORT->Group[AUX_MCU_RX_GROUP].PMUX[AUX_MCU_RX_PINID/2].bit.AUX_MCU_RX_PMUXREGID = AUX_MCU_RX_PMUX_ID;   // AUX MCU RX, MAIN MCU TX
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PMUXEN = 1;                                  // Enable peripheral multiplexer
    PORT->Group[AUX_MCU_TX_GROUP].PMUX[AUX_MCU_TX_PINID/2].bit.AUX_MCU_TX_PMUXREGID = AUX_MCU_TX_PMUX_ID;   // AUX MCU TX, MAIN MCU RX
    PM->APBCMASK.bit.AUXMCU_APB_SERCOM_BIT = 1;                                                             // Enable SERCOM APB Clock Enable
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, AUXMCU_GCLK_SERCOM_ID);                                // Map 48MHz to SERCOM unit    
    
    /* Sercom init */
    /* MSB first, USART frame, async, 8x oversampling, internal clock */
    SERCOM_USART_CTRLA_Type temp_ctrla_reg;
    temp_ctrla_reg.reg = 0;
    temp_ctrla_reg.bit.SAMPR = 2;
    temp_ctrla_reg.bit.RXPO = AUXMCU_RX_TXPO;
    temp_ctrla_reg.bit.TXPO = AUXMCU_TX_PAD;
    temp_ctrla_reg.bit.MODE = SERCOM_USART_CTRLA_MODE_USART_INT_CLK_Val;
    AUXMCU_SERCOM->USART.CTRLA = temp_ctrla_reg;
    /* TX & RX en, 8bits */
    SERCOM_USART_CTRLB_Type temp_ctrlb_reg;
    temp_ctrlb_reg.reg = 0;
    temp_ctrlb_reg.bit.RXEN = 1;
    temp_ctrlb_reg.bit.TXEN = 1;
    while ((AUXMCU_SERCOM->USART.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB) != 0);
    AUXMCU_SERCOM->USART.CTRLB = temp_ctrlb_reg;
    /* Set max baud rate */
    AUXMCU_SERCOM->USART.BAUD.reg = 0;
    /* Enable sercom */
    temp_ctrla_reg.reg |= SERCOM_USART_CTRLA_ENABLE;
    while ((AUXMCU_SERCOM->USART.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE) != 0);
    AUXMCU_SERCOM->USART.CTRLA = temp_ctrla_reg;
}

/*! \fn     platform_io_enable_eic(void)
*   \brief  Initialize interrupt controller
*/
void platform_io_enable_eic(void)
{    
    /* When an external interrupt is configured for level detection GCLK_EIC is not required */
    while ((EIC->STATUS.reg & EIC_STATUS_SYNCBUSY) != 0);
    /* Enable External Interrupt Controller */
    EIC->CTRL.reg = EIC_CTRL_ENABLE;
    /* Enable its interrupts */
    NVIC_EnableIRQ(EIC_IRQn);
}

/*! \fn     platform_io_init_no_comms_input(void)
*   \brief  Initialize no comms input port
*/
void platform_io_init_no_comms_input(void)
{
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].DIRCLR.reg = AUX_MCU_NOCOMMS_MASK;          
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].OUTCLR.reg = AUX_MCU_NOCOMMS_MASK;          
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.INEN = 1; 
}

/*! \fn     platform_io_enable_no_comms_int(void)
*   \brief  Enable no comms interrupt
*/
void platform_io_enable_no_comms_int(void)
{    
    /* Datasheet: Using WAKEUPEN[x]=1 with INTENSET=0 is not recommended */
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].PMUX[AUX_MCU_NOCOMMS_PINID/2].bit.AUX_MCU_NOCOMMS_PMUXREGID = PORT_PMUX_PMUXO_A_Val; // Pin mux to EIC
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.PMUXEN = 1;                                        // Enable peripheral multiplexer
    EIC->CONFIG[NOCOMMS_EXTINT_NUM/8].bit.NOCOMMS_EIC_SENSE_REG = EIC_CONFIG_SENSE0_LOW_Val;                                // Detect low state
    EIC->INTENSET.reg = (1 << NOCOMMS_EXTINT_NUM);                                                                          // Enable interrupt from ext pin
    EIC->WAKEUP.reg |= (1 << NOCOMMS_EXTINT_NUM);                                                                           // Enable wakeup from ext pin    
}

/*! \fn     platform_io_disable_no_comms_int(void)
*   \brief  Disable no comms interrupt
*/
void platform_io_disable_no_comms_int(void)
{
    EIC->CONFIG[NOCOMMS_EXTINT_NUM/8].bit.NOCOMMS_EIC_SENSE_REG = EIC_CONFIG_SENSE0_NONE_Val;               // No detection
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.PMUXEN = 0;                        // Disable peripheral multiplexer
    EIC->WAKEUP.reg &= ~(1 << NOCOMMS_EXTINT_NUM);                                                          // Disable wakeup from ext pin 
}

/*! \fn     platform_io_disable_main_comms(void)
*   \brief  Disable ports dedicated to aux comms
*/
void platform_io_disable_main_comms(void)
{
    /* Reduces standby current by 40uA */
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PMUXEN = 0;                                  // AUX MCU TX, MAIN MCU RX: Disable peripheral multiplexer
    PORT->Group[AUX_MCU_RX_GROUP].PINCFG[AUX_MCU_RX_PINID].bit.PMUXEN = 0;                                  // AUX MCU RX, MAIN MCU TX: Disable peripheral multiplexer
    PORT->Group[AUX_MCU_TX_GROUP].OUTSET.reg = AUX_MCU_TX_MASK;                                             // AUX MCU TX, MAIN MCU RX: Pull up
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PULLEN = 1;                                  // AUX MCU TX, MAIN MCU RX: Pull up
}

/*! \fn     platform_io_enable_main_comms(void)
*   \brief  Enable ports dedicated to aux comms
*/
void platform_io_enable_main_comms(void)
{
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PMUXEN = 1;                                  // AUX MCU TX, MAIN MCU RX: Enable peripheral multiplexer
    PORT->Group[AUX_MCU_RX_GROUP].PINCFG[AUX_MCU_RX_PINID].bit.PMUXEN = 1;                                  // AUX MCU RX, MAIN MCU TX: Enable peripheral multiplexer
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PULLEN = 0;                                  // AUX MCU RX, MAIN MCU TX: Pull up disable
}

/*! \fn     platform_io_init_usb_ports(void)
*   \brief  Initialize the platform USB ports
*/
void platform_io_init_usb_ports(void)
{
    PORT->Group[USB_DM_GROUP].PINCFG[USB_DM_PINID].bit.PMUXEN = 1;
    PORT->Group[USB_DM_GROUP].PMUX[USB_DM_PINID/2].bit.USB_DM_PMUXREGID = USB_DM_PMUX_ID;
    PORT->Group[USB_DP_GROUP].PINCFG[USB_DP_PINID].bit.PMUXEN = 1;
    PORT->Group[USB_DP_GROUP].PMUX[USB_DP_PINID/2].bit.USB_DP_PMUXREGID = USB_DP_PMUX_ID;    
}

/*! \fn     platform_io_init_ports(void)
*   \brief  Initialize the platform IO ports
*/
void platform_io_init_ports(void)
{    
    /* Aux comms */
    platform_io_init_aux_comms();
    
    /* USB comms */
    platform_io_init_usb_ports();
}    

/*! \fn     platform_io_prepare_ports_for_sleep(void)
*   \brief  Prepare the platform ports for sleep
*/
void platform_io_prepare_ports_for_sleep(void)
{        
    /* Disable main comms ports */    
    platform_io_disable_main_comms();
}

/*! \fn     platform_io_prepare_ports_for_sleep_exit(void)
*   \brief  Prepare the platform ports for sleep exit
*/
void platform_io_prepare_ports_for_sleep_exit(void)
{        
    /* Enable main comms ports */
    platform_io_enable_main_comms();
}   