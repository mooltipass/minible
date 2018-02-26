/**
 * \file
 *
 * \brief User Interface
 *
 * Copyright (c) 2014-2015 Atmel Corporation. All rights reserved.
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

#include <asf.h>
#include <string.h>
#include "ui.h"

// Commands
#define CMD_PING                0xA1
#define CMD_MOOLTIPASS_STATUS   0xB9
#define CMD_USB_KEYBOARD_PRESS  0x9B

#define UDI_HID_KBD_REPORT_SIZE 8
COMPILER_WORD_ALIGNED static uint8_t udi_hid_kbd_report_trans[UDI_HID_KBD_REPORT_SIZE];

static uint8_t ui_hid_report[UDI_HID_REPORT_OUT_SIZE];
static bool udi_hid_kbd_press_key(void);
static void ui_hid_raw_keyboard_press(uint8_t modifier, uint8_t key);


/* Interrupt on "pin change" from push button to do wakeup on USB
 * Note:
 * This interrupt is enable when the USB host enable remote wakeup feature
 * This interrupt wakeup the CPU if this one is in idle mode
 */
static void ui_wakeup_handler(void)
{
    /* It is a wakeup then send wakeup USB */
    udc_remotewakeup();
}


void ui_init(void)
{

}

void ui_powerdown(void)
{
}


void ui_wakeup_enable(void)
{
}

void ui_wakeup_disable(void)
{
}

void ui_wakeup(void)
{
}

void ui_start_read(void)
{
}

void ui_stop_read(void)
{
}

void ui_start_write(void)
{
}

void ui_stop_write(void)
{
}

void ui_process(uint16_t framenumber)
{

}

void ui_kbd_led(uint8_t value)
{
}

static void ui_hid_raw_echo( uint8_t *report ){

}

static void ui_hid_raw_mooltipass_status(void){

}

static void udi_hid_kbd_unpress_key (udd_ep_status_t status,
        iram_size_t nb_transfered, udd_ep_id_t ep){
        udi_hid_kbd_report_trans[0] = 0;
        udi_hid_kbd_report_trans[1] = 0;
        udi_hid_kbd_report_trans[2] = 0;
        udi_hid_kbd_report_trans[4] = 0;
        udi_hid_kbd_report_trans[5] = 0;
        udi_hid_kbd_report_trans[6] = 0;
        udi_hid_kbd_report_trans[7] = 0;
        irqflags_t flags = cpu_irq_save();
        (void)udd_ep_run( UDI_HID_KBD_EP_IN,
                false,
                udi_hid_kbd_report_trans,
                UDI_HID_KBD_REPORT_SIZE,
                udi_hid_kbd_unpress_key);
        cpu_irq_restore(flags);
}

static bool udi_hid_kbd_press_key(void)
{
    bool udi_hid_kbd_b_report_trans_ongoing;
    irqflags_t flags = cpu_irq_save();
    udi_hid_kbd_b_report_trans_ongoing =
            udd_ep_run( UDI_HID_KBD_EP_IN,
                            false,
                            udi_hid_kbd_report_trans,
                            UDI_HID_KBD_REPORT_SIZE,
                            udi_hid_kbd_unpress_key);
    cpu_irq_restore(flags);
    return udi_hid_kbd_b_report_trans_ongoing;
}

static void ui_hid_raw_keyboard_press(uint8_t modifier, uint8_t key){
    udi_hid_kbd_report_trans[0] = modifier;
    udi_hid_kbd_report_trans[1] = 0;
    udi_hid_kbd_report_trans[2] = key;
    udi_hid_kbd_report_trans[4] = 0;
    udi_hid_kbd_report_trans[5] = 0;
    udi_hid_kbd_report_trans[6] = 0;
    udi_hid_kbd_report_trans[7] = 0;
    udi_hid_kbd_press_key();
}

void ui_hid_raw(uint8_t *report)
{
    uint8_t len = report[0];
    uint8_t cmd = report[1];

    switch(cmd){
        case CMD_PING:
            ui_hid_raw_echo(report);
            break;
        case CMD_MOOLTIPASS_STATUS:
            ui_hid_raw_mooltipass_status();
            break;
        case CMD_USB_KEYBOARD_PRESS:
            ui_hid_raw_keyboard_press(report[3], report[2]);
            break;
        default:
            ui_hid_raw_echo(report);
            break;
    }
}
