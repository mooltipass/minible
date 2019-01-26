/*!  \file     comms_usb.c
*    \brief    USB packet receive and parsing
*    Created:  22/08/2018
*    Author:   Mathieu Stephan
*/
#include <stdarg.h>
#include <string.h>
#include "platform_defines.h"
#include "comms_main_mcu.h"
#include "comms_usb.h"
#include "defines.h"
#include "usb.h"
#include "dma.h"

/* USB comms buffers */
static __attribute__((aligned(4))) hid_packet_t raw_hid_recv_buffer;
static __attribute__((aligned(4))) hid_packet_t raw_hid_send_buffer;
/* Future message to be sent to MCU */
aux_mcu_message_t comms_usb_temp_mcu_message_to_send;
/* Packet number we're expecting to receive */
uint16_t comms_usb_expected_packet_number = 0;
/* Total number of packets for current message */
uint16_t comms_usb_total_expected_packets;
/* Index in our temp mcu message to send at which to fill data */
uint16_t comms_usb_temp_mcu_message_fill_index = 0;
/* Expected flip bit state */
BOOL comms_usb_expect_flip_bit_state_set = FALSE;
/* Set when we received/send a USB message */
volatile BOOL comms_usb_raw_hid_packet_received = FALSE;
volatile BOOL comms_usb_raw_hid_packet_receive_length = 0;
volatile BOOL comms_usb_raw_hid_packet_being_sent = FALSE;

/* Debug vars */
uint16_t dbg_mcu_hid_msg_sent = 0;
uint16_t dbg_mcu_hid_msg_recv = 0;


/*! \fn     comms_usb_raw_hid_recv_callback(uint16_t recv_bytes)
*   \brief  Function called when a HID packet is received
*/
void comms_usb_raw_hid_recv_callback(uint16_t recv_bytes)
{
    /* Set number of received bytes */
    comms_usb_raw_hid_packet_receive_length = recv_bytes;
    
    /* Set flag */
    comms_usb_raw_hid_packet_received = TRUE;
}

/*! \fn     comms_usb_raw_hid_send_callback(void)
*   \brief  Function called when a HID packet is sent
*/
void comms_usb_raw_hid_send_callback(void)
{
    /* Set flag */
    comms_usb_raw_hid_packet_being_sent = FALSE;
}

/*! \fn     comms_usb_arm_packet_receive(void)
*   \brief  Arm packet receive
*/
void comms_usb_arm_packet_receive(void)
{
    usb_recv(USB_RAWHID_TX_ENDPOINT, (uint8_t*)&raw_hid_recv_buffer, sizeof(raw_hid_recv_buffer));
}

/*! \fn     comms_usb_send_raw_hid_packet(hid_packet_t* packet, BOOL wait_send, uint16_t payload_size)
*   \brief  send raw hid packet
*   \param  packet          Packet to send (must be 4 bytes aligned!)
*   \param  wait_send       Set to wait for end of packet transmission
*   \param  payload_size    Payload size
*/
void comms_usb_send_raw_hid_packet(hid_packet_t* packet, BOOL wait_send, uint16_t payload_size)
{
    /* Wait for possible previous packet to be sent */
    while(comms_usb_raw_hid_packet_being_sent == TRUE);
    
    /* Reset flag */
    comms_usb_raw_hid_packet_being_sent = TRUE;
    
    /* Check payload size parameter */
    if (payload_size > sizeof(hid_packet_t))
    {
        payload_size = sizeof(hid_packet_t);
    }
    
    /* Send packet */
    usb_send(USB_RAWHID_RX_ENDPOINT, (uint8_t*)packet, payload_size);
    
    /* If asked, wait */
    if (wait_send != FALSE)
    {
        while(comms_usb_raw_hid_packet_being_sent == TRUE);
    }    
}

/*! \fn     comms_usb_send_hid_message(aux_mcu_message_t* message)
*   \brief  send HID message to PC
*   \param  message     Message to send
*/
void comms_usb_send_hid_message(aux_mcu_message_t* message)
{
    dbg_mcu_hid_msg_recv++;
    uint8_t total_number_of_packets = ((message->payload_length1 + sizeof(raw_hid_send_buffer.payload) - 1)/sizeof(raw_hid_send_buffer.payload))-1;
    uint16_t remaining_payload_to_send = message->payload_length1;
    uint16_t payload_offset = 0;
    uint8_t packet_id = 0;
    
    /* Generate and send packets */
    while(remaining_payload_to_send > 0)
    {
        /* Wait for a possible previous packet to be sent as we do buffer re-use */
        while(comms_usb_raw_hid_packet_being_sent == TRUE);
        
        /* Generate packet */
        memset((void*)&raw_hid_send_buffer, 0, sizeof(raw_hid_send_buffer));
        raw_hid_send_buffer.byte1.total_packets = total_number_of_packets;
        raw_hid_send_buffer.byte1.packet_id = packet_id;
        
        /* We do not care about the flip bit */
        if (remaining_payload_to_send > sizeof(raw_hid_send_buffer.payload))
        {
            raw_hid_send_buffer.byte0.payload_len = sizeof(raw_hid_send_buffer.payload);
        }
        else
        {
            raw_hid_send_buffer.byte0.payload_len = remaining_payload_to_send;            
        }
        
        /* Copy payload */
        memcpy(raw_hid_send_buffer.payload, &(message->payload[payload_offset]), raw_hid_send_buffer.byte0.payload_len);
        
        /* 0-fill padding */
        uint16_t padding_length = sizeof(raw_hid_send_buffer.payload) - raw_hid_send_buffer.byte0.payload_len;
        if (padding_length != 0)
        {
            memset((void*)(&raw_hid_send_buffer.payload[raw_hid_send_buffer.byte0.payload_len]), 0x00, padding_length);
        }
        
        /* update local vars */
        remaining_payload_to_send -= raw_hid_send_buffer.byte0.payload_len;
        payload_offset += raw_hid_send_buffer.byte0.payload_len;
        packet_id += 1;
        
        /* Send packet: always send 64B due to some strange windows receive trigger thingy */
        //comms_usb_send_raw_hid_packet(&raw_hid_send_buffer, TRUE, sizeof(raw_hid_send_buffer.byte0) + sizeof(raw_hid_send_buffer.byte1) + raw_hid_send_buffer.byte0.payload_len);
        comms_usb_send_raw_hid_packet(&raw_hid_send_buffer, TRUE, USB_RAWHID_RX_SIZE);
    }
}

/*! \fn     comms_usb_configuration_callback(int config)
*   \brief  Called when device is configured, initialize USB comms
*/
void comms_usb_configuration_callback(int config)
{
    /* Unused */
    (void)config;
    
    /* Reset global vars */
    comms_usb_expect_flip_bit_state_set = FALSE;
    comms_usb_temp_mcu_message_fill_index = 0;
    comms_usb_expected_packet_number = 0;
    
    /* Start receiving raw HID packets */
    comms_usb_arm_packet_receive();
} 

/*! \fn     comms_usb_communication_routine(void)
*   \brief  Function called to deal with comms
*/
void comms_usb_communication_routine(void)
{
    /* Did we receive a packet? */
    if (comms_usb_raw_hid_packet_received != FALSE)
    {
        /* Reset flag */
        comms_usb_raw_hid_packet_received = FALSE;
        
        /* Special case: first two bytes set to 0xFF 0xFF, reset flip bit */
        uint8_t* usb_recast = (uint8_t*)&raw_hid_recv_buffer;
        if ((usb_recast[0] == 0xFF) && usb_recast[1] == 0xFF)
        {
            comms_usb_expect_flip_bit_state_set = FALSE;
        }        
        
        /* Check for bit flip state: if it doesn't match, reset fill indexes */
        if (((comms_usb_expect_flip_bit_state_set != FALSE) && (raw_hid_recv_buffer.byte0.flip_bit == 0)) || ((comms_usb_expect_flip_bit_state_set == FALSE) && (raw_hid_recv_buffer.byte0.flip_bit != 0)))
        {
            comms_usb_temp_mcu_message_fill_index = 0;
            comms_usb_expected_packet_number = 0;
            comms_usb_arm_packet_receive();
            return;
        }
        
        /* Check for expected packet number */
        if ((comms_usb_expected_packet_number != 0) && (raw_hid_recv_buffer.byte1.packet_id != comms_usb_expected_packet_number))
        {
            comms_usb_temp_mcu_message_fill_index = 0;
            comms_usb_expected_packet_number = 0;
            comms_usb_arm_packet_receive();
            return;            
        }
        
        /* If first packet, store total number of packets for this hid message */
        if (raw_hid_recv_buffer.byte1.packet_id == 0)
        {
            comms_usb_total_expected_packets = raw_hid_recv_buffer.byte1.total_packets;
            
            /* Reset index to fill temp message payload */
            comms_usb_temp_mcu_message_fill_index = 0;
            
            /* Prepare future packet to send to main MCU */
            memset((void*)&comms_usb_temp_mcu_message_to_send, 0, sizeof(comms_usb_temp_mcu_message_to_send));
            comms_usb_temp_mcu_message_to_send.message_type = AUX_MCU_MSG_TYPE_USB;
        }
        
        /* Check for overflow tentative */
        if ((size_t)(comms_usb_temp_mcu_message_fill_index + raw_hid_recv_buffer.byte0.payload_len) > sizeof(comms_usb_temp_mcu_message_to_send.payload))
        {
            comms_usb_temp_mcu_message_fill_index = 0;
            comms_usb_expected_packet_number = 0;
            comms_usb_arm_packet_receive();
            return;
        }
        
        /* Fill temp mcu message payload */
        memcpy((void*)&comms_usb_temp_mcu_message_to_send.payload[comms_usb_temp_mcu_message_fill_index], (void*)raw_hid_recv_buffer.payload, raw_hid_recv_buffer.byte0.payload_len);
        comms_usb_temp_mcu_message_fill_index += raw_hid_recv_buffer.byte0.payload_len;
        comms_usb_expected_packet_number++;
        
        /* Check for last message */
        if (raw_hid_recv_buffer.byte1.packet_id == comms_usb_total_expected_packets)
        {
            /* Switch flip bit */
            if (comms_usb_expect_flip_bit_state_set == FALSE)
            {
                comms_usb_expect_flip_bit_state_set = TRUE;
            } 
            else
            {
                comms_usb_expect_flip_bit_state_set = FALSE;
            }
            
            /* If ack is requested from host */
            if (raw_hid_recv_buffer.byte0.ack_flag_or_req != 0)
            {
                /* Send the same message */
                memcpy((void*)&raw_hid_send_buffer, (void*)&raw_hid_recv_buffer, sizeof(raw_hid_send_buffer));
                comms_usb_send_raw_hid_packet(&raw_hid_send_buffer, TRUE, comms_usb_raw_hid_packet_receive_length);
            }
            
            /* Prepare and send message to main MCU */
            comms_usb_temp_mcu_message_to_send.payload_length1 = comms_usb_temp_mcu_message_fill_index;
            comms_main_mcu_send_message(&comms_usb_temp_mcu_message_to_send, (uint16_t)sizeof(comms_usb_temp_mcu_message_to_send));
            comms_usb_arm_packet_receive();
            dbg_mcu_hid_msg_sent++;
            
            /* Reset vars */            
            comms_usb_temp_mcu_message_fill_index = 0;
            comms_usb_expected_packet_number = 0;
        }
        else
        {
            /* here we should implement in the future transfers in multiple chunks */
            comms_usb_arm_packet_receive();
        }
    }
}

/*! \fn     comms_usb_debug_printf(const char *fmt, ...) 
*   \brief  Output debug string to USB
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
void comms_usb_debug_printf(const char *fmt, ...) 
{
    char buf[64];    
    va_list ap;    
    va_start(ap, fmt);

    /* Print into our temporary buffer */
    int16_t hypothetical_nb_chars = vsnprintf(buf, sizeof(buf), fmt, ap);
    
    /* No error? */
    if (hypothetical_nb_chars > 0)
    {
        /* Compute number of chars printed to our buffer */
        uint16_t actual_printed_chars = (uint16_t)hypothetical_nb_chars < sizeof(buf)-1? (uint16_t)hypothetical_nb_chars : sizeof(buf)-1;
        
        /* Use message to send to aux mcu as temporary buffer */
        dma_wait_for_main_mcu_packet_sent();
        memset((void*)&comms_usb_temp_mcu_message_to_send, 0, sizeof(comms_usb_temp_mcu_message_to_send));
        comms_usb_temp_mcu_message_to_send.hid_message.message_type = HID_CMD_ID_DEBUG_MSG;
        comms_usb_temp_mcu_message_to_send.hid_message.payload_length = actual_printed_chars*2 + 2;
        comms_usb_temp_mcu_message_to_send.payload_length1 = comms_usb_temp_mcu_message_to_send.hid_message.payload_length + sizeof(comms_usb_temp_mcu_message_to_send.hid_message.payload_length) + sizeof(comms_usb_temp_mcu_message_to_send.hid_message.message_type);
        
        /* Copy to message payload */
        for (uint16_t i = 0; i < actual_printed_chars; i++)
        {
            comms_usb_temp_mcu_message_to_send.hid_message.payload_as_uint16[i] = buf[i];
        }
        
        /* Send message */
        comms_usb_send_hid_message(&comms_usb_temp_mcu_message_to_send);
    }
    va_end(ap);
}
#pragma GCC diagnostic pop