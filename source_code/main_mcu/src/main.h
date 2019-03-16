/*
 * main.h
 *
 * Created: 17/03/2018 15:32:12
 *  Author: limpkin
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#include "platform_defines.h"
#include "lis2hh12.h"
#include "defines.h"
#include "sh1122.h"


/* Prototypes */
void main_platform_init(void);
void main_standby_sleep(void);

/* Global vars to access descriptors */
extern sh1122_descriptor_t plat_oled_descriptor;
extern accelerometer_descriptor_t plat_acc_descriptor;
extern spi_flash_descriptor_t dataflash_descriptor;
extern spi_flash_descriptor_t dbflash_descriptor;

/* Global vars for developer features */
#ifdef SPECIAL_DEVELOPER_CARD_FEATURE
    extern BOOL special_dev_card_inserted;
#endif

#endif /* MAIN_H_ */