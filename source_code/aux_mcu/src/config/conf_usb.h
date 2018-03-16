/**
 * \file
 *
 * \brief USB configuration file
 *
 * Copyright (c) 2009-2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#ifndef _CONF_USB_H_
#define _CONF_USB_H_

#include "compiler.h"

/**
 * USB Device Configuration
 * @{
 */

#define UDD_CLOCK_GEN      GCLK_GENERATOR_1
#define UDD_CLOCK_SOURCE   SYSTEM_CLOCK_SOURCE_DFLL

//! Device definition (mandatory)
#define  USB_DEVICE_VENDOR_ID             0x16D0 //USB_VID_ATMEL
#define  USB_DEVICE_PRODUCT_ID            0x09A0 //USB_PID_ATMEL_ASF_MSC_HIDMOUSE
#define  USB_DEVICE_MAJOR_VERSION         1
#define  USB_DEVICE_MINOR_VERSION         0
#define  USB_DEVICE_POWER                 100 // Consumption on Vbus line (mA)
#define  USB_DEVICE_ATTR                  \
	(USB_CONFIG_ATTR_REMOTE_WAKEUP|USB_CONFIG_ATTR_SELF_POWERED)
//	(USB_CONFIG_ATTR_REMOTE_WAKEUP|USB_CONFIG_ATTR_BUS_POWERED)
//	(USB_CONFIG_ATTR_SELF_POWERED)
// (USB_CONFIG_ATTR_BUS_POWERED)

//! USB Device string definitions (Optional)
#define  USB_DEVICE_MANUFACTURE_NAME      "SE"//"ATMEL ASF"
#define  USB_DEVICE_PRODUCT_NAME          "Mooltipass v3"//"HID Mouse and MSC"
//#define  USB_DEVICE_SERIAL_NAME           "123123123123"	// Disk SN for MSC

/**
 * Device speeds support
 * @{
 */

//! To define a Low speed device
//#define  USB_DEVICE_LOW_SPEED

//! To authorize the High speed
#if (UC3A3||UC3A4)
#  define  USB_DEVICE_HS_SUPPORT
#elif (SAM3XA||SAM3U)
#  define  USB_DEVICE_HS_SUPPORT
#endif
//@}

/**
 * USB Device Callbacks definitions (Optional)
 * @{
 */
#define  UDC_VBUS_EVENT(b_vbus_high)
#define  UDC_SOF_EVENT()                  main_sof_action()
#define  UDC_SUSPEND_EVENT()              main_suspend_action()
#define  UDC_RESUME_EVENT()               main_resume_action()
//! Mandatory when USB_DEVICE_ATTR authorizes remote wakeup feature
#define  UDC_REMOTEWAKEUP_ENABLE()        main_remotewakeup_enable()
#define  UDC_REMOTEWAKEUP_DISABLE()       main_remotewakeup_disable()
//! When a extra string descriptor must be supported
//! other than manufacturer, product and serial string
// #define  UDC_GET_EXTRA_STRING()
//@}

/**
 * USB Device low level configuration
 * When only one interface is used, these configurations are defined by the class module.
 * For composite device, these configuration must be defined here
 * @{
 */
//! Control endpoint size
#define  USB_DEVICE_EP_CTRL_SIZE       64

//! Two interfaces for this device (HID Generic + HID keyboard)
#define  USB_DEVICE_NB_INTERFACE       2

//! 3 endpoints used by HID mouse and MSC interfaces
#define  USB_DEVICE_MAX_EP             3
//@}

//@}


/**
 * USB Interface Configuration
 * @{
 */

/**
 * Configuration of HID Keyboard interface
 * @{
 */
//! Interface callback definition
#define  UDI_HID_KBD_ENABLE_EXT()       main_keyboard_enable()
#define  UDI_HID_KBD_DISABLE_EXT()      main_keyboard_disable()
#define  UDI_HID_KBD_CHANGE_LED(value)  //ui_kbd_led(value)

//! Enable id string of interface to add an extra USB string
//#define  UDI_HID_KBD_STRING_ID            7

/**
 * USB HID Keyboard low level configuration
 * In standalone these configurations are defined by the HID Keyboard module.
 * For composite device, these configuration must be defined here
 * @{
 */
//! Endpoint numbers definition
#define  UDI_HID_KBD_EP_IN           (3 | USB_EP_DIR_IN)

//! Interface number
#define  UDI_HID_KBD_IFACE_NUMBER    1
//@}
//@}

/**
 * Configuration of MSC interface
 * @{
 */
//! Vendor name and Product version of MSC interface
#define UDI_MSC_GLOBAL_VENDOR_ID            \
   'A', 'T', 'M', 'E', 'L', ' ', ' ', ' '
#define UDI_MSC_GLOBAL_PRODUCT_VERSION            \
   '1', '.', '0', '0'

//! Interface callback definition
#define  UDI_MSC_ENABLE_EXT()          main_msc_enable()
#define  UDI_MSC_DISABLE_EXT()         main_msc_disable()

/**
 * USB Interface Configuration
 * @{
 */
/**
 * Configuration of HID Generic interface
 * @{
 */
//! Interface callback definition
#define  UDI_HID_GENERIC_ENABLE_EXT()        main_generic_enable()
#define  UDI_HID_GENERIC_DISABLE_EXT()       main_generic_disable()
#define  UDI_HID_GENERIC_REPORT_OUT(ptr)     usbhid_usb_callback(ptr)
#define  UDI_HID_GENERIC_SET_FEATURE(report) main_hid_set_feature(report)

//! Sizes of I/O reports
#define  UDI_HID_REPORT_IN_SIZE             64
#define  UDI_HID_REPORT_OUT_SIZE            64
#define  UDI_HID_REPORT_FEATURE_SIZE        4

//! Sizes of I/O endpoints
#define  UDI_HID_GENERIC_EP_SIZE            64

//@}
//@}



/**
 * USB MSC low level configuration
 * In standalone these configurations are defined by the MSC module.
 * For composite device, these configuration must be defined here
 * @{
 */
//! Endpoint number used by HID generic interface
#define  UDI_HID_GENERIC_EP_OUT   (2 | USB_EP_DIR_OUT)
#define  UDI_HID_GENERIC_EP_IN    (1 | USB_EP_DIR_IN)

//! Interface number
#define  UDI_HID_GENERIC_IFACE_NUMBER     0
//@}
//@}


//@}


/**
 * Description of Composite Device
 * @{
 */
//! USB Interfaces descriptor structure
#define	UDI_COMPOSITE_DESC_T				\
	udi_hid_generic_desc_t udi_hid_raw;		\
	udi_hid_kbd_desc_t udi_hid_kbd

//! USB Interfaces descriptor value for Full Speed
#define	UDI_COMPOSITE_DESC_FS			\
	.udi_hid_raw               = UDI_HID_GENERIC_DESC,  \
	.udi_hid_kbd               = UDI_HID_KBD_DESC

//! USB Interfaces descriptor value for High Speed
#define	UDI_COMPOSITE_DESC_HS			\
    .udi_hid_raw               = UDI_HID_GENERIC_DESC,  \
    .udi_hid_kbd               = UDI_HID_KBD_DESC

//! USB Interface APIs
#define	UDI_COMPOSITE_API					\
	&udi_api_hid_generic,					\
	&udi_api_hid_kbd
//@}


/**
 * USB Device Driver Configuration
 * @{
 */
//@}

//! The includes of classes and other headers must be done at the end of this file to avoid compile error
#include "udi_hid_kbd.h"
#include "udi_hid_generic.h"
#include "usbhid.h"
#include "main.h"


#endif // _CONF_USB_H_
