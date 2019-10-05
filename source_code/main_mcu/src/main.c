#include <string.h>
#include <asf.h>
#include "mooltipass_graphics_bundle.h"
#include "smartcard_highlevel.h"
#include "functional_testing.h"
#include "smartcard_lowlevel.h"
#include "platform_defines.h"
#include "logic_encryption.h"
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
#include "logic_user.h"
#include "custom_fs.h"
#include "dataflash.h"
#include "text_ids.h"
#include "lis2hh12.h"
#include "nodemgmt.h"
#include "dbflash.h"
#include "sh1122.h"
#include "inputs.h"
#include "utils.h"
#include "fuses.h"
#include "debug.h"
#include "main.h"
#include "rng.h"
#include "dma.h"

/* Our oled & dataflash & dbflash descriptors */
accelerometer_descriptor_t plat_acc_descriptor = {.sercom_pt = ACC_SERCOM, .cs_pin_group = ACC_nCS_GROUP, .cs_pin_mask = ACC_nCS_MASK, .int_pin_group = ACC_INT_GROUP, .int_pin_mask = ACC_INT_MASK, .evgen_sel = ACC_EV_GEN_SEL, .evgen_channel = ACC_EV_GEN_CHANNEL, .dma_channel = 3};
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
    BOOL debugger_present = FALSE;
    
    /* Low level port initializations for power supplies */
    platform_io_enable_switch();                                            // Enable switch and 3v3 stepup
    platform_io_init_power_ports();                                         // Init power port, needed to test if we are battery or usb powered
    platform_io_init_no_comms_signal();                                     // Init no comms signal, used later as wakeup for the aux MCU
    
    /* Check if fuses are correctly programmed (required to read flags), if so initialize our flag system, then finally check if we previously powered off due to low battery and still haven't charged since then */
    if ((fuses_check_program(FALSE) == RETURN_OK) && (custom_fs_settings_init() == CUSTOM_FS_INIT_OK) && (custom_fs_get_device_flag_value(PWR_OFF_DUE_TO_BATTERY_FLG_ID) != FALSE))
    {
        /* Check if USB 3V3 is present and if so clear flag */
        if (platform_io_is_usb_3v3_present() == FALSE)
        {
            platform_io_disable_switch_and_die();
            while(1);
        }
        else
        {
            custom_fs_set_device_flag_value(PWR_OFF_DUE_TO_BATTERY_FLG_ID, FALSE);
        }
    }
    
    /* Measure battery voltage */
    platform_io_init_bat_adc_measurements();                                // Initialize ADC measurements  
    platform_io_enable_vbat_to_oled_stepup();                               // Enable vbat to oled stepup
    platform_io_get_voledin_conversion_result_and_trigger_conversion();     // Start one measurement
    while(platform_io_is_voledin_conversion_result_ready() == FALSE);       // Do measurement even if we are USB powered, to leave exactly 180ms for platform boot
    
    /* Check if battery powered and under-voltage */
    uint16_t battery_voltage = platform_io_get_voledin_conversion_result_and_trigger_conversion();
    logic_power_register_vbat_adc_measurement(battery_voltage);
    if ((platform_io_is_usb_3v3_present() == FALSE) && (battery_voltage < BATTERY_ADC_OUT_CUTOUT))
    {
        platform_io_disable_switch_and_die();
        while(1);
    }
    
    /* Check fuses, program them if incorrectly set */
    fuses_ok = fuses_check_program(TRUE);
    while(fuses_ok != RETURN_OK);
    
    /* Check if debugger present */
    if (DSU->STATUSB.bit.DBGPRES != 0)
    {
        debugger_present = TRUE;
        
        /* Debugger connected but we are not on a dev platform? */
        #ifndef NO_SECURITY_BIT_CHECK
        while(1);
        #endif
    }
    
    /* Switch to 48MHz */
    clocks_start_48MDFLL();
    
    /* Second custom FS init (as fuses may have been programmed since the first), check for data flash, absence of bundle and bundle integrity */
    platform_io_init_flash_ports();
    custom_fs_return = custom_fs_settings_init();
    custom_fs_set_dataflash_descriptor(&dataflash_descriptor);
    dataflash_init_return = dataflash_check_presence(&dataflash_descriptor);
    if (dataflash_init_return == RETURN_OK)
    {
        custom_fs_init_return = custom_fs_init();
        if (custom_fs_init_return == RETURN_OK)
        {
            /* Bundle integrity check */
            bundle_integrity_check_return = custom_fs_compute_and_check_external_bundle_crc32();
        }
    }
    
    /* DMA transfers inits, timebase, platform ios, enable comms */
    dma_init();
    logic_power_init();
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
    
    /* Release aux MCU reset (v2 platform only) */
    platform_io_release_aux_reset();

    /* Check initialization results */
    if (custom_fs_return == CUSTOM_FS_INIT_NO_RWEE)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No RWWE");
        while(1);        
    }
    
    /* Check for data flash */
    if (dataflash_init_return != RETURN_OK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No Dataflash");
        while(1);
    }
    
    /* Check for DB flash */
    if (dbflash_check_presence(&dbflash_descriptor) != RETURN_OK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No DB Flash");
        while(1);
    }
    
    /* Check for accelerometer presence */
    if (lis2hh12_check_presence_and_configure(&plat_acc_descriptor) != RETURN_OK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No Accelerometer");
        while(1);
    }
    
    /* Is Aux MCU present? */
    if (comms_aux_mcu_send_receive_ping() != RETURN_OK)
    {
        /* Try to reset our comms link */
        dma_aux_mcu_disable_transfer();
        comms_aux_arm_rx_and_clear_no_comms();
        
        /* Try again */
        if (comms_aux_mcu_send_receive_ping() != RETURN_OK)
        {
            /* Use hard comms reset procedure */
            comms_aux_mcu_hard_comms_reset_with_aux_mcu_reboot();
            
            if (comms_aux_mcu_send_receive_ping() != RETURN_OK)
            {
                sh1122_put_error_string(&plat_oled_descriptor, u"No Aux MCU");
                while(1);
            }                
        }
    }
    
    /* If debugger attached, let the aux mcu know it shouldn't use the no comms signal */
    if (debugger_present != FALSE)
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_NO_COMMS_UNAV);
    }    
    
    /* If USB present, send USB attach message */
    if (platform_io_is_usb_3v3_present() != FALSE)
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_ATTACH_USB);
        logic_power_usb_enumerate_just_sent();
    } 
    
    // TO REMOVE
    if (custom_fs_get_device_flag_value(FUNCTIONAL_TEST_PASSED_FLAG_ID) == FALSE)
    {
        custom_fs_set_device_flag_value(FUNCTIONAL_TEST_PASSED_FLAG_ID, TRUE);
    }
    /* Check for functional testing passed */
    #ifdef DEVELOPER_FEATURES_ENABLED
    if ((custom_fs_get_device_flag_value(FUNCTIONAL_TEST_PASSED_FLAG_ID) == FALSE) && (mcu_sp_rh_addresses[1] != 0x0201))
    #else
    if (custom_fs_get_device_flag_value(FUNCTIONAL_TEST_PASSED_FLAG_ID) == FALSE)
    #endif
    {
        functional_testing_start(TRUE);
    }
    
    /* Display error messages if something went wrong during custom fs init and bundle check */
    if ((custom_fs_init_return != RETURN_OK) || (bundle_integrity_check_return != RETURN_OK))
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No Bundle");
        timer_start_timer(TIMER_TIMEOUT_FUNCTS, 10000);
        
        /* Wait to load bundle from USB */
        while(1)
        {
            /* Communication function */
            comms_msg_rcvd_te msg_received = comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_BUNDLE);
            
            /* If we received any message, reset timer */
            if (msg_received != NO_MSG_RCVD)
            {
                timer_start_timer(TIMER_TIMEOUT_FUNCTS, 10000);
            }            
            
            /* Check for reindex bundle message */
            if (msg_received == HID_REINDEX_BUNDLE_RCVD)
            {
                /* Try to init our file system */
                custom_fs_init_return = custom_fs_init();
                if (custom_fs_init_return == RETURN_OK)
                {
                    break;
                }
            }
            
            /* Timer timeout, switch off platform */
            if ((timer_has_timer_expired(TIMER_TIMEOUT_FUNCTS, TRUE) == TIMER_EXPIRED) && (platform_io_is_usb_3v3_present() == FALSE))
            {
                /* Switch off OLED, switch off platform */
                platform_io_power_down_oled(); timer_delay_ms(200);
                platform_io_disable_switch_and_die();
            }
        }
    } 
    
    /* If the device went through the bootloader: re-initialize device settings and clear bool */
    if (custom_fs_get_device_flag_value(DEVICE_WENT_THROUGH_BOOTLOADER_FLAG_ID) != FALSE)
    {
        custom_fs_set_device_flag_value(DEVICE_WENT_THROUGH_BOOTLOADER_FLAG_ID, FALSE);
    }
    
    /* Set settings that may not have been set to an initial value */
    custom_fs_set_undefined_settings();
}

/*! \fn     main_reboot(void)
*   \brief  Reboot
*/
void main_reboot(void)
{
    /* Wait for accelerometer DMA transfer end */
    lis2hh12_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor);
    while (dma_acc_check_and_clear_dma_transfer_flag() == FALSE);
    /* Power Off OLED screen */
    sh1122_oled_off(&plat_oled_descriptor);
    platform_io_power_down_oled();
    /* No comms */
    platform_io_set_no_comms();
    /* Wait and reboot */
    timer_delay_ms(100);
    cpu_irq_disable();
    NVIC_SystemReset();
    while(1);
}

/*! \fn     main_standby_sleep(void)
*   \brief  Go to sleep
*/
void main_standby_sleep(void)
{
    aux_mcu_message_t* temp_rx_message;
    
    /* Send a go to sleep message to aux MCU, wait for ack, leave no comms high (automatically set when receiving the sleep received event) */
    comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_SLEEP);
    while(comms_aux_mcu_active_wait(&temp_rx_message, FALSE, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, FALSE, AUX_MCU_EVENT_SLEEP_RECEIVED) != RETURN_OK);
    
    /* Disable aux MCU dma transfers */
    dma_aux_mcu_disable_transfer();
    
    /* Wait for accelerometer DMA transfer end and put it to sleep */
    lis2hh12_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor);
    while (dma_acc_check_and_clear_dma_transfer_flag() == FALSE);
    lis2hh12_deassert_ncs_and_go_to_sleep(&plat_acc_descriptor);
    
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
    
    /* Send any command to DB flash to wake it up (required in case of going to sleep twice) */
    dbflash_check_presence(&dbflash_descriptor);
    
    /* Re-enable AUX comms */
    comms_aux_arm_rx_and_clear_no_comms();
    
    /* Resume accelerometer processing */
    lis2hh12_sleep_exit_and_dma_arm(&plat_acc_descriptor);
    
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
    
    /* Actions for first user device boot */
    #ifdef DEVELOPER_FEATURES_ENABLED
    if ((custom_fs_get_device_flag_value(NOT_FIRST_BOOT_FLAG_ID) == FALSE) && (mcu_sp_rh_addresses[1] != 0x0201))
    #else
    if (custom_fs_get_device_flag_value(NOT_FIRST_BOOT_FLAG_ID) == FALSE)
    #endif
    {
        /* Select language and store it as default */
        if (gui_prompts_select_language_or_keyboard_layout(FALSE, TRUE, TRUE) != RETURN_OK)
        {
            /* We're battery powered, the user didn't select anything, switch off device */
            sh1122_oled_off(&plat_oled_descriptor);
            platform_io_power_down_oled();
            timer_delay_ms(100);
            platform_io_disable_switch_and_die();
            while(1);            
        }
        
        /* Store set language as device default one */
        custom_fs_set_device_default_language(custom_fs_get_current_language_id());
        
        /* Set flag */
        custom_fs_set_device_flag_value(NOT_FIRST_BOOT_FLAG_ID, TRUE);
    }
    
    /* TO REMOVE */
    //platform_io_set_no_comms();while(1);
    platform_io_enable_ble();
    
    /* Special developer features */
    #ifdef SPECIAL_DEVELOPER_CARD_FEATURE
    /* Check if this is running on a device without bootloader, add CPZ entry for special card */
    if (mcu_sp_rh_addresses[1] == 0x0201)
    {
        /* Special card has 0000 CPZ, set 0000 as nonce */
        //dbflash_format_flash(&dbflash_descriptor);
        //nodemgmt_format_user_profile(100, 0, 0);
        cpz_lut_entry_t special_user_profile;
        memset(&special_user_profile, 0, sizeof(special_user_profile));
        special_user_profile.user_id = 100;
                
        /* When developping on a newly flashed board: reset USB connection and reset defaults */
        if (custom_fs_store_cpz_entry(&special_user_profile, special_user_profile.user_id) == RETURN_OK)
        {
            if (platform_io_is_usb_3v3_present() != FALSE)
            {
                comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_DETACH_USB);
                timer_delay_ms(2000);
                comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_ATTACH_USB);
                logic_power_usb_enumerate_just_sent();
            }
        }        
    }
    #endif
    
    /* Activity detected */
    logic_device_activity_detected();
    
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
        /* Power handling routine */
        power_action_te power_action = logic_power_routine();
        
        /* If the power routine tells us to power off, provided we are not updating */
        if ((power_action == POWER_ACT_POWER_OFF) && (gui_dispatcher_get_current_screen() != GUI_SCREEN_FW_FILE_UPDATE))
        {
            /* Set flag */
            custom_fs_set_device_flag_value(PWR_OFF_DUE_TO_BATTERY_FLG_ID, TRUE);
            logic_power_power_down_actions();
            
            /* Out of battery! */
            gui_prompts_display_information_on_screen_and_wait(BATTERY_EMPTY_TEXT_ID, DISP_MSG_WARNING);
            sh1122_oled_off(&plat_oled_descriptor);
            platform_io_power_down_oled();
            timer_delay_ms(100);
            platform_io_disable_switch_and_die();
            while(1);
        }
        
        /* Do not do anything if we're uploading new graphics contents */
        if (gui_dispatcher_get_current_screen() != GUI_SCREEN_FW_FILE_UPDATE)
        {
            /* Do appropriate actions on smartcard insertion / removal */
            if (card_detection_res == RETURN_JDETECT)
            {
                /* Light up the Mooltipass and call the dedicated function */
                logic_device_set_state_changed();
                logic_device_activity_detected();
                logic_smartcard_handle_inserted();
            }
            else if (card_detection_res == RETURN_JRELEASED)
            {
                /* Light up the Mooltipass and call the dedicated function */
                logic_device_activity_detected();
                logic_smartcard_handle_removed();
                logic_device_set_state_changed();

                /* Lock shortcut, if enabled */
                /*if ((mp_lock_unlock_shortcuts != FALSE) && ((getMooltipassParameterInEeprom(LOCK_UNLOCK_FEATURE_PARAM) & LF_WIN_L_SEND_MASK) != 0))
                {
                    usbSendLockShortcut();
                    mp_lock_unlock_shortcuts = FALSE;
                }*/
            
                /* Set correct screen */
                gui_prompts_display_information_on_screen_and_wait(CARD_REMOVED_TEXT_ID, DISP_MSG_INFO);
                gui_dispatcher_set_current_screen(GUI_SCREEN_NINSERTED, TRUE, GUI_INTO_MENU_TRANSITION);
                gui_dispatcher_get_back_to_current_screen();
            }
        
            /* GUI main loop */
            gui_dispatcher_main_loop();            
        }
        
        /* test code */
        if (gui_dispatcher_get_current_screen() == GUI_SCREEN_MAIN_MENU)
        {
            //logic_user_store_credential(u"abracadabralapinou.com", u"encoreunsuperlogin", 0, 0, 0);
            //logic_user_store_credential(u"lapin.fr", u"zsuperlongpassword", 0, 0, 0);
            //gui_prompts_ask_for_login_select(nodemgmt_get_starting_parent_addr());
            //gui_dispatcher_get_back_to_current_screen();
        }
        
        /* Communications */
        if (gui_dispatcher_get_current_screen() != GUI_SCREEN_FW_FILE_UPDATE)
        {
            comms_aux_mcu_routine(MSG_NO_RESTRICT);
        }
        else
        {
            comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_BUNDLE);            
        }
        
        /* Accelerometer interrupt */
        if (lis2hh12_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor) != FALSE)
        {
            rng_feed_from_acc_read();
        }
        
        /* Device state changed, inform aux MCU so it can update its buffer */
        if (logic_device_get_state_changed_and_reset_bool() != FALSE)
        {
            comms_aux_mcu_update_device_status_buffer();
        }
        
        /* Get current smartcard detection result */
        card_detection_res = smartcard_lowlevel_is_card_plugged();
    }
}
