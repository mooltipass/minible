/*
 * port_manager.c
 *
 *  Created on: 8 mar. 2018
 *      Author: mborregotrujillo
 */

#include "port_manager.h"

/**
 * \brief Macro to select specific bit
 */
#define SELECT_BIT(reg, bit)    ((reg & (1 << bit)) >> bit)

/**
 * \brief Pin configuration, i/o or peripheral
 */
#define PINCFG_IO           (0u)
#define PINCFG_PERIPHERAL   (1u)

/**
 * \brief Set of macros to configure specific I/O PIN
 *
 *                               DIR INEN PULLEN OUT
 *  IO_INPUT_DIGITAL_DISABLED     0    0    0     0
 *  IO_INPUT_DISABLED_PULLDOWN    0    0    1     0
 *  IO_INPUT_DISABLED_PULLUP      0    0    1     1
 *  IO_INPUT_DEFAULT              0    1    0     0
 *  IO_INPUT_PULLDOWN             0    1    1     0
 *  IO_INPUT_PULLUP               0    1    1     1
 *
 *  IO_OUTPUT_DEFAULT             1    0    0     X
 *  IO_OUTPUT_WITH_PULL           1    0    1     X
 *  IO_OUTPUT_READBACK            1    1    0     X
 *  IO_OUTPUT_WITH_PULL_READBACK  1    1    1     X
 */
#define IO_INPUT_DIGITAL_DISABLED          (0u)
#define IO_INPUT_DISABLED_PULLDOWN         (2u)
#define IO_INPUT_DISABLED_PULLUP           (3u)
#define IO_INPUT_DEFAULT                   (4u)
#define IO_INPUT_PULLDOWN                  (6u)
#define IO_INPUT_PULLUP                    (7u)

#define IO_OUTPUT_DEFAULT_LOW              (8u)
#define IO_OUTPUT_DEFAULT_HIGH             (9u)
#define IO_OUTPUT_WITH_PULL_LOW            (10u)
#define IO_OUTPUT_WITH_PULL_HIGH           (11u)
#define IO_OUTPUT_READBACK_LOW             (12u)
#define IO_OUTPUT_READBACK_HIGH            (13u)
#define IO_OUTPUT_WITH_PULL_READBACK_LOW   (14u)
#define IO_OUTPUT_WITH_PULL_READBACK_HIGH  (15u)


typedef struct pin_cfg {
    uint32_t pinid  : 8;    /* Pin configuration PA(0..31), PB(32..63) */
    uint32_t pincfg : 1;    /* 0-I/O, 1-Peripheral */
    uint32_t iocfg  : 4;    /* I/O configuration */
    uint32_t muxcfg : 4;    /* MUX peripheral configuration */
} T_pin_cfg;

/* BLE UART 1 -- SERCOM0 */
const T_pin_cfg pin_ble_rx_uart1 = {
    .pinid = PIN_PA04,
    .pincfg = PINCFG_PERIPHERAL,
    .iocfg = IO_INPUT_DEFAULT,
    .muxcfg = MUX_PA04D_SERCOM0_PAD0,
};
const T_pin_cfg pin_ble_tx_uart1 = {
    .pinid = PIN_PA05,
    .pincfg = PINCFG_PERIPHERAL,
    .iocfg = IO_INPUT_DEFAULT,
    .muxcfg = MUX_PA05D_SERCOM0_PAD1,
};
const T_pin_cfg pin_ble_cts_uart1 = {
    .pinid = PIN_PA06,
    .pincfg = PINCFG_PERIPHERAL,
    .iocfg = IO_INPUT_DEFAULT,
    .muxcfg = MUX_PA06D_SERCOM0_PAD2,
};
const T_pin_cfg pin_ble_rts_uart1 = {
    .pinid = PIN_PA07,
    .pincfg = PINCFG_PERIPHERAL,
    .iocfg = IO_INPUT_DEFAULT,
    .muxcfg = MUX_PA07D_SERCOM0_PAD3,
};
/* BLE UART 0 -- SERCOM2 */
const T_pin_cfg pin_ble_rx_uart0 = {
    .pinid = PIN_PA08,
    .pincfg = PINCFG_PERIPHERAL,
    .iocfg = IO_INPUT_DEFAULT,
    .muxcfg = MUX_PA08D_SERCOM2_PAD0,
};
const T_pin_cfg pin_ble_tx_uart0 = {
    .pinid = PIN_PA09,
    .pincfg = PINCFG_PERIPHERAL,
    .iocfg = IO_INPUT_DEFAULT,
    .muxcfg = MUX_PA09D_SERCOM2_PAD1,
};
const T_pin_cfg pin_ble_cts_uart0 = {
    .pinid = PIN_PA10,
    .pincfg = PINCFG_IO,
    .iocfg = IO_INPUT_DEFAULT,
    .muxcfg = 0,
};
const T_pin_cfg pin_ble_rts_uart0 = {
    .pinid = PIN_PA11,
    .pincfg = PINCFG_IO,
    .iocfg = IO_INPUT_DEFAULT,
    .muxcfg = 0,
};
/* AUX UART -- SERCOM1 */
T_pin_cfg pin_aux_tx = {
    .pinid = PIN_PA16,
    .pincfg = PINCFG_PERIPHERAL,
    .iocfg = IO_INPUT_PULLUP,
    .muxcfg = MUX_PA16C_SERCOM1_PAD0,
};
T_pin_cfg pin_aux_rx = {
    .pinid = PIN_PA17,
    .pincfg = PINCFG_PERIPHERAL,
    .iocfg = IO_INPUT_PULLUP,
    .muxcfg = MUX_PA17C_SERCOM1_PAD1,
};

/**
 * \fn      port_manager_config
 * \brief   Configures an specific mcu pin based on pin configuration parameter
 *
 * \param   [in] pin - T_pin_cfg pin configuration
 */
static void port_manager_config(T_pin_cfg* pin);


static void port_manager_config(T_pin_cfg* pin){
    PORT_WRCONFIG_Type wrconfig_tmp;
    uint32_t pinmask;
    uint8_t groupid;
    uint8_t pinid;

    /* Initialize Internal vars */
    groupid = pin->pinid / 32;
    pinid = pin->pinid % 32;
    pinmask = (1UL << pinid);

    wrconfig_tmp.bit.DRVSTR = 0;
    wrconfig_tmp.bit.PMUXEN = pin->pincfg;
    wrconfig_tmp.bit.PULLEN = SELECT_BIT(pin->iocfg, 1);
    wrconfig_tmp.bit.INEN = SELECT_BIT(pin->iocfg, 2);
    wrconfig_tmp.bit.WRPINCFG = 1;

    wrconfig_tmp.bit.PMUX = pin->muxcfg;
    wrconfig_tmp.bit.WRPMUX = 1;

    if(pinid < 16){
        wrconfig_tmp.bit.PINMASK = (uint16_t)(1 << pinid);
        wrconfig_tmp.bit.HWSEL = 0;
    } else{
        wrconfig_tmp.bit.PINMASK = (uint16_t)(1 << (pinid-16));
        wrconfig_tmp.bit.HWSEL = 1;
    }

    // Clear Dir before configuring
    //PORT->Group[groupid].DIRCLR.reg = pinmask;


    /* Write Configuration to update PMUX */
    PORT->Group[groupid].WRCONFIG.reg = wrconfig_tmp.reg;


    /* DIR */
    if(SELECT_BIT(pin->iocfg, 3)){
        PORT->Group[groupid].DIRSET.reg = pinmask;
    } else{
        PORT->Group[groupid].DIRCLR.reg = pinmask;
    }
    /* OUT */
    if(SELECT_BIT(pin->iocfg, 0)){
        PORT->Group[groupid].OUTSET.reg = pinmask;
    } else{
        PORT->Group[groupid].OUTCLR.reg = pinmask;
    }

}


void port_manager_init(void){
    port_manager_config(&pin_aux_tx);
}
