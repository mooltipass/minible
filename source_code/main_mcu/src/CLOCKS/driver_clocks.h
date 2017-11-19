/*!  \file     driver_clocks.h
*    \brief    Platform clocks related functions
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/

#ifndef CLOCKS_POWER_H_
#define CLOCKS_POWER_H_

/* Prototypes */
void clocks_map_gclk_to_peripheral_clock(uint32_t gclk_id, uint32_t peripheral_clk_id);
void clocks_start_48MDFLL(void);


#endif /* CLOCKS_POWER_H_ */