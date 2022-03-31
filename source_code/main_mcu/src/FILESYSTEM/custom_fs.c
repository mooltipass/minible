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
/*
 * custom_fs.c
 *
 * Created: 16/05/2017 11:39:30
 *  Author: stephan
 */
#include <stddef.h>
#include <string.h>
#include <asf.h>
#include "mooltipass_graphics_bundle.h"
#include "custom_fs_emergency_font.h"
#include "platform_defines.h"
#include "driver_sercom.h"
#include "logic_device.h"
#include "custom_fs.h"
#include "dataflash.h"
#include "utils.h"
#include "dma.h"
#include "rng.h"

#ifdef EMULATOR_BUILD
#include "emu_storage.h"
#include "emulator.h"
#endif

/* Default device settings */
const uint8_t custom_fs_default_device_settings[NB_DEVICE_SETTINGS] = { 0,                                       // SETTING_RESERVED_ID                
                                                                        FALSE,                                   // SETTING_RANDOM_PIN_ID              
                                                                        SETTING_DFT_USER_INTERACTION_TIMEOUT,    // SETTING_USER_INTERACTION_TIMEOUT_ID
                                                                        TRUE,                                    // SETTING_FLASH_SCREEN_ID            
                                                                        0,                                       // SETTING_DEVICE_DEFAULT_LANGUAGE    
                                                                        0x09,                                    // SETTINGS_CHAR_AFTER_LOGIN_PRESS    
                                                                        0x0A,                                    // SETTINGS_CHAR_AFTER_PASS_PRESS     
                                                                        15,                                      // SETTINGS_DELAY_BETWEEN_PRESSES     
                                                                        TRUE,                                    // SETTINGS_BOOT_ANIMATION            
                                                                        0x90,                                    // SETTINGS_MASTER_CURRENT_USB
                                                                        TRUE,                                    // SETTINGS_LOCK_ON_DISCONNECT        
                                                                        9,                                       // SETTINGS_KNOCK_DETECT_SENSITIVITY  
                                                                        FALSE,                                   // SETTINGS_LEFT_HANDED_ON_BATTERY    
                                                                        FALSE,                                   // SETTINGS_LEFT_HANDED_ON_USB        
                                                                        FALSE,                                   // SETTINGS_PIN_SHOWN_WHEN_BACK       
                                                                        FALSE,                                   // SETTINGS_UNLOCK_FEATURE_PARAM      
                                                                        TRUE,                                    // SETTINGS_DEVICE_TUTORIAL           
                                                                        FALSE,                                   // SETTINGS_SHOW_PIN_ON_ENTRY         
                                                                        FALSE,                                   // SETTINGS_DISABLE_BLE_ON_CARD_REMOVE
                                                                        FALSE,                                   // SETTINGS_DISABLE_BLE_ON_LOCK  
                                                                        0,                                       // SETTINGS_NB_MINUTES_FOR_LOCK 
                                                                        FALSE,                                   // SETTINGS_SWITCH_OFF_AFTER_USB_DISC
                                                                        FALSE,                                   // SETTINGS_HASH_DISPLAY_FEATURE
                                                                        30,                                      // SETTINGS_INFORMATION_TIME_DELAY
                                                                        FALSE,                                   // SETTINGS_BLUETOOTH_SHORTCUTS
                                                                        0,                                       // SETTINGS_SCREEN_SAVER_ID
                                                                        TRUE,                                    // SETTINGS_PREF_ST_SERV_FEATURE
                                                                        FALSE,                                   // SETTINGS_DISP_TOTP_AFTER_RECALL      
                                                                        0x90,                                    // SETTINGS_MASTER_CURRENT_BAT
                                                                        60,                                      // SETTINGS_DELAY_BEF_UNLOCK_LOGIN
                                                                        FALSE,                                   // SETTINGS_SWITCH_OFF_AFTER_BT_DISC
                                                                        FALSE,                                   // SETTINGS_MC_SUBDOMAIN_FORCE_STATUS
                                                                        FALSE};                                  // SETTINGS_FAV_LAST_USED_SORTED
#ifndef EMULATOR_BUILD
/* Pointer to the platform unique data, stored at the last page of our bootloader */
platform_unique_data_t* custom_fs_plat_data_ptr = (platform_unique_data_t*)(FLASH_ADDR + APP_START_ADDR - NVMCTRL_ROW_SIZE);
#endif
/* Current selected language entry */ 
language_map_entry_t custom_fs_cur_language_entry = {.starting_bitmap = 0, .starting_font = 0, .string_file_index = 0};
/* string describing our device */
uint8_t custom_fs_mini_ble_string[MEMBER_SIZE(custom_platform_settings_t,custom_ble_name)] = "Mooltipass Mini BLE";
/* Temp values to speed up string files reading */
custom_fs_string_count_t custom_fs_current_text_file_string_count = 0;
custom_fs_address_t custom_fs_current_text_file_addr = 0;
/* Our platform settings & flags array, in internal NVM */
custom_platform_settings_t* custom_fs_platform_settings_p = 0;
custom_platform_flags_t* custom_fs_platform_flags_p = 0;
/* dataflash port descriptor */
spi_flash_descriptor_t* custom_fs_dataflash_desc = 0;
/* Flash header */
custom_file_flash_header_t custom_fs_flash_header;
/* Bool to specify if the SPI bus is left opened */
BOOL custom_fs_data_bus_opened = FALSE;
/* Temp string buffers for string reading */
BOOL custom_fs_temp_string1_avail = FALSE;
cust_char_t custom_fs_temp_string1[64];
cust_char_t custom_fs_temp_string2[64];
/* Current language id */
uint8_t custom_fs_cur_language_id = 0;
/* Current keyboard layout id */
custom_fs_address_t custom_fs_usb_keyboard_layout_addr = 0;
uint8_t custom_fs_cur_usb_keyboard_id = 0;
custom_fs_address_t custom_fs_ble_keyboard_layout_addr = 0;
uint8_t custom_fs_cur_ble_keyboard_id = 0;
/* CPZ look up table */
cpz_lut_entry_t* custom_fs_cpz_lut;


/*! \fn     custom_fs_get_platform_internal_serial_number(void)
*   \brief  Get the platform serial number
*   \return The serial number
*/
uint32_t custom_fs_get_platform_internal_serial_number(void)
{
#ifndef EMULATOR_BUILD
    return custom_fs_plat_data_ptr->platform_internal_serial_number;
#else
    return UINT32_MAX;
#endif    
}

/*! \fn     custom_fs_get_platform_ble_mac_addr(uint8_t* buffer)
*   \brief  Get the platform mac address
*   \param  buffer  Where to store the 6 bytes mac address
*   \return If the mac address returned is valid
*/
RET_TYPE custom_fs_get_platform_ble_mac_addr(uint8_t* buffer)
{
#ifndef EMULATOR_BUILD
    memcpy(buffer, custom_fs_plat_data_ptr->platform_ble_mac_addr, sizeof(custom_fs_plat_data_ptr->platform_ble_mac_addr));
    if (memcmp(buffer, "\xFF\xFF\xFF\xFF\xFF\xFF", sizeof(custom_fs_plat_data_ptr->platform_ble_mac_addr)) == 0)
    {
        return RETURN_NOK;
    } 
    else
    {
        return RETURN_OK;
    }
#else
    return RETURN_OK;
#endif
}

/*! \fn     custom_fs_get_device_operations_aes_key(uint8_t* buffer)
*   \brief  Get the platform device operations AES key
*   \param  buffer  Where to store the 32 bytes key
*/
void custom_fs_get_device_operations_aes_key(uint8_t* buffer)
{
#ifndef EMULATOR_BUILD
    memcpy(buffer, custom_fs_plat_data_ptr->device_operations_key, sizeof(custom_fs_plat_data_ptr->device_operations_key));
#else
    memset(buffer, 0xFF, AES_KEY_LENGTH/8);
#endif
}

/*! \fn     custom_fs_get_platform_bundle_version(void)
*   \brief  Get the platform bundle version
*   \return The bundle version
*/
uint16_t custom_fs_get_platform_bundle_version(void)
{
#ifndef EMULATOR_BUILD
    return custom_fs_plat_data_ptr->current_bundle_version;
#else
    return UINT16_MAX;
#endif
}

/*! \fn     custom_fs_get_custom_ble_name(void)
*   \brief  Get a pointer to our custom bluetooth name
*   \return Pointer to our string
*/
uint8_t* custom_fs_get_custom_ble_name(void)
{
    /* Check for invalid string */
    for (uint16_t i = 0; i < MEMBER_ARRAY_SIZE(custom_platform_settings_t, custom_ble_name); i++)
    {
        if (custom_fs_platform_settings_p->custom_ble_name[i] == 0)
        {
            return custom_fs_platform_settings_p->custom_ble_name;
        }
        else if ((custom_fs_platform_settings_p->custom_ble_name[i] < ' ') || (custom_fs_platform_settings_p->custom_ble_name[0] > '}'))
        {
            return custom_fs_mini_ble_string;
        }  
    }
    return custom_fs_mini_ble_string;
}

/*! \fn     custom_fs_set_custom_ble_name(uint8_t* name)
*   \brief  Set our custom bluetooth name
*   \param  name    Pointer to a CUSTOM_BLE_NAME_MAX_LENGTH max long string
*/
void custom_fs_set_custom_ble_name(uint8_t* name)
{
    volatile custom_platform_settings_t temp_settings;
    custom_fs_read_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
    memcpy((uint8_t*)temp_settings.custom_ble_name, name, MEMBER_SIZE(custom_platform_settings_t, custom_ble_name)-1);
    temp_settings.custom_ble_name[MEMBER_SIZE(custom_platform_settings_t, custom_ble_name)-1] = 0;
    custom_fs_write_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
}

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
    if ((address >= CUSTOM_FS_EMERGENCY_FONT_FILE_ADDR) && (size <= sizeof(custom_fs_emergency_font_file)) && ((address-CUSTOM_FS_EMERGENCY_FONT_FILE_ADDR) + size <= sizeof(custom_fs_emergency_font_file)))
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

/*! \fn     custom_fs_get_other_data_from_continuous_read_from_flash(uint8_t* datap, uint32_t size, BOOL use_dma)
*   \brief  Get another chunk of data from continuous read from flash
*   \param  datap       Pointer to where to store the data
    \param  size        How many bytes to read
    \param  use_dma     Boolean to specify if we use DMA: if set, function will return before data is transferred!
*/
void custom_fs_get_other_data_from_continuous_read_from_flash(uint8_t* datap, uint32_t size, BOOL use_dma)
{
    /* Check if we have opened the SPI bus */
    if (custom_fs_data_bus_opened == FALSE)
    {
        return;
    }
    
    /* If we are using DMA */
    if (use_dma != FALSE)
    {
        /* Arm DMA transfer */
        dma_custom_fs_init_transfer(custom_fs_dataflash_desc->sercom_pt, (void*)datap, size);
    }
    else
    {
        /* Read data */
        dataflash_read_bytes_from_opened_transfer(custom_fs_dataflash_desc, datap, size);
    }
}

/*! \fn     custom_fs_continuous_read_from_flash(uint8_t* datap, custom_fs_address_t address, uint32_t size, BOOL use_dma)
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
            dma_custom_fs_init_transfer(custom_fs_dataflash_desc->sercom_pt, (void*)datap, size);
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
#ifndef EMULATOR_BUILD
    /* Start a read on external flash */
    dataflash_read_data_array_start(custom_fs_dataflash_desc, CUSTOM_FS_FILES_ADDR_OFFSET + sizeof(custom_fs_flash_header.magic_header) + sizeof(custom_fs_flash_header.total_size) + sizeof(custom_fs_flash_header.crc32));

    /* Reset DMA controller */
    dma_reset();

    /* Use the DMA controller to compute the crc32 */
    uint32_t crc32 = dma_compute_crc32_from_spi(custom_fs_dataflash_desc->sercom_pt, custom_fs_flash_header.total_size - sizeof(custom_fs_flash_header.magic_header) - sizeof(custom_fs_flash_header.total_size) - sizeof(custom_fs_flash_header.crc32));
    
    /* Stop transfer */
    dataflash_stop_ongoing_transfer(custom_fs_dataflash_desc);
    
    /* Reset DMA controller */
    dma_reset();
    
    /* Do the final check */
    if (custom_fs_flash_header.crc32 == crc32)
    {
        return RETURN_OK;
    } 
    else
    {
        return RETURN_NOK;
    }

#else
    /* We don't emulate the DMA controller, and don't bother with reimplementing the crc32 routines */
    return RETURN_OK;
#endif
}

/*! \fn     custom_fs_stop_continuous_read_from_flash(BOOL was_using_emergency_bundle_data)
*   \brief  Stop a continuous flash read
*   \param  was_using_emergency_bundle_data Boolean to inform if we were using emergency bundle data
*/
void custom_fs_stop_continuous_read_from_flash(BOOL was_using_emergency_bundle_data)
{
    if (was_using_emergency_bundle_data == FALSE)
    {
        dataflash_stop_ongoing_transfer(custom_fs_dataflash_desc);
        custom_fs_data_bus_opened = FALSE;
    }         
}

/*! \fn     custom_fs_settings_init(spi_flash_descriptor_t* desc)
*   \brief  Initialize our settings system
*   \return Intialization success state
*/
custom_fs_init_ret_type_te custom_fs_settings_init(void)
{        
    /* Initialize platform settings pointer, located in slot 0 of internal storage */
    void* flash_ptr = custom_fs_get_custom_storage_slot_ptr(SETTINGS_STORAGE_SLOT);
    if (flash_ptr != 0)
    {
        custom_fs_platform_settings_p = (custom_platform_settings_t*)flash_ptr;
        flash_ptr = custom_fs_get_custom_storage_slot_ptr(FIRST_CPZ_LUT_ENTRY_STORAGE_SLOT);
        custom_fs_cpz_lut = (cpz_lut_entry_t*)flash_ptr;
        flash_ptr = custom_fs_get_custom_storage_slot_ptr(FLAGS_STORAGE_SLOT);
        custom_fs_platform_flags_p = (custom_platform_flags_t*)flash_ptr;
        
#ifndef EMULATOR_BUILD
        /* Quick sanity check on memory boundary */
        if ((uintptr_t)&custom_fs_cpz_lut[MAX_NUMBER_OF_USERS] != FLASH_ADDR + FLASH_SIZE)
        {
            while(1);
        }
#endif
        
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

/*! \fn     custom_fs_get_current_layout_id(BOOL usb_layout)
*   \brief  Get current keyboard layout ID
*   \param  usb_layout FALSE for BLE layout, otherwise TRUE
*   \return The current layout ID
*/
uint8_t custom_fs_get_current_layout_id(BOOL usb_layout)
{
    if (usb_layout == FALSE)
    {
        return custom_fs_cur_ble_keyboard_id;
    } 
    else
    {
        return custom_fs_cur_usb_keyboard_id;
    }
}

/*! \fn     custom_fs_get_recommended_layout_for_current_language(void)
*   \brief  Get the recommended keyboard layout id for the language currently set
*   \return The recommended layout ID
*/
uint8_t custom_fs_get_recommended_layout_for_current_language(void)
{
    return (uint8_t)utils_check_value_for_range(custom_fs_cur_language_entry.keyboard_layout_id, 0, custom_fs_get_number_of_keyb_layouts()-1);
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

/*! \fn     custom_fs_set_current_keyboard_id(uint8_t keyboard_id, BOOL usb_layout)
*   \brief  Set current keyboard ID
*   \param  keyboard_id     Keyboard ID
*   \param  usb_layout      Bool for USB/BLE layout
*   \return RETURN_(N)OK
*/
ret_type_te custom_fs_set_current_keyboard_id(uint8_t keyboard_id, BOOL usb_layout)
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
    if (usb_layout == FALSE)
    {
        custom_fs_ble_keyboard_layout_addr = layout_file_addr;
        custom_fs_cur_ble_keyboard_id = keyboard_id;
    } 
    else
    {
        custom_fs_usb_keyboard_layout_addr = layout_file_addr;
        custom_fs_cur_usb_keyboard_id = keyboard_id;
    }
    
    return RETURN_OK;
}

/*! \fn     custom_fs_get_keyboard_symbols_for_unicode_string(cust_char_t* string_pt, uint16_t* buffer, BOOL usb_layout)
*   \brief  Get keyboard symbols (not keys) for a given unicode string
*   \param  string_pt   Pointer to the unicode BMP string
*   \param  buffer      Where to store the symbols. Non supported symbols will be stored as 0xFFFF
*   \param  usb_layout  Set to TRUE to use USB layout mapping, FALSE for BLE layout mapping
*   \return RETURN_(N)OK depending on if we were able to "translate" the complete string
*   \note   Take care of buffer overflows. One symbol will be generated per unicode point
*/
ret_type_te custom_fs_get_keyboard_symbols_for_unicode_string(cust_char_t* string_pt, uint16_t* buffer, BOOL usb_layout)
{
    unicode_interval_desc_t description_intervals[CUSTOM_FS_KEYB_NB_INT_DESCRIBED];
    custom_fs_address_t layout_address = custom_fs_usb_keyboard_layout_addr;
    BOOL point_support_described = FALSE;
    uint16_t symbol_desc_pt_offset = 0;
    BOOL all_points_described = TRUE;
    uint16_t interval_start = 0;
    
    /* Check for correctly setup keyboard layout */
    if ((custom_fs_usb_keyboard_layout_addr == 0) || (custom_fs_ble_keyboard_layout_addr == 0))
    {
        return RETURN_NOK;
    }   
    
    /* Mapping address based on layout selection */
    if (usb_layout == FALSE)
    {
        layout_address = custom_fs_ble_keyboard_layout_addr;
    }
    
    /* Load the description intervals */
    custom_fs_read_from_flash((uint8_t*)description_intervals, layout_address + CUSTOM_FS_KEYBOARD_DESC_LGTH*sizeof(cust_char_t), sizeof(description_intervals));
    
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
            /* Check for tab or return */
            if (*string_pt == 0x09)
            {
                *buffer = KEY_TAB;
            } 
            else if (*string_pt == 0x0A)
            {
                *buffer = KEY_RETURN;
            } 
            else
            {
                all_points_described = FALSE;
                *buffer = 0xFFFF;
            }
        }
        else
        {
            /* Fetch keyboard symbol: 0xFFFF for "not supported" matches with our definition of not described */
            custom_fs_read_from_flash((uint8_t*)buffer, layout_address + CUSTOM_FS_KEYBOARD_DESC_LGTH*sizeof(cust_char_t) + sizeof(description_intervals) + symbol_desc_pt_offset*sizeof(*string_pt) + (*string_pt - interval_start)*sizeof(*string_pt), sizeof(*buffer));       
            
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
    
    /* Check for valid data */
    if (language_map_table_addr == CUSTOM_FS_ADDRESS_TMAX)
    {
        return RETURN_NOK;
    }
    
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

static void custom_fs_init_custom_storage_slots(void);

/*! \fn     custom_fs_get_buffered_flash_header_pt(void)
*   \brief  Get a point to the buffered flash header that was previouly read
*   \return RETURN_(N)OK
*/
custom_file_flash_header_t* custom_fs_get_buffered_flash_header_pt(void)
{
    return &custom_fs_flash_header;
}

/*! \fn     custom_fs_init(void)
*   \brief  Initialize our custom file system... system
*   \return RETURN_(N)OK
*/
ret_type_te custom_fs_init(void)
{
    _Static_assert(sizeof(bl_section_last_row_t) == NVMCTRL_ROW_SIZE, "Platform unique data struct doesn't have the correct size");
    _Static_assert(sizeof(custom_platform_settings_t) == NVMCTRL_ROW_SIZE, "Platform settings isn't a page long");
    _Static_assert(sizeof(custom_platform_flags_t) == NVMCTRL_ROW_SIZE, "Platform flags isn't a page long");

    /* Initialize internal data structures responsible for "custom storage slots".
     * At the moment this doesn't do anything on the regular, non-emulator build. */
    custom_fs_init_custom_storage_slots();
    
    /* Read flash header */
    custom_fs_read_from_flash((uint8_t*)&custom_fs_flash_header, CUSTOM_FS_FILES_ADDR_OFFSET, sizeof(custom_fs_flash_header));
    
    /* Check correct header */
    if (custom_fs_flash_header.magic_header != CUSTOM_FS_MAGIC_HEADER)
    {
        return RETURN_NOK;
    }
    
    /* Check correct payload length */
    if (CUSTOM_FS_FILES_ADDR_OFFSET + custom_fs_flash_header.total_size > W25Q16_FLASH_SIZE)
    {
        return RETURN_NOK;
    }
    
    #ifdef BOOTLOADER
        return RETURN_OK;
    #else
        /* Fetch default language (if set) */
        uint8_t default_device_language = custom_fs_settings_get_device_setting(SETTING_DEVICE_DEFAULT_LANGUAGE);
    
        /* If not valid (preferences not set, etc...) reset to english (0) */
        if (default_device_language >= custom_fs_get_number_of_languages())
        {
            default_device_language = 0;
        }    
    
    /* Set default language */
        return custom_fs_set_current_language(default_device_language);
    #endif
}

/*! \fn     custom_fs_get_start_address_of_signed_data(void)
*   \brief  Get the start address of signed data in the data flash
*   \return The start address
*/
custom_fs_address_t custom_fs_get_start_address_of_signed_data(void)
{
    return START_OF_SIGNED_DATA_IN_DATA_FLASH;
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
    
    /* Round robin available string */
    cust_char_t* temp_string_pointer;
    if (custom_fs_temp_string1_avail == FALSE)
    {
        temp_string_pointer = custom_fs_temp_string2;
        custom_fs_temp_string1_avail = TRUE;
    } 
    else
    {
        temp_string_pointer = custom_fs_temp_string1;
        custom_fs_temp_string1_avail = FALSE;
    }
    
    /* Read string : *2 because of uint16_t used to store chars */
    custom_fs_read_from_flash((uint8_t*)temp_string_pointer, custom_fs_current_text_file_addr + string_offset + sizeof(string_length), string_length*2);
    
    /* Add terminating 0 just in case */
    custom_fs_temp_string1[(sizeof(custom_fs_temp_string1)/sizeof(custom_fs_temp_string1[0]))-1] = 0;
    custom_fs_temp_string2[(sizeof(custom_fs_temp_string1)/sizeof(custom_fs_temp_string1[0]))-1] = 0;
    
    /* Store pointer to string */
    *string_pt = temp_string_pointer;
    
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

#ifndef EMULATOR_BUILD

/*! \fn     custom_fs_init_custom_storage_slots(void)
*   \brief  Initialize custom slots storage
*/
static void custom_fs_init_custom_storage_slots(void)
{
    /* this is non-empty in the emulator version */
}

/*! \fn     custom_fs_get_custom_storage_slot_ptr(uint32_t slot_id)
*   \brief  Get the internal flash address for a given storage slot id
*   \param  slot_id     slot ID
*   \return 0 if the slot_id is not valid, otherwise the address
*/
void* custom_fs_get_custom_storage_slot_ptr(uint32_t slot_id)
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
    return (void*)(FLASH_ADDR + FLASH_SIZE - emulated_eeprom_size + slot_id*NVMCTRL_ROW_SIZE);
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
    void *flash_ptr = custom_fs_get_custom_storage_slot_ptr(slot_id);
    uint32_t flash_addr = (uint32_t)flash_ptr;
    
    /* Check if we were successful */
    if (flash_addr == 0)
    {
        return;
    }
    
    /* Disable interrupts */
    cpu_irq_enter_critical();
    
    /* Automatic write, disable caching */
    NVMCTRL->CTRLB.bit.MANW = 0;
    NVMCTRL->CTRLB.bit.CACHEDIS = 1;
    
    /* Erase complete row */
    while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
    NVMCTRL->ADDR.reg  = flash_addr/2;
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;

    /* Wait for erase */
    while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
    
    /* Disable automatic write, enable caching */
    NVMCTRL->CTRLB.bit.MANW = 1;
    NVMCTRL->CTRLB.bit.CACHEDIS = 0;
    
    /* Re-enable interrupts */
    cpu_irq_leave_critical();
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
    void *flash_ptr = custom_fs_get_custom_storage_slot_ptr(slot_id);
    uint32_t flash_addr = (uint32_t)flash_ptr;
    
    /* Check if we were successful */
    if (flash_addr == 0)
    {
        return;
    }
    
    /* Disable interrupts */
    cpu_irq_enter_critical();
    
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

    /* Final wait */
    while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);

    /* Disable automatic write, enable caching */
    NVMCTRL->CTRLB.bit.MANW = 1;
    NVMCTRL->CTRLB.bit.CACHEDIS = 0;
    __DSB();
    
    /* Re-enable interrupts */
    cpu_irq_leave_critical();
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
    void* flash_addr = custom_fs_get_custom_storage_slot_ptr(slot_id);
    
    /* Check if we were successful */
    if (flash_addr == 0)
    {
        return;
    }
    
    /* Perform the copy */
    memcpy(array, (const void*)flash_addr, NVMCTRL_ROW_SIZE);
#endif
}

#else

/* This part is used on the emulator build */

static uint8_t eeprom[256 * 128];

static void custom_fs_init_custom_storage_slots(void)
{
    if(!emu_eeprom_open())
        custom_fs_hard_reset_settings();

    emu_eeprom_read(0, eeprom, sizeof(eeprom));
}

void* custom_fs_get_custom_storage_slot_ptr(uint32_t slot_id)
{
    if(slot_id * 256 > sizeof(eeprom))
        return 0;

    return &eeprom[slot_id * 256];
}

void custom_fs_erase_256B_at_internal_custom_storage_slot(uint32_t slot_id) 
{
    if(slot_id * 256 > sizeof(eeprom))
        return;

    memset(eeprom + slot_id * 256, 0xff, 256);
    emu_eeprom_write(slot_id * 256, eeprom + slot_id * 256, 256);
}

void custom_fs_write_256B_at_internal_custom_storage_slot(uint32_t slot_id, void* array)
{
    if(slot_id * 256 > sizeof(eeprom))
        return;

    memcpy(eeprom+slot_id*256, array, 256);
    emu_eeprom_write(slot_id * 256, eeprom + slot_id * 256, 256);
}

void custom_fs_read_256B_at_internal_custom_storage_slot(uint32_t slot_id, void* array)
{
    if(slot_id * 256 > sizeof(eeprom))
        return;

    memcpy(array, eeprom + slot_id * 256, 256);
}

#endif

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
    volatile custom_platform_flags_t temp_flags;
    
    /* Check for overflow and custom fs init state */
    if ((custom_fs_platform_flags_p != 0) && (flag_id < ARRAY_SIZE(custom_fs_platform_flags_p->device_flags)))
    {
        custom_fs_read_256B_at_internal_custom_storage_slot(FLAGS_STORAGE_SLOT, (void*)&temp_flags);
        if (value == FALSE)
        {
            temp_flags.device_flags[flag_id] = 0xFFFF;
        } 
        else
        {
            temp_flags.device_flags[flag_id] = FLAG_SET_BOOL_VALUE;
        }
        custom_fs_write_256B_at_internal_custom_storage_slot(FLAGS_STORAGE_SLOT, (void*)&temp_flags);
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
    if ((custom_fs_platform_flags_p != 0) && (flag_id < ARRAY_SIZE(custom_fs_platform_flags_p->device_flags)))
    {
        if (custom_fs_platform_flags_p->device_flags[flag_id] == FLAG_SET_BOOL_VALUE)
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

/*! \fn     custom_fs_store_power_consumption_log_and_calib_data(power_consumption_log_t* power_log_pt, time_calibration_data_t* time_calib_data_pt)
*   \brief  Store the power consumption log into flash
*   \param  power_log   Pointer to the power consumption log
*/
void custom_fs_store_power_consumption_log_and_calib_data(power_consumption_log_t* power_log_pt, time_calibration_data_t* time_calib_data_pt)
{
    volatile custom_platform_settings_t temp_settings;
    custom_fs_read_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
    memcpy((void*)&temp_settings.power_log, power_log_pt, sizeof(power_consumption_log_t));
    memcpy((void*)&temp_settings.time_calib, time_calib_data_pt, sizeof(time_calibration_data_t));
    custom_fs_write_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
}

/*! \fn     custom_fs_get_time_calibration_data(time_calibration_data_t* time_calib_data_pt)
*   \brief  Get the time calibration data from internal flash (at FFFF... at first boot)
*   \param  time_calib_data_pt  Pointer to where to store the time calibration data
*/
void custom_fs_get_time_calibration_data(time_calibration_data_t* time_calib_data_pt)
{
    if (custom_fs_platform_settings_p != 0)
    {
        memcpy(time_calib_data_pt, &custom_fs_platform_settings_p->time_calib, sizeof(time_calibration_data_t));
    }
}

/*! \fn     custom_fs_get_power_consumption_log(power_consumption_log_t* power_log_pt)
*   \brief  Get the power consumption log from internal flash (at FFFF... at first boot)
*   \param  power_log   Pointer to where to store the power consumption log
*/
void custom_fs_get_power_consumption_log(power_consumption_log_t* power_log_pt)
{
    if (custom_fs_platform_settings_p != 0)
    {
        memcpy(power_log_pt, &custom_fs_platform_settings_p->power_log, sizeof(power_consumption_log_t));
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
        /* Shouldn't happen, except when displaying "no bundle" */
        memset(dump_buffer, 0x00, sizeof(custom_fs_platform_settings_p->device_settings));
        memcpy(dump_buffer, custom_fs_default_device_settings, sizeof(custom_fs_platform_settings_p->device_settings));
        return sizeof(custom_fs_platform_settings_p->device_settings);
    }
    else
    {
        memcpy(dump_buffer, custom_fs_platform_settings_p->device_settings, sizeof(custom_fs_platform_settings_p->device_settings));
        return sizeof(custom_fs_platform_settings_p->device_settings);
    }
}

/*! \fn     custom_fs_set_auth_challenge_counter(uint32_t counter_value)
*   \brief  Set new authentication counter challenge value
*   \counter_value  New counter value to store
*/
void custom_fs_set_auth_challenge_counter(uint32_t counter_value)
{
    volatile custom_platform_settings_t temp_settings;
    custom_fs_read_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
    temp_settings.device_auth_challenge_counter = counter_value;
    custom_fs_write_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);    
}

/*! \fn     custom_fs_get_auth_challenge_counter(void)
*   \brief  Get device authentication challenge counter
*   \return Authentication challenge counter
*/
uint32_t custom_fs_get_auth_challenge_counter(void)
{
    if (custom_fs_platform_settings_p != 0)
    {
        return custom_fs_platform_settings_p->device_auth_challenge_counter;
    }
    else
    {
        return 0;
    }        
}

/*! \fn     custom_fs_program_serial_number(uint32_t serial_number)
*   \brief  Set device soft serial number (not the internal one!)
*   \serial_number  Serial number to store
*/
void custom_fs_program_serial_number(uint32_t serial_number)
{
    volatile custom_platform_settings_t temp_settings;
    custom_fs_read_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);
    temp_settings.platform_serial_number = serial_number;
    custom_fs_write_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&temp_settings);    
}

/*! \fn     custom_fs_get_platform_programmed_serial_number(void)
*   \brief  Get device programmed serial number (not the internal one!)
*   \return The device programmed SN
*/
uint32_t custom_fs_get_platform_programmed_serial_number(void)
{
    if (custom_fs_platform_settings_p != 0)
    {
        return custom_fs_platform_settings_p->platform_serial_number;
    }
    else
    {
        return 0;
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
        platform_settings_copy.nb_settings_last_covered = SETTINGS_NB_USED;
        custom_fs_write_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&platform_settings_copy);
    }
}

/*! \fn     custom_fs_hard_reset_settings(void)
*   \brief  Force reset all settings
*/
void custom_fs_hard_reset_settings(void)
{
    custom_fs_settings_store_dump((uint8_t*)custom_fs_default_device_settings);
}

/*! \fn     custom_fs_set_device_default_language(uint8_t language_id)
*   \brief  Set device default language
*   \param  language_id The default language id
*/
void custom_fs_set_device_default_language(uint8_t language_id)
{
    uint8_t temp_device_settings[NB_DEVICE_SETTINGS];
    memcpy(temp_device_settings, custom_fs_platform_settings_p->device_settings, sizeof(temp_device_settings));
    temp_device_settings[SETTING_DEVICE_DEFAULT_LANGUAGE] = language_id;
    custom_fs_settings_store_dump(temp_device_settings);    
}

/*! \fn     custom_fs_set_settings_value(uint8_t settings_id, uint8_t setting_value)
*   \brief  Set a given settings ID to a given value
*   \param  settings_id     The settings ID
*   \param  setting_value   The settings value
*/
void custom_fs_set_settings_value(uint8_t settings_id, uint8_t setting_value)
{
    /* Check for invalid settings ID */
    if (settings_id < NB_DEVICE_SETTINGS)
    {
        uint8_t temp_device_settings[NB_DEVICE_SETTINGS];
        memcpy(temp_device_settings, custom_fs_platform_settings_p->device_settings, sizeof(temp_device_settings));
        temp_device_settings[settings_id] = setting_value;
        custom_fs_settings_store_dump(temp_device_settings);
    }
}

/*! \fn     custom_fs_get_debug_bt_addr(uint8_t* bt_addr)
*   \brief  Get debug bluetooth address
*   \param  bt_addr Pointer to where to store the debug bluetooth address
*/
void custom_fs_get_debug_bt_addr(uint8_t* bt_addr)
{
    memcpy(bt_addr, custom_fs_platform_settings_p->dbg_bluetooth_addr, sizeof(custom_fs_platform_settings_p->dbg_bluetooth_addr));
}

/*! \fn     custom_fs_set_undefined_settings(BOOL force_flash)
*   \brief  Set settings that may not have been set to a default value
*   \param  force_flash Set to TRUE to force settings flash
*/
void custom_fs_set_undefined_settings(BOOL force_flash)
{
    volatile custom_platform_settings_t platform_settings_copy;
    
    if (custom_fs_platform_settings_p == 0)
    {
        /* Customfs not initialized (shouldn't happen) */
        return;
    }
    else if ((custom_fs_platform_settings_p->nb_settings_last_covered != SETTINGS_NB_USED) || (force_flash != FALSE))
    {        
        /* Copy settings structure stored in flash into ram, overwrite relevant settings part, flash again later */
        memcpy((void*)&platform_settings_copy, custom_fs_platform_settings_p, sizeof(platform_settings_copy));

        /* Check for blank memory & overflow */
        if ((custom_fs_platform_settings_p->nb_settings_last_covered >= NB_DEVICE_SETTINGS) || (force_flash != FALSE))
        {
            platform_settings_copy.nb_settings_last_covered = 0;
        }
        
        /* Only update the non defined settings */
        memcpy((void*)&platform_settings_copy.device_settings[platform_settings_copy.nb_settings_last_covered], &custom_fs_default_device_settings[platform_settings_copy.nb_settings_last_covered], NB_DEVICE_SETTINGS-platform_settings_copy.nb_settings_last_covered);
        
        /* Set number of settings covered */
        platform_settings_copy.nb_settings_last_covered = SETTINGS_NB_USED;
        
        /* Generate random mac address for debug purposes */
        if ((memcmp(custom_fs_platform_settings_p->dbg_bluetooth_addr, "\xFF\xFF\xFF\xFF\xFF\xFF", sizeof(custom_fs_platform_settings_p->dbg_bluetooth_addr)) == 0) || ((custom_fs_platform_settings_p->dbg_bluetooth_addr[5] & 0xC0) != 0xC0))
        {
            rng_fill_array((uint8_t*)platform_settings_copy.dbg_bluetooth_addr, sizeof(platform_settings_copy.dbg_bluetooth_addr));
            
            /* 2 MSBit must be '11' for RANDOM_STATIC address. */
            platform_settings_copy.dbg_bluetooth_addr[5] |= 0xC0;
        }
        
        /* Update memory */
        custom_fs_write_256B_at_internal_custom_storage_slot(SETTINGS_STORAGE_SLOT, (void*)&platform_settings_copy);
    }
}

/*! \fn     custom_fs_get_user_id_for_cpz(uint8_t* cpz, uint8_t* user_id)
*   \brief  Get the user id for a given a CPZ
*   \param  cpz         CPZ bytes
*   \param  user_id     Where to store the user id
*   \return Success status
*/
RET_TYPE custom_fs_get_user_id_for_cpz(uint8_t* cpz, uint8_t* user_id)
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
                *user_id = custom_fs_cpz_lut[i].user_id;
                return RETURN_OK;
            }
        }
    }
    
    return RETURN_NOK;
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

#ifdef EMULATOR_BUILD
    if(emu_get_failure_flags() & EMU_FAIL_EEPROM_FULL)
        return 0;
#endif
    
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
