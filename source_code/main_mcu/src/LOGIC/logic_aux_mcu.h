/*!  \file     logic_aux_mcu.h
*    \brief    Aux MCU logic
*    Created:  09/05/2018
*    Author:   Mathieu Stephan
*/

#ifndef LOGIC_AUX_MCU_H_
#define LOGIC_AUX_MCU_H_

#include "defines.h"

/* Prototypes */
void logic_aux_mcu_set_ble_enabled_bool(BOOL ble_enabled);
void logic_aux_mcu_disable_ble(BOOL wait_for_disabled);
void logic_aux_mcu_enable_ble(BOOL wait_for_enabled);
RET_TYPE logic_aux_mcu_flash_firmware_update(void);
uint32_t logic_aux_mcu_get_ble_chip_id(void);
BOOL logic_aux_mcu_is_ble_enabled(void);

#endif /* LOGIC_AUX_MCU_H_ */