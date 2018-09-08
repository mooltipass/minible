/*!  \file     debug.c
*    \brief    Debug functions for our platform
*    Created:  21/04/2018
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include <asf.h>
#include "comms_hid_msgs_debug.h"
#include "smartcard_highlevel.h"
#include "platform_defines.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "custom_fs.h"
#include "lis2hh12.h"
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
        comms_aux_mcu_routine();
        
        /* Draw menu */
        if (redraw_needed != FALSE)
        {
            /* Clear screen */
            redraw_needed = FALSE;
            #ifdef OLED_INTERNAL_FRAME_BUFFER
            sh1122_clear_frame_buffer(&plat_oled_descriptor);
            #else
            sh1122_clear_current_screen(&plat_oled_descriptor);
            #endif
            
            /* Item selection */
            if (selected_item > 7)
            {
                selected_item = 0;
            }
            else if (selected_item < 0)
            {
                selected_item = 7;
            }
            
            sh1122_put_string_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_CENTER, u"Debug Menu", TRUE);
            
            /* Print items */
            if (selected_item < 4)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Time / Accelerometer / Battery", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Language Switch Test", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Smartcard Debug", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 44, OLED_ALIGN_LEFT, u"Animation Test", TRUE);
            }
            else if (selected_item < 8)
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Main and Aux MCU Info", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Scroll Through Glyphs", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 34, OLED_ALIGN_LEFT, u"Aux MCU BLE Info", TRUE);
                
            }
            else
            {
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 14, OLED_ALIGN_LEFT, u"Main MCU Flash", TRUE);
                sh1122_put_string_xy(&plat_oled_descriptor, 10, 24, OLED_ALIGN_LEFT, u"Aux MCU Flash", TRUE);
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
                debug_language_test();
            }
            else if (selected_item == 2)
            {
                debug_smartcard_info();
            }
            else if (selected_item == 3)
            {
                debug_debug_animation();
            }
            else if (selected_item == 4)
            {
	            debug_mcu_and_aux_info();
            }
            else if (selected_item == 5)
            {
	            debug_glyph_scroll();
            }
            else if (selected_item == 6)
            {
	            debug_atbtlc_info();
            }
            else if (selected_item == 8)
            {
                custom_fs_settings_set_fw_upgrade_flag();
                cpu_irq_disable();
                NVIC_SystemReset();
            }
            else if (selected_item == 9)
            {
                logic_aux_mcu_flash_firmware_update();
            }
            redraw_needed = TRUE;
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
            sh1122_refresh_used_font(&plat_oled_descriptor);
            custom_fs_get_string_from_file(0, &temp_string);
            sh1122_clear_current_screen(&plat_oled_descriptor);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "%d/%d : ", i+1, custom_fs_flash_header.language_map_item_count);
            sh1122_put_string_xy(&plat_oled_descriptor, 30, 0, OLED_ALIGN_LEFT, custom_fs_get_current_language_text_desc(), FALSE);            
            sh1122_printf_xy(&plat_oled_descriptor, 0, 10, OLED_ALIGN_LEFT, FALSE, "String file ID: %d", custom_fs_cur_language_entry.string_file_index);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, FALSE, "Start font file ID: %d", custom_fs_cur_language_entry.starting_font);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, FALSE, "Start bitmap file ID: %d", custom_fs_cur_language_entry.starting_bitmap);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_LEFT, FALSE, "Recommended keyboard file ID: %d", custom_fs_cur_language_entry.keyboard_layout_id);
            sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_LEFT, FALSE, "Line #0:");
            sh1122_put_string_xy(&plat_oled_descriptor, 50, 50, OLED_ALIGN_LEFT, temp_string, FALSE);
            
            /* Return ? */
            timer_start_timer(TIMER_WAIT_FUNCTS, 2000);
            while (timer_has_timer_expired(TIMER_WAIT_FUNCTS, TRUE) != TIMER_EXPIRED)
            {
                if (inputs_get_wheel_action(FALSE, FALSE) == WHEEL_ACTION_SHORT_CLICK)
                {
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
        comms_aux_mcu_routine();
        
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
        if (lis2hh12_check_data_received_flag_and_arm_other_transfer(&acc_descriptor) != FALSE)
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
        sh1122_printf_xy(&plat_oled_descriptor, 0, 20, OLED_ALIGN_LEFT, TRUE, "ACC: Freq %uHz X: %i Y: %i Z: %i", acc_int_nb_interrupts_latched*32, acc_descriptor.fifo_read.acc_data_array[0].acc_x, acc_descriptor.fifo_read.acc_data_array[0].acc_y, acc_descriptor.fifo_read.acc_data_array[0].acc_z);
        
        /* Line 4: battery */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 30, OLED_ALIGN_LEFT, TRUE, "BAT: ADC %u, %u mV", bat_adc_result, bat_adc_result*110/273);
        
        /* Display stats */
        stat_times[5] = timer_get_systick();
        
        /* Line 6: display stats */
        sh1122_printf_xy(&plat_oled_descriptor, 0, 50, OLED_ALIGN_LEFT, TRUE, "STATS MS: text %u, erase %u, stats %u", stat_times[5]-stat_times[4], stat_times[1]-stat_times[0], stat_times[3]-stat_times[2]);

        #ifdef OLED_INTERNAL_FRAME_BUFFER
        sh1122_flush_frame_buffer(&plat_oled_descriptor);
        #endif
        
        /* Get user action */
        wheel_action_ret_te wheel_user_action = inputs_get_wheel_action(FALSE, FALSE);
        
        /* Go to sleep? */
        if (wheel_user_action == WHEEL_ACTION_UP)
        {
            timer_delay_ms(2000);
            main_standby_sleep();
        }   
        
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
    comms_aux_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt);
    temp_tx_message_pt->message_type = AUX_MCU_MSG_TYPE_PLAT_DETAILS;
    temp_tx_message_pt->tx_reply_request_flag = 0x0001;
    
    /* Send message */
    comms_aux_mcu_send_message(TRUE);
    
    /* Wait for message from aux MCU */
    while(comms_aux_mcu_active_wait(&temp_rx_message) == RETURN_NOK){}
        
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
    comms_aux_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt);
    temp_tx_message_pt->message_type = AUX_MCU_MSG_TYPE_PLAT_DETAILS;
    temp_tx_message_pt->tx_reply_request_flag = 0x0001;
    
    /* Send message */
    comms_aux_mcu_send_message(TRUE);
    
    /* Wait for message from aux MCU */
    while(comms_aux_mcu_active_wait(&temp_rx_message) == RETURN_NOK){}
        
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

/*! \fn     debug_glyph_scroll(void)
*   \brief  Scroll through the glyphs
*/
void debug_glyph_scroll(void)
{
    wheel_action_ret_te action_ret = WHEEL_ACTION_UP;
    uint16_t cur_glyph = 0;
    
    while (TRUE)
    {
        /* Clear screen */
        sh1122_clear_current_screen(&plat_oled_descriptor);
        
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
        while(sh1122_get_glyph_width(&plat_oled_descriptor, (cust_char_t)cur_glyph) == 0);
        
        /* Print glyph */     
        sh1122_printf_xy(&plat_oled_descriptor, 0, 0, OLED_ALIGN_LEFT, FALSE, "Glyph %d: ", cur_glyph);
        sh1122_put_char(&plat_oled_descriptor, cur_glyph, FALSE);
        
        /* Get action */
        action_ret = inputs_get_wheel_action(TRUE, FALSE);
    }
}