/*
 * debug.c
 *
 * Created: 25/05/2019 22:53:16
 *  Author: limpkin
 */ 
#include <asf.h>
#include "hid_keyboard_app.h"
#include "comms_main_mcu.h"
#include "defines.h"
#include "debug.h"
uint8_t debug_current_freq_set = 0;
uint16_t debug_inner_loop = 0;

void debug_tx_sweep_start(void)
{
    debug_inner_loop = 0;
    debug_current_freq_set = 0;
    at_ble_dtm_tx_test_start(debug_current_freq_set, 36, PAYL_01010101);    
}

void debug_tx_sweep_inc(void)
{
    if (debug_inner_loop++ < 30)
    {
        /* Continue on this freq */
        at_ble_dtm_tx_test_start(debug_current_freq_set, 36, PAYL_01010101);
    } 
    else
    {
        debug_inner_loop = 0;
        if (debug_current_freq_set++ == 36)
        {
            aux_mcu_message_t message;
            message.message_type = AUX_MCU_MSG_TYPE_AUX_MCU_EVENT;
            message.aux_mcu_event_message.event_id = AUX_MCU_EVENT_TW_SWEEP_DONE;
            comms_main_mcu_send_message((void*)&message, (uint16_t)sizeof(aux_mcu_message_t));
        }
        else
        {
            debug_current_freq_set++;
            at_ble_dtm_tx_test_start(debug_current_freq_set, 36, PAYL_01010101);
        }
    }
}