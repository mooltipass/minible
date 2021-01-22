/*
 * timer.h
 *
 * Created: 19/04/2017 09:38:49
 *  Author: stephan
 */ 


#ifndef DRIVER_TIMER_H_
#define DRIVER_TIMER_H_

#include <asf.h>
#include "platform_defines.h"
#include "defines.h"

/* Defines */
#define MCU_SYSTICK_MAX_PERIOD      0x00FFFFFFUL
#define TIMER_NB_CALLBACK_TIMERS    3

/** Type of the callback functions. */
typedef void (*timer_callback_t)(void* timer_id);

/* Structs */
typedef struct
{
    uint32_t timer_val;
    uint32_t flag;
} timer_struct_t;

typedef struct
{
    BOOL timer_armed;
    BOOL timer_enabled;
    uint32_t timer_val;
    uint32_t timer_set_val;
    timer_callback_t timer_callback;
} timer_callback_struct_t;

/* Typedefs */
typedef RTC_MODE2_CLOCK_Type calendar_t;

/* Enums */
typedef enum {TIMER_WAIT_FUNCTS = 0, TIMER_TIMEOUT_FUNCTS = 1, TIMER_BATTERY_TICK = 2, TIMER_BT_TYPING_TIMEOUT = 3, TIMER_ADC_WATCHDOG = 4, TIMER_MAIN_MCU_WAKE_DELAY = 5, TIMER_USB_SEND_TIMEOUT = 6, TOTAL_NUMBER_OF_TIMERS} timer_id_te;
typedef enum {TIMER_EXPIRED = 0, TIMER_RUNNING = 1} timer_flag_te;
    
/* Macros */
#define CYCLES_IN_DLYTICKS_FUNC     8
#define US_TO_DLYTICKS(us)          (uint32_t)((CPU_SPEED_HF / 1000000UL) * us / CYCLES_IN_DLYTICKS_FUNC)
#define DELAYTICKS(ticks)           {volatile uint32_t n=ticks; while(n--);}                    //takes 8 cycles
#define DELAYUS(us)                 DELAYTICKS(US_TO_DLYTICKS(us))                              //uses 20bytes
#define DELAYMS(ms)                 DELAYTICKS(US_TO_DLYTICKS(ms*1000))                         //uses 20bytes
#define US_TO_DLYTICKS_8M(us)       (uint32_t)((CPU_SPEED_MF / 1000000UL) * us / CYCLES_IN_DLYTICKS_FUNC)
#define DELAYMS_8M(ms)              DELAYTICKS(US_TO_DLYTICKS_8M(ms*1000))                      //uses 20bytes

/* Prototypes */
timer_flag_te timer_has_timer_expired(timer_id_te uid, BOOL clear);
BOOL timer_get_and_clear_too_many_cb_timers_requested_flag(void);
void timer_start_callback_timer(void* timer_id, uint32_t ms);
void* timer_create_callback_timer(void(*timer_cb)(void*));
void timer_start_timer(timer_id_te uid, uint32_t val);
void timer_remove_callback_timer(void* timer_id);
void timer_get_calendar(calendar_t* calendar_pt);
void timer_stop_callback_timer(void* timer_id);
uint32_t timer_get_timer_val(timer_id_te uid);
BOOL timer_get_mcu_systick(uint32_t* value);
void timer_reset_callback_timers(void);
void timer_initialize_timebase(void);
uint32_t timer_get_systick(void);
void timer_delay_ms(uint32_t ms);
void timer_ms_tick(void);

#endif /* TIMER_H_ */