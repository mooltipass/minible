/*!  \file     logic_rng.c
*    \brief    Logic for random number generation
*    Created:  15/12/2019
*    Author:   Mathieu Stephan
*/
#include "comms_main_mcu.h"
#include "logic_rng.h"
#include "defines.h"
/* Buffer containing our random numbers */
uint8_t logic_rng_buffer[32];
/* Number of bytes available in our buffer */
uint16_t logic_rng_bytes_available = 0;


/*! \fn     logic_rng_get_uint16(void)
*   \brief  Get a random uint16
*   \return A random uint16_t
*/
uint16_t logic_rng_get_uint16(void)
{
    /* Check if we have enough in stock */
    if (logic_rng_bytes_available < 2)
    {
        comms_main_mcu_get_32_rng_bytes_from_main_mcu(logic_rng_buffer);
        logic_rng_bytes_available = 32;
    }
    
    /* Prepare answer and send it */
    uint16_t return_val = logic_rng_buffer[sizeof(logic_rng_buffer) - logic_rng_bytes_available];
    logic_rng_bytes_available--;
    return_val = (return_val << 8) & 0x00FF;
    return_val |= logic_rng_buffer[sizeof(logic_rng_buffer) - logic_rng_bytes_available];
    logic_rng_bytes_available--;
    return return_val;
}

/*! \fn     logic_rng_get_uint8(void)
*   \brief  Get a random uint8
*   \return A random uint8_t
*/
uint8_t logic_rng_get_uint8(void)
{
    /* Check if we have enough in stock */
    if (logic_rng_bytes_available < 1)
    {
        comms_main_mcu_get_32_rng_bytes_from_main_mcu(logic_rng_buffer);
        logic_rng_bytes_available = 32;
    }
    
    uint8_t return_val = logic_rng_buffer[sizeof(logic_rng_buffer) - logic_rng_bytes_available];
    logic_rng_bytes_available--;
    return return_val;
}
