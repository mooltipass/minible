/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2019 Stephan Mathieu
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*!  \file     debug.h
*    \brief    Debug functions for our platform
*    Created:  21/04/2018
*    Author:   Mathieu Stephan
*/


#ifndef DEBUG_H_
#define DEBUG_H_

/* bitmap defines */
#define TEST_PATTERN__BITMAP_ID     794

/* Prototypes */
void debug_array_to_hex_u8string(uint8_t* array, uint8_t* string, uint16_t length);
void debug_always_bluetooth_enable_and_click_to_send_cred(void);
void debug_test_pattern_display(void);
void debug_battery_recondition(void);
void debug_kickstarter_video(void);
void debug_mcu_and_aux_info(void);
void debug_debug_animation(void);
void debug_smartcard_info(void);
void debug_setup_dev_card(void);
void debug_smartcard_test(void);
void debug_rf_freq_sweep(void);
void debug_nimh_charging(void);
void debug_language_test(void);
void debug_test_battery(void);
void debug_test_prompts(void);
void debug_debug_screen(void);
void debug_glyph_scroll(void);
void debug_reset_device(void);
void debug_atbtlc_info(void);
void debug_glyphs_show(void);
void debug_debug_menu(void);
void debug_stack_info(void);


#endif /* DEBUG_H_ */
