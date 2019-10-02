#include "driver_timer.h"

void timer_set_calendar(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second){}
timer_flag_te timer_has_timer_expired(timer_id_te uid, BOOL clear) { return TIMER_EXPIRED; }
void timer_start_timer(timer_id_te uid, uint32_t val){}
void timer_get_calendar(calendar_t* calendar_pt){}
uint32_t timer_get_timer_val(timer_id_te uid){}
BOOL timer_get_mcu_systick(uint32_t* value){ return FALSE; }
void timer_initialize_timebase(void){}
uint32_t timer_get_systick(void){}
void timer_delay_ms(uint32_t ms){}
void timer_ms_tick(void){}
