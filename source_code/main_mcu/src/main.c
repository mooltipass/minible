#include <asf.h>
#include "smartcard_highlevel.h"
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
            mooltipass_card_detect_return_te detection_result = smartcard_highlevel_card_detected_routine();
            
            if ((detection_result == RETURN_MOOLTIPASS_PB) || (detection_result == RETURN_MOOLTIPASS_INVALID))
            {
                // Either it is not a card or our Manufacturer Test Zone write/read test failed
                //guiDisplayInformationOnScreenAndWait(ID_STRING_PB_CARD);
                //printSmartCardInfo();
                //removeFunctionSMC();
            }
            else if (detection_result == RETURN_MOOLTIPASS_BLANK)
            {/*
                // This is a user free card, we can ask the user to create a new user inside the Mooltipass
                if (guiAskForConfirmation(1, (confirmationText_t*)readStoredStringToBuffer(ID_STRING_NEWMP_USER)) == RETURN_OK)
                {
                    volatile uint16_t pin_code;
                    
                    // Create a new user with his new smart card
                    if ((guiAskForNewPin(&pin_code, ID_STRING_PIN_NEW_CARD) == RETURN_NEW_PIN_OK) && (addNewUserAndNewSmartCard(&pin_code) == RETURN_OK))
                    {
                        guiDisplayInformationOnScreenAndWait(ID_STRING_USER_ADDED);
                        next_screen = SCREEN_DEFAULT_INSERTED_NLCK;
                        setSmartCardInsertedUnlocked();
                        return_value = RETURN_OK;
                    }
                    else
                    {
                        // Something went wrong, user wasn't added
                        guiDisplayInformationOnScreenAndWait(ID_STRING_USER_NADDED);
                    }
                    pin_code = 0x0000;
                }
                else
                {
                    guiSetCurrentScreen(next_screen);
                    guiGetBackToCurrentScreen();
                    return return_value;
                }
                printSmartCardInfo();*/
            }
            else if (detection_result == RETURN_MOOLTIPASS_USER)
            {/*
                // Call valid card detection function
                uint8_t temp_return = validCardDetectedFunction(0, TRUE);
                
                // This a valid user smart card, we call a dedicated function for the user to unlock the card
                if (temp_return == RETURN_VCARD_OK)
                {
                    unlockFeatureCheck();
                    next_screen = SCREEN_DEFAULT_INSERTED_NLCK;
                    return_value = RETURN_OK;
                }
                else if (temp_return == RETURN_VCARD_UNKNOWN)
                {
                    // Unknown card, go to dedicated screen
                    guiSetCurrentScreen(SCREEN_DEFAULT_INSERTED_UNKNOWN);
                    guiGetBackToCurrentScreen();
                    return return_value;
                }
                else
                {
                    guiSetCurrentScreen(SCREEN_DEFAULT_INSERTED_LCK);
                    guiGetBackToCurrentScreen();
                    return return_value;
                }
                printSmartCardInfo();*/
            }

            /*if(smartcard_lowlevel_first_detect_function() == RETURN_CARD_4_TRIES_LEFT)
            {
                PORT->Group[OLED_CD_GROUP].OUTCLR.reg = OLED_CD_MASK;
            }*/
        }
    }
}
