/*
 * logic_sleep.c
 *
 * Created: 30/07/2019 20:09:38
 *  Author: limpkin
 */ 
#include "platform_defines.h"
#include "comms_raw_hid.h"
#include "platform_io.h"
#include "logic_sleep.h"
#include "platform.h"
#include "defines.h"
/* Boolean indicating if the ble module is set to sleep between events */
BOOL logic_sleep_ble_module_sleep_between_events = FALSE;
/* Booleans indicating what woke us up */
volatile BOOL logic_sleep_awoken_by_no_comms = FALSE;
volatile BOOL logic_sleep_awoken_by_ble = FALSE;
BOOL bla_temp_bool = FALSE;

/*! \fn     logic_sleep_set_awoken_by_ble(void)
*   \brief  Set awoken by ble bool
*/
void logic_sleep_set_awoken_by_ble(void)
{
    logic_sleep_awoken_by_ble = TRUE;
}

/*! \fn     logic_sleep_set_awoken_by_no_comms(void)
*   \brief  Set awoken by no comms bool
*/
void logic_sleep_set_awoken_by_no_comms(void)
{
    logic_sleep_awoken_by_no_comms = TRUE;
}

/*! \fn     logic_sleep_set_ble_to_sleep_between_events(void)
*   \brief  Allow BLE module to sleep between events
*/
void logic_sleep_set_ble_to_sleep_between_events(void)
{
    platform_gpio_set(AT_BLE_EXTERNAL_WAKEUP, AT_BLE_LOW);
    logic_sleep_ble_module_sleep_between_events = TRUE;
    DBG_SLP_LOG("ATBTLC to sleep btw events");
}

/*! \fn     logic_sleep_ble_not_sleeping_between_events(void)
*   \brief  Indicate that the BLE module is not sleeping between events
*/
void logic_sleep_ble_not_sleeping_between_events(void)
{
    logic_sleep_ble_module_sleep_between_events = FALSE;
    DBG_SLP_LOG("ATBTLC to NOT sleep btw events");    
}

/*! \fn     logic_sleep_ble_signal_to_sleep(void)
*   \brief  Called by BLE to signal the platform it can go to sleep
*/
void logic_sleep_ble_signal_to_sleep(void)
{
    if (bla_temp_bool == FALSE)
    {
        DBG_SLP_LOG("going to sleep");
        platform_io_enable_ble_int();
        bla_temp_bool = TRUE;
    }
    else
    {
        if (logic_sleep_awoken_by_ble != FALSE)
        {
            logic_sleep_awoken_by_ble = FALSE;
            DBG_SLP_LOG("awoken!");
            bla_temp_bool = FALSE;
        }
    }
}

/*! \fn     logic_sleep_routine_ble_call(void)
*   \brief  logic sleep routine, called by ble routine
*/
void logic_sleep_routine_ble_call(void)
{
    /* BLE module was set to not sleep between events */
    if (logic_sleep_ble_module_sleep_between_events == FALSE)
    {
        /* Possible improvement: use a timer and not set it directly? */
        logic_sleep_set_ble_to_sleep_between_events();
    }
}