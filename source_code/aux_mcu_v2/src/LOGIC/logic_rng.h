/*!  \file     logic_rng.h
*    \brief    Logic for random number generation
*    Created:  15/12/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_RNG_H_
#define LOGIC_RNG_H_

/* Prototypes */
void logic_rng_get_random_bytes(uint8_t* buffer, uint16_t nb_bytes);
uint16_t logic_rng_get_uint16(void);
uint8_t logic_rng_get_uint8(void);


#endif /* LOGIC_RNG_H_ */