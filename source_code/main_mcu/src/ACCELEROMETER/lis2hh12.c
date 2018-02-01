/*!  \file     lis2hh12.c
*    \brief    LIS2HH12 accelerometer related functions
*    Created:  01/02/2018
*    Author:   Mathieu Stephan
*/
#include "platform_defines.h"
#include "driver_sercom.h"
#include "lis2hh12.h"
#include "defines.h"


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

/*! \fn     lis2hh12_check_presence(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Check the lis2hh12 presence by reading who am i
*   \param  descriptor_pt   Pointer to lis2hh12 descriptor
*   \return RETURN_OK or RETURN_NOK
*/
RET_TYPE lis2hh12_check_presence(accelerometer_descriptor_t* descriptor_pt)
{
    uint8_t query_command[] = {0x8F, 0x00};
    
    /* Query JEDEC ID */
    lis2hh12_send_command(descriptor_pt, query_command, sizeof(query_command));
    
    /* Check correct lis2hh12 ID */
    if(query_command[1] != 0x41)
    {
        return RETURN_NOK;
    }
    else
    {
        return RETURN_OK;
    }
}