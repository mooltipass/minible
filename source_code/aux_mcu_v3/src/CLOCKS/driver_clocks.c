/*!  \file     driver_clocks.c
*    \brief    Platform clocks related functions
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/
#include "sam.h"
#include "platform_defines.h"
#include "driver_clocks.h"


/*! \fn     clocks_map_gclk_to_peripheral_clock(uint32_t gclk_id, uint32_t peripheral_clk_id)
*   \brief  Connect peripheral clock to a given gclk
*   \param  gclk_id             GENCLK ID
*   \param  peripheral_clk_id   Peripheral clock ID
*/
void clocks_map_gclk_to_peripheral_clock(uint32_t gclk_id, uint32_t peripheral_clk_id)
{
    GCLK_CLKCTRL_Type clkctrl;                                          // Clkctrl struct
    clkctrl.reg = 0;                                                    // Reset temp var
    clkctrl.bit.ID = peripheral_clk_id;                                 // Select TCC0 input
    clkctrl.bit.GEN = gclk_id;                                          // Select gclk
    clkctrl.bit.CLKEN = 1;                                              // Enable generator
    while ((GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) != 0);             // Wait for sync
    GCLK->CLKCTRL = clkctrl;                                            // Write register
}

/*! \fn     clocks_start_48MDFLL(void)
*   \brief  Initialize the 48MHz DFLL in open loop
*/
void clocks_start_48MDFLL(void)
{
    GCLK_GENCTRL_Type genctrl;                                  // Genctrl struct
    
    /* Set 1 wait states for on-board flash: from NVM characteristics */
    NVMCTRL->CTRLB.bit.RWS = 1;
    
    /* Reset GCLK module */
    while ((GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) != 0);     // Wait for sync
    GCLK->CTRL.reg = GCLK_CTRL_SWRST;                           // Trigger reset
    while ((GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) != 0);     // Wait for sync
    while ((GCLK->CTRL.reg & GCLK_CTRL_SWRST) != 0);            // Wait for end of reset
    
    /* Remove osc8m clock division */
    SYSCTRL_OSC8M_Type osc8m_register = SYSCTRL->OSC8M;         // load current osc8m register val
    osc8m_register.bit.FRANGE = SYSCTRL_OSC8M_FRANGE_2_Val;     // select 8mhz oscillation range
    osc8m_register.bit.PRESC = SYSCTRL_OSC8M_PRESC_0_Val;       // set 1 prescaler
    osc8m_register.bit.RUNSTDBY = 0;                            // oscillator not running during standby
    //osc8m_register.bit.ENABLE = 1;                              // enable oscillator (not needed as already enabled)
    SYSCTRL->OSC8M = osc8m_register;                            // write register
    
    /************************************/
    /*  Configure DFLL48M in open loop  */
    /************************************/
    
    /* Disable ONDEMAND mode while writing configurations */
    while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY));    // Wait for sync
    SYSCTRL->DFLLCTRL.reg = 0;                                  // Disable ONDEMAND mode while writing configurations
    
    /* Load coarse and fine values from NVM and store them in DFFLVAL.COARSE & DFFLVAL.FINE */
    uint32_t coarse_val = ((*((uint32_t *)FUSES_DFLL48M_COARSE_CAL_ADDR)) & FUSES_DFLL48M_COARSE_CAL_Msk) >> FUSES_DFLL48M_COARSE_CAL_Pos;
    uint32_t fine_val =  ((*((uint32_t *)FUSES_DFLL48M_FINE_CAL_ADDR)) & FUSES_DFLL48M_FINE_CAL_Msk) >> FUSES_DFLL48M_FINE_CAL_Pos;
    SYSCTRL_DFLLVAL_Type dfll_val;                              // DFLL value register
    dfll_val.bit.COARSE = coarse_val;                           // Store coarse value
    dfll_val.bit.FINE = fine_val;                               // Store fine value
    SYSCTRL->DFLLVAL = dfll_val;                                // Store register value, no need for sync
    
    /* Finally, setup DFLL module */
    SYSCTRL_DFLLCTRL_Type dfll_ctrl;                            // DFLL control register
    dfll_ctrl.reg = SYSCTRL_DFLLCTRL_ENABLE;                    // Enable DFLL
    dfll_ctrl.bit.WAITLOCK = 0;                                 // Output clock before locked
    dfll_ctrl.bit.BPLCKC = 0;                                   // Do not bypass coarse lock
    dfll_ctrl.bit.QLDIS = 0;                                    // Enable quick lock
    dfll_ctrl.bit.CCDIS = 0;                                    // Enable chill cycle
    dfll_ctrl.bit.ONDEMAND = 0;                                 // Oscillator always on
    dfll_ctrl.bit.RUNSTDBY = 0;                                 // Do not run in standby
    dfll_ctrl.bit.USBCRM = 0;                                   // No USB recovery mode
    dfll_ctrl.bit.STABLE = 0;                                   // FINE calibration tracks changes
    dfll_ctrl.bit.MODE = 0;                                     // Open loop operation
    while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY));    // Wait for sync
    SYSCTRL->DFLLCTRL = dfll_ctrl;                              // Write register
    /* Wait for lock */
    while (SYSCTRL->PCLKSR.reg & (SYSCTRL_PCLKSR_DFLLRDY | SYSCTRL_PCLKSR_DFLLLCKF | SYSCTRL_PCLKSR_DFLLLCKC));
    
    /* Switch to the 48M clock */
    genctrl.bit.ID = GCLK_CLKCTRL_GEN_GCLK0_Val;                // Select gclk0
    genctrl.bit.SRC = GCLK_GENCTRL_SRC_DFLL48M_Val;             // Assign 48M oscillator
    genctrl.bit.GENEN = 1;                                      // Enable generator
    genctrl.bit.DIVSEL = 0;                                     // Divide clock by gendiv.div
    genctrl.bit.OE = 1;                                         // Output clock
    while ((GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) != 0);     // Wait for sync
    GCLK->GENCTRL = genctrl;                                    // Write register
}