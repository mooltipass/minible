/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2019 Stephan Mathieu
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
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

/*! \fn     lis2hh12_reset(accelerometer_descriptor_t* descriptor_pt)
*   \brief  Completely reset the LIS2HH12
*   \param  descriptor_pt   Pointer to lis2hh12 descriptor
*/
void lis2hh12_reset(accelerometer_descriptor_t* descriptor_pt)
{
    uint8_t softResetCommand[] = {0x24, 0x40};
    lis2hh12_send_command(descriptor_pt, softResetCommand, sizeof(softResetCommand));
    
    /* Wait for end of reset */
    uint8_t getResetStatusCommand[] = {0xA4, 0xFF};
    while ((getResetStatusCommand[1] & 0x40) != 0x00)
    {
        lis2hh12_send_command(descriptor_pt, getResetStatusCommand, sizeof(getResetStatusCommand));
        getResetStatusCommand[0] = 0xA4;
    }
}

/*! \fn     lis2hh12_get_temperature(accelerometer_descriptor_t* descriptor_pt)
*   \brief  Get temperature from LIS2HH12, multiplied by 100
*   \param  descriptor_pt   Pointer to lis2hh12 descriptor
*   \return the temperature 
*	\note	Not working yet! see https://community.st.com/message/188719-re-temperature-sensor-on-lis2hh12-accelerometer?commentID=188719#comment-188719
*/
int16_t lis2hh12_get_temperature(accelerometer_descriptor_t* descriptor_pt)
{
	uint8_t getTemperatureCommand[] = {0x8B, 0x00, 0x00};
	lis2hh12_send_command(descriptor_pt, getTemperatureCommand, sizeof(getTemperatureCommand));
	int16_t tempHex = (int16_t)((uint16_t)getTemperatureCommand[1] | ((uint16_t)getTemperatureCommand[2]) << 8);
	return tempHex;
}

/*! \fn     lis2hh12_check_presence_and_configure(accelerometer_descriptor_t* descriptor_pt)
*   \brief  Check the lis2hh12 presence by reading who am i
*   \param  descriptor_pt   Pointer to lis2hh12 descriptor
*   \return RETURN_OK or RETURN_NOK
*/
RET_TYPE lis2hh12_check_presence_and_configure(accelerometer_descriptor_t* descriptor_pt)
{    
    /* Query Who Am I */
    uint8_t query_command[] = {0x8F, 0x00};
    lis2hh12_send_command(descriptor_pt, query_command, sizeof(query_command));
    
    /* Check correct lis2hh12 ID */
    if(query_command[1] != 0x41)
    {
        return RETURN_NOK;
    }
    
    /* If we are debugging, a reprogram may happen at any time. We therefore need to reset the lis2hh12 */
    lis2hh12_reset(descriptor_pt);
    
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
    /* Clear intflag */
    EVSYS->INTFLAG.reg = ((1 << descriptor_pt->evgen_sel) << 8) << (16*((descriptor_pt->evgen_sel)/8));
    
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
    
    /* Store read command in descriptor */
    descriptor_pt->read_cmd = 0xA8;
    
    /* Enable DMA transfer and clear nCS */
    dma_acc_init_transfer(descriptor_pt->sercom_pt, (void*)&(descriptor_pt->fifo_read), sizeof(descriptor_pt->fifo_read.acc_data_array) + sizeof(descriptor_pt->fifo_read.wasted_byte_for_read_cmd), &(descriptor_pt->read_cmd));
    PORT->Group[descriptor_pt->cs_pin_group].OUTCLR.reg = descriptor_pt->cs_pin_mask;
    
    /* Check for transfer done flag: shouldn't be set before at least 32 (lis2hh12 fifo depth) / Fsample = 80ms at 400Hz). Max read time is 32*3*2*8/F(SPI) =  192us */
    timer_delay_ms(1);
    if (dma_acc_check_and_clear_dma_transfer_flag() != FALSE)
    {
        return RETURN_NOK;
    }
    
    /* Arm watchdog */
    timer_start_timer(TIMER_ACC_WATCHDOG, 60000);
    
    /* Arm timer */
    uint16_t temp_timer_id = timer_get_and_start_timer(100);
    
    /* Loop until timer is running */
    while (timer_has_allocated_timer_expired(temp_timer_id, TRUE) == TIMER_RUNNING)
    {
        /* Check for data received */
        if (lis2hh12_check_data_received_flag_and_arm_other_transfer(descriptor_pt, TRUE) != FALSE)
        {
            /* Free timer */
            timer_deallocate_timer(temp_timer_id);
            
            return RETURN_OK;
        }
        
        timer_delay_ms(1);
    }
    
    /* Free timer */
    timer_deallocate_timer(temp_timer_id);
    
    /* Timeout */    
    return RETURN_NOK;
}

/*! \fn     lis2hh12_sleep_exit_and_dma_arm(accelerometer_descriptor_t* descriptor_pt)
*   \brief  Sleep exit and DMA arming, resume operations
*/
void lis2hh12_sleep_exit_and_dma_arm(accelerometer_descriptor_t* descriptor_pt)
{
    /* 400Hz output data rate, output registers not updated until MSB and LSB read, all axis enabled */
    uint8_t setDataRateCommand[] = {0x20, 0x5F};
    lis2hh12_send_command(descriptor_pt, setDataRateCommand, sizeof(setDataRateCommand));
    timer_delay_ms(1);
    
    /* Enable DMA transfer and clear nCS */
    dma_acc_init_transfer(descriptor_pt->sercom_pt, (void*)&(descriptor_pt->fifo_read), sizeof(descriptor_pt->fifo_read.acc_data_array) + sizeof(descriptor_pt->fifo_read.wasted_byte_for_read_cmd), &(descriptor_pt->read_cmd));
    PORT->Group[descriptor_pt->cs_pin_group].OUTCLR.reg = descriptor_pt->cs_pin_mask;    
}

/*! \fn     lis2hh12_dma_arm(accelerometer_descriptor_t* descriptor_pt)
*   \brief  Arm DMA to resume data transfers
*/
void lis2hh12_dma_arm(accelerometer_descriptor_t* descriptor_pt)
{	
	/* Enable DMA transfer and clear nCS */
	dma_acc_init_transfer(descriptor_pt->sercom_pt, (void*)&(descriptor_pt->fifo_read), sizeof(descriptor_pt->fifo_read.acc_data_array) + sizeof(descriptor_pt->fifo_read.wasted_byte_for_read_cmd), &(descriptor_pt->read_cmd));
	PORT->Group[descriptor_pt->cs_pin_group].OUTCLR.reg = descriptor_pt->cs_pin_mask;
}

/*! \fn     lis2hh12_deassert_ncs_and_go_to_sleep(accelerometer_descriptor_t* descriptor_pt)
*   \brief  Stop any ongoing DMA transfer and go to sleep
*/
void lis2hh12_deassert_ncs_and_go_to_sleep(accelerometer_descriptor_t* descriptor_pt)
{
    /* Deasset nCS */
    PORT->Group[descriptor_pt->cs_pin_group].OUTSET.reg = descriptor_pt->cs_pin_mask;
    timer_delay_ms(1);
    
    /* Send power down command */
    uint8_t powerDownCommand[] = {0x20, 0x00};
    lis2hh12_send_command(descriptor_pt, powerDownCommand, sizeof(powerDownCommand));    
}

/*! \fn     lis2hh12_check_data_received_flag_and_arm_other_transfer(accelerometer_descriptor_t* descriptor_pt, BOOL arm_other_transfer)
*   \brief  Check if received accelerometer data, and arm next transfer is specified
*   \param  descriptor_pt       Pointer to lis2hh12 descriptor
*   \param  arm_other_transfer  Set to TRUE to arm new transfer
*   \return TRUE if we received new data
*/
BOOL lis2hh12_check_data_received_flag_and_arm_other_transfer(accelerometer_descriptor_t* descriptor_pt, BOOL arm_other_transfer)
{
    if (arm_other_transfer == FALSE)
    {
        return dma_acc_check_dma_transfer_flag();
    } 
    else
    {
        if (dma_acc_check_and_clear_dma_transfer_flag() != FALSE)
        {
            /* Deassert nCS : done through the DMA interrupt */
            //PORT->Group[descriptor_pt->cs_pin_group].OUTSET.reg = descriptor_pt->cs_pin_mask;
            
            /* Assert nCS */
            PORT->Group[descriptor_pt->cs_pin_group].OUTCLR.reg = descriptor_pt->cs_pin_mask;
            
            /* Arm next DMA transfer */
            dma_acc_init_transfer(descriptor_pt->sercom_pt, (void*)&(descriptor_pt->fifo_read), sizeof(descriptor_pt->fifo_read.acc_data_array) + sizeof(descriptor_pt->fifo_read.wasted_byte_for_read_cmd), &(descriptor_pt->read_cmd));
                
            /* Check if we were not quick enough to deal rearm RX DMA: check event channel interrupt flag, cleared by our DMA RX routine: if the flag is set it means another acc INT happened */
            /* In case we have a false positive (interrupt happening just after we re-arm) this is not a problem as the DMA will simply discard the trigger */
            if ((EVSYS->INTFLAG.reg & ((1 << descriptor_pt->evgen_channel) << 8) << (16*(descriptor_pt->evgen_channel/8))) != 0)
            {
                /* Clear int and overrun flags */
                EVSYS->INTFLAG.reg = ((1 << descriptor_pt->evgen_channel) << 8) << (16*((descriptor_pt->evgen_channel)/8));
                EVSYS->INTFLAG.reg = ((1 << descriptor_pt->evgen_channel) << 0) << (16*((descriptor_pt->evgen_channel)/8));
                    
                /* Artificially resend event */
                EVSYS_CHANNEL_Type temp_evsys_channel_reg;
                temp_evsys_channel_reg.reg = 0;
                temp_evsys_channel_reg.bit.PATH = EVSYS_CHANNEL_PATH_RESYNCHRONIZED_Val;      // Use resynchronized path (dma requirement)
                temp_evsys_channel_reg.bit.EDGSEL = EVSYS_CHANNEL_EDGSEL_RISING_EDGE_Val;     // Detect rising edge
                temp_evsys_channel_reg.bit.EVGEN = 0;                                         // No selected event generator
                temp_evsys_channel_reg.bit.CHANNEL = descriptor_pt->evgen_channel;            // Map to selected channel
                EVSYS->CHANNEL = temp_evsys_channel_reg;                                      // Write register
                temp_evsys_channel_reg.bit.EVGEN = descriptor_pt->evgen_sel;                  // Select correct EIC output
                EVSYS->CHANNEL = temp_evsys_channel_reg;                                      // Write register
            }
            
            /* Report there's data to be read */
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

/*! \fn     lis2hh12_manual_acc_data_read(accelerometer_descriptor_t* descriptor_pt, acc_data* data_pt)
*   \brief  Manual read of acceleration date
*   \param  descriptor_pt   Pointer to lis2hh12 descriptor
*   \param  data_pt         Pointer to where to store the acceleration data
*/
void lis2hh12_manual_acc_data_read(accelerometer_descriptor_t* descriptor_pt, acc_data_t* data_pt)
{
    uint8_t readAccData[9] = {0xA8};
    lis2hh12_send_command(descriptor_pt, readAccData, sizeof(readAccData));
    memcpy((void*)data_pt, (void*)(&readAccData[1]), 8);
}