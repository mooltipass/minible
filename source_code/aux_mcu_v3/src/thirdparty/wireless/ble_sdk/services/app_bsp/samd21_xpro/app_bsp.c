/*
file for SAMD21
*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <hal_gpio.h>
#include <hal_delay.h>
#include <hal_io.h>
#include <hal_timer.h>

#include "atbtlc1000_pins.h"
#include "driver_init.h"
#include "ble_config.h"
#include "app_bsp.h"
#include "platform.h"

static void ble_app_timer1_cb(const struct timer_task *const timer_task);
static void ble_app_timer2_cb(const struct timer_task *const timer_task);
static void ble_app_timer3_cb(const struct timer_task *const timer_task);

struct io_descriptor *console_usart_io;
uint8_t               print_buf[CONSOLE_BUFFER_SIZE];
volatile bool         console_print_done = true;
volatile bool         console_rx_done    = false;
uint8_t               console_rx_data;

struct timer_task app_timer_1_task, app_timer_2_task, app_timer_3_task;

ble_timer_cb_t ble_timer_cb[MAX_SW_TIMER] = {NULL, NULL, NULL};

platform_hw_timer_callback_t bus_timer_callback;

void bsp_init(void)
{
	system_init();
}

int usart_serial_putchar(struct io_descriptor *io_descr, char data)
{
	static uint8_t putchar_data;
	/* wait for console print to be completed */
	while (console_print_done == false) {
		;
	}

	putchar_data = (uint8_t)data;

	/* Change the Tx Flag to indicate transmission begins */
	console_print_done = false;

	/* Start transmission */
	io_descr->write(io_descr, (const uint8_t *)&putchar_data, 1);

	return 0;
}

int usart_serial_getchar(struct io_descriptor *io_descr, char *data)
{
	while (console_rx_done == false)
		;
	*data           = (char)console_rx_data;
	console_rx_done = false;
	return 0;
}

volatile bool bsp_timer_status[MAX_SW_TIMER] = {
    false,
};

void ble_timer_start(ble_app_timer_t timer_id, uint32_t time, ble_timer_mode_t timer_mode, ble_timer_cb_t timer_cb)
{
	if (MAX_SW_TIMER < timer_id) {
		/* Invalid timer id */
		return;
	}

	switch (timer_id) {
	case BLE_APP_TIMER_ID1: {
		if (!bsp_timer_status[BLE_APP_TIMER_ID1]) {
			ble_timer_cb[BLE_APP_TIMER_ID1] = timer_cb;
			app_timer_1_task.interval       = time;
			app_timer_1_task.cb             = ble_app_timer1_cb;
			app_timer_1_task.mode           = timer_mode;
			timer_add_task(&Timer, &app_timer_1_task);
			bsp_timer_status[BLE_APP_TIMER_ID1] = true;
		}

	} break;

	case BLE_APP_TIMER_ID2: {
		if (!bsp_timer_status[BLE_APP_TIMER_ID2]) {
			ble_timer_cb[BLE_APP_TIMER_ID2] = timer_cb;
			app_timer_2_task.interval       = time;
			app_timer_2_task.cb             = ble_app_timer2_cb;
			app_timer_2_task.mode           = timer_mode;
			timer_add_task(&Timer, &app_timer_2_task);
			bsp_timer_status[BLE_APP_TIMER_ID2] = true;
		}

	} break;

	case BLE_APP_TIMER_ID3: {
		if (!bsp_timer_status[BLE_APP_TIMER_ID3]) {
			ble_timer_cb[BLE_APP_TIMER_ID3] = timer_cb;
			app_timer_3_task.interval       = time;
			app_timer_3_task.cb             = ble_app_timer3_cb;
			app_timer_3_task.mode           = timer_mode;
			timer_add_task(&Timer, &app_timer_3_task);
			bsp_timer_status[BLE_APP_TIMER_ID3] = true;
		}

	} break;

	default:
		break;
	}
	timer_start(&Timer);
}

void ble_timer_stop(ble_app_timer_t timer_id)
{
	if (MAX_SW_TIMER < timer_id) {
		/* Invalid timer id */
		return;
	}

	switch (timer_id) {
	case BLE_APP_TIMER_ID1: {
		if (bsp_timer_status[BLE_APP_TIMER_ID1]) {
			timer_remove_task(&Timer, &app_timer_1_task);
			bsp_timer_status[BLE_APP_TIMER_ID1] = false;
		}
	} break;

	case BLE_APP_TIMER_ID2: {
		if (bsp_timer_status[BLE_APP_TIMER_ID2]) {
			timer_remove_task(&Timer, &app_timer_2_task);
			bsp_timer_status[BLE_APP_TIMER_ID2] = false;
		}
	} break;

	case BLE_APP_TIMER_ID3: {
		if (bsp_timer_status[BLE_APP_TIMER_ID3]) {
			timer_remove_task(&Timer, &app_timer_3_task);
			bsp_timer_status[BLE_APP_TIMER_ID3] = false;
		}
	} break;

	default:
		break;
	}
}

static void ble_app_timer1_cb(const struct timer_task *const timer_task)
{
	if (ble_timer_cb[BLE_APP_TIMER_ID1] == NULL) {
		return;
	} else {
		ble_timer_cb[BLE_APP_TIMER_ID1]();
	}
}

static void ble_app_timer2_cb(const struct timer_task *const timer_task)
{
	if (ble_timer_cb[BLE_APP_TIMER_ID2] == NULL) {
		return;
	} else {
		ble_timer_cb[BLE_APP_TIMER_ID2]();
	}
}

static void ble_app_timer3_cb(const struct timer_task *const timer_task)
{
	if (ble_timer_cb[BLE_APP_TIMER_ID3] == NULL) {
		return;
	} else {
		ble_timer_cb[BLE_APP_TIMER_ID3]();
	}
}

void platform_configure_timer(platform_hw_timer_callback_t bus_tc_cb_ptr)
{
	bus_timer_callback = bus_tc_cb_ptr;
}

void platform_start_bus_timer(uint32_t timeout)
{
	if (timeout == 0xFFFFFFFF) {
		return;
	}
	ble_timer_start(BLE_APP_TIMER_ID1, MS_TIMER(timeout), BLE_TIMER_REPEAT, (ble_timer_cb_t)bus_timer_callback);
}

void platform_stop_bus_timer(void)
{
	ble_timer_stop(BLE_APP_TIMER_ID1);
}
