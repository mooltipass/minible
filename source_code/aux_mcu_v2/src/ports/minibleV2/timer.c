/**
* @file timer
* @date 2026/11/20
* @author tanche
*
*
*/
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/fatal.h>
#include <zephyr/sys/clock.h>

#include "drivers/timer.h"

/* macros ******************************************************************* */
LOG_MODULE_REGISTER(timer, LOG_LEVEL_INF);
/* constants **************************************************************** */
/* types - struct - enum **************************************************** */
struct timer_struct_t{
  timer_flag_te flag;
  struct k_timer timer;
};
/* private functions ******************************************************** */
static void timer_expiry_cb(struct k_timer * timer);
/* private variables ******************************************************** */
static struct timer_struct_t timers[TOTAL_NUMBER_OF_TIMERS] = {0};
/* public variables ********************************************************* */
/* extern functions ********************************************************* */
/* entry points ************************************************************* */

void timer_init(void){
  uint32_t key = irq_lock();
  memset(timers, 0, sizeof(timers));
  for(uint32_t i=0; i<(uint32_t) TOTAL_NUMBER_OF_TIMERS; i++){
    k_timer_init(&timers[i].timer, timer_expiry_cb, NULL);
  }
  irq_unlock(key);
}

void timer_start_timer(timer_id_te uid, uint32_t ms){
  switch(uid){
    case TIMER_WAIT_FUNCTS:
    case TIMER_TIMEOUT_FUNCTS:
    case TIMER_BT_TYPING_TIMEOUT:
    case TIMER_ADC_WATCHDOG:
    case TIMER_MAIN_MCU_WAKE_DELAY:
    case TIMER_USB_SEND_TIMEOUT:
    {
      uint32_t key = irq_lock();
      timers[uid].flag = TIMER_RUNNING;
      k_timer_start(&timers[uid].timer, K_MSEC(ms), K_MSEC(ms));
      irq_unlock(key);
      break;
    }
    default:
      LOG_ERR("Unmanaged timer id");
      k_fatal_halt(uid); 
  }
}

timer_flag_te timer_has_timer_expired(timer_id_te uid, bool clear){
  timer_flag_te flag = TIMER_RUNNING;
  switch(uid){
    case TIMER_WAIT_FUNCTS:
    case TIMER_TIMEOUT_FUNCTS:
    case TIMER_BT_TYPING_TIMEOUT:
    case TIMER_ADC_WATCHDOG:
    case TIMER_MAIN_MCU_WAKE_DELAY:
    case TIMER_USB_SEND_TIMEOUT:
    {
      uint32_t key = irq_lock();
      flag = timers[uid].flag;
      if(clear == true){
        timers[uid].flag = TIMER_RUNNING;
      }
      irq_unlock(key);
      break;
    }
    default:
      LOG_ERR("Unmanaged timer id");
      k_fatal_halt(uid); 
  }
  return flag;
}

uint32_t timer_get_timer_val(timer_id_te uid){
  uint32_t remaining = 0;
  switch(uid){
    case TIMER_WAIT_FUNCTS:
    case TIMER_TIMEOUT_FUNCTS:
    case TIMER_BT_TYPING_TIMEOUT:
    case TIMER_ADC_WATCHDOG:
    case TIMER_MAIN_MCU_WAKE_DELAY:
    case TIMER_USB_SEND_TIMEOUT:
    {
      uint32_t key = irq_lock();
      remaining = k_timer_remaining_get(&timers[uid].timer);
      irq_unlock(key);
      break;
    }
    default:
      LOG_ERR("Unmanaged timer id");
      k_fatal_halt(uid); 
  }
  return remaining;
}

uint32_t timer_get_systick(void){
  return sys_clock_tick_get_32();
}

void timer_delay_ms(uint32_t ms){
  k_msleep(ms);
}
/*******************************************************************************
*                       PRIVATE FUNCTIONS
*******************************************************************************/

/**
* @brief Timer expired callback. We use it to stop zephyr timer. As it it work
*        as a signle shot timer
* @param timer: pointer on struct k_timer that expired
*/
static void timer_expiry_cb(struct k_timer * timer){
  for(uint32_t i=0; i<TOTAL_NUMBER_OF_TIMERS; i++){
    if(timer == &timers[i].timer){
      k_timer_stop(timer);
      timers[i].flag = TIMER_EXPIRED;
      return;
    }
  }
  LOG_ERR("Unmanaged timer id");
  k_fatal_halt(0);
}
