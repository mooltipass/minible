/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2019 Stephan Mathieu
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*!  \file     platform_io.c
*    \brief    Platform IO related functions
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "platform_defines.h"
#include "driver_sercom.h"
#include "driver_clocks.h"
#include "logic_device.h"
#include "driver_timer.h"
#include "platform_io.h"
/* OLED stepup-power source */
oled_stepup_pwr_source_te platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_NONE;
/* Set when a conversion result is ready */
volatile BOOL platform_io_voledin_conv_ready = FALSE;
/* 3v3 detected counter & state */
volatile BOOL platform_io_debounced_3v3_present = FALSE;
volatile uint16_t platform_io_3v3_not_detected_counter = 0;
volatile uint16_t platform_io_3v3_detected_counter = 0;


/*! \fn     EIC_Handler(void)
*   \brief  Routine called for an extint
*/
void EIC_Handler(void)
{
    /* Identify wakeup reason */
    if ((EIC->INTFLAG.reg & (1 << AUX_MCU_NO_COMMS_EXTINT_NUM)) != 0)
    {
        logic_device_set_wakeup_reason(WAKEUP_REASON_AUX_MCU);
    }
    else
    {
        logic_device_set_wakeup_reason(WAKEUP_REASON_OTHER);
    }
    
    /* All the interrupts below are used to wake up the platform from sleep. If we detect any of them, we disable all of them */
    if (((EIC->INTFLAG.reg & (1 << WHEEL_TICKB_EXTINT_NUM)) != 0) || ((EIC->INTFLAG.reg & (1 << WHEEL_CLICK_EXTINT_NUM)) != 0) || ((EIC->INTFLAG.reg & (1 << USB_3V3_EXTINT_NUM)) != 0) || ((EIC->INTFLAG.reg & (1 << AUX_MCU_NO_COMMS_EXTINT_NUM)) != 0) || ((EIC->INTFLAG.reg & (1 << SMC_DET_EXTINT_NUM)) != 0))
    {
        EIC->INTFLAG.reg = (1 << WHEEL_TICKB_EXTINT_NUM);
        EIC->INTENCLR.reg = (1 << WHEEL_TICKB_EXTINT_NUM);
        EIC->INTFLAG.reg = (1 << WHEEL_CLICK_EXTINT_NUM);
        EIC->INTENCLR.reg = (1 << WHEEL_CLICK_EXTINT_NUM);
        EIC->INTFLAG.reg = (1 << USB_3V3_EXTINT_NUM);
        EIC->INTENCLR.reg = (1 << USB_3V3_EXTINT_NUM);
        EIC->INTFLAG.reg = (1 << AUX_MCU_NO_COMMS_EXTINT_NUM);
        EIC->INTENCLR.reg = (1 << AUX_MCU_NO_COMMS_EXTINT_NUM);
        EIC->INTFLAG.reg = (1 << SMC_DET_EXTINT_NUM);
        EIC->INTENCLR.reg = (1 << SMC_DET_EXTINT_NUM);
    }
}

/*! \fn     AUXMCU_SERCOM_HANDLER(void)
*   \brief  Routine called upon USART RX interrupt, to assert no comms
*/
void AUXMCU_SERCOM_HANDLER(void)
{
    platform_io_set_no_comms();
    platform_io_disable_rx_usart_rx_interrupt();
}

/*! \fn     platform_io_scan_3v3(void)
*   \brief  Scan 3v3 presence for debouncing purposes
*/
void platform_io_scan_3v3(void)
{
    if ((PORT->Group[USB_3V3_GROUP].IN.reg & USB_3V3_MASK) == 0)
    {
        platform_io_3v3_detected_counter = 0;
        if (platform_io_3v3_not_detected_counter == 250)
        {
            platform_io_debounced_3v3_present = FALSE;
        }
        if (platform_io_3v3_not_detected_counter != UINT16_MAX)
        {
            platform_io_3v3_not_detected_counter++;
        }
    }
    else
    {
        platform_io_3v3_not_detected_counter = 0;
        if (platform_io_3v3_detected_counter == 250)
        {
            platform_io_debounced_3v3_present = TRUE;
        }
        if (platform_io_3v3_detected_counter != UINT16_MAX)
        {
            platform_io_3v3_detected_counter++;
        }
    }    
}

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
    /* Charge capacitor: pull down for slow ramp down */
    PORT->Group[WHEEL_SW_GROUP].OUTCLR.reg = WHEEL_SW_MASK;
    DELAYMS_8M(10);
    
    /* Charge capacitor: strong pull to ground */
    PORT->Group[WHEEL_SW_GROUP].DIRSET.reg = WHEEL_SW_MASK;
    DELAYMS_8M(10);
    
    /* Disable 3V3 stepup */
    PORT->Group[SWDET_EN_GROUP].OUTCLR.reg = SWDET_EN_MASK;
}

/*! \fn     platform_io_release_aux_reset(void)
*   \brief  Release aux MCU reset
*/
void platform_io_release_aux_reset(void)
{
    #if defined(PLAT_V1_SETUP) || defined(PLAT_V2_SETUP)
    PORT->Group[MCU_AUX_RST_EN_GROUP].OUTCLR.reg = MCU_AUX_RST_EN_MASK;
    #endif     
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

/*! \fn     platform_io_get_voledin_conversion_result(void)
*   \brief  Fetch voled conversion result
*   \return 12 bit conversion result
*/
uint16_t platform_io_get_voledin_conversion_result(void)
{
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
    return ADC->RESULT.reg;
}

/*! \fn     platform_io_get_voledin_conversion_result_and_trigger_conversion(void)
*   \brief  Fetch voled conversion result and trigger new conversion
*   \return 12 bit conversion result
*/
uint16_t platform_io_get_voledin_conversion_result_and_trigger_conversion(void)
{
    /* Rearm watchdog */
    timer_start_timer(TIMER_ADC_WATCHDOG, 60000);
    
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
    platform_io_voledin_conv_ready = FALSE;
    uint16_t return_val = ADC->RESULT.reg;
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
    ADC->SWTRIG.reg = ADC_SWTRIG_FLUSH;
    while ((ADC->SWTRIG.reg & ADC_SWTRIG_FLUSH) != 0);
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
    ADC->SWTRIG.reg = ADC_SWTRIG_START;
    return return_val;
}

/*! \fn     platform_io_get_voledinmv_conversion_result_and_trigger_conversion(void)
*   \brief  Fetch voled conversion result, convert it to mV and trigger new conversion
*   \return mV of battery
*/
uint16_t platform_io_get_voledinmv_conversion_result_and_trigger_conversion(void)
{
    /* Rearm watchdog */
    timer_start_timer(TIMER_ADC_WATCHDOG, 60000);
    
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
    platform_io_voledin_conv_ready = FALSE;
    uint32_t return_val = ADC->RESULT.reg;
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
    ADC->SWTRIG.reg = ADC_SWTRIG_FLUSH;
    while ((ADC->SWTRIG.reg & ADC_SWTRIG_FLUSH) != 0);
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
    ADC->SWTRIG.reg = ADC_SWTRIG_START;
    #ifdef PLAT_V3_SETUP
    return_val = (return_val*103) >> 8;
    #else
    return_val = (return_val*199) >> 9;
    #endif
    return (uint16_t)return_val;
}

/*! \fn     platform_io_init_bat_adc_measurements(void)
*   \brief  Initialize ADC to later perform battery voltage measurements
*   \note   Function called when CPU is at 8MHz (boot), or 48MHz (runtime)
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
    temp_adc_ctrb_reg.bit.PRESCALER = ADC_CTRLB_PRESCALER_DIV64_Val;                            // Set fclk_adc to 48M / 64 = 750kHz (or 125kHz if at 8M)
    ADC->CTRLB = temp_adc_ctrb_reg;                                                             // Write ctrlb
    ADC->AVGCTRL.reg = ADC_AVGCTRL_ADJRES(4) | ADC_AVGCTRL_SAMPLENUM_1024;                      // Average on 1024 samples. Expected time for avg: 750k/7/1024 = 104Hz = 9.6ms (or 58ms if at 8M). Single conversion mode, single ended, 12bit
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);                                       // Wait for sync
    ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXPOS(VBAT_ADC_PIN_MUXPOS) | ADC_INPUTCTRL_MUXNEG_GND;  // 1x gain, one channel set to voled in
    ADC->INTENSET.reg = ADC_INTENSET_RESRDY;                                                    // Enable in result ready interrupt
    NVIC_EnableIRQ(ADC_IRQn);                                                                   // Enable int
    ADC_CALIB_Type calib_register;                                                              // Create our calibration register values
    calib_register.reg = 0x0000;                                                                // Clear it
    calib_register.bit.LINEARITY_CAL = (((*((uint32_t *)ADC_FUSES_LINEARITY_1_ADDR)) & ADC_FUSES_LINEARITY_1_Msk) >> ADC_FUSES_LINEARITY_1_Pos) << 5;   // Fetch & recompose linearity_cal
    calib_register.bit.LINEARITY_CAL |=  ((*((uint32_t *)ADC_FUSES_LINEARITY_0_ADDR)) & ADC_FUSES_LINEARITY_0_Msk) >> ADC_FUSES_LINEARITY_0_Pos;        // Fetch & recompose linearity_cal
    calib_register.bit.BIAS_CAL = ((*((uint32_t *)ADC_FUSES_BIASCAL_ADDR)) & ADC_FUSES_BIASCAL_Msk) >> ADC_FUSES_BIASCAL_Pos;                           // Fetch & recompose bias cal
    ADC->CALIB = calib_register;                                                                // Store calibration values
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);                                       // Wait for sync
    ADC->CTRLA.reg = ADC_CTRLA_ENABLE;                                                          // And enable ADC
}

/*! \fn     platform_io_get_single_bandgap_measurement(void)
*   \brief  Perform single measurement of the bandgap voltage, then resume battery voltage measurement
*   \return ADC value
*   \note   Function takes 62ms
*/
uint16_t platform_io_get_single_bandgap_measurement(void)
{
    /* Wait for end of previous measurement */
    while(platform_io_voledin_conv_ready == FALSE);
    platform_io_voledin_conv_ready = FALSE;
    
    /* Enable routing of the bandgap voltage to the ADC */
    SYSCTRL->VREF.bit.BGOUTEN = 1;
    
    /* If the internal bandgap voltage or temperature sensor input channel is selected, then the Sampling Time Length bit group in the SamplingControl register must be written. */
    ADC->SAMPCTRL.bit.SAMPLEN = 0x3F;
    
    /* Set ADC input to bandgap, averaging is still set to 1024 samples from the battery adc measurements */
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
    ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXPOS(ADC_INPUTCTRL_MUXPOS_BANDGAP_Val) | ADC_INPUTCTRL_MUXNEG_GND;
    
    /* Wait for voltage stabilization */
    DELAYMS(10);
    
    /* Trigger conversion */
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
    ADC->SWTRIG.reg = ADC_SWTRIG_FLUSH;
    while ((ADC->SWTRIG.reg & ADC_SWTRIG_FLUSH) != 0);
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
    ADC->SWTRIG.reg = ADC_SWTRIG_START;
    
    /* Wait for end of measurement */
    while(platform_io_voledin_conv_ready == FALSE);
    platform_io_voledin_conv_ready = FALSE;
    
    /* Get result */
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
    uint16_t return_val = ADC->RESULT.reg;
    
    /* Set ADC input to battery voltage */
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
    ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXPOS(VBAT_ADC_PIN_MUXPOS) | ADC_INPUTCTRL_MUXNEG_GND;
    
    /* Disable routing of the bandgap voltage to the ADC */
    SYSCTRL->VREF.bit.BGOUTEN = 0;
    
    /* Reset SAMPLEN register to its default value */
    ADC->SAMPCTRL.bit.SAMPLEN = 0;
    
    /* Start battery measurement */
    platform_io_get_voledin_conversion_result_and_trigger_conversion();
    
    return return_val;
}

/*! \fn     platform_io_enable_scroll_wheel_wakeup_interrupts(void)
*   \brief  Enable scroll wheel external interrupt to wake up platform (click only)
*/
void platform_io_enable_scroll_wheel_wakeup_interrupts(void)
{
    /* Change wheel pull up to pull downs for power consumption */
    PORT->Group[WHEEL_A_GROUP].OUTCLR.reg = WHEEL_A_MASK;
    PORT->Group[WHEEL_B_GROUP].OUTCLR.reg = WHEEL_B_MASK;
    
    /* Datasheet: Using WAKEUPEN[x]=1 with INTENSET=0 is not recommended */    
    PORT->Group[WHEEL_SW_GROUP].PMUX[WHEEL_SW_PINID/2].bit.WHEEL_SW_PMUXREGID = PORT_PMUX_PMUXO_A_Val;  // Pin mux to EIC
    PORT->Group[WHEEL_SW_GROUP].PINCFG[WHEEL_SW_PINID].bit.PMUXEN = 1;                                  // Enable peripheral multiplexer
    EIC->CONFIG[WHEEL_CLICK_EXTINT_NUM/8].bit.WHEEL_CLICK_EIC_SENSE_REG = EIC_CONFIG_SENSE0_LOW_Val;    // Detect low state
    EIC->INTFLAG.reg = (1 << WHEEL_CLICK_EXTINT_NUM);                                                   // Clear interrupt just in case
    EIC->INTENSET.reg = (1 << WHEEL_CLICK_EXTINT_NUM);                                                  // Enable interrupt from ext pin
    EIC->WAKEUP.reg |= (1 << WHEEL_CLICK_EXTINT_NUM);                                                   // Enable wakeup from ext pin
}

/*! \fn     platform_io_enable_smartcard_interrupt(void)
*   \brief  Enable smartcard insert/remove interrupt
*/
void platform_io_enable_smartcard_interrupt(void)
{
    BOOL is_smc_inserted = FALSE;
    
    /* Check for smc inserted state to later setup correct interrupt */
    if ((PORT->Group[SMC_DET_GROUP].IN.reg & SMC_DET_MASK) == 0)
    {
        is_smc_inserted = TRUE;
    }
    
    /* Datasheet: Using WAKEUPEN[x]=1 with INTENSET=0 is not recommended */
    PORT->Group[SMC_DET_GROUP].PMUX[SMC_DET_PINID/2].bit.SMC_DET_PMUXREGID = PORT_PMUX_PMUXO_A_Val;     // Pin mux to EIC
    PORT->Group[SMC_DET_GROUP].PINCFG[SMC_DET_PINID].bit.PMUXEN = 1;                                    // Enable peripheral multiplexer
    if (is_smc_inserted == FALSE)
    {
        EIC->CONFIG[SMC_DET_EXTINT_NUM/8].bit.SMC_DET_EXTINT_SENSE_REG = EIC_CONFIG_SENSE0_LOW_Val;     // Detect low state
    } 
    else
    {
        EIC->CONFIG[SMC_DET_EXTINT_NUM/8].bit.SMC_DET_EXTINT_SENSE_REG = EIC_CONFIG_SENSE0_HIGH_Val;    // Detect high state
    }
    EIC->INTFLAG.reg = (1 << SMC_DET_EXTINT_NUM);                                                       // Clear interrupt just in case
    EIC->INTENSET.reg = (1 << SMC_DET_EXTINT_NUM);                                                      // Enable interrupt from ext pin
    EIC->WAKEUP.reg |= (1 << SMC_DET_EXTINT_NUM);                                                       // Enable wakeup from ext pin
}

/*! \fn     platform_io_disable_smartcard_interrupt(void)
*   \brief  Disable smartcard insert/remove interrupt
*/
void platform_io_disable_smartcard_interrupt(void)
{
    PORT->Group[SMC_DET_GROUP].PMUX[AUX_MCU_NOCOMMS_PINID/2].bit.SMC_DET_PMUXREGID = EIC_CONFIG_SENSE0_NONE_Val;    // No detection
    PORT->Group[SMC_DET_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.PMUXEN = 0;                                        // Disable peripheral multiplexer
    EIC->WAKEUP.reg &= ~(1 << SMC_DET_EXTINT_NUM);                                                                  // Disable wakeup from ext pin
}

/*! \fn     platform_io_enable_aux_tx_wakeup_interrupt(void)
*   \brief  Enable aux MCU TX interrupt
*   \note   Not used anymore, replaced by bidirectional wakeup on no-comms
*/
void platform_io_enable_aux_tx_wakeup_interrupt(void)
{
    /* Datasheet: Using WAKEUPEN[x]=1 with INTENSET=0 is not recommended */
    PORT->Group[AUX_MCU_TX_GROUP].PMUX[AUX_MCU_TX_PINID/2].bit.AUX_MCU_TX_PMUXREGID = PORT_PMUX_PMUXO_A_Val;    // Pin mux to EIC
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PMUXEN = 1;                                      // Enable peripheral multiplexer
    EIC->CONFIG[AUX_MCU_TX_EXTINT_NUM/8].bit.AUX_MCU_TX_EIC_SENSE_REG = EIC_CONFIG_SENSE0_LOW_Val;              // Detect low state
    EIC->INTFLAG.reg = (1 << AUX_MCU_TX_EXTINT_NUM);                                                            // Clear interrupt just in case
    EIC->INTENSET.reg = (1 << AUX_MCU_TX_EXTINT_NUM);                                                           // Enable interrupt from ext pin
    EIC->WAKEUP.reg |= (1 << AUX_MCU_TX_EXTINT_NUM);                                                            // Enable wakeup from ext pin
}

/*! \fn     platform_io_enable_usb_3v3_wakeup_interrupt(void)
*   \brief  Enable  USB 3V3 external interrupt to wake up platform
*/
void platform_io_enable_usb_3v3_wakeup_interrupt(void)
{
    /* Datasheet: Using WAKEUPEN[x]=1 with INTENSET=0 is not recommended */
    PORT->Group[USB_3V3_GROUP].PMUX[USB_3V3_PINID/2].bit.USB_3V3_PMUXREGID = PORT_PMUX_PMUXO_A_Val;     // Pin mux to EIC
    PORT->Group[USB_3V3_GROUP].PINCFG[USB_3V3_PINID].bit.PMUXEN = 1;                                    // Enable peripheral multiplexer
    EIC->CONFIG[USB_3V3_EXTINT_NUM/8].bit.USB_3V3_EIC_SENSE_REG = EIC_CONFIG_SENSE0_HIGH_Val;           // Detect high state
    EIC->INTFLAG.reg = (1 << USB_3V3_EXTINT_NUM);                                                       // Clear interrupt just in case
    EIC->INTENSET.reg = (1 << USB_3V3_EXTINT_NUM);                                                      // Enable interrupt from ext pin
    EIC->WAKEUP.reg |= (1 << USB_3V3_EXTINT_NUM);    
}

/*! \fn     platform_io_disable_scroll_wheel_wakeup_interrupts(void)
*   \brief  Disable scroll wheel external interrupt to wake up platform
*/
void platform_io_disable_scroll_wheel_wakeup_interrupts(void)
{    
    /* Set wheel encoder pull-ups back */
    PORT->Group[WHEEL_A_GROUP].OUTSET.reg = WHEEL_A_MASK;
    PORT->Group[WHEEL_B_GROUP].OUTSET.reg = WHEEL_B_MASK;
    
    EIC->CONFIG[WHEEL_CLICK_EXTINT_NUM/8].bit.WHEEL_CLICK_EIC_SENSE_REG = EIC_CONFIG_SENSE0_NONE_Val;   // No detection
    PORT->Group[WHEEL_SW_GROUP].PINCFG[WHEEL_SW_PINID].bit.PMUXEN = 0;                                  // Disable peripheral multiplexer
    EIC->WAKEUP.reg &= ~(1 << WHEEL_CLICK_EXTINT_NUM);                                                  // Disable wakeup from ext pin
}

/*! \fn     platform_io_disable_aux_tx_wakeup_interrupt(void)
*   \brief  Disable aux MCU TX interrupt
*   \note   Not used anymore, replaced by bidirectional wakeup on no-comms
*/
void platform_io_disable_aux_tx_wakeup_interrupt(void)
{
    PORT->Group[AUX_MCU_TX_GROUP].PMUX[AUX_MCU_TX_PINID/2].bit.AUX_MCU_TX_PMUXREGID = EIC_CONFIG_SENSE0_NONE_Val;   // No detection
    EIC->WAKEUP.reg &= ~(1 << AUX_MCU_TX_EXTINT_NUM);                                                               // Disable wakeup from ext pin
}

/*! \fn     platform_io_disable_scroll_wheel_ports(void)
*   \brief  Disable scroll wheel ports (but not the click!)
*/
void platform_io_disable_scroll_wheel_ports(void)
{
    PORT->Group[WHEEL_A_GROUP].PINCFG[WHEEL_A_PINID].bit.PULLEN = 0;
    PORT->Group[WHEEL_A_GROUP].PINCFG[WHEEL_A_PINID].bit.INEN = 0;
    PORT->Group[WHEEL_B_GROUP].PINCFG[WHEEL_B_PINID].bit.PULLEN = 0;
    PORT->Group[WHEEL_B_GROUP].PINCFG[WHEEL_B_PINID].bit.INEN = 0;
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

/*! \fn     platform_io_set_wheel_click_pull_down(void)
*   \brief  Setup pull down for wheel click instead of pullup
*/
void platform_io_set_wheel_click_pull_down(void)
{
    PORT->Group[WHEEL_SW_GROUP].OUTCLR.reg = WHEEL_SW_MASK;    
}

/*! \fn     platform_io_set_wheel_click_low(void)
*   \brief  Setup set wheel click input low
*/
void platform_io_set_wheel_click_low(void)
{
    PORT->Group[WHEEL_SW_GROUP].DIRSET.reg = WHEEL_SW_MASK;
    PORT->Group[WHEEL_SW_GROUP].OUTCLR.reg = WHEEL_SW_MASK; 
}

/*! \fn     platform_io_init_smc_ports(void)
*   \brief  Basic initialization of SMC ports at boot
*/
void platform_io_init_smc_ports(void)
{
    PORT->Group[SMC_DET_GROUP].DIRCLR.reg = SMC_DET_MASK;                           // Setup card detection input (with pull-up)
    PORT->Group[SMC_DET_GROUP].PINCFG[SMC_DET_PINID].bit.INEN = 1;                  // Setup card detection input (with pull-up)
    PORT->Group[SMC_DET_GROUP].OUTSET.reg = SMC_DET_MASK;                           // Setup card detection input with pull-up    
    PORT->Group[SMC_DET_GROUP].PINCFG[SMC_DET_PINID].bit.PULLEN = 1;                // Setup card detection input with pull-up
    PORT->Group[SMC_POW_NEN_GROUP].PINCFG[SMC_POW_NEN_PINID].bit.PMUXEN = 0;        // Setup power enable, disabled by default
    PORT->Group[SMC_POW_NEN_GROUP].DIRSET.reg = SMC_POW_NEN_MASK;                   // Setup power enable, disabled by default
    PORT->Group[SMC_POW_NEN_GROUP].OUTSET.reg = SMC_POW_NEN_MASK;                   // Setup power enable, disabled by default
    PORT->Group[SMC_MOSI_GROUP].DIRSET.reg = SMC_MOSI_MASK;                         // MOSI Ouput Low By Default
    PORT->Group[SMC_MOSI_GROUP].OUTCLR.reg = SMC_MOSI_MASK;                         // MOSI Ouput Low By Default
    PM->APBCMASK.bit.SMARTCARD_APB_SERCOM_BIT = 1;                                  // APB Clock Enable
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, SMARTCARD_GCLK_SERCOM_ID);     // Map 48MHz to SERCOM unit
    sercom_spi_init(SMARTCARD_SERCOM, SMARTCARD_BAUD_DIVIDER, SPI_MODE0, SPI_HSS_DISABLE, SMARTCARD_MISO_PAD, SMARTCARD_MOSI_SCK_PADS, TRUE); 
}

/*! \fn     platform_io_use_internal_smc_det_pullup(void)
*   \brief  Use the internal pullup for smartcard detection
*/
void platform_io_use_internal_smc_det_pullup(void)
{
    #if defined(PLAT_V5_SETUP) || defined(PLAT_V6_SETUP) || defined(PLAT_V7_SETUP)
        PORT->Group[SMC_DET_GROUP].PINCFG[SMC_DET_PINID].bit.PULLEN = 1;            // Card removed: use internal "low" impedance pull-up
    #endif    
}

/*! \fn     platform_io_use_external_smc_det_pullup(void)
*   \brief  Use the internal pullup for smartcard detection
*/
void platform_io_use_external_smc_det_pullup(void)
{
    #if defined(PLAT_V5_SETUP) || defined(PLAT_V6_SETUP) || defined(PLAT_V7_SETUP)
        PORT->Group[SMC_DET_GROUP].PINCFG[SMC_DET_PINID].bit.PULLEN = 0;            // Card inserted: rely on external pull-up!
    #endif    
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
    PORT->Group[SMC_MOSI_GROUP].DIRSET.reg = SMC_MOSI_MASK;                         // MOSI Ouput Low By Default
    PORT->Group[SMC_MOSI_GROUP].OUTCLR.reg = SMC_MOSI_MASK;                         // MOSI Ouput Low By Default
    PORT->Group[SMC_SCK_GROUP].DIRCLR.reg = SMC_SCK_MASK;                           // Disable SPI functionality
}

/*! \fn     platform_io_smc_inserted_function(void)
*   \brief  Function called when smartcard is inserted
*/
void platform_io_smc_inserted_function(void)
{
    /* Limit inrush to the smartcard cap: drive PMOSFET with the MCU pulldown then switch to push pull stage */
    PORT->Group[SMC_POW_NEN_GROUP].DIRCLR.reg = SMC_POW_NEN_MASK;
    PORT->Group[SMC_POW_NEN_GROUP].OUTCLR.reg = SMC_POW_NEN_MASK;
    PORT->Group[SMC_POW_NEN_GROUP].PINCFG[SMC_POW_NEN_PINID].bit.PULLEN = 1;
    timer_delay_ms(1);
    PORT->Group[SMC_POW_NEN_GROUP].PINCFG[VOLED_1V2_EN_PINID].bit.PULLEN = 0;
    PORT->Group[SMC_POW_NEN_GROUP].OUTCLR.reg = SMC_POW_NEN_MASK;
    PORT->Group[SMC_POW_NEN_GROUP].DIRSET.reg = SMC_POW_NEN_MASK;
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
    /* PIN MUX on MOSI isn't required: writing to smartcard is always done with big banging */
    //PORT->Group[SMC_MOSI_GROUP].PINCFG[SMC_MOSI_PINID].bit.PMUXEN = 1;              // Enable SPI functionality
    //PORT->Group[SMC_MOSI_GROUP].PMUX[SMC_MOSI_PINID/2].bit.SMC_MOSI_PMUXREGID = SMC_MOSI_PMUX_ID;
    PORT->Group[SMC_MISO_GROUP].PINCFG[SMC_MISO_PINID].bit.PMUXEN = 1;              // Enable SPI functionality
    /****************************************************************************************/
    PORT->Group[SMC_MISO_GROUP].PMUX[SMC_MISO_PINID/2].bit.SMC_MISO_PMUXREGID = SMC_MISO_PMUX_ID;
    PORT->Group[SMC_MISO_GROUP].PINCFG[SMC_MISO_PINID].bit.INEN = 1;                // MISO as input (required when switching to bit banging)
}

/*! \fn     platform_io_smc_switch_to_bb(void)
*   \brief  Switch to bit banging mode for SPI
*/
void platform_io_smc_switch_to_bb(void)
{
    PORT->Group[SMC_SCK_GROUP].PINCFG[SMC_SCK_PINID].bit.PMUXEN = 0;                // Disable SPI functionality
    /* PIN MUX on MOSI isn't required: writing to smartcard is always done with big banging */
    //PORT->Group[SMC_MOSI_GROUP].PINCFG[SMC_MOSI_PINID].bit.PMUXEN = 0;              // Disable SPI functionality
    /****************************************************************************************/
    PORT->Group[SMC_MISO_GROUP].PINCFG[SMC_MISO_PINID].bit.PMUXEN = 0;              // Disable SPI functionality
}

/*! \fn     platform_io_smc_switch_to_spi(void)
*   \brief  Switch to bit banging mode for SPI
*/
void platform_io_smc_switch_to_spi(void)
{
    PORT->Group[SMC_SCK_GROUP].PINCFG[SMC_SCK_PINID].bit.PMUXEN = 1;                // Enable SPI functionality
    /* PIN MUX on MOSI isn't required: writing to smartcard is always done with big banging */
    //PORT->Group[SMC_MOSI_GROUP].PINCFG[SMC_MOSI_PINID].bit.PMUXEN = 1;              // Enable SPI functionality
    /****************************************************************************************/
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
    PORT->Group[ACC_MISO_GROUP].DIRCLR.reg = ACC_MISO_MASK;                                                                 // MISO, INPUT with pull-down
    PORT->Group[ACC_MISO_GROUP].OUTCLR.reg = ACC_MISO_MASK;                                                                 // MISO, INPUT with pull-down
    PORT->Group[ACC_MISO_GROUP].PINCFG[ACC_MISO_PINID].bit.PULLEN = 1;                                                      // MISO, INPUT with pull-down
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
    PORT->Group[DATAFLASH_MISO_GROUP].DIRCLR.reg = DATAFLASH_MISO_MASK;                                                     // DATAFLASH MISO, INPUT with pull-down
    PORT->Group[DATAFLASH_MISO_GROUP].OUTCLR.reg = DATAFLASH_MISO_MASK;                                                     // DATAFLASH MISO, INPUT with pull-down
    PORT->Group[DATAFLASH_MISO_GROUP].PINCFG[DATAFLASH_MISO_PINID].bit.PULLEN = 1;                                          // DATAFLASH MISO, INPUT with pull-down
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
    PORT->Group[DBFLASH_MISO_GROUP].OUTCLR.reg = DBFLASH_MISO_MASK;                                                         // DBFLASH MISO, INPUT with pull-down
    PORT->Group[DBFLASH_MISO_GROUP].PINCFG[DBFLASH_MISO_PINID].bit.PULLEN = 1;                                              // DBFLASH MISO, INPUT with pull-down
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

/*! \fn     platform_io_enable_vbat_to_oled_stepup(void)
*   \brief  Enable Vbat to oled stepup
*/
void platform_io_enable_vbat_to_oled_stepup(void)
{
    PORT->Group[VOLED_1V2_EN_GROUP].OUTSET.reg = VOLED_1V2_EN_MASK; 
    platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_VBAT;
}

/*! \fn     platform_io_disable_vbat_to_oled_stepup(void)
*   \brief  Disable Vbat to oled stepup
*/
void platform_io_disable_vbat_to_oled_stepup(void)
{
    PORT->Group[VOLED_1V2_EN_GROUP].OUTCLR.reg = VOLED_1V2_EN_MASK;
    platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_NONE;
}

/*! \fn     platform_io_enable_3v3_to_oled_stepup(void)
*   \brief  Enable 3V3 to oled stepup
*/
void platform_io_enable_3v3_to_oled_stepup(void)
{
    PORT->Group[VOLED_3V3_EN_GROUP].OUTSET.reg = VOLED_3V3_EN_MASK; 
    platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_3V3;
}

/*! \fn     platform_io_disable_3v3_to_oled_stepup(void)
*   \brief  Disable 3V3 to oled stepup
*/
void platform_io_disable_3v3_to_oled_stepup(void)
{
    PORT->Group[VOLED_3V3_EN_GROUP].OUTCLR.reg = VOLED_3V3_EN_MASK;
    platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_NONE;
}

/*! \fn     platform_io_assert_oled_reset(void)
*   \brief  Assert oled reset
*/
void platform_io_assert_oled_reset(void)
{
    PORT->Group[OLED_nRESET_GROUP].OUTSET.reg = OLED_nRESET_MASK;
    DELAYUS(15);    
}

/*! \fn     platform_io_power_up_oled(BOOL power_3v3)
*   \brief  OLED powerup routine (3V3, 12V, reset release)
*   \param  power_3v3   TRUE to use USB 3V3 as source for 12V stepup
*   \note   1v2 is already present at the stepup input when this function is called at boot
*/
void platform_io_power_up_oled(BOOL power_3v3)
{
    /* Limit inrush to the OLED caps: drive VBAT NMOSFETs with the MCU pullup then switch to push pull stage: for 3.1V measured for 36us */
    PORT->Group[VOLED_1V2_EN_GROUP].DIRCLR.reg = VOLED_1V2_EN_MASK;
    PORT->Group[VOLED_1V2_EN_GROUP].OUTSET.reg = VOLED_1V2_EN_MASK;
    PORT->Group[VOLED_1V2_EN_GROUP].PINCFG[VOLED_1V2_EN_PINID].bit.PULLEN = 1;
    timer_delay_ms(1);
    PORT->Group[VOLED_1V2_EN_GROUP].PINCFG[VOLED_1V2_EN_PINID].bit.PULLEN = 0;
    PORT->Group[VOLED_1V2_EN_GROUP].OUTCLR.reg = VOLED_1V2_EN_MASK;
    PORT->Group[VOLED_1V2_EN_GROUP].DIRSET.reg = VOLED_1V2_EN_MASK;
    
    /* 3V3 is already there when arriving here, just enable the 12V */
    if (power_3v3 == FALSE)
    {
        PORT->Group[VOLED_3V3_EN_GROUP].OUTCLR.reg = VOLED_3V3_EN_MASK;
        PORT->Group[VOLED_1V2_EN_GROUP].OUTSET.reg = VOLED_1V2_EN_MASK; 
        platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_VBAT;
    }
    else
    {
        PORT->Group[VOLED_1V2_EN_GROUP].OUTCLR.reg = VOLED_1V2_EN_MASK;
        PORT->Group[VOLED_3V3_EN_GROUP].OUTSET.reg = VOLED_3V3_EN_MASK;
        platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_3V3;
    }
    
    /* Worst Voled rise time is 30ms when Vbat is at 1.05V */
    timer_delay_ms(30);
    
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
    platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_NONE;
}

/*! \fn     platform_io_get_voled_stepup_pwr_source(void)
*   \brief  Get the oled stepup power source
*   \return The boolean
*/
oled_stepup_pwr_source_te platform_io_get_voled_stepup_pwr_source(void)
{
    return platform_io_oled_stepup_power_source;
}

/*! \fn     platform_io_is_usb_3v3_present_raw(void)
*   \brief  Check if the USB 3V3 is present (not debounced)
*   \return TRUE or FALSE
*/
BOOL platform_io_is_usb_3v3_present_raw(void)
{
    if ((PORT->Group[USB_3V3_GROUP].IN.reg & USB_3V3_MASK) == 0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/*! \fn     platform_io_is_usb_3v3_present(void)
*   \brief  Check if the USB 3V3 is present (debounced)
*   \return TRUE or FALSE
*/
BOOL platform_io_is_usb_3v3_present(void)
{
    return platform_io_debounced_3v3_present;
}

/*! \fn     platform_io_bypass_3v3_detection_debounce(void)
*   \brief  Bypass the 3v3 debouncing
*/
void platform_io_bypass_3v3_detection_debounce(void)
{
    if (platform_io_is_usb_3v3_present_raw() != FALSE)
    {
        platform_io_debounced_3v3_present = TRUE;
    }
}

/*! \fn     platform_io_set_voled_vin_as_pulldown(void)
*   \brief  Setup voled vin as pulldown
*/
void platform_io_set_voled_vin_as_pulldown(void)
{
#if defined(PLAT_V2_SETUP) || defined(PLAT_V3_SETUP) || defined(PLAT_V4_SETUP) || defined(PLAT_V5_SETUP) || defined(PLAT_V6_SETUP) || defined(PLAT_V7_SETUP)
    PORT->Group[VOLED_VIN_GROUP].DIRCLR.reg = VOLED_VIN_MASK;
    PORT->Group[VOLED_VIN_GROUP].OUTCLR.reg = VOLED_VIN_MASK;
    PORT->Group[VOLED_VIN_GROUP].PINCFG[VOLED_VIN_PINID].bit.PMUXEN = 0;
    PORT->Group[VOLED_VIN_GROUP].PINCFG[VOLED_VIN_PINID].bit.PULLEN = 1;
#endif
}


/*! \fn     platform_io_set_voled_vin_as_adc_input(void)
*   \brief  Setup voled vin as adc input
*/
void platform_io_set_voled_vin_as_adc_input(void)
{
#if defined(PLAT_V2_SETUP) || defined(PLAT_V3_SETUP) || defined(PLAT_V4_SETUP) || defined(PLAT_V5_SETUP) || defined(PLAT_V6_SETUP) || defined(PLAT_V7_SETUP)
    PORT->Group[VOLED_VIN_GROUP].DIRCLR.reg = VOLED_VIN_MASK;
    PORT->Group[VOLED_VIN_GROUP].PINCFG[VOLED_VIN_PINID].bit.PULLEN = 0;
    PORT->Group[VOLED_VIN_GROUP].PINCFG[VOLED_VIN_PINID].bit.PMUXEN = 1;
    PORT->Group[VOLED_VIN_GROUP].PMUX[VOLED_VIN_PINID/2].bit.VOLED_VIN_PMUXREGID = VOLED_VIN_PMUX_ID;
#endif
}

/*! \fn     platform_io_init_power_ports(void)
*   \brief  Initialize the platform power ports
*/
void platform_io_init_power_ports(void)
{
    /* Configure analog input */
#if defined(PLAT_V2_SETUP) || defined(PLAT_V3_SETUP) || defined(PLAT_V4_SETUP) || defined(PLAT_V5_SETUP) || defined(PLAT_V6_SETUP) || defined(PLAT_V7_SETUP)
    platform_io_set_voled_vin_as_adc_input();
#endif

    /* USB 3V3 presence */
    PORT->Group[USB_3V3_GROUP].DIRCLR.reg = USB_3V3_MASK;                                                   // Setup USB 3V3 detection input with pull-down
    PORT->Group[USB_3V3_GROUP].OUTCLR.reg = USB_3V3_MASK;                                                   // Setup USB 3V3 detection input with pull-down
    PORT->Group[USB_3V3_GROUP].PINCFG[USB_3V3_PINID].bit.PULLEN = 1;                                        // Setup USB 3V3 detection input with pull-down
    PORT->Group[USB_3V3_GROUP].PINCFG[USB_3V3_PINID].bit.INEN = 1;                                          // Setup USB 3V3 detection input with pull-down
    
    /* OLED stepup ports */
    PORT->Group[OLED_nRESET_GROUP].DIRSET.reg = OLED_nRESET_MASK;                                           // OLED nRESET, OUTPUT
    PORT->Group[OLED_nRESET_GROUP].OUTCLR.reg = OLED_nRESET_MASK;                                           // OLED nRESET, asserted
    PORT->Group[VOLED_1V2_EN_GROUP].DIRSET.reg = VOLED_1V2_EN_MASK;                                         // OLED HV enable from 1V2, OUTPUT low by default
    PORT->Group[VOLED_1V2_EN_GROUP].OUTCLR.reg = VOLED_1V2_EN_MASK;                                         // OLED HV enable from 1V2, OUTPUT low by default
    PORT->Group[VOLED_3V3_EN_GROUP].DIRSET.reg = VOLED_3V3_EN_MASK;                                         // OLED HV enable from 3V3, OUTPUT low by default
    PORT->Group[VOLED_3V3_EN_GROUP].OUTCLR.reg = VOLED_3V3_EN_MASK;                                         // OLED HV enable from 3V3, OUTPUT low by default
}

/*! \fn     platform_io_disable_aux_comms(void)
*   \brief  Disable ports dedicated to aux comms
*/
void platform_io_disable_aux_comms(void)
{
    /* Reduces standby current by 40uA */
    AUXMCU_SERCOM->USART.CTRLA.reg &= ~SERCOM_USART_CTRLA_ENABLE;                                           // Disable USART
    PORT->Group[AUX_MCU_RX_GROUP].OUTSET.reg = AUX_MCU_RX_MASK;                                             // AUX MCU RX, MAIN MCU TX: Pull up
    PORT->Group[AUX_MCU_RX_GROUP].PINCFG[AUX_MCU_RX_PINID].bit.PULLEN = 1;                                  // AUX MCU RX, MAIN MCU TX: Pull up
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PMUXEN = 0;                                  // AUX MCU TX, MAIN MCU RX: Disable peripheral multiplexer
    PORT->Group[AUX_MCU_RX_GROUP].PINCFG[AUX_MCU_RX_PINID].bit.PMUXEN = 0;                                  // AUX MCU RX, MAIN MCU TX: Disable peripheral multiplexer
    platform_io_set_no_comms();                                                                             // No comms during sleep
}

/*! \fn     platform_io_enable_aux_comms(void)
*   \brief  Enable ports dedicated to aux comms
*/
void platform_io_enable_aux_comms(void)
{
    AUXMCU_SERCOM->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;                                            // Enable USART
    PORT->Group[AUX_MCU_TX_GROUP].PMUX[AUX_MCU_TX_PINID/2].bit.AUX_MCU_TX_PMUXREGID = AUX_MCU_TX_PMUX_ID;   // Pin mux to UASRT
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PMUXEN = 1;                                  // AUX MCU TX, MAIN MCU RX: Enable peripheral multiplexer
    PORT->Group[AUX_MCU_RX_GROUP].PINCFG[AUX_MCU_RX_PINID].bit.PMUXEN = 1;                                  // AUX MCU RX, MAIN MCU TX: Enable peripheral multiplexer
    PORT->Group[AUX_MCU_RX_GROUP].PINCFG[AUX_MCU_RX_PINID].bit.PULLEN = 0;                                  // AUX MCU RX, MAIN MCU TX: Pull up disable
}

/*! \fn     platform_io_set_no_comms(void)
*   \brief  Set no comms signal
*/
void platform_io_set_no_comms(void)
{
    /* Platform v3 */
    #if defined(PLAT_V3_SETUP) || defined(PLAT_V4_SETUP) || defined(PLAT_V5_SETUP) || defined(PLAT_V6_SETUP) || defined(PLAT_V7_SETUP)
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].DIRCLR.reg = AUX_MCU_NOCOMMS_MASK;               // NO COMMS as an input as it'll be pulled-up by aux MCU
    #endif
}

/*! \fn     platform_io_clear_no_comms(void)
*   \brief  Clear no comms signal
*/
void platform_io_clear_no_comms(void)
{
    /* Platform v3 */
    #if defined(PLAT_V3_SETUP) || defined(PLAT_V4_SETUP) || defined(PLAT_V5_SETUP) || defined(PLAT_V6_SETUP) || defined(PLAT_V7_SETUP)
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].DIRSET.reg = AUX_MCU_NOCOMMS_MASK;               // NO COMMS as output, driven low
    #endif
}

/*! \fn     platform_io_arm_rx_usart_rx_interrupt(void)
*   \brief  Arm USART RX interrupt, used to assert no comms
*/
void platform_io_arm_rx_usart_rx_interrupt(void)
{
    /* Enable RX interrupt */
    AUXMCU_SERCOM->USART.INTENSET.bit.RXC = 1;
    
    /* Enable interrupt line that may be enabled later */
    NVIC_EnableIRQ(AUXMCU_SERCOM_INTERUPT);
}

/*! \fn     platform_io_disable_rx_usart_rx_interrupt(void)
*   \brief  Disable USART RX interrupt, used to assert no comms
*/
void platform_io_disable_rx_usart_rx_interrupt(void)
{    
    /* Disable RX interrupt */
    AUXMCU_SERCOM->USART.INTENCLR.bit.RXC = 1;
    
    /* Enable interrupt line that may be enabled later */
    NVIC_DisableIRQ(AUXMCU_SERCOM_INTERUPT);
}

/*! \fn     platform_io_init_no_comms_signal(void)
*   \brief  Initialize the aux comms signal, used as wakeup for aux MCU at boot
*/
void platform_io_init_no_comms_signal(void)
{    
    /* Platform v3 */
    #if defined(PLAT_V3_SETUP) || defined(PLAT_V4_SETUP) || defined(PLAT_V5_SETUP) || defined(PLAT_V6_SETUP) || defined(PLAT_V7_SETUP)
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.PMUXEN = 0;    // Setup NO COMMS, enabled by default (pulled-up by aux mcu)
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.INEN = 1;      // Setup NO COMMS, enabled by default (pulled-up by aux mcu)
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].DIRCLR.reg = AUX_MCU_NOCOMMS_MASK;               // Setup NO COMMS, enabled by default (pulled-up by aux mcu)
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].OUTCLR.reg = AUX_MCU_NOCOMMS_MASK;               // We enable no comms by simply changing the DIR register...
    #endif    
}

/*! \fn     platform_io_set_no_comms_as_wakeup_interrupt(void)
*   \brief  Configure no comms signal as interrupt
*/
void platform_io_set_no_comms_as_wakeup_interrupt(void)
{
    /* Platform v3 */
    #if defined(PLAT_V3_SETUP) || defined(PLAT_V4_SETUP) || defined(PLAT_V5_SETUP) || defined(PLAT_V6_SETUP) || defined(PLAT_V7_SETUP)
        /* Datasheet: Using WAKEUPEN[x]=1 with INTENSET=0 is not recommended */
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].PMUX[AUX_MCU_NOCOMMS_PINID/2].bit.AUX_MCU_NOCOMMS_PMUXREGID = PORT_PMUX_PMUXO_A_Val; // Pin mux to EIC
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.PMUXEN = 1;                                        // Enable peripheral multiplexer
        EIC->CONFIG[AUX_MCU_NO_COMMS_EXTINT_NUM/8].bit.AUX_MCU_NO_COMMS_EXTINT_SENSE_REG = EIC_CONFIG_SENSE0_LOW_Val;           // Detect low state
        EIC->INTFLAG.reg = (1 << AUX_MCU_NO_COMMS_EXTINT_NUM);                                                                  // Clear interrupt just in case
        EIC->INTENSET.reg = (1 << AUX_MCU_NO_COMMS_EXTINT_NUM);                                                                 // Enable interrupt from ext pin
        EIC->WAKEUP.reg |= (1 << AUX_MCU_NO_COMMS_EXTINT_NUM);                                                                  // Allow device wakeup
    #endif
}

/*! \fn     platform_io_disable_no_comms_as_wakeup_interrupt(void)
*   \brief  Disable no comms as wakeup interrupt
*/
void platform_io_disable_no_comms_as_wakeup_interrupt(void)
{
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].PMUX[AUX_MCU_NOCOMMS_PINID/2].bit.AUX_MCU_NOCOMMS_PMUXREGID = EIC_CONFIG_SENSE0_NONE_Val;// No detection
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.PMUXEN = 0;                                            // Disable peripheral multiplexer
    EIC->WAKEUP.reg &= ~(1 << AUX_MCU_NO_COMMS_EXTINT_NUM);                                                                     // Disable wakeup from ext pin
}

/*! \fn     platform_io_init_aux_comms_ports(void)
*   \brief  Initialize the ports used for communication with aux MCU
*/
void platform_io_init_aux_comms(void)
{
    /* Port init */
    PORT->Group[AUX_MCU_RX_GROUP].PINCFG[AUX_MCU_RX_PINID].bit.PMUXEN = 1;                                  // Enable peripheral multiplexer
    PORT->Group[AUX_MCU_RX_GROUP].PMUX[AUX_MCU_RX_PINID/2].bit.AUX_MCU_RX_PMUXREGID = AUX_MCU_RX_PMUX_ID;   // AUX MCU RX, MAIN MCU TX
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PMUXEN = 1;                                  // Enable peripheral multiplexer
    PORT->Group[AUX_MCU_TX_GROUP].PMUX[AUX_MCU_TX_PINID/2].bit.AUX_MCU_TX_PMUXREGID = AUX_MCU_TX_PMUX_ID;   // AUX MCU TX, MAIN MCU RX
    PM->APBCMASK.bit.AUXMCU_APB_SERCOM_BIT = 1;                                                             // Enable SERCOM APB Clock Enable
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, AUXMCU_GCLK_SERCOM_ID);                                // Map 48MHz to SERCOM unit
    
    /* Sercom init */
    /* LSB first, USART frame, async, 8x oversampling, internal clock */
    SERCOM_USART_CTRLA_Type temp_ctrla_reg;
    temp_ctrla_reg.reg = 0;
    temp_ctrla_reg.bit.DORD = 1;
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
    /* Set max baud rate */
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
    /* Enable its interrupts */
    NVIC_EnableIRQ(EIC_IRQn);
    
    /* Scrollwheel */
    platform_io_init_scroll_wheel_ports();
    
    /* OLED display */
    platform_io_init_oled_ports();
    
    /* Accelerometer */
    platform_io_init_accelerometer_ports();

    #if defined(PLAT_V1_SETUP) || defined(PLAT_V2_SETUP)
    /* AUX MCU, reset by default */
    PORT->Group[MCU_AUX_RST_EN_GROUP].DIRSET.reg = MCU_AUX_RST_EN_MASK;
    PORT->Group[MCU_AUX_RST_EN_GROUP].OUTSET.reg = MCU_AUX_RST_EN_MASK;
    #endif
    
    /* Aux comms */
    platform_io_init_aux_comms();

    /* BLE enable, disabled by default */
    PORT->Group[BLE_EN_GROUP].DIRSET.reg = BLE_EN_MASK;
    PORT->Group[BLE_EN_GROUP].OUTCLR.reg = BLE_EN_MASK;

    /* Smartcards port */
    platform_io_init_smc_ports();
}

/*! \fn     platform_io_prepare_ports_for_sleep(void)
*   \brief  Prepare the platform ports for sleep
*/
void platform_io_prepare_ports_for_sleep(void)
{        
    /* Disable AUX comms ports */    
    platform_io_disable_aux_comms();
    
    /* Enable USB 3V3 interrupt */
    platform_io_enable_usb_3v3_wakeup_interrupt();
    
    /* Configure no comms as bidirectional wakeup */
    platform_io_set_no_comms_as_wakeup_interrupt();
    
    /* Enable wheel interrupt */
    platform_io_enable_scroll_wheel_wakeup_interrupts();
    
    /* Enable smartcard interrupt */
    platform_io_enable_smartcard_interrupt();
}

/*! \fn     platform_io_prepare_ports_for_sleep_exit(void)
*   \brief  Prepare the platform ports for sleep exit
*/
void platform_io_prepare_ports_for_sleep_exit(void)
{
    /* Disable smartcard interrupt */
    platform_io_disable_smartcard_interrupt();
    
    /* Disable wheel interrupt */
    platform_io_disable_scroll_wheel_wakeup_interrupts();
    
    /* Disable AUX MCU through NO COMMS interrupt */
    platform_io_disable_no_comms_as_wakeup_interrupt();
    
    /* Reconfigure wheel port */
    platform_io_init_scroll_wheel_ports();
    
    /* Enable AUX comms ports */
    platform_io_enable_aux_comms();
}    