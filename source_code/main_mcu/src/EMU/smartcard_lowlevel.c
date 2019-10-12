#include "smartcard_lowlevel.h"
#include <string.h>

static det_ret_type_te smartcard_status = RETURN_REL;
static uint8_t smc[1568];
static uint8_t fuses[3];

static void emu_smartcard_init(void)
{
    memset(smc, 0xff, sizeof(smc));
}

det_ret_type_te smartcard_lowlevel_is_card_plugged(void)
{
    if(smartcard_status == RETURN_REL) {
        smartcard_status = RETURN_JDETECT;
        emu_smartcard_init();

    } else if(smartcard_status == RETURN_JDETECT) {
        smartcard_status = RETURN_DET;
    }

    return smartcard_status;
}

RET_TYPE smartcard_lowlevel_check_for_const_val_in_smc_array(uint16_t nb_bytes_total_read, uint16_t start_record_index, uint8_t value){ return RETURN_OK; }

/* 178-180 - manufacturer zone */
uint8_t* smartcard_lowlevel_read_smc(uint16_t nb_bytes_total_read, uint16_t start_record_index, uint8_t* data_to_receive) 
{
    /* nb_bytes_total_read name is horribly misleading :( */
    nb_bytes_total_read -= start_record_index;
    memcpy(data_to_receive, smc + start_record_index, nb_bytes_total_read);
    return data_to_receive;
}

void smartcard_lowlevel_write_smc(uint16_t start_index_bit, uint16_t nb_bits, uint8_t* data_to_write)
{
    /* these tests are not bulletproof, but they work with normally behaved accesses */
    if(start_index_bit >= 1424 && start_index_bit <= 1440 && fuses[MAN_FUSE])
        return;

    /* FIXME: doesn't support sub-byte accesses */
    memcpy(smc + start_index_bit/8, data_to_write, nb_bits/8);
}
pin_check_return_te smartcard_lowlevel_validate_code(volatile uint16_t* code){ return RETURN_PIN_OK; }
void smartcard_lowlevel_erase_application_zone1_nzone2(BOOL zone1_nzone2){}

card_detect_return_te smartcard_lowlevel_first_detect_function(void)
{
    return RETURN_CARD_4_TRIES_LEFT; 
}

void smartcard_lowlevel_blow_fuse(card_fuse_type_te fuse_name)
{
    fuses[fuse_name] = 1;
}


RET_TYPE smartcard_low_level_is_smc_absent(void){return RETURN_NOK; }
void smartcard_lowlevel_detect(void){}

