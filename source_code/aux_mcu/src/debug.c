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
BOOL debug_tx_test_just_started = FALSE;
BOOL debug_tx_test_just_stopped = FALSE;
BOOL debug_tx_test_in_progress = FALSE;
BOOL debug_rx_test_just_started = FALSE;
BOOL debug_rx_test_just_stopped = FALSE;
BOOL debug_rx_test_in_progress = FALSE;
uint8_t debug_current_freq_set = 0;
uint16_t debug_inner_loop_goal = 0;
BOOL debug_tx_test_cb_set = FALSE;
uint8_t debug_payload_length = 0;
uint8_t debug_payload_set = 0;

/*#define IS_MTB_ENABLED REG_MTB_MASTER & MTB_MASTER_EN
#define DISABLE_MTB REG_MTB_MASTER = REG_MTB_MASTER & ~MTB_MASTER_EN
#define ENABLE_MTB REG_MTB_MASTER = REG_MTB_MASTER | MTB_MASTER_EN
__attribute__((aligned(1024))) volatile char __tracebuffer__[1024];
volatile int __tracebuffersize__ = sizeof(__tracebuffer__);
void debug_init_trace_buffer(void)
{
    int index = 0;
    uint32_t mtbEnabled = IS_MTB_ENABLED;
    DISABLE_MTB;
    for(index =0; index<1024; index++)
    {
        __tracebuffer__[index];
        __tracebuffersize__;
    }
    if(mtbEnabled)
    ENABLE_MTB;
}*/

static at_ble_status_t debug_dtm_test_status(void *param)
{
    at_ble_dtm_t* test_status = (at_ble_dtm_t*)param;
    DBG_LOG("DTM Test status callback, opcode 0x%04x status %d", test_status->op_code, test_status->status);
    
    switch(test_status->op_code)
    {
        case AT_BLE_RESET_CMD_OPCODE :
        {
            DBG_LOG("DTM reset callback");
            
            /* Do we want to start the sweep */
            if (debug_tx_test_just_started != FALSE)
            {
                while(at_ble_dtm_tx_test_start(debug_current_freq_set, debug_payload_length, debug_payload_set) != AT_BLE_SUCCESS)
                {
                    DBG_LOG("ERROR: couldn't start DTM TX");
                }
                DBG_LOG("Starting DTM TX test");

                /* Reset bool */
                debug_tx_test_just_started = FALSE;
            }
            else if (debug_rx_test_just_started != FALSE)
            {
                while(at_ble_dtm_rx_test_start(debug_current_freq_set) != AT_BLE_SUCCESS)
                {
                    DBG_LOG("ERROR: couldn't start DTM RX");
                }
                DBG_LOG("Starting DTM RX test");

                /* Reset bool */
                debug_rx_test_just_started = FALSE;
            }
            else
            {
                DBG_LOG("ERROR: not sure why received");
            }
            break;
        }
        case AT_BLE_RX_TEST_CMD_OPCODE:
        {
            DBG_LOG("DTM RX test callback");
            
            /* Are we actually sweeping */
            if (debug_rx_test_in_progress != FALSE)
            {
                DBG_LOG("DTM RX started");
            }
            else
            {
                DBG_LOG("ERROR: not sure why received");
            }
            break;
        }
        case AT_BLE_TX_TEST_CMD_OPCODE:
        {
            DBG_LOG("DTM TX test callback");
            
            /* Are we actually sweeping */
            if (debug_tx_test_in_progress != FALSE)
            {
                DBG_LOG("DTM TX started");
            } 
            else
            {
                DBG_LOG("ERROR: not sure why received");
            }
            break;
        }
        case AT_BLE_TEST_END_CMD_OPCODE:
        {
            DBG_LOG("DTM test end callback: should it be here?");
            break;
        }

        default: break;
    }

    return AT_BLE_SUCCESS;
}

static at_ble_status_t debug_dtm_test_report(void *param)
{
    aux_mcu_message_t* temp_message_pt;
    at_ble_dtm_t* test_report = (at_ble_dtm_t*)param;
    DBG_LOG("DTM Test report callback, opcode 0x%04x status %d", test_report->op_code, test_report->status);
    
    if (debug_tx_test_just_stopped != FALSE)
    {
        DBG_LOG("TX test report: test stopped, %d packets", test_report->number_of_packets);
        
        /* Send success report */
        comms_main_mcu_get_empty_packet_ready_to_be_sent(&temp_message_pt, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT);
        temp_message_pt->aux_mcu_event_message.event_id = AUX_MCU_EVENT_TX_SWEEP_DONE;
        temp_message_pt->payload_length1 = sizeof(temp_message_pt->aux_mcu_event_message.event_id);
        comms_main_mcu_send_message((void*)temp_message_pt, (uint16_t)sizeof(aux_mcu_message_t));
        
        /* Reset bool */
        debug_tx_test_just_stopped = FALSE;
        debug_tx_test_in_progress = FALSE;
    }
    else if (debug_rx_test_just_stopped != FALSE)
    {
        DBG_LOG("RX test report: test stopped, %d packets", test_report->number_of_packets);
        
        /* Send success report */
        comms_main_mcu_get_empty_packet_ready_to_be_sent(&temp_message_pt, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT);
        temp_message_pt->aux_mcu_event_message.event_id = AUX_MCU_EVENT_RX_DTM_DONE;
        temp_message_pt->aux_mcu_event_message.payload_as_uint16[0] = test_report->number_of_packets;
        temp_message_pt->payload_length1 = sizeof(temp_message_pt->aux_mcu_event_message.event_id) + sizeof(uint16_t);
        comms_main_mcu_send_message((void*)temp_message_pt, (uint16_t)sizeof(aux_mcu_message_t));
        
        /* Reset bool */
        debug_rx_test_just_stopped = FALSE;
        debug_rx_test_in_progress = FALSE;
    }
    
    return AT_BLE_SUCCESS;
}

static const ble_dtm_event_cb_t dtm_custom_event_cb = {
    .le_test_status = debug_dtm_test_status,
    .le_packet_report = debug_dtm_test_report
};


void debug_dtm_rx(uint16_t frequency_index)
{
    DBG_LOG_DEV("DTM rx command received, freq %d", frequency_index);
    debug_current_freq_set = (uint8_t)frequency_index;
    while(logic_bluetooth_stop_advertising() != RETURN_OK);
    
    /* Register callbacks */
    if (debug_tx_test_cb_set == FALSE)
    {
        ble_mgr_events_callback_handler(REGISTER_CALL_BACK, BLE_DTM_EVENT_TYPE, &dtm_custom_event_cb);
        DBG_LOG("DTM callbacks set");
        debug_tx_test_cb_set = TRUE;
    }

    /* Reset test mode */
    while(at_ble_dtm_reset() != AT_BLE_SUCCESS)
    {
        DBG_LOG("ERROR: couldn't request DTM reset");
    }
    DBG_LOG("DTM reset requested");

    /* Set bool, wait for callback */
    debug_rx_test_just_started = TRUE;    
    debug_rx_test_in_progress = TRUE;
}

void debug_tx_band_send(uint16_t frequency_index, uint16_t payload_type, uint16_t payload_length)
{
    DBG_LOG_DEV("TX sweep start command received, freq %d, payload type %d, payload length %d", frequency_index, payload_type, payload_length);
    debug_payload_set = (uint8_t)payload_type;
    debug_payload_length = (uint8_t)payload_length;
    debug_current_freq_set = (uint8_t)frequency_index;
    while(logic_bluetooth_stop_advertising() != RETURN_OK);
    
    /* Register callbacks */
    if (debug_tx_test_cb_set == FALSE)
    {
        ble_mgr_events_callback_handler(REGISTER_CALL_BACK, BLE_DTM_EVENT_TYPE, &dtm_custom_event_cb);
        DBG_LOG("DTM callbacks set");
        debug_tx_test_cb_set = TRUE;
    }

    /* Reset test mode */
    while(at_ble_dtm_reset() != AT_BLE_SUCCESS)
    {
        DBG_LOG("ERROR: couldn't request DTM reset");
    }
    DBG_LOG("DTM reset requested");

    /* Set bool, wait for callback */
    debug_tx_test_just_started = TRUE;    
    debug_tx_test_in_progress = TRUE;
}

void debug_tx_stop_continuous_tone(void)
{
    /* Reset test mode */
    while(at_ble_dtm_stop_test() != AT_BLE_SUCCESS)
    {
        DBG_LOG("ERROR: couldn't request DTM stop");
    }
    DBG_LOG("End of test: DTM stop requested");

    /* Set bool, wait for callback */
    if (debug_tx_test_in_progress != FALSE)
    {
        debug_tx_test_just_stopped = TRUE;
    } 
    else if (debug_rx_test_in_progress != FALSE)
    {
        debug_rx_test_just_stopped = TRUE;
    }
}
