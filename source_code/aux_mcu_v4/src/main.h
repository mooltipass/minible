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
void main_standby_sleep(BOOL startup_run);
uint32_t main_check_stack_usage(void);
void main_init_stack_tracking(void);
void main_set_bootloader_flag(void);
void main_platform_init(void);


#endif /* MAIN_H_ */
