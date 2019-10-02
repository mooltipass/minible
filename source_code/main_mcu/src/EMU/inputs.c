#include "inputs.h"
wheel_action_ret_te inputs_get_wheel_action(BOOL wait_for_action, BOOL ignore_incdec) { return WHEEL_ACTION_NONE; }
RET_TYPE inputs_get_last_returned_action(void){ return RETURN_OK; }
det_ret_type_te inputs_is_wheel_clicked(void) { return RETURN_REL; }
int16_t inputs_get_wheel_increment(void) { return 0; }
BOOL inputs_raw_is_wheel_released(void) { return TRUE; }
void inputs_clear_detections(void) {}
void inputs_scan(void) {}

