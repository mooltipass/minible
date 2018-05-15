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
#include "clock_manager.h"
#include "dma.h"
#include "comm.h"
#include "ble/ble.h"

static volatile bool main_b_keyboard_enable = false;
static volatile bool main_b_generic_enable = false;
static volatile uint8_t main_b_vbus_status = 0;

/****************************************************************************/
/* The blob of code below is aimed at facilitating our development process  */
/* To understand this, you need to know that:                               */
/* - the Cortex M0 expects the stack pointer address to be at addr 0x0000   */
/* - the Cortex M0 expects the reset handler address to be at addr 0x0004   */
/*                                                                          */
/* So this is what we do:                                                   */
/* - put the sram base address + something at addr0 using the array below   */
/* - then put the address of a custom function made to boot the main code   */
/*                                                                          */
/* This is made possible by:                                                */
/* - adding .flash_start_addr=0x0 to linker option                          */
/* - adding .start_app_function_addr=0x200 (matches the 2nd element in array*/
/* - adding --undefined=jump_to_application_function to linker option       */
/* - adding --undefined=jump_to_application_function_addr to linker option  */
/*                                                                          */
/* To move the application address, change APP_START_ADDR & .text           */
/* What I don't understand: why the "+1" in the second array element        */
/****************************************************************************/
#define APP_START_ADDR (0x1000)
const uint32_t jump_to_application_function_addr[2] __attribute__((used,section (".flash_start_addr"))) = {HMCRAMC0_ADDR+100,0x200+1};
void jump_to_application_function(void) __attribute__((used,section (".start_app_function_addr")));
void jump_to_application_function(void)
{
    /* Overwriting the default value of the NVMCTRL.CTRLB.MANW bit (errata reference 13134) */
    NVMCTRL->CTRLB.bit.MANW = 1;

    /* Pointer to the Application Section */
    void (*application_code_entry)(void);

    /* Rebase the Stack Pointer */
    __set_MSP(*(uint32_t*)APP_START_ADDR);

    /* Rebase the vector table base address */
    SCB->VTOR = ((uint32_t)APP_START_ADDR & SCB_VTOR_TBLOFF_Msk);

    /* Load the Reset Handler address of the application */
    application_code_entry = (void (*)(void))(unsigned *)(*(unsigned *)(APP_START_ADDR + 4));

    /* Jump to user Reset Handler in the application */
    application_code_entry();
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

    // Port init
    port_manager_init();

    // Power Manager init
    power_manager_init();

    // Clock Manager init
    clock_manager_init();
#if 1
    // DMA init
    dma_init();

    // Initialize USBHID
    usbhid_init();

    // Init Serial communications
    comm_init();

    // Start USB stack to authorize VBus monitoring
    udc_start();
    // The main loop manages only the power mode
    // because the USB management is done by interrupt
    while (true) {
        //sleepmgr_enter_sleep();
        comm_task();
    }
#else
    while (true) {
        ble_main();
    }
#endif
}

void main_suspend_action(void) {
}

void main_resume_action(void) {

}

void main_sof_action(void) {
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
    main_b_vbus_status = b_vbus_high;
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
