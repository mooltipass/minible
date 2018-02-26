/*
 * \file    usbhid.h
 * \author  MBorregoTrujillo
 * \date    22-February-2018
 * \brief   USB HID Protocol
 */

#ifndef USBHID_H_
#define USBHID_H_

#include "conf_usb.h"
/**
 * \fn      USBHID_init
 * \brief   Initialize USBHID internal variables
 */
void USBHID_init(void);

/**
 * \fn          USBHID_usb_callback
 * \brief       Callback function which receives raw hid packet to be processed
 *              following minible USB HID Protocol. The callback is called from
 *              udi_hid_generic_report_out_received function and configured in
 *              conf_usb.h file.
 *
 * \param data  Pointer to raw hid packet received from USB
 */
void USBHID_usb_callback(uint8_t *data);


#endif /* USBHID_H_ */
