/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_examples.h"
#include "driver_init.h"
#include "utils.h"

static void button_on_PA15_pressed(void)
{
}

/**
 * Example of using EXTERNAL_IRQ_0
 */
void EXTERNAL_IRQ_0_example(void)
{

	ext_irq_register(PIN_PA15, button_on_PA15_pressed);
}

/**
 * Example of using BLE_FC_UART to write "Hello World" using the IO abstraction.
 *
 * Since the driver is asynchronous we need to use statically allocated memory for string
 * because driver initiates transfer and then returns before the transmission is completed.
 *
 * Once transfer has been completed the tx_cb function will be called.
 */

static uint8_t example_BLE_FC_UART[12] = "Hello World!";

static void tx_cb_BLE_FC_UART(const struct usart_async_descriptor *const io_descr)
{
	/* Transfer completed */
}

void BLE_FC_UART_example(void)
{
	struct io_descriptor *io;

	usart_async_register_callback(&BLE_FC_UART, USART_ASYNC_TXC_CB, tx_cb_BLE_FC_UART);
	/*usart_async_register_callback(&BLE_FC_UART, USART_ASYNC_RXC_CB, rx_cb);
	usart_async_register_callback(&BLE_FC_UART, USART_ASYNC_ERROR_CB, err_cb);*/
	usart_async_get_io_descriptor(&BLE_FC_UART, &io);
	usart_async_enable(&BLE_FC_UART);

	io_write(io, example_BLE_FC_UART, 12);
}

/**
 * Example of using BLE_UART to write "Hello World" using the IO abstraction.
 *
 * Since the driver is asynchronous we need to use statically allocated memory for string
 * because driver initiates transfer and then returns before the transmission is completed.
 *
 * Once transfer has been completed the tx_cb function will be called.
 */

static uint8_t example_BLE_UART[12] = "Hello World!";

static void tx_cb_BLE_UART(const struct usart_async_descriptor *const io_descr)
{
	/* Transfer completed */
}

void BLE_UART_example(void)
{
	struct io_descriptor *io;

	usart_async_register_callback(&BLE_UART, USART_ASYNC_TXC_CB, tx_cb_BLE_UART);
	/*usart_async_register_callback(&BLE_UART, USART_ASYNC_RXC_CB, rx_cb);
	usart_async_register_callback(&BLE_UART, USART_ASYNC_ERROR_CB, err_cb);*/
	usart_async_get_io_descriptor(&BLE_UART, &io);
	usart_async_enable(&BLE_UART);

	io_write(io, example_BLE_UART, 12);
}

/**
 * Example of using TARGET_IO to write "Hello World" using the IO abstraction.
 */
void TARGET_IO_example(void)
{
	struct io_descriptor *io;
	usart_sync_get_io_descriptor(&TARGET_IO, &io);
	usart_sync_enable(&TARGET_IO);

	io_write(io, (uint8_t *)"Hello World!", 12);
}

static struct timer_task Timer_task1, Timer_task2;

/**
 * Example of using Timer.
 */
static void Timer_task1_cb(const struct timer_task *const timer_task)
{
}

static void Timer_task2_cb(const struct timer_task *const timer_task)
{
}

void Timer_example(void)
{
	Timer_task1.interval = 100;
	Timer_task1.cb       = Timer_task1_cb;
	Timer_task1.mode     = TIMER_TASK_REPEAT;
	Timer_task2.interval = 200;
	Timer_task2.cb       = Timer_task2_cb;
	Timer_task2.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&Timer, &Timer_task1);
	timer_add_task(&Timer, &Timer_task2);
	timer_start(&Timer);
}
