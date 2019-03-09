/*!  \file     logic_device.c
*    \brief    General logic for device
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_DEVICE_H_
#define LOGIC_DEVICE_H_

/* Defines */
// Min & max timeout value for user interactions
#define SETTING_MIN_USER_INTERACTION_TIMEOUT    7
#define SETTING_MAX_USER_INTERACTION_TIMOUT     25

/* Prototypes */
void logic_device_bundle_update_start(BOOL from_debug_messages);
void logic_device_bundle_update_end(BOOL from_debug_messages);
void logic_device_activity_detected(void);

#endif /* LOGIC_DEVICE_H_ */