/*
 * logic_sleep.c
 *
 * Created: 30/07/2019 20:09:38
 *  Author: limpkin
 */ 
#include "platform_defines.h"
#include "conf_serialdrv.h"
#include "comms_raw_hid.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "logic_sleep.h"
#include "serial_drv.h"
#include "platform.h"
#include "defines.h"
#include "logic.h"
#include "main.h"
/* Full platform sleep requested */
BOOL logic_sleep_full_platform_sleep_requested = FALSE;


/*! \fn     logic_sleep_wakeup_main_mcu_if_needed(void)
*   \brief  Wake-up main MCU if needed
*/
void logic_sleep_wakeup_main_mcu_if_needed(void)
{
    /* First check if main MCU is asleep */
    if (logic_sleep_full_platform_sleep_requested != FALSE)
    {
        DBG_SLP_LOG("waking up main MCU");
        
        /* Make sure we're not generating that pulse just after we've been set to sleep */
        while (timer_has_timer_expired(TIMER_MAIN_MCU_WAKE_DELAY, FALSE) == TIMER_RUNNING);
        
        /* Re-enable main comms */
        platform_io_enable_main_comms();
        comms_main_init_rx();
        
        /* Generate wakeup pulse */
        platform_io_generate_no_comms_wakeup_pulse();
        
        /* Leave some time for correct no comms readout */
        timer_delay_ms(1);
        
        /* Wait for no comms release */
        while (platform_io_is_no_comms_asserted() == RETURN_OK);
        
        /* Leave some time before sending our message */
        timer_delay_ms(1);
        
        /* Reset bool now that the main MCU is awake */
        logic_sleep_full_platform_sleep_requested = FALSE;
    }
}

/*! \fn     logic_sleep_ble_signal_to_sleep(void)
*   \brief  Called by BLE to signal the platform it can go to sleep
*/
void logic_sleep_ble_signal_to_sleep(void)
{
    if (logic_sleep_full_platform_sleep_requested != FALSE)
    {
        if (platform_io_is_wakeup_in_pin_low() != FALSE)
        {
            asm volatile("NOP");
            DBG_SLP_LOG("can't go to sleep: data ready pin low");
        }
        if (platform_io_is_ble_wakeup_output_high() != FALSE)
        {
            asm volatile("NOP");
            DBG_SLP_LOG("can't go to sleep: wakeup pin high");
        }
    }
    
    /* If main MCU asserted no comms in the mean time... no point in going to sleep anymore, re-enable comms with main MCU */
    if (logic_is_no_comms_unavailable() == FALSE)
    {
        if ((logic_sleep_full_platform_sleep_requested != FALSE) && (platform_io_is_no_comms_asserted() != RETURN_OK))
        {
            DBG_SLP_LOG("wanted to go to sleep but main MCU woke up in the mean time!");
            logic_sleep_full_platform_sleep_requested = FALSE;
            platform_io_enable_main_comms();
            comms_main_init_rx();
        }
    }
    
    /* If full platform sleep was requested, if no processing needs to be done, if we enable sleep between events */
    if ((logic_sleep_full_platform_sleep_requested != FALSE) && (platform_io_is_wakeup_in_pin_low() == FALSE) && (platform_io_is_ble_wakeup_output_high() == FALSE))
    {        
        /* Set Host RTS to High */
        platform_set_ble_rts_high();
        
        /* Some processing was requested? Do not go to sleep */    
        if (platform_io_is_wakeup_in_pin_low() != FALSE)
        {
            platform_set_ble_rts_low();
            return;
        }
        
        DBG_SLP_LOG("Going to sleep");
        
        /* Disable main comms */
        platform_io_disable_main_comms();
        
        /* Enable BLE interrupt for wakeup */
        platform_io_enable_ble_int();
        
        /* Go to sleep, re-enable comms only if we were awoken by the main MCU */
        main_standby_sleep(FALSE);
        
        /* We just woke up */
        platform_io_disable_ble_int();
        DBG_SLP_LOG("Waking up");
        
        /* Awoken by BLE or main MCU? */
        if (platform_io_is_no_comms_asserted() == RETURN_OK)
        {
            /* If awoken by BLE, allow processing of events */
            platform_io_assert_ble_wakeup();
        }
        else
        {
            /* If awoken by main MCU, enable comms */
            logic_sleep_full_platform_sleep_requested = FALSE;
            platform_io_enable_main_comms();
            comms_main_init_rx();            
        }
        
        /* Set Host RTS Low to receive the data */
        platform_set_ble_rts_low();
        
        /* Awoken by BLE: for the moment, wakeup the main MCU */
        if ((platform_io_is_no_comms_asserted() == RETURN_OK) && FALSE)
        {
            logic_sleep_full_platform_sleep_requested = FALSE;
            platform_io_generate_no_comms_wakeup_pulse();
            platform_io_enable_main_comms();
            comms_main_init_rx();
        }            
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

/*! \fn     logic_sleep_is_full_platform_sleep_requested(void)
*   \brief  Know if full platform sleep was requested
*/
BOOL logic_sleep_is_full_platform_sleep_requested(void)
{
    return logic_sleep_full_platform_sleep_requested;
}

/*! \fn     logic_sleep_routine_ble_call(void)
*   \brief  logic sleep routine, called by ble routine
*/
void logic_sleep_routine_ble_call(void)
{
}