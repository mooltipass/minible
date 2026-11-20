/**
* @file debug
* @date 2026/11/10
* @author tanche
*
*
*/

#include "ble_utils.h"
#include "platform_io.h"

/* macros ******************************************************************* */
/* constants **************************************************************** */
/* types - struct - enum **************************************************** */
/* private functions ******************************************************** */
/* private variables ******************************************************** */
/* public variables ********************************************************* */

/**
 * Defining ble_utils_printf function to use to display ble log
 */
void (*ble_utils_printf)(const char *fmt, ...) =
#if defined(USB_PRINTF)
    comms_usb_debug_printf
#else
  platform_io_uart_debug_printf
#endif
;
/* extern functions ********************************************************* */
/* entry points ************************************************************* */

/*******************************************************************************
*                       PRIVATE FUNCTIONS
*******************************************************************************/

/**
* @brief
* @param
* @return
*/

