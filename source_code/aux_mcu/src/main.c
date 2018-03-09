/**
 * \file
 *
 * \brief Main functions for USB composite example
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
#include <asf.h>
#include "usbhid.h"
#include "conf_usb.h"
#include "port_manager.h"
#include "driver_sercom.h"
#include "power_manager.h"

static volatile bool main_b_keyboard_enable = false;
static volatile bool main_b_generic_enable = false;

static inline void pin_set_peripheral_function(uint32_t pinmux)
{
    uint8_t port = (uint8_t)((pinmux >> 16)/32);
    PORT->Group[port].PINCFG[((pinmux >> 16) - (port*32))].bit.PMUXEN = 1;
    PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg &= ~(0xF << (4 * ((pinmux >>
    16) & 0x01u)));
    PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg |= (uint8_t)((pinmux &
    0x0000FFFF) << (4 * ((pinmux >> 16) & 0x01u)));
}

/*! \brief Main function. Execution starts here.
 */
int main(void) {
    irq_initialize_vectors();
    cpu_irq_enable();

    // Initialize the sleep manager
    //sleepmgr_init();

    // System init
    system_init();

    // Power Manager init
    power_manager_init();

    // Initialize Serial
    pin_set_peripheral_function(PINMUX_PA16C_SERCOM1_PAD0);
    pin_set_peripheral_function(PINMUX_PA17C_SERCOM1_PAD1);

    // Port init
    //port_manager_init();

    /* Setup clock for module */
    struct system_gclk_chan_config gclk_chan_config;
    system_gclk_chan_get_config_defaults(&gclk_chan_config);
    gclk_chan_config.source_generator = GCLK_GENERATOR_1;
    system_gclk_chan_set_config(SERCOM1_GCLK_ID_CORE, &gclk_chan_config);
    system_gclk_chan_enable(SERCOM1_GCLK_ID_CORE);

// 52 is  115200 at 48Mhz clock
    sercom_usart_init(SERCOM1, 1, USART_RX_PAD1, USART_TX_P0_XCK_P1);

    // Initialize USBHID
    USBHID_init();

    // Start USB stack to authorize VBus monitoring
    udc_start();

    // The main loop manages only the power mode
    // because the USB management is done by interrupt
    while (true) {
        // sleepmgr_enter_sleep();
        sercom_send_single_byte(SERCOM1, 0x55);
    }
}

void main_suspend_action(void) {
}

void main_resume_action(void) {

}

void main_sof_action(void) {
    if ((!main_b_keyboard_enable))
        return;
    //ui_process(udd_get_frame_number());
}

void main_remotewakeup_enable(void) {

}

void main_remotewakeup_disable(void) {

}

bool main_keyboard_enable(void) {
    main_b_keyboard_enable = true;
    return true;
}

void main_keyboard_disable(void) {
    main_b_keyboard_enable = false;
}

void main_vbus_event(uint8_t b_vbus_high) {
    (void)b_vbus_high;
    return;
}

bool main_generic_enable(void) {
    main_b_generic_enable = true;
    return true;
}

void main_generic_disable(void) {
    main_b_generic_enable = false;
}

void main_hid_set_feature(uint8_t* report) {
    (void)report;
}
