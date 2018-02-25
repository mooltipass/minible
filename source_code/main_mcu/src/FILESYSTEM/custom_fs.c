/*
 * custom_fs.c
 *
 * Created: 16/05/2017 11:39:30
 *  Author: stephan
 */
#include <string.h>
#include <asf.h>
#include "mooltipass_graphics_bundle.h"
#include "platform_defines.h"
#include "driver_sercom.h"
#include "custom_fs.h"
#include "dataflash.h"
#include "dma.h"

/* Current selected language entry */
language_map_entry_t custom_fs_cur_language_entry = {.starting_bitmap = 0, .starting_font = 0, .string_file_index = 0};
/* Temp values to speed up string files reading */
custom_fs_string_count_t custom_fs_current_text_file_string_count = 0;
custom_fs_address_t custom_fs_current_text_file_addr = 0;
/* Our platform settings array, in internal NVM */
custom_platform_settings_t* custom_fs_platform_settings_p = 0;
/* dataflash port descriptor */
spi_flash_descriptor_t* custom_fs_dataflash_desc = 0;
/* Flash header */
custom_file_flash_header_t custom_fs_flash_header;
/* Bool to specify if the SPI bus is left opened */
BOOL custom_fs_data_bus_opened = FALSE;
/* Temp string buffers for string reading */
uint16_t custom_fs_temp_string1[128];


/*! \fn     custom_fs_read_from_flash(uint8_t* datap, custom_fs_address_t address, uint32_t size)
*   \brief  Read data from the external flash
*   \param  datap       Pointer to where to store the data
*   \param  address     Where to read the data
    \param  size        How many bytes to read
*   \return success status
*/
RET_TYPE custom_fs_read_from_flash(uint8_t* datap, custom_fs_address_t address, uint32_t size)
{
    //dataflash_read_data_array(custom_fs_dataflash_desc, address, datap, size);
    memcpy(datap, &mooltipass_bundle[address], size);
    return RETURN_OK;
}

/*! \fn     custom_fs_continuous_read_from_flash(uint8_t* datap, custom_fs_address_t address, uint32_t size)
*   \brief  Continuous data read from the external flash
*   \param  datap       Pointer to where to store the data
*   \param  address     Where to read the data
    \param  size        How many bytes to read
    \param  use_dma     Boolean to specify if we use DMA: if set, function will return before data is transferred!
*   \return success status
*/
RET_TYPE custom_fs_continuous_read_from_flash(uint8_t* datap, custom_fs_address_t address, uint32_t size, BOOL use_dma)
{
    /* Check if we have opened the SPI bus */
    if (custom_fs_data_bus_opened == FALSE)
    {
        dataflash_read_data_array_start(custom_fs_dataflash_desc, address);
        custom_fs_data_bus_opened = TRUE;
    } 
    
    /* If we are using DMA */
    if (use_dma != FALSE)
    {
        /* Arm DMA transfer */        
        dma_custom_fs_init_transfer((void*)&custom_fs_dataflash_desc->sercom_pt->SPI.DATA.reg, (void*)datap, size);
    } 
    else
    {
        /* Read data */
        dataflash_read_bytes_from_opened_transfer(custom_fs_dataflash_desc, datap, size);
    }
    
    return RETURN_OK;
}

/*! \fn     custom_fs_compute_and_check_external_bundle_crc32(void)
*   \brief  Compute the crc32 of our bundle
*   \return Success status
*/
RET_TYPE custom_fs_compute_and_check_external_bundle_crc32(void)
{
    /* Start a read on external flash */
    dataflash_read_data_array_start(custom_fs_dataflash_desc, CUSTOM_FS_FILES_ADDR_OFFSET + sizeof(custom_fs_flash_header.magic_header) + sizeof(custom_fs_flash_header.total_size) + sizeof(custom_fs_flash_header.crc32));
    
    /* Use the DMA controller to compute the crc32 */
    uint32_t crc32 = dma_bootloader_compute_crc32_from_spi((void*)&custom_fs_dataflash_desc->sercom_pt->SPI.DATA.reg, custom_fs_flash_header.total_size - sizeof(custom_fs_flash_header.magic_header) - sizeof(custom_fs_flash_header.total_size) - sizeof(custom_fs_flash_header.crc32));
    
    /* Stop transfer */
    dataflash_stop_ongoing_transfer(custom_fs_dataflash_desc);
    
    /* Do the final check */
    if (custom_fs_flash_header.crc32 == crc32)
    {
        return RETURN_OK;
    } 
    else
    {
        return RETURN_NOK;
    }
}

/*! \fn     custom_fs_stop_continuous_read_from_flash(void)
*   \brief  Stop a continuous flash read
*/
void custom_fs_stop_continuous_read_from_flash(void)
{
    dataflash_stop_ongoing_transfer(custom_fs_dataflash_desc);
    custom_fs_data_bus_opened = FALSE;
}

/*! \fn     custom_fs_settings_init(spi_flash_descriptor_t* desc)
*   \brief  Initialize our settings system
*/
void custom_fs_settings_init(void)
{        
    /* Initialize platform settings pointer, located in slot 0 of internal storage */
    uint32_t flash_addr = custom_fs_get_custom_storage_slot_addr(SETTINGS_STORAGE_SLOT);
    if (flash_addr != 0)
    {
        custom_fs_platform_settings_p = (custom_platform_settings_t*)flash_addr;
    }    
}

/*! \fn     custom_fs_get_number_of_languages(void)
*   \brief  Get number of languages currently supported
*   \return I'll let you guess...
*/
uint32_t custom_fs_get_number_of_languages(void)
{
    return custom_fs_flash_header.language_map_item_count;
}

/*! \fn     custom_fs_get_current_language_text_desc(void)
*   \brief  Get current language string description
*   \return Max 18 chars long string
*/
cust_char_t* custom_fs_get_current_language_text_desc(void)
{
    return custom_fs_cur_language_entry.language_descr;
}

/*! \fn     custom_fs_set_current_language(uint16_t language_id)
*   \brief  Set current language
*   \param  language_id     Language ID
*   \return RETURN_(N)OK
*/
ret_type_te custom_fs_set_current_language(uint16_t language_id)
{
    /* Check for valid language id */
    if (language_id >= custom_fs_flash_header.language_map_item_count)
    {
        return RETURN_NOK;
    }
    
    /* Load address to language map table */
    custom_fs_address_t language_map_table_addr;
    custom_fs_read_from_flash((uint8_t*)&language_map_table_addr, CUSTOM_FS_FILES_ADDR_OFFSET + custom_fs_flash_header.language_map_offset, sizeof(language_map_table_addr));
    
    /* Load language map entry */
    custom_fs_read_from_flash((uint8_t*)&custom_fs_cur_language_entry, CUSTOM_FS_FILES_ADDR_OFFSET + language_map_table_addr + (language_id*sizeof(custom_fs_cur_language_entry)), sizeof(custom_fs_cur_language_entry));
    
    /* Try to read address and file count of text file for this language */
    if (custom_fs_get_file_address(custom_fs_cur_language_entry.string_file_index, &custom_fs_current_text_file_addr, CUSTOM_FS_STRING_TYPE) != RETURN_NOK)
    {
        custom_fs_read_from_flash((uint8_t*)&custom_fs_current_text_file_string_count, custom_fs_current_text_file_addr, sizeof(custom_fs_current_text_file_string_count));
    }
    
    return RETURN_OK;
}

/*! \fn     custom_fs_init(void)
*   \brief  Initialize our custom file system... system
*   \param  desc    Pointer to the SPI flash port descriptor
*/
void custom_fs_init(spi_flash_descriptor_t* desc)
{
    /* Locally copy the flash descriptor */
    custom_fs_dataflash_desc = desc;
    
    /* Read flash header */
    custom_fs_read_from_flash((uint8_t*)&custom_fs_flash_header, CUSTOM_FS_FILES_ADDR_OFFSET, sizeof(custom_fs_flash_header));
    
    /* Set default language */
    custom_fs_set_current_language(0);
}

/*! \fn     custom_fs_get_string_from_file(uint32_t text_file_id, uint32_t string_id, char* string_pt)
*   \brief  Read a string from a string file
*   \param  string_id   String ID
*   \param  string_pt   Pointer to the returned string
*   \return success status
*/
RET_TYPE custom_fs_get_string_from_file(uint32_t string_id, cust_char_t** string_pt)
{
    custom_fs_string_offset_t string_offset;
    custom_fs_string_length_t string_length;
    
    /* Check that file #0 was requested and that file doesn't actually exist */
    if (custom_fs_current_text_file_addr == 0)
    {
        return RETURN_NOK;
    }
    
    /* Check if string_id is valid */
    if (string_id >= custom_fs_current_text_file_string_count)
    {
        return RETURN_NOK;
    }
    
    /* Read string offset */
    custom_fs_read_from_flash((uint8_t*)&string_offset, custom_fs_current_text_file_addr + sizeof(custom_fs_current_text_file_string_count) + string_id * sizeof(string_offset), sizeof(string_offset));
    
    /* Read string length */
    custom_fs_read_from_flash((uint8_t*)&string_length, custom_fs_current_text_file_addr + string_offset, sizeof(string_length));
    
    /* Check string length (already contains terminating 0) */
    if (string_length > sizeof(custom_fs_temp_string1))
    {
        string_length = sizeof(custom_fs_temp_string1);
    }
    
    /* Read string : *2 because of uint16_t used to store chars */
    custom_fs_read_from_flash((uint8_t*)custom_fs_temp_string1, custom_fs_current_text_file_addr + string_offset + sizeof(string_length), string_length*2);
    
    /* Add terminating 0 just in case */
    custom_fs_temp_string1[(sizeof(custom_fs_temp_string1)/sizeof(custom_fs_temp_string1[0]))-1] = 0;
    
    /* Store pointer to string */
    *string_pt = custom_fs_temp_string1;
    
    return RETURN_OK;
}

/*! \fn     custom_fs_get_file_address(uint32_t file_id, custom_fs_address_t* address)
*   \brief  Get an address for a file stored in the external flash
*   \param  file_id     File ID
*   \param  address     Pointer to where to store the address
*   \param  file_type   File type (see enum)
*   \return success status
*/
RET_TYPE custom_fs_get_file_address(uint32_t file_id, custom_fs_address_t* address, custom_fs_file_type_te file_type)
{
    custom_fs_address_t file_table_address;
    uint32_t language_offset = 0;

    // Check for invalid file index or flash not formatted
    if (file_type == CUSTOM_FS_STRING_TYPE)
    {
        if ((file_id >= custom_fs_flash_header.string_file_count) || (custom_fs_flash_header.string_file_count == CUSTOM_FS_MAX_FILE_COUNT))
        {
            return RETURN_NOK;
        }
        else
        {
            file_table_address = custom_fs_flash_header.string_file_offset;
        }
    }
    else if (file_type == CUSTOM_FS_FONTS_TYPE)
    {        
        /* Take into account possible offset due to different font for current language */
        language_offset = custom_fs_cur_language_entry.starting_font;
        
        if (((file_id + language_offset) >= custom_fs_flash_header.fonts_file_count) || (custom_fs_flash_header.fonts_file_count == CUSTOM_FS_MAX_FILE_COUNT))
        {
            return RETURN_NOK;
        }
        else
        {
            file_table_address = custom_fs_flash_header.fonts_file_offset;
        }
    }
    else if (file_type == CUSTOM_FS_BITMAP_TYPE)
    {        
        /* Take into account possible offset due to different font for current language */
        if (file_id >= custom_fs_flash_header.language_bitmap_starting_id)
        {
            language_offset = custom_fs_cur_language_entry.starting_bitmap;
        }
        
        if (((file_id + language_offset) >= custom_fs_flash_header.bitmap_file_count) || (custom_fs_flash_header.bitmap_file_count == CUSTOM_FS_MAX_FILE_COUNT))
        {
            return RETURN_NOK;
        }
        else
        {
            file_table_address = custom_fs_flash_header.bitmap_file_offset;
        }
    }
    else if (file_type == CUSTOM_FS_BINARY_TYPE)
    {
        if ((file_id >= custom_fs_flash_header.binary_img_file_count) || (custom_fs_flash_header.binary_img_file_count == CUSTOM_FS_MAX_FILE_COUNT))
        {
            return RETURN_NOK;
        }
        else
        {
            file_table_address = custom_fs_flash_header.binary_img_file_offset;
        }
    }
    else if (file_type == CUSTOM_FS_FW_UPDATE_TYPE)
    {
        if ((file_id >= custom_fs_flash_header.update_file_count) || (custom_fs_flash_header.update_file_count == CUSTOM_FS_MAX_FILE_COUNT))
        {
            return RETURN_NOK;
        }
        else
        {
            file_table_address = custom_fs_flash_header.update_file_offset;
        }
    }
    else
    {
        return RETURN_NOK;
    }

    /* Read the file address : <filecount> <fileid0><address0> <fileid1><address1> ... */
    custom_fs_read_from_flash((uint8_t*)address, CUSTOM_FS_FILES_ADDR_OFFSET + file_table_address + (file_id + language_offset) * sizeof(*address), sizeof(*address));
    
    /* Add the file address offset */
    *address += CUSTOM_FS_FILES_ADDR_OFFSET;
    
    return RETURN_OK;
}

/*! \fn     custom_fs_get_custom_storage_slot_addr(uint32_t slot_id)
*   \brief  Get the internal flash address for a given storage slot id
*   \param  slot_id     slot ID
*   \return 0 if the slot_id is not valid, otherwise the address
*/
uint32_t custom_fs_get_custom_storage_slot_addr(uint32_t slot_id)
{
    uint32_t emulated_eeprom_size = 0;
    
    #if 256 != NVMCTRL_ROW_SIZE
        #error "NVM ROW SIZE ERROR"
    #endif
    
    /* Get RWWEEE memory settings */
    uint32_t fuse = ((*((uint32_t *)NVMCTRL_AUX0_ADDRESS)) & NVMCTRL_FUSES_EEPROM_SIZE_Msk) >> 4;
    
    if (fuse == 7)
    {
        /* No simulated eeprom configured */
        return 0;
    }
    else
    {
        /* Compute amount of simulated EEPROM */
        emulated_eeprom_size = NVMCTRL_ROW_SIZE << (6 - fuse);
    }
    
    /* Check slot_id */
    if (slot_id >= emulated_eeprom_size/NVMCTRL_ROW_SIZE)
    {
        return 0;
    }
    
    /* Compute address of where we want to write data */
    return (FLASH_ADDR + FLASH_SIZE - emulated_eeprom_size + slot_id*NVMCTRL_ROW_SIZE);
}

/*! \fn     custom_fs_write_256B_at_internal_custom_storage_slot(uint32_t slot_id, void* array)
*   \brief  Write 256 bytes in a custom storage slot, located in NVM configured as EEPROM or EEPROM
*   \param  slot_id     slot ID
*   \param  array       256 bytes array (matches NVMCTRL_ROW_SIZE)
*   \note   Please make sure the fuses are correctly configured
*   \note   NVM configured as EEPROM allows access to NVM when EEPROM is being written or erased (convenient when interrupts occur)
*/
void custom_fs_write_256B_at_internal_custom_storage_slot(uint32_t slot_id, void* array)
{
#ifndef FEATURE_NVM_RWWEE    
    /* Compute address of where we want to write data */
    uint32_t flash_addr = custom_fs_get_custom_storage_slot_addr(slot_id);
    
    /* Check if we were successful */
    if (flash_addr == 0)
    {
        return;
    }
    
    /* Automatic write, disable caching */
    NVMCTRL->CTRLB.bit.MANW = 0;
    NVMCTRL->CTRLB.bit.CACHEDIS = 1;
    
    /* Erase complete row */
    while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
    NVMCTRL->ADDR.reg  = flash_addr/2;
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;
        
    /* Flash bytes */
    for (uint32_t j = 0; j < 4; j++)
    {
        /* Flash 4 consecutive pages */
        while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
        for(uint32_t i = 0; i < NVMCTRL_ROW_SIZE/4; i+=2)
        {
            NVM_MEMORY[(flash_addr+j*(NVMCTRL_ROW_SIZE/4)+i)/2] = ((uint16_t*)array)[(j*(NVMCTRL_ROW_SIZE/4)+i)/2];
        }
    }

    /* Disable automatic write, enable caching */
    NVMCTRL->CTRLB.bit.MANW = 1;
    NVMCTRL->CTRLB.bit.CACHEDIS = 0;
#endif
}

/*! \fn     custom_fs_read_256B_at_internal_custom_storage_slot(uint32_t slot_id, void* array)
*   \brief  Read 256 bytes in a custom storage slot, located in NVM configured as EEPROM or EEPROM
*   \param  slot_id     slot ID
*   \param  array       256 bytes array (matches NVMCTRL_ROW_SIZE)
*   \note   Please make sure the fuses are correctly configured
*   \note   NVM configured as EEPROM allows access to NVM when EEPROM is being written or erased (convenient when interrupts occur)
*/
void custom_fs_read_256B_at_internal_custom_storage_slot(uint32_t slot_id, void* array)
{
#ifndef FEATURE_NVM_RWWEE
    /* Compute address of where we want to read data */
    uint32_t flash_addr = custom_fs_get_custom_storage_slot_addr(slot_id);
    
    /* Check if we were successful */
    if (flash_addr == 0)
    {
        return;
    }
    
    /* Perform the copy */
    memcpy(array, (const void*)flash_addr, NVMCTRL_ROW_SIZE);
#endif
}

/*! \fn     custom_fs_settings_set_fw_upgrade_flag(void)
*   \brief  Set the fw upgrade flag inside our settings
*/
void custom_fs_settings_set_fw_upgrade_flag(void)
{
    volatile custom_platform_settings_t temp_settings;
    custom_fs_read_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
    temp_settings.start_upgrade_flag = FIRMWARE_UPGRADE_FLAG;
    custom_fs_write_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
    return;    
}

/*! \fn     custom_fs_settings_clear_fw_upgrade_flag(void)
*   \brief  Clear the fw upgrade flag inside our settings
*/
void custom_fs_settings_clear_fw_upgrade_flag(void)
{
    volatile custom_platform_settings_t temp_settings;
    custom_fs_read_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
    temp_settings.start_upgrade_flag = 0;
    custom_fs_write_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
    return;    
}

/*! \fn     custom_fs_settings_check_fw_upgrade_flag(void)
*   \brief  Check for fw upgrade flag inside our settings
*   \return The boolean
*/
BOOL custom_fs_settings_check_fw_upgrade_flag(void)
{
    if ((custom_fs_platform_settings_p != 0) && (custom_fs_platform_settings_p->start_upgrade_flag == FIRMWARE_UPGRADE_FLAG))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}