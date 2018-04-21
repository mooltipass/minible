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

/* Our oled & dataflash & dbflash descriptors */
sh1122_descriptor_t plat_oled_descriptor = {.sercom_pt = OLED_SERCOM, .dma_trigger_id = OLED_DMA_SERCOM_TX_TRIG, .sh1122_cs_pin_group = OLED_nCS_GROUP, .sh1122_cs_pin_mask = OLED_nCS_MASK, .sh1122_cd_pin_group = OLED_CD_GROUP, .sh1122_cd_pin_mask = OLED_CD_MASK};
spi_flash_descriptor_t dataflash_descriptor = {.sercom_pt = DATAFLASH_SERCOM, .cs_pin_group = DATAFLASH_nCS_GROUP, .cs_pin_mask = DATAFLASH_nCS_MASK};
spi_flash_descriptor_t dbflash_descriptor = {.sercom_pt = DBFLASH_SERCOM, .cs_pin_group = DBFLASH_nCS_GROUP, .cs_pin_mask = DBFLASH_nCS_MASK};


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
    /* Initialize our platform */
    main_platform_init();
	sh1122_put_error_string(&plat_oled_descriptor, u"Bootloader here!");
	while(1);
}
