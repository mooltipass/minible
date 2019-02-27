#include <string.h>
#include <asf.h>
#include "mooltipass_graphics_bundle.h"
#include "smartcard_highlevel.h"
#include "smartcard_lowlevel.h"
#include "platform_defines.h"
#include "logic_smartcard.h"
#include "gui_dispatcher.h"
#include "logic_aux_mcu.h"
#include "driver_clocks.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "logic_device.h"
#include "gui_prompts.h"
#include "logic_power.h"
#include "platform_io.h"
#include "custom_fs.h"
#include "dataflash.h"
#include "lis2hh12.h"
#include "nodemgmt.h"
#include "dbflash.h"
#include "defines.h"
#include "sh1122.h"
#include "inputs.h"
#include "fuses.h"
#include "debug.h"
#include "main.h"
#include "dma.h"

/* Our oled & dataflash & dbflash descriptors */
accelerometer_descriptor_t acc_descriptor = {.sercom_pt = ACC_SERCOM, .cs_pin_group = ACC_nCS_GROUP, .cs_pin_mask = ACC_nCS_MASK, .int_pin_group = ACC_INT_GROUP, .int_pin_mask = ACC_INT_MASK, .evgen_sel = ACC_EV_GEN_SEL, .evgen_channel = ACC_EV_GEN_CHANNEL, .dma_channel = 3};
sh1122_descriptor_t plat_oled_descriptor = {.sercom_pt = OLED_SERCOM, .dma_trigger_id = OLED_DMA_SERCOM_TX_TRIG, .sh1122_cs_pin_group = OLED_nCS_GROUP, .sh1122_cs_pin_mask = OLED_nCS_MASK, .sh1122_cd_pin_group = OLED_CD_GROUP, .sh1122_cd_pin_mask = OLED_CD_MASK};
spi_flash_descriptor_t dataflash_descriptor = {.sercom_pt = DATAFLASH_SERCOM, .cs_pin_group = DATAFLASH_nCS_GROUP, .cs_pin_mask = DATAFLASH_nCS_MASK};
spi_flash_descriptor_t dbflash_descriptor = {.sercom_pt = DBFLASH_SERCOM, .cs_pin_group = DBFLASH_nCS_GROUP, .cs_pin_mask = DBFLASH_nCS_MASK};

/* Used to know if there is no bootloader and if the special card is inserted*/
#ifdef DEVELOPER_FEATURES_ENABLED
BOOL special_dev_card_inserted = FALSE;
uint32_t* mcu_sp_rh_addresses = 0;
#endif

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
    /* Initialization results vars */
    custom_fs_init_ret_type_te custom_fs_return = CUSTOM_FS_INIT_NO_RWEE;
    RET_TYPE bundle_integrity_check_return = RETURN_NOK;
    RET_TYPE custom_fs_init_return = RETURN_NOK;
    RET_TYPE dataflash_init_return = RETURN_NOK;
    RET_TYPE fuses_ok = RETURN_NOK;
    
    /* Measure 1Vx, boot aux MCU, check if USB powered */
    platform_io_enable_switch();                                            // Enable switch and 3v3 stepup
    platform_io_init_power_ports();                                         // Init power port, needed later to test if we are battery or usb powered
    platform_io_init_no_comms_signal();                                     // Init no comms signal, used as wakeup for aux MCU at boot
    platform_io_init_bat_adc_measurements();                                // Initialize ADC measurements  
    platform_io_enable_vbat_to_oled_stepup();                               // Enable vbat to oled stepup
    platform_io_get_voledin_conversion_result_and_trigger_conversion();     // Start one measurement
    while(platform_io_is_voledin_conversion_result_ready() == FALSE);       // Do measurement even if we are USB powered, to leave exactly 180ms for platform boot
    
    /* Check if battery powered and undervoltage */
    uint16_t battery_voltage = platform_io_get_voledin_conversion_result_and_trigger_conversion();
    if ((platform_io_is_usb_3v3_present() == FALSE) && (battery_voltage < BATTERY_ADC_OUT_CUTOUT))
    {
        platform_io_disable_switch_and_die();
        while(1);
    }
    
    /* Check fuses, program them if incorrectly set */
    fuses_ok = fuses_check_program(TRUE);
    while(fuses_ok == RETURN_NOK);
    
    /* Switch to 48MHz */
    clocks_start_48MDFLL();
    
    /* Custom FS init, check for data flash, absence of bundle and bundle integrity */
    platform_io_init_flash_ports();
    custom_fs_return = custom_fs_settings_init();
    custom_fs_set_dataflash_descriptor(&dataflash_descriptor);
    dataflash_init_return = dataflash_check_presence(&dataflash_descriptor);
    if (dataflash_init_return != RETURN_NOK)
    {
        custom_fs_init_return = custom_fs_init();
        if (custom_fs_init_return != RETURN_NOK)
        {
            /* Bundle integrity check */
            bundle_integrity_check_return = custom_fs_compute_and_check_external_bundle_crc32();
        }
    }
    
    /* DMA transfers inits, timebase, platform ios, enable comms */
    dma_init();
    timer_initialize_timebase();
    platform_io_init_ports();
    comms_aux_arm_rx_and_clear_no_comms();
    platform_io_init_bat_adc_measurements();
    
    /* Initialize OLED screen */
    if (platform_io_is_usb_3v3_present() == FALSE)
    {
        logic_power_set_power_source(BATTERY_POWERED);
        platform_io_power_up_oled(FALSE);
    } 
    else
    {
        logic_power_set_power_source(USB_POWERED);
        platform_io_power_up_oled(TRUE);
    }
    sh1122_init_display(&plat_oled_descriptor);
    
    /* Release aux MCU reset (old platform only) and enable bluetooth AND gate */
    platform_io_release_aux_reset();
    platform_io_enable_ble();

    /* Check initialization results */
    if (custom_fs_return == CUSTOM_FS_INIT_NO_RWEE)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No RWWE");
        while(1);        
    }
    
    /* Check for data flash */
    if (dataflash_init_return == RETURN_NOK)
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
    
    /* Is Aux MCU present? */
    if (comms_aux_mcu_send_receive_ping() == RETURN_NOK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No Aux MCU");
        while(1);
    }
    
    /* Is battery present? */
    // TODO: completely change the code below
    if (battery_voltage > BATTERY_ADC_OVER_VOLTAGE)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No battery");
        while(1);
    }
    
    /* If USB present, send USB attach message */
    if (platform_io_is_usb_3v3_present() != FALSE)
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_ATTACH_USB);
    }
    
    /* Display error messages if something went wrong during custom fs init and bundle check */
    if ((custom_fs_init_return == RETURN_NOK) || (bundle_integrity_check_return == RETURN_NOK))
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No Bundle");
        
        /* Wait to load bundle from USB */
        while(1)
        {
            comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_BUNDLE);
        }
    }
    else
    {
        /* Now that our custom filesystem is loaded, load the default font from flash */
        sh1122_refresh_used_font(&plat_oled_descriptor, DEFAULT_FONT_ID);        
    }    
    
    /* Check for first boot, perform functional testing */
    //if (custom_fs_is_first_boot() == TRUE)
    if (custom_fs_is_first_boot() == FALSE)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"First Boot Tests...");
        
        /* Get BLE ID */
        logic_aux_mcu_enable_ble(TRUE);
        if (logic_aux_mcu_get_ble_chip_id() == 0)
        {
            sh1122_put_error_string(&plat_oled_descriptor, u"ATBTLC1000 error!");
            while(1);
        }
        
        /* Clear flag */
        custom_fs_settings_clear_first_boot_flag();
    }
}

/*! \fn     main_standby_sleep(void)
*   \brief  Go to sleep
*/
void main_standby_sleep(void)
{    
    /* Send a go to sleep message to aux MCU */
    platform_io_set_no_comms();
    comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_SLEEP);
    dma_wait_for_aux_mcu_packet_sent();
    
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
    comms_aux_arm_rx_and_clear_no_comms();
    
    /* Resume accelerometer processing */
    lis2hh12_sleep_exit_and_dma_arm(&acc_descriptor);
    
    /* Clear wheel detection */
    inputs_clear_detections();
}

/*! \fn     main(void)
*   \brief  Program Main
*/
int main(void)
{
    /* Initialize our platform */
    main_platform_init();
    
    /* Special developer features */
    #ifdef SPECIAL_DEVELOPER_CARD_FEATURE
    /* Check if this is running on a device without bootloader, add CPZ entry for special card */
    if (mcu_sp_rh_addresses[1] == 0x0201)
    {
        /* Special card has 0000 CPZ, set 0000 as nonce */
        cpz_lut_entry_t special_user_profile;
        memset(&special_user_profile, 0, sizeof(special_user_profile));
        special_user_profile.user_id = 100;
        custom_fs_store_cpz_entry(&special_user_profile, special_user_profile.user_id);
    }
    #endif
    
    /* Activity detected */
    logic_device_activity_detected();    

    cust_char_t* bla1 = u"themooltipass.com";
    cust_char_t* bla2 = u"Do you want to login with";
    cust_char_t* bla3 = u"mysuperextralonglogin@mydomain.com";
    cust_char_t* bla4 = u"this is the wonderful line 4... how are you doing?";
    confirmationText_t conf_text = {.lines[0]=bla1, .lines[1]=bla2, .lines[2]=bla3, .lines[3]=bla4};
    //gui_prompts_ask_for_confirmation(1, (confirmationText_t*)u"Erase  Current  User?", TRUE);
    //gui_prompts_ask_for_confirmation(3, &conf_text, TRUE);
    //debug_debug_menu();
    /*gui_prompts_display_information_on_screen_and_wait(31, DISP_MSG_INFO);
    gui_prompts_display_information_on_screen_and_wait(31, DISP_MSG_ACTION);
    gui_prompts_display_information_on_screen_and_wait(31, DISP_MSG_WARNING);
    timer_delay_ms(2000);*/
    volatile uint16_t pinpin;
    gui_prompts_get_user_pin(&pinpin, 32);
    
    #ifndef DEV_SKIP_INTRO_ANIM
    /* Start animation */    
    for (uint16_t i = GUI_ANIMATION_FFRAME_ID; i < GUI_ANIMATION_NBFRAMES; i++)
    {
        timer_start_timer(TIMER_WAIT_FUNCTS, 20);
        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, i, FALSE);
        while(timer_has_timer_expired(TIMER_WAIT_FUNCTS, TRUE) == TIMER_RUNNING)
        {
            comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        }
    }    
    
    /* End of animation: freeze on image, perform long time taking inits... */
    timer_start_timer(TIMER_WAIT_FUNCTS, 1500);
    while(timer_has_timer_expired(TIMER_WAIT_FUNCTS, TRUE) == TIMER_RUNNING)
    {
        comms_aux_mcu_routine(MSG_RESTRICT_ALL);
    }
    #endif
    
    /* Get current smartcard detection result */
    det_ret_type_te card_detection_res = smartcard_lowlevel_is_card_plugged();
        
    /* Set startup screen */
    gui_dispatcher_set_current_screen(GUI_SCREEN_NINSERTED, TRUE, GUI_INTO_MENU_TRANSITION);
    logic_device_activity_detected();
    if (card_detection_res != RETURN_JDETECT)
    {
        gui_dispatcher_get_back_to_current_screen();
    }
    
    /* Infinite loop */
    while(TRUE)
    {
        /* Power supply change */
        if ((logic_power_get_power_source() == BATTERY_POWERED) && (platform_io_is_usb_3v3_present() != FALSE))
        {
            logic_power_set_power_source(USB_POWERED);
            sh1122_oled_off(&plat_oled_descriptor);
            timer_delay_ms(50);
            platform_io_power_up_oled(TRUE);
            sh1122_oled_on(&plat_oled_descriptor);
        } 
        else if ((logic_power_get_power_source() == USB_POWERED) && (platform_io_is_usb_3v3_present() == FALSE))
        {
            logic_power_set_power_source(BATTERY_POWERED);
            sh1122_oled_off(&plat_oled_descriptor);
            timer_delay_ms(50);
            platform_io_power_up_oled(FALSE);
            sh1122_oled_on(&plat_oled_descriptor);
        }        
        
        /* Do appropriate actions on smartcard insertion / removal */
        if (card_detection_res == RETURN_JDETECT)
        {
            /* Light up the Mooltipass and call the dedicated function */
            logic_device_activity_detected();
            logic_smartcard_handle_inserted();
        }
        else if (card_detection_res == RETURN_JRELEASED)
        {
            /* Light up the Mooltipass and call the dedicated function */
            logic_device_activity_detected();
            logic_smartcard_handle_removed();

            /* Lock shortcut, if enabled */
            /*if ((mp_lock_unlock_shortcuts != FALSE) && ((getMooltipassParameterInEeprom(LOCK_UNLOCK_FEATURE_PARAM) & LF_WIN_L_SEND_MASK) != 0))
            {
                usbSendLockShortcut();
                mp_lock_unlock_shortcuts = FALSE;
            }*/
            
            /* Set correct screen */
            gui_prompts_display_information_on_screen_and_wait(ID_STRING_CARD_REMOVED, DISP_MSG_INFO);
            gui_dispatcher_set_current_screen(GUI_SCREEN_NINSERTED, TRUE, GUI_INTO_MENU_TRANSITION);
            gui_dispatcher_get_back_to_current_screen();
        }
        
        /* GUI main loop */
        gui_dispatcher_main_loop();
        
        /* Communications */        
        comms_aux_mcu_routine(MSG_NO_RESTRICT);
        
        /* Accelerometer interrupt */
        if (lis2hh12_check_data_received_flag_and_arm_other_transfer(&acc_descriptor) != FALSE)
        {
        }
        
        /* Get current smartcard detection result */
        card_detection_res = smartcard_lowlevel_is_card_plugged();
    }
}
