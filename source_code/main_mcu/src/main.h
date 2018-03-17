/*
 * main.h
 *
 * Created: 17/03/2018 15:32:12
 *  Author: limpkin
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#include "platform_defines.h"
#include "sh1122.h"


/* Global vars for debug */
#if defined(DEBUG_USB_COMMANDS_ENABLED)
    extern sh1122_descriptor_t plat_oled_descriptor;
#endif


#endif /* MAIN_H_ */