/*
 * logic_bluetooth.h
 *
 * Created: 02/06/2019 18:08:31
 *  Author: limpkin
 */ 


#ifndef LOGIC_BLUETOOTH_H_
#define LOGIC_BLUETOOTH_H_

#include "at_ble_api.h"

/* ATMEL ugliness */
/** @brief Maximum text length */
#define MAX_TEXT_LEN (11)
/** @brief Represent zero position */
#define POSITION_ZERO (0)
/** @brief Represent six position */
#define POSITION_SIX (6)
/** @brief Enable caps */
#define CAPS_ON (2)
/** @brief Disable caps */
#define CAPS_OFF (0)

/* Prototypes */
at_ble_status_t ble_char_changed_app_event(void *param);
void logic_bluetoot_set_battery_level(uint8_t pct);
void logic_bluetooth_start_bluetooth(void);
void logic_bluetooth_routine(void);


#endif /* LOGIC_BLUETOOTH_H_ */