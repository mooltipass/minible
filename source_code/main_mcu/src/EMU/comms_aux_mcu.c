#include "comms_aux_mcu.h"
RET_TYPE comms_aux_mcu_active_wait(aux_mcu_message_t** rx_message_pt_pt, BOOL do_not_touch_dma_flags, uint16_t expected_packet, BOOL single_try, int16_t expected_event) { return RETURN_OK; }
void comms_aux_mcu_get_empty_packet_ready_to_be_sent(aux_mcu_message_t** message_pt_pt, uint16_t message_type){}
comms_msg_rcvd_te comms_aux_mcu_routine(msg_restrict_type_te answer_restrict_type){ return NO_MSG_RCVD; }
void comms_aux_mcu_deal_with_received_event(aux_mcu_message_t* received_message){}
aux_mcu_message_t* comms_aux_mcu_get_temp_tx_message_object_pt(void){ return 0; }
void comms_aux_mcu_send_simple_command_message(uint16_t command){}
void comms_aux_mcu_hard_comms_reset_with_aux_mcu_reboot(void){}
void comms_aux_mcu_update_device_status_buffer(void){}
void comms_aux_mcu_send_message(BOOL wait_for_send){}
RET_TYPE comms_aux_mcu_send_receive_ping(void){return RETURN_OK; }
void comms_aux_mcu_wait_for_message_sent(void){}
void comms_aux_arm_rx_and_clear_no_comms(void){}

