/**
 * \file
 *
 * \brief SAM L21 serial driver configuration.
 *
 * Copyright (c) 2017-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
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

static inline void ble_reset(void)
{
	/* BTLC1000 Reset Sequence @Todo */
	//ble_enable_pin_set_high();
	//ble_wakeup_pin_set_high();
	//delay_ms(BTLC1000_RESET_MS);
	//
	//ble_enable_pin_set_low();
	//ble_wakeup_pin_set_low();
	//delay_ms(BTLC1000_RESET_MS);
	//
	//ble_enable_pin_set_high();
	//ble_wakeup_pin_set_high();
	//delay_ms(BTLC1000_RESET_MS);
}

#endif /* CONF_SERIALDRV_H_INCLUDED */
