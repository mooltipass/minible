/*!  \file     platform_io.c
*    \brief    Platform IO related functions
*    Created:  22/08/2018
*    Author:   Mathieu Stephan
*/
#ifndef BOOTLOADER
    #include <stdarg.h>
    #include <string.h>
    #include <asf.h>
    #include "conf_serialdrv.h"
    #include "driver_timer.h"
    #include "logic_sleep.h"
#else
    #include "sam.h"
#endif
#include "platform_defines.h"
//#include "driver_sercom.h"
#include "driver_clocks.h"
#include "platform_io.h"
/* Set when a conversion result is ready */
volatile BOOL platform_cur_sense_conv_ready = FALSE;
/* Set when we are measuring low current sense */
volatile BOOL platform_io_measuring_lcursense = FALSE;
/* Current measured values for high & low current */
volatile uint16_t platform_io_high_cur_val;
volatile uint16_t platform_io_low_cur_val;
/* For debug purposes: voltage set for stepdown and matching DATA register value */
uint16_t platform_io_stepdown_voltage_set = 0;
uint16_t platform_io_dac_data_register_set = 0;
/* Boolean to know if no comms signal is unavailable */
BOOL platform_io_no_comms_unavailable = FALSE;


/*! \fn     EIC_Handler(void)
*   \brief  Routine called for an extint
*/
void EIC_Handler(void)
{
    /* No comms interrupt: ACK and disable interrupt */
    if ((EIC->INTFLAG.reg & (1 << NOCOMMS_EXTINT_NUM)) != 0)
    {
        EIC->INTFLAG.reg = (1 << NOCOMMS_EXTINT_NUM);
        EIC->INTENCLR.reg = (1 << NOCOMMS_EXTINT_NUM);
    }
    
    /* BLE IN interrupt: ACK and disable interrupt */
    if ((EIC->INTFLAG.reg & (1 << BLE_WAKE_IN_EXTINT_NUM)) != 0)
    {
        EIC->INTFLAG.reg = (1 << BLE_WAKE_IN_EXTINT_NUM);
        EIC->INTENCLR.reg = (1 << BLE_WAKE_IN_EXTINT_NUM);
    }
}

/*! \fn     ADC_Handler(void)
*   \brief  Called once a conversion result is ready
*/
#ifndef BOOTLOADER
void ADC_Handler(void)
{    
    /* Clear interrupt */
    ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;
    
    if (platform_io_measuring_lcursense == FALSE)
    {        
        /* Switch bool */
        platform_io_measuring_lcursense = TRUE;
        
        /* Store value */
        while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
        platform_io_high_cur_val = ADC->RESULT.reg;
        
        /* Set low current sense at adc input */
        while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
        ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXPOS(LCURSENSE_ADC_PIN_MUXPOS) | ADC_INPUTCTRL_MUXNEG_GND;
        
        /* Start conversion */
        while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
        ADC->SWTRIG.reg = ADC_SWTRIG_FLUSH;
        while ((ADC->SWTRIG.reg & ADC_SWTRIG_FLUSH) != 0);
        while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
        ADC->SWTRIG.reg = ADC_SWTRIG_START;
    } 
    else
    {        
        /* Switch bool */
        platform_io_measuring_lcursense = FALSE;
        
        /* Store value */
        while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
        platform_io_low_cur_val = ADC->RESULT.reg;
        
        /* Set high current sense at adc input */
        while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
        ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXPOS(HCURSENSE_ADC_PIN_MUXPOS) | ADC_INPUTCTRL_MUXNEG_GND;
        
        /* Set conv ready bool */
        platform_cur_sense_conv_ready = TRUE;
    }
}
#endif

/*! \fn     platform_io_is_current_sense_conversion_result_ready(void)
*   \brief  Ask if a current sense conversion result is ready
*   \return the bool
*/
BOOL platform_io_is_current_sense_conversion_result_ready(void)
{
    return platform_cur_sense_conv_ready;
}

/*! \fn     platform_io_get_dac_data_register_set(void)
*   \brief  Get the last value set in the DAC data register
*/
uint16_t platform_io_get_dac_data_register_set(void)
{
    return platform_io_dac_data_register_set;
}

/*! \fn     platform_io_get_cursense_conversion_result(BOOL trigger_conversion)
*   \brief  Fetch current sense conversion result and trigger new conversion if asked
*   \param  trigger_conversion: set to TRUE to trigger new conversion
*   \return 32bit value: 16bit MSB is high current sense, 16bit LSB is low current sense
*/
uint32_t platform_io_get_cursense_conversion_result(BOOL trigger_conversion)
{
    /* Reset flag */
    platform_cur_sense_conv_ready = FALSE;
    
    /* Trigger new conversion (mux is already set at the right input in the interrupt */
    if (trigger_conversion != FALSE)
    {
        /* Rearm watchdog */
        #ifndef BOOTLOADER
        timer_start_timer(TIMER_ADC_WATCHDOG, 60000);
        #endif
        
        while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
        ADC->SWTRIG.reg = ADC_SWTRIG_FLUSH;
        while ((ADC->SWTRIG.reg & ADC_SWTRIG_FLUSH) != 0);
        while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);
        ADC->SWTRIG.reg = ADC_SWTRIG_START;
    }
    
    return ((uint32_t)platform_io_high_cur_val << 16) | (uint32_t)platform_io_low_cur_val;
}

/*! \fn     platform_io_enable_charge_mosfets()
*   \brief  Enable charge mosfets
*/
void platform_io_enable_charge_mosfets(void)
{
    PORT->Group[MOS_CHARGE_GROUP].OUTSET.reg = MOS_CHARGE_MASK;    
}

/*! \fn     platform_io_disable_charge_mosfets()
*   \brief  Disable charge mosfets
*/
void platform_io_disable_charge_mosfets(void)
{
    PORT->Group[MOS_CHARGE_GROUP].OUTCLR.reg = MOS_CHARGE_MASK;    
}

/*! \fn     platform_io_enable_step_down(uint16_t voltage)
*   \brief  Enable the platform step down to charge the battery
*   \param  voltage     Voltage in mV that the step-down should output
*/
void platform_io_enable_step_down(uint16_t voltage)
{    
    /* DAC PORT */
    PORT->Group[DAC_GROUP].DIRCLR.reg = DAC_MASK;
    PORT->Group[DAC_GROUP].PINCFG[DAC_PINID].bit.PMUXEN = 1;
    PORT->Group[DAC_GROUP].PMUX[DAC_PINID/2].bit.DAC_PMUXREGID = DAC_PMUX_ID;
    
    /* Setup DAC clock */
    if (PM->APBCMASK.bit.DAC_ == 0)
    {
        PM->APBCMASK.bit.DAC_ = 1;                                                                  // Enable DAC bus clock
        clocks_map_gclk_to_peripheral_clock(GCLK_ID_200K, GCLK_CLKCTRL_ID_DAC_Val);                 // Map 200kHz to DAC unit
    }
    
    /* Setup DAC */
    DAC_CTRLB_Type temp_dac_ctrlb_reg;                                                              // Temp register
    temp_dac_ctrlb_reg.reg = 0;                                                                     // Set to 0
    temp_dac_ctrlb_reg.bit.REFSEL = DAC_CTRLB_REFSEL_INT1V_Val;                                     // 1V reference
    temp_dac_ctrlb_reg.bit.VPD = 1;                                                                 // Voltage pump disabled
    temp_dac_ctrlb_reg.bit.EOEN = 1;                                                                // Drive VOUT pin
    temp_dac_ctrlb_reg.bit.BDWP = 1;                                                                // Bypass DATABUF
    DAC->CTRLB = temp_dac_ctrlb_reg;                                                                // Write register
    platform_io_update_step_down_voltage(voltage);                                                  // Set output voltage
    while ((DAC->STATUS.reg & DAC_STATUS_SYNCBUSY) != 0);                                           // Wait for sync
    DAC->CTRLA.reg = DAC_CTRLA_ENABLE;                                                              // And enable DAC
    PORT->Group[CHARGE_EN_GROUP].OUTSET.reg = CHARGE_EN_MASK;                                       // Enable step-down
}

/*! \fn     platform_io_disable_step_down(void)
*   \brief  Disable step-down
*/
void platform_io_disable_step_down(void)
{
    PORT->Group[DAC_GROUP].PINCFG[DAC_PINID].bit.PMUXEN = 0;                                        // Disable DAC pin mux
    PORT->Group[CHARGE_EN_GROUP].OUTCLR.reg = CHARGE_EN_MASK;                                       // Disable step-down
    while ((DAC->STATUS.reg & DAC_STATUS_SYNCBUSY) != 0);                                           // Wait for sync
    DAC->CTRLA.reg = DAC_CTRLA_SWRST;                                                               // And reset DAC
    while ((DAC->CTRLA.reg & DAC_CTRLA_SWRST) != 0);                                                // Wait for sync
    while ((DAC->STATUS.reg & DAC_STATUS_SYNCBUSY) != 0);                                           // Wait for sync  
}

/*! \fn     platform_io_update_step_down_voltage(uint16_t voltage)
*   \brief  Update step down voltage
*   \param  voltage     Voltage in mV that the step-down should output
*/
void platform_io_update_step_down_voltage(uint16_t voltage)
{
    /* We can't output less than 550mV */
    if (voltage < 550)
    {
        voltage = 550;
    }
    
    uint32_t complicated_math = ((((uint32_t)voltage)*191) >> 9);
    if (complicated_math > 698)
    {
        complicated_math = 698;
    }    
    complicated_math = 698 - complicated_math;                                                      // DAC val from voltage to output
    while ((DAC->STATUS.reg & DAC_STATUS_SYNCBUSY) != 0);                                           // Wait for sync
    DAC->DATA.reg = (uint16_t)complicated_math;                                                     // Write value    
    
    /* Store debug values */
    platform_io_stepdown_voltage_set = voltage;
    platform_io_dac_data_register_set = complicated_math;
}

/*! \fn     platform_io_set_high_cur_sense_as_pull_down(void)
*   \brief  Configure high current sense pin as pull down
*/
void platform_io_set_high_cur_sense_as_pull_down(void)
{
    /* Current sense inputs */
    PORT->Group[HIGH_CUR_SENSE_GROUP].DIRCLR.reg = HIGH_CUR_SENSE_MASK;
    PORT->Group[HIGH_CUR_SENSE_GROUP].OUTCLR.reg = HIGH_CUR_SENSE_MASK;
    PORT->Group[HIGH_CUR_SENSE_GROUP].PINCFG[HIGH_CUR_SENSE_PINID].bit.PMUXEN = 0;
    PORT->Group[HIGH_CUR_SENSE_GROUP].PINCFG[HIGH_CUR_SENSE_PINID].bit.PULLEN = 1;
}

/*! \fn     platform_io_set_high_cur_sense_as_sense(void)
*   \brief  Configure high current sense pin as current sense
*/
void platform_io_set_high_cur_sense_as_sense(void)
{
    /* Current sense inputs */
    PORT->Group[HIGH_CUR_SENSE_GROUP].DIRCLR.reg = HIGH_CUR_SENSE_MASK;
    PORT->Group[HIGH_CUR_SENSE_GROUP].PINCFG[HIGH_CUR_SENSE_PINID].bit.PMUXEN = 1;
    PORT->Group[HIGH_CUR_SENSE_GROUP].PINCFG[HIGH_CUR_SENSE_PINID].bit.PULLEN = 0;
    PORT->Group[HIGH_CUR_SENSE_GROUP].PMUX[HIGH_CUR_SENSE_PINID/2].bit.HIGH_CUR_SENSE_PMUXREGID = HIGH_CUR_SENSE_PMUX_ID;
}

/*! \fn     platform_io_enable_battery_charging_ports(void)
*   \brief  Initialize the ports used for battery charging
*/
void platform_io_enable_battery_charging_ports(void)
{
    /* charge DC/DC enable, low output */
    PORT->Group[CHARGE_EN_GROUP].DIRSET.reg = CHARGE_EN_MASK;
    PORT->Group[CHARGE_EN_GROUP].OUTCLR.reg = CHARGE_EN_MASK;
    
    /* charge mosfet enable, low output */
    PORT->Group[MOS_CHARGE_GROUP].DIRSET.reg = MOS_CHARGE_MASK;
    PORT->Group[MOS_CHARGE_GROUP].OUTCLR.reg = MOS_CHARGE_MASK;
    
    /* Current sense inputs */    
    PORT->Group[HIGH_CUR_SENSE_GROUP].DIRCLR.reg = HIGH_CUR_SENSE_MASK;
    PORT->Group[HIGH_CUR_SENSE_GROUP].PINCFG[HIGH_CUR_SENSE_PINID].bit.PMUXEN = 1;
    PORT->Group[HIGH_CUR_SENSE_GROUP].PMUX[HIGH_CUR_SENSE_PINID/2].bit.HIGH_CUR_SENSE_PMUXREGID = HIGH_CUR_SENSE_PMUX_ID;    
    PORT->Group[LOW_CUR_SENSE_GROUP].DIRCLR.reg = LOW_CUR_SENSE_MASK;
    PORT->Group[LOW_CUR_SENSE_GROUP].PINCFG[LOW_CUR_SENSE_PINID].bit.PMUXEN = 1;
    PORT->Group[LOW_CUR_SENSE_GROUP].PMUX[LOW_CUR_SENSE_PINID/2].bit.LOW_CUR_SENSE_PMUXREGID = LOW_CUR_SENSE_PMUX_ID;

    /* Setup ADC */
    PM->APBCMASK.bit.ADC_ = 1;                                                                      // Enable ADC bus clock
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, GCLK_CLKCTRL_ID_ADC_Val);                      // Map 48MHz to ADC unit
    ADC->REFCTRL.reg = ADC_REFCTRL_REFSEL(ADC_REFCTRL_REFSEL_INTVCC0_Val);                          // Set VCC/1.48 as a reference
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);                                           // Wait for sync
    ADC_CTRLB_Type temp_adc_ctrb_reg;                                                               // Temp register
    temp_adc_ctrb_reg.reg = 0;                                                                      // Set to 0
    temp_adc_ctrb_reg.bit.RESSEL = ADC_CTRLB_RESSEL_16BIT_Val;                                      // Set to 16bit result to allow averaging mode
    temp_adc_ctrb_reg.bit.PRESCALER = ADC_CTRLB_PRESCALER_DIV32_Val;                                // Set fclk_adc to 48M / 32 = 1.5MHz
    ADC->CTRLB = temp_adc_ctrb_reg;                                                                 // Write ctrlb
    ADC->AVGCTRL.reg = ADC_AVGCTRL_ADJRES(4) | ADC_AVGCTRL_SAMPLENUM_1024;                          // Average on 1024 samples. Expected time for avg: 1.5M/(12-1)/1024 = 133Hz = 7.5ms. Single conversion mode, single ended, 12bit
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);                                           // Wait for sync
    ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXPOS(HCURSENSE_ADC_PIN_MUXPOS) | ADC_INPUTCTRL_MUXNEG_GND; // 1x gain, one channel set to high cur sense
    ADC->INTENSET.reg = ADC_INTENSET_RESRDY;                                                        // Enable in result ready interrupt
    NVIC_EnableIRQ(ADC_IRQn);                                                                       // Enable int
    ADC_CALIB_Type calib_register;                                                                  // Create our calibration register values
    calib_register.reg = 0x0000;                                                                    // Clear it
    calib_register.bit.LINEARITY_CAL = (((*((uint32_t *)ADC_FUSES_LINEARITY_1_ADDR)) & ADC_FUSES_LINEARITY_1_Msk) >> ADC_FUSES_LINEARITY_1_Pos) << 5;   // Fetch & recompose linearity_cal
    calib_register.bit.LINEARITY_CAL |=  ((*((uint32_t *)ADC_FUSES_LINEARITY_0_ADDR)) & ADC_FUSES_LINEARITY_0_Msk) >> ADC_FUSES_LINEARITY_0_Pos;        // Fetch & recompose linearity_cal
    calib_register.bit.BIAS_CAL = ((*((uint32_t *)ADC_FUSES_BIASCAL_ADDR)) & ADC_FUSES_BIASCAL_Msk) >> ADC_FUSES_BIASCAL_Pos;                           // Fetch & recompose bias cal
    ADC->CALIB = calib_register;                                                                    // Store calibration values
    while ((ADC->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0);                                           // Wait for sync
    ADC->CTRLA.reg = ADC_CTRLA_ENABLE;                                                              // And enable ADC
}

/*! \fn     platform_io_disable_no_comms_signal(void)
*   \brief  Disable no comms input signal
*/
void platform_io_disable_no_comms_signal(void)
{
    platform_io_no_comms_unavailable = TRUE;
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].DIRCLR.reg = AUX_MCU_NOCOMMS_MASK;
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.INEN = 0;
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.PMUXEN = 0;
}

/*! \fn     platform_io_init_aux_comms_ports(void)
*   \brief  Initialize the ports used for communication with aux MCU
*/
void platform_io_init_aux_comms(void)
{
    /* Port init */
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].DIRCLR.reg = AUX_MCU_NOCOMMS_MASK;                                   // No comms as input
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.INEN = 1;                          // No comms as input
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.PMUXEN = 0;                        // No comms as input
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
    temp_ctrla_reg.bit.RXPO = AUXMCU_RX_TXPO;
    temp_ctrla_reg.bit.TXPO = AUXMCU_TX_PAD;
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

/*! \fn     platform_io_enable_debug_uart(void)
*   \brief  Initialize debug uart at 3Mb/s
*/
void platform_io_enable_debug_uart(void)
{
    /* Port init */
    PORT->Group[DBG_UART_TX_GROUP].PINCFG[DBG_UART_TX_PINID].bit.PMUXEN = 1;                                    // Enable peripheral multiplexer
    PORT->Group[DBG_UART_TX_GROUP].PMUX[DBG_UART_TX_PINID/2].bit.DBG_UART_TX_PMUXREGID = DBG_UART_TX_PMUX_ID;   // Debug uart TX
    PM->APBCMASK.bit.DBG_UART_APB_SERCOM_BIT = 1;                                                               // Enable SERCOM APB Clock Enable
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, DBG_UART_GCLK_SERCOM_ID);                                  // Map 48MHz to SERCOM unit
    
    /* Sercom init */
    /* LSB first, USART frame, async, 16x oversampling, internal clock */
    SERCOM_USART_CTRLA_Type temp_ctrla_reg;
    temp_ctrla_reg.reg = 0;
    temp_ctrla_reg.bit.DORD = 1;
    temp_ctrla_reg.bit.SAMPR = 0;
    temp_ctrla_reg.bit.TXPO = DBG_UART_TX_PAD;
    temp_ctrla_reg.bit.MODE = SERCOM_USART_CTRLA_MODE_USART_INT_CLK_Val;
    DBG_UART_SERCOM->USART.CTRLA = temp_ctrla_reg;
    /* TX & RX en, 8bits */
    SERCOM_USART_CTRLB_Type temp_ctrlb_reg;
    temp_ctrlb_reg.reg = 0;
    temp_ctrlb_reg.bit.RXEN = 0;
    temp_ctrlb_reg.bit.TXEN = 1;
    while ((DBG_UART_SERCOM->USART.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB) != 0);
    DBG_UART_SERCOM->USART.CTRLB = temp_ctrlb_reg;
    /* Set max baud rate */
    DBG_UART_SERCOM->USART.BAUD.reg = 0;
    /* Enable sercom */
    temp_ctrla_reg.reg |= SERCOM_USART_CTRLA_ENABLE;
    while ((DBG_UART_SERCOM->USART.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE) != 0);
    DBG_UART_SERCOM->USART.CTRLA = temp_ctrla_reg;
}

/*! \fn     platform_io_enable_eic(void)
*   \brief  Initialize interrupt controller
*/
void platform_io_enable_eic(void)
{    
    /* When an external interrupt is configured for level detection GCLK_EIC is not required */
    while ((EIC->STATUS.reg & EIC_STATUS_SYNCBUSY) != 0);
    /* Enable External Interrupt Controller */
    EIC->CTRL.reg = EIC_CTRL_ENABLE;
    /* Enable its interrupts */
    NVIC_EnableIRQ(EIC_IRQn);
}

/*! \fn     platform_io_is_no_comms_asserted(void)
*   \brief  check if main MCU asserted no comms
*   \return Assertion status
*/
RET_TYPE platform_io_is_no_comms_asserted(void)
{
    if (platform_io_no_comms_unavailable == FALSE)
    {
        if ((PORT->Group[AUX_MCU_NOCOMMS_GROUP].IN.reg & AUX_MCU_NOCOMMS_MASK) == 0)
        {
            return RETURN_NOK;
        }
        else
        {
            return RETURN_OK;
        }
    } 
    else
    {
        #ifndef BOOTLOADER
        DELAYMS(1000);
        #endif
        return RETURN_NOK;        
    }       
}

/*! \fn     platform_io_init_no_comms_input(void)
*   \brief  Initialize no comms input port
*/
void platform_io_init_no_comms_input(void)
{
    if (platform_io_no_comms_unavailable == FALSE)
    {
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].DIRCLR.reg = AUX_MCU_NOCOMMS_MASK;
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].OUTSET.reg = AUX_MCU_NOCOMMS_MASK;
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.INEN = 1;
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.PULLEN = 1;
    }
}

/*! \fn     platform_io_init_no_comms_pullup_port(void)
*   \brief  Initialize the port dedicated to pulling up the no comms signal
*/
void platform_io_init_no_comms_pullup_port(void)
{
    PORT->Group[AUX_MCU_NOCOMMS_PULLUP_GROUP].DIRCLR.reg = AUX_MCU_NOCOMMS_PULLUP_MASK;
    PORT->Group[AUX_MCU_NOCOMMS_PULLUP_GROUP].OUTSET.reg = AUX_MCU_NOCOMMS_PULLUP_MASK;
    PORT->Group[AUX_MCU_NOCOMMS_PULLUP_GROUP].PINCFG[AUX_MCU_NOCOMMS_PULLUP_PINID].bit.PULLEN = 1;    
}

/*! \fn     platform_io_generate_no_comms_wakeup_pulse(void)
*   \brief  Generate a negative pulse on nocomms io to wakeup main MCU
*/
void platform_io_generate_no_comms_wakeup_pulse(void)
{
    if (platform_io_no_comms_unavailable == FALSE)
    {
        PORT->Group[AUX_MCU_NOCOMMS_PULLUP_GROUP].OUTCLR.reg = AUX_MCU_NOCOMMS_PULLUP_MASK;
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].OUTCLR.reg = AUX_MCU_NOCOMMS_MASK;
        #ifndef BOOTLOADER
        DELAYMS(1);
        #endif
        PORT->Group[AUX_MCU_NOCOMMS_PULLUP_GROUP].OUTSET.reg = AUX_MCU_NOCOMMS_PULLUP_MASK;
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].OUTSET.reg = AUX_MCU_NOCOMMS_MASK;
    }       
}

/*! \fn     platform_io_enable_no_comms_int(void)
*   \brief  Enable no comms interrupt
*/
void platform_io_enable_no_comms_int(void)
{    
    /* Datasheet: Using WAKEUPEN[x]=1 with INTENSET=0 is not recommended */
    if (platform_io_no_comms_unavailable == FALSE)
    {
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].PMUX[AUX_MCU_NOCOMMS_PINID/2].bit.AUX_MCU_NOCOMMS_PMUXREGID = PORT_PMUX_PMUXO_A_Val; // Pin mux to EIC
        PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.PMUXEN = 1;                                        // Enable peripheral multiplexer
        EIC->CONFIG[NOCOMMS_EXTINT_NUM/8].bit.NOCOMMS_EIC_SENSE_REG = EIC_CONFIG_SENSE0_LOW_Val;                                // Detect low state
        EIC->INTFLAG.reg = (1 << NOCOMMS_EXTINT_NUM);                                                                           // Clear interrupt just in case
        EIC->INTENCLR.reg = (1 << NOCOMMS_EXTINT_NUM);                                                                          // Clear interrupt just in case
        EIC->INTENSET.reg = (1 << NOCOMMS_EXTINT_NUM);                                                                          // Enable interrupt from ext pin
        EIC->WAKEUP.reg |= (1 << NOCOMMS_EXTINT_NUM);                                                                           // Enable wakeup from ext pin    
    }    
}

/*! \fn     platform_io_enable_ble_int(void)
*   \brief  Enable bluetooth interrupt
*/
void platform_io_enable_ble_int(void)
{
    /* Datasheet: Using WAKEUPEN[x]=1 with INTENSET=0 is not recommended */
    PORT->Group[BLE_WAKE_IN_GROUP].PMUX[BLE_WAKE_IN_PINID/2].bit.BLE_WAKE_IN_PMUXREGID = PORT_PMUX_PMUXO_A_Val;             // Pin mux to EIC
    PORT->Group[BLE_WAKE_IN_GROUP].PINCFG[BLE_WAKE_IN_PINID].bit.PMUXEN = 1;                                                // Enable peripheral multiplexer
    EIC->CONFIG[BLE_WAKE_IN_EXTINT_NUM/8].bit.BLE_WAKE_IN_EIC_SENSE_REG = EIC_CONFIG_SENSE0_LOW_Val;                        // Detect low state
    EIC->INTFLAG.reg = (1 << BLE_WAKE_IN_EXTINT_NUM);                                                                       // Clear interrupt just in case
    EIC->INTENCLR.reg = (1 << BLE_WAKE_IN_EXTINT_NUM);                                                                      // Clear interrupt just in case
    EIC->INTENSET.reg = (1 << BLE_WAKE_IN_EXTINT_NUM);                                                                      // Enable interrupt from ext pin
    EIC->WAKEUP.reg |= (1 << BLE_WAKE_IN_EXTINT_NUM);                                                                       // Enable wakeup from ext pin
}

/*! \fn     platform_io_disable_no_comms_int(void)
*   \brief  Disable no comms interrupt
*/
void platform_io_disable_no_comms_int(void)
{
    EIC->CONFIG[NOCOMMS_EXTINT_NUM/8].bit.NOCOMMS_EIC_SENSE_REG = EIC_CONFIG_SENSE0_NONE_Val;               // No detection
    PORT->Group[AUX_MCU_NOCOMMS_GROUP].PINCFG[AUX_MCU_NOCOMMS_PINID].bit.PMUXEN = 0;                        // Disable peripheral multiplexer
    EIC->WAKEUP.reg &= ~(1 << NOCOMMS_EXTINT_NUM);                                                          // Disable wakeup from ext pin 
}

/*! \fn     platform_io_disable_ble_int(void)
*   \brief  Disable bluetooth interrupt
*/
void platform_io_disable_ble_int(void)
{
    EIC->CONFIG[BLE_WAKE_IN_EXTINT_NUM/8].bit.BLE_WAKE_IN_EIC_SENSE_REG = EIC_CONFIG_SENSE0_NONE_Val;       // No detection
    PORT->Group[BLE_WAKE_IN_GROUP].PINCFG[BLE_WAKE_IN_PINID].bit.PMUXEN = 0;                                // Disable peripheral multiplexer
    EIC->WAKEUP.reg &= ~(1 << BLE_WAKE_IN_EXTINT_NUM);                                                      // Disable wakeup from ext pin
}

/*! \fn     platform_io_ble_enabled_inits(void)
*   \brief  Port initializations when enabling BLE module
*/
void platform_io_ble_enabled_inits(void)
{
    /* host wake up: input, pull up */
    PORT->Group[BLE_WAKE_IN_GROUP].DIRCLR.reg = BLE_WAKE_IN_MASK;
    PORT->Group[BLE_WAKE_IN_GROUP].OUTSET.reg = BLE_WAKE_IN_MASK;
    PORT->Group[BLE_WAKE_IN_GROUP].PINCFG[BLE_WAKE_IN_PINID].bit.PULLEN = 1;
    PORT->Group[BLE_WAKE_IN_GROUP].PINCFG[BLE_WAKE_IN_PINID].bit.INEN = 1;
    
    /* Wait a little then enable bluetooth module */
    #ifndef BOOTLOADER
        timer_delay_ms(50);
    #endif
    platform_io_assert_ble_enable();
    platform_io_assert_ble_wakeup();
}

/*! \fn     platform_io_is_wakeup_in_pin_low(void)
*   \brief  Check if the wakeup in pin is low, indicating data to be processed
*   \return TRUE if pin low
*/
BOOL platform_io_is_wakeup_in_pin_low(void)
{
    if ((PORT->Group[BLE_WAKE_IN_GROUP].IN.reg & BLE_WAKE_IN_MASK) == 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*! \fn     platform_io_assert_ble_wakeup(void)
*   \brief  Wakeup BLE module
*/
void platform_io_assert_ble_wakeup(void)
{
    PORT->Group[BLE_WAKE_OUT_GROUP].OUTSET.reg = BLE_WAKE_OUT_MASK;    
}

/*! \fn     platform_io_deassert_ble_wakeup(void)
*   \brief  Allow BLE module to sleep between events
*/
void platform_io_deassert_ble_wakeup(void)
{
    PORT->Group[BLE_WAKE_OUT_GROUP].OUTCLR.reg = BLE_WAKE_OUT_MASK;    
}

/*! \fn     platform_io_is_ble_wakeup_output_high(void)
*   \brief  Check if we set the wakeup output high
*   \return TRUE if we set the pin high
*/
BOOL platform_io_is_ble_wakeup_output_high(void)
{
    if ((PORT->Group[BLE_WAKE_OUT_GROUP].OUT.reg & BLE_WAKE_OUT_MASK) == 0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/*! \fn     platform_io_assert_ble_enable(void)
*   \brief  Enable BLE module
*/
void platform_io_assert_ble_enable(void)
{
    PORT->Group[BLE_EN_GROUP].OUTSET.reg = BLE_EN_MASK;
}

/*! \fn     platform_io_deassert_ble_enable(void)
*   \brief  Disable BLE module
*/
void platform_io_deassert_ble_enable(void)
{
    PORT->Group[BLE_EN_GROUP].OUTCLR.reg = BLE_EN_MASK;
}

/*! \fn     platform_io_ble_disabled_actions(void)
*   \brief  IO changes when BLE gets disabled
*/
void platform_io_ble_disabled_actions(void)
{
    /* host wake up: disable IN and pullup */
    PORT->Group[BLE_WAKE_IN_GROUP].PINCFG[BLE_WAKE_IN_PINID].bit.PULLEN = 0;
    PORT->Group[BLE_WAKE_IN_GROUP].PINCFG[BLE_WAKE_IN_PINID].bit.INEN = 0;
}

/*! \fn     platform_io_disable_main_comms(void)
*   \brief  Disable ports dedicated to aux comms
*/
void platform_io_disable_main_comms(void)
{
    /* Reduces standby current by 40uA */
    AUXMCU_SERCOM->USART.CTRLA.reg &= ~SERCOM_USART_CTRLA_ENABLE;                                           // Disable USART
    PORT->Group[AUX_MCU_TX_GROUP].OUTSET.reg = AUX_MCU_TX_MASK;                                             // AUX MCU TX, MAIN MCU RX: Pull up
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PULLEN = 1;                                  // AUX MCU TX, MAIN MCU RX: Pull up
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PMUXEN = 0;                                  // AUX MCU TX, MAIN MCU RX: Disable peripheral multiplexer
    PORT->Group[AUX_MCU_RX_GROUP].PINCFG[AUX_MCU_RX_PINID].bit.PMUXEN = 0;                                  // AUX MCU RX, MAIN MCU TX: Disable peripheral multiplexer
}

/*! \fn     platform_io_enable_main_comms(void)
*   \brief  Enable ports dedicated to aux comms
*/
void platform_io_enable_main_comms(void)
{
    AUXMCU_SERCOM->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;                                            // Enable USART
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PMUXEN = 1;                                  // AUX MCU TX, MAIN MCU RX: Enable peripheral multiplexer
    PORT->Group[AUX_MCU_RX_GROUP].PINCFG[AUX_MCU_RX_PINID].bit.PMUXEN = 1;                                  // AUX MCU RX, MAIN MCU TX: Enable peripheral multiplexer
    PORT->Group[AUX_MCU_TX_GROUP].PINCFG[AUX_MCU_TX_PINID].bit.PULLEN = 0;                                  // AUX MCU RX, MAIN MCU TX: Pull up disable
}

/*! \fn     platform_io_reset_ble_uarts(void)
*   \brief  Reset UARTs used by the BLE module
*/
void platform_io_reset_ble_uarts(void)
{
    #ifndef BOOTLOADER
    /* No need to wait for sync as there's no reason for back to back BLE disable / enable */
    CONF_BLE_USART_MODULE->USART.CTRLA.bit.SWRST = 1;
    CONF_FLCR_BLE_USART_MODULE->USART.CTRLA.bit.SWRST = 1;
    #endif
}

/*! \fn     platform_io_init_ble_ports_for_disabled(void)
*   \brief  Initialize the platform BLE ports for a disabled BTLC1000
*/
void platform_io_init_ble_ports_for_disabled(void)
{
    PORT->Group[BLE_EN_GROUP].DIRSET.reg = BLE_EN_MASK;
    PORT->Group[BLE_EN_GROUP].OUTCLR.reg = BLE_EN_MASK;
    PORT->Group[BLE_UART0_TX_GROUP].DIRSET.reg = BLE_UART0_TX_MASK;
    PORT->Group[BLE_UART0_TX_GROUP].OUTCLR.reg = BLE_UART0_TX_MASK;
    PORT->Group[BLE_UART1_RTS_GROUP].DIRSET.reg = BLE_UART1_RTS_MASK;
    PORT->Group[BLE_UART1_RTS_GROUP].OUTCLR.reg = BLE_UART1_RTS_MASK;
    PORT->Group[BLE_UART0_TX_GROUP].PINCFG[BLE_UART0_TX_PINID].bit.PMUXEN = 0;
    PORT->Group[BLE_UART0_RX_GROUP].PINCFG[BLE_UART0_RX_PINID].bit.PMUXEN = 0;
    PORT->Group[BLE_UART0_RX_GROUP].PINCFG[BLE_UART0_RX_PINID].bit.INEN = 0;
    PORT->Group[BLE_UART1_TX_GROUP].PINCFG[BLE_UART1_TX_PINID].bit.PMUXEN = 0;
    PORT->Group[BLE_UART1_TX_GROUP].PINCFG[BLE_UART1_TX_PINID].bit.PULLEN = 0;
    PORT->Group[BLE_UART1_RX_GROUP].PINCFG[BLE_UART1_RX_PINID].bit.PMUXEN = 0;
    PORT->Group[BLE_UART1_RX_GROUP].PINCFG[BLE_UART1_RX_PINID].bit.INEN = 0;
    PORT->Group[BLE_UART1_RTS_GROUP].PINCFG[BLE_UART1_RTS_PINID].bit.PMUXEN = 0;
    PORT->Group[BLE_UART1_CTS_GROUP].PINCFG[BLE_UART1_CTS_PINID].bit.PMUXEN = 0;
    PORT->Group[BLE_UART1_CTS_GROUP].PINCFG[BLE_UART1_CTS_PINID].bit.PULLEN = 0;
    PORT->Group[BLE_UART1_CTS_GROUP].PINCFG[BLE_UART1_CTS_PINID].bit.INEN = 0;
    
    /* Device wakeup: output, set low */
    PORT->Group[BLE_WAKE_OUT_GROUP].DIRSET.reg = BLE_WAKE_OUT_MASK;
    PORT->Group[BLE_WAKE_OUT_GROUP].OUTCLR.reg = BLE_WAKE_OUT_MASK;
    
    /* Device enable: output, set low */
    PORT->Group[BLE_EN_GROUP].DIRSET.reg = BLE_EN_MASK;
    PORT->Group[BLE_EN_GROUP].OUTCLR.reg = BLE_EN_MASK;
    
    /* Host wakeup: disable pullup */
    PORT->Group[BLE_WAKE_IN_GROUP].PINCFG[BLE_WAKE_IN_PINID].bit.INEN = 0;
    PORT->Group[BLE_WAKE_IN_GROUP].PINCFG[BLE_WAKE_IN_PINID].bit.PULLEN = 0;
    PORT->Group[BLE_WAKE_IN_GROUP].PINCFG[BLE_WAKE_IN_PINID].bit.PMUXEN = 0;
    
    /* Below: leave disabled, doesn't change anything */
    //PORT->Group[BLE_UART1_TX_GROUP].DIRSET.reg = BLE_UART1_TX_MASK;
    //PORT->Group[BLE_UART1_TX_GROUP].OUTSET.reg = BLE_UART1_TX_MASK;
}

/*! \fn     platform_io_init_usb_ports(void)
*   \brief  Initialize the platform USB ports
*/
void platform_io_init_usb_ports(void)
{
    PORT->Group[USB_DM_GROUP].PMUX[USB_DM_PINID/2].bit.USB_DM_PMUXREGID = USB_DM_PMUX_ID;
    PORT->Group[USB_DM_GROUP].PINCFG[USB_DM_PINID].bit.PMUXEN = 1;
    PORT->Group[USB_DP_GROUP].PMUX[USB_DP_PINID/2].bit.USB_DP_PMUXREGID = USB_DP_PMUX_ID;
    PORT->Group[USB_DP_GROUP].PINCFG[USB_DP_PINID].bit.PMUXEN = 1;  
}

/*! \fn     platform_io_disable_usb_ports(void)
*   \brief  Disable the platform USB ports
*/
void platform_io_disable_usb_ports(void)
{
    PORT->Group[USB_DM_GROUP].PINCFG[USB_DM_PINID].bit.PMUXEN = 0;
    PORT->Group[USB_DP_GROUP].PINCFG[USB_DP_PINID].bit.PMUXEN = 0; 
}

/*! \fn     platform_io_init_ports(void)
*   \brief  Initialize the platform IO ports
*/
void platform_io_init_ports(void)
{    
    /* Aux comms */
    platform_io_init_aux_comms();
    
    /* USB comms */
    //platform_io_init_usb_ports();
    
    /* NiMH charging ports */
    platform_io_enable_battery_charging_ports();
    
    /* BLE */
    platform_io_init_ble_ports_for_disabled();
    
    /* Debug UART */
    platform_io_enable_debug_uart();
}    

/*! \fn     platform_io_prepare_ports_for_sleep(void)
*   \brief  Prepare the platform ports for sleep
*/
void platform_io_prepare_ports_for_sleep(void)
{        
}

/*! \fn     platform_io_prepare_ports_for_sleep_exit(void)
*   \brief  Prepare the platform ports for sleep exit
*/
void platform_io_prepare_ports_for_sleep_exit(void)
{        
}   

#ifndef BOOTLOADER
/*! \fn     platform_io_uart_debug_printf(const char *fmt, ...)
*   \brief  Output debug string to USB
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
void platform_io_uart_debug_printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);

    /* Print into our temporary buffer */
    int16_t hypothetical_nb_chars = vsnprintf(buf, sizeof(buf), fmt, ap);
    
    for (int16_t i = 0; i < hypothetical_nb_chars; i++)
    {
        while((DBG_UART_SERCOM->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_DRE) == 0);
        DBG_UART_SERCOM->USART.DATA.reg = buf[i];
    }    
    
    while((DBG_UART_SERCOM->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_DRE) == 0);
    DBG_UART_SERCOM->USART.DATA.reg = '\r';
    while((DBG_UART_SERCOM->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_DRE) == 0);
    DBG_UART_SERCOM->USART.DATA.reg = '\n';
    
    va_end(ap);
}
#pragma GCC diagnostic pop
#endif
