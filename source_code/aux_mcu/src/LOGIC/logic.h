/*!  \file     logic.h
*    \brief    General logic handling
*    Created:  06/09/2018
*    Author:   Mathieu Stephan
*/



#ifndef LOGIC_H_
#define LOGIC_H_

/* Includes */
#include "logic.h"

/* Prototypes */
BOOL logic_get_and_clear_bluetooth_to_be_enabled(uint8_t** mac_address_pt_pt);
void logic_set_bluetooth_to_be_enabled(uint8_t* mac_addr);
void logic_set_nocomms_unavailable(void);
BOOL logic_is_no_comms_unavailable(void);
void logic_set_ble_disabled(void);
void logic_set_ble_enabled(void);
BOOL logic_is_ble_enabled(void);


#endif /* LOGIC_H_ */