/*!  \file     functional_testing.c
*    \brief    File dedicated to the functional testing function
*    Created:  18/05/2019
*    Author:   Mathieu Stephan
*/
#include "functional_testing.h"
#include "smartcard_highlevel.h"
#include "smartcard_lowlevel.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "logic_power.h"
#include "platform_io.h"
#include "custom_fs.h"
#include "sh1122.h"
#include "inputs.h"
#include "main.h"


/*! \fn     functional_testing_start(BOOL clear_first_boot_flag)
*   \brief  Functional testing function
*   \param  clear_first_boot_flag   Set to TRUE to clear first boot flag on successful test
*/
void functional_testing_start(BOOL clear_first_boot_flag)
{
    /* Test no comms signal */
    platform_io_set_no_comms();
    if (comms_aux_mcu_send_receive_ping() == RETURN_OK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Nocomms error!");
        while(1);
    }
    platform_io_clear_no_comms();
    
    /* First boot should be done using battery */
    if (platform_io_is_usb_3v3_present() != FALSE)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Power using battery first!");
        while(1);
    }
    
    /* Check for removed card */
    if (smartcard_low_level_is_smc_absent() != RETURN_OK)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Please remove the card first!");
        while(1);
    }
    
    /* Wheel testing */
    sh1122_put_error_string(&plat_oled_descriptor, u"scroll up");
    while (inputs_get_wheel_action(FALSE, FALSE) != WHEEL_ACTION_UP);
    sh1122_put_error_string(&plat_oled_descriptor, u"scroll down");
    while (inputs_get_wheel_action(FALSE, FALSE) != WHEEL_ACTION_DOWN);
    sh1122_put_error_string(&plat_oled_descriptor, u"click");
    while (inputs_get_wheel_action(FALSE, FALSE) != WHEEL_ACTION_SHORT_CLICK);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    
    /* Ask to connect USB to test USB LDO + LDO 3V3 to 8V  */
    sh1122_put_error_string(&plat_oled_descriptor, u"connect USB");
    while (platform_io_is_usb_3v3_present() == FALSE);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    
    /* Switch to LDO for voled stepup */
    logic_power_set_power_source(USB_POWERED);
    sh1122_oled_off(&plat_oled_descriptor);
    timer_delay_ms(50);
    platform_io_power_up_oled(TRUE);
    sh1122_oled_on(&plat_oled_descriptor);    
    
    /* Start BLE... takes a little while... */
    sh1122_put_error_string(&plat_oled_descriptor, u"please wait...");
    logic_aux_mcu_enable_ble(TRUE);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    
    /* Get BLE ID */
    if (logic_aux_mcu_get_ble_chip_id() == 0)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"ATBTLC1000 error!");
        while(1);
    }
    
    /* Ask for card, tests all SMC related signals */
    sh1122_put_error_string(&plat_oled_descriptor, u"insert card");
    while (smartcard_low_level_is_smc_absent() == RETURN_OK);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    mooltipass_card_detect_return_te detection_result = smartcard_highlevel_card_detected_routine();
    if ((detection_result == RETURN_MOOLTIPASS_PB) || (detection_result == RETURN_MOOLTIPASS_INVALID))
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Invalid card!");
        while(1);
    }
    
    /* Clear flag */
    if (clear_first_boot_flag != FALSE)
    {
        custom_fs_settings_clear_first_boot_flag();
    }
    
    /* We're good! */
    sh1122_put_error_string(&plat_oled_descriptor, u"All Good!");    
    timer_delay_ms(2000);
}