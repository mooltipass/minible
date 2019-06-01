/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file or main.c
 * to avoid loosing it when reconfiguring.
 */
#include "atmel_start.h"
#include "ble_config.h"
#include "hal_delay.h"

#ifndef BTLC1000_WAKEUP_PIN
#warning "BTLC1000 Wakeup pin not defined, Default to PORTA-PIN21"
#define BTLC1000_WAKEUP_PIN GPIO(GPIO_PORTA, 21)
#endif

#ifndef BTLC1000_CHIP_ENABLE_PIN
#warning "BTLC1000 Chip Enable pin not defined, Default to PORTA-PIN21"
#define BTLC1000_CHIP_ENABLE_PIN GPIO(GPIO_PORTB, 15)
#endif

/* BTLC1000 50ms Reset Duration */
#define BTLC1000_RESET_MS (50)

#define IOPORT_PIN_LEVEL_LOW (false)
#define IOPORT_PIN_LEVEL_HIGH (true)

/* Set BLE Wakeup pin to be low */
void ble_wakeup_pin_set_low(void)
{
	gpio_set_pin_level(BTLC1000_WAKEUP_PIN, IOPORT_PIN_LEVEL_LOW);
}

/* Set BLE Wakeup pin to be low */
bool ble_wakeup_pin_level(void)
{
	return (gpio_get_pin_level(BTLC1000_WAKEUP_PIN));
}

/* Set wakeup pin to high */
void ble_wakeup_pin_set_high(void)
{
	gpio_set_pin_level(BTLC1000_WAKEUP_PIN, IOPORT_PIN_LEVEL_HIGH);
}

/* Set enable pin to Low */
void ble_enable_pin_set_low(void)
{
	gpio_set_pin_level(BTLC1000_CHIP_ENABLE_PIN, IOPORT_PIN_LEVEL_LOW);
}

/* Set enable pin to high */
void ble_enable_pin_set_high(void)
{
	gpio_set_pin_level(BTLC1000_CHIP_ENABLE_PIN, IOPORT_PIN_LEVEL_HIGH);
}

/* Configure the BTLC1000 control(chip_enable, wakeup) pins */
void ble_configure_control_pin(void)
{
	/* initialize the delay before use */
	delay_init(NULL);

	gpio_set_pin_direction(BTLC1000_WAKEUP_PIN, GPIO_DIRECTION_OUT);
	gpio_set_pin_function(BTLC1000_WAKEUP_PIN, GPIO_PIN_FUNCTION_OFF);

	/* set wakeup pin to high */
	ble_wakeup_pin_set_high();

	gpio_set_pin_direction(BTLC1000_CHIP_ENABLE_PIN, GPIO_DIRECTION_OUT);
	gpio_set_pin_function(BTLC1000_CHIP_ENABLE_PIN, GPIO_PIN_FUNCTION_OFF);

	/* set chip enable to low */
	ble_enable_pin_set_low();

	/* Delay for 50ms */
	delay_ms(BTLC1000_RESET_MS);

	/* set chip enable to high */
	ble_enable_pin_set_high();
}
