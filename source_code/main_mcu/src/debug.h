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
void debug_test_pattern_display(void);
void debug_mcu_and_aux_info(void);
void debug_debug_animation(void);
void debug_smartcard_info(void);
void debug_setup_dev_card(void);
void debug_smartcard_test(void);
void debug_rf_freq_sweep(void);
void debug_nimh_charging(void);
void debug_language_test(void);
void debug_test_prompts(void);
void debug_debug_screen(void);
void debug_glyph_scroll(void);
void debug_reset_device(void);
void debug_atbtlc_info(void);
void debug_glyphs_show(void);
void debug_debug_menu(void);


#endif /* DEBUG_H_ */
