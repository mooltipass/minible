/*!  \file     logic_bluetooth.h
*    \brief    General logic for bluetooth
*    Created:  07/03/2019
*    Author:   Mathieu Stephan
*/ 


#ifndef LOGIC_BLUETOOTH_H_
#define LOGIC_BLUETOOTH_H_

/* Enums */
typedef enum    {BT_STATE_CONNECTED = 0, BT_STATE_OFF, BT_STATE_ON} bt_state_te;

/* Prototypes */
bt_state_te logic_bluetooth_get_state(void);


#endif /* LOGIC_BLUETOOTH_H_ */