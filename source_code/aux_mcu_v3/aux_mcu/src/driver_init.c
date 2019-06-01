/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_init.h"
#include <peripheral_clk_config.h>
#include <utils.h>
#include <hal_init.h>
#include <hpl_gclk_base.h>
#include <hpl_pm_base.h>

/*! The buffer size for USART */
#define BLE_FC_UART_BUFFER_SIZE 16

/*! The buffer size for USART */
#define BLE_UART_BUFFER_SIZE 16

struct usart_async_descriptor BLE_FC_UART;
struct usart_async_descriptor BLE_UART;
struct timer_descriptor       Timer;

static uint8_t BLE_FC_UART_buffer[BLE_FC_UART_BUFFER_SIZE];
static uint8_t BLE_UART_buffer[BLE_UART_BUFFER_SIZE];

struct usart_sync_descriptor TARGET_IO;

void EXTERNAL_IRQ_0_init(void)
{
	_gclk_enable_channel(EIC_GCLK_ID, CONF_GCLK_EIC_SRC);

	// Set pin direction to input
	gpio_set_pin_direction(BLE_APP_SW, GPIO_DIRECTION_IN);

	gpio_set_pin_pull_mode(BLE_APP_SW,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_UP);

	gpio_set_pin_function(BLE_APP_SW, PINMUX_PA15A_EIC_EXTINT15);

	ext_irq_init();
}

/**
 * \brief USART Clock initialization function
 *
 * Enables register interface and peripheral clock
 */
void BLE_FC_UART_CLOCK_init()
{

	_pm_enable_bus_clock(PM_BUS_APBC, SERCOM0);
	_gclk_enable_channel(SERCOM0_GCLK_ID_CORE, CONF_GCLK_SERCOM0_CORE_SRC);
}

/**
 * \brief USART pinmux initialization function
 *
 * Set each required pin to USART functionality
 */
void BLE_FC_UART_PORT_init()
{

	gpio_set_pin_function(PA04, PINMUX_PA04D_SERCOM0_PAD0);

	gpio_set_pin_function(PA05, PINMUX_PA05D_SERCOM0_PAD1);

	gpio_set_pin_function(PA06, PINMUX_PA06D_SERCOM0_PAD2);

	gpio_set_pin_function(PA07, PINMUX_PA07D_SERCOM0_PAD3);
}

/**
 * \brief USART initialization function
 *
 * Enables USART peripheral, clocks and initializes USART driver
 */
void BLE_FC_UART_init(void)
{
	BLE_FC_UART_CLOCK_init();
	usart_async_init(&BLE_FC_UART, SERCOM0, BLE_FC_UART_buffer, BLE_FC_UART_BUFFER_SIZE, (void *)NULL);
	BLE_FC_UART_PORT_init();
}

/**
 * \brief USART Clock initialization function
 *
 * Enables register interface and peripheral clock
 */
void BLE_UART_CLOCK_init()
{

	_pm_enable_bus_clock(PM_BUS_APBC, SERCOM2);
	_gclk_enable_channel(SERCOM2_GCLK_ID_CORE, CONF_GCLK_SERCOM2_CORE_SRC);
}

/**
 * \brief USART pinmux initialization function
 *
 * Set each required pin to USART functionality
 */
void BLE_UART_PORT_init()
{

	gpio_set_pin_function(PA08, PINMUX_PA08D_SERCOM2_PAD0);

	gpio_set_pin_function(PA09, PINMUX_PA09D_SERCOM2_PAD1);
}

/**
 * \brief USART initialization function
 *
 * Enables USART peripheral, clocks and initializes USART driver
 */
void BLE_UART_init(void)
{
	BLE_UART_CLOCK_init();
	usart_async_init(&BLE_UART, SERCOM2, BLE_UART_buffer, BLE_UART_BUFFER_SIZE, (void *)NULL);
	BLE_UART_PORT_init();
}

void TARGET_IO_PORT_init(void)
{

	gpio_set_pin_function(PA22, PINMUX_PA22C_SERCOM3_PAD0);

	gpio_set_pin_function(PA23, PINMUX_PA23C_SERCOM3_PAD1);
}

void TARGET_IO_CLOCK_init(void)
{
	_pm_enable_bus_clock(PM_BUS_APBC, SERCOM3);
	_gclk_enable_channel(SERCOM3_GCLK_ID_CORE, CONF_GCLK_SERCOM3_CORE_SRC);
}

void TARGET_IO_init(void)
{
	TARGET_IO_CLOCK_init();
	usart_sync_init(&TARGET_IO, SERCOM3, (void *)NULL);
	TARGET_IO_PORT_init();
}

/**
 * \brief Timer initialization function
 *
 * Enables Timer peripheral, clocks and initializes Timer driver
 */
static void Timer_init(void)
{
	_pm_enable_bus_clock(PM_BUS_APBC, TC3);
	_gclk_enable_channel(TC3_GCLK_ID, CONF_GCLK_TC3_SRC);

	timer_init(&Timer, TC3, _tc_get_timer());
}

void system_init(void)
{
	init_mcu();

	// GPIO on PA01

	gpio_set_pin_level(CE_PIN,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(CE_PIN, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(CE_PIN, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PA03

	gpio_set_pin_level(WAKEUP_PIN,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(WAKEUP_PIN, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(WAKEUP_PIN, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PB01

	gpio_set_pin_level(BTLC1000_CHIP_ENABLE_PIN,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(BTLC1000_CHIP_ENABLE_PIN, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(BTLC1000_CHIP_ENABLE_PIN, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PB07

	gpio_set_pin_level(BTLC1000_WAKEUP_PIN,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(BTLC1000_WAKEUP_PIN, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(BTLC1000_WAKEUP_PIN, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PB30

	gpio_set_pin_level(BLE_APP_LED,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(BLE_APP_LED, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(BLE_APP_LED, GPIO_PIN_FUNCTION_OFF);

	EXTERNAL_IRQ_0_init();

	BLE_FC_UART_init();
	BLE_UART_init();

	TARGET_IO_init();

	Timer_init();
}
