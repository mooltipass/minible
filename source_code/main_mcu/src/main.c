#include <string.h>
#include <asf.h>
#include "mooltipass_graphics_bundle.h"
#include "smartcard_highlevel.h"
#include "logic_accelerometer.h"
#include "functional_testing.h"
#include "smartcard_lowlevel.h"
#include "platform_defines.h"
#include "logic_encryption.h"
#include "logic_smartcard.h"
#include "gui_dispatcher.h"
#include "logic_security.h"
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
/* A wheel action that may be used to pass to our GUI routine */
wheel_action_ret_te virtual_wheel_action = WHEEL_ACTION_NONE;
/* Know if debugger is present */
BOOL debugger_present = FALSE;

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
/* The "+1" in the second array element indicates that MCU starts in Thumb  */
/* mode (the only mode supported by Cortex M0)                              */
/****************************************************************************/
#ifndef EMULATOR_BUILD

extern uint32_t _estack; //Start of stack as defined by linker
extern uint32_t _sstack; //End of stack as defined by linker

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
#endif

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
    
    /* Low level port initializations for power supplies */
    platform_io_enable_switch();                                            // Enable switch and 3v3 stepup
    platform_io_init_power_ports();                                         // Init power port, needed to test if we are battery or usb powered
    platform_io_init_no_comms_signal();                                     // Init no comms signal, used later as wakeup for the aux MCU
    
    /* Check if fuses are correctly programmed (required to read flags), if so initialize our flag system, then finally check if we previously powered off due to low battery and still haven't charged since then */
    if ((fuses_check_program(FALSE) == RETURN_OK) && (custom_fs_settings_init() == CUSTOM_FS_INIT_OK) && (custom_fs_get_device_flag_value(PWR_OFF_DUE_TO_BATTERY_FLG_ID) != FALSE))
    {
        /* Check if USB 3V3 is present and if so clear flag */
        if (platform_io_is_usb_3v3_present_raw() == FALSE)
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
    uint32_t battery_voltage = platform_io_get_voledin_conversion_result_and_trigger_conversion();
    if ((platform_io_is_usb_3v3_present_raw() == FALSE) && (battery_voltage < BATTERY_ADC_OUT_CUTOUT))
    {
        platform_io_disable_switch_and_die();
        while(1);
    }
    
    /* Register vbat measurement and adjust it if we are USB powered */
    if (platform_io_is_usb_3v3_present_raw() == FALSE)
    {
        logic_power_register_vbat_adc_measurement((uint16_t)battery_voltage);
    } 
    else
    {
        /* Real ratio is 3300 / 3188 */
        battery_voltage = (battery_voltage*265) >> 8;
        logic_power_register_vbat_adc_measurement((uint16_t)battery_voltage);
    }
    
    /* Check fuses, program them if incorrectly set */
    fuses_ok = fuses_check_program(TRUE);
    while(fuses_ok != RETURN_OK);
    
#ifndef EMULATOR_BUILD
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
#endif
    
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
    if (platform_io_is_usb_3v3_present_raw() == FALSE)
    {
        logic_power_set_power_source(BATTERY_POWERED);
        platform_io_power_up_oled(FALSE);
    } 
    else
    {
        platform_io_bypass_3v3_detection_debounce();
        logic_power_set_power_source(USB_POWERED);
        platform_io_power_up_oled(TRUE);
    }
    sh1122_init_display(&plat_oled_descriptor, FALSE);
    
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
    if (platform_io_is_usb_3v3_present_raw() != FALSE)
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_ATTACH_USB);
        logic_power_usb_enumerate_just_sent();
    } 
    
#ifndef EMULATOR_BUILD
    /* Check for non-RF functional testing passed */
    #ifdef DEVELOPER_FEATURES_ENABLED
    if ((custom_fs_get_device_flag_value(FUNCTIONAL_TEST_PASSED_FLAG_ID) == FALSE) && (mcu_sp_rh_addresses[1] != 0x0201))
    #else
    if (custom_fs_get_device_flag_value(FUNCTIONAL_TEST_PASSED_FLAG_ID) == FALSE)
    #endif
    {
        functional_testing_start(TRUE);
    }

    /* Check for RF functional testing passed */
    #ifdef DEVELOPER_FEATURES_ENABLED
    if ((custom_fs_get_device_flag_value(RF_TESTING_PASSED_FLAG_ID) == FALSE) && (mcu_sp_rh_addresses[1] != 0x0201))
    #else
    if (custom_fs_get_device_flag_value(RF_TESTING_PASSED_FLAG_ID) == FALSE)
    #endif
    {
        /* Start continuous tone, wait for test to press long click or timeout to die */
        functional_rf_testing_start();
        timer_start_timer(TIMER_TIMEOUT_FUNCTS, 50000);
        while (inputs_get_wheel_action(FALSE, TRUE) != WHEEL_ACTION_LONG_CLICK)
        {
            /* Timer timeout, switch off platform */
            if ((timer_has_timer_expired(TIMER_TIMEOUT_FUNCTS, FALSE) == TIMER_EXPIRED) && (platform_io_is_usb_3v3_present() == FALSE))
            {
                /* Switch off OLED, switch off platform */
                platform_io_power_down_oled(); timer_delay_ms(200);
                platform_io_disable_switch_and_die();
            }
            
            /* Handle possible power switches */
            logic_power_check_power_switch_and_battery(FALSE);
            
            /* Comms functions */
            comms_aux_mcu_routine(MSG_RESTRICT_ALL);
        }
        custom_fs_set_device_flag_value(RF_TESTING_PASSED_FLAG_ID, TRUE);
        sh1122_clear_current_screen(&plat_oled_descriptor);
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_clear_frame_buffer(&plat_oled_descriptor);
        #endif
    }
#endif
    
    /* Display error messages if something went wrong during custom fs init and bundle check */
    if ((custom_fs_init_return != RETURN_OK) || (bundle_integrity_check_return != RETURN_OK))
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"No Bundle");
        timer_start_timer(TIMER_TIMEOUT_FUNCTS, 30000);
        
        /* Wait to load bundle from USB */
        while(1)
        {
            /* Communication function */
            comms_msg_rcvd_te msg_received = comms_aux_mcu_routine(MSG_RESTRICT_ALLBUT_BUNDLE);
            
            /* If we received any message, reset timer */
            if (msg_received != NO_MSG_RCVD)
            {
                timer_start_timer(TIMER_TIMEOUT_FUNCTS, 30000);
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
            
            /* Handle possible power switches */
            logic_power_check_power_switch_and_battery(FALSE);
            
            /* Timer timeout, switch off platform */
            if ((timer_has_timer_expired(TIMER_TIMEOUT_FUNCTS, FALSE) == TIMER_EXPIRED) && (platform_io_is_usb_3v3_present() == FALSE))
            {
                /* Switch off OLED, switch off platform */
                platform_io_power_down_oled(); timer_delay_ms(200);
                platform_io_disable_switch_and_die();
            }
        }
    } 
    
    /* Set settings that may not have been set to an initial value */
    custom_fs_set_undefined_settings();
    
    /* Apply possible screen inversion */
    BOOL screen_inverted = logic_power_get_power_source() == BATTERY_POWERED?(BOOL)custom_fs_settings_get_device_setting(SETTINGS_LEFT_HANDED_ON_BATTERY):(BOOL)custom_fs_settings_get_device_setting(SETTINGS_LEFT_HANDED_ON_USB);
    sh1122_set_screen_invert(&plat_oled_descriptor, screen_inverted);    
    inputs_set_inputs_invert_bool(screen_inverted);
    
    /* Set screen brightness */
    sh1122_set_contrast_current(&plat_oled_descriptor, custom_fs_settings_get_device_setting(SETTINGS_MASTER_CURRENT));

    /* Actions for first user device boot */
    #ifdef DEVELOPER_FEATURES_ENABLED
    if ((custom_fs_get_device_flag_value(NOT_FIRST_BOOT_FLAG_ID) == FALSE) && (mcu_sp_rh_addresses[1] != 0x0201))
    #else
    if ((custom_fs_get_device_flag_value(NOT_FIRST_BOOT_FLAG_ID) == FALSE) || (custom_fs_get_device_flag_value(DEVICE_WENT_THROUGH_BOOTLOADER_FLAG_ID) != FALSE))
    #endif
    {
        /* Select language and store it as default */
        if (gui_prompts_select_language_or_keyboard_layout(FALSE, TRUE, TRUE, FALSE) != RETURN_OK)
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
        
        /* Clear frame buffer */
        sh1122_fade_into_darkness(&plat_oled_descriptor, OLED_IN_OUT_TRANS);
    }
    
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
            if (platform_io_is_usb_3v3_present_raw() != FALSE)
            {
                comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_DETACH_USB);
                timer_delay_ms(2000);
                comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_ATTACH_USB);
                logic_power_usb_enumerate_just_sent();
            }
        }   
        
        /* Disable tutorial */
        //custom_fs_set_settings_value(SETTINGS_DEVICE_TUTORIAL, FALSE);     
    }
    #endif
    
    /* If the device went through the bootloader: re-initialize device settings and clear bool */
    if (custom_fs_get_device_flag_value(DEVICE_WENT_THROUGH_BOOTLOADER_FLAG_ID) != FALSE)
    {
        custom_fs_set_device_flag_value(DEVICE_WENT_THROUGH_BOOTLOADER_FLAG_ID, FALSE);
    }
}

/*! \fn     main_reboot(void)
*   \brief  Reboot
*/
void main_reboot(void)
{
#ifndef EMULATOR_BUILD
    /* Power down actions */
    logic_power_power_down_actions();
    
    /* Wait for accelerometer DMA transfer end */
    lis2hh12_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor, TRUE);
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
#else
    exit(0);
#endif
}

/*! \fn     main_standby_sleep(void)
*   \brief  Go to sleep
*/
void main_standby_sleep(void)
{
#ifndef EMULATOR_BUILD
    if (debugger_present == FALSE)
    {
        aux_mcu_message_t* temp_rx_message;
    
        /* Only if we actually wokeup the aux mcu */
        if (comms_aux_mcu_are_comms_disabled() == FALSE)
        {
            /* Send a go to sleep message to aux MCU, wait for ack, leave no comms high (automatically set when receiving the sleep received event) */
            comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_SLEEP);
            while(comms_aux_mcu_active_wait(&temp_rx_message, FALSE, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, FALSE, AUX_MCU_EVENT_SLEEP_RECEIVED) != RETURN_OK);
            
            /* Wait for end of message we were possibly sending */
            comms_aux_mcu_wait_for_message_sent();
            
            /* Disable aux MCU dma transfers */
            dma_aux_mcu_disable_transfer();
        }
    
        /* Wait for accelerometer DMA transfer end and put it to sleep */
        lis2hh12_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor, TRUE);
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
    
        /* Clear wakeup reason */
        logic_device_clear_wakeup_reason();
        
        /* Specify that comms are disabled */
        comms_aux_mcu_set_comms_disabled();
    
        /* Enter deep sleep */
        SCB->SCR = SCB_SCR_SLEEPDEEP_Msk;
        __DSB();
        __WFI();
    
        /* Damn errata... enable interrupts */
        cpu_irq_leave_critical();
    
        /* Prepare ports for sleep exit */
        platform_io_prepare_ports_for_sleep_exit();
    
        /* Dataflash power up */
        dataflash_exit_power_down(&dataflash_descriptor);
    
        /* Send any command to DB flash to wake it up (required in case of going to sleep twice) */
        dbflash_check_presence(&dbflash_descriptor);
        
        /* Resume accelerometer processing */
        lis2hh12_sleep_exit_and_dma_arm(&plat_acc_descriptor);
    
        /* Get wakeup reason */
        platform_wakeup_reason_te wakeup_reason = logic_device_get_wakeup_reason();
        
        /* Re-enable AUX comms */
        if (wakeup_reason != WAKEUP_REASON_20M_TIMER)
        {
            comms_aux_arm_rx_and_clear_no_comms();
        }
    
        /* Switch on OLED depending on wakeup reason */
        if (wakeup_reason == WAKEUP_REASON_20M_TIMER)
        {
            /* Timer whose sole purpose is to periodically wakeup device to check battery level */
            platform_io_power_up_oled(platform_io_is_usb_3v3_present_raw());
        
            /* 100ms to measure battery's voltage, after enabling OLED stepup */
            timer_start_timer(TIMER_SCREEN, 100);
            
            /* As the platform won't be awake for long, skip the battery measurement queue logic */
            logic_power_skip_queue_logic_for_upcoming_adc_measurements();
        }
        else if (wakeup_reason == WAKEUP_REASON_AUX_MCU)
        {
            /* AUX MCU woke up our device, set a sleep timer to a low value so we can deal with the incoming packet */
            timer_start_timer(TIMER_SCREEN, SLEEP_AFTER_AUX_WAKEUP_MS);
        }
        else
        {
            /* User action: switch on OLED screen */
            platform_io_power_up_oled(platform_io_is_usb_3v3_present_raw());
            sh1122_oled_on(&plat_oled_descriptor);
            logic_device_activity_detected();
        }
    
        /* Clear wheel detection */
        inputs_clear_detections();
    }
#endif
}

/*! \fn     main(void)
*   \brief  Program Main
*/
#ifdef EMULATOR_BUILD
int minible_main(void);

int minible_main(void)
#else
int main(void)
#endif
{
    /* Initialize stack usage tracking */
    main_init_stack_tracking();

    /* Initialize our platform */
    main_platform_init();
    
    /* Activity detected */
    logic_device_activity_detected();
    
    /* Start animation */    
    if ((BOOL)custom_fs_settings_get_device_setting(SETTINGS_BOOT_ANIMATION) != FALSE)
    {
        for (uint16_t i = GUI_ANIMATION_FFRAME_ID; i < GUI_ANIMATION_NBFRAMES; i++)
        {
            timer_start_timer(TIMER_DEVICE_ACTION_TIMEOUT, 28);
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, i, FALSE);
            while(timer_has_timer_expired(TIMER_DEVICE_ACTION_TIMEOUT, TRUE) == TIMER_RUNNING)
            {
                logic_power_check_power_switch_and_battery(FALSE);
                comms_aux_mcu_routine(MSG_RESTRICT_ALL);
                logic_accelerometer_routine();
                
                /* Click to exit animation */
                if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
                {
                    i = 0x1234;
                    break;
                }
            }
        }
    }  
    
    /* Do we need to display device tutorial? */
    if ((BOOL)custom_fs_settings_get_device_setting(SETTINGS_DEVICE_TUTORIAL) != FALSE)
    {        
        /* Display tutorial */
        gui_prompts_display_tutorial();
        
        /* Tutorial displayed, reset boolean */
        custom_fs_set_settings_value(SETTINGS_DEVICE_TUTORIAL, FALSE);
    }
    
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
        /* Power routine */
        logic_power_routine();
        
        /* Check flag to be logged off */
        if (logic_user_get_and_clear_user_to_be_logged_off_flag() != FALSE)
        {
            gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_LCK, TRUE, GUI_OUTOF_MENU_TRANSITION);
            gui_dispatcher_get_back_to_current_screen();
            logic_device_set_state_changed();
            logic_smartcard_handle_removed();
            timer_delay_ms(250);
        }
        
        /* Check if we should leave management mode */
        if ((gui_dispatcher_get_current_screen() == GUI_SCREEN_MEMORY_MGMT) && (logic_security_should_leave_management_mode() != FALSE))
        {
            /* Device state is going to change... */
            logic_device_set_state_changed();
            
            /* Clear bool */
            logic_device_activity_detected();
            logic_security_clear_management_mode();
            
            /* Set next screen */
            gui_dispatcher_set_current_screen(GUI_SCREEN_MAIN_MENU, TRUE, GUI_INTO_MENU_TRANSITION);
            gui_dispatcher_get_back_to_current_screen();
            nodemgmt_scan_node_usage();
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
                logic_user_locked_feature_trigger();
            
                /* Set correct screen */
                gui_prompts_display_information_on_screen_and_wait(CARD_REMOVED_TEXT_ID, DISP_MSG_INFO, FALSE);
                gui_dispatcher_set_current_screen(GUI_SCREEN_NINSERTED, TRUE, GUI_INTO_MENU_TRANSITION);
                gui_dispatcher_get_back_to_current_screen();
            }
            
            /* USB connection timeout */
            if (logic_device_get_and_clear_usb_timeout_detected() != FALSE)
            {
                /* Assume the usb connected computer comes locked */
                logic_user_inform_computer_locked_state(TRUE, TRUE);
                
                /* Lock device */
                if (logic_security_is_smc_inserted_unlocked() != FALSE)
                {
                    gui_dispatcher_set_current_screen(GUI_SCREEN_INSERTED_LCK, TRUE, GUI_OUTOF_MENU_TRANSITION);
                    gui_dispatcher_get_back_to_current_screen();
                    logic_smartcard_handle_removed();
                    logic_device_set_state_changed();
                }
            }
            
            /* Make sure all power switches are handled before calling GUI code */
            logic_power_routine();
        
            /* GUI main loop, pass a possible virtual wheel action and reset it */
            gui_dispatcher_main_loop(virtual_wheel_action);
            virtual_wheel_action = WHEEL_ACTION_NONE;      
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
        
        /* ADC watchdog */
        if (timer_has_timer_expired(TIMER_ADC_WATCHDOG, TRUE) == TIMER_EXPIRED)
        {
            platform_io_get_voledin_conversion_result_and_trigger_conversion();
        }
        
        /* Accelerometer routine */
        BOOL is_screen_on_copy = sh1122_is_oled_on(&plat_oled_descriptor);
        acc_detection_te accelerometer_routine_return = logic_accelerometer_routine();
        if (accelerometer_routine_return == ACC_FAILING)
        {
            /* Accelerometer failing */
            if (gui_prompts_display_information_on_screen_and_wait(CONTACT_SUPPORT_001_TEXT_ID, DISP_MSG_WARNING, FALSE) == GUI_INFO_DISP_RET_LONG_CLICK)
            {
                /* Trick to not brick the device: long click still allows comms */
                while (TRUE)
                {
                    comms_aux_mcu_routine(MSG_NO_RESTRICT);
                }
            }
            gui_dispatcher_get_back_to_current_screen();
        }
        else if (accelerometer_routine_return == ACC_DET_MOVEMENT)
        {
            /* Movement was detected by the accelerometer routine */
            if ((gui_dispatcher_get_current_screen() == GUI_SCREEN_INSERTED_LCK) && (is_screen_on_copy == FALSE))
            {
                /* Card inserted and device locked, simulate wheel action to prompt PIN entering */
                virtual_wheel_action = WHEEL_ACTION_VIRTUAL;
            }
        }
        else if ((accelerometer_routine_return == ACC_INVERT_SCREEN) || (accelerometer_routine_return == ACC_NINVERT_SCREEN))
        {
            uint16_t prompt_id = accelerometer_routine_return == ACC_INVERT_SCREEN? QPROMPT_LEFT_HAND_MODE_TEXT_ID:QPROMPT_RIGHT_HAND_MODE_TEXT_ID;
            BOOL invert_bool = accelerometer_routine_return == ACC_INVERT_SCREEN? TRUE:FALSE;
            
            /* Make sure we're not harassing the user */
            if (timer_has_timer_expired(TIMER_HANDED_MODE_CHANGE, FALSE) == TIMER_EXPIRED)
            {
                sh1122_set_screen_invert(&plat_oled_descriptor, invert_bool);
                inputs_set_inputs_invert_bool(invert_bool);
                
                /* Ask the user to change mode */
                mini_input_yes_no_ret_te prompt_return = gui_prompts_ask_for_one_line_confirmation(prompt_id, FALSE, FALSE, TRUE);
                
                /* In case of power switch, do not touch anything to not confuse the user */
                if (prompt_return == MINI_INPUT_RET_POWER_SWITCH)
                {
                    /* Wait before asking again */
                    timer_start_timer(TIMER_HANDED_MODE_CHANGE, 30000);
                }
                else if (prompt_return == MINI_INPUT_RET_YES)
                {
                    /* Invert screen and inputs */
                    sh1122_set_screen_invert(&plat_oled_descriptor, invert_bool);
                    inputs_set_inputs_invert_bool(invert_bool);
                    
                    /* Store settings */
                    if (logic_power_get_power_source() == USB_POWERED)
                        custom_fs_set_settings_value(SETTINGS_LEFT_HANDED_ON_USB, (uint8_t)invert_bool);
                    else
                        custom_fs_set_settings_value(SETTINGS_LEFT_HANDED_ON_BATTERY, (uint8_t)invert_bool);
                }
                else
                {
                    /* Wait before asking again */
                    timer_start_timer(TIMER_HANDED_MODE_CHANGE, 30000);
                    sh1122_set_screen_invert(&plat_oled_descriptor, !invert_bool);
                    inputs_set_inputs_invert_bool(!invert_bool);
                }
                gui_dispatcher_get_back_to_current_screen();
            } 
            else
            {
                /* Routine wants to rotate the screen but we already asked the user not long ago, rearm the timer */
                timer_start_timer(TIMER_HANDED_MODE_CHANGE, 30000);
            }            
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

#if defined(STACK_MEASURE_ENABLED) && !defined(EMULATOR_BUILD)

uint32_t main_stack_low_water_mark = ~0U;

/*! \fn     util_check_stack_usage(void)
*   \brief  check the stack usage
*   \return current low water mark
*/
uint32_t main_check_stack_usage(void)
{
    uint32_t stack_start;
    uint32_t stack_end;
    uint32_t i;
    uint32_t curr_low_water_mark;

    /*
     * Get the pointers to the start and end of the stack. These are symbols
     * available from the compiler/linker.
     */
    stack_start = (uint32_t) &_estack;
    stack_end = (uint32_t) &_sstack;

    for (i = stack_end; i < stack_start; ++i)
    {
        uint8_t *ptr = (uint8_t *) i;
        if (*ptr != DEBUG_STACK_TRACKING_COOKIE)
        {
            break;
        }
    }
    curr_low_water_mark = i - stack_end;

    if (curr_low_water_mark < main_stack_low_water_mark)
    {
        main_stack_low_water_mark = curr_low_water_mark;
    }
    return main_stack_low_water_mark;
}

/*! \fn     main_init_stack_tracking(void)
*   \brief  Initialize stack tracking
*/
void main_init_stack_tracking(void)
{
    uint32_t stack_start;
    uint32_t stack_end;
    uint8_t *ptr;

    stack_start = utils_get_SP() - sizeof(uint32_t);
    stack_end = (uint32_t) &_sstack;

    //Inline implementation of memset since we we *might* be destroying
    //our own stack when calling the memset function.
    for (ptr = (uint8_t *) stack_end; (uint32_t) ptr < stack_start; ++ptr)
    {
        *ptr = DEBUG_STACK_TRACKING_COOKIE;
    }
}

#else

/*! \fn     main_check_stack_usage(void)
*   \brief  check the stack usage
*   \return current low water mark
*/
uint32_t main_check_stack_usage(void)
{
    return 0;
}

/*! \fn     main_init_stack_tracking(void)
*   \brief  Initialize stack tracking
*/
void main_init_stack_tracking(void) { }

#endif

