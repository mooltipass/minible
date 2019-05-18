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
#include "platform_io.h"
#include "custom_fs.h"
#include "sh1122.h"
#include "main.h"


/*! \fn     functional_testing_start(void)
*   \brief  Functional testing function
*/
void functional_testing_start(void)
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
    
    /* Start BLE... takes a little while... */
    sh1122_put_error_string(&plat_oled_descriptor, u"First boot tests...");
    logic_aux_mcu_enable_ble(TRUE);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    
    /* Get BLE ID */
    if (logic_aux_mcu_get_ble_chip_id() == 0)
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"ATBTLC1000 error!");
        while(1);
    }
    
    /* Plug card... */
    sh1122_put_error_string(&plat_oled_descriptor, u"Insert card");
    while (smartcard_low_level_is_smc_absent() == RETURN_OK);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    mooltipass_card_detect_return_te detection_result = smartcard_highlevel_card_detected_routine();
    if ((detection_result == RETURN_MOOLTIPASS_PB) || (detection_result == RETURN_MOOLTIPASS_INVALID))
    {
        sh1122_put_error_string(&plat_oled_descriptor, u"Invalid card!");
        while(1);
    }
    
    /* Connect USB... */
    sh1122_put_error_string(&plat_oled_descriptor, u"Please connect USB...");
    while (platform_io_is_usb_3v3_present() == FALSE);
    sh1122_clear_current_screen(&plat_oled_descriptor);
    
    /* Clear flag */
    custom_fs_settings_clear_first_boot_flag();
    
    /* We're good! */
    sh1122_put_error_string(&plat_oled_descriptor, u"All Good!");    
    timer_delay_ms(2000);
}