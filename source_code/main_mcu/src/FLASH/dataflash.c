/*!  \file     dataflash.c
*    \brief    Low level driver for W25Q16 flash
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "driver_sercom.h"
#include "dataflash.h"
#include "defines.h"


/*! \fn     dataflash_write_array_to_memory(spi_flash_descriptor_t* descriptor_pt, uint32_t address, uint8_t* data, uint32_t length)
*   \brief  Function to write an array to the dataflash memory
*   \param  descriptor_pt   Pointer to dataflash descriptor
*   \param  address         Address at which we should write the page
*   \param  data            Pointer to the buffer containing the data of interest
*   \param  length          Length of data to write
*   \note   Flash should be previously erased before calling this function
*/
void dataflash_write_array_to_memory(spi_flash_descriptor_t* descriptor_pt, uint32_t address, uint8_t* data, uint32_t length)
{
    uint32_t nb_bytes_to_write = 0;
    
    /* First run: check if we're aligned and compute number of bytes to write accordingly */
    if ((address & 0x0FF) != 0)
    {
        nb_bytes_to_write = W25Q16_PAGE_SIZE - (address & 0x0FF);
        if (nb_bytes_to_write > length)
        {
            nb_bytes_to_write = length;
        }
    } 
    else
    {
        nb_bytes_to_write = W25Q16_PAGE_SIZE;
        if (nb_bytes_to_write > length)
        {
            nb_bytes_to_write = length;
        }
    }    
    
    /* Start writing data */  
    while (length != 0)
    {
        /* Guaranteed to not go below 0 */
        length -= nb_bytes_to_write;
        
        /* Write enable */
        dataflash_send_write_enable(descriptor_pt);
        
        /* SS low */
        PORT->Group[descriptor_pt->cs_pin_group].OUTCLR.reg = descriptor_pt->cs_pin_mask;
        
        /* Send write command */
        sercom_spi_send_single_byte(descriptor_pt->sercom_pt, 0x02);
        sercom_spi_send_single_byte(descriptor_pt->sercom_pt, (uint8_t)((address >> 16) & 0x0FF));
        sercom_spi_send_single_byte(descriptor_pt->sercom_pt, (uint8_t)((address >> 8) & 0x0FF));
        sercom_spi_send_single_byte(descriptor_pt->sercom_pt, (uint8_t)((address >> 0) & 0x0FF));
        
        /* Send data */
        for (uint32_t i = 0; i < nb_bytes_to_write; i++)
        {
            sercom_spi_send_single_byte(descriptor_pt->sercom_pt, *data++);
        }
        
        /* SS high */
        PORT->Group[descriptor_pt->cs_pin_group].OUTSET.reg = descriptor_pt->cs_pin_mask;
        
        /* Increment address */
        address += nb_bytes_to_write;
        
        /* Compute remaining bytes to write */
        nb_bytes_to_write = W25Q16_PAGE_SIZE;
        if (nb_bytes_to_write > length)
        {
            nb_bytes_to_write = length;
        }
        
        /* Wait for write */
        dataflash_wait_for_not_busy(descriptor_pt);
    }
}

/*! \fn     dataflash_read_data_array(spi_flash_descriptor_t* descriptor_pt, uint32_t address, uint8_t* data, uint32_t length)
*   \brief  Function to read an array from the dataflash memory
*   \param  descriptor_pt   Pointer to dataflash descriptor
*   \param  address         Address at which we should read data
*   \param  data            Pointer to the buffer to store the data to
*   \param  length          Length of data to read
*/
void dataflash_read_data_array(spi_flash_descriptor_t* descriptor_pt, uint32_t address, uint8_t* data, uint32_t length)
{
    /* SS low */
    PORT->Group[descriptor_pt->cs_pin_group].OUTCLR.reg = descriptor_pt->cs_pin_mask;
    
    /* Send write command */
    sercom_spi_send_single_byte(descriptor_pt->sercom_pt, 0x0B);
    sercom_spi_send_single_byte(descriptor_pt->sercom_pt, (uint8_t)((address >> 16) & 0x0FF));
    sercom_spi_send_single_byte(descriptor_pt->sercom_pt, (uint8_t)((address >> 8) & 0x0FF));
    sercom_spi_send_single_byte(descriptor_pt->sercom_pt, (uint8_t)((address >> 0) & 0x0FF));
    sercom_spi_send_single_byte(descriptor_pt->sercom_pt, 0);
    
    /* Send data */
    for (uint32_t i = 0; i < length; i++)
    {
        *data++ = sercom_spi_send_single_byte(descriptor_pt->sercom_pt, 0);
    }
    
    /* SS high */
    PORT->Group[descriptor_pt->cs_pin_group].OUTSET.reg = descriptor_pt->cs_pin_mask;    
}

/*! \fn     dataflash_read_data_array_start(spi_flash_descriptor_t* descriptor_pt, uint32_t address)
*   \brief  Function to start a read process on the flash
*   \param  descriptor_pt   Pointer to dataflash descriptor
*   \param  address         Address at which we should read data
*/
void dataflash_read_data_array_start(spi_flash_descriptor_t* descriptor_pt, uint32_t address)
{
    /* SS low */
    PORT->Group[descriptor_pt->cs_pin_group].OUTCLR.reg = descriptor_pt->cs_pin_mask;
    
    /* Send write command */
    sercom_spi_send_single_byte(descriptor_pt->sercom_pt, 0x0B);
    sercom_spi_send_single_byte(descriptor_pt->sercom_pt, (uint8_t)((address >> 16) & 0x0FF));
    sercom_spi_send_single_byte(descriptor_pt->sercom_pt, (uint8_t)((address >> 8) & 0x0FF));
    sercom_spi_send_single_byte(descriptor_pt->sercom_pt, (uint8_t)((address >> 0) & 0x0FF));
    sercom_spi_send_single_byte(descriptor_pt->sercom_pt, 0);
}

/*! \fn     dataflash_read_bytes_from_opened_transfer(spi_flash_descriptor_t* descriptor_pt, uint32_t length)
*   \brief  Function to read bytes from the spi bus
*   \param  descriptor_pt   Pointer to dataflash descriptor
*   \param  data            Pointer to the buffer to store the data to
*   \param  length          Length of data to read
*/
void dataflash_read_bytes_from_opened_transfer(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length)
{    
    /* Send data */
    for (uint32_t i = 0; i < length; i++)
    {
        *data++ = sercom_spi_send_single_byte(descriptor_pt->sercom_pt, 0);
    }    
}

/*! \fn     dataflash_stop_ongoing_transfer(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Function to stop an ongoing spi transfer
*   \param  descriptor_pt   Pointer to dataflash descriptor
*/
void dataflash_stop_ongoing_transfer(spi_flash_descriptor_t* descriptor_pt)
{
    /* SS high */
    PORT->Group[descriptor_pt->cs_pin_group].OUTSET.reg = descriptor_pt->cs_pin_mask;    
}

/*! \fn     dataflash_send_command(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length)
*   \brief  Send a command to the flash
*   \param  descriptor_pt   Pointer to dataflash descriptor
*   \param  data            Pointer to the buffer containing the data
*   \param  length          Length of data to send
*/
void dataflash_send_command(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length)
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

/*! \fn     dataflash_send_single_byte_command(spi_flash_descriptor_t* descriptor_pt, uin8_t command)
*   \brief  Send a single byte command to the flash
*   \param  descriptor_pt   Pointer to dataflash descriptor
*   \param  command         The command
*/
void dataflash_send_single_byte_command(spi_flash_descriptor_t* descriptor_pt, uint8_t command)
{
    PORT->Group[descriptor_pt->cs_pin_group].OUTCLR.reg = descriptor_pt->cs_pin_mask;
    sercom_spi_send_single_byte(descriptor_pt->sercom_pt, command);
    PORT->Group[descriptor_pt->cs_pin_group].OUTSET.reg = descriptor_pt->cs_pin_mask;
}   

/*! \fn     dataflash_send_write_enable(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Send write enable to flash
*   \param  descriptor_pt   Pointer to dataflash descriptor
*/
void dataflash_send_write_enable(spi_flash_descriptor_t* descriptor_pt)
{
    dataflash_send_single_byte_command(descriptor_pt, 0x06);
} 

/*! \fn     dataflash_read_status_register(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Read flash flag status register
*   \param  descriptor_pt   Pointer to dataflash descriptor
*   \return Status register contents
*/
uint8_t dataflash_read_status_register(spi_flash_descriptor_t* descriptor_pt)
{
    uint8_t read_sr1_cmd[] = {0x05, 0x00};
    dataflash_send_command(descriptor_pt, read_sr1_cmd, sizeof(read_sr1_cmd));
    return read_sr1_cmd[1];
}  

/*! \fn     dataflash_is_busy(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Check to see if flash is busy
*   \param  descriptor_pt   Pointer to dataflash descriptor
*   \return TRUE or FALSE
*/
RET_TYPE dataflash_is_busy(spi_flash_descriptor_t* descriptor_pt)
{
    uint8_t sr1 = dataflash_read_status_register(descriptor_pt);
    
    /* Check busy bit */
    if((sr1 & 0x01) != 0)
    {
        return TRUE;
    }    
    else
    {
        return FALSE;
    }    
}

/*! \fn     dataflash_wait_for_not_busy(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Wait for the flash to be not busy
*   \param  descriptor_pt   Pointer to dataflash descriptor
*/
void dataflash_wait_for_not_busy(spi_flash_descriptor_t* descriptor_pt)
{
    while(dataflash_is_busy(descriptor_pt) == TRUE);
}

/*! \fn     dataflash_erase_64kb_block(spi_flash_descriptor_t* descriptor_pt, uint32_t address)
*   \brief  Erase a 64KB block
*   \param  descriptor_pt   Pointer to dataflash descriptor
*   \param  address         Address of the 64KB block
*   \note   This command takes a while (around 120ms), please call flash_check_busy to know termination
*/
void dataflash_erase_64kb_block(spi_flash_descriptor_t* descriptor_pt, uint32_t address)
{
    uint8_t erase_64kb_cmd[] = {0xD8, (uint8_t)((address >> 16) & 0xFF), (uint8_t)((address >> 8) & 0xFF), (uint8_t)((address >> 0) & 0xFF)};
    dataflash_send_write_enable(descriptor_pt);
    dataflash_send_command(descriptor_pt, erase_64kb_cmd, sizeof(erase_64kb_cmd));
} 

/*! \fn     dataflash_bulk_erase_with_wait(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Erase the complete flash (will take a long while)
*   \param  descriptor_pt   Pointer to dataflash descriptor
*/
void dataflash_bulk_erase_with_wait(spi_flash_descriptor_t* descriptor_pt)
{
    dataflash_send_write_enable(descriptor_pt);
    dataflash_send_single_byte_command(descriptor_pt, 0xC7);
    dataflash_wait_for_not_busy(descriptor_pt);
}

/*! \fn     dataflash_bulk_erase_without_wait(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Erase the complete flash (will take a long while)
*   \param  descriptor_pt   Pointer to dataflash descriptor
*/
void dataflash_bulk_erase_without_wait(spi_flash_descriptor_t* descriptor_pt)
{
    dataflash_send_write_enable(descriptor_pt);
    dataflash_send_single_byte_command(descriptor_pt, 0xC7);
}

/*! \fn     dataflash_check_presence(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Check the dataflash presence by reading the ID register
*   \param  descriptor_pt   Pointer to dataflash descriptor
*   \return RETURN_OK or RETURN_NOK
*/
RET_TYPE dataflash_check_presence(spi_flash_descriptor_t* descriptor_pt)
{
    uint8_t jedec_query_command[] = {0x9F, 0x00, 0x00, 0x00};
    
    /* Query JEDEC ID */
    dataflash_send_command(descriptor_pt, jedec_query_command, sizeof(jedec_query_command));
    
    /* Check correct dataflash ID */
    if((jedec_query_command[1] != 0xEF) || (jedec_query_command[2] != 0x40) || (jedec_query_command[3] != 0x15))
    {
        return RETURN_NOK;
    }
    else
    {
        return RETURN_OK;
    }
}

/*! \fn     dataflash_check_presence(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Enter power down
*   \param  descriptor_pt   Pointer to dataflash descriptor
*/
void dataflash_power_down(spi_flash_descriptor_t* descriptor_pt)
{
    uint8_t enter_power_down[] = {0xB9};
    
    /* Query JEDEC ID */
    dataflash_send_command(descriptor_pt, enter_power_down, sizeof(enter_power_down));    
}
