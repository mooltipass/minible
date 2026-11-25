/*
 * logic_keyboard.c
 *
 * Created: 29/09/2019 13:58:51
 *  Author: limpkin
 */ 
#include <string.h>
#include "platform_defines.h"
#include "logic_bluetooth.h"
#include "logic_keyboard.h"
#include "driver_timer.h"
#include "usb.h"
#include "udc.h"
/* Buffer containing the keys to be sent through USB */
uint8_t logic_keyboard_usb_hid_keys_buffer[8];


/*! \fn     logic_keyboard_type_lock_shortcut(hid_interface_te interface_id, uint8_t l_symbol)
*   \brief  Type the lock shortcut
*   \param  interface           HID interface on which to type the symbol
*   \param  symbol              The l symbol
*/
void logic_keyboard_type_lock_shortcut(hid_interface_te interface_id, uint8_t l_symbol)
{
    /* Check for enumeration */
    if ((interface_id == USB_INTERFACE) && ((usb_get_config() == 0) || (udc_get_nb_ms_before_last_usb_activity() > 100)))
    {
        return;
    }
    
    if (interface_id == USB_INTERFACE)
    {
        logic_keyboard_usb_hid_keys_buffer[2] = KEY_WIN_L;
        usb_send(USB_KEYBOARD_ENDPOINT, (uint8_t*)logic_keyboard_usb_hid_keys_buffer, sizeof(logic_keyboard_usb_hid_keys_buffer));
        timer_delay_ms(100);
        logic_keyboard_usb_hid_keys_buffer[3] = l_symbol;
        usb_send(USB_KEYBOARD_ENDPOINT, (uint8_t*)logic_keyboard_usb_hid_keys_buffer, sizeof(logic_keyboard_usb_hid_keys_buffer));
        timer_delay_ms(100);
        memset(logic_keyboard_usb_hid_keys_buffer, 0, sizeof(logic_keyboard_usb_hid_keys_buffer));
        usb_send(USB_KEYBOARD_ENDPOINT, (uint8_t*)logic_keyboard_usb_hid_keys_buffer, sizeof(logic_keyboard_usb_hid_keys_buffer));
    }
    else
    {
        logic_bluetooth_send_modifier_and_key(0, KEY_WIN_L, 0);
        timer_delay_ms(100);
        logic_bluetooth_send_modifier_and_key(0, KEY_WIN_L, l_symbol);
        timer_delay_ms(100);
        logic_bluetooth_send_modifier_and_key(0, 0, 0);     
    }
}

/*! \fn     logic_keyboard_type_key_with_modifier(hid_interface_te interface, uint8_t key, uint8_t modifier, uint16_t delay_between_types)
*   \brief  Perform a single keystroke
*   \param  interface           HID interface on which to type the keystroke
*   \param  key                 Key to send
*   \param  modifier            Modifier (alt, shift...)
*   \param  delay_between_types Delay between types in ms
*   \return If we were able to type the key
*/
ret_type_te logic_keyboard_type_key_with_modifier(hid_interface_te interface, uint8_t key, uint8_t modifier, uint16_t delay_between_types)
{
    /* Check for enumeration */
    if ((interface == USB_INTERFACE) && ((usb_get_config() == 0) || (udc_get_nb_ms_before_last_usb_activity() > 100)))
    {
        return RETURN_NOK;
    }
    
    // Send modifier
    if (modifier != 0)
    {
        if (interface == USB_INTERFACE)
        {
            logic_keyboard_usb_hid_keys_buffer[0] = modifier;
            usb_send(USB_KEYBOARD_ENDPOINT, (uint8_t*)logic_keyboard_usb_hid_keys_buffer, sizeof(logic_keyboard_usb_hid_keys_buffer));
        } 
        else
        {
            if (logic_bluetooth_send_modifier_and_key(modifier, 0, 0) != RETURN_OK)
            {
                return RETURN_NOK;
            }
        }
        timer_delay_ms(delay_between_types);
    }
    
    // Send modifier + key
    if (interface == USB_INTERFACE)
    {
        logic_keyboard_usb_hid_keys_buffer[2] = key;
        usb_send(USB_KEYBOARD_ENDPOINT, (uint8_t*)logic_keyboard_usb_hid_keys_buffer, sizeof(logic_keyboard_usb_hid_keys_buffer));
    }
    else
    {
        if (logic_bluetooth_send_modifier_and_key(modifier, key, 0) != RETURN_OK)
        {
            return RETURN_NOK;
        }
    }
    timer_delay_ms(delay_between_types);
    
    // Release all
    if (interface == USB_INTERFACE)
    {
        logic_keyboard_usb_hid_keys_buffer[0] = 0;
        logic_keyboard_usb_hid_keys_buffer[2] = 0;
        usb_send(USB_KEYBOARD_ENDPOINT, (uint8_t*)logic_keyboard_usb_hid_keys_buffer, sizeof(logic_keyboard_usb_hid_keys_buffer));
    }
    else
    {
        if (logic_bluetooth_send_modifier_and_key(0, 0, 0) != RETURN_OK)
        {
            return RETURN_NOK;
        }
    }
    timer_delay_ms(delay_between_types);    
    
    return RETURN_OK; 
}

/*! \fn     logic_keyboard_type_symbol(hid_interface_te interface, uint8_t symbol, BOOL is_dead_key, uint16_t delay_between_types)
*   \brief  Type an encoded symbol through a given interface
*   \param  interface           HID interface on which to type the symbol
*   \param  symbol              The symbol
*   \param  is_dead_key         Is the symbol a dead key?
*   \param  delay_between_types Delay between key presses
*   \return If we were able to type the symbol
*/
ret_type_te logic_keyboard_type_symbol(hid_interface_te interface, uint8_t symbol, BOOL is_dead_key, uint16_t delay_between_types)
{
    uint8_t masked_key = symbol & (SHIFT_MASK|ALTGR_MASK);
    ret_type_te return_val;
        
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
        return_val = logic_keyboard_type_key_with_modifier(interface, KEY_EUROPE_2_REAL, mod_tbs, delay_between_types);
    }
    else if (masked_key == (SHIFT_MASK|ALTGR_MASK))
    {
        return_val = logic_keyboard_type_key_with_modifier(interface, symbol & ~(SHIFT_MASK|ALTGR_MASK), KEY_SHIFT|KEY_RIGHT_ALT, delay_between_types);
    }
    else if (masked_key == SHIFT_MASK)
    {
        // If we need shift
        return_val = logic_keyboard_type_key_with_modifier(interface, symbol & ~SHIFT_MASK, KEY_SHIFT, delay_between_types);
    }
    else if (masked_key == ALTGR_MASK)
    {
        // We need altgr for the numbered keys, only possible because we don't use the numerical keypad
        return_val = logic_keyboard_type_key_with_modifier(interface, symbol & ~ALTGR_MASK, KEY_RIGHT_ALT, delay_between_types);
    }
    else
    {
        return_val = logic_keyboard_type_key_with_modifier(interface, symbol, 0, delay_between_types);
    }
    
    /* Add space if typed character is a dead key */
    if ((is_dead_key != FALSE) && (return_val == RETURN_OK))
    {
        return_val = logic_keyboard_type_key_with_modifier(interface, KEY_SPACE, 0, delay_between_types);        
    }
    
    return return_val;
}