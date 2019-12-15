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
#include "comms_hid_msgs_debug.h"
#include "platform_defines.h"
#include "logic_bluetooth.h"
#include "comms_hid_msgs.h"
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "logic_power.h"
#include "sh1122.h"
#include "main.h"
#include "dma.h"
#include "rng.h"
/* Received and sent MCU messages */
aux_mcu_message_t aux_mcu_receive_message;
aux_mcu_message_t aux_mcu_send_message;
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
    dma_aux_mcu_init_rx_transfer(AUXMCU_SERCOM, (void*)&aux_mcu_receive_message, sizeof(aux_mcu_receive_message));
    platform_io_clear_no_comms();
}

/*! \fn     comms_aux_mcu_get_temp_tx_message_object_pt(void)
*   \brief  Get a pointer to our temporary tx message object
*/
aux_mcu_message_t* comms_aux_mcu_get_temp_tx_message_object_pt(void)
{
    return &aux_mcu_send_message;
}

/*! \fn     comms_aux_mcu_send_message(BOOL wait_for_send)
*   \brief  Send aux_mcu_send_message to the AUX MCU
*   \param  wait_for_send   Set to TRUE for function return when message is sent
*   \note   Transfer is done through DMA so aux_mcu_send_message will be accessed after this function returns if boolean is set to false
*/
void comms_aux_mcu_send_message(BOOL wait_for_send)
{    
    /* The function below does wait for a previous transfer to finish */
    dma_aux_mcu_init_tx_transfer(AUXMCU_SERCOM, (void*)&aux_mcu_send_message, sizeof(aux_mcu_send_message));
    
    /* If asked, wait for message sent */
    if (wait_for_send != FALSE)
    {
        dma_wait_for_aux_mcu_packet_sent();
    }
}

/*! \fn     comms_aux_mcu_wait_for_message_sent(void)
*   \brief  Wait for previous message to be sent to aux MCU
*/
void comms_aux_mcu_wait_for_message_sent(void)
{
    dma_wait_for_aux_mcu_packet_sent();
}

/*! \fn     comms_aux_mcu_send_simple_command_message(uint16_t command)
*   \brief  Send simple command message to aux MCU
*   \param  command The command to send
*/
void comms_aux_mcu_send_simple_command_message(uint16_t command)
{
    comms_aux_mcu_wait_for_message_sent();
    memset((void*)&aux_mcu_send_message, 0, sizeof(aux_mcu_send_message));
    aux_mcu_send_message.message_type = AUX_MCU_MSG_TYPE_MAIN_MCU_CMD;
    aux_mcu_send_message.payload_length1 = sizeof(aux_mcu_send_message.main_mcu_command_message.command);
    aux_mcu_send_message.main_mcu_command_message.command = command;
    comms_aux_mcu_send_message(FALSE);
}

/*! \fn     comms_aux_mcu_update_device_status_buffer(void)
*   \brief  Update the device status buffer on the aux MCU
*/
void comms_aux_mcu_update_device_status_buffer(void)
{
    comms_aux_mcu_wait_for_message_sent();
    memset((void*)&aux_mcu_send_message, 0, sizeof(aux_mcu_send_message));
    aux_mcu_send_message.message_type = AUX_MCU_MSG_TYPE_MAIN_MCU_CMD;
    aux_mcu_send_message.payload_length1 = sizeof(aux_mcu_send_message.main_mcu_command_message.command) + 4;
    comms_hid_msgs_fill_get_status_message_answer(aux_mcu_send_message.main_mcu_command_message.payload_as_uint16);
    aux_mcu_send_message.main_mcu_command_message.command = MAIN_MCU_COMMAND_UPDT_DEV_STAT;
    comms_aux_mcu_send_message(FALSE);    
}

/*! \fn     comms_aux_mcu_get_empty_packet_ready_to_be_sent(aux_mcu_message_t** message_pt_pt, uint16_t message_type)
*   \brief  Get an empty message ready to be sent
*   \param  message_pt_pt           Pointer to where to store message pointer
*   \param  message_type            Message type
*/
void comms_aux_mcu_get_empty_packet_ready_to_be_sent(aux_mcu_message_t** message_pt_pt, uint16_t message_type)
{
    /* Wait for possible ongoing message to be sent */
    comms_aux_mcu_wait_for_message_sent();
    
    /* Get pointer to our message to be sent buffer */
    aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_temp_tx_message_object_pt();
    
    /* Clear it */
    memset((void*)temp_tx_message_pt, 0, sizeof(*temp_tx_message_pt));
    
    /* Populate the fields */
    temp_tx_message_pt->message_type = message_type;
    
    /* Store message pointer */
    *message_pt_pt = temp_tx_message_pt;
}

/*! \fn     comms_aux_mcu_hard_comms_reset_with_aux_mcu_reboot(void)
*   \brief  Hard reset of aux MCU comms using an aux MCU reboot
*/
void comms_aux_mcu_hard_comms_reset_with_aux_mcu_reboot(void)
{
    aux_mcu_message_t* temp_tx_message_pt;
    
    /* Set no comms (keep platform in sleep after its reboot) */
    platform_io_set_no_comms();
    
    /* Wait for possible ongoing message to be sent */
    comms_aux_mcu_wait_for_message_sent();
    
    /* Get pointer to our message to be sent buffer */
    temp_tx_message_pt = comms_aux_mcu_get_temp_tx_message_object_pt();
    
    /* Fill message with magic 0xFF, send it twice */
    memset((void*)temp_tx_message_pt, 0xFF, sizeof(*temp_tx_message_pt));
    comms_aux_mcu_send_message(TRUE);comms_aux_mcu_send_message(TRUE);
    
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
    aux_mcu_message_t* temp_tx_message_pt;
    RET_TYPE return_val;    
    
    /* Prepare ping message and send it */
    comms_aux_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_PING_WITH_INFO);
    comms_hid_msgs_fill_get_status_message_answer(temp_tx_message_pt->ping_with_info_message.initial_device_status_value);
    temp_tx_message_pt->payload_length1 = sizeof(ping_with_info_message_t);
    comms_aux_mcu_send_message(TRUE);
    
    /* Wait for answer: no need to parse answer as filter is done in comms_aux_mcu_active_wait */
    return_val = comms_aux_mcu_active_wait(&temp_rx_message_pt, FALSE, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, FALSE, AUX_MCU_EVENT_IM_HERE);
    
    /* Rearm receive */
    comms_aux_arm_rx_and_clear_no_comms();
    
    return return_val;
}

/*! \fn     comms_aux_mcu_deal_with_received_event(aux_mcu_message_t* received_message)
*   \brief  Deal with received aux MCU event
*   \param  aux_mcu_receive_message     Pointer to received message
*/
void comms_aux_mcu_deal_with_received_event(aux_mcu_message_t* received_message)
{    
    switch(aux_mcu_receive_message.main_mcu_command_message.command)
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
        case AUX_MCU_EVENT_CHARGE_DONE:
        {
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
            logic_bluetooth_set_connected_state(TRUE);
            break;
        }
        case AUX_MCU_EVENT_BLE_DISCONNECTED:
        {
            logic_bluetooth_set_connected_state(FALSE);
            break;
        }
        default: break;
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
        
        /* Reset bool */
        aux_mcu_message_answered_using_first_bytes = FALSE;
    }
    
    /* Return if we shouldn't deal with packet, or if payload has the incorrect size */
    if ((should_deal_with_packet == FALSE) || (payload_length > AUX_MCU_MSG_PAYLOAD_LENGTH))
    {
        /* Recursivity: remove flag */
        if (function_already_called == FALSE)
        {
            aux_mcu_comms_aux_mcu_routine_function_called = FALSE;
        }
        
        /* Note: there's a case where we don't rearm DMA if the message is valid but payload is too long... was lazy to implement it */
        return NO_MSG_RCVD;
    }
            
    /* USB / BLE Messages */
    if ((aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_USB) || (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_BLE))
    {
        /* Cast payloads into correct type */
        int16_t hid_reply_payload_length = -1;
        
        /* Store message type */
        uint16_t receive_message_type = aux_mcu_receive_message.message_type;
                
        /* Clear TX message just in case */
        memset((void*)&aux_mcu_send_message, 0, sizeof(aux_mcu_send_message));

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
        hid_reply_payload_length = comms_hid_msgs_parse(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), &aux_mcu_send_message.hid_message, answer_restrict_type);
        #else
        if (aux_mcu_receive_message.hid_message.message_type >= HID_MESSAGE_START_CMD_ID_DBG)
        {
            hid_reply_payload_length = comms_hid_msgs_parse_debug(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), &aux_mcu_send_message.hid_message, answer_restrict_type);
        }
        else
        {
            hid_reply_payload_length = comms_hid_msgs_parse(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), &aux_mcu_send_message.hid_message, answer_restrict_type);
        }
        #endif
                
        /* Send reply if needed */
        if (hid_reply_payload_length >= 0)
        {
            /* Set same message type and compute payload size */
            aux_mcu_send_message.message_type = receive_message_type;
            aux_mcu_send_message.payload_length1 = hid_reply_payload_length + sizeof(aux_mcu_receive_message.hid_message.message_type) + sizeof(aux_mcu_receive_message.hid_message.payload_length);
                    
            /* Send message */
            comms_aux_mcu_send_message(FALSE);
        }
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
        aux_mcu_send_message.message_type = aux_mcu_receive_message.message_type;
        rng_fill_array(aux_mcu_send_message.payload, 32);
        aux_mcu_send_message.payload_length1 = 32;
        
        /* Send message */
        comms_aux_mcu_send_message(FALSE);
    }
    else
    {
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
            /* Reloop, rearm receive */
            reloop = TRUE;
            dma_aux_mcu_check_and_clear_dma_transfer_flag();
            comms_aux_arm_rx_and_clear_no_comms();
            
            // TODO2: take necessary action in case we received an unwanted message (left to be implemented: reqs from aux MCU */
            if ((aux_mcu_receive_message.message_type == expected_packet) && (aux_mcu_receive_message.aux_mcu_event_message.event_id != expected_event))
            {
                /* Received another event... deal with it (doesn't generate answers */
                comms_aux_mcu_deal_with_received_event(&aux_mcu_receive_message);
            }
            else if ((aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_USB) || (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_BLE))
            {
                /* Cast payloads into correct type */
                int16_t hid_reply_payload_length = -1;
                
                /* Store message type */
                uint16_t receive_message_type = aux_mcu_receive_message.message_type;
                
                /* Clear TX message just in case */
                memset((void*)&aux_mcu_send_message, 0, sizeof(aux_mcu_send_message));
                
                /* Parse message */
                #ifndef DEBUG_USB_COMMANDS_ENABLED
                hid_reply_payload_length = comms_hid_msgs_parse(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), &aux_mcu_send_message.hid_message, MSG_RESTRICT_ALL);
                #else
                if (aux_mcu_receive_message.hid_message.message_type >= HID_MESSAGE_START_CMD_ID_DBG)
                {
                    hid_reply_payload_length = comms_hid_msgs_parse_debug(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), &aux_mcu_send_message.hid_message, MSG_RESTRICT_ALL);
                }
                else
                {
                    hid_reply_payload_length = comms_hid_msgs_parse(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), &aux_mcu_send_message.hid_message, MSG_RESTRICT_ALL);
                }
                #endif
                
                /* Send reply if needed */
                if (hid_reply_payload_length >= 0)
                {
                    /* Set same message type and compute payload size */
                    aux_mcu_send_message.message_type = receive_message_type;
                    aux_mcu_send_message.payload_length1 = hid_reply_payload_length + sizeof(aux_mcu_receive_message.hid_message.message_type) + sizeof(aux_mcu_receive_message.hid_message.payload_length);
                    
                    /* Send message */
                    comms_aux_mcu_send_message(FALSE);
                }
            }
            else if (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_RNG_TRANSFER)
            {                
                /* Clear TX message just in case */
                memset((void*)&aux_mcu_send_message, 0, sizeof(aux_mcu_send_message));
                
                aux_mcu_send_message.message_type = aux_mcu_receive_message.message_type;
                rng_fill_array(aux_mcu_send_message.payload, 32);
                aux_mcu_send_message.payload_length1 = 32;
                
                /* Send message */
                comms_aux_mcu_send_message(FALSE);
            }
        }
    }while (reloop != FALSE);
        
    /* Store pointer to message */
    *rx_message_pt_pt = &aux_mcu_receive_message;
        
    /* Return OK */
    return RETURN_OK;
}
