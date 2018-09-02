/**
 * \file
 *
 * \brief SAM L21 serial driver configuration.
 *
 * Copyright (c) 2017 Atmel Corporation. All rights reserved.
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

#ifndef CONF_SERIALDRV_H_INCLUDED
#define CONF_SERIALDRV_H_INCLUDED

#define CONF_BLE_USART_MODULE  		SERCOM2
#define CONF_BLE_MUX_SETTING   		USART_RX_1_TX_0_XCK_1
#define CONF_BLE_PINMUX_PAD0   		PINMUX_PA08D_SERCOM2_PAD0
#define CONF_BLE_PINMUX_PAD1   		PINMUX_PA09D_SERCOM2_PAD1
#define CONF_BLE_PINMUX_PAD2   		PINMUX_UNUSED
#define CONF_BLE_PINMUX_PAD3   		PINMUX_UNUSED
#define CONF_UART_BAUDRATE      	115200
#define CONF_BLE_UART_CLOCK	   		GCLK_GENERATOR_0

#define CONF_FLCR_BLE_USART_MODULE  SERCOM0
#define CONF_FLCR_BLE_MUX_SETTING   USART_RX_1_TX_0_RTS_2_CTS_3
#define CONF_FLCR_BLE_PINMUX_PAD0   PINMUX_PA04D_SERCOM0_PAD0
#define CONF_FLCR_BLE_PINMUX_PAD1   PINMUX_PA05D_SERCOM0_PAD1
#define CONF_FLCR_BLE_PINMUX_PAD2   PINMUX_PA06D_SERCOM0_PAD2
#define CONF_FLCR_BLE_PINMUX_PAD3   PINMUX_PA07D_SERCOM0_PAD3
#define CONF_FLCR_BLE_BAUDRATE      115200
#define CONF_FLCR_BLE_UART_CLOCK	GCLK_GENERATOR_0

/* BTLC1000 Wakeup Pin */
#define BTLC1000_WAKEUP_PIN			    (PIN_PA15)
#define BTLC1000_HOST_WAKEUP_PIN        (PIN_PA03)
#define BTLC1000_HOST_WAKEUP_EIC_PIN    (PIN_PA03A_EIC_EXTINT3)
#define BTLC1000_HOST_WAKEUP_EIC_MUX    (MUX_PA03A_EIC_EXTINT3)
#define BTLC1000_HOST_WAKEUP_EIC_LINE   (3)
#define BTLC1000_CHIP_ENABLE_PIN	    (PIN_PA01)
#define BLE_UART_RTS_PIN			    (PIN_PA06)
#define BTLC1000_UART_RTS_PIN           (BLE_UART_RTS_PIN)
#define BLE_UART_CTS_PIN			    (PIN_PA07)
#define BTLC1000_UART_CTS_PIN           (BLE_UART_CTS_PIN)
#define HOST_SYSTEM_SLEEP_MODE          (SYSTEM_SLEEPMODE_IDLE_2) 

/* BTLC1000 50ms Reset Duration */
#define BTLC1000_RESET_MS			(50)

/* set port pin high */
#define IOPORT_PIN_LEVEL_HIGH		(true)
/* Set port pin low */
#define IOPORT_PIN_LEVEL_LOW		(false)

void serial_rx_callback(void);
void serial_tx_callback(void);

#define SERIAL_DRV_RX_CB serial_rx_callback
#define SERIAL_DRV_TX_CB serial_tx_callback
#define SERIAL_DRV_TX_CB_ENABLE  true
#define SERIAL_DRV_RX_CB_ENABLE  true

#define BLE_MAX_RX_PAYLOAD_SIZE 1024
#define BLE_MAX_TX_PAYLOAD_SIZE 1024

void platform_host_wake_interrupt_handler(void);
static inline void btlc1000_host_wakeup_config(void);
static inline void btlc1000_host_wakeup_handler(void);

static inline void btlc1000_host_wakeup_config(void)
{
	struct extint_chan_conf eint_chan_conf;
	extint_chan_get_config_defaults(&eint_chan_conf);

	eint_chan_conf.gpio_pin           = BTLC1000_HOST_WAKEUP_EIC_PIN;
	eint_chan_conf.gpio_pin_pull      = EXTINT_PULL_UP;
	eint_chan_conf.gpio_pin_mux       = BTLC1000_HOST_WAKEUP_EIC_MUX;
	eint_chan_conf.detection_criteria = EXTINT_DETECT_FALLING;
	eint_chan_conf.filter_input_signal = true;
	eint_chan_conf.wake_if_sleeping = true;
	
	extint_chan_set_config(BTLC1000_HOST_WAKEUP_EIC_LINE, &eint_chan_conf);
	
	extint_register_callback(btlc1000_host_wakeup_handler,
	BTLC1000_HOST_WAKEUP_EIC_LINE,
	EXTINT_CALLBACK_TYPE_DETECT);
	
	extint_chan_enable_callback(BTLC1000_HOST_WAKEUP_EIC_LINE,
	EXTINT_CALLBACK_TYPE_DETECT);
	
}

static inline void btlc1000_host_wakeup_handler(void)
{
	platform_host_wake_interrupt_handler();
}

static inline bool host_event_data_ready_pin_level(void)
{
	return (port_pin_get_input_level(BTLC1000_HOST_WAKEUP_PIN));
}

static inline bool btlc1000_cts_pin_level(void)
{
	return (port_pin_get_output_level(BTLC1000_UART_CTS_PIN));
}

/* Set BLE Wakeup pin to be low */
static inline bool ble_wakeup_pin_level(void)
{
	return (port_pin_get_output_level(BTLC1000_WAKEUP_PIN));
}

/* Set BLE Wakeup pin to be low */
static inline void ble_wakeup_pin_set_low(void)
{
	port_pin_set_output_level(BTLC1000_WAKEUP_PIN, 
							IOPORT_PIN_LEVEL_LOW);
}

/* Set wakeup pin to high */
static inline void ble_wakeup_pin_set_high(void)
{
	port_pin_set_output_level(BTLC1000_WAKEUP_PIN, 
							IOPORT_PIN_LEVEL_HIGH);
}

/* Set enable pin to Low */
static inline void ble_enable_pin_set_low(void)
{
	port_pin_set_output_level(BTLC1000_CHIP_ENABLE_PIN, 
							IOPORT_PIN_LEVEL_LOW);
}

/* Set enable pin to high */
static inline void ble_enable_pin_set_high(void)
{
	port_pin_set_output_level(BTLC1000_CHIP_ENABLE_PIN, 
							IOPORT_PIN_LEVEL_HIGH);
}

/* Configure the BTLC1000 control(chip_enable, wakeup) pins */
static inline void ble_configure_control_pin(void)
{
	struct port_config pin_conf;
	
	/* initialize the delay before use */
	delay_init();
	
	/* get the default values for port pin configuration */
	port_get_config_defaults(&pin_conf);

	/* Configure control pins as output */
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	pin_conf.input_pull = PORT_PIN_PULL_DOWN;
	
	port_pin_set_config(BTLC1000_WAKEUP_PIN, &pin_conf);
	/* set wakeup pin to low */
	ble_wakeup_pin_set_high();
	
	port_pin_set_config(BTLC1000_CHIP_ENABLE_PIN, &pin_conf);
	/* set chip enable to low */
	ble_enable_pin_set_low();
	
	/* Delay for 50ms */
	delay_ms(BTLC1000_RESET_MS);
	
	/* set chip enable to high */
	ble_enable_pin_set_high();
}

static inline void ble_reset(void)
{
	/* BTLC1000 Reset Sequence @Todo */
	ble_enable_pin_set_high();
	ble_wakeup_pin_set_high();
	delay_ms(BTLC1000_RESET_MS);
	
	ble_enable_pin_set_low();
	ble_wakeup_pin_set_low();
	delay_ms(BTLC1000_RESET_MS);
	
	ble_enable_pin_set_high();
	ble_wakeup_pin_set_high();
	delay_ms(BTLC1000_RESET_MS);
}

#endif /* CONF_SERIALDRV_H_INCLUDED */
