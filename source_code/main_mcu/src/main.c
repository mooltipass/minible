#include <asf.h>
#include "smartcard_lowlevel.h"
#include "platform_defines.h"
#include "driver_clocks.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "custom_fs.h"
#include "dataflash.h"
#include "dbflash.h"
#include "defines.h"
#include "dma.h"
/* Our dataflash & dbflash descriptors */
spi_flash_descriptor_t dataflash_descriptor = {.sercom_pt = DATAFLASH_SERCOM, .cs_pin_group = DATAFLASH_nCS_GROUP, .cs_pin_mask = DATAFLASH_nCS_MASK};
spi_flash_descriptor_t dbflash_descriptor = {.sercom_pt = DBFLASH_SERCOM, .cs_pin_group = DBFLASH_nCS_GROUP, .cs_pin_mask = DBFLASH_nCS_MASK};


int main (void)
{
	custom_fs_settings_init();                                          // Initialize our settings system
    clocks_start_48MDFLL();                                             // Switch to 48M main clock
    dma_init();                                                         // Initialize the DMA controller
    timer_initialize_timebase();                                        // Initialize the platform time base
    platform_io_init_ports();                                           // Initialize platform IO ports
    if (dataflash_check_presence(&dataflash_descriptor) == RETURN_NOK)  // Check for data flash
    {
        while(1);
    }
    if (dbflash_check_presence(&dbflash_descriptor) == RETURN_NOK)      // Check for db flash
    {
        while(1);
    }
    custom_fs_init(&dataflash_descriptor);
	
	while(1)
    {
        if (smartcard_lowlevel_is_card_plugged() == RETURN_JDETECT)
        {
            smartcard_lowlevel_first_detect_function();
        }
    }
}
