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
#include "fuses.h"
#include "main.h"
#include "dma.h"
/* Defines for flashing */
volatile uint32_t current_address = APP_START_ADDR;
/* Our oled & dataflash & dbflash descriptors */
sh1122_descriptor_t plat_oled_descriptor = {.sercom_pt = OLED_SERCOM, .dma_trigger_id = OLED_DMA_SERCOM_TX_TRIG, .sh1122_cs_pin_group = OLED_nCS_GROUP, .sh1122_cs_pin_mask = OLED_nCS_MASK, .sh1122_cd_pin_group = OLED_CD_GROUP, .sh1122_cd_pin_mask = OLED_CD_MASK};
spi_flash_descriptor_t dataflash_descriptor = {.sercom_pt = DATAFLASH_SERCOM, .cs_pin_group = DATAFLASH_nCS_GROUP, .cs_pin_mask = DATAFLASH_nCS_MASK};
spi_flash_descriptor_t dbflash_descriptor = {.sercom_pt = DBFLASH_SERCOM, .cs_pin_group = DBFLASH_nCS_GROUP, .cs_pin_mask = DBFLASH_nCS_MASK};

#ifdef DEVELOPER_FEATURES_ENABLED
BOOL special_dev_card_inserted = FALSE;
#endif

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

/*! \fn     main(void)
*   \brief  Program Main
*/
int main(void)
{
#if defined(PLAT_V7_SETUP)
    /* Mass production units get signed firmware updates */
    br_aes_ct_ctrcbc_keys bootloader_cur_aes_context;                                                                   // The AES encryption context
    uint8_t new_aes_key[AES_KEY_LENGTH/8];                                                                              // New AES encryption key
    uint8_t cur_aes_key[AES_KEY_LENGTH/8];                                                                              // AES encryption key
    uint8_t cur_cbc_mac[16];                                                                                            // Current CBCMAC val
    BOOL aes_key_update_bool;                                                                                           // Boolean specifying that we want to update the aes key
    
    /* Sanity checks */
    _Static_assert((W25Q16_FLASH_SIZE-START_OF_SIGNED_DATA_IN_DATA_FLASH) % 16 == 0, "CBCMAC address space isn't a multiple of block size");
    _Static_assert(sizeof(bundle_data) % 16 == 0, "Bundle buffer size is not a multiple of block size");
    _Static_assert(sizeof(cur_cbc_mac) == 16, "Invalid MAC buffer size");
#endif

    /* Local variables */
    custom_fs_address_t current_data_flash_addr;                                                                        // Current data flash address
    uint16_t bundle_data[NVMCTRL_ROW_SIZE/2];                                                                           // Bundle data, page sized
    //uint8_t old_version_number[4];                                                                                      // Old firmware version identifier
    //uint8_t new_version_number[4];                                                                                      // New firmware version identifier
    
    /* Enable switch and 3V3 stepup, set no comms signal, leave some time for stepup powerup */
    platform_io_enable_switch();
    platform_io_init_no_comms_signal();
    DELAYMS_8M(100);
    
    /* Fuses not programmed, start application */
    if (fuses_check_program(FALSE) != RETURN_OK)
    {
        start_application();
    }
    
    /* Initialize our settings system */
    custom_fs_settings_init();
    
    /* If no flag set, jump to application */
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
        start_application();
    }
    
    /* Custom file system initialization */
    custom_fs_init();
    
    /* Look for update file address */
    custom_fs_address_t fw_file_address;
    custom_fs_binfile_size_t fw_file_size;
    if (custom_fs_get_file_address(0, &fw_file_address, CUSTOM_FS_FW_UPDATE_TYPE) == RETURN_NOK)
    {
        /* If we couldn't find the update file */
        custom_fs_settings_clear_fw_upgrade_flag();
        start_application();
    }
    
    /* Read file size */
    custom_fs_read_from_flash((uint8_t*)&fw_file_size, fw_file_address, sizeof(fw_file_size));
    fw_file_address += sizeof(fw_file_size);
    
    /* Check CRC32 */
    if (custom_fs_compute_and_check_external_bundle_crc32() == RETURN_NOK)
    {
        /* Wrong CRC32 */
        custom_fs_settings_clear_fw_upgrade_flag();
        start_application();
    }

    /* Fetch encryption key: TODO */
    #if defined(PLAT_V7_SETUP)
    memset(cur_aes_key, 0, sizeof(cur_aes_key));
    #endif
    
    /* Automatic write, disable caching */
    NVMCTRL->CTRLB.bit.MANW = 0;
    NVMCTRL->CTRLB.bit.CACHEDIS = 1;

    /* Two passes: one to check the signature, one to flash and check previously stored signature for fw file */
    for (uint16_t nb_pass = 0; nb_pass < 2; nb_pass++)
    {
        #if defined(PLAT_V7_SETUP)
        /* Initialize encryption context, set IV to 0 */
        br_aes_ct_ctrcbc_init(&bootloader_cur_aes_context, cur_aes_key, AES_KEY_LENGTH/8);
        memset((void*)cur_cbc_mac, 0x00, sizeof(cur_cbc_mac));
        #endif

        /* Set current dataflash address at the beginning of the signed data */
        current_data_flash_addr = custom_fs_get_start_address_of_signed_data();

        /* Booleans to know if we are in the right address space to fetch firmware data */
        uint32_t address_in_mcu_memory = APP_START_ADDR;
        uint32_t nb_bytes_written_in_mcu_memory = 0;
        BOOL address_passed_for_fw_data = FALSE;
        BOOL address_valid_for_fw_data = FALSE;

        /* CBCMAC the complete memory */
        while (current_data_flash_addr < W25Q16_FLASH_SIZE)
        {
            /* Compute number of bytes to read */
            uint32_t nb_bytes_to_read = W25Q16_FLASH_SIZE - current_data_flash_addr;
            if (nb_bytes_to_read > sizeof(bundle_data))
            {
                nb_bytes_to_read = sizeof(bundle_data);
            }

            /* Read the bytes... */
            custom_fs_read_from_flash((uint8_t*)bundle_data, current_data_flash_addr, nb_bytes_to_read);

            /* CBCMAC the crap out of it */
            #if defined(PLAT_V7_SETUP)
            br_aes_ct_ctrcbc_mac(&bootloader_cur_aes_context, cur_cbc_mac, bundle_data, nb_bytes_to_read);
            #endif

            /* Where the fw data is valid inside our read buffer */
            uint32_t valid_fw_data_offset = 0;

            /* Check if we are in the right address space for fw data */
            if ((address_valid_for_fw_data == FALSE) && (address_passed_for_fw_data == FALSE) && ((current_data_flash_addr + nb_bytes_to_read) > fw_file_address))
            {
                /* Our firmware is longer than a page, we should be good ;) */
                address_valid_for_fw_data = TRUE;

                /* Compute the fw data offset */
                valid_fw_data_offset = fw_file_address - current_data_flash_addr;
            }

            /* Second pass, should we flash into main MCU? */
            if ((nb_pass == 1) 
                && TRUE 
                && (address_valid_for_fw_data != FALSE))
            {
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
                    NVM_MEMORY[address_in_mcu_memory/2] = bundle_data[i];

                    /* Increment address counter */
                    nb_bytes_written_in_mcu_memory += 2;
                    address_in_mcu_memory += 2;

                    /* Check for end of fw file... */
                    if (nb_bytes_written_in_mcu_memory >= fw_file_size)
                    {
                        address_passed_for_fw_data = TRUE;
                        address_valid_for_fw_data = FALSE;
                        break;
                    }
                }
            }

            /* Increment address */
            current_data_flash_addr += nb_bytes_to_read;
        }
    }
    
    /* Final wait, clear flag, reset */
    while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
    custom_fs_settings_clear_fw_upgrade_flag();
    NVIC_SystemReset();
    while(1);
}
