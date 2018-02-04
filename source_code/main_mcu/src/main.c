#include <asf.h>
#include "mooltipass_graphics_bundle.h"
#include "smartcard_highlevel.h"
#include "smartcard_lowlevel.h"
#include "platform_defines.h"
#include "driver_clocks.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "custom_fs.h"
#include "dataflash.h"
#include "lis2hh12.h"
#include "dbflash.h"
#include "defines.h"
#include "sh1122.h"
#include "dma.h"
/* Our oled & dataflash & dbflash descriptors */
sh1122_descriptor_t oled_descriptor = {.sercom_pt = OLED_SERCOM, .dma_trigger_id = OLED_DMA_SERCOM_TX_TRIG, .sh1122_cs_pin_group = OLED_nCS_GROUP, .sh1122_cs_pin_mask = OLED_nCS_MASK, .sh1122_cd_pin_group = OLED_CD_GROUP, .sh1122_cd_pin_mask = OLED_CD_MASK};
accelerometer_descriptor_t  acc_descriptor = {.sercom_pt = ACC_SERCOM, .cs_pin_group = ACC_nCS_GROUP, .cs_pin_mask = ACC_nCS_MASK, .int_pin_group = ACC_INT_GROUP, .int_pin_mask = ACC_INT_MASK};
spi_flash_descriptor_t dataflash_descriptor = {.sercom_pt = DATAFLASH_SERCOM, .cs_pin_group = DATAFLASH_nCS_GROUP, .cs_pin_mask = DATAFLASH_nCS_MASK};
spi_flash_descriptor_t dbflash_descriptor = {.sercom_pt = DBFLASH_SERCOM, .cs_pin_group = DBFLASH_nCS_GROUP, .cs_pin_mask = DBFLASH_nCS_MASK};

// REMINDER FOR LATER: if power consumption too high, check default state for miso on smc...

int main (void)
{
    platform_io_enable_switch();                                        // Enable switch and 3v3 stepup
    DELAYMS_8M(100);                                                    // Leave 100ms for stepup powerup
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
    if (lis2hh12_check_presence_and_configure(&acc_descriptor) == RETURN_NOK)
    {
        while(1);
    }
    custom_fs_init(&dataflash_descriptor);                              // Initialize our custom file system stored in data flash
    
    // Test code: burn internal graphics data into external flash.
    //dataflash_bulk_erase_with_wait(&dataflash_descriptor);
    //dataflash_write_array_to_memory(&dataflash_descriptor, 0, mooltipass_bundle, (uint32_t)sizeof(mooltipass_bundle));
    //custom_fs_init(&dataflash_descriptor);
    
    /* Initialize OLED screen */
    platform_io_power_up_oled(FALSE);
    //while(1);
    //PORT->Group[OLED_nRESET_GROUP].OUTSET.reg = OLED_nRESET_MASK;
    //timer_delay_ms(1);
    sh1122_init_display(&oled_descriptor);
    
    acc_data temp_acc_data;
    uint32_t cnt = 0;
    while(1)
    {
        lis2hh12_manual_acc_data_read(&acc_descriptor, &temp_acc_data);
        sh1122_draw_rectangle(&oled_descriptor, 15, 0, 35, 45, 0);
        sh1122_printf_xy(&oled_descriptor, 0, 0, OLED_ALIGN_LEFT, "X: %i", temp_acc_data.acc_x);
        sh1122_printf_xy(&oled_descriptor, 0, 15, OLED_ALIGN_LEFT, "Y: %i", temp_acc_data.acc_y);
        sh1122_printf_xy(&oled_descriptor, 0, 30, OLED_ALIGN_LEFT, "Z: %i", temp_acc_data.acc_z);
        sh1122_printf_xy(&oled_descriptor, 0, 45, OLED_ALIGN_LEFT, "cnt: %i", cnt++);
        timer_delay_ms(10);
    }
    
    sh1122_printf_xy(&oled_descriptor, 0, 0, OLED_ALIGN_CENTER, "FPS: %u", 32);
    while(1);
    
    /*
    sh1122_put_string_xy(&oled_descriptor, 1, 10, OLED_ALIGN_LEFT, u"F");
    sh1122_put_string_xy(&oled_descriptor, 0, 20, OLED_ALIGN_LEFT, u"supermarmotte!");
    while (1);
    {
        sh1122_display_bitmap_from_flash(&oled_descriptor, 1, 0, 12);
        timer_delay_ms(3000);while(1);
        sh1122_display_bitmap_from_flash(&oled_descriptor, 0, 0, 10);
        timer_delay_ms(3000);//while(1);
        for (uint32_t j = 0; j < 40; j++)
        for (uint32_t i = 0; i < 10; i++)
            sh1122_display_bitmap_from_flash(&oled_descriptor, 0, 0, i);
        
    }*/
    while(1)
    {
        for (uint32_t i = 0; i < 10; i++)
        {
            PORT->Group[DBFLASH_nCS_GROUP].OUTSET.reg = DBFLASH_nCS_MASK; 
            sh1122_display_bitmap_from_flash(&oled_descriptor, 0, 0, i);
            //PORT->Group[DBFLASH_nCS_GROUP].OUTCLR.reg = DBFLASH_nCS_MASK;
            //sh1122_display_bitmap_from_flash(&oled_descriptor, 0, 0, i);
        }
    }
    
	
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
            {
                 /* Card is new - transform it into a Mooltipass card */
                 uint16_t factory_pin = SMARTCARD_FACTORY_PIN;

                 /* Try to authenticate with factory pin */
                 pin_check_return_te pin_try_return = smartcard_lowlevel_validate_code(&factory_pin);

                 if (pin_try_return == RETURN_PIN_OK)
                 {
                     smartcard_highlevel_erase_smartcard();
                     PORT->Group[OLED_CD_GROUP].OUTCLR.reg = OLED_CD_MASK;
                     //PORT->Group[OLED_CD_GROUP].OUTCLR.reg = OLED_CD_MASK;
                 }
            
            /*
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
