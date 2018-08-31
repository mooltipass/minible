/*
 * comms_usb.h
 *
 * Created: 23/08/2018 21:52:22
 *  Author: limpkin
 */ 


#ifndef COMMS_USB_H_
#define COMMS_USB_H_

#include "defines.h"
#include "comms_main_mcu.h"

/* Type defs */
typedef struct
{
    struct
    {
        uint8_t  payload_len:6;
        uint8_t  ack_flag_or_req:1;
        uint8_t  flip_bit:1;
    } byte0;
    struct
    {
        uint8_t  total_packets:4;
        uint8_t  packet_id:4;
    } byte1;
    uint8_t payload[62];    
} hid_packet_t;

/* Prototypes */
void comms_usb_send_raw_hid_packet(hid_packet_t* packet, BOOL wait_send, uint16_t payload_size);
void comms_usb_send_hid_message(aux_mcu_message_t* message);
void comms_usb_raw_hid_recv_callback(uint16_t recv_bytes);
void comms_usb_debug_printf(const char *fmt, ...);
void comms_usb_configuration_callback(int config);
void comms_usb_raw_hid_send_callback(void);
void comms_usb_communication_routine(void);
void comms_usb_arm_packet_receive(void);


#endif /* COMMS_USB_H_ */