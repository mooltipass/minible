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
/* Flag set if comms are disabled */
BOOL aux_mcu_comms_disabled = FALSE;
/* Flag set if we have treated a message by only looking at its first bytes */
BOOL aux_mcu_message_answered_using_first_bytes = FALSE;
/* Flag set for comms_aux_mcu_routine for recursive calls */
BOOL aux_mcu_comms_aux_mcu_routine_function_called = FALSE;
/* Flag set to specify that the first aux mcu function call wanted to rearm rx */
BOOL aux_mcu_comms_prev_aux_mcu_routine_wants_to_arm_rx = FALSE;


/*! \fn     comms_aux_arm_rx_and_clear_no_comms(void)
*   \brief  Init RX communications with aux MCU
*/
void comms_aux_arm_rx_and_clear_no_comms(void)
{
    if (dma_aux_mcu_is_rx_transfer_already_init() == FALSE)
    {
        dma_aux_mcu_init_rx_transfer(AUXMCU_SERCOM, (void*)&aux_mcu_receive_message, sizeof(aux_mcu_receive_message));
    }
    else
    {
        /* Should never happen! */
    }
    platform_io_clear_no_comms();
    aux_mcu_comms_disabled = FALSE;
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
    /* Check for enabled comms */
    if (aux_mcu_comms_disabled != FALSE)
    {
        /* Wake up aux MCU and give it some time to wakeup */
        comms_aux_arm_rx_and_clear_no_comms();
        timer_delay_ms(10);
    }
    
    /* Wait for possible ongoing message to be sent */
    comms_aux_mcu_wait_for_message_sent();
    
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
    
    /* As the aux MCU has 3 buffers (one for USB messages, one for BLE messages, one for others), check for timeout in case message is of type other */
    if ((message_to_send->message_type != AUX_MCU_MSG_TYPE_USB) && (message_to_send->message_type != AUX_MCU_MSG_TYPE_BLE))
    {
        while (timer_has_timer_expired(TIMER_AUX_MCU_FLOOD, FALSE) == TIMER_RUNNING);
    }
    
    /* The function below does wait for a previous transfer to finish */
    dma_aux_mcu_init_tx_transfer(AUXMCU_SERCOM, (void*)message_to_send, sizeof(*message_to_send));
    
    /* As the aux MCU has 3 buffers (one for USB messages, one for BLE messages, one for others), start a timer in case message is of type other */
    if ((message_to_send->message_type != AUX_MCU_MSG_TYPE_USB) && (message_to_send->message_type != AUX_MCU_MSG_TYPE_BLE))
    {
        timer_start_timer(TIMER_AUX_MCU_FLOOD, AUX_FLOOD_TIMEOUT_MS);        
    }
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
    comms_hid_msgs_fill_get_status_message_answer(temp_send_message_pt->main_mcu_command_message.payload_as_uint16);
    temp_send_message_pt->payload_length1 = sizeof(temp_send_message_pt->main_mcu_command_message.command) + 4;
    temp_send_message_pt->main_mcu_command_message.command = MAIN_MCU_COMMAND_UPDT_DEV_STAT;
    comms_aux_mcu_send_message(temp_send_message_pt);
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
    comms_hid_msgs_fill_get_status_message_answer(temp_tx_message_pt->ping_with_info_message.initial_device_status_value);
    temp_tx_message_pt->payload_length1 = sizeof(ping_with_info_message_t);
    comms_aux_mcu_send_message(temp_tx_message_pt);

    /* Wait for answer: no need to parse answer as filter is done in comms_aux_mcu_active_wait */
    return_val = comms_aux_mcu_active_wait(&temp_rx_message_pt, FALSE, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, FALSE, AUX_MCU_EVENT_IM_HERE);

    /* Rearm receive */
    comms_aux_arm_rx_and_clear_no_comms();

    return return_val;
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
                gui_prompts_get_six_digits_pin(bluetooth_pin, ENTER_BT_PIN_TEXT_ID);
                
                /* Prepare answer and send it */
                aux_mcu_message_t* temp_send_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(received_message_type);
                memcpy(temp_send_message_pt->ble_message.payload, bluetooth_pin, sizeof(bluetooth_pin));
                temp_send_message_pt->payload_length1 = sizeof(received_message_id) + 6;
                temp_send_message_pt->ble_message.message_id = received_message_id;
                comms_aux_mcu_send_message(temp_send_message_pt);
                return_val = BLE_6PIN_REQ_RCVD;
                
                /* Clear buffer */
                memset(bluetooth_pin, 0, sizeof(bluetooth_pin));
                break;
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
        default: break;
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
            logic_aux_mcu_set_ble_enabled_bool(TRUE);
            break;
        }
        case AUX_MCU_EVENT_USB_ENUMERATED:
        {
            logic_aux_mcu_set_usb_enumerated_bool(TRUE);
            break;
        }
        case AUX_MCU_EVENT_USB_DETACHED:
        {
            if (logic_power_get_power_source() == TRANSITIONING_TO_BATTERY_POWER)
            {
                logic_power_set_power_source(BATTERY_POWERED);
            }
            break;
        }
        case AUX_MCU_EVENT_CHARGE_DONE:
        {
            logic_power_set_max_charging_voltage_from_aux(received_message->aux_mcu_event_message.payload_as_uint16[0]);
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
            logic_bluetooth_set_connected_state(TRUE);
            break;
        }
        case AUX_MCU_EVENT_BLE_DISCONNECTED:
        {
            /* If user selected to, lock device */
            if ((logic_bluetooth_get_do_not_lock_device_after_disconnect_flag() == FALSE) && (logic_bluetooth_get_state() == BT_STATE_CONNECTED) && (logic_security_is_smc_inserted_unlocked() != FALSE) && (logic_aux_mcu_is_usb_enumerated() == FALSE) && ((BOOL)custom_fs_settings_get_device_setting(SETTINGS_LOCK_ON_DISCONNECT) != FALSE))
            {
                logic_user_set_user_to_be_logged_off_flag();
            }
            
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
            break;
        }
        default: break;
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
                msg_rcvd = UNKNOW_MSG_RCVD;
                break;
            }
        }
    }

    return msg_rcvd;
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
    uint16_t payload_length;
    /* First part of message */
    if ((nb_received_bytes_for_ongoing_transfer >= sizeof(aux_mcu_receive_message.message_type) + sizeof(aux_mcu_receive_message.payload_length1)) && (aux_mcu_message_answered_using_first_bytes == FALSE))
    {
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
                
                /* Useless message, except for debugging */
                //comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_INVALID_MSG_RCVD);
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
            /* Useless message, except for debugging */
            //comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_INVALID_MSG_RCVD);
        }

        /* Reset bool */
        aux_mcu_message_answered_using_first_bytes = FALSE;
    }
    
    /* Invalid payload length, rearm dma */
    if ((should_deal_with_packet != FALSE) && (payload_length > AUX_MCU_MSG_PAYLOAD_LENGTH))
    {
        /* Arm next RX DMA transfer */
        comms_aux_arm_rx_and_clear_no_comms();
        
        /* Useless message, except for debugging */
        //comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_INVALID_MSG_RCVD);
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
        
        /* Depending on command ID, prepare return */
        if (aux_mcu_receive_message.hid_message.message_type == HID_CMD_ID_CANCEL_REQ)
        {
            msg_rcvd = HID_CANCEL_MSG_RCVD;
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
        comms_hid_msgs_parse(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), answer_restrict_type, is_message_from_usb);
        #else
        if (aux_mcu_receive_message.hid_message.message_type >= HID_MESSAGE_START_CMD_ID_DBG)
        {
            comms_hid_msgs_parse_debug(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), answer_restrict_type, is_message_from_usb);
        }
        else
        {
            comms_hid_msgs_parse(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), answer_restrict_type, is_message_from_usb);
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
        /* Useless message, except for debugging */
        comms_aux_mcu_send_simple_command_message(MAIN_MCU_COMMAND_INVALID_MSG_RCVD);
        
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

/*! \fn     comms_aux_mcu_active_wait(aux_mcu_message_t** rx_message_pt_pt, BOOL do_not_touch_dma_flags, uint16_t expected_packet, BOOL single_try, int16_t expected_event)
*   \brief  Active wait for a message from the aux MCU.
*   \param  rx_message_pt_pt        Pointer to where to store the pointer to the received message
*   \param  do_not_touch_dma_logic  Set to TRUE to not mess with the DMA flags
*   \param  expected_packet         Expected packet
*   \param  single_try              Set to TRUE to not use any timeout
*   \param  expected_event          If an event is expected, event ID or -1
*   \return OK if a message was received
*   \note   Special care must be taken to discard other message we don't want (either with a please_retry or other mechanisms)
*   \note   DMA RX arm must be called to rearm message receive as a rearm in this code would enable data to be overwritten
*   \note   This function is not touching the no comms signal except in case the wrong message type isn't received
*/
RET_TYPE comms_aux_mcu_active_wait(aux_mcu_message_t** rx_message_pt_pt, BOOL do_not_touch_dma_flags, uint16_t expected_packet, BOOL single_try, int16_t expected_event)
{
    /* Bool for the do{} */
    BOOL reloop = FALSE;
    
    /* Comms disabled? Should not happen... */
    if (comms_aux_mcu_are_comms_disabled() != FALSE)
    {
        main_reboot();
    }

    /* Arm timer */
    if (single_try == FALSE)
    {
        timer_start_timer(TIMER_TIMEOUT_FUNCTS, AUX_MCU_MESSAGE_REPLY_TIMEOUT_MS);
    }
    else
    {
        timer_start_timer(TIMER_TIMEOUT_FUNCTS, 0);
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
            if (do_not_touch_dma_flags == FALSE)
            {
                dma_check_return = dma_aux_mcu_check_and_clear_dma_transfer_flag();
            }
            else
            {
                dma_check_return = dma_aux_mcu_check_dma_transfer_flag();
            }
            timer_flag_return = timer_has_timer_expired(TIMER_TIMEOUT_FUNCTS, FALSE);
        }

        /* Did the timer expire? */
        if (dma_check_return == FALSE)
        {
            return RETURN_NOK;
        }
        
        /* No expiration, rearm timer! */
        if (single_try == FALSE)
        {
            timer_start_timer(TIMER_TIMEOUT_FUNCTS, AUX_MCU_MESSAGE_REPLY_TIMEOUT_MS);
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

    /* Return OK */
    return RETURN_OK;
}
