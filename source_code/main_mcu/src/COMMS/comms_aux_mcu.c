/*!  \file     comms_aux_mcu.c
*    \brief    Communications with aux MCU
*    Created:  03/03/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "comms_hid_msgs.h"
#include "comms_aux_mcu.h"
#include "defines.h"
#include "dma.h"
aux_mcu_message_t aux_mcu_message_buffer1;
aux_mcu_message_t aux_mcu_message_buffer2;
BOOL aux_mcu_msg_rcv_on_buffer1 = TRUE;


/*! \fn     comms_aux_init(void)
*   \brief  Init communications with aux MCU
*/
void comms_aux_init(void)
{
    dma_aux_mcu_init_rx_transfer((void*)&AUXMCU_SERCOM->USART.DATA.reg, (void*)&aux_mcu_message_buffer1, sizeof(aux_mcu_message_buffer1));
}

/*! \fn     comms_aux_mcu_routine(void)
*   \brief  Routine dealing with aux mcu comms
*/
void comms_aux_mcu_routine(void)
{
    /* Pointer to the currently received message and possible packet to fill as an answer */
    aux_mcu_message_t* rcv_msg_pointer = &aux_mcu_message_buffer2;
    aux_mcu_message_t* tosend_msg_pointer = &aux_mcu_message_buffer1;
    if (aux_mcu_msg_rcv_on_buffer1 != FALSE)
    {
        rcv_msg_pointer = &aux_mcu_message_buffer1;
        tosend_msg_pointer = &aux_mcu_message_buffer2;
    }
	
    /* Did we receive a message? */
    if (dma_aux_mcu_check_and_clear_dma_transfer_flag() != FALSE)
    {
        if ((rcv_msg_pointer->message_type == AUX_MCU_MSG_TYPE_USB) || (rcv_msg_pointer->message_type == AUX_MCU_MSG_TYPE_BLE))
        {
            comms_hid_msgs_parse(rcv_msg_pointer->payload, rcv_msg_pointer->payload_length, tosend_msg_pointer->payload);
        }
    }
}
