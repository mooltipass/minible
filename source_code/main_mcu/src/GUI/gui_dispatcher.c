/*!  \file     gui_dispatcher.c
*    \brief    GUI functions dispatcher
*    Created:  16/11/2018
*    Author:   Mathieu Stephan
*/
#include "comms_hid_msgs_debug.h"
#include "gui_dispatcher.h"
#include "driver_timer.h"
#include "gui_carousel.h"
#include "logic_device.h"
#include "gui_prompts.h"
#include "logic_power.h"
#include "platform_io.h"
#include "gui_menu.h"
#include "inputs.h"
#include "debug.h"
#include "main.h"
// Current screen
gui_screen_te gui_dispatcher_current_screen = GUI_SCREEN_INVALID;
// Current idle animation frame ID
uint16_t gui_dispatcher_current_idle_anim_frame_id = 0;


/*! \fn     gui_dispatcher_get_current_screen(void)
*   \brief  Get current screen
*   \return The current screen
*/
gui_screen_te gui_dispatcher_get_current_screen(void)
{
    return gui_dispatcher_current_screen;
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
    /* switch to let the compiler optimize instead of function pointer array */
    switch (gui_dispatcher_current_screen)
    {
        case GUI_SCREEN_INSERTED_LCK:
        case GUI_SCREEN_NINSERTED:          {
                                                sh1122_display_bitmap_from_flash_at_recommended_position(&plat_oled_descriptor, GUI_LOCKED_MINI_BITMAP_ID, TRUE); 
                                                #ifdef OLED_INTERNAL_FRAME_BUFFER
                                                    sh1122_flush_frame_buffer(&plat_oled_descriptor);
                                                #endif
                                                break;
                                            }             
        case GUI_SCREEN_INSERTED_INVALID:   gui_prompts_display_information_on_screen(ID_STRING_REMOVE_CARD, DISP_MSG_ACTION); break;
        case GUI_SCREEN_INSERTED_UNKNOWN:   gui_prompts_display_information_on_screen(ID_STRING_UNKNOWN_CARD, DISP_MSG_INFO); break;
        case GUI_SCREEN_FW_FILE_UPDATE:     gui_prompts_display_information_on_screen(ID_STRING_FW_FILE_UPDATE, DISP_MSG_INFO); break;        
        case GUI_SCREEN_MEMORY_MGMT:        gui_prompts_display_information_on_screen(ID_STRING_IN_MMM, DISP_MSG_INFO); break;
        case GUI_SCREEN_CATEGORIES:         break;
        case GUI_SCREEN_FAVORITES:          break;
        case GUI_SCREEN_LOGIN:              break;
        case GUI_SCREEN_LOCK:               break;
        /* Common menu architecture */
        case GUI_SCREEN_MAIN_MENU:
        case GUI_SCREEN_BT:
        case GUI_SCREEN_OPERATIONS:
        case GUI_SCREEN_SETTINGS:           gui_menu_event_render(WHEEL_ACTION_NONE);break;
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
    
    /* switch to let the compiler optimize instead of function pointer array */
    switch (gui_dispatcher_current_screen)
    {
        case GUI_SCREEN_NINSERTED:
        {
            #ifdef DEBUG_MENU_ENABLED            
            /* If button press, go to debug menu */
            if (wheel_action == WHEEL_ACTION_LONG_CLICK)
            {
                debug_debug_menu();
                rerender_bool = TRUE;
            }
            #endif
            break;
        }
        case GUI_SCREEN_INSERTED_LCK:       break;
        case GUI_SCREEN_INSERTED_INVALID:   break;        
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
    /* Temp uint16_t */
    uint16_t temp_uint16;
    
    /* switch to let the compiler optimize instead of function pointer array */
    switch (gui_dispatcher_current_screen)
    {
        case GUI_SCREEN_NINSERTED:          break;
        case GUI_SCREEN_INSERTED_LCK:       break;
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
        case GUI_SCREEN_SETTINGS:           break;
        default: break;
    }    
}

/*! \fn     gui_dispatcher_main_loop(void)
*   \brief  GUI main loop
*/
void gui_dispatcher_main_loop(void)
{
    BOOL is_screen_on_copy = sh1122_is_oled_on(&plat_oled_descriptor);
    
    // Get user action
    wheel_action_ret_te user_action = inputs_get_wheel_action(FALSE, FALSE);
    
    // No activity, turn off screen
    if (timer_has_timer_expired(TIMER_SCREEN, TRUE) == TIMER_EXPIRED)
    {
        /* Display "going to sleep", switch off screen */
        gui_prompts_display_information_on_screen_and_wait(ID_STRING_GOING_TO_SLEEP, DISP_MSG_INFO);
        sh1122_oled_off(&plat_oled_descriptor);
        gui_dispatcher_get_back_to_current_screen();
        platform_io_power_down_oled();
        
        if (logic_power_get_power_source() == BATTERY_POWERED)
        {
            /* Good night */
            main_standby_sleep();
            
            /* We are awake now! */
            logic_device_activity_detected();
        }
    }
    
    // Run main GUI screen loop if there was an action. TODO: screen saver
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