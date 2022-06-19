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
/*!  \file     comms_aux_mcu.c
*    \brief    Communications with aux MCU
*    Created:  03/03/2018
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include <asf.h>
#include "comms_hid_msgs_debug_defines.h"
#include "comms_hid_msgs_debug.h"
#include "platform_defines.h"
#include "logic_bluetooth.h"
#include "comms_hid_msgs.h"
#include "logic_security.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "logic_device.h"
#include "platform_io.h"
#include "logic_power.h"
#include "logic_fido2.h"
#include "gui_prompts.h"
#include "logic_user.h"
#include "nodemgmt.h"
#include "text_ids.h"
#include "sh1122.h"
#include "main.h"
#include "dma.h"
#include "rng.h"
/* Received and sent MCU messages */
aux_mcu_message_t aux_mcu_receive_message;
aux_mcu_message_t aux_mcu_send_message_2;
aux_mcu_message_t aux_mcu_send_message_1;
BOOL aux_mcu_message_1_reserved = FALSE;
BOOL aux_mcu_message_2_reserved = FALSE;
/* Flag set if comms are disabled */
BOOL aux_mcu_comms_disabled = FALSE;
/* Flag set if we have treated a message by only looking at its first bytes */
BOOL aux_mcu_message_answered_using_first_bytes = FALSE;
/* Flag set for comms_aux_mcu_routine for recursive calls */
BOOL aux_mcu_comms_aux_mcu_routine_function_called = FALSE;
/* Flag set to specify that the first aux mcu function call wanted to rearm rx */
BOOL aux_mcu_comms_prev_aux_mcu_routine_wants_to_arm_rx = FALSE;
/* Flag set when an invalid message was received */
BOOL aux_mcu_comms_invalid_message_received = FALSE;
/* Flag set when rx transfer is already armed */
BOOL aux_mcu_comms_rx_already_armed = FALSE;
/* Flag set when the second tx buffer is requested */
BOOL aux_mcu_comms_second_buffer_rerequested = FALSE;
/* Timeout delay for aux MCU communications */
BOOL aux_mcu_comms_timeout_delay = AUX_MCU_MESSAGE_REPLY_TIMEOUT_MS;


/*! \fn     comms_aux_mcu_set_invalid_message_received(void)
*   \brief  Register that an invalid message was received
*/
void comms_aux_mcu_set_invalid_message_received(void)
{
    aux_mcu_comms_invalid_message_received = TRUE;
}

/*! \fn     comms_aux_mcu_update_timeout_delay(uint16_t timeout_delay)
*   \brief  Update the timeout delay for communications with the aux MCU
*   \param  timeout_delay   New timeout delay for comms
*/
void comms_aux_mcu_update_timeout_delay(uint16_t timeout_delay)
{
    aux_mcu_comms_timeout_delay = timeout_delay;
}

/*! \fn     comms_aux_mcu_get_and_clear_invalid_message_received(void)
*   \brief  Get and clear the invalid message received flag
*/
BOOL comms_aux_mcu_get_and_clear_invalid_message_received(void)
{
    BOOL ret_val = aux_mcu_comms_invalid_message_received;
    aux_mcu_comms_invalid_message_received = FALSE;
    return ret_val;
}

/*! \fn     comms_aux_mcu_get_and_clear_rx_transfer_already_armed(void)
*   \brief  Get and clear the rx transfer already armed flag
*/
BOOL comms_aux_mcu_get_and_clear_rx_transfer_already_armed(void)
{
    BOOL ret_val = aux_mcu_comms_rx_already_armed;
    aux_mcu_comms_rx_already_armed = FALSE;
    return ret_val;
}

/*! \fn     comms_aux_mcu_get_and_clear_second_tx_buffer_rerequested(void)
*   \brief  Get and clear the tx buffer 2 re requested flag
*/
BOOL comms_aux_mcu_get_and_clear_second_tx_buffer_rerequested(void)
{
    BOOL ret_val = aux_mcu_comms_second_buffer_rerequested;
    aux_mcu_comms_second_buffer_rerequested = FALSE;
    return ret_val;
}

/*! \fn     comms_aux_arm_rx_and_clear_no_comms(void)
*   \brief  Init RX communications with aux MCU
*/
void comms_aux_arm_rx_and_clear_no_comms(void)
{
    if (dma_aux_mcu_is_rx_transfer_already_init() == FALSE)
    {
        /* Arm USART RX interrupt to assert no comms signal, arm DMA transfer */
        dma_aux_mcu_init_rx_transfer(AUXMCU_SERCOM, (void*)&aux_mcu_receive_message, sizeof(aux_mcu_receive_message));
        
        /* While this sounded like a good idea, this isn't really one as this interrupt will only fire if the DMA fires after the second USART bytes is received (as one byte was received before the DMA could fetch it */
        /* It is however left here (because, why not?) but the no comms signal is asserted at the DMA end of transfer interrupt. Delay between interrupt firing and no comms assertion was measured at 4us */
        platform_io_arm_rx_usart_rx_interrupt();
    }
    else
    {
        /* Should never happen! */
        aux_mcu_comms_rx_already_armed = TRUE;
    }
    aux_mcu_comms_disabled = FALSE;
    platform_io_clear_no_comms();
}

/*! \fn     comms_aux_mcu_clear_rx_already_armed_error(void)
*   \brief  Does what it says
*/
void comms_aux_mcu_clear_rx_already_armed_error(void)
{
    aux_mcu_comms_rx_already_armed = FALSE;
}

/*! \fn     comms_aux_mcu_set_comms_disabled(void)
*   \brief  Specify that comms are disabled
*/
void comms_aux_mcu_set_comms_disabled(void)
{
    aux_mcu_comms_disabled = TRUE;
}

/*! \fn     comms_aux_mcu_are_comms_disabled(void)
*   \brief  Know if aux comms are disabled
*   \return If comms are disabled
*/
BOOL comms_aux_mcu_are_comms_disabled(void)
{
    return aux_mcu_comms_disabled;
}

/*! \fn     comms_aux_mcu_wait_for_message_sent(void)
*   \brief  Wait for previous message to be sent to aux MCU
*/
void comms_aux_mcu_wait_for_message_sent(void)
{
    dma_wait_for_aux_mcu_packet_sent();
}

/*! \fn     comms_aux_mcu_get_free_tx_message_object_pt(void)
*   \brief  Get a pointer to our temporary tx message object
*/
aux_mcu_message_t* comms_aux_mcu_get_free_tx_message_object_pt(void)
{    
    /* Wait for possible ongoing message to be sent */
    comms_aux_mcu_wait_for_message_sent();
    
    /* Check for error */
    if (aux_mcu_message_2_reserved != FALSE)
    {
        aux_mcu_comms_second_buffer_rerequested = TRUE;
    }
    
    /* A bit of background: the code is structured in such a way that every time a
    pointer is asked, the message is shortly sent after. There are a few cases where 
    a pointer is asked to prepare a message, then another pointer is asked to get information
    through this new message to complete the first message.
    TLDR: for every pointer requested only one message is sent */
    
    /* Check for message 1 reserved */
    if (aux_mcu_message_1_reserved == FALSE)
    {
        aux_mcu_message_1_reserved = TRUE;
        return &aux_mcu_send_message_1;
    }
    else
    {
        aux_mcu_message_2_reserved = TRUE;
        return &aux_mcu_send_message_2;        
    }
}

/*! \fn     comms_aux_mcu_get_empty_packet_ready_to_be_sent(uint16_t message_type)
*   \brief  Get an empty message ready to be sent
*   \param  message_type    Message type
*   \return Pointer to the message ready to be sent
*/
aux_mcu_message_t* comms_aux_mcu_get_empty_packet_ready_to_be_sent(uint16_t message_type)
{
    /* Get pointer to our message to be sent buffer */
    aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_free_tx_message_object_pt();

    /* Clear it */
    memset((void*)temp_tx_message_pt, 0, sizeof(*temp_tx_message_pt));

    /* Populate the fields */
    temp_tx_message_pt->message_type = message_type;

    /* Return message pointer */
    return temp_tx_message_pt;
}

/*! \fn     comms_aux_mcu_send_message(aux_mcu_message_t* message_to_send)
*   \brief  Send a message to the AUX MCU
*   \param  message_to_send Pointer to the message to send (should be equal to &aux_mcu_send_message !)
*   \note   Transfer is done through DMA so aux_mcu_send_message will be accessed after this function returns if boolean is set to false
*/
void comms_aux_mcu_send_message(aux_mcu_message_t* message_to_send)
{
    /* Do we need to wake-up aux mcu? */
    if (aux_mcu_comms_disabled != FALSE)
    {
        platform_io_disable_no_comms_as_wakeup_interrupt();
        comms_aux_arm_rx_and_clear_no_comms();
        
        /* give some time to AUX to wakeup */
        timer_delay_ms(200);
    }        
        
    /* Check that we're indeed sending the aux_mcu_send_message.... */
    if ((message_to_send != &aux_mcu_send_message_1) && (message_to_send != &aux_mcu_send_message_2))
    {
        main_reboot();
    }
    
    /* Is reserved message 1 freed? */
    if ((aux_mcu_message_1_reserved != FALSE) && (message_to_send == &aux_mcu_send_message_1))
    {
        aux_mcu_message_1_reserved = FALSE;
    }
    
    /* Is reserved message 2 freed? */
    if ((aux_mcu_message_2_reserved != FALSE) && (message_to_send == &aux_mcu_send_message_2))
    {
        aux_mcu_message_2_reserved = FALSE;
    }
    
    /* The function below does wait for a previous transfer to finish */
    dma_aux_mcu_init_tx_transfer(AUXMCU_SERCOM, (void*)message_to_send, sizeof(*message_to_send));
}

/*! \fn     comms_aux_mcu_send_simple_command_message(uint16_t command)
*   \brief  Send simple command message to aux MCU
*   \param  command The command to send
*/
void comms_aux_mcu_send_simple_command_message(uint16_t command)
{
    aux_mcu_message_t* temp_send_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_MAIN_MCU_CMD);
    temp_send_message_pt->payload_length1 = sizeof(temp_send_message_pt->main_mcu_command_message.command);
    temp_send_message_pt->main_mcu_command_message.command = command;
    comms_aux_mcu_send_message(temp_send_message_pt);
}

/*! \fn     comms_aux_mcu_update_device_status_buffer(void)
*   \brief  Update the device status buffer on the aux MCU
*/
void comms_aux_mcu_update_device_status_buffer(void)
{
    aux_mcu_message_t* temp_send_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_MAIN_MCU_CMD);
    uint16_t payload_size = comms_hid_msgs_fill_get_status_message_answer(temp_send_message_pt->main_mcu_command_message.payload_as_uint16);
    temp_send_message_pt->payload_length1 = sizeof(temp_send_message_pt->main_mcu_command_message.command) + payload_size;
    temp_send_message_pt->main_mcu_command_message.command = MAIN_MCU_COMMAND_UPDT_DEV_STAT;
    comms_aux_mcu_send_message(temp_send_message_pt);
    comms_aux_mcu_wait_for_aux_event(AUX_MCU_EVENT_NEW_STATUS_RCVD);
}

/*! \fn     comms_aux_mcu_hard_comms_reset_with_aux_mcu_reboot(void)
*   \brief  Hard reset of aux MCU comms using an aux MCU reboot
*/
void comms_aux_mcu_hard_comms_reset_with_aux_mcu_reboot(void)
{
    /* Set no comms (keep platform in sleep after its reboot) */
    platform_io_set_no_comms();

    /* Generate two packets full of 0xFF... */
    aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(0xFFFF);
    memset((void*)temp_tx_message_pt, 0xFF, sizeof(*temp_tx_message_pt));
    comms_aux_mcu_send_message(temp_tx_message_pt);
    temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(0xFFFF);
    memset((void*)temp_tx_message_pt, 0xFF, sizeof(*temp_tx_message_pt));
    comms_aux_mcu_send_message(temp_tx_message_pt);

    /* Wait for platform to boot */
    timer_delay_ms(100);

    /* Reset our comms */
    dma_aux_mcu_disable_transfer();

    /* Enable our comms, clear no comms signal */
    comms_aux_arm_rx_and_clear_no_comms();

    /* Leave some time to boot before chatting again */
    timer_delay_ms(100);
}

/*! \fn     comms_aux_mcu_send_receive_ping(void)
*   \brief  Try to ping the aux MCU
*   \return Success or not
*/
RET_TYPE comms_aux_mcu_send_receive_ping(void)
{
    aux_mcu_message_t* temp_rx_message_pt;
    RET_TYPE return_val;

    /* Prepare ping message and send it */
    aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_PING_WITH_INFO);
    temp_tx_message_pt->payload_length1 = sizeof(ping_with_info_message_t);
    comms_aux_mcu_send_message(temp_tx_message_pt);

    /* Wait for answer: no need to parse answer as filter is done in comms_aux_mcu_active_wait */
    return_val = comms_aux_mcu_active_wait(&temp_rx_message_pt, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, FALSE, AUX_MCU_EVENT_IM_HERE);

    /* Rearm receive */
    comms_aux_arm_rx_and_clear_no_comms();

    return return_val;
}

/*! \fn     comms_aux_mcu_get_aux_status(void)
*   \brief  Request the aux MCU for its status, check if it's alive
*   \return Different status (see enum)
*/
aux_status_return_te comms_aux_mcu_get_aux_status(void)
{
    aux_mcu_message_t* temp_rx_message_pt;
    aux_status_return_te ret_val;

    /* Send get status message */
    comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_GET_STATUS);
    
    /* Change delay: when connected over bluetooth, we've seen 3s once or twice */
    comms_aux_mcu_update_timeout_delay(4000);

    /* Wait for answer: no need to parse answer as filter is done in comms_aux_mcu_active_wait */
    if (comms_aux_mcu_active_wait(&temp_rx_message_pt, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, FALSE, AUX_MCU_EVENT_HERES_MY_STATUS) == RETURN_NOK)
    {
        comms_aux_mcu_update_timeout_delay(AUX_MCU_MESSAGE_REPLY_TIMEOUT_MS);
        return RETURN_AUX_STAT_TIMEOUT;
    }
    
    /* Reset timeout delay */
    comms_aux_mcu_update_timeout_delay(AUX_MCU_MESSAGE_REPLY_TIMEOUT_MS);
    
    /* Most important thing: incorrect received messages */
    if (temp_rx_message_pt->aux_mcu_event_message.payload[2] != FALSE)
    {
        ret_val = RETURN_AUX_STAT_INV_MAIN_MSG;
    }
    else if (temp_rx_message_pt->aux_mcu_event_message.payload[3] != FALSE)
    {
        ret_val = RETURN_AUX_STAT_TOO_MANY_CB;
    }
    else if (temp_rx_message_pt->aux_mcu_event_message.payload[4] != FALSE)
    {
        ret_val = RETURN_AUX_STAT_ADC_WATCHDOG_FIRED;
    }
    else if (temp_rx_message_pt->aux_mcu_event_message.payload[0] != FALSE)
    {
        /* Second priority: check if BLE is enabled */
        if (temp_rx_message_pt->aux_mcu_event_message.payload[1] != FALSE)
        {
            ret_val = RETURN_AUX_STAT_OK_WITH_BLE;
        }
        else
        {
            ret_val = RETURN_AUX_STAT_BLE_ISSUE;
        }
    }
    else
    {
        ret_val = RETURN_AUX_STAT_OK;
    }

    /* Rearm receive */
    comms_aux_arm_rx_and_clear_no_comms();

    return ret_val;
}

/*! \fn     comms_aux_mcu_deal_with_ble_message(aux_mcu_message_t* received_message, msg_restrict_type_te answer_restrict_type)
*   \brief  Deal with received BLE message
*   \param  received_message        Pointer to received message
*   \param  answer_restrict_type    Message restriction type, which can be used to allow bonding information storage
*   \return Type of message that was received (see enum)
*/
comms_msg_rcvd_te comms_aux_mcu_deal_with_ble_message(aux_mcu_message_t* received_message, msg_restrict_type_te answer_restrict_type)
{
    uint16_t received_message_id = received_message->ble_message.message_id;
    uint16_t received_message_type = received_message->message_type;
    comms_msg_rcvd_te return_val = BLE_CMD_MSG_RCVD;
    
    switch(received_message->ble_message.message_id)
    {
        case BLE_MESSAGE_GET_BT_6_DIGIT_CODE:
        {
            if (answer_restrict_type == MSG_RESTRICT_ALLBUT_BOND_STORE)
            {
                /* Get Bluetooth PIN from user */
                uint8_t bluetooth_pin[6];
                RET_TYPE prompt_return_val = gui_prompts_get_six_digits_pin(bluetooth_pin, ENTER_BT_PIN_TEXT_ID);
                
                /* Prepare answer and send it */
                aux_mcu_message_t* temp_send_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(received_message_type);
                memcpy(temp_send_message_pt->ble_message.payload, bluetooth_pin, sizeof(bluetooth_pin));
                if (prompt_return_val == RETURN_OK)
                {
                    temp_send_message_pt->payload_length1 = sizeof(received_message_id) + 6;
                } 
                else
                {
                    temp_send_message_pt->payload_length1 = sizeof(received_message_id);
                }
                temp_send_message_pt->ble_message.message_id = received_message_id;
                comms_aux_mcu_send_message(temp_send_message_pt);
                return_val = BLE_6PIN_REQ_RCVD;
                
                /* Clear buffer */
                memset(bluetooth_pin, 0, sizeof(bluetooth_pin));
            } 
            else
            {
                /* Prepare answer and send it */
                aux_mcu_message_t* temp_send_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(received_message_type);
                temp_send_message_pt->payload_length1 = sizeof(received_message_id);
                temp_send_message_pt->ble_message.message_id = received_message_id;
                comms_aux_mcu_send_message(temp_send_message_pt);
                return_val = BLE_6PIN_REQ_RCVD;            
            }               
            break;
        }
        case BLE_MESSAGE_STORE_BOND_INFO:
        {
            if (answer_restrict_type == MSG_RESTRICT_ALLBUT_BOND_STORE)
            {
                /* We are allowed to store bonding information, check if we actually can */
                if (nodemgmt_store_bluetooth_bonding_information(&received_message->ble_message.bonding_information_to_store_message) == RETURN_OK)
                {
                    /* Set connected state */
                    return_val = BLE_BOND_STORE_RCVD;
                    logic_bluetooth_set_connected_state(TRUE);
                }
                else
                {
                    /* Not seeing the point of displaying a notification as our memory is huge, especially given the added logic in case we're filtering for other messages */
                    main_reboot();
                }
            }
            break;
        }
        case BLE_MESSAGE_RECALL_BOND_INFO:
        {
            /* Prepare answer */
            aux_mcu_message_t* temp_send_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(received_message_type);
            temp_send_message_pt->ble_message.message_id = received_message_id;
            
            /* Try to fetch bonding information */
            if (nodemgmt_get_bluetooth_bonding_information_for_mac_addr(received_message->ble_message.payload[0], &received_message->ble_message.payload[1], &temp_send_message_pt->ble_message.bonding_information_to_store_message) == RETURN_OK)
            {
                temp_send_message_pt->payload_length1 = sizeof(temp_send_message_pt->ble_message.message_id) + sizeof(temp_send_message_pt->ble_message.bonding_information_to_store_message);
            }
            else
            {
                temp_send_message_pt->payload_length1 = sizeof(temp_send_message_pt->ble_message.message_id);
            }

            /* Send message */
            comms_aux_mcu_send_message(temp_send_message_pt);
            break;
        }
        case BLE_MESSAGE_RECALL_BOND_INFO_IRK:
        {
            /* Prepare answer */
            aux_mcu_message_t* temp_send_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(received_message_type);
            temp_send_message_pt->ble_message.message_id = received_message_id;
            
            /* Try to fetch bonding information */
            if (nodemgmt_get_bluetooth_bonding_information_for_irk(received_message->ble_message.payload, &temp_send_message_pt->ble_message.bonding_information_to_store_message) == RETURN_OK)
            {
                temp_send_message_pt->payload_length1 = sizeof(temp_send_message_pt->ble_message.message_id) + sizeof(temp_send_message_pt->ble_message.bonding_information_to_store_message);
            }
            else
            {
                temp_send_message_pt->payload_length1 = sizeof(temp_send_message_pt->ble_message.message_id);
            }

            /* Send message */
            comms_aux_mcu_send_message(temp_send_message_pt);
            break;
        }
        case BLE_MESSAGE_GET_IRK_KEYS:
        {
            uint16_t nb_irk_keys;
            
            /* Prepare answer */
            aux_mcu_message_t* temp_send_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(received_message_type);
            temp_send_message_pt->ble_message.message_id = received_message_id;
            
            /* Static asserts */
            _Static_assert(NB_MAX_BONDING_INFORMATION + 1 < (AUX_MCU_MSG_PAYLOAD_LENGTH-2)/MEMBER_SIZE(nodemgmt_bluetooth_bonding_information_t,peer_irk_key), "Message size doesn't allow storage of all IRK keys");
            
            /* Store IRKs: first 2 bytes is the number of IRKs, next is the aggregated keys, then an empty IRK key */
            nodemgmt_get_bluetooth_bonding_information_irks(&nb_irk_keys, &(temp_send_message_pt->ble_message.payload[2]));
            
            /* Add empty irk key to the count (set to 0 by the memset above) */
            nb_irk_keys += 1;
            
            /* Update payload size */
            temp_send_message_pt->payload_length1 = sizeof(nb_irk_keys) + nb_irk_keys*MEMBER_SIZE(nodemgmt_bluetooth_bonding_information_t,peer_irk_key);
            temp_send_message_pt->ble_message.payload_as_uint16_t[0] = nb_irk_keys;

            /* Send message */
            comms_aux_mcu_send_message(temp_send_message_pt);
            break;
        }
        default: 
        {
            /* Flag invalid message */
            comms_aux_mcu_set_invalid_message_received();
            break;
        }            
    }    
    
    return return_val;
}

/*! \fn     comms_aux_mcu_deal_with_received_event(aux_mcu_message_t* received_message)
*   \brief  Deal with received aux MCU event
*   \param  received_message    Pointer to received message
*/
void comms_aux_mcu_deal_with_received_event(aux_mcu_message_t* received_message)
{
    switch(received_message->main_mcu_command_message.command)
    {
        case AUX_MCU_EVENT_BLE_ENABLED:
        {
            /* BLE just got enabled */
            comms_aux_mcu_update_timeout_delay(AUX_MCU_MESSAGE_REPLY_TIMEOUT_MS);
            logic_aux_mcu_set_ble_enabled_bool(TRUE);
            break;
        }
        case AUX_MCU_EVENT_USB_ENUMERATED:
        {
            nodemgmt_allow_new_change_number_increment();
            logic_aux_mcu_set_usb_enumerated_bool(TRUE);
            logic_device_set_state_changed();
            break;
        }
        case AUX_MCU_EVENT_CHARGE_DONE:
        {
            power_eoc_event_diag_data_t diag_data;
            diag_data.peak_voltage_level = received_message->main_mcu_command_message.payload_as_uint16[0];
            diag_data.nb_abnormal_eoc = received_message->main_mcu_command_message.payload_as_uint16[1];
            diag_data.peak_voltage_level = received_message->main_mcu_command_message.payload_as_uint16[3];
            diag_data.peak_voltage_level = diag_data.peak_voltage_level << 16;
            diag_data.peak_voltage_level += received_message->main_mcu_command_message.payload_as_uint16[2];
            logic_power_store_aux_charge_done_diag_data(&diag_data);
            logic_power_set_battery_level_update_from_aux(10);
            logic_power_set_battery_charging_bool(FALSE, TRUE);
            break;
        }
        case AUX_MCU_EVENT_CHARGE_FAIL:
        {
            logic_power_set_battery_charging_bool(FALSE, FALSE);
            logic_power_signal_battery_error();
            break;
        }
        case AUX_MCU_EVENT_BLE_CONNECTED:
        {
            logic_bluetooth_set_do_not_lock_device_after_disconnect_flag(FALSE);
            nodemgmt_allow_new_change_number_increment();
            logic_bluetooth_set_connected_state(TRUE);
            logic_device_set_state_changed();
            break;
        }
        case AUX_MCU_EVENT_BLE_DISCONNECTED:
        {
            /* If user selected to, lock device */
            if ((logic_bluetooth_get_do_not_lock_device_after_disconnect_flag() == FALSE) && (logic_bluetooth_get_state() == BT_STATE_CONNECTED) && (logic_security_is_smc_inserted_unlocked() != FALSE) && (logic_aux_mcu_is_usb_enumerated() == FALSE) && ((BOOL)custom_fs_settings_get_device_setting(SETTINGS_LOCK_ON_DISCONNECT) != FALSE))
            {
                logic_user_set_user_to_be_logged_off_flag();
            }
            
            /* If user selected to, switch off device */
            if (((BOOL)custom_fs_settings_get_device_setting(SETTINGS_SWITCH_OFF_AFTER_BT_DISC) != FALSE) && (logic_power_get_power_source() != USB_POWERED))
            {
                logic_device_power_off();
            }
            
            logic_user_reset_computer_locked_state(FALSE);
            logic_bluetooth_set_connected_state(FALSE);
            break;
        }
        case AUX_MCU_EVENT_CHARGE_LVL_UPDATE:
        {
            logic_power_set_battery_level_update_from_aux(received_message->aux_mcu_event_message.payload[0]);
            break;
        }
        case AUX_MCU_EVENT_USB_TIMEOUT:
        {
            logic_device_set_usb_timeout_detected();
            logic_device_activity_detected();
            break;
        }
        case AUX_MCU_EVENT_BLE_CON_SPAM:
        {
            logic_bluetooth_set_too_many_failed_connections();
            break;
        }
        default: 
        {
            /* Flag invalid message */
            comms_aux_mcu_set_invalid_message_received();
            break;
        }            
    }
}

/*! \fn     comms_msg_rcvd_te comms_aux_mcu_handle_fido2_auth_cred_msg(fido2_message_t* received_message)
*   \brief  routine handling authenticating a credential
*   \param  received_message    The received message
*   \return fido2_MSG_RCVD
*/
static comms_msg_rcvd_te comms_aux_mcu_handle_fido2_auth_cred_msg(fido2_message_t* received_message)
{
    fido2_auth_cred_req_message_t* incoming_message = &received_message->fido2_auth_cred_req_message;
    logic_fido2_process_exclude_list_item(incoming_message);
    return FIDO2_MSG_RCVD;
}

/*! \fn     comms_aux_mcu_handle_fido2_make_credential_msg(fido2_message_t* received_message)
*   \brief  routine handling making a new credential
*   \param  received_message    The received message
*   \return FIDO2_MSG_RCVD
*/
static comms_msg_rcvd_te comms_aux_mcu_handle_fido2_make_credential_msg(fido2_message_t* received_message)
{
    fido2_make_credential_req_message_t* request = &received_message->fido2_make_credential_req_message;
    logic_fido2_process_make_credential(request);
    return FIDO2_MSG_RCVD;
}

/*! \fn     comms_aux_mcu_handle_fido2_get_assertion_msg(fido2_message_t* received_message)
*   \brief  routine handling asserting a credential
*   \param  received_message    The received message
*   \param  send_message        The response message
*   \return FIDO2_MSG_RCVD
*/
static comms_msg_rcvd_te comms_aux_mcu_handle_fido2_get_assertion_msg(fido2_message_t* received_message)
{
    fido2_get_assertion_req_message_t* request = &received_message->fido2_get_assertion_req_message;
    logic_fido2_process_get_assertion(request);
    return FIDO2_MSG_RCVD;
}

/*! \fn     comms_aux_mcu_handle_fido2_unknown_msg(fido2_message_t const *received_message)
*   \brief  routine handling unknown fido2 messages
*   \param  received_message The received message
*   \param  send_message The response message
*   \return UNKNOW_MSG_RCVD
*/
static comms_msg_rcvd_te comms_aux_mcu_handle_fido2_unknown_msg(fido2_message_t const *received_message)
{
    return UNKNOW_MSG_RCVD;
}

/*! \fn     comms_aux_mcu_handle_fido2_message(fido2_message_t *received_message)
*   \brief  routine dealing with fido2 messages
*   \param  received_message The received message
*   \return fido2_MSG_RCVD or UNKNOW_MSG_RCVD
*/
static comms_msg_rcvd_te comms_aux_mcu_handle_fido2_message(fido2_message_t* received_message)
{
    uint16_t message_type = received_message->message_type;
    comms_msg_rcvd_te msg_rcvd = UNKNOW_MSG_RCVD;

    if (message_type >= AUX_MCU_MSG_TYPE_FIDO2_START && message_type <= AUX_MCU_MSG_TYPE_FIDO2_END)
    {
        switch (message_type)
        {
            case AUX_MCU_FIDO2_AUTH_CRED_REQ:
            {
                msg_rcvd = comms_aux_mcu_handle_fido2_auth_cred_msg(received_message);
                break;
            }
            case AUX_MCU_FIDO2_MC_REQ:
            {
                msg_rcvd = comms_aux_mcu_handle_fido2_make_credential_msg(received_message);
                break;
            }
            case AUX_MCU_FIDO2_GA_REQ:
            {
                msg_rcvd = comms_aux_mcu_handle_fido2_get_assertion_msg(received_message);
                break;
            }
            case AUX_MCU_FIDO2_AUTH_CRED_RSP:
            case AUX_MCU_FIDO2_MC_RSP:
            case AUX_MCU_FIDO2_GA_RSP:
            {
                msg_rcvd = comms_aux_mcu_handle_fido2_unknown_msg(received_message);
                break;
            }
            default:
            {
                /* Flag invalid message */
                comms_aux_mcu_set_invalid_message_received();
                
                msg_rcvd = UNKNOW_MSG_RCVD;
                break;
            }
        }
    }

    return msg_rcvd;
}

/*! \fn     comms_aux_mcu_prepare_for_active_rx_packet_receive(void)
*   \brief  This function is called to whenever we are preparing for an active RX wait.
*           The calling function may be located inside the comms_aux_mcu_routine call stack
*           OR outside of it. This makes things particularly tricky as comms_aux_mcu_routine deals
*           with packets when they are not completely received.
*           This function therefore makes sure the comms_aux_mcu_routine isn't disturbed by an active wait
*   \note   After calling this routine, to not disturb the rest of the logic, we actively awaited packet should be received as a whole (clearing dma flags)
*/
void comms_aux_mcu_prepare_for_active_rx_packet_receive(void)
{
    /* To know if this is called from within the comms_aux_mcu_routine call stack, where we'd need to rearm the rx transfer */
    if (aux_mcu_comms_aux_mcu_routine_function_called != FALSE)
    {
        /* If we just answered a message based on its first bytes */
        if (aux_mcu_message_answered_using_first_bytes != FALSE)
        {
            /* Wait for full packet reception */
            dma_aux_mcu_wait_for_current_packet_reception_and_clear_flag();
            
            /* Clear flag */
            aux_mcu_message_answered_using_first_bytes = FALSE;
            
            /* Rearm RX */
            comms_aux_arm_rx_and_clear_no_comms();
        }
        else if (aux_mcu_comms_prev_aux_mcu_routine_wants_to_arm_rx != FALSE)
        {
            /* Full packet receive arm RX and set flag to not rearm twice rx */
            comms_aux_arm_rx_and_clear_no_comms();
            aux_mcu_comms_prev_aux_mcu_routine_wants_to_arm_rx = FALSE;
        }
    }
    else
    {
        /* Called outside of comms_aux_mcu_routine */
        
        /* If we just previously answered a message based on its first bytes */
        if (aux_mcu_message_answered_using_first_bytes != FALSE)
        {
            /* Wait for full packet reception */
            dma_aux_mcu_wait_for_current_packet_reception_and_clear_flag();
            
            /* Clear flag */
            aux_mcu_message_answered_using_first_bytes = FALSE;
            
            /* Rearm RX */
            comms_aux_arm_rx_and_clear_no_comms();
        }    
        else
        {
            /* Nothing to do: called from outside comms_aux_mcu_routine, with DMA already acked */
        }        
    }
}

/*! \fn     comms_aux_mcu_routine(msg_restrict_type_te answer_restrict_type)
*   \brief  Routine dealing with aux mcu comms
*   \param  answer_restrict_type    Enum restricting which messages we can answer
*   \return The type of message received, if any
*   \note   The message for which the type of message is return may or may not be valid!
*/
comms_msg_rcvd_te comms_aux_mcu_routine(msg_restrict_type_te answer_restrict_type)
{    
#ifdef EMULATOR_BUILD
    DELAYMS(1);
#endif

    /* Comms disabled? */
    if (comms_aux_mcu_are_comms_disabled() != FALSE)
    {
        return NO_MSG_RCVD;
    }

    /* Recursivity: set function called flag */
    BOOL function_already_called = FALSE;
    if (aux_mcu_comms_aux_mcu_routine_function_called == FALSE)
    {
        aux_mcu_comms_aux_mcu_routine_function_called = TRUE;
    }
    else
    {
        function_already_called = TRUE;
    }

    /* Recursivity: re-arm rx if previous function call wanted to */
    if ((function_already_called != FALSE) && (aux_mcu_comms_prev_aux_mcu_routine_wants_to_arm_rx != FALSE))
    {
        comms_aux_arm_rx_and_clear_no_comms();
        aux_mcu_comms_prev_aux_mcu_routine_wants_to_arm_rx = FALSE;
    }

    /* Ongoing RX transfer received bytes */
    uint16_t nb_received_bytes_for_ongoing_transfer = sizeof(aux_mcu_receive_message) - dma_aux_mcu_get_remaining_bytes_for_rx_transfer();

    /* For return: type of message received */
    comms_msg_rcvd_te msg_rcvd = NO_MSG_RCVD;

    /* Bool to treat packet */
    BOOL should_deal_with_packet = FALSE;

    /* Bool to specify if we should rearm RX DMA transfer */
    BOOL arm_rx_transfer = FALSE;

    /* Received message payload length */
    uint16_t payload_length = 0;
    /* First part of message */
    if ((nb_received_bytes_for_ongoing_transfer >= sizeof(aux_mcu_receive_message.message_type) + sizeof(aux_mcu_receive_message.payload_length1)) && (aux_mcu_message_answered_using_first_bytes == FALSE))
    {
        /* Issue no comms just in case */
        platform_io_set_no_comms();
        
        /* Check if we were too slow to deal with the message before complete packet transfer */
        if (dma_aux_mcu_check_and_clear_dma_transfer_flag() != FALSE)
        {
            /* Complete packet receive, treat packet if valid flag is set or payload length #1 != 0 */
            aux_mcu_message_answered_using_first_bytes = FALSE;

            if (aux_mcu_receive_message.payload_length1 != 0)
            {
                arm_rx_transfer = TRUE;
                should_deal_with_packet = TRUE;
                payload_length = aux_mcu_receive_message.payload_length1;
                aux_mcu_comms_prev_aux_mcu_routine_wants_to_arm_rx = TRUE;
            }
            else if (aux_mcu_receive_message.rx_payload_valid_flag != 0)
            {
                arm_rx_transfer = TRUE;
                should_deal_with_packet = TRUE;
                payload_length = aux_mcu_receive_message.payload_length2;
                aux_mcu_comms_prev_aux_mcu_routine_wants_to_arm_rx = TRUE;
            }
            else
            {
                /* Arm next RX DMA transfer */
                comms_aux_arm_rx_and_clear_no_comms();
                
                /* FLag invalid message */
                comms_aux_mcu_set_invalid_message_received();
            }
        }
        else if ((aux_mcu_receive_message.payload_length1 != 0) && (nb_received_bytes_for_ongoing_transfer >= sizeof(aux_mcu_receive_message.message_type) + sizeof(aux_mcu_receive_message.payload_length1) + aux_mcu_receive_message.payload_length1))
        {
            /* First part receive, payload is small enough so we can answer */
            should_deal_with_packet = TRUE;
            aux_mcu_message_answered_using_first_bytes = TRUE;
            payload_length = aux_mcu_receive_message.payload_length1;
        }
    }
    else if (dma_aux_mcu_check_and_clear_dma_transfer_flag() != FALSE)
    {
        /* Second part transfer, check if we have already dealt with this packet and if it is valid */
        if ((aux_mcu_message_answered_using_first_bytes == FALSE) && ((aux_mcu_receive_message.payload_length1 != 0) || ((aux_mcu_receive_message.payload_length1 == 0) && (aux_mcu_receive_message.rx_payload_valid_flag != 0))))
        {
            arm_rx_transfer = TRUE;
            should_deal_with_packet = TRUE;
            if (aux_mcu_receive_message.payload_length1 == 0)
            {
                payload_length = aux_mcu_receive_message.payload_length2;
            }
            else
            {
                payload_length = aux_mcu_receive_message.payload_length1;
            }
            aux_mcu_comms_prev_aux_mcu_routine_wants_to_arm_rx = TRUE;
        }
        else
        {
            /* Arm next RX DMA transfer */
            comms_aux_arm_rx_and_clear_no_comms();
        }
        
        if ((aux_mcu_receive_message.payload_length1 == 0) && (aux_mcu_receive_message.rx_payload_valid_flag == 0))
        {
            /* Flag invalid message */
            comms_aux_mcu_set_invalid_message_received();
        }

        /* Reset bool */
        aux_mcu_message_answered_using_first_bytes = FALSE;
    }
    
    /* Invalid payload length, rearm dma */
    if ((should_deal_with_packet != FALSE) && (payload_length > AUX_MCU_MSG_PAYLOAD_LENGTH))
    {
        /* Arm next RX DMA transfer */
        comms_aux_arm_rx_and_clear_no_comms();
        
        /* Flag invalid message */
        comms_aux_mcu_set_invalid_message_received();
    }

    /* Return if we shouldn't deal with packet, or if payload has the incorrect size */
    if ((should_deal_with_packet == FALSE) || (payload_length > AUX_MCU_MSG_PAYLOAD_LENGTH))
    {
        /* Recursivity: remove flag */
        if (function_already_called == FALSE)
        {
            aux_mcu_comms_aux_mcu_routine_function_called = FALSE;
        }

        return NO_MSG_RCVD;
    }
    
    /* Here there's a message we need to deal with. If we're awake but screen off, increase the fake screen timer to leave time for a potential next message */
    if (platform_io_get_voled_stepup_pwr_source() == OLED_STEPUP_SOURCE_NONE)
    {
        timer_start_timer(TIMER_SCREEN, SLEEP_AFTER_AUX_WAKEUP_MS);
    }

    /* USB / BLE Messages */
    if ((aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_USB) || (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_BLE))
    {        
        /* Store interface bool */
        BOOL is_message_from_usb = (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_USB)?TRUE:FALSE;
        
        /* Bool if parsing HID message required */
        BOOL hid_parsing_required = TRUE;
        
        /* Depending on command ID, prepare return */
        if (aux_mcu_receive_message.hid_message.message_type == HID_CMD_ID_CANCEL_REQ)
        {
            msg_rcvd = HID_CANCEL_MSG_RCVD;
            hid_parsing_required = FALSE;
        }
        else if (aux_mcu_receive_message.hid_message.message_type == HID_CMD_ID_REINDEX_BUNDLE)
        {
            msg_rcvd = HID_REINDEX_BUNDLE_RCVD;
        }
        else
        {
            msg_rcvd = HID_MSG_RCVD;
        }

        /* Parse message */
        #ifndef DEBUG_USB_COMMANDS_ENABLED
        if (hid_parsing_required != FALSE)
        {
            comms_hid_msgs_parse(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), answer_restrict_type, is_message_from_usb);
        }
        #else
        if (hid_parsing_required != FALSE)
        {
            if (aux_mcu_receive_message.hid_message.message_type >= HID_MESSAGE_START_CMD_ID_DBG)
            {
                comms_hid_msgs_parse_debug(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), answer_restrict_type, is_message_from_usb);
                msg_rcvd = HID_DBG_MSG_RCVD;
            }
            else
            {
                comms_hid_msgs_parse(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), answer_restrict_type, is_message_from_usb);
            }
        }        
        #endif
    }
    else if (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_BOOTLOADER)
    {
        msg_rcvd = BL_MSG_RCVD;
        asm("Nop");
    }
    else if (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_MAIN_MCU_CMD)
    {
        msg_rcvd = MAIN_MCU_MSG_RCVD;
        asm("Nop");
    }
    else if (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_AUX_MCU_EVENT)
    {
        msg_rcvd = EVENT_MSG_RCVD;

        /* Call dedicated function */
        comms_aux_mcu_deal_with_received_event(&aux_mcu_receive_message);
    }
    else if (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_RNG_TRANSFER)
    {
        msg_rcvd = RNG_MSG_RCVD;

        /* Set same message type and fill with random numbers */
        aux_mcu_message_t* temp_send_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(aux_mcu_receive_message.message_type);
        rng_fill_array(temp_send_message_pt->payload, 32);
        temp_send_message_pt->payload_length1 = 32;

        /* Send message */
        comms_aux_mcu_send_message(temp_send_message_pt);
    }
    else if (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_FIDO2)
    {
        if (answer_restrict_type == MSG_NO_RESTRICT)
        {
            msg_rcvd = comms_aux_mcu_handle_fido2_message(&aux_mcu_receive_message.fido2_message);
        } 
        else
        {
            /* Filtering, send a please retry packet to aux MCU */
            aux_mcu_message_t* temp_send_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
            temp_send_message_pt->payload_length1 = sizeof(temp_send_message_pt->fido2_message.message_type);
            temp_send_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_RETRY;
            comms_aux_mcu_send_message(temp_send_message_pt);
            msg_rcvd = FIDO2_MSG_RCVD;
        }
    }
    else if (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_BLE_CMD)
    {
        msg_rcvd = comms_aux_mcu_deal_with_ble_message(&aux_mcu_receive_message, answer_restrict_type);
    }
    else
    {
        /* Flag invalid message */
        comms_aux_mcu_set_invalid_message_received();
        
        msg_rcvd = UNKNOW_MSG_RCVD;
        asm("Nop");
    }

    /* If we need to rearm RX */
    if (arm_rx_transfer != FALSE)
    {
        /* Check that a possible recursion didn't already rearm RX for us */
        if ((function_already_called != FALSE) || (aux_mcu_comms_prev_aux_mcu_routine_wants_to_arm_rx != FALSE))
        {
            aux_mcu_comms_prev_aux_mcu_routine_wants_to_arm_rx = FALSE;
            comms_aux_arm_rx_and_clear_no_comms();
        }
    }

    /* Recursivity: remove flag */
    if (function_already_called == FALSE)
    {
        aux_mcu_comms_aux_mcu_routine_function_called = FALSE;
    }

    /* Return type of message received */
    return msg_rcvd;
}

/*! \fn     comms_aux_mcu_wait_for_aux_event(uint16_t aux_mcu_event)
*   \brief  Actively wait for an event from the aux MCU
*   \param  aux_mcu_event   The event to wait for
*   \return A pointer to the received message
*   \note   Do not call the power switching routines inside the while loop due to rx buffer reuse.
*/
aux_mcu_message_t* comms_aux_mcu_wait_for_aux_event(uint16_t aux_mcu_event)
{
    aux_mcu_message_t* temp_rx_message_pt;
    uint16_t nb_loops_done = 0;
    
    /* Wait for the expected event... but not indefinitely */
    while ((comms_aux_mcu_active_wait(&temp_rx_message_pt, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, FALSE, aux_mcu_event) != RETURN_OK) && (nb_loops_done < NB_ACTIVE_WAIT_LOOP_TIMEOUT))
    {
        nb_loops_done++;
    }
    
    /* Rearm comms only if we received the message */
    if (nb_loops_done != NB_ACTIVE_WAIT_LOOP_TIMEOUT)
    {
        comms_aux_arm_rx_and_clear_no_comms();
    }
    
    #ifdef EMULATOR_BUILD
    if (aux_mcu_event == AUX_MCU_EVENT_ATTACH_CMD_RCVD)
    {
        logic_aux_mcu_set_usb_enumerated_bool(TRUE);
        logic_device_set_state_changed();
    }
    #endif
    
    return temp_rx_message_pt;
}

/*! \fn     comms_aux_mcu_active_wait(aux_mcu_message_t** rx_message_pt_pt, uint16_t expected_packet, BOOL single_try, int16_t expected_event)
*   \brief  Active wait for a message from the aux MCU.
*   \param  rx_message_pt_pt        Pointer to where to store the pointer to the received message
*   \param  expected_packet         Expected packet
*   \param  single_try              Set to TRUE to not use any timeout
*   \param  expected_event          If an event is expected, event ID or -1
*   \return OK if a message was received
*   \note   Special care must be taken to discard other message we don't want (either with a please_retry or other mechanisms)
*   \note   DMA RX arm must be called to rearm message receive as a rearm in this code would enable data to be overwritten
*   \note   This function is not touching the no comms signal except in case the wrong message type isn't received
*/
RET_TYPE comms_aux_mcu_active_wait(aux_mcu_message_t** rx_message_pt_pt, uint16_t expected_packet, BOOL single_try, int16_t expected_event)
{
    uint16_t temp_timer_id;
    
    /* Bool for the do{} */
    BOOL reloop = FALSE;
    
    /* Prepare for active wait */
    comms_aux_mcu_prepare_for_active_rx_packet_receive();
    
    /* Comms disabled? Should not happen... */
    if (comms_aux_mcu_are_comms_disabled() != FALSE)
    {
        main_reboot();
    }

    /* Arm timer */
    if (single_try == FALSE)
    {
        temp_timer_id = timer_get_and_start_timer(aux_mcu_comms_timeout_delay);
    }
    else
    {
        temp_timer_id = timer_get_and_start_timer(0);
    }

    do
    {
        /* Do not reloop by default */
        reloop = FALSE;

        /* Wait for complete message to be received */
        BOOL dma_check_return = FALSE;
        timer_flag_te timer_flag_return = TIMER_RUNNING;
        while((dma_check_return == FALSE) && (timer_flag_return == TIMER_RUNNING))
        {
            dma_check_return = dma_aux_mcu_check_and_clear_dma_transfer_flag();
            timer_flag_return = timer_has_allocated_timer_expired(temp_timer_id, FALSE);
        }

        /* Did the timer expire? */
        if (dma_check_return == FALSE)
        {
            /* Free timer */
            timer_deallocate_timer(temp_timer_id);
            return RETURN_NOK;
        }

        /* Get payload length */
        uint16_t payload_length;
        if (aux_mcu_receive_message.payload_length1 != 0)
        {
            payload_length = aux_mcu_receive_message.payload_length1;
        }
        else
        {
            payload_length = aux_mcu_receive_message.payload_length2;
        }

        /* Check if message is invalid */
        if ((payload_length > AUX_MCU_MSG_PAYLOAD_LENGTH) || ((aux_mcu_receive_message.payload_length1 == 0) && (aux_mcu_receive_message.rx_payload_valid_flag == 0)))
        {
            /* Reloop, rearm receive */
            reloop = TRUE;
            dma_aux_mcu_check_and_clear_dma_transfer_flag();
            comms_aux_mcu_set_invalid_message_received();
            comms_aux_arm_rx_and_clear_no_comms();
        }

        /* Check if received message is the one we expected */
        if ((aux_mcu_receive_message.message_type != expected_packet) || ((expected_event >= 0) && (aux_mcu_receive_message.aux_mcu_event_message.event_id != expected_event)))
        {
            /* Reloop, clear DMA flag */
            reloop = TRUE;
            dma_aux_mcu_check_and_clear_dma_transfer_flag();
            
            if ((aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_AUX_MCU_EVENT) && (aux_mcu_receive_message.aux_mcu_event_message.event_id != expected_event))
            {
                /* Received another event... deal with it (doesn't generate answers */
                comms_aux_mcu_deal_with_received_event(&aux_mcu_receive_message);
            }
            else if ((aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_USB) || (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_BLE))
            {                
                /* Store interface bool */
                BOOL is_message_from_usb = (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_USB)?TRUE:FALSE;
                                
                /* Parse message */
                #ifndef DEBUG_USB_COMMANDS_ENABLED
                comms_hid_msgs_parse(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), MSG_RESTRICT_ALL, is_message_from_usb);
                #else
                if (aux_mcu_receive_message.hid_message.message_type >= HID_MESSAGE_START_CMD_ID_DBG)
                {
                    comms_hid_msgs_parse_debug(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), MSG_RESTRICT_ALL, is_message_from_usb);
                }
                else
                {
                    comms_hid_msgs_parse(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), MSG_RESTRICT_ALL, is_message_from_usb);
                }
                #endif
            }
            else if (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_RNG_TRANSFER)
            {
                /* Set same message type and fill with random numbers */
                aux_mcu_message_t* temp_send_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(aux_mcu_receive_message.message_type);
                rng_fill_array(temp_send_message_pt->payload, 32);
                temp_send_message_pt->payload_length1 = 32;
                
                /* Send message */
                comms_aux_mcu_send_message(temp_send_message_pt);
            }
            else if (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_FIDO2)
            {
                /* Send a please retry packet to aux MCU */
                aux_mcu_message_t* temp_send_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
                temp_send_message_pt->payload_length1 = sizeof(temp_send_message_pt->fido2_message.message_type);
                temp_send_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_RETRY;
                comms_aux_mcu_send_message(temp_send_message_pt);
            }
            else if (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_BLE_CMD)
            {
                /* We can still tackle these requests as they do not generate prompts */
                comms_aux_mcu_deal_with_ble_message(&aux_mcu_receive_message, MSG_RESTRICT_ALL);
            }
            
            /* Rearm receive */
            comms_aux_arm_rx_and_clear_no_comms();
        }
    }while (reloop != FALSE);

    /* Store pointer to message */
    *rx_message_pt_pt = &aux_mcu_receive_message;
    
    /* Free timer */
    timer_deallocate_timer(temp_timer_id);

    /* Return OK */
    return RETURN_OK;
}
