/*
 * \file    usbhid.h
 * \author  MBorregoTrujillo
 * \date    22-February-2018
 * \brief   USB HID Protocol
 */

#ifndef USBHID_H_
#define USBHID_H_

#include "conf_usb.h"

/** USBHID Constants **/
#define USBHID_MAX_TOTAL_PKTS   (16U)
#define USBHID_MAX_PAYLOAD      (62U)
#define USBHID_MAX_MSG_SIZE     (USBHID_MAX_PAYLOAD*USBHID_MAX_TOTAL_PKTS)
#define USBHID_MSG_HEADER_SIZE  (4U)
#define USBHID_PKT_HEADER_SIZE  (2U)

void usbhid_init(void);
void usbhid_usb_callback(uint8_t *data);
void usbhid_send_to_usb(uint8_t* buff, uint16_t buff_len);

#endif /* USBHID_H_ */
