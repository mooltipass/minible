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
#include "debug.h"
#include "main.h"
#include "dma.h"

/* Our oled & dataflash & dbflash descriptors */
accelerometer_descriptor_t acc_descriptor = {.sercom_pt = ACC_SERCOM, .cs_pin_group = ACC_nCS_GROUP, .cs_pin_mask = ACC_nCS_MASK, .int_pin_group = ACC_INT_GROUP, .int_pin_mask = ACC_INT_MASK, .evgen_sel = ACC_EV_GEN_SEL, .evgen_channel = ACC_EV_GEN_CHANNEL, .dma_channel = 3};
sh1122_descriptor_t plat_oled_descriptor = {.sercom_pt = OLED_SERCOM, .dma_trigger_id = OLED_DMA_SERCOM_TX_TRIG, .sh1122_cs_pin_group = OLED_nCS_GROUP, .sh1122_cs_pin_mask = OLED_nCS_MASK, .sh1122_cd_pin_group = OLED_CD_GROUP, .sh1122_cd_pin_mask = OLED_CD_MASK};
spi_flash_descriptor_t dataflash_descriptor = {.sercom_pt = DATAFLASH_SERCOM, .cs_pin_group = DATAFLASH_nCS_GROUP, .cs_pin_mask = DATAFLASH_nCS_MASK};
spi_flash_descriptor_t dbflash_descriptor = {.sercom_pt = DBFLASH_SERCOM, .cs_pin_group = DBFLASH_nCS_GROUP, .cs_pin_mask = DBFLASH_nCS_MASK};


/****************************************************************************/
/* The blob of code below is aimed at facilitating our development process  */
/* To understand this, you need to know that:                               */
/* - the Cortex M0 expects the stack pointer address to be at addr 0x0000   */
/* - the Cortex M0 expects the reset handler address to be at addr 0x0004   */
/*                                                                          */
/* So this is what we do:                                                   */
/* - put the sram base address + something at addr0 using the array below   */
/* - then put the address of a custom function made to boot the main code   */
/*                                                                          */
/* This is made possible by:                                                */
/* - adding .flash_start_addr=0x0 to linker option                          */
/* - adding .start_app_function_addr=0x200 (matches the 2nd element in array*/
/* - adding --undefined=jump_to_application_function to linker option       */
/* - adding --undefined=jump_to_application_function_addr to linker option  */
/*                                                                          */
/* To move the application address, change APP_START_ADDR & .text           */
/* What I don't understand: why the "+1" in the second array element        */
/****************************************************************************/
const uint32_t jump_to_application_function_addr[2] __attribute__((used,section (".flash_start_addr"))) = {HMCRAMC0_ADDR+100,0x200+1};
void jump_to_application_function(void) __attribute__((used,section (".start_app_function_addr")));
void jump_to_application_function(void)
{
    /* Overwriting the default value of the NVMCTRL.CTRLB.MANW bit (errata reference 13134) */
    NVMCTRL->CTRLB.bit.MANW = 1;
    
    /* Pointer to the Application Section */
    void (*application_code_entry)(void);
    
    /* Rebase the Stack Pointer */
    __set_MSP(*(uint32_t*)APP_START_ADDR);
    
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
    
    /* Check for accelerometer presence */
    if (lis2hh12_check_presence_and_configure(&acc_descriptor) == RETURN_NOK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No Accelerometer");
        while(1);
    }
    
    /* Initialize our custom file system stored in data flash */
    if (custom_fs_init() == RETURN_NOK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No Bundle");
        
        /* Wait to load bundle from USB */
        /*while(1)
        {
            comms_aux_mcu_routine();
        }*/
    }
    
    /* Now that our custom filesystem is loaded, load the default font from flash */
    sh1122_refresh_used_font(&plat_oled_descriptor);    
}

/*! \fn     main_standby_sleep(void)
*   \brief  Go to sleep
*/
void main_standby_sleep(void)
{    
    /* Disable aux MCU dma transfers */
    dma_aux_mcu_disable_transfer();
    
    /* Wait for accelerometer DMA transfer end and put it to sleep */
    lis2hh12_check_data_received_flag_and_arm_other_transfer(&acc_descriptor);
    while (dma_acc_check_and_clear_dma_transfer_flag() == FALSE);
    lis2hh12_deassert_ncs_and_go_to_sleep(&acc_descriptor);
    
    /* DB & Dataflash power down */
    dbflash_enter_ultra_deep_power_down(&dbflash_descriptor);
    dataflash_power_down(&dataflash_descriptor);
    
    /* Switch off OLED */
    sh1122_oled_off(&plat_oled_descriptor);
    platform_io_power_down_oled();
    
    /* Errata 10416: disable interrupt routines */
    cpu_irq_enter_critical();
        
    /* Prepare the ports for sleep */
    platform_io_prepare_ports_for_sleep();
    
    /* Enter deep sleep */
    SCB->SCR = SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
    
    /* Prepare ports for sleep exit */
    platform_io_prepare_ports_for_sleep_exit();
    
    /* Damn errata... enable interrupts */
    cpu_irq_leave_critical();    
    
    /* Switch on OLED */    
    platform_io_power_up_oled(platform_io_is_usb_3v3_present());
    sh1122_oled_on(&plat_oled_descriptor);
    
    /* Dataflash power up */
    dataflash_exit_power_down(&dataflash_descriptor);
    
    /* Re-enable AUX comms */
    comms_aux_init();
    
    /* Resume accelerometer processing */
    lis2hh12_sleep_exit_and_dma_arm(&acc_descriptor);
}

/*! \fn     main(void)
*   \brief  Program Main
*/
int main(void)
{
    /* Initialize our platform */
    main_platform_init();
    //timer_delay_ms(2000);
    //main_standby_sleep();
    debug_debug_screen();
    
    // Test code: burn internal graphics data into external flash.
    //dataflash_bulk_erase_with_wait(&dataflash_descriptor);
    //dataflash_write_array_to_memory(&dataflash_descriptor, 0, mooltipass_bundle, (uint32_t)sizeof(mooltipass_bundle));
    //custom_fs_init(&dataflash_descriptor);
    
    /*platform_io_get_voledin_conversion_result_and_trigger_conversion();
    while (platform_io_is_voledin_conversion_result_ready() == FALSE);
    platform_io_get_voledin_conversion_result_and_trigger_conversion();
    uint16_t result = 0;
    while (TRUE)
    {
        volatile uint32_t ts_start = timer_get_systick();
        while (platform_io_is_voledin_conversion_result_ready() == FALSE);
        volatile uint32_t ts_end = timer_get_systick();
        sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, "%u", ts_end-ts_start);
        sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, "%u", result);
        result = platform_io_get_voledin_conversion_result_and_trigger_conversion();        
    }*/
    
    /*while(1)
    {
        comms_aux_mcu_routine();
         if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
            break;
    }   
    custom_fs_init(&dataflash_descriptor);     */
    
    
    /* Animation test */
    calendar_t temp_calendar;
    uint32_t total_time=0;
    uint32_t start_time;
    uint32_t end_time;
    uint32_t abc = 0;
    uint32_t cntt = 0;
    while(1)
    {
        start_time = timer_get_systick();
        for (uint32_t i = 0; i < 120; i++)
        {
            abc++;
            comms_aux_mcu_routine();
            if (lis2hh12_check_data_received_flag_and_arm_other_transfer(&acc_descriptor) != FALSE)
            {
                cntt++;
                if (abc > 400)
                {
                    asm("Nop");
                }
            }
            
            if (SERCOM1->SPI.STATUS.bit.BUFOVF != 0)
            {
                sh1122_put_error_string(&plat_oled_descriptor, u"ACC Overflow");      
            }
            if (SERCOM4->SPI.STATUS.bit.BUFOVF != 0)
            {
                sh1122_put_error_string(&plat_oled_descriptor, u"AUX COM Overflow");      
            }
            
            //timer_start_timer(TIMER_TIMEOUT_FUNCTS, 25);
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, i);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_LEFT, "%i %i %i", acc_descriptor.fifo_read.acc_data_array[0].acc_x, acc_descriptor.fifo_read.acc_data_array[0].acc_y, acc_descriptor.fifo_read.acc_data_array[0].acc_z);
            timer_get_calendar(&temp_calendar);
            sh1122_printf_xy(&plat_oled_descriptor, 160, 50, OLED_ALIGN_LEFT, "%u:%u:%u", temp_calendar.bit.HOUR, temp_calendar.bit.MINUTE, temp_calendar.bit.SECOND);
            //while (timer_has_timer_expired(TIMER_TIMEOUT_FUNCTS, TRUE) == TIMER_RUNNING);
            
            if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_UP)
            {
                timer_delay_ms(2000);
                main_standby_sleep();
                /*while (TRUE)
                {
                    comms_aux_mcu_routine();
                    if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
                    {
                        custom_fs_init();
                        break;
                    }                        
                }*/
            }   
            //timer_delay_ms(100);
        }
        if ((abc == 240) || (abc == 480))
        {
            main_standby_sleep();
        }
        end_time = timer_get_systick();
        total_time = end_time - start_time;
        sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_CENTER, "%u Nb ms: %u", cntt, total_time);
        sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_CENTER, "%u", abc);
        //timer_delay_ms(100);
    }
    
    /* Language feature test */
    while(1)
    {
        cust_char_t* lapin;
        for (uint16_t i = 0; i < custom_fs_get_number_of_languages(); i++)
        {
            custom_fs_set_current_language(i);
            sh1122_refresh_used_font(&plat_oled_descriptor);
            custom_fs_get_string_from_file(0, &lapin);
            sh1122_display_bitmap_from_flash(&plat_oled_descriptor, 0, 0, 24);
            sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, custom_fs_get_current_language_text_desc());
            sh1122_put_string_xy(&plat_oled_descriptor, 0, 45, OLED_ALIGN_CENTER, lapin);
            timer_delay_ms(1000);
        }        
    }
    
    /*
    sh1122_put_string_xy(&plat_oled_descriptor, 1, 10, OLED_ALIGN_LEFT, u"F");
    sh1122_put_string_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, u"supermarmotte!");
    while (1);
    {
        sh1122_display_bitmap_from_flash(&plat_oled_descriptor, 1, 0, 12);
        timer_delay_ms(3000);while(1);
        sh1122_display_bitmap_from_flash(&plat_oled_descriptor, 0, 0, 10);
        timer_delay_ms(3000);//while(1);
        for (uint32_t j = 0; j < 40; j++)
        for (uint32_t i = 0; i < 10; i++)
            sh1122_display_bitmap_from_flash(&plat_oled_descriptor, 0, 0, i);
        
    }*/
    
    
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
