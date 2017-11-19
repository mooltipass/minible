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

/* Typedefs */
typedef RTC_MODE2_CLOCK_Type calendar_t;

/* Enums */
typedef enum {TIMER_WAIT_FUNCTS = 0, TIMER_TIMEOUT_FUNCTS = 1, TOTAL_NUMBER_OF_TIMERS} timer_id_te;
typedef enum {TIMER_EXPIRED = 0, TIMER_RUNNING = 1} timer_flag_te;
    
/* Macros */

/* Prototypes */
timer_flag_te timer_has_timer_expired(timer_id_te uid, BOOL clear);
void timer_start_timer(timer_id_te uid, uint32_t val);
void timer_get_calendar(calendar_t* calendar_pt);
uint32_t timer_get_timer_val(timer_id_te uid);
void timer_initialize_timebase(void);
uint32_t timer_get_systick(void);
void timer_delay_ms(uint32_t ms);
void timer_ms_tick(void);

#endif /* TIMER_H_ */