/*!  \file     comms_aux_mcu.c
*    \brief    Communications with aux MCU
*    Created:  03/03/2018
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include <asf.h>
#include "comms_hid_msgs_debug.h"
#include "platform_defines.h"
#include "comms_hid_msgs.h"
#include "comms_aux_mcu.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "defines.h"
#include "dma.h"
/* Received and sent MCU messages */
aux_mcu_message_t aux_mcu_receive_message;
aux_mcu_message_t aux_mcu_send_message;
/* Flag set if we have treated a message by only looking at its first bytes */
BOOL aux_mcu_message_answered_using_first_bytes = FALSE;


/*! \fn     comms_aux_arm_rx_and_clear_no_comms(void)
*   \brief  Init communications with aux MCU
*/
void comms_aux_arm_rx_and_clear_no_comms(void)
{
    dma_aux_mcu_init_rx_transfer((void*)&AUXMCU_SERCOM->USART.DATA.reg, (void*)&aux_mcu_receive_message, sizeof(aux_mcu_receive_message));
    platform_io_clear_no_comms();
}

/*! \fn     comms_aux_mcu_get_temp_tx_message_object_pt(void)
*   \brief  Get a pointer to our temporary tx message object
*/
aux_mcu_message_t* comms_aux_mcu_get_temp_tx_message_object_pt(void)
{
    return &aux_mcu_send_message;
}

/*! \fn     comms_aux_mcu_send_message(aux_mcu_message_t* message)
*   \brief  Send a message to the AUX MCU
*   \param  message         Pointer to the message to send
*   \note   Transfer is done through DMA so data will be accessed after this function returns
*/
void comms_aux_mcu_send_message(aux_mcu_message_t* message)
{    
    /* The function below does wait for a previous transfer to finish */
    dma_aux_mcu_init_tx_transfer((void*)&AUXMCU_SERCOM->USART.DATA.reg, (void*)message, sizeof(aux_mcu_message_t));    
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
    comms_aux_mcu_send_message((void*)&aux_mcu_send_message);
    comms_aux_mcu_wait_for_message_sent();    
}

/*! \fn     comms_aux_mcu_send_receive_ping(void)
*   \brief  Try to ping the aux MCU
*   \return Success or not
*/
RET_TYPE comms_aux_mcu_send_receive_ping(void)
{
    aux_mcu_message_t* temp_rx_message_pt;
    RET_TYPE return_val = RETURN_OK;

    /* Tell the aux MCU to not send us messages */
    platform_io_set_no_comms();
    
    /* Wait for possible previous message to be sent */
    comms_aux_mcu_wait_for_message_sent();
    
    /* Prepare packet */
    memset((void*)&aux_mcu_send_message, 0, sizeof(aux_mcu_send_message));
    aux_mcu_send_message.message_type = AUX_MCU_MSG_TYPE_MAIN_MCU_CMD;
    aux_mcu_send_message.payload_length1 = sizeof(aux_mcu_send_message.main_mcu_command_message.command);
    aux_mcu_send_message.main_mcu_command_message.command = MAIN_MCU_COMMAND_PING;
    comms_aux_mcu_send_message((void*)&aux_mcu_send_message);
    comms_aux_mcu_wait_for_message_sent();
    
    /* Wait for answer: no need to parse answer as filter is done in comms_aux_mcu_active_wait */
    return_val = comms_aux_mcu_active_wait(&temp_rx_message_pt);
    
    /* Rearm receive */
    comms_aux_arm_rx_and_clear_no_comms();
    
    return return_val;
}

/*! \fn     comms_aux_mcu_routine(void)
*   \brief  Routine dealing with aux mcu comms
*/
void comms_aux_mcu_routine(void)
{	
    /* Ongoing RX transfer received bytes */
    uint16_t nb_received_bytes_for_ongoing_transfer = sizeof(aux_mcu_receive_message) - dma_aux_mcu_get_remaining_bytes_for_rx_transfer();
    
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
            }
            else if (aux_mcu_receive_message.rx_payload_valid_flag != 0)
            {
                arm_rx_transfer = TRUE;
                should_deal_with_packet = TRUE;
                payload_length = aux_mcu_receive_message.payload_length2;                
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
        /* Note: there's a case where we don't rearm DMA if the message is valid but payload is too long... was lazy to implement it */
        return;
    }
            
    /* USB / BLE Messages */
    if ((aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_USB) || (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_BLE))
    {
        /* Cast payloads into correct type */
        int16_t hid_reply_payload_length = -1;
                
        /* Clear TX message just in case */
        memset((void*)&aux_mcu_send_message, 0, sizeof(aux_mcu_send_message));
                
        /* Parse message */
        #ifndef DEBUG_USB_COMMANDS_ENABLED
        hid_reply_payload_length = comms_hid_msgs_parse(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), &aux_mcu_send_message.hid_message);
        #else
        if (aux_mcu_receive_message.hid_message.message_type >= HID_MESSAGE_START_CMD_ID_DBG)
        {
            hid_reply_payload_length = comms_hid_msgs_parse_debug(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), &aux_mcu_send_message.hid_message);
        }
        else
        {
            hid_reply_payload_length = comms_hid_msgs_parse(&aux_mcu_receive_message.hid_message, payload_length - sizeof(aux_mcu_receive_message.hid_message.message_type) - sizeof(aux_mcu_receive_message.hid_message.payload_length), &aux_mcu_send_message.hid_message);
        }
        #endif
                
        /* Send reply if needed */
        if (hid_reply_payload_length >= 0)
        {
            /* Set same message type and compute payload size */
            aux_mcu_send_message.message_type = aux_mcu_receive_message.message_type;
            aux_mcu_send_message.payload_length1 = hid_reply_payload_length + sizeof(aux_mcu_receive_message.hid_message.message_type) + sizeof(aux_mcu_receive_message.hid_message.payload_length);
                    
            /* Send message */
            comms_aux_mcu_send_message((void*)&aux_mcu_send_message);
        }
    } 
    else if (aux_mcu_receive_message.message_type == AUX_MCU_MSG_TYPE_BOOTLOADER)
    {
        asm("Nop");
    }   
    else
    {
        asm("Nop");        
    } 
    
    /* If we need to rearm RX */
    if (arm_rx_transfer != FALSE)
    {
        comms_aux_arm_rx_and_clear_no_comms();
    }          
}

/*! \fn     comms_aux_mcu_active_wait(aux_mcu_message_t** rx_message_pt_pt)
*   \brief  Active wait for a message from the aux MCU. 
*   \param  rx_message_pt_pt   Pointer to where to store the pointer to the received message
*   \return OK if a message was received
*   \note   Special care must be taken to discard other message we don't want (either with a please_retry or other mechanisms)
*   \note   DMA RX arm must be called to rearm message receive as a rearm in this code would enable data to be overwritten
*/
RET_TYPE comms_aux_mcu_active_wait(aux_mcu_message_t** rx_message_pt_pt)
{
    /* Bool for the do{} */
    BOOL reloop = FALSE;
    
    /* Arm timer */
    timer_start_timer(TIMER_TIMEOUT_FUNCTS, AUX_MCU_MESSAGE_REPLY_TIMEOUT_MS);
    
    do
    {
        /* Do not reloop by default */
        reloop = FALSE;
        
        /* Wait for complete message to be received */
        while((dma_aux_mcu_check_and_clear_dma_transfer_flag() == FALSE) && (timer_has_timer_expired(TIMER_TIMEOUT_FUNCTS, FALSE) == TIMER_RUNNING));
        
        /* Did the timer expire? */
        if (timer_has_timer_expired(TIMER_TIMEOUT_FUNCTS, FALSE) == TIMER_EXPIRED)
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
            dma_aux_mcu_init_rx_transfer((void*)&AUXMCU_SERCOM->USART.DATA.reg, (void*)&aux_mcu_receive_message, sizeof(aux_mcu_receive_message));
        }        
    }while (reloop != FALSE);
        
    /* Store pointer to message */
    *rx_message_pt_pt = &aux_mcu_receive_message;
        
    /* Return OK */
    return RETURN_OK;
}
