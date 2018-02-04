/*!  \file     lis2hh12.c
*    \brief    LIS2HH12 accelerometer related functions
*    Created:  01/02/2018
*    Author:   Mathieu Stephan
*/
#include "platform_defines.h"
#include "driver_sercom.h"
#include "driver_timer.h"
#include "lis2hh12.h"
#include "defines.h"
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
    
    /* Query Who Am I */
    uint8_t query_command[] = {0x8F, 0x00};
    lis2hh12_send_command(descriptor_pt, query_command, sizeof(query_command));
    
    /* Check correct lis2hh12 ID */
    if(query_command[1] != 0x41)
    {
        return RETURN_NOK;
    }
    
    /* Check for absence of interrupt as we haven't enabled the ACC yet */
    if (!((PORT->Group[descriptor_pt->int_pin_group].IN.reg & descriptor_pt->int_pin_mask) == 0))
    {
        return RETURN_NOK;
    }
    
    /* 400Hz output data rate, output registers not updated until MSB and LSB read, all axis enabled */
    uint8_t setDataRateCommand[] = {0x20, 0x5F};
    lis2hh12_send_command(descriptor_pt, setDataRateCommand, sizeof(setDataRateCommand));
    
    /* Set data ready signal on INT1 */
    uint8_t setDataReadyOnINT1[] = {0x22, 0x01};
    lis2hh12_send_command(descriptor_pt, setDataReadyOnINT1, sizeof(setDataReadyOnINT1));
    
    /* Send command to disable accelerometer I2C block and keep address inc */
    uint8_t disableI2cBlockCommand[] = {0x23, 0x06};
    lis2hh12_send_command(descriptor_pt, disableI2cBlockCommand, sizeof(disableI2cBlockCommand));
    
    /* Wait for data ready signal to appear */
    timer_delay_ms(10);
    
    /* Check for interrupt */
    if ((PORT->Group[descriptor_pt->int_pin_group].IN.reg & descriptor_pt->int_pin_mask) == 0)
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