/*!  \file     dbflash.c
*    \brief    Low level driver for AT45DB* flash
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/
#include "platform_defines.h"
#include "driver_sercom.h"
#include "dbflash.h"
#include "defines.h"


/*! \fn     dbflash_memory_boundary_error_callblack(void)
*   \brief  Function called when a memory boundary issue occurs
*/
void dbflash_memory_boundary_error_callblack(void)
{
    // We'll add more debug later if needed
    #ifdef USB_MESSAGES_FOR_CRITICAL_CALLBACKS
        usbPutstr("#MBE");
    #endif
    while(1);
}

/*! \fn     dbflash_send_command(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length)
*   \brief  Send a command to the flash
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \param  data            Pointer to the buffer containing the data
*   \param  length          Length of data to send
*/
void dbflash_send_command(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length)
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

/*! \fn     dbflash_check_presence(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Check the dbflash presence by reading the ID register
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \return RETURN_OK or RETURN_NOK
*/
RET_TYPE dbflash_check_presence(spi_flash_descriptor_t* descriptor_pt)
{
    uint8_t jedec_query_command[] = {DBFLASH_OPCODE_READ_DEV_INFO, 0x00, 0x00, 0x00};
        
    /* Query JEDEC ID */
    dbflash_send_command(descriptor_pt, jedec_query_command, sizeof(jedec_query_command));
    
    /* Check correct dbflash ID */
    if((jedec_query_command[1] != DBFLASH_MANUF_ID) || (jedec_query_command[2] != MAN_FAM_DEN_VAL))
    {
        return RETURN_NOK;
    }
    else
    {
        return RETURN_OK;
    }
}

/*! \fn     dbflash_fill_page_read_write_erase_opcode_from_address(uint16_t pageNumber, uint16_t offset, uint8_t* buffer)
*   \brief  Fill the opcode address field from the page number and offset
*   \param  pageNumber  Page number
*   \param  offset      Offset in the page
*   \param  buffer      Pointer to the buffer to fill
*/
static inline void dbflash_fill_page_read_write_erase_opcode_from_address(uint16_t pageNumber, uint16_t offset, uint8_t* buffer)
{
    #if (READ_OFFSET_SHT_AMT != WRITE_SHT_AMT) || (READ_OFFSET_SHT_AMT != PAGE_ERASE_SHT_AMT)
        #error "read / write / erase bitshifts differ"
    #endif
    uint16_t temp_uint = (pageNumber << (READ_OFFSET_SHT_AMT-8)) | (offset >> 8);
    buffer[0] = (uint8_t)(temp_uint >> 8);
    buffer[1] = (uint8_t)temp_uint;
    buffer[2] = (uint8_t)offset;
}

/*! \fn     dbflash_send_data_with_four_bytes_opcode(spi_flash_descriptor_t* descriptor_pt, uint8_t* opcode, uint8_t* buffer, uint16_t buffer_size)
*   \brief  Send data with a four bytes opcode to flash
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \param  opcode      Pointer to 4 bytes long opcode
*   \param  buffer      Pointer to the buffer of data
*   \param  buffer_size Length of the buffer
*/
void dbflash_send_data_with_four_bytes_opcode(spi_flash_descriptor_t* descriptor_pt, uint8_t* opcode, uint8_t* buffer, uint16_t buffer_size)
{   
    /* SS low */
    PORT->Group[descriptor_pt->cs_pin_group].OUTCLR.reg = descriptor_pt->cs_pin_mask;
    
    /* Send opcode */
    *opcode = sercom_spi_send_single_byte(descriptor_pt->sercom_pt, *opcode);opcode++;
    *opcode = sercom_spi_send_single_byte(descriptor_pt->sercom_pt, *opcode);opcode++;
    *opcode = sercom_spi_send_single_byte(descriptor_pt->sercom_pt, *opcode);opcode++;
    *opcode = sercom_spi_send_single_byte(descriptor_pt->sercom_pt, *opcode);opcode++;
    
    /* Send data */
    for (uint32_t i = 0; i < buffer_size; i++)
    {
        *buffer = sercom_spi_send_single_byte(descriptor_pt->sercom_pt, *buffer);buffer++;
    }
    
    /* SS high */
    PORT->Group[descriptor_pt->cs_pin_group].OUTSET.reg = descriptor_pt->cs_pin_mask;
}

/*! \fn     db_wait_for_not_busy(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Wait for the flash to be not busy
*   \param  descriptor_pt   Pointer to dataflash descriptor
*/
void dbflash_wait_for_not_busy(spi_flash_descriptor_t* descriptor_pt)
{
    /* SS low */
    PORT->Group[descriptor_pt->cs_pin_group].OUTCLR.reg = descriptor_pt->cs_pin_mask;
        
    /* Active wait */
    BOOL tempBool = TRUE;
    while(tempBool == TRUE)
    {
        /* Read status register command */
        sercom_spi_send_single_byte(descriptor_pt->sercom_pt, DBFLASH_OPCODE_READ_STAT_REG);
        
        /*  Check busy flag */
        if(sercom_spi_send_single_byte(descriptor_pt->sercom_pt, 0) & DBFLASH_READY_BITMASK)
        {
            tempBool = FALSE;
        }
    }
    
    /* SS high */
    PORT->Group[descriptor_pt->cs_pin_group].OUTSET.reg = descriptor_pt->cs_pin_mask;
}

/*! \fn     dbflash_sector_zero_erase(spi_flash_descriptor_t* descriptor_pt, uint8_t sectorNumber)
*   \brief  Erases sector 0a if sectorNumber is DBFLASH_SECTOR_ZERO_A_CODE. Deletes sector 0b if sectorNumber is DBFLASH_SECTOR_ZERO_B_CODE.
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \param  sectorNumber    The sector to erase
*   \note   Sets all bits in sector to Logic 1 (High)
*/
void dbflash_sector_zero_erase(spi_flash_descriptor_t* descriptor_pt, uint8_t sectorNumber)
{
    #ifdef MEMORY_BOUNDARY_CHECKS
        // Error check parameter sectorNumber
        if(!(sectorNumber == DBFLASH_SECTOR_ZERO_A_CODE || sectorNumber == DBFLASH_SECTOR_ZERO_B_CODE))
        {
            dbflash_memory_boundary_error_callblack();
        }    
    #endif
    
    uint16_t temp_uint = (uint16_t)sectorNumber << (SECTOR_ERASE_0_SHT_AMT-8);
    uint8_t opcode[4] = {DBFLASH_OPCODE_SECTOR_ERASE, (uint8_t)(temp_uint >> 8), (uint8_t)temp_uint, 0};
    dbflash_send_command(descriptor_pt, opcode, sizeof(opcode));
    
    /* Wait until memory is ready */
    dbflash_wait_for_not_busy(descriptor_pt);
}

/*! \fn     dbflash_sector_erase(spi_flash_descriptor_t* descriptor_pt, uint8_t sectorNumber)
*   \brief  Erases sector sectorNumber (SECTOR_START -> SECTOR_END inclusive valid).
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \param  sectorNumber    The sector to erase
*   \note   Sets all bits in sector to Logic 1 (High)
*/
void dbflash_sector_erase(spi_flash_descriptor_t* descriptor_pt, uint8_t sectorNumber)
{    
    #ifdef MEMORY_BOUNDARY_CHECKS
        // Error check parameter sectorNumber
        if((sectorNumber < SECTOR_START) || (sectorNumber > SECTOR_END)) // Ex: 1M -> SECTOR_START = 1, SECTOR_END = 3  sectorNumber must be 1, 2, or 3
        {
            dbflash_memory_boundary_error_callblack();
        }
    #endif
    
    uint16_t temp_uint = (uint16_t)sectorNumber << (SECTOR_ERASE_N_SHT_AMT-8);
    uint8_t opcode[4] = {DBFLASH_OPCODE_SECTOR_ERASE, (uint8_t)(temp_uint >> 8), (uint8_t)temp_uint, 0};
    dbflash_send_command(descriptor_pt, opcode, sizeof(opcode));
    
    /* Wait until memory is ready */
    dbflash_wait_for_not_busy(descriptor_pt);   
}

/*! \fn     dbflash_chip_erase(spi_flash_descriptor_t* descriptor_pt)
*   \brief  Erase the complete memory (filled with 1s)
*   \param  descriptor_pt   Pointer to dbflash descriptor
*/
void dbflash_chip_erase(spi_flash_descriptor_t* descriptor_pt)
{
    uint8_t opcode[4] = {0xC7, 0x94, 0x80, 0x9A};
    dbflash_send_command(descriptor_pt, opcode, sizeof(opcode));
    
    /* Wait until memory is ready */
    dbflash_wait_for_not_busy(descriptor_pt);   
}

/*! \fn     dbflash_block_erase(spi_flash_descriptor_t* descriptor_pt, uint16_t blockNumber)
*   \brief  Erases block blockNumber (0 up to BLOCK_COUNT valid).
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \param  blockNumber     The block to erase
*   \note   Sets all bits in block to Logic 1 (High)
*/
void dbflash_block_erase(spi_flash_descriptor_t* descriptor_pt, uint16_t blockNumber)
{
    #ifdef MEMORY_BOUNDARY_CHECKS
        // Error check parameter blockNumber
        if(blockNumber >= BLOCK_COUNT)// Ex: 1M -> BLOCK_COUNT = 64.. valid pageNumber 0-63
        {
            dbflash_memory_boundary_error_callblack();
        }
    #endif
    
    uint16_t temp_uint = blockNumber << (BLOCK_ERASE_SHT_AMT-8);
    uint8_t opcode[4] = {DBFLASH_OPCODE_BLOCK_ERASE, (uint8_t)(temp_uint >> 8), (uint8_t)temp_uint, 0};
    dbflash_send_command(descriptor_pt, opcode, sizeof(opcode));
    
    /* Wait until memory is ready */
    dbflash_wait_for_not_busy(descriptor_pt);
}

/*! \fn     dbflash_page_erase(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber)
*   \brief  Erases page pageNumber (0 up to PAGE_COUNT valid).
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \param  pageNumber      The page to erase
*   \note   Sets all bits in page to Logic 1 (High)
*/
void dbflash_page_erase(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber)
{    
    #ifdef MEMORY_BOUNDARY_CHECKS
        // Error check parameter pageNumber
        if(pageNumber >= PAGE_COUNT) // Ex: 1M -> PAGE_COUNT = 512.. valid pageNumber 0-511
        {
            dbflash_memory_boundary_error_callblack();
        }
    #endif
    
    uint8_t opcode[4] = {DBFLASH_OPCODE_PAGE_ERASE};
    dbflash_fill_page_read_write_erase_opcode_from_address(pageNumber, 0, &opcode[1]);    // We can add the offset as they're "don't care" in the datasheet
    dbflash_send_command(descriptor_pt, opcode, sizeof(opcode));
    
    /* Wait until memory is ready */
    dbflash_wait_for_not_busy(descriptor_pt);
}

/*! \fn     dbflash_format_flash(spi_flash_descriptor_t* descriptor_pt) 
*   \brief  Erases the entirety of spi flash memory by calling the appropriate erase functions.
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \note   Sets all bits in spi flash memory to Logic 1 (High)
*/
void dbflash_format_flash(spi_flash_descriptor_t* descriptor_pt) 
{    
    dbflash_sector_zero_erase(descriptor_pt, DBFLASH_SECTOR_ZERO_A_CODE); // erase sector 0a
    dbflash_sector_zero_erase(descriptor_pt, DBFLASH_SECTOR_ZERO_B_CODE); // erase sector 0b
    
    for(uint8_t i = SECTOR_START; i <= SECTOR_END; i++)
    {
        dbflash_sector_erase(descriptor_pt, i);
    }
}

/*! \fn     dbflash_load_page_to_internal_buffer(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber)
*   \brief  Load a given page in the flash internal buffer
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \param  pageNumber      The target page number of flash memory
*/
void dbflash_load_page_to_internal_buffer(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber)
{
    #ifdef MEMORY_BOUNDARY_CHECKS
        // Error check the parameter pageNumber
        if(pageNumber >= PAGE_COUNT) // Ex: 1M -> PAGE_COUNT = 512.. valid pageNumber 0-511
        {
            dbflash_memory_boundary_error_callblack();
        }
    #endif
    
    // Load the page in the internal buffer
    uint8_t opcode[4] = {DBFLASH_OPCODE_MAINP_TO_BUF};
    dbflash_fill_page_read_write_erase_opcode_from_address(pageNumber, 0, &opcode[1]);
    dbflash_send_command(descriptor_pt, opcode, sizeof(opcode));
    
    /* Wait until memory is ready */
    dbflash_wait_for_not_busy(descriptor_pt);
}

/*! \fn     dbflash_write_data_to_flash(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber, uint16_t offset, uint16_t dataSize, void *data)
*   \brief  Writes a data buffer to flash memory. The data is written starting at offset of a page.
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \param  pageNumber      The target page number of flash memory
*   \param  offset          The starting byte offset to begin writing in pageNumber
*   \param  dataSize        The number of bytes to write from the data buffer (assuming the data buffer is sufficiently large)
*   \param  data            The buffer containing the data to write to flash memory
*   \note   The buffer will be destroyed.
*   \note   Function does not allow crossing page boundaries.
*/
void dbflash_write_data_to_flash(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber, uint16_t offset, uint16_t dataSize, void *data)
{    
    #ifdef MEMORY_BOUNDARY_CHECKS
        // Error check the parameter pageNumber
        if(pageNumber >= PAGE_COUNT) // Ex: 1M -> PAGE_COUNT = 512.. valid pageNumber 0-511
        {
            dbflash_memory_boundary_error_callblack();
        }
    
        // Error check the parameters offset and dataSize
        if((offset + dataSize - 1) >= BYTES_PER_PAGE) // Ex: 1M -> BYTES_PER_PAGE = 264 offset + dataSize MUST be less than 264 (0-263 valid)
        {
            dbflash_memory_boundary_error_callblack();
        }
    #endif
    
    // Load the page in the internal buffer
    dbflash_load_page_to_internal_buffer(descriptor_pt, pageNumber);
    
    // Write the bytes in the buffer, write the buffer to page
    uint8_t opcode[4] = {DBFLASH_OPCODE_MMP_PROG_TBUF};
    dbflash_fill_page_read_write_erase_opcode_from_address(pageNumber, offset, &opcode[1]); 
    dbflash_send_data_with_four_bytes_opcode(descriptor_pt, opcode, data, dataSize);
    
    /* Wait until memory is ready */
    dbflash_wait_for_not_busy(descriptor_pt);
}

/*! \fn     dbflash_read_data_from_flash(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber, uint16_t offset, uint16_t dataSize, void *data)
*   \brief  Reads a data buffer of flash memory. The data is read starting at offset of a page.
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \param  pageNumber      The target page number of flash memory
*   \param  offset          The starting byte offset to begin reading in pageNumber
*   \param  dataSize        The number of bytes to read from the flash memory into the data buffer (assuming the data buffer is sufficiently large)
*   \param  data            The buffer used to store the data read from flash
*   \note   Function does not allow crossing page boundaries.
*/
void dbflash_read_data_from_flash(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber, uint16_t offset, uint16_t dataSize, void *data)
{        
    #ifdef MEMORY_BOUNDARY_CHECKS
        // Error check the parameter pageNumber
        if(pageNumber >= PAGE_COUNT) // Ex: 1M -> PAGE_COUNT = 512.. valid pageNumber 0-511
        {
            dbflash_memory_boundary_error_callblack();
        }    
        // Error check the parameters offset and dataSize
        if((offset + dataSize - 1) >= BYTES_PER_PAGE) // Ex: 1M -> BYTES_PER_PAGE = 264 offset + dataSize MUST be less than 264 (0-263 valid)
        {
            dbflash_memory_boundary_error_callblack();
        }
    #endif
    
    uint8_t opcode[4] = {DBFLASH_OPCODE_LOWF_READ};
    dbflash_fill_page_read_write_erase_opcode_from_address(pageNumber, offset, &opcode[1]);
    dbflash_send_data_with_four_bytes_opcode(descriptor_pt, opcode, data, dataSize);
} 

/*! \fn     dbflash_raw_read(spi_flash_descriptor_t* descriptor_pt, uint8_t* datap, uint16_t addr, uint16_t size)
*   \brief  Contiguous data read across flash page boundaries with a max 65k bytes addressing space
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \param  datap           pointer to the buffer to store the read data
*   \param  addr            byte offset in the flash
*   \param  size            the number of bytes to read
*   \note   bypasses the memory buffer
*/
void dbflash_raw_read(spi_flash_descriptor_t* descriptor_pt, uint8_t* datap, uint16_t addr, uint16_t size)
{    
    uint16_t page_number = (addr/BYTES_PER_PAGE);
    uint8_t high_byte = page_number >> (16 - READ_OFFSET_SHT_AMT);
    addr = (page_number << READ_OFFSET_SHT_AMT) | (addr % BYTES_PER_PAGE);
    uint8_t op[] = {DBFLASH_OPCODE_LOWF_READ, high_byte, (uint8_t)(addr >> 8), (uint8_t)addr};            

    /* Read from flash */
    dbflash_send_data_with_four_bytes_opcode(descriptor_pt, op, datap, size);
}

/*! \fn     dbflash_write_buffer(spi_flash_descriptor_t* descriptor_pt, uint8_t* datap, uint16_t offset, uint16_t size)
*   \brief  Write data into the internal memory buffer
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \param  datap pointer to data to write
*   \param  offset offset to start writing to in the internal memory buffer
*   \param  size the number of bytes to write
*   \note    if the end of the internal buffer is reached then writing will wrap to the start of the internal buffer.
*/
void dbflash_write_buffer(spi_flash_descriptor_t* descriptor_pt, uint8_t* datap, uint16_t offset, uint16_t size)
{
    uint8_t op[4] = {DBFLASH_OPCODE_BUF_WRITE};
    dbflash_fill_page_read_write_erase_opcode_from_address(0, offset, &op[1]);
    dbflash_send_data_with_four_bytes_opcode(descriptor_pt, op, datap, size);
    dbflash_wait_for_not_busy(descriptor_pt);
}

/*! \fn     dbflash_flash_write_buffer_to_page(spi_flash_descriptor_t* descriptor_pt, uint16_t page)
*   \brief  write the contents of the internal memory buffer to a page in flash
*   \param  descriptor_pt   Pointer to dbflash descriptor
*   \param  page    the page to store the buffer in
*/
void dbflash_flash_write_buffer_to_page(spi_flash_descriptor_t* descriptor_pt, uint16_t page)
{
    uint8_t op[4] = {DBFLASH_OPCODE_BUF_TO_PAGE};
    dbflash_fill_page_read_write_erase_opcode_from_address(page, 0, &op[1]);
    dbflash_send_data_with_four_bytes_opcode(descriptor_pt, op, op, 0);
    dbflash_wait_for_not_busy(descriptor_pt);
}