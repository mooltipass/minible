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
#include "sh1122.h"


/* Prototypes */
void main_platform_init(void);
void main_standby_sleep(void);

/* Global vars for debug */
#if defined(DEBUG_USB_COMMANDS_ENABLED)
    extern sh1122_descriptor_t plat_oled_descriptor;
    extern accelerometer_descriptor_t acc_descriptor;
    extern spi_flash_descriptor_t dataflash_descriptor;
#endif


#endif /* MAIN_H_ */