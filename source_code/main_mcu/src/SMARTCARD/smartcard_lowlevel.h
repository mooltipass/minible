/*!  \file     smartcard_lowlevel.h
*    \brief    Low level driver for AT88SC102 smartcard
*    Created:  28/11/2017
*    Author:   Mathieu Stephan
*/
#ifndef SMARTCARD_H_
#define SMARTCARD_H_

#include "platform_defines.h"
#include "defines.h"
#include <stdint.h>

/* Typedefs */
typedef enum    {MAN_FUSE = 0, EC2EN_FUSE = 1, ISSUER_FUSE = 2} card_fuse_type_te;
    
/* Defines */
#define CART_DELAY_FOR_DETECTION    250

// Prototypes
uint8_t* smartcard_lowlevel_read_smc(uint16_t nb_bytes_total_read, uint16_t start_record_index, uint8_t* data_to_receive);
void smartcard_lowlevel_write_smc(uint16_t start_index_bit, uint16_t nb_bits, uint8_t* data_to_write);
pin_check_return_te smartcard_lowlevel_validate_code(volatile uint16_t* code);
void smartcard_lowlevel_erase_application_zone1_nzone2(BOOL zone1_nzone2);
card_detect_return_te smartcard_lowlevel_first_detect_function(void);
void smartcard_lowlevel_blow_fuse(card_fuse_type_te fuse_name);
det_ret_type_te smartcard_lowlevel_is_card_plugged(void);
void smartcard_lowlevel_write_nerase(BOOL is_write);
void smartcard_lowlevel_inverted_clock_pulse(void);
void smartcard_lowlevel_clear_pgmrst_signals(void);
void smartcard_lowlevel_set_pgmrst_signals(void);
void smartcard_lowlevel_hpulse_delay(void);
void smartcard_lowlevel_clock_pulse(void);
void smartcard_lowlevel_detect(void);

// Macros
/*! \fn     smartcard_low_level_is_smc_absent(void)
*   \brief  Function used to check if the smartcard is absent
*   \note   This function should only be used to check if the smartcard is absent. It works because scanSMCDectect reports the
*           smartcard absent when it is not here during only one tick. It also works because the smartcard is always reported
*           released via isCardPlugged
*   \return RETURN_OK if absent
*/
static inline RET_TYPE smartcard_low_level_is_smc_absent(void)
{    
    if ((PORT->Group[SMC_DET_GROUP].IN.reg & SMC_DET_MASK) == 0)
    {
        return RETURN_NOK;
    }
    else
    {
        return RETURN_OK;
    }
}

// Defines
#define SMARTCARD_FABRICATION_ZONE  0x0F0F
#define SMARTCARD_FACTORY_PIN       0xF0F0
#define SMARTCARD_DEFAULT_PIN       0xF0F0
#define SMARTCARD_AZ_BIT_LENGTH     512
#define SMARTCARD_AZ1_BIT_START     176
#define SMARTCARD_AZ1_BIT_RESERVED  16
#define SMARTCARD_MTP_PASS_LENGTH   (SMARTCARD_AZ_BIT_LENGTH - SMARTCARD_AZ1_BIT_RESERVED - AES_KEY_LENGTH)
#define SMARTCARD_MTP_PASS_OFFSET   (SMARTCARD_AZ1_BIT_RESERVED + AES_KEY_LENGTH)
#define SMARTCARD_AZ2_BIT_START     736
#define SMARTCARD_AZ2_BIT_RESERVED  16
#define SMARTCARD_MTP_LOGIN_LENGTH  (SMARTCARD_AZ_BIT_LENGTH - SMARTCARD_AZ2_BIT_RESERVED)
#define SMARTCARD_MTP_LOGIN_OFFSET  SMARTCARD_AZ2_BIT_RESERVED
#define SMARTCARD_CPZ_LENGTH        8
#define SMARTCARD_ISSUER_ZONE_LGTH  8

#endif /* SMARTCARD_H_ */
