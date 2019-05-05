/*!  \file     debug.c
*    \brief    Debug functions for our platform
*    Created:  21/04/2018
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include <asf.h>
#include "comms_hid_msgs_debug.h"
#include "smartcard_highlevel.h"
#include "smartcard_lowlevel.h"
#include "platform_defines.h"
#include "logic_smartcard.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "gui_prompts.h"
#include "platform_io.h"
#include "logic_power.h"
#include "custom_fs.h"
#include "nodemgmt.h"
#include "lis2hh12.h"
#include "text_ids.h"
#include "sh1122.h"
#include "inputs.h"
#include "debug.h"
#include "main.h"
#include "dma.h"


/*! \fn     debug_array_to_hex_u8string(uint8_t* array, uint8_t* string, uint16_t length)
*   \brief  Generate a string to display an array contents
*   \param  array   Pointer to the array
*   \param  string  Pointer to the u8 string
*   \param  length  Length of the array
*   \note   string length should be twice the size of the array
*/
void debug_array_to_hex_u8string(uint8_t* array, uint8_t* string, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++)
    {
        sprintf((char*)&string[i*2], "%02x", array[i]);
    }
}

/*! \fn     debug_test_prompts(void)
*   \brief  Display several prompts or notifications to check code
*/
void debug_test_prompts(void)
{
    volatile uint16_t pinpin;
    
    /* Set language */    
    custom_fs_set_current_language(0);
    
    /* One line prompt */
    gui_prompts_ask_for_one_line_confirmation(CREATE_NEW_USER_TEXT_ID, TRUE);
        
    /* 2 lines prompt */
    cust_char_t* two_line_prompt_1 = (cust_char_t*)u"Delete Service?";
    cust_char_t* two_line_prompt_2 = (cust_char_t*)u"myreallysuperextralongwebsite.com";
    confirmationText_t conf_text_2_lines = {.lines[0]=two_line_prompt_1, .lines[1]=two_line_prompt_2};
    gui_prompts_ask_for_confirmation(2, &conf_text_2_lines, TRUE);
    
    /* 3 lines prompt */
    cust_char_t* three_line_prompt_1 = (cust_char_t*)u"themooltipass.com";
    cust_char_t* three_line_prompt_2 = (cust_char_t*)u"Login With:";
    cust_char_t* three_line_prompt_3 = (cust_char_t*)u"mysuperextralonglogin@mydomain.com";
    confirmationText_t conf_text_3_lines = {.lines[0]=three_line_prompt_1, .lines[1]=three_line_prompt_2, .lines[2]=three_line_prompt_3};
    gui_prompts_ask_for_confirmation(3, &conf_text_3_lines, TRUE);
    
    /* 4 lines prompt */
    cust_char_t* four_line_prompt_1 = (cust_char_t*)u"SSH Daemon";
    cust_char_t* four_line_prompt_2 = (cust_char_t*)u"myserver.com";
    cust_char_t* four_line_prompt_3 = (cust_char_t*)u"Login With:";
    cust_char_t* four_line_prompt_4 = (cust_char_t*)u"mysuperextralonglogin@mydomain.com";
    confirmationText_t conf_text_4_lines = {.lines[0]=four_line_prompt_1, .lines[1]=four_line_prompt_2, .lines[2]=four_line_prompt_3, .lines[3]=four_line_prompt_4};
    gui_prompts_ask_for_confirmation(4, &conf_text_4_lines, TRUE);
    
    /* Notifications */
    gui_prompts_display_information_on_screen_and_wait(35, DISP_MSG_INFO);
    gui_prompts_display_information_on_screen_and_wait(35, DISP_MSG_ACTION);
    gui_prompts_display_information_on_screen_and_wait(35, DISP_MSG_WARNING);
    
    /* Pin get */
    gui_prompts_get_user_pin(&pinpin, 32);    
}

/*! \fn     debug_debug_menu(void)
*   \brief  Debug menu
*/
void debug_debug_menu(void)
{
    wheel_action_ret_te wheel_user_action;
    int16_t selected_item = 0;
    BOOL redraw_needed = TRUE;
    
    while(1)
    {
        /* Still deal with comms */
        comms_aux_mcu_routine(MSG_NO_RESTRICT);
        
        /* Draw menu */
        if (redraw_needed != FALSE)
        {
            /* Set Emergency Font */
            sh1122_set_emergency_font(&plat_oled_descriptor);
            
            /* Clear screen */
            redraw_needed = FALSE;
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            #else
            sh1122_clear_current_screen(&plat_oled_descriptor);
            #endif
            
            /* Item selection */
            if (selected_item > 15)
            {
                selected_item = 0;
            }
            else if (selected_item < 0)
            {
                selected_item = 15;
            }
            
            sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Debug Menu", TRUE);
            
            /* Print items */
            if (selected_item < 4)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Time / Accelerometer / Battery", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Main and Aux MCU Info", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Aux MCU BLE Info", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"NiMH Charging", TRUE);
            }
            else if (selected_item < 8)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Display All Fonts Glyphs", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Scroll Through Glyphs", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Language Switch Test", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"Animation Test", TRUE);            
            }
            else if (selected_item < 12)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Smartcard Debug", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Smartcard Test", TRUE);                
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Main MCU Flash", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"Aux MCU Flash", TRUE);
            }
            else
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Switch Off", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Sleep", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Leave Menu", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"Setup Developper Card", TRUE);
            }
            
            /* Cursor */
            sh1122_put_string_xy(&plat_oled_descriptor, 0, 14 + (selected_item%4)*10, OLED_ALIGN_LEFT, u"-", TRUE);
            
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_flush_frame_buffer(&plat_oled_descriptor);
            #endif
        }
        
        /* Get user action */
        wheel_user_action = inputs_get_wheel_action(FALSE, FALSE);
        
        /* action depending on scroll */
        if (wheel_user_action == WHEEL_ACTION_UP)
        {
            redraw_needed = TRUE;
            selected_item--;
        }
        else if (wheel_user_action == WHEEL_ACTION_DOWN)
        {
            redraw_needed = TRUE;
            selected_item++;
        }
        else if (wheel_user_action == WHEEL_ACTION_SHORT_CLICK)
        {
            if (selected_item == 0)
            {
                debug_debug_screen();
            }
            else if (selected_item == 1)
            {
                debug_mcu_and_aux_info();
            }
            else if (selected_item == 2)
            {
                debug_atbtlc_info();
            }
            else if (selected_item == 3)
            {
                debug_nimh_charging();
            }
            else if (selected_item == 4)
            {
                debug_glyphs_show();
            }
            else if (selected_item == 5)
            {
                debug_glyph_scroll();
            }
            else if (selected_item == 6)
            {
                debug_language_test();
            }
            else if (selected_item == 7)
            {
                debug_debug_animation();
            }
            else if (selected_item == 8)
            {
                debug_smartcard_info();
            }
            else if (selected_item == 9)
            {
                debug_smartcard_test();
            }
            else if (selected_item == 10)
            {
                custom_fs_settings_set_fw_upgrade_flag();
                cpu_irq_disable();
                NVIC_SystemReset();
            }
            else if (selected_item == 11)
            {
                logic_aux_mcu_flash_firmware_update();
            }
            else if (selected_item == 12)
            {
                sh1122_oled_off(&plat_oled_descriptor);     // Display off command    
                platform_io_power_down_oled();              // Switch off stepup            
                platform_io_set_wheel_click_pull_down();    // Pull down on wheel click to slowly discharge capacitor
                timer_delay_ms(100);                        // From OLED datasheet wait before removing 3V3
                platform_io_set_wheel_click_low();          // Completely discharge cap
                timer_delay_ms(10);                         // Wait a tad
                platform_io_disable_switch_and_die();       // Die!
            }
            else if (selected_item == 13)
            {
                main_standby_sleep();
            }
            else if (selected_item == 14)
            {
                return;
            }
            else if (selected_item == 15)
            {
                debug_setup_dev_card();
            }
            redraw_needed = TRUE;
        }
    }
}

/*! \fn     debug_setup_dev_card(void)
*   \brief  Setup a smartcard to ease developer's work
*/
void debug_setup_dev_card(void)
{
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Please insert a blank card", FALSE);
    
    while(1)
    {
        /* Return ? */
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
        
        if (smartcard_lowlevel_is_card_plugged() == RETURN_JDETECT)
        {
            /* Get detection result */
            mooltipass_card_detect_return_te detection_result = smartcard_highlevel_card_detected_routine();
            
            /* Card debug info */
            if (detection_result == RETURN_MOOLTIPASS_BLANK)
            {
                volatile uint16_t pin_code = SMARTCARD_DEFAULT_PIN;
                uint8_t temp_buffer[AES_KEY_LENGTH/8];
                
                /* all arrays set to 0, pin code set to default smartcard pin code */
                memset(temp_buffer, 0, sizeof(temp_buffer));                
                
                /* Special user for special card has ID 100 */
                nodemgmt_format_user_profile(100);
                
                /* Setup smartcard */
                smartcard_highlevel_write_protected_zone(temp_buffer);
                smartcard_highlevel_write_aes_key(temp_buffer);
                smartcard_highlevel_write_security_code(&pin_code);

                /* Special card has 0000 CPZ, set 0000 as nonce */
                cpz_lut_entry_t special_user_profile;
                memset(&special_user_profile, 0, sizeof(special_user_profile));
                special_user_profile.user_id = 100;
                custom_fs_store_cpz_entry(&special_user_profile, special_user_profile.user_id);             
            }
            else
            {
                return;
            }
            
            /* Remove power to the card */
            platform_io_smc_remove_function();
            
            /* Inform user */
            sh1122_put_string_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_CENTER, u"Done! Please reboot device!", FALSE);
            timer_delay_ms(2000);
            return;
        }
    }    
}

/*! \fn     debug_smartcard_test(void)
*   \brief  Smartcard Read/Write test
*/
void debug_smartcard_test(void)
{
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Please insert a blank card", FALSE);
    
    while(1)
    {
        /* Return ? */
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
        
        if (smartcard_lowlevel_is_card_plugged() == RETURN_JDETECT)
        {            
            /* Get detection result */
            mooltipass_card_detect_return_te detection_result = smartcard_highlevel_card_detected_routine();
            
            /* Card debug info */
            if (detection_result == RETURN_MOOLTIPASS_BLANK)
            {
                uint8_t random_data[256/8];
                uint16_t success_count = 0;
                
                /* Perform a 100 read / write tests */
                for (uint16_t i = 0; i < 100; i++)
                {
                    /* Fill buffer with accelerometer data */
                    while(lis2hh12_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor) == FALSE);
                    memcpy((void*)random_data, plat_acc_descriptor.fifo_read.acc_data_array, sizeof(random_data));
                    
                    /* Stats show */
                    sh1122_clear_current_screen(&plat_oled_descriptor);
                    sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Testing...", FALSE);
                    sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, FALSE, "%d/%d OK", success_count, i);
                    
                    /* Reset card */
                    smartcard_lowlevel_erase_application_zone1_nzone2(FALSE);
                    smartcard_lowlevel_erase_application_zone1_nzone2(TRUE);
                    smartcard_highlevel_set_authenticated_readwrite_to_zone1and2();
                    
                    /* Fill random bytes */
                    if (smartcard_highlevel_write_aes_key(random_data) != RETURN_NOK)
                    {
                        success_count++;
                    }                   
                }
            }
            else
            {
                return;
            }
            
            /* Remove power to the card */
            platform_io_smc_remove_function();
        }
    }
}    

/*! \fn     debug_smartcard_info(void)
*   \brief  Smartcard Debug Info
*/
void debug_smartcard_info(void)
{
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Please insert card", FALSE);
    
    while(1)
    {
        /* Return ? */
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
        
        if (smartcard_lowlevel_is_card_plugged() == RETURN_JDETECT)
        {
            /* Erase screen */
            sh1122_clear_current_screen(&plat_oled_descriptor);
            
            /* Get detection result */
            mooltipass_card_detect_return_te detection_result = smartcard_highlevel_card_detected_routine();
            
            /* Inform what the card is */
            if (detection_result == RETURN_MOOLTIPASS_INVALID)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Invalid Card", FALSE);
            }
            else if (detection_result == RETURN_MOOLTIPASS_PB)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Problem with card", FALSE);
            }
            else if (detection_result == RETURN_MOOLTIPASS_BLANK)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Blank card", FALSE);
            }
            else if (detection_result == RETURN_MOOLTIPASS_USER)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"User card", FALSE);
            }    
            else if (detection_result == RETURN_MOOLTIPASS_BLOCKED)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Blocked card", FALSE);
            }    
            
            /* Card debug info */
            if (detection_result != RETURN_MOOLTIPASS_INVALID)
            {                
                uint8_t temp_string[40];
                uint8_t data_buffer[20];
                
                /* Security mode */
                sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, FALSE, "Security mode: %c", (smartcard_highlevel_check_security_mode2() == RETURN_OK) ? '2' : '1');
                
                /* Fabrication zone / Memory test zone / Manufacturer zone */
                smartcard_highlevel_read_fab_zone(data_buffer);
                uint16_t fz = (uint16_t)data_buffer[0] | (uint16_t)data_buffer[1]<<8;
                smartcard_highlevel_read_mem_test_zone(data_buffer);
                uint16_t mtz = (uint16_t)data_buffer[0] | (uint16_t)data_buffer[1]<<8;
                smartcard_highlevel_read_manufacturer_zone(data_buffer);
                uint16_t mz = (uint16_t)data_buffer[1] | (uint16_t)data_buffer[0]<<8;
                sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, FALSE, "FZ: %04X / MTZ: %04X / MZ: %04X", fz, mtz, mz);
                
                /* Issuer zone */
                strcpy((char*)temp_string, "IZ: ");
                smartcard_highlevel_read_issuer_zone(data_buffer);
                debug_array_to_hex_u8string(data_buffer, temp_string + 4, SMARTCARD_ISSUER_ZONE_LGTH);
                sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, FALSE, (const char*)temp_string);
                
                /* Code protected zone */
                strcpy((char*)temp_string, "CPZ: ");
                smartcard_highlevel_read_code_protected_zone(data_buffer);
                debug_array_to_hex_u8string(data_buffer, temp_string + 5, SMARTCARD_CPZ_LENGTH);
                sh1122_printf_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_LEFT, FALSE, (const char*)temp_string);
                
                /* First bytes of AZ1 */
                strcpy((char*)temp_string, "AZ1: ");
                smartcard_lowlevel_read_smc((SMARTCARD_AZ1_BIT_START + 16*8)/8, (SMARTCARD_AZ1_BIT_START)/8, data_buffer);
                debug_array_to_hex_u8string(data_buffer, temp_string + 5, 16);
                sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_LEFT, FALSE, (const char*)temp_string);
            }            
            
            /* Remove power to the card */
            platform_io_smc_remove_function();         
        }              
    }
}

/*! \fn     debug_language_test(void)
*   \brief  Language test
*/
void debug_language_test(void)
{
    /* Language feature test */
    while(1)
    {
        cust_char_t* temp_string;
        for (uint16_t i = 0; i < custom_fs_get_number_of_languages(); i++)
        {
            custom_fs_set_current_language(i);
            custom_fs_get_string_from_file(0, &temp_string, FALSE);
            sh1122_set_emergency_font(&plat_oled_descriptor);
            sh1122_clear_current_screen(&plat_oled_descriptor);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "%d/%d : ", i+1, custom_fs_flash_header.language_map_item_count);
            sh1122_put_string_xy(&plat_oled_descriptor, 30, 0, OLED_ALIGN_LEFT, custom_fs_get_current_language_text_desc(), FALSE);            
            sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, FALSE, "String file ID: %d", custom_fs_cur_language_entry.string_file_index);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, FALSE, "Start font file ID: %d", custom_fs_cur_language_entry.starting_font);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, FALSE, "Start bitmap file ID: %d", custom_fs_cur_language_entry.starting_bitmap);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_LEFT, FALSE, "Recommended keyboard file ID: %d", custom_fs_cur_language_entry.keyboard_layout_id);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_LEFT, FALSE, "Line #0:");
            sh1122_refresh_used_font(&plat_oled_descriptor, FONT_UBUNTU_MEDIUM_15_ID);
            sh1122_put_string_xy(&plat_oled_descriptor, 50, 50, OLED_ALIGN_LEFT, temp_string, FALSE);
            
            /* Return ? */
            timer_start_timer(TIMER_WAIT_FUNCTS, 2000);
            while (timer_has_timer_expired(TIMER_WAIT_FUNCTS, TRUE) != TIMER_EXPIRED)
            {
                if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
                {
                    sh1122_set_emergency_font(&plat_oled_descriptor);
                    return;
                }                
            }
        }
    }
}

/*! \fn     debug_debug_animation(void)
*   \brief  Debug animation
*/
void debug_debug_animation(void)
{
    while (1)
    {
        for (uint16_t i = 0; i < 120; i++)
        {
            sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, i, FALSE);
            
            /* Return ? */
            if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
            {
                return;
            }
        }
    }
}

/*! \fn     debug_debug_screen(void)
*   \brief  Debug screen
*/
void debug_debug_screen(void)
{        
    /* Debug variables */
    uint32_t acc_int_nb_interrupts_latched = 0;
    uint32_t acc_int_nb_interrupts = 0;
    uint16_t bat_adc_result = 0;
    calendar_t temp_calendar;
    uint32_t stat_times[6];    
    timer_get_calendar(&temp_calendar);
    uint32_t last_stat_s = temp_calendar.bit.SECOND;
    
    while(1)
    {
        /* Deal with comms */
        comms_aux_mcu_routine(MSG_NO_RESTRICT);
        
        /* Clear screen */
        stat_times[0] = timer_get_systick();
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_check_for_flush_and_terminate(&plat_oled_descriptor);
        sh1122_clear_frame_buffer(&plat_oled_descriptor);
        #else
        sh1122_clear_current_screen(&plat_oled_descriptor);
        #endif
        stat_times[1] = timer_get_systick();
        
        /* Data acq */
        stat_times[2] = timer_get_systick();
        
        /* Accelerometer interrupt */
        if (lis2hh12_check_data_received_flag_and_arm_other_transfer(&plat_acc_descriptor) != FALSE)
        {
            acc_int_nb_interrupts++;
        }
        
        /* Battery measurement */
        if (platform_io_is_voledin_conversion_result_ready() != FALSE)
        {
            bat_adc_result = platform_io_get_voledin_conversion_result_and_trigger_conversion();
        }
        
        /* Get calendar */
        timer_get_calendar(&temp_calendar);
        
        /* Check for Accelerometer SERCOM buffer overflow */   
        if (ACC_SERCOM->SPI.STATUS.bit.BUFOVF != 0)
        {
            sh1122_put_error_string(&plat_oled_descriptor, u"ACC Overflow");      
        }
        
        /* Check for aux comms SERCOM buffer overflow */
        if (AUXMCU_SERCOM->SPI.STATUS.bit.BUFOVF != 0)
        {
            sh1122_put_error_string(&plat_oled_descriptor, u"AUX COM Overflow");      
        }
        
        /* End data acq */
        stat_times[3] = timer_get_systick();
        
        /* Display stats */
        stat_times[4] = timer_get_systick();
        
        /* Stats latched at second changes */        
        if (temp_calendar.bit.SECOND != last_stat_s)
        {
            acc_int_nb_interrupts_latched = acc_int_nb_interrupts;
            last_stat_s = temp_calendar.bit.SECOND;
            acc_int_nb_interrupts = 0;
        }
         
        /* Line 2: date */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, TRUE, "CURRENT TIME: %u:%u:%u %u/%u/%u", temp_calendar.bit.HOUR, temp_calendar.bit.MINUTE, temp_calendar.bit.SECOND, temp_calendar.bit.DAY, temp_calendar.bit.MONTH, temp_calendar.bit.YEAR);
        
        /* Line 3: accelerometer */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, TRUE, "ACC: Freq %uHz X: %i Y: %i Z: %i", acc_int_nb_interrupts_latched*32, plat_acc_descriptor.fifo_read.acc_data_array[0].acc_x, plat_acc_descriptor.fifo_read.acc_data_array[0].acc_y, plat_acc_descriptor.fifo_read.acc_data_array[0].acc_z);
        
        /* Line 4: battery */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, TRUE, "BAT: ADC %u, %u mV", bat_adc_result, bat_adc_result*110/273);
        
        /* Line 5: MCU system timer */
        uint32_t systick_val;
        timer_get_mcu_systick(&systick_val);
        sh1122_printf_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_LEFT, TRUE, "MCU systick: %u", systick_val);
        
        /* Display stats */
        stat_times[5] = timer_get_systick();
        
        /* Line 6: display stats */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_LEFT, TRUE, "STATS MS: text %u, erase %u, stats %u", stat_times[5]-stat_times[4], stat_times[1]-stat_times[0], stat_times[3]-stat_times[2]);

        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_flush_frame_buffer(&plat_oled_descriptor);
        #endif
        
        /* Get user action */
        wheel_action_ret_te wheel_user_action = inputs_get_wheel_action(FALSE, FALSE);  
        
        /* Return ? */
        if (wheel_user_action == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
    }
}

/*! \fn     debug_mcu_and_aux_info(void)
*   \brief  Print info about main & aux MCU
*/
void debug_mcu_and_aux_info(void)
{
    aux_mcu_message_t* temp_rx_message;
    aux_mcu_message_t* temp_tx_message_pt;
    
    /* Get part number */
    char part_number[20] = "unknown";
    if (DSU->DID.bit.DEVSEL == 0x05)
    {
        strcpy(part_number, "ATSAMD21G18A");
    }
    
    /* Print info */
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "Main MCU, fw %d.%d", FW_MAJOR, FW_MINOR);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, FALSE, "DID 0x%08x (%s), rev %c", DSU->DID.reg, part_number, 'A' + DSU->DID.bit.REVISION);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, FALSE, "UID: 0x%08x%08x%08x%08x", *(uint32_t*)0x0080A00C, *(uint32_t*)0x0080A040, *(uint32_t*)0x0080A044, *(uint32_t*)0x0080A048);
    
    /* Prepare status message request */
    comms_aux_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_PLAT_DETAILS);
    
    /* Send message */
    comms_aux_mcu_send_message(TRUE);
    
    /* Wait for message from aux MCU */
    while(comms_aux_mcu_active_wait(&temp_rx_message, FALSE, AUX_MCU_MSG_TYPE_PLAT_DETAILS) == RETURN_NOK){}
        
    /* Cast aux MCU DID */
    DSU_DID_Type aux_mcu_did;
    aux_mcu_did.reg = temp_rx_message->aux_details_message.aux_did_register;
    
    /* From DID, get part number */
    if (aux_mcu_did.bit.DEVSEL == 0x0B)
    {
        strcpy(part_number, "ATSAMD21E17A");
    }
    if (aux_mcu_did.bit.DEVSEL == 0x0A)
    {
        strcpy(part_number, "ATSAMD21E18A");
    }
    else
    {
        strcpy(part_number, "unknown");
    }    
        
    /* This is debug, no need to check if it is the correct received message */
    sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, FALSE, "Aux MCU fw %d.%d", temp_rx_message->aux_details_message.aux_fw_ver_major, temp_rx_message->aux_details_message.aux_fw_ver_minor);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_LEFT, FALSE, "DID 0x%08x (%s), rev %c", aux_mcu_did.reg, part_number, 'A' + aux_mcu_did.bit.REVISION);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_LEFT, FALSE, "UID: 0x%08x%08x%08x%08x", temp_rx_message->aux_details_message.aux_uid_registers[0], temp_rx_message->aux_details_message.aux_uid_registers[1], temp_rx_message->aux_details_message.aux_uid_registers[2], temp_rx_message->aux_details_message.aux_uid_registers[3]);
    
    /* Info printed, rearm DMA RX */
    comms_aux_arm_rx_and_clear_no_comms();
    
    /* Check for click to return */
    while(1)
    {
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
    }
}

/*! \fn     debug_atbtlc_info(void)
*   \brief  Print info about aux MCU ATBTLC1000
*/
void debug_atbtlc_info(void)
{    
    aux_mcu_message_t* temp_rx_message;   
    aux_mcu_message_t* temp_tx_message_pt;
    
    /* Enable BLE */
    logic_aux_mcu_enable_ble(TRUE);
    
    /* Generate our packet */
    comms_aux_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_PLAT_DETAILS);
    
    /* Send message */
    comms_aux_mcu_send_message(TRUE);
    
    /* Wait for message from aux MCU */
    while(comms_aux_mcu_active_wait(&temp_rx_message, FALSE, AUX_MCU_MSG_TYPE_PLAT_DETAILS) == RETURN_NOK){}
        
    /* Output debug info */
    sh1122_clear_current_screen(&plat_oled_descriptor);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 00, OLED_ALIGN_LEFT, FALSE, "BluSDK Lib: %X.%X", temp_rx_message->aux_details_message.blusdk_lib_maj, temp_rx_message->aux_details_message.blusdk_lib_min);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, FALSE, "BluSDK Fw: %X.%X.%X", temp_rx_message->aux_details_message.blusdk_fw_maj, temp_rx_message->aux_details_message.blusdk_fw_min, temp_rx_message->aux_details_message.blusdk_fw_build);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, FALSE, "ATBTLC RF Ver: 0x%8X", (unsigned int)temp_rx_message->aux_details_message.atbtlc_rf_ver);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, FALSE, "ATBTLC Chip ID: 0x%6X", (unsigned int)temp_rx_message->aux_details_message.atbtlc_chip_id);
    sh1122_printf_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_LEFT, FALSE, "ATBTLC Addr: 0x%02X%02X%02X%02X%02X%02X", temp_rx_message->aux_details_message.atbtlc_address[5], temp_rx_message->aux_details_message.atbtlc_address[4], temp_rx_message->aux_details_message.atbtlc_address[3], temp_rx_message->aux_details_message.atbtlc_address[2], temp_rx_message->aux_details_message.atbtlc_address[1], temp_rx_message->aux_details_message.atbtlc_address[0]);

    /* Info printed, rearm DMA RX */
    comms_aux_arm_rx_and_clear_no_comms();
    
    /* Check for click to return */
    while(1)
    {
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
    }
}

/*! \fn     debug_glyphs_show(void)
*   \brief  Show all glyphs for a given font
*/
void debug_glyphs_show(void)
{
    wheel_action_ret_te action_ret = WHEEL_ACTION_NONE;
    uint16_t current_char = ' ';
    uint8_t current_font = 0;

    /* Allow going to next line */
    plat_oled_descriptor.line_feed_allowed = TRUE;
    
    while (TRUE)
    {
        /* Clear screen */
        sh1122_clear_current_screen(&plat_oled_descriptor);

        /* Set Emergency Font */
        sh1122_set_emergency_font(&plat_oled_descriptor);

        /* Show current font ID */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "Font #%d: ", current_font);

        /* Set current font */
        sh1122_refresh_used_font(&plat_oled_descriptor, current_font);
        
        /* Dirty hack: set '?' support to FALSE so non supported chars aren't replaced with it */
        plat_oled_descriptor.question_mark_support_described = FALSE;

        /* Set XY for following chars */
        plat_oled_descriptor.cur_text_x = 50;
        plat_oled_descriptor.cur_text_y = 0;

        /* Display all chars */
        while(sh1122_put_char(&plat_oled_descriptor, current_char++, FALSE) >= 0);
        
        /* Get action */
        action_ret = inputs_get_wheel_action(TRUE, FALSE);
        
        /* Exit ? */
        if (action_ret == WHEEL_ACTION_LONG_CLICK)
        {
            plat_oled_descriptor.question_mark_support_described = TRUE;
            sh1122_set_emergency_font(&plat_oled_descriptor);
            plat_oled_descriptor.line_feed_allowed = FALSE;
            return;
        }
        else if (action_ret == WHEEL_ACTION_UP)
        {
            while(sh1122_refresh_used_font(&plat_oled_descriptor, --current_font) != RETURN_OK);
            current_char = ' ';
        }
        else if (action_ret == WHEEL_ACTION_DOWN)
        {
            while(sh1122_refresh_used_font(&plat_oled_descriptor, ++current_font) != RETURN_OK);
            current_char = ' ';
        }
    }
}

/*! \fn     debug_glyph_scroll(void)
*   \brief  Scroll through the glyphs
*/
void debug_glyph_scroll(void)
{
    wheel_action_ret_te action_ret = WHEEL_ACTION_UP;
    uint8_t current_font = 0;
    uint16_t temp_uint16 = 0;
    uint16_t cur_glyph = ' ';
    
    /* Dirty hack: set '?' support to FALSE so non supported chars aren't replaced with it */
    plat_oled_descriptor.question_mark_support_described = FALSE;
    
    while (TRUE)
    {        
        /* Find a glyph to print */
        do
        {
            if (action_ret == WHEEL_ACTION_DOWN)
            {
                cur_glyph--;
            }
            else
            {
                cur_glyph++;
            }
        }
        while(sh1122_get_glyph_width(&plat_oled_descriptor, (cust_char_t)cur_glyph, &temp_uint16) == 0);
        
        /* Clear screen */
        sh1122_clear_current_screen(&plat_oled_descriptor);
        
        /* Set Emergency Font */
        sh1122_set_emergency_font(&plat_oled_descriptor);
        
        /* Print glyph */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "Font %d, glyph %d: ", current_font, cur_glyph);

        /* Set current font */
        sh1122_refresh_used_font(&plat_oled_descriptor, current_font);
                
        /* Print glyph */
        sh1122_put_char(&plat_oled_descriptor, cur_glyph, FALSE);
        
        /* Get action */
        action_ret = inputs_get_wheel_action(TRUE, FALSE);
        
        /* Exit ? */
        if (action_ret == WHEEL_ACTION_LONG_CLICK)
        {
            plat_oled_descriptor.question_mark_support_described = TRUE;
            sh1122_set_emergency_font(&plat_oled_descriptor);
            return;
        }
        else if (action_ret == WHEEL_ACTION_SHORT_CLICK)
        {
            while(sh1122_refresh_used_font(&plat_oled_descriptor, ++current_font) != RETURN_OK);
            cur_glyph = ' ';
        }
    }
}

/*! \fn     debug_nimh_charging(void)
*   \brief  NiMH charge with debug info
*/
void debug_nimh_charging(void)
{
    int16_t ystarts[256];
    uint16_t sec_counter = 59;
    uint16_t current_index = 0;
    aux_mcu_message_t* temp_rx_message;
    aux_mcu_message_t* temp_tx_message_pt;
    
    /* Clear screen */
    sh1122_clear_current_screen(&plat_oled_descriptor);
    memset((void*)ystarts, 63, sizeof(ystarts));
    
    /* Send battery charge message */
    if (logic_power_get_power_source() == USB_POWERED)
    {
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_NIMH_CHARGE);
    }
    
    /* Arm charge status request timer */
    timer_start_timer(TIMER_TIMEOUT_FUNCTS, 1000);
    
    /* Local vars */
    uint16_t bat_mv = 0;
    
    while(TRUE)
    {   
        /* Battery measurement */
        if (platform_io_is_voledin_conversion_result_ready() != FALSE)
        {
            bat_mv = platform_io_get_voledinmv_conversion_result_and_trigger_conversion();
        }
        
        /* Send a packet to aux MCU? */
        if (timer_has_timer_expired(TIMER_TIMEOUT_FUNCTS, TRUE) == TIMER_EXPIRED)
        {            
            /* Generate our packet */
            comms_aux_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_NIMH_CHARGE);
            
            /* Send message */
            comms_aux_mcu_send_message(TRUE);
            
            /* Wait for message from aux MCU */
            while(comms_aux_mcu_active_wait(&temp_rx_message, FALSE, AUX_MCU_MSG_TYPE_NIMH_CHARGE) == RETURN_NOK){}
            
            /* Clear screen */
            sh1122_clear_current_screen(&plat_oled_descriptor);
            
            /* Debug info */
            if (logic_power_get_power_source() == BATTERY_POWERED)
            {
                sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "MCU Vbat: %umV", bat_mv);
            } 
            else
            {
                if (temp_rx_message->nimh_charge_message.charge_status == LB_CHARGING_DONE)
                {
                    sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "Charging done");
                } 
                else
                {
                    sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "%u - VBat: %umV - Cur %i", temp_rx_message->nimh_charge_message.charge_status, (temp_rx_message->nimh_charge_message.battery_voltage*103)>>8, temp_rx_message->nimh_charge_message.charge_current);
                }
            }
                
            /* Info printed, rearm DMA RX */
            comms_aux_arm_rx_and_clear_no_comms();
            
            /* Time to update our graph? */
            if (sec_counter++ == 59)
            {
                sec_counter = 0;
                
                if (current_index == 255)
                {
                    /* Shift bars */
                    for (uint16_t i = 0; i < 255; i++)
                    {
                        ystarts[i] = ystarts[i+1];
                    }
                } 
                
                /* Store data */
                if (logic_power_get_power_source() == BATTERY_POWERED)
                {
                    ystarts[current_index] = 63-((bat_mv-1100)>>3);
                }
                else
                {
                    ystarts[current_index] = 63-((((temp_rx_message->nimh_charge_message.battery_voltage*103)>>8)-1100)>>3);
                }                
                
                /* Increment index */
                if (current_index != 255)
                {
                    current_index++;
                }                    
            }
            
            /* Draw graph */
            for (uint16_t x = 0; x < 256; x++)
            {
                sh1122_draw_vertical_line(&plat_oled_descriptor, x, ystarts[x], 63, 0x04, FALSE);
            }
            
            if (temp_rx_message->nimh_charge_message.charge_status == LB_CHARGING_DONE)
            {
                while (TRUE)
                {
                    if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
                    {
                        return;
                    }
                }
            }
            
            /* Arm charge status request timer */
            timer_start_timer(TIMER_TIMEOUT_FUNCTS, 1000);       
        }
        
        if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
        {
            return;
        }
    }
}
