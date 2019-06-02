/*
 * Copyright (c) 2016, Alex Taradov <alex@taradov.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/*- Includes ----------------------------------------------------------------*/
#include <stdalign.h>
#include "usb.h"
#include "usb_descriptors.h"
#include "platform_defines.h"

/*- Variables ---------------------------------------------------------------*/
alignas(4) usb_device_descriptor_t usb_device_descriptor =
{
  .bLength            = sizeof(usb_device_descriptor_t),
  .bDescriptorType    = USB_DEVICE_DESCRIPTOR,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = 0x00,
  .bDeviceSubClass    = 0x00,
  .bDeviceProtocol    = 0x00,
  .bMaxPacketSize0    = 64,
  .idVendor           = USB_VENDOR_ID,
  .idProduct          = USB_PRODUCT_ID,
  .bcdDevice          = 0x0100,
  .iManufacturer      = USB_STR_MANUFACTURER,
  .iProduct           = USB_STR_PRODUCT,
  .iSerialNumber      = USB_STR_ZERO,
  .bNumConfigurations = 1,
};

alignas(4) usb_configuration_hierarchy_t usb_configuration_hierarchy =
{
  .configuration =
  {
    .bLength             = sizeof(usb_configuration_descriptor_t),
    .bDescriptorType     = USB_CONFIGURATION_DESCRIPTOR,
    .wTotalLength        = sizeof(usb_configuration_hierarchy_t),
    .bNumInterfaces      = 2,
    .bConfigurationValue = 1,
    .iConfiguration      = USB_STR_ZERO,
    .bmAttributes        = 0x80,
    .bMaxPower           = 50,  // not technically ok...
  },

  .raw_interface =
  {
    .bLength             = sizeof(usb_interface_descriptor_t),
    .bDescriptorType     = USB_INTERFACE_DESCRIPTOR,
    .bInterfaceNumber    = USB_RAWHID_INTERFACE,
    .bAlternateSetting   = 0,
    .bNumEndpoints       = 2,
    .bInterfaceClass     = 0x03,
    .bInterfaceSubClass  = 0x00,
    .bInterfaceProtocol  = 0x00,
    .iInterface          = USB_STR_RAW_INTERFACE,
  },

  .raw_hid =
  {
    .bLength             = sizeof(usb_hid_descriptor_t),
    .bDescriptorType     = USB_HID_DESCRIPTOR,
    .bcdHID              = 0x0111,
    .bCountryCode        = 0,
    .bNumDescriptors     = 1,
    .bDescriptorType1    = USB_HID_REPORT_DESCRIPTOR,
    .wDescriptorLength   = sizeof(usb_hid_report_descriptor),
  },

  .raw_ep_in =
  {
    .bLength             = sizeof(usb_endpoint_descriptor_t),
    .bDescriptorType     = USB_ENDPOINT_DESCRIPTOR,
    .bEndpointAddress    = USB_IN_ENDPOINT | USB_RAWHID_RX_ENDPOINT,
    .bmAttributes        = USB_INTERRUPT_ENDPOINT,
    .wMaxPacketSize      = 64,
    .bInterval           = 1,
  },

  .raw_ep_out =
  {
    .bLength             = sizeof(usb_endpoint_descriptor_t),
    .bDescriptorType     = USB_ENDPOINT_DESCRIPTOR,
    .bEndpointAddress    = USB_OUT_ENDPOINT | USB_RAWHID_TX_ENDPOINT,
    .bmAttributes        = USB_INTERRUPT_ENDPOINT,
    .wMaxPacketSize      = 64,
    .bInterval           = 1,
  },

  .keyb_interface =
  {
    .bLength             = sizeof(usb_interface_descriptor_t),
    .bDescriptorType     = USB_INTERFACE_DESCRIPTOR,
    .bInterfaceNumber    = USB_KEYBOARD_INTERFACE,
    .bAlternateSetting   = 0,
    .bNumEndpoints       = 1,
    .bInterfaceClass     = 0x03,
    .bInterfaceSubClass  = 0x01,
    .bInterfaceProtocol  = 0x01,
    .iInterface          = USB_STR_KEYB_INTERFACE,
  },

  .keyb_hid =
  {
    .bLength             = sizeof(usb_hid_descriptor_t),
    .bDescriptorType     = USB_HID_DESCRIPTOR,
    .bcdHID              = 0x0111,
    .bCountryCode        = 0,
    .bNumDescriptors     = 1,
    .bDescriptorType1    = USB_HID_REPORT_DESCRIPTOR,
    .wDescriptorLength   = sizeof(keyboard_hid_report_desc),
  },

  .keyb_ep_in =
  {
    .bLength             = sizeof(usb_endpoint_descriptor_t),
    .bDescriptorType     = USB_ENDPOINT_DESCRIPTOR,
    .bEndpointAddress    = USB_IN_ENDPOINT | USB_KEYBOARD_ENDPOINT,
    .bmAttributes        = USB_INTERRUPT_ENDPOINT,
    .wMaxPacketSize      = USB_KEYBOARD_SIZE,
    .bInterval           = 1,
  },
};

alignas(4) uint8_t usb_hid_report_descriptor[28] =
{
    0x06, USB_RAWHID_USAGE_PAGE & 0xFF, (USB_RAWHID_USAGE_PAGE >> 8) & 0xFF, 
    0x0A, USB_RAWHID_USAGE & 0xFF, (USB_RAWHID_USAGE >> 8) & 0xFF,
    0xA1, 0x01,                         // Collection 0x01
    0x75, 0x08,                         // report size = 8 bits
    0x15, 0x00,                         // logical minimum = 0
    0x26, 0xFF, 0x00,                   // logical maximum = 255
    0x95, USB_RAWHID_TX_SIZE,           // report count
    0x09, 0x01,                         // usage
    0x81, 0x02,                         // Input (array)
    0x95, USB_RAWHID_RX_SIZE,           // report count
    0x09, 0x02,                         // usage
    0x91, 0x02,                         // Output (array)
    0xC0                                // end collection
};

alignas(4) uint8_t keyboard_hid_report_desc[63] =
{
    0x05, 0x01,                         // Usage Page (Generic Desktop),
    0x09, 0x06,                         // Usage (Keyboard),
    0xA1, 0x01,                         // Collection (Application),
    0x75, 0x01,                         //   Report Size (1),
    0x95, 0x08,                         //   Report Count (8),
    0x05, 0x07,                         //   Usage Page (Key Codes),
    0x19, 0xE0,                         //   Usage Minimum (224),
    0x29, 0xE7,                         //   Usage Maximum (231),
    0x15, 0x00,                         //   Logical Minimum (0),
    0x25, 0x01,                         //   Logical Maximum (1),
    0x81, 0x02,                         //   Input (Data, Variable, Absolute), ;Modifier byte
    0x95, 0x01,                         //   Report Count (1),
    0x75, 0x08,                         //   Report Size (8),
    0x81, 0x03,                         //   Input (Constant),                 ;Reserved byte
    0x95, 0x05,                         //   Report Count (5),
    0x75, 0x01,                         //   Report Size (1),
    0x05, 0x08,                         //   Usage Page (LEDs),
    0x19, 0x01,                         //   Usage Minimum (1),
    0x29, 0x05,                         //   Usage Maximum (5),
    0x91, 0x02,                         //   Output (Data, Variable, Absolute), ;LED report
    0x95, 0x01,                         //   Report Count (1),
    0x75, 0x03,                         //   Report Size (3),
    0x91, 0x03,                         //   Output (Constant),                 ;LED report padding
    0x95, 0x06,                         //   Report Count (6),
    0x75, 0x08,                         //   Report Size (8),
    0x15, 0x00,                         //   Logical Minimum (0),
    0x25, 0xff,                         //   Logical Maximum(255), was 0x68 (104) before
    0x05, 0x07,                         //   Usage Page (Key Codes),
    0x19, 0x00,                         //   Usage Minimum (0),
    0x29, 0xe7,                         //   Usage Maximum (231), was 0x68 (104) before
    0x81, 0x00,                         //   Input (Data, Array),
    0xc0                                // End Collection
};

alignas(4) usb_string_descriptor_zero_t usb_string_descriptor_zero =
{
  .bLength               = sizeof(usb_string_descriptor_zero_t),
  .bDescriptorType       = USB_STRING_DESCRIPTOR,
  .wLANGID               = 0x0409, // English (United States)
};

const char *usb_strings[] =
{
  [USB_STR_MANUFACTURER]  = "Stephan Electronics",
  [USB_STR_PRODUCT]       = "Mooltipass Mini BLE",
  [USB_STR_RAW_INTERFACE] = "Raw HID",
  [USB_STR_KEYB_INTERFACE] = "Keyboard HID",
};

alignas(4) uint8_t usb_string_descriptor_buffer[64];

