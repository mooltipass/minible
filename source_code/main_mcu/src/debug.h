/*!  \file     debug.h
*    \brief    Debug functions for our platform
*    Created:  21/04/2018
*    Author:   Mathieu Stephan
*/


#ifndef DEBUG_H_
#define DEBUG_H_

/* Prototypes */
void debug_array_to_hex_u8string(uint8_t* array, uint8_t* string, uint16_t length);
void debug_mcu_and_aux_info(void);
void debug_debug_animation(void);
void debug_smartcard_info(void);
void debug_language_test(void);
void debug_debug_screen(void);
void debug_debug_menu(void);


#endif /* DEBUG_H_ */