/*
 * debug.c
 *
 * Created: 25/05/2019 22:53:16
 *  Author: limpkin
 */ 
#include <asf.h>
#include "logic_bluetooth.h"
#include "comms_main_mcu.h"
#include "driver_timer.h"
#include "ble_manager.h"
#include "at_ble_api.h"
#include "defines.h"
#include "debug.h"
uint8_t debug_current_freq_set = 0;
uint16_t debug_inner_loop = 0;
uint8_t debug_payload_set = 0;
BOOL debug_tx_test_cb_set = FALSE;


static at_ble_status_t debug_tx_sweep_inc(void *param)
{
    if (debug_inner_loop++ < 100)
    {
        /* Continue on this freq */
        DELAYUS(10);
        DBG_LOG_DEV("Inner %d, Freq %d", debug_inner_loop, debug_current_freq_set);
        while(at_ble_dtm_tx_test_start(debug_current_freq_set, 36, debug_payload_set) != AT_BLE_SUCCESS);
    }
    else
    {
        debug_inner_loop = 0;
        aux_mcu_message_t message;
        message.message_type = AUX_MCU_MSG_TYPE_AUX_MCU_EVENT;
        message.aux_mcu_event_message.event_id = AUX_MCU_EVENT_TW_SWEEP_DONE;
        message.payload_length1 = sizeof(message.aux_mcu_event_message.event_id);
        comms_main_mcu_send_message((void*)&message, (uint16_t)sizeof(aux_mcu_message_t));
    }
    return AT_BLE_SUCCESS;
}

static const ble_dtm_event_cb_t dtm_custom_event_cb = {
    .le_test_status = debug_tx_sweep_inc
};

void debug_tx_sweep_start(uint16_t frequency_index, uint16_t payload_type)
{
    debug_inner_loop = 0;
    debug_payload_set = (uint8_t)payload_type;
    debug_current_freq_set = (uint8_t)frequency_index;
    if (debug_tx_test_cb_set == FALSE)
    {
        ble_mgr_events_callback_handler(REGISTER_CALL_BACK, BLE_DTM_EVENT_TYPE, &dtm_custom_event_cb);
        debug_tx_test_cb_set = TRUE;
    }
    while(at_ble_dtm_tx_test_start(debug_current_freq_set, 36, debug_payload_set) != AT_BLE_SUCCESS);
}
