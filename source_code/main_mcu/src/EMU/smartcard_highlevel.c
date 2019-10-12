#include "smartcard_highlevel.h"

RET_TYPE smartcard_highlevel_write_to_appzone_and_check(uint16_t addr, uint16_t nb_bits, uint8_t* buffer, uint8_t* temp_buffer){ return RETURN_OK; }
mooltipass_card_detect_return_te smartcard_high_level_mooltipass_card_detected_routine(volatile uint16_t* pin_code){ return RETURN_MOOLTIPASS_4_TRIES_LEFT;}
RET_TYPE smartcard_highlevel_check_hidden_aes_key_contents(void){return RETURN_OK; }
RET_TYPE smartcard_highlevel_check_security_mode2(void){return RETURN_OK; }
RET_TYPE smartcard_highlevel_check_authenticated_readwrite_to_zone12(void){return RETURN_OK; }
RET_TYPE smartcard_highlevel_write_card_password(uint8_t* buffer){return RETURN_OK; }
RET_TYPE smartcard_highlevel_check_authenticated_readwrite_to_zone1(void){return RETURN_OK; }
RET_TYPE smartcard_highlevel_check_authenticated_readwrite_to_zone2(void){return RETURN_OK; }
RET_TYPE smartcard_highlevel_write_card_login(uint8_t* buffer){return RETURN_OK; }
RET_TYPE smartcard_highlevel_set_authenticated_readwrite_to_zone1(void){return RETURN_OK; }
RET_TYPE smartcard_highlevel_set_authenticated_readwrite_to_zone2(void){return RETURN_OK; }
void smartcard_highlevel_set_authenticated_readwrite_to_zone1and2(void){}
void smartcard_highlevel_write_security_code(volatile uint16_t* code){}
RET_TYPE smartcard_high_level_transform_blank_card_into_mooltipass(void){return RETURN_OK; }
uint8_t smartcard_highlevel_get_nb_sec_tries_left(void){}
RET_TYPE smartcard_highlevel_write_second_aes_key(uint8_t* buffer){return RETURN_OK; }
RET_TYPE smartcard_highlevel_write_aes_key(uint8_t* buffer){return RETURN_OK; }
uint8_t smartcard_highlevel_get_nb_az2_writes_left(void){}
mooltipass_card_detect_return_te smartcard_highlevel_card_detected_routine(void){return RETURN_MOOLTIPASS_BLANK;}
void printSMCDebugInfoToUSB(void){}
uint16_t smartcard_highlevel_read_security_code(void){}
void smartcard_highlevel_erase_smartcard(void){}
void smartcard_highlevel_reset_blank_card(void){}
void smartcard_highlevel_read_aes_key(uint8_t* buffer){}
void smartcard_highlevel_read_second_aes_key(uint8_t* buffer){}
void smartcard_highlevel_read_application_zone1(uint8_t* buffer){}
void smartcard_highlevel_write_application_zone1(uint8_t* buffer){}
void smartcard_highlevel_read_application_zone2(uint8_t* buffer){}
void smartcard_highlevel_write_application_zone2(uint8_t* buffer){}
void smartcard_highlevel_read_card_login(uint8_t* buffer){}
void smartcard_highlevel_read_card_password(uint8_t* buffer){}
uint8_t* smartcard_highlevel_read_fab_zone(uint8_t* buffer){}
uint8_t* smartcard_highlevel_read_issuer_zone(uint8_t* buffer){}
void smartcard_highlevel_write_issuer_zone(uint8_t* buffer){}
uint8_t* smartcard_highlevel_read_code_attempts_counter(uint8_t* buffer){}
uint8_t* smartcard_highlevel_read_code_protected_zone(uint8_t* buffer){}
void smartcard_highlevel_write_protected_zone(uint8_t* buffer){}
uint8_t* smartcard_highlevel_read_appzone1_erase_key(uint8_t* buffer){}
void smartcard_highlevel_write_appzone1_erase_key(uint8_t* buffer){}
uint8_t* smartcard_highlevel_read_appzone2_erase_key(uint8_t* buffer){}
void smartcard_highlevel_write_appzone2_erase_key(uint8_t* buffer){}
uint8_t* smartcard_highlevel_read_mem_test_zone(uint8_t* buffer){}
void smartcard_highlevel_write_mem_test_zone(uint8_t* buffer){}
uint8_t* smartcard_highlevel_read_manufacturer_zone(uint8_t* buffer){}
void smartcard_highlevel_write_manufacturer_zone(uint8_t* buffer){}
void smartcard_highlevel_write_manufacturer_fuse(void){}
void smartcard_highlevel_write_issuer_fuse(void){}
void smartcard_highlevel_write_ec2en_fuse(void){}
