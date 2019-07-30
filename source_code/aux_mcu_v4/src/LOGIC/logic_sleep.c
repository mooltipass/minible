/*
 * logic_sleep.c
 *
 * Created: 30/07/2019 20:09:38
 *  Author: limpkin
 */ 
#include "platform_defines.h"
#include "conf_serialdrv.h"
#include "comms_raw_hid.h"
#include "platform_io.h"
#include "logic_sleep.h"
#include "serial_drv.h"
#include "platform.h"
#include "defines.h"
#include "main.h"
/* Boolean indicating if the ble module is set to sleep between events */
BOOL logic_sleep_ble_module_sleep_between_events = FALSE;
/* Booleans indicating what woke us up */
volatile BOOL logic_sleep_awoken_by_no_comms = FALSE;
volatile BOOL logic_sleep_awoken_by_ble = FALSE;
/* Full platform sleep requested */
BOOL logic_sleep_full_platform_sleep_requested = FALSE;

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
    /* Check that no data needs to be processed */
    if(host_event_data_ready_pin_level())
    {
        ble_wakeup_pin_set_low();
        DBG_SLP_LOG("ATBTLC to sleep btw events");
        logic_sleep_ble_module_sleep_between_events = TRUE;
    }
    else
    {
        DBG_SLP_LOG("Couldn't set ATBTLC to sleep btw events: events to be processed");
    }
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
    if (logic_sleep_full_platform_sleep_requested != FALSE)
    {
        if (!host_event_data_ready_pin_level())
        {
            DBG_SLP_LOG("can't go to sleep: data ready pin low");
        }
        if (ble_wakeup_pin_level())
        {
            DBG_SLP_LOG("can't go to sleep: wakeup pin high");
        }
    }
    
    /* If full platform sleep was requested, if no processing needs to be done, if we enable sleep between events */
    if ((logic_sleep_full_platform_sleep_requested != FALSE) && (host_event_data_ready_pin_level()) && (!ble_wakeup_pin_level()))
    {
        /* Set Host RTS to High */
        platform_set_ble_rts_high();
        
        /* Some processing was requested? Do not go to sleep */    
        if(!host_event_data_ready_pin_level())
        {
            platform_set_ble_rts_low();
            return;
        }
        
        
        /* Enable BLE interrupt for wakeup */
        DBG_SLP_LOG("Going to sleep");
        platform_io_enable_ble_int();
        main_standby_sleep(FALSE);
        
        /* We just woke up */
        DBG_SLP_LOG("Waking up");
        
        /* Set Host RTS Low to receive the data */
        platform_set_ble_rts_low();
    }
}

/*! \fn     logic_sleep_set_full_platform_sleep_requested(void)
*   \brief  Called when a full platform sleep is requested
*/
void logic_sleep_set_full_platform_sleep_requested(void)
{
    logic_sleep_full_platform_sleep_requested = TRUE;
    DBG_SLP_LOG("full platform sleep requested");
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