/**
 * \file serial_drv.c
 *
 * \brief Handles Serial driver functionalities
 *
 * Copyright (c) 2013-2015 Atmel Corporation. All rights reserved.
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
 */

/* === INCLUDES ============================================================ */

#include "serial_drv.h"
#include "ble_utils.h"
#include "hal_usart_async.h"
#include "platform.h"
#include "ble_config.h"
#include "driver_init.h"
#include "atbtlc1000_pins.h"

#if UART_FLOWCONTROL_6WIRE_MODE == true
#define BLE_COM_UART BLE_UART
#endif
#define BLE_COM_FC_UART BLE_FC_UART

enum pf_uart_state { Before_patch, After_patch } pf_uart_state_t;

/* === TYPES =============================================================== */

/* === MACROS ============================================================== */

/* === PROTOTYPES ========================================================== */
static void serial_drv_read_cb(const struct usart_async_descriptor *const io_descr);
static void serial_drv_write_cb(const struct usart_async_descriptor *const io_descr);

struct io_descriptor *usart_io;

/* === GLOBALS ========================================================== */

volatile uint8_t g_txdata;

/* === IMPLEMENTATION ====================================================== */
static inline void usart_configure_flowcontrol(void)
{
#if UART_FLOWCONTROL_6WIRE_MODE == true
	usart_async_disable(&BLE_COM_UART);
	usart_async_deinit(&BLE_COM_UART);
#endif

	union usart_flow_control_state fw_state;
	fw_state.bit.cts = 1;
	fw_state.bit.rts = 1;

	usart_async_set_flow_control(&BLE_COM_FC_UART, fw_state);
	usart_async_register_callback(&BLE_COM_FC_UART, USART_ASYNC_RXC_CB, serial_drv_read_cb);
	usart_async_register_callback(&BLE_COM_FC_UART, USART_ASYNC_TXC_CB, serial_drv_write_cb);
	usart_async_get_io_descriptor(&BLE_COM_FC_UART, &usart_io);
	usart_async_enable(&BLE_COM_FC_UART);
	pf_uart_state_t = After_patch;
}

uint8_t configure_serial_drv(void)
{
#if UART_FLOWCONTROL_4WIRE_MODE == true
	usart_configure_flowcontrol();
#warning "This mode works only if Flow Control Permanently Enabled in the BTLC1000"
#else
	usart_async_register_callback(&BLE_COM_UART, USART_ASYNC_RXC_CB, serial_drv_read_cb);
	usart_async_register_callback(&BLE_COM_UART, USART_ASYNC_TXC_CB, serial_drv_write_cb);
	usart_async_get_io_descriptor(&BLE_COM_UART, &usart_io);
	usart_async_enable(&BLE_COM_UART);
	pf_uart_state_t = Before_patch;
#endif

	return 0;
}

void configure_usart_after_patch(void)
{
#if UART_FLOWCONTROL_6WIRE_MODE == true
	usart_configure_flowcontrol();
#endif
}

uint16_t serial_drv_send(uint8_t *data, uint16_t len)
{

	while (platform_serial_drv_tx_status())
		;
	if (len) {
		g_txdata = true;
		io_write(usart_io, data, len);
		while (platform_serial_drv_tx_status())
			;
	}

	/* Wait for ongoing transmission complete */
	while (g_txdata)
		;

	return 0;
}

extern void platform_process_rxdata(uint8_t *buf, uint16_t len);

static void serial_drv_read_cb(const struct usart_async_descriptor *const io_descr)
{
	uint8_t  buf_data[250];
	uint16_t length;

	length = ringbuffer_num(&io_descr->rx);
	if (length) {
		/* Enough Room to push all the data */
		length = usart_io->read(usart_io, buf_data, length);
		platform_process_rxdata(buf_data, length);
	}
}

static void serial_drv_write_cb(const struct usart_async_descriptor *const io_descr)
{
	while (platform_serial_drv_tx_status())
		;
	g_txdata = false;
}

uint32_t platform_serial_drv_tx_status(void)
{
#if UART_FLOWCONTROL_6WIRE_MODE == true
	if (pf_uart_state_t == After_patch)
		return (!usart_async_is_tx_empty(&BLE_COM_FC_UART));
	else
		return (!usart_async_is_tx_empty(&BLE_COM_UART));
#else
	return (!usart_async_is_tx_empty(&BLE_COM_FC_UART));
#endif
}

/* EOF */
