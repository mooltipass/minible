#include <asf.h>
#include "mooltipass_graphics_bundle.h"
#include "smartcard_highlevel.h"
#include "smartcard_lowlevel.h"
#include "platform_defines.h"
#include "driver_clocks.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "custom_fs.h"
#include "dataflash.h"
#include "lis2hh12.h"
#include "dbflash.h"
#include "defines.h"
#include "sh1122.h"
#include "inputs.h"
#include "main.h"
#include "dma.h"
/* Defines for flashing */
volatile uint32_t current_address = APP_START_ADDR;
/* Our oled & dataflash & dbflash descriptors */
sh1122_descriptor_t plat_oled_descriptor = {.sercom_pt = OLED_SERCOM, .dma_trigger_id = OLED_DMA_SERCOM_TX_TRIG, .sh1122_cs_pin_group = OLED_nCS_GROUP, .sh1122_cs_pin_mask = OLED_nCS_MASK, .sh1122_cd_pin_group = OLED_CD_GROUP, .sh1122_cd_pin_mask = OLED_CD_MASK};
spi_flash_descriptor_t dataflash_descriptor = {.sercom_pt = DATAFLASH_SERCOM, .cs_pin_group = DATAFLASH_nCS_GROUP, .cs_pin_mask = DATAFLASH_nCS_MASK};
spi_flash_descriptor_t dbflash_descriptor = {.sercom_pt = DBFLASH_SERCOM, .cs_pin_group = DBFLASH_nCS_GROUP, .cs_pin_mask = DBFLASH_nCS_MASK};


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

/*! \fn     main_platform_init(void)
*   \brief  Initialize our platform
*/
void main_platform_init(void)
{
    platform_io_enable_switch();                                        // Enable switch and 3v3 stepup
    DELAYMS_8M(100);                                                    // Leave 100ms for stepup powerup
    custom_fs_settings_init();                                          // Initialize our settings system
    clocks_start_48MDFLL();                                             // Switch to 48M main clock
    dma_init();                                                         // Initialize the DMA controller
    timer_initialize_timebase();                                        // Initialize the platform time base
    platform_io_init_ports();                                           // Initialize platform IO ports
    platform_io_init_bat_adc_measurements();                            // Initialize ADC for battery measurements
    comms_aux_init();                                                   // Initialize communication handling with aux MCU
    custom_fs_set_dataflash_descriptor(&dataflash_descriptor);          // Store the dataflash descriptor for our custom fs library
    
    /* Initialize OLED screen */
    platform_io_power_up_oled(platform_io_is_usb_3v3_present());
    sh1122_init_display(&plat_oled_descriptor);
    
    /* Release aux MCU reset and enable bluetooth */
    platform_io_release_aux_reset();
    platform_io_enable_ble();
    
    /* Check for data flash */
    if (dataflash_check_presence(&dataflash_descriptor) == RETURN_NOK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No Dataflash");
        while(1);
    }
    
    /* Check for DB flash */
    if (dbflash_check_presence(&dbflash_descriptor) == RETURN_NOK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No DB Flash");
        while(1);
    }
    
    /* Initialize our custom file system stored in data flash */
    if (custom_fs_init() == RETURN_NOK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No Bundle");
    }
}

/*! \fn     main(void)
*   \brief  Program Main
*/
int main(void)
{    
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
        while(1);
    }
    
    /* Custom file system initialization */
    custom_fs_init();
    
    /* Look for update file address */
    custom_fs_address_t fw_file_address;
    custom_fs_binfile_size_t fw_file_size;
    if (custom_fs_get_file_address(0, &fw_file_address, CUSTOM_FS_FW_UPDATE_TYPE) == RETURN_NOK)
    {
        /* If we couldn't find the update file */
        start_application();
    }
    
    /* Read file size */
    custom_fs_read_from_flash((uint8_t*)&fw_file_size, fw_file_address, sizeof(fw_file_size));
    fw_file_address += sizeof(fw_file_size);
    
    /* Check CRC32 */
    if (custom_fs_compute_and_check_external_bundle_crc32() == RETURN_NOK)
    {
        /* Wrong CRC32 */
        start_application();
    }
    
    /* Automatic write, disable caching */
    NVMCTRL->CTRLB.bit.MANW = 0;
    NVMCTRL->CTRLB.bit.CACHEDIS = 1;
    uint16_t temp_buffer[NVMCTRL_ROW_SIZE/2];
    
    /* Start flashing the firmware */
    for (uint32_t current_flash_address = APP_START_ADDR; current_flash_address < APP_START_ADDR + fw_file_size; current_flash_address += NVMCTRL_ROW_SIZE)
    {
        /* Erase complete row, composed of 4 pages */
        while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
        NVMCTRL->ADDR.reg  = current_flash_address/2;
        NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;
        
        /* Read data to be written */
        custom_fs_read_from_flash((uint8_t*)&temp_buffer, fw_file_address, sizeof(temp_buffer));
        fw_file_address += NVMCTRL_ROW_SIZE;
        
        /* Flash bytes */
        for (uint32_t j = 0; j < 4; j++)
        {
            /* Flash 4 consecutive pages */
            while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
            for(uint32_t i = 0; i < NVMCTRL_ROW_SIZE/4; i+=2)
            {
                NVM_MEMORY[(current_flash_address+j*(NVMCTRL_ROW_SIZE/4)+i)/2] = temp_buffer[(j*(NVMCTRL_ROW_SIZE/4)+i)/2];
            }
        }
    }
    
    while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
    custom_fs_settings_clear_fw_upgrade_flag();
    NVIC_SystemReset();
    while(1);
}
