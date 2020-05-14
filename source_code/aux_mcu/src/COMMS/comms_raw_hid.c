/*!  \file     comms_usb.c
*    \brief    USB packet receive and parsing
*    Created:  22/08/2018
*    Author:   Mathieu Stephan
*/
#include <stdarg.h>
#include <string.h>
#include "platform_defines.h"
#include "logic_bluetooth.h"
#include "comms_main_mcu.h"
#include "comms_raw_hid.h"
#include "driver_timer.h"
#include "ble_manager.h"
#include "defines.h"
#include "usb.h"
#include "udc.h"
#include "dma.h"
#include "ctaphid.h"

/* USB comms buffers */
static hid_packet_t raw_hid_recv_buffer[NB_HID_INTERFACES];
static hid_packet_t raw_hid_send_buffer[NB_HID_INTERFACES];
/* Future message to be sent to MCU */
aux_mcu_message_t comms_raw_hid_temp_mcu_message_to_send[NB_HID_INTERFACES];
/* Packet number we're expecting to receive */
uint16_t comms_raw_hid_expected_packet_number[NB_HID_INTERFACES] = {0,0,0};
/* Total number of packets for current message */
uint16_t comms_raw_hid_total_expected_packets[NB_HID_INTERFACES];
/* Index in our temp mcu message to send at which to fill data */
uint16_t comms_raw_hid_temp_mcu_message_fill_index[NB_HID_INTERFACES] = {0,0,0};
/* Expected flip bit state */
BOOL comms_raw_hid_expect_flip_bit_state_set[NB_HID_INTERFACES] = {FALSE, FALSE, FALSE};
/* Set when we received/send a USB message */
volatile BOOL comms_raw_hid_packet_being_sent[NB_HID_INTERFACES] = {FALSE, FALSE, FALSE};
volatile BOOL comms_raw_hid_packet_received[NB_HID_INTERFACES] = {FALSE, FALSE, FALSE};
volatile BOOL comms_raw_hid_packet_receive_length[NB_HID_INTERFACES] = {0, 0, 0};
/* Set when we just got enumerated */
volatile BOOL comms_usb_just_enumerated = FALSE;
/* Set when a USB timeout is detected */
volatile BOOL comms_usb_timeout_detected = FALSE;
/* Device status cache so we don't need to query main mcu */
uint8_t comms_hid_device_status_cache[4];
/* Set when we are enumerated */
BOOL comms_usb_enumerated = FALSE;
/* Set when we received a new device status */
BOOL comms_raw_hid_new_device_status_received = FALSE;


/*! \fn     comms_raw_hid_update_device_status_cache(uint8_t* buffer)
*   \brief  Update device status cache
*   \param  buffer  4 bytes buffer containing device status
*/
void comms_raw_hid_update_device_status_cache(uint8_t* buffer)
{
    memcpy(comms_hid_device_status_cache, buffer, sizeof(comms_hid_device_status_cache));
    comms_raw_hid_new_device_status_received = TRUE;
}

/*! \fn     comms_raw_hid_get_recv_buffer(hid_interface_te hid_interface)
*   \brief  Get the pointer to a receive buffer
*   \param  hid_interface   HID interface
*/
uint8_t* comms_raw_hid_get_recv_buffer(hid_interface_te hid_interface)
{
    return (uint8_t*)&(raw_hid_recv_buffer[hid_interface]);
}

/*! \fn     comms_raw_hid_get_send_buffer(hid_interface_te hid_interface)
*   \brief  Get the pointer to a send buffer
*   \param  hid_interface   HID interface
*   \return Pointer to the send buffer
*/
hid_packet_t* comms_raw_hid_get_send_buffer(hid_interface_te hid_interface)
{
    return &(raw_hid_send_buffer[hid_interface]);
}

/*! \fn     comms_raw_hid_recv_callback(hid_interface_te hid_interface, uint16_t recv_bytes)
*   \brief  Function called when a HID packet is received
*   \param  hid_interface   interface from which we received the packet
*   \param  recv_bytes      Number of received bytes
*/
void comms_raw_hid_recv_callback(hid_interface_te hid_interface, uint16_t recv_bytes)
{
    /* Set number of received bytes */
    comms_raw_hid_packet_receive_length[hid_interface] = recv_bytes;
    
    /* Set flag */
    comms_raw_hid_packet_received[hid_interface] = TRUE;
}

/*! \fn     comms_raw_hid_send_callback(hid_interface_te hid_interface)
*   \brief  Function called when a HID packet is sent
*   \param  hid_interface   interface from which we received the packet
*/
void comms_raw_hid_send_callback(hid_interface_te hid_interface)
{
    /* Set flag */
    comms_raw_hid_packet_being_sent[hid_interface] = FALSE;
}

/*! \fn     comms_raw_hid_arm_packet_receive(hid_interface_te hid_interface)
*   \brief  Arm packet receive
*/
void comms_raw_hid_arm_packet_receive(hid_interface_te hid_interface)
{
    if (hid_interface == USB_INTERFACE)
    {
        usb_recv(USB_RAWHID_TX_ENDPOINT, (uint8_t*)&raw_hid_recv_buffer[hid_interface], sizeof(raw_hid_recv_buffer[0]));
    }
    else if (hid_interface == CTAP_INTERFACE)
    {
        usb_recv(USB_CTAP_TX_ENDPOINT, (uint8_t*)&raw_hid_recv_buffer[hid_interface], sizeof(raw_hid_recv_buffer[0]));
    }
    else
    {
    }
}

/*! \fn     comms_raw_hid_send_packet(hid_interface_te hid_interface, hid_packet_t* packet, BOOL wait_send, uint16_t payload_size)
*   \brief  send raw hid packet
*   \param  hid_interface   HID interface on which to send the packet
*   \param  packet          Packet to send (must be 4 bytes aligned!)
*   \param  wait_send       Set to wait for end of packet transmission
*   \param  payload_size    Payload size
*/
void comms_raw_hid_send_packet(hid_interface_te hid_interface, hid_packet_t* packet, BOOL wait_send, uint16_t payload_size)
{
    /* Wait for possible previous packet to be sent */
    while(comms_raw_hid_packet_being_sent[hid_interface] == TRUE);
    
    /* Reset flag */
    comms_raw_hid_packet_being_sent[hid_interface] = TRUE;
    
    /* Check payload size parameter */
    if (payload_size > sizeof(hid_packet_t))
    {
        payload_size = sizeof(hid_packet_t);
    }
    
    /* Send packet */
    if (hid_interface == USB_INTERFACE)
    {
        usb_send(USB_RAWHID_RX_ENDPOINT, (uint8_t*)packet, payload_size);
    }
    else if (hid_interface == CTAP_INTERFACE)
    {
        //comms_usb_debug_printf("Send CTAP response: IF: %d size: %d\n", CTAP_INTERFACE, payload_size);
        usb_send(USB_CTAP_RX_ENDPOINT, (uint8_t*)packet, payload_size);
    }
    else
    {
        logic_bluetooth_raw_send((uint8_t*)packet, payload_size);
    }
    
    /* If asked, wait */
    if (wait_send != FALSE)
    {
        timer_start_timer(TIMER_BT_TYPING_TIMEOUT, 3000);        
        while(comms_raw_hid_packet_being_sent[hid_interface] == TRUE)
        {
            if (hid_interface == BLE_INTERFACE)
            {
                ble_event_task();
            }
            
            /* Check for BLE timeout */
            if ((hid_interface == BLE_INTERFACE) && (timer_has_timer_expired(TIMER_BT_TYPING_TIMEOUT, FALSE) == TIMER_EXPIRED))
            {
                comms_raw_hid_packet_being_sent[hid_interface] = FALSE;
                return;
            }
            
            /* Check for usb disconnection */
            if (((hid_interface == USB_INTERFACE) || (hid_interface == CTAP_INTERFACE)) && ((usb_get_config() == 0) || (udc_get_nb_ms_before_last_usb_activity() > 100)))
            {
                comms_raw_hid_packet_being_sent[hid_interface] = FALSE;
                return;
            }
        }
    }    
}

/*! \fn     comms_raw_hid_send_hid_message(hid_interface_te hid_interface, aux_mcu_message_t* message)
*   \brief  send HID message to PC
*   \param  hid_interface   interface from which we received the packet
*   \param  message     Message to send
*/
void comms_raw_hid_send_hid_message(hid_interface_te hid_interface, aux_mcu_message_t* message)
{
    uint8_t total_number_of_packets = ((message->payload_length1 + sizeof(raw_hid_send_buffer[0].mtc_hid_packet.payload) - 1)/sizeof(raw_hid_send_buffer[0].mtc_hid_packet.payload))-1;
    uint16_t remaining_payload_to_send = message->payload_length1;
    uint16_t payload_offset = 0;
    uint8_t packet_id = 0;
    
    /* Generate and send packets */
    while(remaining_payload_to_send > 0)
    {
        /* Wait for a possible previous packet to be sent as we do buffer re-use */
        while(comms_raw_hid_packet_being_sent[hid_interface] == TRUE)
        {
            /* Bluetooth busy sending previous packet... */
            if (hid_interface == BLE_INTERFACE)
            {
                ble_event_task();
            }
            
            /* Check for usb disconnection */
            if (((hid_interface == USB_INTERFACE) || (hid_interface == CTAP_INTERFACE)) && ((usb_get_config() == 0) || (udc_get_nb_ms_before_last_usb_activity() > 100)))
            {
                return;
            }
        }
        
        /* Generate packet */
        memset((void*)&raw_hid_send_buffer[hid_interface], 0, sizeof(raw_hid_send_buffer[0]));
        raw_hid_send_buffer[hid_interface].mtc_hid_packet.byte1.total_packets = total_number_of_packets;
        raw_hid_send_buffer[hid_interface].mtc_hid_packet.byte1.packet_id = packet_id;
        
        /* We do not care about the flip bit */
        if (remaining_payload_to_send > sizeof(raw_hid_send_buffer[0].mtc_hid_packet.payload))
        {
            raw_hid_send_buffer[hid_interface].mtc_hid_packet.byte0.payload_len = sizeof(raw_hid_send_buffer[0].mtc_hid_packet.payload);
        }
        else
        {
            raw_hid_send_buffer[hid_interface].mtc_hid_packet.byte0.payload_len = remaining_payload_to_send;            
        }
        
        /* Copy payload */
        memcpy(raw_hid_send_buffer[hid_interface].mtc_hid_packet.payload, &(message->payload[payload_offset]), raw_hid_send_buffer[hid_interface].mtc_hid_packet.byte0.payload_len);
        
        /* 0-fill padding */
        uint16_t padding_length = sizeof(raw_hid_send_buffer[0].mtc_hid_packet.payload) - raw_hid_send_buffer[hid_interface].mtc_hid_packet.byte0.payload_len;
        if (padding_length != 0)
        {
            memset((void*)(&raw_hid_send_buffer[hid_interface].mtc_hid_packet.payload[raw_hid_send_buffer[hid_interface].mtc_hid_packet.byte0.payload_len]), 0x00, padding_length);
        }
        
        /* update local vars */
        remaining_payload_to_send -= raw_hid_send_buffer[hid_interface].mtc_hid_packet.byte0.payload_len;
        payload_offset += raw_hid_send_buffer[hid_interface].mtc_hid_packet.byte0.payload_len;
        packet_id += 1;
        
        /* Send packet: always send 64B due to some strange windows receive trigger thingy */
        //comms_raw_hid_send_packet(&raw_hid_send_buffer, TRUE, sizeof(raw_hid_send_buffer.byte0) + sizeof(raw_hid_send_buffer.byte1) + raw_hid_send_buffer.byte0.payload_len);
        comms_raw_hid_send_packet(hid_interface, &raw_hid_send_buffer[hid_interface], TRUE, USB_RAWHID_RX_SIZE);
    }
}

/*! \fn     comms_usb_is_enumerated(void)
*   \brief  Check if we're enumerated
*   \return The Bool
*/
BOOL comms_usb_is_enumerated(void)
{
    return comms_usb_enumerated;
}

/*! \fn     comms_usb_clear_enumerated(void)
*   \brief  Clear enumerated bool
*/
void comms_usb_clear_enumerated(void)
{
    comms_usb_enumerated = FALSE;
}

/*! \fn     comms_raw_hid_connection_set_callback(hid_interface_te hid_interface)
*   \brief  Called when one of the communication interface has been set
*   \param  hid_interface   interface from which we received the packet
*/
void comms_raw_hid_connection_set_callback(hid_interface_te hid_interface)
{
    if (hid_interface == USB_INTERFACE)
    {
        /* Set enumerated booleans */
        comms_usb_timeout_detected = FALSE;
        comms_usb_just_enumerated = TRUE;
        comms_usb_enumerated = TRUE;
    } 
    else
    {
    }    
    
    /* Reset global vars */
    comms_raw_hid_expect_flip_bit_state_set[hid_interface] = FALSE;
    comms_raw_hid_temp_mcu_message_fill_index[hid_interface] = 0;
    comms_raw_hid_expected_packet_number[hid_interface] = 0;
    comms_raw_hid_packet_being_sent[hid_interface] = FALSE;
} 

/*! \fn     comms_usb_communication_routine(void)
*   \brief  Function called to deal with comms
*   \return What happened
*/
comms_usb_ret_te comms_usb_communication_routine(void)
{
    comms_usb_ret_te ret_val = COMMS_USB_NO_RET;
    
    /* If we just were enumerated, let main MCU know */
    if (comms_usb_just_enumerated != FALSE)
    {
        comms_main_mcu_send_simple_event(AUX_MCU_EVENT_USB_ENUMERATED);
        comms_raw_hid_arm_packet_receive(USB_INTERFACE);
        comms_raw_hid_arm_packet_receive(CTAP_INTERFACE);
        comms_usb_just_enumerated = FALSE;
    }
    
    /* USB time out detected? */
    if ((comms_usb_timeout_detected == FALSE) && (comms_usb_enumerated != FALSE) && (udc_get_nb_ms_before_last_usb_activity() > 65000))
    {
        comms_main_mcu_send_simple_event(AUX_MCU_EVENT_USB_TIMEOUT);
        comms_usb_timeout_detected = TRUE;
    }
    
    /* We received a new device status, push it to the host */
    if (comms_raw_hid_new_device_status_received != FALSE)
    {
        /* Temporary buffer, where we cheat */
        uint32_t shorter_aux_mcu_message[USB_RAWHID_RX_SIZE/sizeof(uint32_t)];
        aux_mcu_message_t* temp_message_pt = (aux_mcu_message_t*)shorter_aux_mcu_message;
        
        /* Prepare message to be sent */
        temp_message_pt->hid_message.message_type = HID_CMD_GET_DEVICE_STATUS;
        temp_message_pt->hid_message.payload_length = sizeof(comms_hid_device_status_cache);
        memcpy(temp_message_pt->hid_message.payload, comms_hid_device_status_cache, sizeof(comms_hid_device_status_cache));
        temp_message_pt->payload_length1 = sizeof(temp_message_pt->hid_message.message_type) + sizeof(temp_message_pt->hid_message.payload_length) + temp_message_pt->hid_message.payload_length;
        
        /* Can send to USB? */
        if (comms_usb_enumerated != FALSE)
        {
            comms_raw_hid_send_hid_message(USB_INTERFACE, temp_message_pt);
        }
        
        /* Can send to bluetooth? */
        if (logic_bluetooth_can_talk_to_host() != FALSE)
        {
            comms_raw_hid_send_hid_message(BLE_INTERFACE, temp_message_pt);
        }
        
        /* Clear Bool */
        comms_raw_hid_new_device_status_received = FALSE;
    }
    
    /* Packet processing logic for all interfaces */
    for (uint16_t hid_interface = 0; hid_interface < NB_HID_INTERFACES; hid_interface++)
    {
        /* Did we receive a packet? */
        if (comms_raw_hid_packet_received[hid_interface] != FALSE)
        {
            /* Reset flag */
            comms_raw_hid_packet_received[hid_interface] = FALSE;

            if (hid_interface == CTAP_INTERFACE)
            {
                ctaphid_handle_packet(raw_hid_recv_buffer[hid_interface].raw_packet_uint32);
                comms_raw_hid_arm_packet_receive(hid_interface);
                return ret_val;
            }

            /* Special case: first two bytes set to 0xFF 0xFF, reset flip bit */
            uint8_t* usb_recast = (uint8_t*)&raw_hid_recv_buffer[hid_interface];
            if ((usb_recast[0] == 0xFF) && (usb_recast[1] == 0xFF))
            {
                comms_raw_hid_expect_flip_bit_state_set[hid_interface] = FALSE;
                comms_raw_hid_temp_mcu_message_fill_index[hid_interface] = 0;
                comms_raw_hid_expected_packet_number[hid_interface] = 0;
                comms_raw_hid_arm_packet_receive(hid_interface);
                return ret_val;
            }
            
            /* Check for bit flip state: if it doesn't match, reset fill indexes */
            if (((comms_raw_hid_expect_flip_bit_state_set[hid_interface] != FALSE) && (raw_hid_recv_buffer[hid_interface].mtc_hid_packet.byte0.flip_bit == 0)) || ((comms_raw_hid_expect_flip_bit_state_set[hid_interface] == FALSE) && (raw_hid_recv_buffer[hid_interface].mtc_hid_packet.byte0.flip_bit != 0)))
            {
                comms_raw_hid_temp_mcu_message_fill_index[hid_interface] = 0;
                comms_raw_hid_expected_packet_number[hid_interface] = 0;
                comms_raw_hid_arm_packet_receive(hid_interface);
                return ret_val;
            }
            
            /* Check for expected packet number */
            if ((comms_raw_hid_expected_packet_number[hid_interface] != 0) && (raw_hid_recv_buffer[hid_interface].mtc_hid_packet.byte1.packet_id != comms_raw_hid_expected_packet_number[hid_interface]))
            {
                comms_raw_hid_temp_mcu_message_fill_index[hid_interface] = 0;
                comms_raw_hid_expected_packet_number[hid_interface] = 0;
                comms_raw_hid_arm_packet_receive(hid_interface);
                return ret_val;
            }
            
            /* If first packet, store total number of packets for this hid message */
            if (raw_hid_recv_buffer[hid_interface].mtc_hid_packet.byte1.packet_id == 0)
            {
                comms_raw_hid_total_expected_packets[hid_interface] = raw_hid_recv_buffer[hid_interface].mtc_hid_packet.byte1.total_packets;
                
                /* Reset index to fill temp message payload */
                comms_raw_hid_temp_mcu_message_fill_index[hid_interface] = 0;
                
                /* Prepare future packet to send to main MCU */
                memset((void*)&comms_raw_hid_temp_mcu_message_to_send[hid_interface], 0, sizeof(comms_raw_hid_temp_mcu_message_to_send[0]));
                uint16_t msg_type_lut[NB_HID_INTERFACES] = {AUX_MCU_MSG_TYPE_USB, AUX_MCU_MSG_TYPE_BLE};
                comms_raw_hid_temp_mcu_message_to_send[hid_interface].message_type = msg_type_lut[hid_interface];
            }
            
            /* Check for overflow tentative */
            if ((size_t)(comms_raw_hid_temp_mcu_message_fill_index[hid_interface] + raw_hid_recv_buffer[hid_interface].mtc_hid_packet.byte0.payload_len) > sizeof(comms_raw_hid_temp_mcu_message_to_send[0].payload))
            {
                comms_raw_hid_temp_mcu_message_fill_index[hid_interface] = 0;
                comms_raw_hid_expected_packet_number[hid_interface] = 0;
                comms_raw_hid_arm_packet_receive(hid_interface);
                return ret_val;
            }
            
            /* Fill temp mcu message payload */
            memcpy((void*)&comms_raw_hid_temp_mcu_message_to_send[hid_interface].payload[comms_raw_hid_temp_mcu_message_fill_index[hid_interface]], (void*)raw_hid_recv_buffer[hid_interface].mtc_hid_packet.payload, raw_hid_recv_buffer[hid_interface].mtc_hid_packet.byte0.payload_len);
            comms_raw_hid_temp_mcu_message_fill_index[hid_interface] += raw_hid_recv_buffer[hid_interface].mtc_hid_packet.byte0.payload_len;
            comms_raw_hid_expected_packet_number[hid_interface]++;
            
            /* Check for last message */
            if (raw_hid_recv_buffer[hid_interface].mtc_hid_packet.byte1.packet_id == comms_raw_hid_total_expected_packets[hid_interface])
            {
                /* Switch flip bit */
                if (comms_raw_hid_expect_flip_bit_state_set[hid_interface] == FALSE)
                {
                    comms_raw_hid_expect_flip_bit_state_set[hid_interface] = TRUE;
                }
                else
                {
                    comms_raw_hid_expect_flip_bit_state_set[hid_interface] = FALSE;
                }
                
                /* If ack is requested from host */
                if (raw_hid_recv_buffer[hid_interface].mtc_hid_packet.byte0.ack_flag_or_req != 0)
                {
                    /* Send the same message */
                    memcpy((void*)&raw_hid_send_buffer[hid_interface], (void*)&raw_hid_recv_buffer[hid_interface], sizeof(raw_hid_send_buffer[0]));
                    comms_raw_hid_send_packet(hid_interface, &raw_hid_send_buffer[hid_interface], TRUE, comms_raw_hid_packet_receive_length[hid_interface]);
                }
                
                /* Prepare and send message to main MCU */
                comms_raw_hid_temp_mcu_message_to_send[hid_interface].payload_length1 = comms_raw_hid_temp_mcu_message_fill_index[hid_interface];
                
                /* Check for special case were the device status is requested: send local cache instead */
                if (comms_raw_hid_temp_mcu_message_to_send[hid_interface].hid_message.message_type == HID_CMD_GET_DEVICE_STATUS)
                {
                    comms_raw_hid_temp_mcu_message_to_send[hid_interface].hid_message.payload_length = sizeof(comms_hid_device_status_cache);
                    memcpy(comms_raw_hid_temp_mcu_message_to_send[hid_interface].hid_message.payload, comms_hid_device_status_cache, sizeof(comms_hid_device_status_cache));
                    comms_raw_hid_temp_mcu_message_to_send[hid_interface].payload_length1 = sizeof(comms_raw_hid_temp_mcu_message_to_send[hid_interface].hid_message.message_type) + sizeof(comms_raw_hid_temp_mcu_message_to_send[hid_interface].hid_message.payload_length) + comms_raw_hid_temp_mcu_message_to_send[hid_interface].hid_message.payload_length;
                    comms_raw_hid_send_hid_message(hid_interface, &comms_raw_hid_temp_mcu_message_to_send[hid_interface]);
                } 
                else
                {
                    comms_main_mcu_send_message(&comms_raw_hid_temp_mcu_message_to_send[hid_interface], (uint16_t)sizeof(comms_raw_hid_temp_mcu_message_to_send[0]));
                }                
                
                /* Rearm hid interface */
                comms_raw_hid_arm_packet_receive(hid_interface);
                
                /* Reset vars */
                comms_raw_hid_temp_mcu_message_fill_index[hid_interface] = 0;
                comms_raw_hid_expected_packet_number[hid_interface] = 0;
            }
            else
            {
                /* here we should implement in the future transfers in multiple chunks */
                comms_raw_hid_arm_packet_receive(hid_interface);
            }
        }
    }
    
    return ret_val;
}

/*! \fn     comms_usb_debug_printf(const char *fmt, ...) 
*   \brief  Output debug string to USB
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
void comms_usb_debug_printf(const char *fmt, ...) 
{    
    char buf[128];    
    va_list ap;    
    va_start(ap, fmt);

    /* Print into our temporary buffer */
    int16_t hypothetical_nb_chars = vsnprintf(buf, sizeof(buf), fmt, ap);
    
    /* No error? */
    if ((hypothetical_nb_chars > 0) && (comms_usb_enumerated != FALSE))
    {
        /* Compute number of chars printed to our buffer */
        uint16_t actual_printed_chars = (uint16_t)hypothetical_nb_chars < sizeof(buf)-1? (uint16_t)hypothetical_nb_chars : sizeof(buf)-1;
        
        /* Use message to send to aux mcu as temporary buffer */
        dma_wait_for_main_mcu_packet_sent();
        memset((void*)&comms_raw_hid_temp_mcu_message_to_send[USB_INTERFACE], 0, sizeof(comms_raw_hid_temp_mcu_message_to_send[USB_INTERFACE]));
        comms_raw_hid_temp_mcu_message_to_send[USB_INTERFACE].hid_message.message_type = HID_CMD_ID_DEBUG_MSG;
        comms_raw_hid_temp_mcu_message_to_send[USB_INTERFACE].hid_message.payload_length = actual_printed_chars*2 + 2;
        comms_raw_hid_temp_mcu_message_to_send[USB_INTERFACE].payload_length1 = comms_raw_hid_temp_mcu_message_to_send[USB_INTERFACE].hid_message.payload_length + sizeof(comms_raw_hid_temp_mcu_message_to_send[USB_INTERFACE].hid_message.payload_length) + sizeof(comms_raw_hid_temp_mcu_message_to_send[USB_INTERFACE].hid_message.message_type);
        
        /* Copy to message payload */
        for (uint16_t i = 0; i < actual_printed_chars; i++)
        {
            comms_raw_hid_temp_mcu_message_to_send[USB_INTERFACE].hid_message.payload_as_uint16[i] = buf[i];
        }
        
        /* Send message */
        comms_raw_hid_send_hid_message(USB_INTERFACE, &comms_raw_hid_temp_mcu_message_to_send[USB_INTERFACE]);
    }
    va_end(ap);
}
#pragma GCC diagnostic pop
