/*!  \file     smartcard_highlevel.h
*    \brief    High level driver for AT88SC102 smartcard
*    Created:  29/11/2017
*    Author:   Mathieu Stephan
*/
#ifndef SMART_CARD_HIGHER_LEVEL_FUNCTIONS_H_
#define SMART_CARD_HIGHER_LEVEL_FUNCTIONS_H_

#include <stdlib.h>
#include "smartcard_lowlevel.h"
#include "defines.h"

#ifdef DEBUG_SMC_DUMP_USB_PRINT
    #define printSmartCardInfo() printSMCDebugInfoToUSB()
#else
    #define printSmartCardInfo()
#endif


/************ PROTOTYPES ************/
RET_TYPE smartcard_highlevel_write_to_appzone_and_check(uint16_t addr, uint16_t nb_bits, uint8_t* buffer, uint8_t* temp_buffer);
mooltipass_card_detect_return_te smartcard_high_level_mooltipass_card_detected_routine(volatile uint16_t* pin_code);
RET_TYPE smartcard_highlevel_check_security_mode2(void);
RET_TYPE smartcard_highlevel_check_authenticated_readwrite_to_zone12(void);
RET_TYPE smartcard_highlevel_write_card_password(uint8_t* buffer);
RET_TYPE smartcard_highlevel_check_authenticated_readwrite_to_zone1(void);
RET_TYPE smartcard_highlevel_check_authenticated_readwrite_to_zone2(void);
RET_TYPE smartcard_highlevel_write_card_login(uint8_t* buffer);
RET_TYPE smartcard_highlevel_set_authenticated_readwrite_to_zone1(void);
RET_TYPE smartcard_highlevel_set_authenticated_readwrite_to_zone2(void);
void smartcard_highlevel_set_authenticated_readwrite_to_zone1and2(void);
void smartcard_highlevel_write_security_code(volatile uint16_t* code);
RET_TYPE smartcard_high_level_transform_blank_card_into_mooltipass(void);
uint8_t smartcard_highlevel_get_nb_sec_tries_left(void);
RET_TYPE smartcard_highlevel_write_aes_key(uint8_t* buffer);
uint8_t smartcard_highlevel_get_nb_az2_writes_left(void);
mooltipass_card_detect_return_te smartcard_highlevel_card_detected_routine(void);
void printSMCDebugInfoToUSB(void);
uint16_t smartcard_highlevel_read_security_code(void);
void smartcard_highlevel_erase_smartcard(void);
void smartcard_highlevel_reset_blank_card(void);
void smartcard_highlevel_read_aes_key(uint8_t* buffer);
void smartcard_highlevel_read_application_zone1(uint8_t* buffer);
void smartcard_highlevel_write_application_zone1(uint8_t* buffer);
void smartcard_highlevel_read_application_zone2(uint8_t* buffer);
void smartcard_highlevel_write_application_zone2(uint8_t* buffer);
void smartcard_highlevel_read_card_login(uint8_t* buffer);
void smartcard_highlevel_read_card_password(uint8_t* buffer);
uint8_t* smartcard_highlevel_read_fab_zone(uint8_t* buffer);
uint8_t* smartcard_highlevel_read_issuer_zone(uint8_t* buffer);
void smartcard_highlevel_write_issuer_zone(uint8_t* buffer);
uint8_t* smartcard_highlevel_read_code_attempts_counter(uint8_t* buffer);
uint8_t* smartcard_highlevel_read_code_protected_zone(uint8_t* buffer);
void smartcard_highlevel_write_protected_zone(uint8_t* buffer);
uint8_t* smartcard_highlevel_read_appzone1_erase_key(uint8_t* buffer);
void smartcard_highlevel_write_appzone1_erase_key(uint8_t* buffer);
uint8_t* smartcard_highlevel_read_appzone2_erase_key(uint8_t* buffer);
void smartcard_highlevel_write_appzone2_erase_key(uint8_t* buffer);
uint8_t* smartcard_highlevel_read_mem_test_zone(uint8_t* buffer);
void smartcard_highlevel_write_mem_test_zone(uint8_t* buffer);
uint8_t* smartcard_highlevel_read_manufacturer_zone(uint8_t* buffer);
void smartcard_highlevel_write_manufacturer_zone(uint8_t* buffer);
void smartcard_highlevel_write_manufacturer_fuse(void);
void smartcard_highlevel_write_issuer_fuse(void);
void smartcard_highlevel_write_ec2en_fuse(void);

/*
                SMART CARD MEMORY MAP

Bit Address                 Description                 Bits    Words
0–15            Fabrication Zone (FZ)                   16      1
16–79           Issuer Zone (IZ)                        64      4
80–95           Security Code (SC)                      16      1
96–111          Security Code Attempts counter (SCAC)   16      1
112–175         Code Protected Zone (CPZ)               64      4
176–687         Application Zone 1 (AZ1)                512     32
688–735         Application Zone 1 Erase Key (EZ1)      48      3
736–1247        Application Zone 2 (AZ2)                512     32
1248–1279       Application Zone 2 Erase Key (EZ2)      32      2
1280–1407       Application Zone 2 Erase Counter (EC2)  128     8
1408–1423       Memory Test Zone (MTZ)                  16      1
1424–1439       Manufacturer’s Zone (MFZ)               16      1
1440–1455       Block Write/Erase                       16      1
1456–1471       MANUFACTURER’S FUSE                     16      1
1529            EC2EN FUSE (Controls use of EC2)        1
1552 - 1567     ISSUER FUSE                             161

AZ1 composition (bits): 16 reserved + 256 AES key + 240 MTP password
AZ2 composition (bits): 16 reserved + 496 MTP login

*/

#endif /* SMART_CARD_HIGHER_LEVEL_FUNCTIONS_H_ */
