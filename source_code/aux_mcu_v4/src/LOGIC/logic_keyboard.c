/*
 * logic_keyboard.c
 *
 * Created: 29/09/2019 13:58:51
 *  Author: limpkin
 */ 
#include <string.h>
#include "platform_defines.h"
#include "logic_keyboard.h"
#include "driver_timer.h"
#include "usb.h"


/*! \fn     logic_keyboard_type_key_with_modifier(uint8_t key, uint8_t modifier, uint16_t delay_between_types)
*   \brief  Perform a single keystroke
*   \param  key                 Key to send
*   \param  modifier            Modifier (alt, shift...)
*   \param  delay_between_types Delay between types in ms
*/
void logic_keyboard_type_key_with_modifier(uint8_t key, uint8_t modifier, uint16_t delay_between_types)
{    
    uint8_t hid_packet_to_be_sent[8];
    memset(hid_packet_to_be_sent, 0, sizeof(hid_packet_to_be_sent));
    
    // Send modifier
    if (modifier != 0)
    {
        hid_packet_to_be_sent[0] = modifier;
        usb_send(USB_KEYBOARD_ENDPOINT, (uint8_t*)hid_packet_to_be_sent, sizeof(hid_packet_to_be_sent));
        timer_delay_ms(delay_between_types);
    }
    
    // Send modifier + key
    hid_packet_to_be_sent[2] = key;
    usb_send(USB_KEYBOARD_ENDPOINT, (uint8_t*)hid_packet_to_be_sent, sizeof(hid_packet_to_be_sent));
    timer_delay_ms(delay_between_types);
    
    // Release all
    hid_packet_to_be_sent[0] = 0;
    hid_packet_to_be_sent[2] = 0;
    usb_send(USB_KEYBOARD_ENDPOINT, (uint8_t*)hid_packet_to_be_sent, sizeof(hid_packet_to_be_sent));
    timer_delay_ms(delay_between_types);     
}

/*! \fn     logic_keyboard_type_symbol(hid_interface_te interface, uint8_t symbol, BOOL is_dead_key, uint16_t delay_between_types)
*   \brief  Type an encoded symbol through a given interface
*   \param  interface           HID interface on which to type the symbol
*   \param  symbol              The symbol
*   \param  is_dead_key         Is the symbol a dead key?
*   \param  delay_between_types Delay between key presses
*/
void logic_keyboard_type_symbol(hid_interface_te interface, uint8_t symbol, BOOL is_dead_key, uint16_t delay_between_types)
{
    /*if (symbol == 0x0A)
    {
        // New line
        return usbKeyboardPress(KEY_RETURN, 0);
    }
    else if (symbol == 0x09)
    {
        // TAB
        return usbKeyboardPress(KEY_TAB, 0);
    }
    else*/
    {
        uint8_t masked_key = symbol & (SHIFT_MASK|ALTGR_MASK);
        
        if ((symbol & 0x3F) == KEY_EUROPE_2)
        {
            // Because of a redefine of KEY_EUROPE_2 for storage purposes we need to do that
            uint8_t mod_tbs = 0;
            
            if (masked_key == (SHIFT_MASK|ALTGR_MASK))
            {
                mod_tbs = KEY_SHIFT|KEY_RIGHT_ALT;
            }
            else if (masked_key == SHIFT_MASK)
            {
                mod_tbs = KEY_SHIFT;
            }
            else if (masked_key == ALTGR_MASK)
            {
                mod_tbs = KEY_RIGHT_ALT;
            }
            
            // Send the correct KEY_EUROPE_2 with the correct modifier
            logic_keyboard_type_key_with_modifier(KEY_EUROPE_2_REAL, mod_tbs, delay_between_types);
        }
        else if (masked_key == (SHIFT_MASK|ALTGR_MASK))
        {
            logic_keyboard_type_key_with_modifier(symbol & ~(SHIFT_MASK|ALTGR_MASK), KEY_SHIFT|KEY_RIGHT_ALT, delay_between_types);
        }
        else if (masked_key == SHIFT_MASK)
        {
            // If we need shift
            logic_keyboard_type_key_with_modifier(symbol & ~SHIFT_MASK, KEY_SHIFT, delay_between_types);
        }
        else if (masked_key == ALTGR_MASK)
        {
            // We need altgr for the numbered keys, only possible because we don't use the numerical keypad
            logic_keyboard_type_key_with_modifier(symbol & ~ALTGR_MASK, KEY_RIGHT_ALT, delay_between_types);
        }
        else
        {
            logic_keyboard_type_key_with_modifier(symbol, 0, delay_between_types);
        }
    }
    
    /* Add space if typed character is a dead key */
    if (is_dead_key != FALSE)
    {
        logic_keyboard_type_key_with_modifier(KEY_SPACE, 0, delay_between_types);        
    }
}