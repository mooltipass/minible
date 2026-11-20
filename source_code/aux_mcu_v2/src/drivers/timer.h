#ifndef TIMER_20261120
#define TIMER_20261120

/**
* @file timer
* @date 2026/11/20
* @author tanche
*
*/


#include <stdbool.h>
#include <stdint.h>

/* macros ******************************************************************* */
/* constants **************************************************************** */
/* enums ******************************************************************** */
/**
 * @enum timer_flag_te
 * Status of timers
 */
typedef enum timer_flag_te{
  TIMER_EXPIRED = 0,
  TIMER_RUNNING = 1
} timer_flag_te;

/**
 * @enum timer_id_te
 * List of available timer
 */
typedef enum {
  TIMER_WAIT_FUNCTS = 0,
  TIMER_TIMEOUT_FUNCTS = 1,
  TIMER_BT_TYPING_TIMEOUT = 2,
  TIMER_ADC_WATCHDOG = 3,
  TIMER_MAIN_MCU_WAKE_DELAY = 4,
  TIMER_USB_SEND_TIMEOUT = 5,
  TOTAL_NUMBER_OF_TIMERS
} timer_id_te;

/* structures *************************************************************** */
/* types ******************************************************************** */
/* public variables ********************************************************* */
/* public function  ********************************************************* */

/**
 * @brief create timer and initialize it
 */
void timer_init(void);

/**
 * @brief Start the timer uid to expired in ms
 * @param[in] uid: Timer ID
 * @param[in] ms: timeout in millisecond
*/
void timer_start_timer(timer_id_te uid, uint32_t ms);

/**
 * @brief Let you know if timer expired
 * @param[in] uid: Timer ID
 * @param[in] clear: indicate if the function must reset the timer status
 * @retval TIMER_RUNNING if timer isn't expired
 * @retval TIMER_EXPIRED if it expired
*/
timer_flag_te timer_has_timer_expired(timer_id_te uid, bool clear);

/**
 * @brief Give you the remaining time in ms before timer expire
 * @param[in] uid: Timer ID
 * @return remaing time in millisecond. If timer isn't running return 0
 */
uint32_t timer_get_timer_val(timer_id_te uid);

/**
 * @brief Give you acess to system tick counter value
 * @retval number of tick since system started
 */
uint32_t timer_get_systick(void);

/**
 * @brief blocking waiting function.
 * @param[in] ms: time to wait in millisecond
*/
void timer_delay_ms(uint32_t ms);

#endif /* TIMER_20261120 */
