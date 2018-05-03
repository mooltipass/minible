/**
 * \file    pm.c
 * \author  MBorregoTrujillo
 * \date    07-March-2018
 * \brief   Power Manager
 */
#include "power_manager.h"

/* Only configure apbcmask right now */
const PM_APBCMASK_Type power_manager_apbcmask_cfg = {
    .bit = {
        .PAC2_    = POWER_MANAGER_PAC2_CFG,
        .EVSYS_   = POWER_MANAGER_EVSYS_CFG,
        .SERCOM0_ = POWER_MANAGER_SERCOM0_CFG,
        .SERCOM1_ = POWER_MANAGER_SERCOM1_CFG,
        .SERCOM2_ = POWER_MANAGER_SERCOM2_CFG,
        .SERCOM3_ = POWER_MANAGER_SERCOM3_CFG,
        .SERCOM4_ = POWER_MANAGER_SERCOM4_CFG,
        .SERCOM5_ = POWER_MANAGER_SERCOM5_CFG,
        .TCC0_ =    POWER_MANAGER_TCC0_CFG,
        .TCC1_ =    POWER_MANAGER_TCC1_CFG,
        .TCC2_ =    POWER_MANAGER_TCC2_CFG,
        .TC3_ =     POWER_MANAGER_TC3_CFG,
        .TC4_ =     POWER_MANAGER_TC4_CFG,
        .TC5_ =     POWER_MANAGER_TC5_CFG,
        .TC6_ =     POWER_MANAGER_TC6_CFG,
        .TC7_ =     POWER_MANAGER_TC7_CFG,
        .ADC_ =     POWER_MANAGER_ADC_CFG,
        .AC_  =     POWER_MANAGER_AC_CFG,
        .DAC_ =     POWER_MANAGER_DAC_CFG,
        .PTC_ =     POWER_MANAGER_PTC_CFG,
        .I2S_ =     POWER_MANAGER_I2S_CFG,
        .AC1_ =     POWER_MANAGER_AC1_CFG,
    }
};

/**
 * \fn      power_manager_init
 * \brief   Enable / Disable peripheral clocks based on configuration
 */
void power_manager_init(void){
    /* APBCMASK */
    PM->APBCMASK = power_manager_apbcmask_cfg;
}

/**
 * \fn      power_manager_deinit
 * \brief   Disable all peripheral clocks
 */
void power_manager_deinit(void){
    /* APBCMASK */
    PM->APBCMASK.reg = 0;
}
