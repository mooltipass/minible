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
/*!  \file     gui_dispatcher.c
*    \brief    GUI functions dispatcher
*    Created:  16/11/2018
*    Author:   Mathieu Stephan
*/
#include "comms_hid_msgs_debug.h"
#include "logic_smartcard.h"
#include "logic_bluetooth.h"
#include "logic_security.h"
#include "gui_dispatcher.h"
#include "driver_timer.h"
#include "gui_carousel.h"
#include "logic_device.h"
#include "gui_prompts.h"
#include "logic_power.h"
#include "platform_io.h"
#include "gui_menu.h"
#include "text_ids.h"
#include "inputs.h"
#include "debug.h"
#include "main.h"
// Current screen
gui_screen_te gui_dispatcher_current_screen = GUI_SCREEN_INVALID;
// Current idle animation frame ID
uint16_t gui_dispatcher_current_idle_anim_frame_id = 0;
// Current idle animation loop number
uint16_t gui_dispatcher_current_idle_anim_loop = 0;
// Current battery charging animation index
battery_state_te gui_dispatcher_battery_charging_anim_index = 0;


/*! \fn     gui_dispatcher_get_current_screen(void)
*   \brief  Get current screen
*   \return The current screen
*/
gui_screen_te gui_dispatcher_get_current_screen(void)
{
    return gui_dispatcher_current_screen;
}

/*! \fn     gui_dispatcher_display_battery_bt_overlay(BOOL write_to_buffer)
*   \brief  Display battery / bt overlay in current frame buffer
*   \param  write_to_buffer Set to TRUE to write to buffer
*/
void gui_dispatcher_display_battery_bt_overlay(BOOL write_to_buffer)
{    
    /* Charging or not? */
    if (logic_power_is_battery_charging() == FALSE)
    {
        /* Our enum allows us to do so */
        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_BATTERY_0PCT_ID+logic_power_get_battery_state(), write_to_buffer);
        gui_dispatcher_battery_charging_anim_index = logic_power_get_battery_state();
    } 
    else
    {
        /* Animation timer */
        if (timer_has_timer_expired(TIMER_BATTERY_ANIM, TRUE) == TIMER_EXPIRED)
        {
            timer_start_timer(TIMER_BATTERY_ANIM, GUI_BATTERY_ANIM_DELAY_MS);
            gui_dispatcher_battery_charging_anim_index++;
        }
        
        /* Check for end condition */
        if (gui_dispatcher_battery_charging_anim_index > BATTERY_100PCT)
        {
            /* Animation starts from current state (updated when charging) */
            gui_dispatcher_battery_charging_anim_index = logic_power_get_battery_state();
            
            /* To always display an animation... */
            if (gui_dispatcher_battery_charging_anim_index != BATTERY_0PCT)
            {
                gui_dispatcher_battery_charging_anim_index--;
            }
        }
        
        /* Display */
        sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_BATTERY_0PCT_ID+(gui_dispatcher_battery_charging_anim_index), write_to_buffer);
    }
    
    /* Bluetooth display */
    sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, BITMAP_TRAY_BT_CONNECTED_ID+logic_bluetooth_get_state(), write_to_buffer);
}

/*! \fn     gui_dispatcher_set_current_screen(gui_screen_te screen)
*   \brief  Set current screen
*   \param  screen          The screen
*   \param  reset_states    Set to true to reset previously selected items
*/
void gui_dispatcher_set_current_screen(gui_screen_te screen, BOOL reset_states, oled_transition_te transition)
{
    /* Store transition, screen, call menu reset routine */
    plat_oled_descriptor.loaded_transition = transition;
    gui_dispatcher_current_idle_anim_frame_id = 0;
    gui_dispatcher_current_idle_anim_loop = 0;
    gui_dispatcher_current_screen = screen;    
    gui_menu_reset_selected_items(reset_states);
    
    /* If we're going into a menu, set the selected menu */
    if ((screen >= GUI_SCREEN_MAIN_MENU) && (screen <= GUI_SCREEN_SETTINGS))
    {
        gui_menu_set_selected_menu(screen-GUI_SCREEN_MAIN_MENU+MAIN_MENU);
        gui_menu_update_menus();
    }
}

/*! \fn     gui_dispatcher_get_back_to_current_screen(void)
*   \brief  Get back to the current screen
*/
void gui_dispatcher_get_back_to_current_screen(void)
{
    if (gui_dispatcher_current_screen == GUI_SCREEN_LOGIN_NOTIF)
    {
        /* We're currently displaying a login notification but were interrupted for another prompt... go to main menu */
        if (logic_security_is_management_mode_set() != FALSE)
        {
            gui_dispatcher_current_screen = GUI_SCREEN_MEMORY_MGMT;
        }
        else
        {
            gui_dispatcher_current_screen = GUI_SCREEN_MAIN_MENU;
        }
    }

    /* switch to let the compiler optimize instead of function pointer array */
    switch (gui_dispatcher_current_screen)
    {
        case GUI_SCREEN_INSERTED_LCK:
        case GUI_SCREEN_NINSERTED:          {
                                                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, GUI_LOCKED_MINI_BITMAP_ID, TRUE);
                                                timer_start_timer(TIMER_ANIMATIONS, GUI_BATTERY_ANIM_DELAY_MS);
                                                gui_dispatcher_display_battery_bt_overlay(TRUE);
                                                #ifdef OLED_INTERNAL_FRAME_BUFFER
                                                    sh1122_flush_frame_buffer(&plat_oled_descriptor);
                                                #endif
                                                break;
                                            }             
        case GUI_SCREEN_INSERTED_INVALID:   gui_prompts_display_information_on_screen(REMOVE_CARD_TEXT_ID, DISP_MSG_ACTION); break;
        case GUI_SCREEN_INSERTED_UNKNOWN:   gui_prompts_display_information_on_screen(UNKNOWN_CARD_TEXT_ID, DISP_MSG_INFO); break;
        case GUI_SCREEN_FW_FILE_UPDATE:     gui_prompts_display_information_on_screen(FW_FILE_UPDATE_TEXT_ID, DISP_MSG_INFO); break;        
        case GUI_SCREEN_MEMORY_MGMT:        gui_prompts_display_information_on_screen(ID_STRING_IN_MMM, DISP_MSG_INFO); break;
        case GUI_SCREEN_CATEGORIES:         break;
        case GUI_SCREEN_FAVORITES:          break;
        case GUI_SCREEN_LOGIN:              break;
        case GUI_SCREEN_LOCK:               break;
        /* Common menu architecture */
        case GUI_SCREEN_MAIN_MENU:
        case GUI_SCREEN_BT:
        case GUI_SCREEN_OPERATIONS:
        case GUI_SCREEN_SETTINGS:           
                                            {
                                                gui_menu_event_render(WHEEL_ACTION_NONE);
                                                break;
                                            }                                                
        default: break;
    }
}

/*! \fn     gui_dispatcher_event_dispatch(wheel_action_ret_te wheel_action)
*   \brief  Called for the GUI to do something with a wheel action
*   \param  wheel_action    Wheel action that just happened
*/
void gui_dispatcher_event_dispatch(wheel_action_ret_te wheel_action)
{
    /* Bool to know if we should rerender */
    BOOL rerender_bool = FALSE;
    
    /* No point in dealing with a discarded event */
    if (wheel_action == WHEEL_ACTION_DISCARDED)
    {
        return;
    }
    
    /* switch to let the compiler optimize instead of function pointer array */
    switch (gui_dispatcher_current_screen)
    {
        case GUI_SCREEN_NINSERTED:
        {       
            /* If button press, go to debug menu or power off */
            if (wheel_action == WHEEL_ACTION_LONG_CLICK)
            {
                /* Only if battery powered */
                if ((platform_io_is_usb_3v3_present() == FALSE) && TRUE)
                {
                    /* Prompt user */
                    if (gui_prompts_ask_for_one_line_confirmation(QSWITCH_OFF_DEVICE_TEXT_ID, FALSE, FALSE, TRUE) == MINI_INPUT_RET_YES)
                    {
                        logic_power_power_down_actions();           // Power down actions
                        sh1122_oled_off(&plat_oled_descriptor);     // Display off command
                        platform_io_power_down_oled();              // Switch off stepup
                        platform_io_set_wheel_click_pull_down();    // Pull down on wheel click to slowly discharge capacitor
                        timer_delay_ms(100);                        // From OLED datasheet wait before removing 3V3
                        platform_io_set_wheel_click_low();          // Completely discharge cap
                        timer_delay_ms(10);                         // Wait a tad
                        platform_io_disable_switch_and_die();       // Die!
                    }
                    else
                    {
                        rerender_bool = TRUE;
                    }
                }
            }
            break;
        }

        case GUI_SCREEN_LOGIN_NOTIF:
        {
            /* If action, end animation */
            if (logic_security_is_management_mode_set() != FALSE)
            {
                gui_dispatcher_set_current_screen(GUI_SCREEN_MEMORY_MGMT, FALSE, GUI_INTO_MENU_TRANSITION);
                gui_dispatcher_get_back_to_current_screen();
            }
            else
            {
                gui_dispatcher_set_current_screen(GUI_SCREEN_MAIN_MENU, FALSE, GUI_INTO_MENU_TRANSITION);
                gui_dispatcher_get_back_to_current_screen();
            }
            break;
        }
        
        case GUI_SCREEN_INSERTED_LCK:
        {
            /* Long click to power off */
            if (wheel_action == WHEEL_ACTION_LONG_CLICK)
            {          
                /* Only if battery powered */ 
                if ((platform_io_is_usb_3v3_present() == FALSE) && TRUE)
                {
                    /* Prompt user */
                    if (gui_prompts_ask_for_one_line_confirmation(QSWITCH_OFF_DEVICE_TEXT_ID, FALSE, FALSE, TRUE) == MINI_INPUT_RET_YES)
                    {
                        sh1122_oled_off(&plat_oled_descriptor);     // Display off command
                        platform_io_power_down_oled();              // Switch off stepup
                        platform_io_set_wheel_click_pull_down();    // Pull down on wheel click to slowly discharge capacitor
                        timer_delay_ms(100);                        // From OLED datasheet wait before removing 3V3
                        platform_io_set_wheel_click_low();          // Completely discharge cap
                        timer_delay_ms(10);                         // Wait a tad
                        platform_io_disable_switch_and_die();       // Die!                        
                    }
                    else
                    {
                        rerender_bool = TRUE;                 
                    }
                } 
            }
            else
            {
                logic_smartcard_handle_inserted();
                logic_device_set_state_changed();
            }            
            break;
        }            
        case GUI_SCREEN_INSERTED_INVALID:   
        {
            /* Long click when invalid card inserted for debug menu */
            #ifdef DEBUG_MENU_ENABLED
            if (wheel_action == WHEEL_ACTION_LONG_CLICK)
            {
                debug_debug_menu();
                rerender_bool = TRUE;
            }
            #endif
            break;
        }            
        case GUI_SCREEN_INSERTED_UNKNOWN:   break;
        case GUI_SCREEN_MEMORY_MGMT:        break;
        case GUI_SCREEN_CATEGORIES:         break;
        case GUI_SCREEN_FAVORITES:          break;
        case GUI_SCREEN_LOGIN:              break;
        case GUI_SCREEN_LOCK:               break;
        /* Common menu architecture */        
        case GUI_SCREEN_MAIN_MENU:
        case GUI_SCREEN_BT:
        case GUI_SCREEN_OPERATIONS:
        case GUI_SCREEN_SETTINGS:           rerender_bool = gui_menu_event_render(wheel_action);break;
        default: break;
    }    
    
    /* Should we rerender? */
    if (rerender_bool != FALSE)
    {
        gui_dispatcher_get_back_to_current_screen();
    }
}

/*! \fn     gui_dispatcher_idle_call(void)
*   \brief  Called for idle actions
*/
void gui_dispatcher_idle_call(void)
{
    /* Copy of current frame id */
    uint16_t current_frame_id = gui_dispatcher_current_idle_anim_frame_id;

    /* Temp uint16_t */
    uint16_t temp_uint16;
    
    /* switch to let the compiler optimize instead of function pointer array */
    switch (gui_dispatcher_current_screen)
    {
        case GUI_SCREEN_NINSERTED:
        case GUI_SCREEN_INSERTED_LCK:       {
                                                gui_dispatcher_display_battery_bt_overlay(FALSE);   
                                                break;                                                
                                            }
        case GUI_SCREEN_LOGIN_NOTIF:        {
                                                if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
                                                {
                                                    /* Display new animation frame bitmap, rearm timer with provided value */
                                                    gui_prompts_display_information_on_string_single_anim_frame(&gui_dispatcher_current_idle_anim_frame_id, &temp_uint16, DISP_MSG_INFO);
                                                    timer_start_timer(TIMER_ANIMATIONS, temp_uint16);
                                                    
                                                    /* Enough animation loops to go back to main menu */
                                                    if (gui_dispatcher_current_idle_anim_loop == 4)
                                                    {
                                                        if (logic_security_is_management_mode_set() != FALSE)
                                                        {
                                                            gui_dispatcher_set_current_screen(GUI_SCREEN_MEMORY_MGMT, FALSE, GUI_INTO_MENU_TRANSITION);
                                                            gui_dispatcher_get_back_to_current_screen();
                                                        } 
                                                        else
                                                        {
                                                            gui_dispatcher_set_current_screen(GUI_SCREEN_MAIN_MENU, FALSE, GUI_INTO_MENU_TRANSITION);
                                                            gui_dispatcher_get_back_to_current_screen();
                                                        }
                                                    } 
                                                }
                                                break;
                                            }
        case GUI_SCREEN_INSERTED_INVALID:   {
                                                if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
                                                {
                                                    /* Display new animation frame bitmap, rearm timer with provided value */
                                                    gui_prompts_display_information_on_string_single_anim_frame(&gui_dispatcher_current_idle_anim_frame_id, &temp_uint16, DISP_MSG_ACTION);
                                                    timer_start_timer(TIMER_ANIMATIONS, temp_uint16);
                                                }
                                                break;
                                            }
        case GUI_SCREEN_INSERTED_UNKNOWN:   {
                                                if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
                                                {
                                                    /* Display new animation frame bitmap, rearm timer with provided value */
                                                    gui_prompts_display_information_on_string_single_anim_frame(&gui_dispatcher_current_idle_anim_frame_id, &temp_uint16, DISP_MSG_INFO);
                                                    timer_start_timer(TIMER_ANIMATIONS, temp_uint16);
                                                }
                                                break;
                                            }
        case GUI_SCREEN_MEMORY_MGMT:        {
                                                if (timer_has_timer_expired(TIMER_ANIMATIONS, TRUE) == TIMER_EXPIRED)
                                                {
                                                    /* Display new animation frame bitmap, rearm timer with provided value */
                                                    gui_prompts_display_information_on_string_single_anim_frame(&gui_dispatcher_current_idle_anim_frame_id, &temp_uint16, DISP_MSG_INFO);
                                                    timer_start_timer(TIMER_ANIMATIONS, temp_uint16);
                                                }
                                                break;
                                            }
        case GUI_SCREEN_CATEGORIES:         break;
        case GUI_SCREEN_FAVORITES:          break;
        case GUI_SCREEN_LOGIN:              break;
        case GUI_SCREEN_LOCK:               break;
        /* Common menu architecture */
        case GUI_SCREEN_MAIN_MENU:
        case GUI_SCREEN_BT:
        case GUI_SCREEN_OPERATIONS:
        case GUI_SCREEN_SETTINGS:           {
                                                gui_dispatcher_display_battery_bt_overlay(FALSE);
                                                break;
                                            }                                                
        default: break;
    }    

    /* Update loop number */
    if ((gui_dispatcher_current_idle_anim_frame_id != (current_frame_id + 1)) && (gui_dispatcher_current_idle_anim_frame_id != current_frame_id))
    {
        gui_dispatcher_current_idle_anim_loop++;
    }
}

/*! \fn     gui_dispatcher_main_loop(wheel_action_ret_te wheel_action)
*   \brief  GUI main loop
*   \param  wheel_action    A wheel action to pass to the internal logic in case no real wheel action is detected
*/
void gui_dispatcher_main_loop(wheel_action_ret_te wheel_action)
{
    BOOL is_screen_on_copy = sh1122_is_oled_on(&plat_oled_descriptor);
    
    // Get user action
    wheel_action_ret_te user_action = inputs_get_wheel_action(FALSE, FALSE);
    
    // In case of no action, accept override
    if (user_action == WHEEL_ACTION_NONE)
    {
        user_action = wheel_action;
    }
    
    // No activity, turn off screen
    if  ((gui_dispatcher_get_current_screen() != GUI_SCREEN_MEMORY_MGMT) && (gui_dispatcher_get_current_screen() != GUI_SCREEN_LOGIN_NOTIF) && (timer_has_timer_expired(TIMER_SCREEN, TRUE) == TIMER_EXPIRED))
    {
        /* Display "going to sleep", switch off screen */
        if (sh1122_is_oled_on(&plat_oled_descriptor) != FALSE)
        {
            if (gui_prompts_display_information_on_screen_and_wait(GOING_TO_SLEEP_TEXT_ID, DISP_MSG_INFO, TRUE) == GUI_INFO_DISP_RET_OK)
            {
                /* Uninterrupted animation */
                sh1122_oled_off(&plat_oled_descriptor);
                gui_dispatcher_get_back_to_current_screen();
            }
            else
            {
                /* Interrupted animation */
                gui_dispatcher_get_back_to_current_screen();
                return;     
            }        
        }
        
        /* The notification display routine calls the activity detected routine */
        timer_start_timer(TIMER_SCREEN, 0);
        timer_has_timer_expired(TIMER_SCREEN, TRUE);
        
        /* If we're battery powered, go to sleep */
        if (logic_power_get_power_source() != USB_POWERED)
        {
            /* Hack: only disable OLED stepup when battery powered. When USB powered, if 3V3 USB isn't loaded then we can't detect USB disconnection */
            platform_io_power_down_oled();
            
            /* Call power routine so it registers we powered down the oled screen */
            logic_power_check_power_switch_and_battery(TRUE);
            
            /* Good night */
            main_standby_sleep();
                
            /* We are awake now! */
            
            /* Restart ADC conversions */
            platform_io_get_voledin_conversion_result_and_trigger_conversion();
        }
    }
    
    // Run main GUI screen loop if there was an action. TODO3: screen saver
    if (((is_screen_on_copy != FALSE) && (TRUE /* screen saver place holder */)) || (gui_dispatcher_current_screen == GUI_SCREEN_INSERTED_LCK))
    {
        if (user_action != WHEEL_ACTION_NONE)
        {
            gui_dispatcher_event_dispatch(user_action);            
        }
        else
        {
            gui_dispatcher_idle_call();
        }
    }
}
