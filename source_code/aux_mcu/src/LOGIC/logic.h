/*!  \file     logic.h
*    \brief    General logic handling
*    Created:  06/09/2018
*    Author:   Mathieu Stephan
*/



#ifndef LOGIC_H_
#define LOGIC_H_

/* Includes */
#include "comms_main_mcu.h"
#include "logic.h"

/* Prototypes */
BOOL logic_get_and_clear_bluetooth_to_be_enabled(dis_device_information_t** dis_device_info_pt_pt);
void logic_set_bluetooth_to_be_enabled(dis_device_information_t* device_information_pt);
void logic_set_nocomms_unavailable(void);
BOOL logic_is_no_comms_unavailable(void);
void logic_set_ble_disabled(void);
void logic_set_ble_enabled(void);
BOOL logic_is_ble_enabled(void);


#endif /* LOGIC_H_ */