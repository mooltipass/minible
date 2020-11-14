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
 * timer.h
 *
 * Created: 19/04/2017 09:38:49
 *  Author: stephan
 */ 


#ifndef DRIVER_TIMER_H_
#define DRIVER_TIMER_H_

#include <asf.h>
#include "defines.h"

/* Structs */
typedef struct
{
    uint32_t timer_val;
    uint32_t flag;
} timerEntry_t;

typedef struct
{
    uint32_t timer_val;
    uint32_t flag;
    BOOL allocated;
} allocatedTimerEntry_t;

/* Typedefs */
typedef RTC_MODE2_CLOCK_Type calendar_t;

/* Enums */
typedef enum {  TIMER_WAITING_FUNCT = 0, 
                TIMER_USER_INTERACTION = 1, 
                TIMER_SCROLLING = 2, 
                TIMER_ANIMATIONS = 3, 
                TIMER_SCREEN = 4, 
                TIMER_CHECK_PASSWORD = 5, 
                TIMER_BATTERY_ANIM = 6, 
                TIMER_HANDED_MODE_CHANGE = 7, 
                TIMER_ADC_WATCHDOG = 8, 
                TIMER_AUX_MCU_PING = 9,
                TOTAL_NUMBER_OF_TIMERS} timer_id_te;
typedef enum {TIMER_EXPIRED = 0, TIMER_RUNNING = 1} timer_flag_te;
    
/* Macros */
#ifdef EMULATOR_BUILD
#include <unistd.h>
#define DELAYUS(us)                 usleep(us)
#define DELAYMS(ms)                 usleep((ms)*1000)
#define DELAYMS_8M(ms)              usleep((ms)*1000)
#else
#define CYCLES_IN_DLYTICKS_FUNC     8
#define US_TO_DLYTICKS(us)          (uint32_t)((CPU_SPEED_HF / 1000000UL) * us / CYCLES_IN_DLYTICKS_FUNC)
#define DELAYTICKS(ticks)           {volatile uint32_t n=ticks; while(n--);}                    //takes 8 cycles
#define DELAYUS(us)                 DELAYTICKS(US_TO_DLYTICKS(us))                              //uses 20bytes
#define DELAYMS(ms)                 DELAYTICKS(US_TO_DLYTICKS(ms*1000))                         //uses 20bytes
#define US_TO_DLYTICKS_8M(us)       (uint32_t)((CPU_SPEED_MF / 1000000UL) * us / CYCLES_IN_DLYTICKS_FUNC)
#define DELAYMS_8M(ms)              DELAYTICKS(US_TO_DLYTICKS_8M(ms*1000))                      //uses 20bytes
#endif

#define IS_LEAP_YEAR(year)  ((((year) % 4 == 0) && ((year) % 100 != 0)) || ((year) % 400 == 0))
#define SEC_IN_HOUR (60 * 60)
#define SEC_IN_DAY (24 * SEC_IN_HOUR)

/* Constants */
//Days Per Month
static uint8_t const driver_timer_dph[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static uint8_t const LEAP_MONTH = 1; //Feburary
static uint16_t const EPOCH_YEAR = 1970;

/* Prototypes */
void driver_timer_set_rtc_timestamp(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second);
timer_flag_te timer_has_allocated_timer_expired(uint16_t uid, BOOL clear);
void timer_start_logoff_timer(uint16_t nb_20mins_ticks_before_lock);
timer_flag_te timer_has_timer_expired(timer_id_te uid, BOOL clear);
void timer_arm_mcu_systick_for_aux_tx_flood_protection(void);
void timer_rearm_allocated_timer(uint16_t uid, uint32_t val);
void timer_start_timer(timer_id_te uid, uint32_t val);
uint64_t driver_timer_get_rtc_timestamp_uint64t(void);
uint32_t driver_timer_get_rtc_timestamp_uint32t(void);
void timer_wait_for_aux_tx_flood_protection(void);
uint16_t timer_get_and_start_timer(uint32_t val);
void timer_deallocate_timer(uint16_t timer_id);
uint32_t timer_get_timer_val(timer_id_te uid);
int16_t timer_get_fine_30mins_s_adjust(void);
BOOL timer_get_mcu_systick(uint32_t* value);
void timer_initialize_timebase(void);
uint32_t timer_get_systick(void);
void timer_delay_ms(uint32_t ms);
void timer_ms_tick(void);

#endif /* TIMER_H_ */
