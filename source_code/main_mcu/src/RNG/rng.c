/*!  \file     rng.c
*    \brief    Random number generator
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/
#include "defines.h"
#include "rng.h"


/*! \fn     rng_fill_array(uint8_t* array, uint16_t nb_bytes)
*   \brief  Fill array with random numbers
*   \param  array       Array to fill
*   \param  nb_bytes    Number of bytes to fill
*/
void rng_fill_array(uint8_t* array, uint16_t nb_bytes)
{
    for (uint16_t i = 0; i < nb_bytes; i++)
    {
        array[i] = 4; // Chosen by fair dice roll. Guaranteed to be random.
    }
}