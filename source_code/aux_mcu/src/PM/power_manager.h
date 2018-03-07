/*
 * \file    pm.h
 * \author  MBorregoTrujillo
 * \date    07-March-2018
 * \brief   Power Manager
 */

#ifndef POWER_MANAGER_H_
#define POWER_MANAGER_H_

#include <asf.h>
#include "conf_power_manager.h"

/**
 * \fn      Power_manager_init
 * \brief   Enable / Disable peripheral clocks based on configuration
 */
void power_manager_init(void);

#endif /* POWER_MANAGER_H_ */
