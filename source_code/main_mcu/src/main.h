/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2019 Stephan Mathieu
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * main.h
 *
 * Created: 17/03/2018 15:32:12
 *  Author: limpkin
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#include "platform_defines.h"
#include "oled_wrapper.h"
#include "acc_wrapper.h"


/* Prototypes */
void main_create_virtual_wheel_movement(void);
uint32_t main_check_stack_usage(void);
void main_init_stack_tracking(void);
void main_platform_init(void);
void main_standby_sleep(void);
void main_reboot(void);

/* Global vars to access descriptors */
extern accelerometer_descriptor_t plat_acc_descriptor;
extern spi_flash_descriptor_t dataflash_descriptor;
extern spi_flash_descriptor_t dbflash_descriptor;
extern oled_descriptor_t plat_oled_descriptor;

/* Global vars for developer features */
#ifdef SPECIAL_DEVELOPER_CARD_FEATURE
    extern BOOL special_dev_card_inserted;
#endif

#endif /* MAIN_H_ */
