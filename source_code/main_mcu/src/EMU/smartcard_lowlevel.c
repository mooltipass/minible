#include "smartcard_lowlevel.h"
RET_TYPE smartcard_lowlevel_check_for_const_val_in_smc_array(uint16_t nb_bytes_total_read, uint16_t start_record_index, uint8_t value){ return RETURN_OK; }
uint8_t* smartcard_lowlevel_read_smc(uint16_t nb_bytes_total_read, uint16_t start_record_index, uint8_t* data_to_receive){ return 0;}
void smartcard_lowlevel_write_smc(uint16_t start_index_bit, uint16_t nb_bits, uint8_t* data_to_write){}
pin_check_return_te smartcard_lowlevel_validate_code(volatile uint16_t* code){ return RETURN_PIN_OK; }
void smartcard_lowlevel_erase_application_zone1_nzone2(BOOL zone1_nzone2){}
card_detect_return_te smartcard_lowlevel_first_detect_function(void){ return RETURN_CARD_NDET; }
void smartcard_lowlevel_blow_fuse(card_fuse_type_te fuse_name){}
det_ret_type_te smartcard_lowlevel_is_card_plugged(void){ return RETURN_REL;}
void smartcard_lowlevel_write_nerase(BOOL is_write){}
void smartcard_lowlevel_inverted_clock_pulse(void){}
void smartcard_lowlevel_clear_pgmrst_signals(void){}
void smartcard_lowlevel_set_pgmrst_signals(void){}
RET_TYPE smartcard_low_level_is_smc_absent(void){return RETURN_OK; }
void smartcard_lowlevel_hpulse_delay(void){}
void smartcard_lowlevel_clock_pulse(void){}
void smartcard_lowlevel_detect(void){}

