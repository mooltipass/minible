/*
 * custom_fs.c
 *
 * Created: 16/05/2017 11:39:30
 *  Author: stephan
 */
#include <string.h>
#include <asf.h>
#include "mooltipass_graphics_bundle.h"
#include "custom_fs_emergency_font.h"
#include "platform_defines.h"
#include "driver_sercom.h"
#include "logic_device.h"
#include "custom_fs.h"
#include "dataflash.h"
#include "dma.h"

/* Default device settings */
const uint8_t custom_fs_default_device_settings[NB_DEVICE_SETTINGS] = {0,FALSE,SETTING_DFT_USER_INTERACTION_TIMEOUT,TRUE,0};
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
/* Current language id */
uint8_t custom_fs_cur_language_id = 0;
/* Current keyboard layout id */
custom_fs_address_t custom_fs_keyboard_layout_addr = 0;
uint8_t custom_fs_cur_keyboard_id = 0;
/* CPZ look up table */
cpz_lut_entry_t* custom_fs_cpz_lut;


/*! \fn     custom_fs_read_from_flash(uint8_t* datap, custom_fs_address_t address, uint32_t size)
*   \brief  Read data from the external flash
*   \param  datap       Pointer to where to store the data
*   \param  address     Where to read the data
    \param  size        How many bytes to read
*   \return success status
*/
RET_TYPE custom_fs_read_from_flash(uint8_t* datap, custom_fs_address_t address, uint32_t size)
{
    /* Check for emergency font file exception */
    if ((address >= CUSTOM_FS_EMERGENCY_FONT_FILE_ADDR) && (address+size <= CUSTOM_FS_EMERGENCY_FONT_FILE_ADDR + sizeof(custom_fs_emergency_font_file)))
    {
        memcpy(datap, &custom_fs_emergency_font_file[address-CUSTOM_FS_EMERGENCY_FONT_FILE_ADDR], size);
    } 
    else
    {
        dataflash_read_data_array(custom_fs_dataflash_desc, address, datap, size);
        //memcpy(datap, &mooltipass_bundle[address], size);
    }
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
    /* Check for emergency font file exception */
    if ((address >= CUSTOM_FS_EMERGENCY_FONT_FILE_ADDR) && (address+size <= CUSTOM_FS_EMERGENCY_FONT_FILE_ADDR + sizeof(custom_fs_emergency_font_file)))
    {
        memcpy(datap, &custom_fs_emergency_font_file[address-CUSTOM_FS_EMERGENCY_FONT_FILE_ADDR], size);
        
        /* If we are using DMA, set the flag indicating transfer done */
        if (use_dma != FALSE)
        {
            dma_set_custom_fs_flag_done();
        }
    }
    else
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

    /* Main FW: reset DMA controler */
    #ifndef BOOTLOADER
    dma_reset();
    #endif

    /* Use the DMA controller to compute the crc32 */
    uint32_t crc32 = dma_compute_crc32_from_spi((void*)&custom_fs_dataflash_desc->sercom_pt->SPI.DATA.reg, custom_fs_flash_header.total_size - sizeof(custom_fs_flash_header.magic_header) - sizeof(custom_fs_flash_header.total_size) - sizeof(custom_fs_flash_header.crc32));
    
    /* Stop transfer */
    dataflash_stop_ongoing_transfer(custom_fs_dataflash_desc);
    
    /* Main FW: reset DMA controler */
    #ifndef BOOTLOADER
    dma_reset();
    #endif
    
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
*   \return Intialization success state
*/
custom_fs_init_ret_type_te custom_fs_settings_init(void)
{        
    /* Initialize platform settings pointer, located in slot 0 of internal storage */
    uint32_t flash_addr = custom_fs_get_custom_storage_slot_addr(SETTINGS_STORAGE_SLOT);
    if (flash_addr != 0)
    {
        custom_fs_platform_settings_p = (custom_platform_settings_t*)flash_addr;
        flash_addr = custom_fs_get_custom_storage_slot_addr(FIRST_CPZ_LUT_ENTRY_STORAGE_SLOT);
        custom_fs_cpz_lut = (cpz_lut_entry_t*)flash_addr;
        
        /* Quick sanity check on memory boundary */
        if ((uint32_t)&custom_fs_cpz_lut[MAX_NUMBER_OF_USERS] != FLASH_ADDR + FLASH_SIZE)
        {
            while(1);
        }
        
        return CUSTOM_FS_INIT_OK;
    }    
    else
    {
        return CUSTOM_FS_INIT_NO_RWEE;
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

/*! \fn     custom_fs_get_current_language_id(void)
*   \brief  Get current language ID
*   \return The current language ID
*/
uint8_t custom_fs_get_current_language_id(void)
{
    return custom_fs_cur_language_id;
} 

/*! \fn     custom_fs_get_current_layout_id(void)
*   \brief  Get current keyboard layout ID
*   \return The current layout ID
*/
uint8_t custom_fs_get_current_layout_id(void)
{
    return custom_fs_cur_keyboard_id;
}

/*! \fn     custom_fs_get_number_of_keyb_layouts(void)
*   \brief  Get total number of supported keyboard layouts
*   \return Number of keyboard layouts
*/
uint32_t custom_fs_get_number_of_keyb_layouts(void)
{
    return custom_fs_flash_header.binary_img_file_count;
}

/*! \fn     custom_fs_get_keyboard_descriptor_string(uint8_t keyboard_id, cust_char_t* string_pt)
*   \brief  Get the string describing a given keyboard layout
*   \param  keyboard_id     The keyboard ID
*   \param  string_pt       Where to store the string (CUSTOM_FS_KEYBOARD_DESC_LGTH long)
*   \return RETURN_(N)OK
*/
ret_type_te custom_fs_get_keyboard_descriptor_string(uint8_t keyboard_id, cust_char_t* string_pt)
{
    custom_fs_address_t layout_file_addr;
    
    /* Try to fetch layout file address */
    ret_type_te file_address_found = custom_fs_get_file_address((uint32_t)keyboard_id, &layout_file_addr, CUSTOM_FS_BINARY_TYPE);
    
    /* Check for success */
    if (file_address_found != RETURN_OK)
    {
        return RETURN_NOK;
    }
    
    /* Load description, 0 terminate it in case */
    custom_fs_read_from_flash((uint8_t*)string_pt, CUSTOM_FS_FILES_ADDR_OFFSET + layout_file_addr, CUSTOM_FS_KEYBOARD_DESC_LGTH*sizeof(cust_char_t));
    string_pt[CUSTOM_FS_KEYBOARD_DESC_LGTH-1] = 0;
    
    return RETURN_OK;
}

/*! \fn     custom_fs_set_current_keyboard_id(uint8_t keyboard_id)
*   \brief  Set current keyboard ID
*   \param  keyboard_id     Keyboard ID
*   \return RETURN_(N)OK
*/
ret_type_te custom_fs_set_current_keyboard_id(uint8_t keyboard_id)
{
    custom_fs_address_t layout_file_addr;
    
    /* Try to fetch layout file address */
    ret_type_te file_address_found = custom_fs_get_file_address((uint32_t)keyboard_id, &layout_file_addr, CUSTOM_FS_BINARY_TYPE);
    
    /* Check for success */
    if (file_address_found != RETURN_OK)
    {
        return RETURN_NOK;
    }
    
    /* Store address and ID */
    custom_fs_keyboard_layout_addr = layout_file_addr;
    custom_fs_cur_keyboard_id = keyboard_id;
    
    return RETURN_OK;
}

/*! \fn     custom_fs_get_keyboard_symbols_for_unicode_string(cust_char_t* string_pt, uint16_t* buffer)
*   \brief  Get keyboard symbols (not keys) for a given unicode string
*   \param  string_pt   Pointer to the unicode BMP string
*   \param  buffer      Where to store the symbols. Non supported symbols will be stored as 0xFFFF
*   \return RETURN_(N)OK depending on if we were able to "translate" the complete string
*   \note   Take care of buffer overflows. One symbol will be generated per unicode point
*/
ret_type_te custom_fs_get_keyboard_symbols_for_unicode_string(cust_char_t* string_pt, uint16_t* buffer)
{
    unicode_interval_desc_t description_intervals[CUSTOM_FS_KEYB_NB_INT_DESCRIBED];
    BOOL point_support_described = FALSE;
    uint16_t symbol_desc_pt_offset = 0;
    BOOL all_points_described = TRUE;
    uint16_t interval_start = 0;
    
    /* Check for correctly setup keyboard layout */
    if (custom_fs_keyboard_layout_addr == 0)
    {
        return RETURN_NOK;
    }   
    
    /* Load the description intervals */
    custom_fs_read_from_flash((uint8_t*)description_intervals, custom_fs_keyboard_layout_addr + CUSTOM_FS_KEYBOARD_DESC_LGTH*sizeof(cust_char_t), sizeof(description_intervals));
    
    /* Iterate over string */
    while (*string_pt != 0)
    {
        /* Reset vars */
        point_support_described = FALSE;
        symbol_desc_pt_offset = 0;
        interval_start = 0;
        
        /* Check that support for this point is described */
        for (uint16_t i = 0; i < ARRAY_SIZE(description_intervals); i++)
        {
            /* Check if char is within this interval */
            if ((description_intervals[i].interval_start != 0xFFFF) && (description_intervals[i].interval_start <= *string_pt) && (description_intervals[i].interval_end >= *string_pt))
            {
                interval_start = description_intervals[i].interval_start;
                point_support_described = TRUE;
                break;
            }
            
            /* Add offset to descriptor */
            symbol_desc_pt_offset += description_intervals[i].interval_end - description_intervals[i].interval_start + 1;
        }
        
        /* Check for described point support */
        if (point_support_described == FALSE)
        {
            all_points_described = FALSE;
            *buffer = 0xFFFF;
        }
        else
        {
            /* Fetch keyboard symbol: 0xFFFF for "not supported" matches with our definition of not described */
            custom_fs_read_from_flash((uint8_t*)buffer, custom_fs_keyboard_layout_addr + CUSTOM_FS_KEYBOARD_DESC_LGTH*sizeof(cust_char_t) + sizeof(description_intervals) + symbol_desc_pt_offset*sizeof(*string_pt) + (*string_pt - interval_start)*sizeof(*string_pt), sizeof(*buffer));       
            
            /* Is this symbol supported? */
            if (*buffer == 0xFFFF)
            {
                all_points_described = FALSE;
            }
        }        
        
        /* Move on to the next point */
        string_pt++;
        buffer++;
    }
    
    /* Return depending on if we were able to translate all the string */
    if (all_points_described == FALSE)
    {
        return RETURN_NOK;
    } 
    else
    {
        return RETURN_OK;
    }
}    

/*! \fn     custom_fs_set_current_language(uint8_t language_id)
*   \brief  Set current language
*   \param  language_id     Language ID
*   \return RETURN_(N)OK
*/
ret_type_te custom_fs_set_current_language(uint8_t language_id)
{
    /* Check for valid language id */
    if ((language_id >= custom_fs_flash_header.language_map_item_count) || (custom_fs_flash_header.language_map_item_count == CUSTOM_FS_MAX_FILE_COUNT))
    {
        return RETURN_NOK;
    }
    
    /* Load address to language map table */
    custom_fs_address_t language_map_table_addr;
    custom_fs_read_from_flash((uint8_t*)&language_map_table_addr, CUSTOM_FS_FILES_ADDR_OFFSET + custom_fs_flash_header.language_map_offset, sizeof(language_map_table_addr));
    
    /* Load language map entry, 0 terminate it in case */
    custom_fs_read_from_flash((uint8_t*)&custom_fs_cur_language_entry, CUSTOM_FS_FILES_ADDR_OFFSET + language_map_table_addr + (language_id*sizeof(custom_fs_cur_language_entry)), sizeof(custom_fs_cur_language_entry));
    custom_fs_cur_language_entry.language_descr[MEMBER_ARRAY_SIZE(language_map_entry_t,language_descr)-1] = 0;
    
    /* Try to read address and file count of text file for this language */
    if (custom_fs_get_file_address(custom_fs_cur_language_entry.string_file_index, &custom_fs_current_text_file_addr, CUSTOM_FS_STRING_TYPE) != RETURN_NOK)
    {
        custom_fs_read_from_flash((uint8_t*)&custom_fs_current_text_file_string_count, custom_fs_current_text_file_addr, sizeof(custom_fs_current_text_file_string_count));
    }
    
    /* Language changed, stored current language ID */
    custom_fs_cur_language_id = language_id;
    
    return RETURN_OK;
}

/*! \fn     custom_fs_get_language_description(uint8_t language_id, cust_char_t* string_pt)
*   \brief  Get language description for a given language
*   \param  language_id     Language ID
*   \param  string_pt       Pointer to where to store the description
*   \return RETURN_(N)OK
*/
ret_type_te custom_fs_get_language_description(uint8_t language_id, cust_char_t* string_pt)
{
    /* Check for valid language id */
    if ((language_id >= custom_fs_flash_header.language_map_item_count) || (custom_fs_flash_header.language_map_item_count == CUSTOM_FS_MAX_FILE_COUNT))
    {
        return RETURN_NOK;
    }
    
    /* Load address to language map table */
    custom_fs_address_t language_map_table_addr;
    custom_fs_read_from_flash((uint8_t*)&language_map_table_addr, CUSTOM_FS_FILES_ADDR_OFFSET + custom_fs_flash_header.language_map_offset, sizeof(language_map_table_addr));
    
    /* Load language map entry, 0 terminate it in case */
    custom_fs_read_from_flash((uint8_t*)string_pt, CUSTOM_FS_FILES_ADDR_OFFSET + language_map_table_addr + (language_id*sizeof(custom_fs_cur_language_entry)), MEMBER_SIZE(language_map_entry_t,language_descr));
    string_pt[MEMBER_ARRAY_SIZE(language_map_entry_t,language_descr)-1] = 0;   
    
    return RETURN_OK; 
}

/*! \fn     custom_fs_set_dataflash_descriptor(spi_flash_descriptor_t* desc)
*   \brief  Store the flash descriptor used by our customfs library
*   \param  desc    Pointer to the SPI flash port descriptor
*/
void custom_fs_set_dataflash_descriptor(spi_flash_descriptor_t* desc)
{
    /* Locally copy the flash descriptor */
    custom_fs_dataflash_desc = desc;    
}

/*! \fn     custom_fs_init(void)
*   \brief  Initialize our custom file system... system
*   \return RETURN_(N)OK
*/
ret_type_te custom_fs_init(void)
{    
    /* Read flash header */
    custom_fs_read_from_flash((uint8_t*)&custom_fs_flash_header, CUSTOM_FS_FILES_ADDR_OFFSET, sizeof(custom_fs_flash_header));
    
    /* Check correct header */
    if (custom_fs_flash_header.magic_header != CUSTOM_FS_MAGIC_HEADER)
    {
        return RETURN_NOK;
    }
    
    /* Set default language */
    return custom_fs_set_current_language(0);
}

/*! \fn     custom_fs_get_string_from_file(uint32_t text_file_id, uint32_t string_id, char* string_pt, BOOL lock_on_fail)
*   \brief  Read a string from a string file
*   \param  string_id       String ID
*   \param  string_pt       Pointer to the returned string
*   \param  lock_on_fail    Set to TRUE to lock device if we fail to fetch the string
*   \return success status
*/
RET_TYPE custom_fs_get_string_from_file(uint32_t string_id, cust_char_t** string_pt, BOOL lock_on_fail)
{
    custom_fs_string_offset_t string_offset;
    custom_fs_string_length_t string_length;
    
    /* Check that file #0 was requested and that file doesn't actually exist */
    if (custom_fs_current_text_file_addr == 0)
    {
        if (lock_on_fail != FALSE)
        {
            while(1);
        }
        return RETURN_NOK;
    }
    
    /* Check if string_id is valid */
    if (string_id >= custom_fs_current_text_file_string_count)
    {
        if (lock_on_fail != FALSE)
        {
            while(1);
        }
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

/*! \fn     custom_fs_erase_256B_at_internal_custom_storage_slot(uint32_t slot_id)
*   \brief  Erase 256 bytes in a custom storage slot, located in NVM configured as EEPROM
*   \param  slot_id     slot ID
*   \note   Please make sure the fuses are correctly configured
*   \note   NVM configured as EEPROM allows access to NVM when EEPROM is being written or erased (convenient when interrupts occur)
*/
void custom_fs_erase_256B_at_internal_custom_storage_slot(uint32_t slot_id)
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

    /* Disable automatic write, enable caching */
    NVMCTRL->CTRLB.bit.MANW = 1;
    NVMCTRL->CTRLB.bit.CACHEDIS = 0;
#endif
}

/*! \fn     custom_fs_write_256B_at_internal_custom_storage_slot(uint32_t slot_id, void* array)
*   \brief  Write 256 bytes in a custom storage slot, located in NVM configured as EEPROM
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
}

/*! \fn     custom_fs_set_device_flag_value(custom_fs_flag_id_te flag_id, BOOL value)
*   \brief  Set the boolean value for a given device flag
*   \param  flag_id     Flag ID (see defines)
*   \param  value       TRUE or FALSE
*/
void custom_fs_set_device_flag_value(custom_fs_flag_id_te flag_id, BOOL value)
{
    volatile custom_platform_settings_t temp_settings;
    
    /* Check for overflow and custom fs init state */
    if ((custom_fs_platform_settings_p != 0) && (flag_id < ARRAY_SIZE(custom_fs_platform_settings_p->device_flags)))
    {
        custom_fs_read_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
        if (value == FALSE)
        {
            temp_settings.device_flags[flag_id] = 0xFFFF;
        } 
        else
        {
            temp_settings.device_flags[flag_id] = FLAG_SET_BOOL_VALUE;
        }
        custom_fs_write_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
    }    
}

/*! \fn     custom_fs_get_device_flag_value(custom_fs_flag_id_te flag_id)
*   \brief  Get the boolean value for a given device flag
*   \param  flag_id     Flag ID (see defines)
*   \return TRUE or FALSE
*/
BOOL custom_fs_get_device_flag_value(custom_fs_flag_id_te flag_id)
{
    /* Check for overflow and custom fs init state */
    if ((custom_fs_platform_settings_p != 0) && (flag_id < ARRAY_SIZE(custom_fs_platform_settings_p->device_flags)))
    {
        if (custom_fs_platform_settings_p->device_flags[flag_id] == FLAG_SET_BOOL_VALUE)
        {
            return TRUE;
        } 
        else
        {
            return FALSE;
        }
    }    
    
    return FALSE;
}

/*! \fn     custom_fs_define_nb_ms_since_last_full_charge(uint32_t nb_ms)
*   \brief  Store the number of ms since last full charge
*   \param  nb_ms   Number of ms
*/
void custom_fs_define_nb_ms_since_last_full_charge(uint32_t nb_ms)
{
    volatile custom_platform_settings_t temp_settings;
    custom_fs_read_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
    temp_settings.nb_ms_since_last_full_charge = nb_ms;
    custom_fs_write_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
    return;    
}

/*! \fn     custom_fs_get_nb_ms_since_last_full_charge(void)
*   \brief  Get number of ms since last full charge (first boot: this will be at 0xFFFF and this is actually perfect
*   \return Number of ms
*/
uint32_t custom_fs_get_nb_ms_since_last_full_charge(void)
{
    if (custom_fs_platform_settings_p != 0)
    {
        return custom_fs_platform_settings_p->nb_ms_since_last_full_charge;
    }
    else
    {
        return 0;
    }
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

/*! \fn     custom_fs_settings_get_device_setting(uint16_t setting_id)
*   \brief  Get a given setting for the device
*   \param  setting_id  The setting ID
*   \return The settings value or 0 if there's any error
*/
uint8_t custom_fs_settings_get_device_setting(uint16_t setting_id)
{
    if ((custom_fs_platform_settings_p == 0) || (setting_id >= NB_DEVICE_SETTINGS))
    {
        return 0;
    }
    else
    {
        return custom_fs_platform_settings_p->device_settings[setting_id];
    }
}

/*! \fn     custom_fs_settings_get_dump(uint8_t* dump_buffer)
*   \brief  Get a dump of all device settings
*   \param  dump_buffer Where to put all device settings
*   \return Number of bytes written
*/
uint16_t custom_fs_settings_get_dump(uint8_t* dump_buffer)
{
    if (custom_fs_platform_settings_p == 0)
    {
        return 0;
    }
    else
    {
        memcpy(dump_buffer, custom_fs_platform_settings_p->device_settings, sizeof(custom_fs_platform_settings_p->device_settings));
        return sizeof(custom_fs_platform_settings_p->device_settings);
    }
}

/*! \fn     custom_fs_settings_store_dump(uint8_t* settings_buffer)
*   \brief  Write device settings at once
*   \param  settings_buffer     All device settings
*/
void custom_fs_settings_store_dump(uint8_t* settings_buffer)
{
    custom_platform_settings_t platform_settings_copy;

    if (custom_fs_platform_settings_p != 0)
    {
        /* Copy settings structure stored in flash into ram, overwrite relevant settings part, flash again */
        memcpy(&platform_settings_copy, custom_fs_platform_settings_p, sizeof(custom_platform_settings_t));
        memcpy(platform_settings_copy.device_settings, settings_buffer, sizeof(platform_settings_copy.device_settings));
        custom_fs_write_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&platform_settings_copy);
    }
}

/*! \fn     custom_fs_settings_set_defaults(void)
*   \brief  Set default settings for device
*/
void custom_fs_settings_set_defaults(void)
{
    custom_fs_settings_store_dump((uint8_t*)custom_fs_default_device_settings);
}

/*! \fn     custom_fs_get_cpz_lut_entry(uint8_t* cpz, cpz_lut_entry_t** cpz_entry_pt)
*   \brief  Get a pointer to a CPZ LUT entry given a CPZ 
*   \param  cpz         CPZ bytes
*   \param  cpz_entry   Where to store the pointer to the CPZ entry
*   \return Success status
*/
RET_TYPE custom_fs_get_cpz_lut_entry(uint8_t* cpz, cpz_lut_entry_t** cpz_entry_pt)
{
    // Loop through LUT entries
    for (uint16_t i = 0; i < MAX_NUMBER_OF_USERS; i++)
    {
        // Check for valid user ID (erased flash)
        if (custom_fs_cpz_lut[i].user_id != UINT8_MAX)
        {
            // Check for CPZ match
            if (memcmp(custom_fs_cpz_lut[i].cards_cpz, cpz, sizeof(custom_fs_cpz_lut[0].cards_cpz)) == 0)
            {
                *cpz_entry_pt = &custom_fs_cpz_lut[i];
                return RETURN_OK;
            }
        }
    }
    
    return RETURN_NOK;
}

/*! \fn     custom_fs_detele_user_cpz_lut_entry(uint8_t user_id)
*   \brief  Delete single CPZ LUT entry
*   \param  user_id     The user ID
*/
void custom_fs_detele_user_cpz_lut_entry(uint8_t user_id)
{
    cpz_lut_entry_t one_page_of_lut_entries[4];
    _Static_assert(sizeof(one_page_of_lut_entries) == NVMCTRL_ROW_SIZE, "One row != 4 LUT entries");    
    
    /* Sanity check */
    if (user_id >= MAX_NUMBER_OF_USERS)
    {
        return;
    }
    
    /* Read one page, write it back with modified bytes */
    custom_fs_read_256B_at_internal_custom_storage_slot(FIRST_CPZ_LUT_ENTRY_STORAGE_SLOT + (user_id >> 2), (void*)one_page_of_lut_entries);
    memset(&one_page_of_lut_entries[user_id&0x03], 0xFF, sizeof(one_page_of_lut_entries[0]));
    custom_fs_write_256B_at_internal_custom_storage_slot(FIRST_CPZ_LUT_ENTRY_STORAGE_SLOT + (user_id >> 2), (void*)one_page_of_lut_entries);
}

/*! \fn     custom_fs_get_nb_free_cpz_lut_entries(uint8_t* first_available_user_id)
*   \brief  Get the number of free CPZ LUT entries
*   \param  first_available_user_id     Pointer to where to store the ID of the first available user
*   \return Guess what...
*/
uint16_t custom_fs_get_nb_free_cpz_lut_entries(uint8_t* first_available_user_id)
{
    *first_available_user_id = 0xFF;
    uint16_t return_val = 0;
    
    for (uint8_t i = 0; i < MAX_NUMBER_OF_USERS; i++)
    {
        if (custom_fs_cpz_lut[i].user_id == UINT8_MAX)
        {
            if (*first_available_user_id == 0xFF)
            {
                *first_available_user_id = i;
            }
            return_val++;
        }
    }
    
    return return_val;
}

/*! \fn     custom_fs_update_cpz_entry(cpz_lut_entry_t* cpz_entry, uint8_t user_id)
*   \brief  Update a CPZ LUT entry for a given user ID
*   \param  cpz_entry   Pointer to the CPZ LUT entry
*   \param  user_id     User ID
*   \return Operation result
*/
RET_TYPE custom_fs_update_cpz_entry(cpz_lut_entry_t* cpz_entry, uint8_t user_id)
{
    cpz_lut_entry_t one_page_of_lut_entries[4];
    _Static_assert(sizeof(one_page_of_lut_entries) == NVMCTRL_ROW_SIZE, "One row != 4 LUT entries");
    
    /* Store CPZ entry */
    cpz_entry->user_id = user_id;
    custom_fs_read_256B_at_internal_custom_storage_slot(FIRST_CPZ_LUT_ENTRY_STORAGE_SLOT + (user_id >> 2), (void*)one_page_of_lut_entries);
    memcpy(&one_page_of_lut_entries[user_id&0x03], cpz_entry, sizeof(one_page_of_lut_entries[0]));
    custom_fs_write_256B_at_internal_custom_storage_slot(FIRST_CPZ_LUT_ENTRY_STORAGE_SLOT + (user_id >> 2), (void*)one_page_of_lut_entries);
    return RETURN_OK;
}

/*! \fn     custom_fs_store_cpz_entry(cpz_lut_entry_t* cpz_entry, uint8_t user_id)
*   \brief  Store a CPZ LUT entry for a given user ID
*   \param  cpz_entry   Pointer to the CPZ LUT entry
*   \param  user_id     User ID
*   \return Operation result
*/
RET_TYPE custom_fs_store_cpz_entry(cpz_lut_entry_t* cpz_entry, uint8_t user_id)
{    
    /* Check for availability */
    if (custom_fs_cpz_lut[user_id].user_id != UINT8_MAX)
    {
        return RETURN_NOK;
    }
    
    /* Use update function */
    return custom_fs_update_cpz_entry(cpz_entry, user_id);
}
