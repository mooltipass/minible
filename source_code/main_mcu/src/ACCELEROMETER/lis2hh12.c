/*!  \file     lis2hh12.c
*    \brief    LIS2HH12 accelerometer related functions
*    Created:  01/02/2018
*    Author:   Mathieu Stephan
*/
#include "platform_defines.h"
#include "driver_sercom.h"
#include "driver_clocks.h"
#include "driver_timer.h"
#include "lis2hh12.h"
#include "defines.h"
#include "dma.h"
#include <string.h>


/*! \fn     lis2hh12_send_command(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length)
*   \brief  Send a command to the lis2hh12
*   \param  descriptor_pt   Pointer to lis2hh12 descriptor
*   \param  data            Pointer to the buffer containing the data
*   \param  length          Length of data to send
*/
void lis2hh12_send_command(accelerometer_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length)
{
    /* SS low */
    PORT->Group[descriptor_pt->cs_pin_group].OUTCLR.reg = descriptor_pt->cs_pin_mask;
    
    for(uint32_t i = 0; i < length; i++)
    {
        *data = sercom_spi_send_single_byte(descriptor_pt->sercom_pt, *data);
        data++;
    }
    
    /* SS high */
    PORT->Group[descriptor_pt->cs_pin_group].OUTSET.reg = descriptor_pt->cs_pin_mask;
}

/*! \fn     lis2hh12_check_presence_and_configure(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Check the lis2hh12 presence by reading who am i
*   \param  descriptor_pt   Pointer to lis2hh12 descriptor
*   \return RETURN_OK or RETURN_NOK
*/
RET_TYPE lis2hh12_check_presence_and_configure(accelerometer_descriptor_t* descriptor_pt)
{    
    /* In case of device reboot, as we check INT1 later, disable INT1 output */
    uint8_t disableINT1[] = {0x22, 0x00};
    lis2hh12_send_command(descriptor_pt, disableINT1, sizeof(disableINT1));
    
    /* Enable Event system clock, used to generate event from external interrupt */
    PM->APBCMASK.bit.EVSYS_ = 1;
    
    /* Map global clock to user event channel clock */
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, GCLK_CLKCTRL_ID_EVSYS_0_Val + descriptor_pt->evgen_channel);    
    
    /* Map user event channel to event DMA channel */
    EVSYS_USER_Type temp_evsys_usert;
    temp_evsys_usert.reg = 0;
    temp_evsys_usert.bit.CHANNEL = descriptor_pt->evgen_channel + 1;
    temp_evsys_usert.bit.USER = descriptor_pt->dma_channel;
    EVSYS->USER = temp_evsys_usert;
    
    /* Configure event channel to use external interrupt as input */
    EVSYS_CHANNEL_Type temp_evsys_channel_reg;
    temp_evsys_channel_reg.reg = 0;
    temp_evsys_channel_reg.bit.PATH = EVSYS_CHANNEL_PATH_RESYNCHRONIZED_Val;      // Use resynchronized path (dma requirement)
    temp_evsys_channel_reg.bit.EDGSEL = EVSYS_CHANNEL_EDGSEL_RISING_EDGE_Val;     // Detect rising edge
    temp_evsys_channel_reg.bit.EVGEN = descriptor_pt->evgen_sel;                  // Select correct EIC output
    temp_evsys_channel_reg.bit.CHANNEL = descriptor_pt->evgen_channel;            // Map to selected channel
    EVSYS->CHANNEL = temp_evsys_channel_reg;                                      // Write register
    
    /* Query Who Am I */
    uint8_t query_command[] = {0x8F, 0x00};
    lis2hh12_send_command(descriptor_pt, query_command, sizeof(query_command));
    
    /* Check correct lis2hh12 ID */
    if(query_command[1] != 0x41)
    {
        return RETURN_NOK;
    }
    
    /* 400Hz output data rate, output registers not updated until MSB and LSB read, all axis enabled */
    uint8_t setDataRateCommand[] = {0x20, 0x5F};
    lis2hh12_send_command(descriptor_pt, setDataRateCommand, sizeof(setDataRateCommand));
    
    /* FIFO in stream mode */
    uint8_t fifoStreamModeCommand[] = {0x2E, 0x40};
    lis2hh12_send_command(descriptor_pt, fifoStreamModeCommand, sizeof(fifoStreamModeCommand));
    
    /* Set fifo overrun signal on INT1, enable fifo */
    uint8_t setDataReadyOnINT1[] = {0x22, 0x84};
    lis2hh12_send_command(descriptor_pt, setDataReadyOnINT1, sizeof(setDataReadyOnINT1));
    
    /* Send command to disable accelerometer I2C block and keep address inc */
    uint8_t disableI2cBlockCommand[] = {0x23, 0x06};
    lis2hh12_send_command(descriptor_pt, disableI2cBlockCommand, sizeof(disableI2cBlockCommand));
    
    /* Enable DMA transfer */
    dma_acc_init_transfer((void*)&descriptor_pt->sercom_pt->SPI.DATA.reg, (void*)descriptor_pt->data_rcv_buffer, sizeof(descriptor_pt->data_rcv_buffer));
    
    /* Check for transfer done flag: shouldn't be set before at least 32 (lis2hh12 fifo depth) / Fsample = 80ms at 400Hz). Max read time is 32*3*2/F(SPI) =  24us */
    timer_delay_ms(1);
    if (dma_acc_check_and_clear_dma_transfer_flag() != FALSE)
    {
        return RETURN_NOK;
    }
    
    /* Give enough time and check for transfer done flag */
    timer_delay_ms(100);
    
    /* Check for interrupt */
    if (dma_acc_check_and_clear_dma_transfer_flag() == FALSE)
    {
        return RETURN_NOK;
    }
    
    return RETURN_OK;
}

/*! \fn     lis2hh12_manual_acc_data_read(accelerometer_descriptor_t* descriptor_pt, acc_data* data_pt)
*   \brief  Manual read of acceleration date
*   \param  descriptor_pt   Pointer to lis2hh12 descriptor
*   \param  data_pt         Pointer to where to store the acceleration data
*/
void lis2hh12_manual_acc_data_read(accelerometer_descriptor_t* descriptor_pt, acc_data* data_pt)
{
    uint8_t readAccData[9] = {0xA8};
    lis2hh12_send_command(descriptor_pt, readAccData, sizeof(readAccData));
    memcpy((void*)data_pt, (void*)(&readAccData[1]), 8);
}