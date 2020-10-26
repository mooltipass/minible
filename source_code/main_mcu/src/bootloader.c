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
#include <asf.h>
#include "mooltipass_graphics_bundle.h"
#include "smartcard_highlevel.h"
#include "smartcard_lowlevel.h"
#include "platform_defines.h"
#include "bearssl_block.h"
#include "driver_clocks.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "bearssl_ec.h"
#include "custom_fs.h"
#include "dataflash.h"
#include "lis2hh12.h"
#include "dbflash.h"
#include "defines.h"
#include "sh1122.h"
#include "inputs.h"
#include "utils.h"
#include "fuses.h"
#include "main.h"
#include "dma.h"
/* Our oled & dataflash & dbflash descriptors */
sh1122_descriptor_t plat_oled_descriptor = {.sercom_pt = OLED_SERCOM, .dma_trigger_id = OLED_DMA_SERCOM_TX_TRIG, .sh1122_cs_pin_group = OLED_nCS_GROUP, .sh1122_cs_pin_mask = OLED_nCS_MASK, .sh1122_cd_pin_group = OLED_CD_GROUP, .sh1122_cd_pin_mask = OLED_CD_MASK};
spi_flash_descriptor_t dataflash_descriptor = {.sercom_pt = DATAFLASH_SERCOM, .cs_pin_group = DATAFLASH_nCS_GROUP, .cs_pin_mask = DATAFLASH_nCS_MASK};
spi_flash_descriptor_t dbflash_descriptor = {.sercom_pt = DBFLASH_SERCOM, .cs_pin_group = DBFLASH_nCS_GROUP, .cs_pin_mask = DBFLASH_nCS_MASK};
/* Pointer to the platform unique data, stored at the last page of our bootloader */
bl_section_last_row_t* bl_last_row_ptr = (bl_section_last_row_t*)(FLASH_ADDR + APP_START_ADDR - NVMCTRL_ROW_SIZE);
/* Big buffers */
#define BUNDLE_RX_TEMP_BUFFER_SIZE  8192                                        // Size of receive buffer
uint16_t bundle_data_b1[BUNDLE_RX_TEMP_BUFFER_SIZE/2];                          // First buffer for bundle data
uint16_t bundle_data_b2[BUNDLE_RX_TEMP_BUFFER_SIZE/2];                          // Second buffer for bundle data

/**
 * \brief Function to start the application.
 */
static void start_application(void)
{
    /* Pointer to the Application Section */
    void (*application_code_entry)(void);

    /* Rebase the Stack Pointer */
    __set_MSP(*(uint32_t *)APP_START_ADDR);

    /* Rebase the vector table base address */
    SCB->VTOR = ((uint32_t)APP_START_ADDR & SCB_VTOR_TBLOFF_Msk);

    /* Load the Reset Handler address of the application */
    application_code_entry = (void (*)(void))(unsigned *)(*(unsigned *)(APP_START_ADDR + 4));

    /* Jump to user Reset Handler in the application */
    application_code_entry();
}

/*! \fn     brick_main_mcu(void)
*   \brief  Nice way to brick the main MCU
*/
static void brick_main_mcu(void)
{
    /* Automatic flash write, disable caching */
    NVMCTRL->CTRLB.bit.MANW = 0;
    NVMCTRL->CTRLB.bit.CACHEDIS = 1;

    /* Erase all pages of internal memory */
    for (uint32_t current_flash_address = APP_START_ADDR; current_flash_address < FLASH_SIZE; current_flash_address += NVMCTRL_ROW_SIZE)
    {
        /* Erase complete row, composed of 4 pages */
        while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
        NVMCTRL->ADDR.reg  = current_flash_address/2;
        NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;
    }
}

#if defined(PLAT_V7_SETUP)
/*! \fn     brick_main_mcu_disp_error_switch_off(BOOL disp_error)
*   \brief  Brick the main mcu, display an error message and switch off
*   \param  disp_error  If we should display an error message
*/
static void brick_main_mcu_disp_error_switch_off(BOOL disp_error)
{
    brick_main_mcu();
    #if defined(PLAT_V7_SETUP)
    /* Screen debug */
    if (disp_error != FALSE)
    {
        sh1122_erase_screen_and_put_top_left_emergency_string(&plat_oled_descriptor, u"Update error!");
        DELAYMS(5000);
    }
    #endif
    platform_io_disable_switch_and_die();
    while(1);    
}

/*! \fn     perform_end_of_flash_operations(custom_file_flash_header_t* buffered_flash_header_ptr)
*   \brief  Perform the end of flash operations
*   \param  buffered_flash_header_ptr Pointer to the buffered flash header
*/
static void perform_end_of_flash_operations(custom_file_flash_header_t* buffered_flash_header_ptr)
{
    uint8_t temp_ctr[AES256_CTR_LENGTH/8];
    uint8_t encryption_aes_key[AES_KEY_LENGTH/8];
    br_aes_ct_ctrcbc_keys bootloader_encryption_aes_context;
    memcpy(encryption_aes_key, bl_last_row_ptr->platform_unique_data.bundle_signing_key, sizeof(encryption_aes_key));
    
    /* Copy of our platform unique data in RAM */
    bl_section_last_row_t bl_section_last_row_to_flash;
    memcpy(&bl_section_last_row_to_flash, bl_last_row_ptr, sizeof(bl_section_last_row_to_flash));
    
    /* Update bundle version */
    bl_section_last_row_to_flash.platform_unique_data.current_bundle_version = buffered_flash_header_ptr->bundle_version;
    
    /* Should we update the signing key? */
    if (buffered_flash_header_ptr->signing_key_update_bool != FALSE)
    {
        memset(temp_ctr, 0, sizeof(temp_ctr));
        br_aes_ct_ctrcbc_init(&bootloader_encryption_aes_context, encryption_aes_key, AES_KEY_LENGTH/8);
        br_aes_ct_ctrcbc_ctr(&bootloader_encryption_aes_context, (void*)temp_ctr, (void*)buffered_flash_header_ptr->encrypted_new_signing_key, sizeof(buffered_flash_header_ptr->encrypted_new_signing_key));
        memcpy(bl_section_last_row_to_flash.platform_unique_data.bundle_signing_key, buffered_flash_header_ptr->encrypted_new_signing_key, sizeof(buffered_flash_header_ptr->encrypted_new_signing_key));
    }
    
    /* Update the platform data */
    while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
    NVMCTRL->ADDR.reg  = ((uint32_t)bl_last_row_ptr)/2;
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;
    
    /* Flash bytes */
    for (uint32_t j = 0; j < 4; j++)
    {
        /* Flash 4 consecutive pages */
        while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
        for(uint32_t i = 0; i < NVMCTRL_ROW_SIZE/4; i+=2)
        {
            NVM_MEMORY[(((uint32_t)bl_last_row_ptr)+j*(NVMCTRL_ROW_SIZE/4)+i)/2] = bl_section_last_row_to_flash.row_data[(j*(NVMCTRL_ROW_SIZE/4)+i)/2];
        }
    }
}
#endif

/*! \fn     main(void)
*   \brief  Program Main
*/
int main(void)
{
    custom_fs_address_t current_data_flash_addr;                                    // Current data flash address
    uint16_t* available_data_buffer;                                                // Available buffer to receive data
    uint16_t* received_data_buffer;                                                 // Buffer in which we received data

#if defined(PLAT_V7_SETUP)
    /* Mass production units get signed firmware updates */
    #define NUMBER_OF_INT_MACS      (FLASH_SIZE/BUNDLE_RX_TEMP_BUFFER_SIZE)         // Number of intermediary MACs for fw update file checks (more than enough as the bootloader is already taking space)
    br_aes_ct_ctrcbc_keys bootloader_signing_aes_context;                           // The AES signing context
    uint8_t int_mcu_fw_macs[NUMBER_OF_INT_MACS][16];                                // The intermediary MACs
    uint16_t current_intermerdiary_mac_slot = 0;                                    // Current slot to fill
    uint8_t signing_aes_key[AES_KEY_LENGTH/8];                                      // AES signing key
    uint8_t cur_cbc_mac[16];                                                        // Currently computed CBCMAC val
    
    /* Sanity checks */
    _Static_assert((W25Q16_FLASH_SIZE-START_OF_SIGNED_DATA_IN_DATA_FLASH) % 16 == 0, "CBCMAC address space isn't a multiple of block size");
    _Static_assert(sizeof(bl_section_last_row_t) == NVMCTRL_ROW_SIZE, "Platform unique data struct doesn't have the correct size");
    _Static_assert(sizeof(bundle_data_b2) % 16 == 0, "Bundle buffer size is not a multiple of block size");
    _Static_assert(sizeof(bundle_data_b1) % 16 == 0, "Bundle buffer size is not a multiple of block size");
    _Static_assert(sizeof(cur_cbc_mac) == 16, "Invalid MAC buffer size");
#endif
    
    /* Enable switch and 3V3 stepup, set no comms signal, leave some time for stepup powerup */
    platform_io_enable_switch();
    platform_io_init_no_comms_signal();
    DELAYMS_8M(100);
    
    /* Fuses not programmed, start application who will check them anyway */
    if (fuses_check_program(FALSE) != RETURN_OK)
    {
        start_application();
    }
    
    /* Initialize our settings system: should not returned failed as fuses are programmed for rwee */
    if (custom_fs_settings_init() != CUSTOM_FS_INIT_OK)
    {
        platform_io_disable_switch_and_die();
        while(1);
    }
    
    /* If no upgrade flag set, jump to application */
    if (custom_fs_settings_check_fw_upgrade_flag() == FALSE)
    {
        start_application();
    }
    
    /* Store the dataflash descriptor for our custom fs library */
    custom_fs_set_dataflash_descriptor(&dataflash_descriptor);
    
    /* Change the MCU main clock to 48MHz */
    clocks_start_48MDFLL();
    
    /* Initialize flash io ports */
    platform_io_init_flash_ports();
    
    /* Check for external flash presence */
    if (dataflash_check_presence(&dataflash_descriptor) == RETURN_NOK)
    {
        custom_fs_settings_clear_fw_upgrade_flag();
        NVIC_SystemReset();
    }
    
    /* Custom file system initialization */
    if (custom_fs_init() != RETURN_OK)
    {
        custom_fs_settings_clear_fw_upgrade_flag();
        NVIC_SystemReset();     
    }
    
    /* Get a pointer to the buffer bundle header */
    #if defined(PLAT_V7_SETUP)
    custom_file_flash_header_t* buffered_flash_header = custom_fs_get_buffered_flash_header_pt();
    #endif
    
    /* Look for update file address */
    custom_fs_address_t fw_file_address;
    custom_fs_binfile_size_t fw_file_size;
    if (custom_fs_get_file_address(0, &fw_file_address, CUSTOM_FS_FW_UPDATE_TYPE) == RETURN_NOK)
    {
        /* If we couldn't find the update file */
        custom_fs_settings_clear_fw_upgrade_flag();
        NVIC_SystemReset();
    }
    
    /* Read file size */
    custom_fs_read_from_flash((uint8_t*)&fw_file_size, fw_file_address, sizeof(fw_file_size));
    fw_file_address += sizeof(fw_file_size);   
    
    /* Check CRC32 */
    if (custom_fs_compute_and_check_external_bundle_crc32() == RETURN_NOK)
    {
        /* Wrong CRC32 : invalid bundle, start application which will also detect it */
        custom_fs_settings_clear_fw_upgrade_flag();
        NVIC_SystemReset();
    }

    /* Setup DMA controller for data flash transfers */
    dma_init();
    
    #if defined(PLAT_V7_SETUP)
    /* Initialize OLED ports & power ports, used for OLED PSU */
    platform_io_init_power_ports();
    platform_io_init_oled_ports();
    
    /* Initialize the OLED only if 3V3 is present */
    BOOL is_usb_power_present_at_boot = platform_io_is_usb_3v3_present_raw();
    if (is_usb_power_present_at_boot != FALSE)
    {
        platform_io_power_up_oled(TRUE);
        sh1122_init_display(&plat_oled_descriptor, FALSE);
    }
    #endif

    #if defined(PLAT_V7_SETUP)
    /* Fetch bundle signing key */
    memcpy(signing_aes_key, bl_last_row_ptr->platform_unique_data.bundle_signing_key, sizeof(signing_aes_key));
    #endif
    
    /* Automatic flash write, disable caching */
    NVMCTRL->CTRLB.bit.MANW = 0;
    NVMCTRL->CTRLB.bit.CACHEDIS = 1;

    /* Two passes: one to check the signature, one to flash and check previously stored signature for fw file */
    for (uint16_t nb_pass = 0; nb_pass < 2; nb_pass++)
    {
        #if defined(PLAT_V7_SETUP)
        /* Initialize encryption context, set IV to 0 */
        br_aes_ct_ctrcbc_init(&bootloader_signing_aes_context, signing_aes_key, AES_KEY_LENGTH/8);
        memset((void*)cur_cbc_mac, 0x00, sizeof(cur_cbc_mac));
        #endif
        
        #if defined(PLAT_V7_SETUP)
        /* Screen debug */
        if ((is_usb_power_present_at_boot != FALSE) && (platform_io_is_usb_3v3_present_raw() != FALSE))
        {
            if (nb_pass == 0)
                sh1122_erase_screen_and_put_top_left_emergency_string(&plat_oled_descriptor, u"Bundle check");
            else
                sh1122_erase_screen_and_put_top_left_emergency_string(&plat_oled_descriptor, u"Flashing MCU");
        }
        #endif

        /* Set current dataflash address at the beginning of the signed data */
        current_data_flash_addr = custom_fs_get_start_address_of_signed_data();

        /* Booleans to know if we are in the right address space to fetch firmware data */
        uint32_t address_in_mcu_memory = APP_START_ADDR;
        uint32_t nb_bytes_written_in_mcu_memory = 0;
        BOOL address_passed_for_fw_data = FALSE;
        BOOL address_valid_for_fw_data = FALSE;
        #if defined(PLAT_V7_SETUP)
        current_intermerdiary_mac_slot = 0;
        #endif

        /* Arm first DMA transfer */
        custom_fs_continuous_read_from_flash((uint8_t*)bundle_data_b1, current_data_flash_addr, sizeof(bundle_data_b1), TRUE);
        available_data_buffer = bundle_data_b2;
        received_data_buffer = bundle_data_b1;

        /* CBCMAC the complete memory */
        while (current_data_flash_addr < W25Q16_FLASH_SIZE)
        {
            #if defined(PLAT_V7_SETUP)
            /* Screen debug */
            if ((is_usb_power_present_at_boot != FALSE) && (platform_io_is_usb_3v3_present_raw() != FALSE))
            {
                sh1122_add_emergency_dot_to_current_position(&plat_oled_descriptor);
            }
            #endif
            
            /* Compute number of bytes to read */
            uint32_t nb_bytes_to_read = W25Q16_FLASH_SIZE - current_data_flash_addr;
            if (nb_bytes_to_read > sizeof(bundle_data_b1))
            {
                nb_bytes_to_read = sizeof(bundle_data_b1);
            }

            /* Wait for received DMA transfer */
            while(dma_custom_fs_check_and_clear_dma_transfer_flag() == FALSE);

            /* Arm next DMA transfer */
            if (available_data_buffer == bundle_data_b1)
            {
                custom_fs_get_other_data_from_continuous_read_from_flash((uint8_t*)bundle_data_b1, sizeof(bundle_data_b1), TRUE);
            } 
            else
            {
                custom_fs_get_other_data_from_continuous_read_from_flash((uint8_t*)bundle_data_b2, sizeof(bundle_data_b2), TRUE);
            }

            #if defined(PLAT_V7_SETUP)
            /* CBCMAC the crap out of it */
            br_aes_ct_ctrcbc_mac(&bootloader_signing_aes_context, cur_cbc_mac, received_data_buffer, nb_bytes_to_read);
            
            /* End of flash cbcmac */
            if ((nb_pass == 0) && ((current_data_flash_addr + nb_bytes_to_read) == W25Q16_FLASH_SIZE))
            {
                /* Check for correct CBCMAC */
                if (utils_side_channel_safe_memcmp(buffered_flash_header->signed_hash, cur_cbc_mac, sizeof(cur_cbc_mac)) != 0)
                {
                    if ((is_usb_power_present_at_boot != FALSE) && (platform_io_is_usb_3v3_present_raw() != FALSE))
                    {
                        sh1122_erase_screen_and_put_top_left_emergency_string(&plat_oled_descriptor, u"Corrupted Bundle!");
                    }
                    dataflash_bulk_erase_with_wait(&dataflash_descriptor);
                    custom_fs_settings_clear_fw_upgrade_flag();
                    NVIC_SystemReset();
                }
                
                /* Check for bundle version superior to the one we have */
                if ((bl_last_row_ptr->platform_unique_data.current_bundle_version != 0xFFFF) && (buffered_flash_header->bundle_version < bl_last_row_ptr->platform_unique_data.current_bundle_version))
                {
                    custom_fs_settings_clear_fw_upgrade_flag();
                    NVIC_SystemReset();
                }
            }
            #endif

            /* Where the fw data is valid inside our read buffer */
            uint32_t valid_fw_data_offset = 0;

            /* Check if we are in the right address space for fw data */
            if ((address_valid_for_fw_data == FALSE) && (address_passed_for_fw_data == FALSE) && ((current_data_flash_addr + nb_bytes_to_read) > fw_file_address))
            {
                /* Our firmware is longer than 8k, we should be good ;) */
                address_valid_for_fw_data = TRUE;

                /* Compute the fw data offset */
                valid_fw_data_offset = fw_file_address - current_data_flash_addr;
            }
            
            #if defined(PLAT_V7_SETUP)
            /* First pass: store intermediary MACs */
            if ((nb_pass == 0) && (address_valid_for_fw_data != FALSE))
            {
                if (current_intermerdiary_mac_slot >= NUMBER_OF_INT_MACS)
                {
                    /* Somehow we need more intermediary MACs than we budgeted? */
                    brick_main_mcu_disp_error_switch_off(((is_usb_power_present_at_boot != FALSE) && (platform_io_is_usb_3v3_present_raw() != FALSE))?TRUE:FALSE);
                } 
                else
                {
                    /* Store intermediary MAC */
                    memcpy(int_mcu_fw_macs[current_intermerdiary_mac_slot++], cur_cbc_mac, sizeof(cur_cbc_mac));
                }
                
                /* Check if we just passed the end of fw data */
                if ((address_valid_for_fw_data != FALSE) && ((current_data_flash_addr + nb_bytes_to_read) > (fw_file_address + fw_file_size)))
                {
                    address_passed_for_fw_data = TRUE;
                    address_valid_for_fw_data = FALSE;
                }                    
            }
            #endif
            
            /* Second pass: flash stuff */
            if ((nb_pass == 1) && (address_valid_for_fw_data != FALSE))
            {
                /* Check the CBCMAC from the previous pass matches the current one's */
                #if defined(PLAT_V7_SETUP)
                if (utils_side_channel_safe_memcmp(int_mcu_fw_macs[current_intermerdiary_mac_slot++], cur_cbc_mac, sizeof(cur_cbc_mac)) != 0)
                {
                    /* Somehow the CBCMAC changed between both passes, which can only be explained by a malicious attempt */
                    brick_main_mcu_disp_error_switch_off(((is_usb_power_present_at_boot != FALSE) && (platform_io_is_usb_3v3_present_raw() != FALSE))?TRUE:FALSE);
                }
                #endif
                
                 /* Keep in mind we are 16 bytes aligned here */
                 for (uint32_t i = valid_fw_data_offset/2; i < nb_bytes_to_read/2; i++)
                 {
                     /* Erase row? */
                     if (address_in_mcu_memory % NVMCTRL_ROW_SIZE == 0)
                     {
                         /* Erase complete row, composed of 4 pages */
                         while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
                         NVMCTRL->ADDR.reg  = address_in_mcu_memory/2;
                         NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;
                     }

                     /* Wait for flash? */
                     if (address_in_mcu_memory % (NVMCTRL_ROW_SIZE/4) == 0)
                     {
                         while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
                     }

                     /* Write the data, finally. */
                     NVM_MEMORY[address_in_mcu_memory/2] = received_data_buffer[i];

                     /* Increment address counter */
                     nb_bytes_written_in_mcu_memory += 2;
                     address_in_mcu_memory += 2;

                     /* Check for end of fw file... */
                     if (nb_bytes_written_in_mcu_memory >= fw_file_size)
                     {
                         /* Set booleans */
                         address_passed_for_fw_data = TRUE;
                         address_valid_for_fw_data = FALSE;
                         
                         /* Perform final operations (aes key update, bundle version store...) */
                         #if defined(PLAT_V7_SETUP)
                         perform_end_of_flash_operations(buffered_flash_header);
                         #endif
                                                  
                         /* Set success flag */
                         custom_fs_set_device_flag_value(SUCCESSFUL_UPDATE_FLAG_ID, TRUE);

                         /* Do not perform CBC mac until end of bundle */
                         current_data_flash_addr = W25Q16_FLASH_SIZE;
                         break;
                     }
                 }
            }

            /* Increment scan address */
            current_data_flash_addr += nb_bytes_to_read;

            /* Set correct buffer pointers, DMA transfers were already triggered */
            if (available_data_buffer == bundle_data_b1)
            {
                available_data_buffer = bundle_data_b2;
                received_data_buffer = bundle_data_b1;
            }
            else
            {
                available_data_buffer = bundle_data_b1;
                received_data_buffer = bundle_data_b2;
            }
        }

        /* End of pass */
        while(dma_custom_fs_check_and_clear_dma_transfer_flag() == FALSE);
        custom_fs_stop_continuous_read_from_flash(FALSE);
    }
    
    #if defined(PLAT_V7_SETUP)    
    /* Switch off OLED */
    if (is_usb_power_present_at_boot != FALSE)
    {
        sh1122_oled_off(&plat_oled_descriptor);
        platform_io_power_down_oled();
    }
    #endif
    
    /* Final wait, clear flag, reset */
    while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
    custom_fs_settings_clear_fw_upgrade_flag();
    NVIC_SystemReset();
    while(1);
}
