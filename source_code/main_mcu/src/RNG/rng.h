/*!  \file     rng.h
*    \brief    Random number generator
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/


#ifndef RNG_H_
#define RNG_H_

#include "defines.h"

/* Prototypes */
void rng_fill_array(uint8_t* array, uint16_t nb_bytes);
uint16_t rng_get_random_uint16_t(void);
uint8_t rng_get_random_uint8_t(void);
void rng_feed_from_acc_read(void);


#endif /* RNG_H_ */