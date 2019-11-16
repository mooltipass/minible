/*!  \file     logic_device.c
*    \brief    General logic for device
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_DEVICE_H_
#define LOGIC_DEVICE_H_

#include "defines.h"

/* Defines */
// Min & max timeout value for user interactions
#define SETTING_MIN_USER_INTERACTION_TIMEOUT    7
#define SETTING_DFT_USER_INTERACTION_TIMEOUT    15
#define SETTING_MAX_USER_INTERACTION_TIMOUT     25

/* Prototypes */
ret_type_te logic_device_bundle_update_start(BOOL from_debug_messages);
void logic_device_bundle_update_end(BOOL from_debug_messages);
BOOL logic_device_get_state_changed_and_reset_bool(void);
void logic_device_set_state_changed(void);
void logic_device_activity_detected(void);

#endif /* LOGIC_DEVICE_H_ */