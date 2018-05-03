/**
 * \file    pm.h
 * \author  MBorregoTrujillo
 * \date    07-March-2018
 * \brief   Enable / Disable peripheral clocks based on conf_power_manager.h file
 */
#ifndef POWER_MANAGER_H_
#define POWER_MANAGER_H_

#include <asf.h>
#include "conf_power_manager.h"

void power_manager_init(void);
void power_manager_deinit(void);

#endif /* POWER_MANAGER_H_ */
