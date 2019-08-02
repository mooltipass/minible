/*
 * main.h
 *
 * Created: 22/08/2018 15:32:12
 *  Author: limpkin
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#include "platform_defines.h"


/* Prototypes */
void main_standby_sleep(BOOL startup_run, volatile BOOL* reenable_comms);
void main_set_bootloader_flag(void);
void main_platform_init(void);


#endif /* MAIN_H_ */