/*!  \file     inputs.h
*    \brief    Scroll wheel driver
*    Created:  20/02/2018
*    Author:   Mathieu Stephan
*/


#ifndef INPUTS_H_
#define INPUTS_H_

/* Defines */
// How many ms is considered as a long press
#define LONG_PRESS_MS               1000

/* Prototypes */
wheel_action_ret_te inputs_get_wheel_action(BOOL wait_for_action, BOOL ignore_incdec);
RET_TYPE inputs_get_last_returned_action(void);
det_ret_type_te inputs_is_wheel_clicked(void);
int16_t inputs_get_wheel_increment(void);
void inputs_clear_detections(void);
void inputs_scan(void);


#endif /* INPUTS_H_ */