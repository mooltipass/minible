/*
 * logic_sleep.h
 *
 * Created: 30/07/2019 20:09:48
 *  Author: limpkin
 */ 


#ifndef LOGIC_SLEEP_H_
#define LOGIC_SLEEP_H_

#include "defines.h"

/* defines */
#define BT_NB_MS_BEFORE_DEASSERTING_WAKEUP  10000

/* debug defines */
#define DEBUG_SLEEP_LOG_DISABLED
#if defined DEBUG_SLEEP_LOG_DISABLED
	#define DBG_SLP_LOG(...)
#else
    #ifndef USB_PRINTF
        #define DBG_SLP_LOG	    platform_io_uart_debug_printf
    #else
        #define DBG_SLP_LOG	    comms_usb_debug_printf
    #endif
#endif

/* Prototypes */
void logic_sleep_set_full_platform_sleep_requested(void);
BOOL logic_sleep_is_full_platform_sleep_requested(void);
void logic_sleep_wakeup_main_mcu_if_needed(void);
void logic_sleep_ble_signal_to_sleep(void);
void logic_sleep_routine_ble_call(void);


#endif /* LOGIC_SLEEP_H_ */