/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */
#ifndef DRIVER_INIT_INCLUDED
#define DRIVER_INIT_INCLUDED

#include "atbtlc1000_pins.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_atomic.h>
#include <hal_delay.h>
#include <hal_gpio.h>
#include <hal_init.h>
#include <hal_io.h>
#include <hal_sleep.h>

#include <hal_ext_irq.h>

#include <hal_usart_async.h>
#include <hal_usart_async.h>

#include <hal_usart_sync.h>
#include <hal_timer.h>
#include <hpl_tc_base.h>

extern struct usart_async_descriptor BLE_FC_UART;
extern struct usart_async_descriptor BLE_UART;

extern struct usart_sync_descriptor TARGET_IO;
extern struct timer_descriptor      Timer;

void BLE_FC_UART_PORT_init(void);
void BLE_FC_UART_CLOCK_init(void);
void BLE_FC_UART_init(void);

void BLE_UART_PORT_init(void);
void BLE_UART_CLOCK_init(void);
void BLE_UART_init(void);

void TARGET_IO_PORT_init(void);
void TARGET_IO_CLOCK_init(void);
void TARGET_IO_init(void);

/**
 * \brief Perform system initialization, initialize pins and clocks for
 * peripherals
 */
void system_init(void);

#ifdef __cplusplus
}
#endif
#endif // DRIVER_INIT_INCLUDED
