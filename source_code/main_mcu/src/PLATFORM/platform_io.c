/*!  \file     platform_io.c
*    \brief    Platform IO related functions
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "platform_defines.h"
#include "driver_sercom.h"
#include "driver_clocks.h"
#include "driver_timer.h"
#include "platform_io.h"
/* Set when a conversion result is ready */
volatile BOOL platform_io_voledin_conv_ready = FALSE;


/*! \fn     platform_io_enable_switch(void)
*   \brief  Enable switch (and 3v3 stepup)
*/
void platform_io_enable_switch(void)
{
    PORT->Group[SWDET_EN_GROUP].DIRSET.reg = SWDET_EN_MASK;
    PORT->Group[SWDET_EN_GROUP].OUTSET.reg = SWDET_EN_MASK;
}

/*! \fn     platform_io_disable_switch_and_die(void)
*   \brief  Disable switch and 3v3 (die)
*/
void platform_io_disable_switch_and_die(void)
{
    PORT->Group[SWDET_EN_GROUP].OUTCLR.reg = SWDET_EN_MASK;
}

/*! \fn     platform_io_release_aux_reset(void)
*   \brief  Release aux MCU reset
*/
void platform_io_release_aux_reset(void)
{
     PORT->Group[MCU_AUX_RST_EN_GROUP].OUTCLR.reg = MCU_AUX_RST_EN_MASK;
}

/*! \fn     platform_io_enable_ble(void)
*   \brief  Enable BLE module
*/
void platform_io_enable_ble(void)
{
    PORT->Group[BLE_EN_GROUP].OUTSET.reg = BLE_EN_MASK;
}

/*! \fn     platform_io_disable_ble(void)
*   \brief  Enable BLE module
*/
void platform_io_disable_ble(void)
{
    PORT->Group[BLE_EN_GROUP].OUTCLR.reg = BLE_EN_MASK;
}

/*! \fn     ADC_Handler(void)
*   \brief  Called once a conversion result is ready
*/
void ADC_Handler(void)
{
    /* Set conv ready bool and clear interrupt */
    platform_io_voledin_conv_ready = TRUE;
    ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;
}

/*! \fn     platform_io_is_voledin_conversion_result_ready(void)
*   \brief  Ask if a voledin conversion result is ready
*   \return the bool
*/
BOOL platform_io_is_voledin_conversion_result_ready(void)
{
    return platform_io_voledin_conv_ready;
}

/*! \fn     platform_io_get_voledin_conversion_result_and_trigger_conversion(void)
*   \brief  Fetch voled conversion result and trigger new conversion
*   \return conversion result
*/
uint16_t platform_io_get_voledin_conversion_result_and_trigger_conversion(void)
{
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
    platform_io_voledin_conv_ready = FALSE;
    uint16_t return_val = ADC->RESULT.reg;
    ADC->SWTRIG.reg = ADC_SWTRIG_START;
    return return_val;
}

/*! \fn     platform_io_init_bat_adc_measurements(void)
*   \brief  Initialize ADC to later perform battery voltage measurements
*/
void platform_io_init_bat_adc_measurements(void)
{
    PM->APBCMASK.bit.ADC_ = 1;                                                                  // Enable ADC bus clock
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, GCLK_CLKCTRL_ID_ADC_Val);                  // Map 48MHz to ADC unit
    ADC->REFCTRL.reg = ADC_REFCTRL_REFSEL(ADC_REFCTRL_REFSEL_INTVCC1_Val);                      // Set VCC/2 as a reference
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);                                       // Wait for sync
    ADC_CTRLB_Type temp_adc_ctrb_reg;                                                           // Temp register
    temp_adc_ctrb_reg.reg = 0;                                                                  // Set to 0
    temp_adc_ctrb_reg.bit.RESSEL = ADC_CTRLB_RESSEL_16BIT_Val;                                  // Set to 16bit result to allow averaging mode
    temp_adc_ctrb_reg.bit.PRESCALER = ADC_CTRLB_PRESCALER_DIV128_Val;                           // Set fclk_adc to 48M / 128 = 375kHz
    ADC->CTRLB = temp_adc_ctrb_reg;                                                             // Write ctrlb
    ADC->AVGCTRL.reg = ADC_AVGCTRL_ADJRES(4) | ADC_AVGCTRL_SAMPLENUM_1024;                      // Average on 1024 samples. Expected time for avg: 375k/(12-1)/1024 = 33.3Hz = 30ms. Single conversion mode, single ended, 12bit
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);                                       // Wait for sync
    ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXPOS(VBAT_ADC_PIN_MUXPOS) | ADC_INPUTCTRL_MUXNEG_GND;  // 1x gain, one channel set to voled in
    ADC->INTENSET.reg = ADC_INTENSET_RESRDY;                                                    // Enable in result ready interrupt
    NVIC_EnableIRQ(ADC_IRQn);                                                                   // Enable int
    uint16_t calib_val = ((*((uint32_t *)ADC_FUSES_LINEARITY_1_ADDR)) & 0x3F) << 5;             // Fetch calibration value
    calib_val |=  ((*((uint32_t *)ADC_FUSES_LINEARITY_0_ADDR)) & ADC_FUSES_LINEARITY_0_Msk) >> ADC_FUSES_LINEARITY_0_Pos;
    ADC->CALIB.reg = calib_val;                                                                 // Store calibration value
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);                                       // Wait for sync
    ADC->CTRLA.reg = ADC_CTRLA_ENABLE;                                                          // And enable ADC
}

/*! \fn     platform_io_init_scroll_wheel_ports(void)
*   \brief  Basic initialization of scroll wheels at boot
*/
void platform_io_init_scroll_wheel_ports(void)
{
    PORT->Group[WHEEL_A_GROUP].DIRCLR.reg = WHEEL_A_MASK;
    PORT->Group[WHEEL_A_GROUP].OUTSET.reg = WHEEL_A_MASK;
    PORT->Group[WHEEL_A_GROUP].PINCFG[WHEEL_A_PINID].bit.PULLEN = 1;
    PORT->Group[WHEEL_A_GROUP].PINCFG[WHEEL_A_PINID].bit.INEN = 1;
    PORT->Group[WHEEL_B_GROUP].DIRCLR.reg = WHEEL_B_MASK;
    PORT->Group[WHEEL_B_GROUP].OUTSET.reg = WHEEL_B_MASK;
    PORT->Group[WHEEL_B_GROUP].PINCFG[WHEEL_B_PINID].bit.PULLEN = 1;
    PORT->Group[WHEEL_B_GROUP].PINCFG[WHEEL_B_PINID].bit.INEN = 1;
    PORT->Group[WHEEL_SW_GROUP].DIRCLR.reg = WHEEL_SW_MASK;
    PORT->Group[WHEEL_SW_GROUP].OUTSET.reg = WHEEL_SW_MASK;
    PORT->Group[WHEEL_SW_GROUP].PINCFG[WHEEL_SW_PINID].bit.PULLEN = 1;
    PORT->Group[WHEEL_SW_GROUP].PINCFG[WHEEL_SW_PINID].bit.INEN = 1;
}

/*! \fn     platform_io_init_smc_ports(void)
*   \brief  Basic initialization of SMC ports at boot
*/
void platform_io_init_smc_ports(void)
{
    PORT->Group[SMC_DET_GROUP].DIRCLR.reg = SMC_DET_MASK;                           // Setup card detection input with pull-up
    PORT->Group[SMC_DET_GROUP].OUTSET.reg = SMC_DET_MASK;                           // Setup card detection input with pull-up    
    PORT->Group[SMC_DET_GROUP].PINCFG[SMC_DET_PINID].bit.PULLEN = 1;                // Setup card detection input with pull-up
    PORT->Group[SMC_DET_GROUP].PINCFG[SMC_DET_PINID].bit.INEN = 1;                  // Setup card detection input with pull-up
    PORT->Group[SMC_POW_NEN_GROUP].DIRSET.reg = SMC_POW_NEN_MASK;                   // Setup power enable, disabled by default
    PORT->Group[SMC_POW_NEN_GROUP].OUTSET.reg = SMC_POW_NEN_MASK;                   // Setup power enable, disabled by default
    PM->APBCMASK.bit.SMARTCARD_APB_SERCOM_BIT = 1;                                  // APB Clock Enable
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, SMARTCARD_GCLK_SERCOM_ID);     // Map 48MHz to SERCOM unit
    sercom_spi_init(SMARTCARD_SERCOM, SMARTCARD_BAUD_DIVIDER, SPI_MODE0, SPI_HSS_DISABLE, SMARTCARD_MISO_PAD, SMARTCARD_MOSI_SCK_PADS, TRUE); 
}

/*! \fn     platform_io_smc_remove_function(void)
*   \brief  Function called when smartcard is removed
*/
void platform_io_smc_remove_function(void)
{
    PORT->Group[SMC_POW_NEN_GROUP].OUTSET.reg = SMC_POW_NEN_MASK;                   // Deactivate power to the smart card
    PORT->Group[SMC_PGM_GROUP].DIRCLR.reg = SMC_PGM_MASK;                           // Setup all output pins as tri-state
    PORT->Group[SMC_RST_GROUP].DIRCLR.reg = SMC_RST_MASK;                           // Setup all output pins as tri-state
    PORT->Group[SMC_SCK_GROUP].PINCFG[SMC_SCK_PINID].bit.PMUXEN = 0;                // Disable SPI functionality
    PORT->Group[SMC_MOSI_GROUP].PINCFG[SMC_MOSI_PINID].bit.PMUXEN = 0;              // Disable SPI functionality
    PORT->Group[SMC_MISO_GROUP].PINCFG[SMC_MISO_PINID].bit.PMUXEN = 0;              // Disable SPI functionality
    PORT->Group[SMC_MOSI_GROUP].DIRCLR.reg = SMC_MOSI_MASK;                         // Disable SPI functionality
    PORT->Group[SMC_SCK_GROUP].DIRCLR.reg = SMC_SCK_MASK;                           // Disable SPI functionality
}

/*! \fn     platform_io_smc_inserted_function(void)
*   \brief  Function called when smartcard is inserted
*/
void platform_io_smc_inserted_function(void)
{
    PORT->Group[SMC_POW_NEN_GROUP].OUTCLR.reg = SMC_POW_NEN_MASK;                   // Enable power to the smart card
    PORT->Group[SMC_PGM_GROUP].DIRSET.reg = SMC_PGM_MASK;                           // PGM to 0
    PORT->Group[SMC_PGM_GROUP].OUTCLR.reg = SMC_PGM_MASK;                           // PGM to 0
    PORT->Group[SMC_RST_GROUP].DIRSET.reg = SMC_RST_MASK;                           // RST to 1
    PORT->Group[SMC_RST_GROUP].OUTSET.reg = SMC_RST_MASK;                           // RST to 1
    PORT->Group[SMC_SCK_GROUP].DIRSET.reg = SMC_SCK_MASK;                           // Enable SPI functionality
    PORT->Group[SMC_SCK_GROUP].OUTCLR.reg = SMC_SCK_MASK;                           // Enable SPI functionality
    PORT->Group[SMC_SCK_GROUP].PINCFG[SMC_SCK_PINID].bit.PMUXEN = 1;                // Enable SPI functionality
    PORT->Group[SMC_SCK_GROUP].PMUX[SMC_SCK_PINID/2].bit.SMC_SCK_PMUXREGID = SMC_SCK_PMUX_ID;
    PORT->Group[SMC_MOSI_GROUP].DIRSET.reg = SMC_MOSI_MASK;                         // Enable SPI functionality
    PORT->Group[SMC_MOSI_GROUP].OUTCLR.reg = SMC_MOSI_MASK;                         // Enable SPI functionality
    PORT->Group[SMC_MOSI_GROUP].PINCFG[SMC_MOSI_PINID].bit.PMUXEN = 1;              // Enable SPI functionality
    PORT->Group[SMC_MOSI_GROUP].PMUX[SMC_MOSI_PINID/2].bit.SMC_MOSI_PMUXREGID = SMC_MOSI_PMUX_ID;
    PORT->Group[SMC_MISO_GROUP].PINCFG[SMC_MISO_PINID].bit.PMUXEN = 1;              // Enable SPI functionality
    PORT->Group[SMC_MISO_GROUP].PMUX[SMC_MISO_PINID/2].bit.SMC_MISO_PMUXREGID = SMC_MISO_PMUX_ID;
    PORT->Group[SMC_MISO_GROUP].PINCFG[SMC_MISO_PINID].bit.INEN = 1;                // MISO as input (required when switching to bit banging)
}

/*! \fn     platform_io_smc_switch_to_bb(void)
*   \brief  Switch to bit banging mode for SPI
*/
void platform_io_smc_switch_to_bb(void)
{
    PORT->Group[SMC_SCK_GROUP].PINCFG[SMC_SCK_PINID].bit.PMUXEN = 0;                // Disable SPI functionality
    PORT->Group[SMC_MOSI_GROUP].PINCFG[SMC_MOSI_PINID].bit.PMUXEN = 0;              // Disable SPI functionality
    PORT->Group[SMC_MISO_GROUP].PINCFG[SMC_MISO_PINID].bit.PMUXEN = 0;              // Disable SPI functionality
}

/*! \fn     platform_io_smc_switch_to_spi(void)
*   \brief  Switch to bit banging mode for SPI
*/
void platform_io_smc_switch_to_spi(void)
{
    PORT->Group[SMC_SCK_GROUP].PINCFG[SMC_SCK_PINID].bit.PMUXEN = 1;                // Enable SPI functionality
    PORT->Group[SMC_MOSI_GROUP].PINCFG[SMC_MOSI_PINID].bit.PMUXEN = 1;              // Enable SPI functionality
    PORT->Group[SMC_MISO_GROUP].PINCFG[SMC_MISO_PINID].bit.PMUXEN = 1;              // Enable SPI functionality
}

/*! \fn     platform_io_init_accelerometer(void)
*   \brief  Initialize the platform accelerometer IO ports
*/
void platform_io_init_accelerometer_ports(void)
{
    EIC->EVCTRL.reg |= (1 << ACC_EXTINT_NUM);                                                                               // Enable events from extint pin
    EIC->CONFIG[ACC_EXTINT_NUM/8].bit.ACC_EIC_SENSE_REG = EIC_CONFIG_SENSE0_HIGH_Val;                                       // Detect high state
    PORT->Group[ACC_INT_GROUP].DIRCLR.reg = ACC_INT_MASK;                                                                   // Interrupt input, high Z
    PORT->Group[ACC_INT_GROUP].PINCFG[ACC_INT_PINID].bit.PMUXEN = 1;                                                        // Enable peripheral multiplexer
    PORT->Group[ACC_INT_GROUP].PMUX[ACC_INT_PINID/2].bit.ACC_INT_PMUXREGID = PORT_PMUX_PMUXO_A_Val;                         // Pin mux to EIC
    PORT->Group[ACC_nCS_GROUP].DIRSET.reg = ACC_nCS_MASK;                                                                   // nCS, OUTPUT high by default
    PORT->Group[ACC_nCS_GROUP].OUTSET.reg = ACC_nCS_MASK;                                                                   // nCS, OUTPUT high by default
    PORT->Group[ACC_SCK_GROUP].DIRSET.reg = ACC_SCK_MASK;                                                                   // SCK, OUTPUT
    PORT->Group[ACC_SCK_GROUP].PINCFG[ACC_SCK_PINID].bit.PMUXEN = 1;                                                        // Enable peripheral multiplexer
    PORT->Group[ACC_SCK_GROUP].PMUX[ACC_SCK_PINID/2].bit.ACC_SCK_PMUXREGID = ACC_SCK_PMUX_ID;                               // SCK, OUTPUT
    PORT->Group[ACC_MOSI_GROUP].DIRSET.reg = DBFLASH_MOSI_MASK;                                                             // MOSI, OUTPUT
    PORT->Group[ACC_MOSI_GROUP].PINCFG[ACC_MOSI_PINID].bit.PMUXEN = 1;                                                      // Enable peripheral multiplexer
    PORT->Group[ACC_MOSI_GROUP].PMUX[ACC_MOSI_PINID/2].bit.ACC_MOSI_PMUXREGID = ACC_MOSI_PMUX_ID;                           // MOSI, OUTPUT
    PORT->Group[ACC_MISO_GROUP].DIRCLR.reg = ACC_MISO_MASK;                                                                 // MISO, INPUT
    PORT->Group[ACC_MISO_GROUP].PINCFG[ACC_MISO_PINID].bit.PMUXEN = 1;                                                      // Enable peripheral multiplexer
    PORT->Group[ACC_MISO_GROUP].PMUX[ACC_MISO_PINID/2].bit.ACC_MISO_PMUXREGID = ACC_MISO_PMUX_ID;                           // MOSI, OUTPUT
    PM->APBCMASK.bit.ACC_APB_SERCOM_BIT = 1;                                                                                // APB Clock Enable
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, ACC_GCLK_SERCOM_ID);                                                   // Map 48MHz to SERCOM unit
    sercom_spi_init(ACC_SERCOM, ACC_BAUD_DIVIDER, SPI_MODE0, SPI_HSS_DISABLE, ACC_MISO_PAD, ACC_MOSI_SCK_PADS, TRUE);    
}

/*! \fn     platform_io_init_flash_ports(void)
*   \brief  Initialize the platform flash IO ports
*/
void platform_io_init_flash_ports(void)
{    
    /* DATAFLASH */
    PORT->Group[DATAFLASH_nCS_GROUP].DIRSET.reg = DATAFLASH_nCS_MASK;                                                       // DATAFLASH nCS, OUTPUT high by default
    PORT->Group[DATAFLASH_nCS_GROUP].OUTSET.reg = DATAFLASH_nCS_MASK;                                                       // DATAFLASH nCS, OUTPUT high by default
    PORT->Group[DATAFLASH_SCK_GROUP].DIRSET.reg = DATAFLASH_SCK_MASK;                                                       // DATAFLASH SCK, OUTPUT
    PORT->Group[DATAFLASH_SCK_GROUP].PINCFG[DATAFLASH_SCK_PINID].bit.PMUXEN = 1;                                            // Enable peripheral multiplexer
    PORT->Group[DATAFLASH_SCK_GROUP].PMUX[DATAFLASH_SCK_PINID/2].bit.DATAFLASH_SCK_PMUXREGID = DATAFLASH_SCK_PMUX_ID;       // DATAFLASH SCK, OUTPUT
    PORT->Group[DATAFLASH_MOSI_GROUP].DIRSET.reg = DATAFLASH_MOSI_MASK;                                                     // DATAFLASH MOSI, OUTPUT
    PORT->Group[DATAFLASH_MOSI_GROUP].PINCFG[DATAFLASH_MOSI_PINID].bit.PMUXEN = 1;                                          // Enable peripheral multiplexer
    PORT->Group[DATAFLASH_MOSI_GROUP].PMUX[DATAFLASH_MOSI_PINID/2].bit.DATAFLASH_MOSI_PMUXREGID = DATAFLASH_MOSI_PMUX_ID;   // DATAFLASH MOSI, OUTPUT
    PORT->Group[DATAFLASH_MISO_GROUP].DIRCLR.reg = DATAFLASH_MISO_MASK;                                                     // DATAFLASH MISO, INPUT
    PORT->Group[DATAFLASH_MISO_GROUP].PINCFG[DATAFLASH_MISO_PINID].bit.PMUXEN = 1;                                          // Enable peripheral multiplexer
    PORT->Group[DATAFLASH_MISO_GROUP].PMUX[DATAFLASH_MISO_PINID/2].bit.DATAFLASH_MISO_PMUXREGID = DATAFLASH_MISO_PMUX_ID;   // DATAFLASH MOSI, OUTPUT 
    PM->APBCMASK.bit.DATAFLASH_APB_SERCOM_BIT = 1;                                                                          // APB Clock Enable
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, DATAFLASH_GCLK_SERCOM_ID);                                             // Map 48MHz to SERCOM unit
    sercom_spi_init(DATAFLASH_SERCOM, DATAFLASH_BAUD_DIVIDER, SPI_MODE0, SPI_HSS_DISABLE, DATAFLASH_MISO_PAD, DATAFLASH_MOSI_SCK_PADS, TRUE);    

    /* DBFLASH */
    PORT->Group[DBFLASH_nCS_GROUP].DIRSET.reg = DBFLASH_nCS_MASK;                                                           // DBFLASH nCS, OUTPUT high by default
    PORT->Group[DBFLASH_nCS_GROUP].OUTSET.reg = DBFLASH_nCS_MASK;                                                           // DBFLASH nCS, OUTPUT high by default
    PORT->Group[DBFLASH_SCK_GROUP].DIRSET.reg = DBFLASH_SCK_MASK;                                                           // DBFLASH SCK, OUTPUT
    PORT->Group[DBFLASH_SCK_GROUP].PINCFG[DBFLASH_SCK_PINID].bit.PMUXEN = 1;                                                // Enable peripheral multiplexer
    PORT->Group[DBFLASH_SCK_GROUP].PMUX[DBFLASH_SCK_PINID/2].bit.DBFLASH_SCK_PMUXREGID = DBFLASH_SCK_PMUX_ID;               // DBFLASH SCK, OUTPUT
    PORT->Group[DBFLASH_MOSI_GROUP].DIRSET.reg = DBFLASH_MOSI_MASK;                                                         // DBFLASH MOSI, OUTPUT
    PORT->Group[DBFLASH_MOSI_GROUP].PINCFG[DBFLASH_MOSI_PINID].bit.PMUXEN = 1;                                              // Enable peripheral multiplexer
    PORT->Group[DBFLASH_MOSI_GROUP].PMUX[DBFLASH_MOSI_PINID/2].bit.DBFLASH_MOSI_PMUXREGID = DBFLASH_MOSI_PMUX_ID;           // DBFLASH MOSI, OUTPUT
    PORT->Group[DBFLASH_MISO_GROUP].DIRCLR.reg = DBFLASH_MISO_MASK;                                                         // DBFLASH MISO, INPUT
    PORT->Group[DBFLASH_MISO_GROUP].PINCFG[DBFLASH_MISO_PINID].bit.PMUXEN = 1;                                              // Enable peripheral multiplexer
    PORT->Group[DBFLASH_MISO_GROUP].PMUX[DBFLASH_MISO_PINID/2].bit.DBFLASH_MISO_PMUXREGID = DBFLASH_MISO_PMUX_ID;           // DBFLASH MOSI, OUTPUT
    PM->APBCMASK.bit.DBFLASH_APB_SERCOM_BIT = 1;                                                                            // APB Clock Enable
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, DBFLASH_GCLK_SERCOM_ID);                                               // Map 48MHz to SERCOM unit
    sercom_spi_init(DBFLASH_SERCOM, DBFLASH_BAUD_DIVIDER, SPI_MODE0, SPI_HSS_DISABLE, DBFLASH_MISO_PAD, DBFLASH_MOSI_SCK_PADS, TRUE);
}

/*! \fn     platform_io_init_oled_ports(void)
*   \brief  Initialize the platform oled IO ports
*/
void platform_io_init_oled_ports(void)
{
    PORT->Group[VOLED_1V2_EN_GROUP].DIRSET.reg = VOLED_1V2_EN_MASK;                                     // OLED HV enable from 1V2, OUTPUT low by default
    PORT->Group[VOLED_1V2_EN_GROUP].OUTCLR.reg = VOLED_1V2_EN_MASK;                                     // OLED HV enable from 1V2, OUTPUT low by default
    PORT->Group[VOLED_3V3_EN_GROUP].DIRSET.reg = VOLED_3V3_EN_MASK;                                     // OLED HV enable from 3V3, OUTPUT low by default
    PORT->Group[VOLED_3V3_EN_GROUP].OUTCLR.reg = VOLED_3V3_EN_MASK;                                     // OLED HV enable from 3V3, OUTPUT low by default
    PORT->Group[OLED_nRESET_GROUP].DIRSET.reg = OLED_nRESET_MASK;                                       // OLED nRESET, OUTPUT
    PORT->Group[OLED_nRESET_GROUP].OUTCLR.reg = OLED_nRESET_MASK;                                       // OLED nRESET, asserted
    PORT->Group[OLED_nCS_GROUP].DIRSET.reg = OLED_nCS_MASK;                                             // OLED nCS, OUTPUT high by default
    PORT->Group[OLED_nCS_GROUP].OUTSET.reg = OLED_nCS_MASK;                                             // OLED nCS, OUTPUT high by default
    PORT->Group[OLED_CD_GROUP].DIRSET.reg = OLED_CD_MASK;                                               // OLED CD, OUTPUT high by default
    PORT->Group[OLED_CD_GROUP].OUTSET.reg = OLED_CD_MASK;                                               // OLED CD, OUTPUT high by default
    PORT->Group[OLED_SCK_GROUP].DIRSET.reg = OLED_SCK_MASK;                                             // OLED SCK, OUTPUT
    PORT->Group[OLED_SCK_GROUP].PINCFG[OLED_SCK_PINID].bit.PMUXEN = 1;                                  // Enable peripheral multiplexer
    PORT->Group[OLED_SCK_GROUP].PMUX[OLED_SCK_PINID/2].bit.OLED_SCK_PMUXREGID = OLED_SCK_PMUX_ID;       // OLED SCK, OUTPUT
    PORT->Group[OLED_MOSI_GROUP].DIRSET.reg = OLED_MOSI_MASK;                                           // OLED MOSI, OUTPUT
    PORT->Group[OLED_MOSI_GROUP].PINCFG[OLED_MOSI_PINID].bit.PMUXEN = 1;                                // Enable peripheral multiplexer
    PORT->Group[OLED_MOSI_GROUP].PMUX[OLED_MOSI_PINID/2].bit.OLED_MOSI_PMUXREGID = OLED_MOSI_PMUX_ID;   // OLED MOSI, OUTPUT
    PM->APBCMASK.bit.OLED_APB_SERCOM_BIT = 1;                                                           // Enable SERCOM APB Clock Enable
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, OLED_GCLK_SERCOM_ID);                              // Map 48MHz to SERCOM unit
    sercom_spi_init(OLED_SERCOM, OLED_BAUD_DIVIDER, SPI_MODE0, SPI_HSS_DISABLE, OLED_MISO_PAD, OLED_MOSI_SCK_PADS, FALSE);
}

/*! \fn     platform_io_power_up_oled(BOOL power_1v2)
*   \brief  OLED powerup routine (3V3, 12V, reset release)
*   \param  power_1v2   TRUE to use 1V2 as source for 12V stepup
*/
void platform_io_power_up_oled(BOOL power_1v2)
{
    /* 3V3 is already there when arriving here, just enable the 12V */
    if (power_1v2 != FALSE)
    {
        PORT->Group[VOLED_1V2_EN_GROUP].OUTSET.reg = VOLED_1V2_EN_MASK; 
    }
    else
    {
        PORT->Group[VOLED_3V3_EN_GROUP].OUTSET.reg = VOLED_3V3_EN_MASK;
    }
    
    /* Depending on battery voltage, it may take a while for the 12v to rise */
    timer_delay_ms(20);
    
    /* Release reset */
    PORT->Group[OLED_nRESET_GROUP].OUTSET.reg = OLED_nRESET_MASK;
    
    /* Datasheet mentions a 2us reset time */
    timer_delay_ms(1);
}

/*! \fn     platform_io_power_down_oled(void)
*   \brief  OLED power down routine (12v off, reset on)
*/
void platform_io_power_down_oled(void)
{
    PORT->Group[VOLED_1V2_EN_GROUP].OUTCLR.reg = VOLED_1V2_EN_MASK;
    PORT->Group[VOLED_3V3_EN_GROUP].OUTCLR.reg = VOLED_3V3_EN_MASK;
    PORT->Group[OLED_nRESET_GROUP].OUTSET.reg = OLED_nRESET_MASK; 
}

/*! \fn     platform_io_init_power_ports(void)
*   \brief  Initialize the platform power ports
*/
void platform_io_init_power_ports(void)
{
    /* Configure analog input */
#if defined(PLAT_V2_SETUP)
    PORT->Group[VOLED_VIN_GROUP].DIRCLR.reg = VOLED_VIN_MASK;
    PORT->Group[VOLED_VIN_GROUP].PINCFG[VOLED_VIN_PINID].bit.PMUXEN = 1;
    PORT->Group[VOLED_VIN_GROUP].PMUX[VOLED_VIN_PINID/2].bit.VOLED_VIN_PMUXREGID = VOLED_VIN_PMUX_ID;
#endif
}

/*! \fn     platform_io_init_aux_comms_ports(void)
*   \brief  Initialize the ports used for communication with aux MCU
*/
void platform_io_init_aux_comms(void)
{
    /* Port init */
    PORT->Group[AUX_MCU_RX_GROUP].DIRSET.reg = AUX_MCU_RX_MASK;                                             // AUX MCU RX, MAIN MCU TX
    PORT->Group[AUX_MCU_RX_GROUP].PINCFG[AUX_MCU_RX_PINID].bit.PMUXEN = 1;                                  // Enable peripheral multiplexer
    PORT->Group[AUX_MCU_RX_GROUP].PMUX[AUX_MCU_RX_PINID/2].bit.AUX_MCU_RX_PMUXREGID = AUX_MCU_RX_PMUX_ID;   // AUX MCU RX, MAIN MCU TX
    PORT->Group[AUX_MCU_TX_GROUP].DIRCLR.reg = AUX_MCU_TX_MASK;                                             // AUX MCU TX, MAIN MCU RX
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PMUXEN = 1;                                  // Enable peripheral multiplexer
    PORT->Group[AUX_MCU_TX_GROUP].PMUX[AUX_MCU_TX_PINID/2].bit.AUX_MCU_TX_PMUXREGID = AUX_MCU_TX_PMUX_ID;   // AUX MCU TX, MAIN MCU RX
    PM->APBCMASK.bit.AUXMCU_APB_SERCOM_BIT = 1;                                                             // Enable SERCOM APB Clock Enable
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, AUXMCU_GCLK_SERCOM_ID);                                // Map 48MHz to SERCOM unit
    
    /* Sercom init */
    /* MSB first, USART frame, async, 8x oversampling, internal clock */
    SERCOM_USART_CTRLA_Type temp_ctrla_reg;
    temp_ctrla_reg.reg = 0;
    temp_ctrla_reg.bit.SAMPR = 2;
    temp_ctrla_reg.bit.RXPO = AUXMCU_TX_PAD;
    temp_ctrla_reg.bit.TXPO = AUXMCU_RX_TXPO;
    temp_ctrla_reg.bit.MODE = SERCOM_USART_CTRLA_MODE_USART_INT_CLK_Val;
    AUXMCU_SERCOM->USART.CTRLA = temp_ctrla_reg;
    /* TX & RX en, 8bits */
    SERCOM_USART_CTRLB_Type temp_ctrlb_reg;
    temp_ctrlb_reg.reg = 0;
    temp_ctrlb_reg.bit.RXEN = 1;
    temp_ctrlb_reg.bit.TXEN = 1;   
    while ((AUXMCU_SERCOM->USART.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB) != 0);
    AUXMCU_SERCOM->USART.CTRLB = temp_ctrlb_reg;
    /* Set max bit rate of 3MHz */
    AUXMCU_SERCOM->USART.BAUD.reg = 0;
    /* Enable sercom */
    temp_ctrla_reg.reg |= SERCOM_USART_CTRLA_ENABLE;
    while ((AUXMCU_SERCOM->USART.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE) != 0);
    AUXMCU_SERCOM->USART.CTRLA = temp_ctrla_reg;    
}

/*! \fn     platform_io_init_ports(void)
*   \brief  Initialize the platform IO ports
*/
void platform_io_init_ports(void)
{    
    /* When an external interrupt is configured for level detection GCLK_EIC is not required */
    while ((EIC->STATUS.reg & EIC_STATUS_SYNCBUSY) != 0);
    /* Enable External Interrupt Controller */
    EIC->CTRL.reg = EIC_CTRL_ENABLE;
    
    /* Power */
    platform_io_init_power_ports();
    
    /* Scrollwheel */
    platform_io_init_scroll_wheel_ports();
    
    /* OLED display */
    platform_io_init_oled_ports();
    
    /* External Flash */
    platform_io_init_flash_ports();
    
    /* Accelerometer */
    platform_io_init_accelerometer_ports();

    /* AUX MCU, reset by default */
    PORT->Group[MCU_AUX_RST_EN_GROUP].DIRSET.reg = MCU_AUX_RST_EN_MASK;
    PORT->Group[MCU_AUX_RST_EN_GROUP].OUTSET.reg = MCU_AUX_RST_EN_MASK;
    platform_io_release_aux_reset();
    
    /* Aux comms */
    platform_io_init_aux_comms();

    /* BLE enable, disabled by default */
    PORT->Group[BLE_EN_GROUP].DIRSET.reg = BLE_EN_MASK;
    PORT->Group[BLE_EN_GROUP].OUTCLR.reg = BLE_EN_MASK;
    platform_io_enable_ble();

    /* Smartcards port */
    platform_io_init_smc_ports();
}
