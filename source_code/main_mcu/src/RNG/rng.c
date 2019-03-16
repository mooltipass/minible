/*!  \file     rng.c
*    \brief    Random number generator
*    Created:  27/01/2019
*    Author:   Mathieu Stephan
*/
#include "main.h"
#include "rng.h"
/* Current available random numbers */
uint8_t rng_acc_feed_available_pool[128];
uint16_t rng_acc_feed_available_byte_index = 0;
uint16_t rng_acc_feed_available_bytes_in_pool = 0;


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

/*! \fn     rng_feed_from_acc_read(void)
*   \brief  Feed RNG from accelerometer read
*/
void rng_feed_from_acc_read(void)
{
    uint16_t current_bit_offset = 0;
    uint8_t current_byte = 0;
    
    /* Skip first element in case it was overwritten by DMA transfer */
    for (uint16_t i = 1; i < ARRAY_SIZE(plat_acc_descriptor.fifo_read.acc_data_array); i++)
    {
        /* Extract the bits */
        uint16_t nb_extracted_bits = 6;
        uint16_t extracted_bits =   ((plat_acc_descriptor.fifo_read.acc_data_array[i].acc_x & 0x0003) << 4) \
                                    | ((plat_acc_descriptor.fifo_read.acc_data_array[i].acc_y & 0x0003) << 2) \
                                    | ((plat_acc_descriptor.fifo_read.acc_data_array[i].acc_z & 0x0003) << 0);
        
        /* Fill current byte */
        current_byte |= (uint8_t)(extracted_bits << current_bit_offset);
        
        /* Update current bit offset */
        current_bit_offset += nb_extracted_bits;
        
        /* Check if we filled our current byte */
        if (current_bit_offset >= sizeof(uint8_t))
        {
            /* Check where to store byte in our pool */
            uint16_t storage_index = rng_acc_feed_available_byte_index + rng_acc_feed_available_bytes_in_pool;
            if (storage_index > sizeof(rng_acc_feed_available_pool))
            {
                storage_index -= sizeof(rng_acc_feed_available_pool);
            }
            rng_acc_feed_available_pool[storage_index] = current_byte;
            
            /* Is the pool full? */
            if (rng_acc_feed_available_bytes_in_pool == sizeof(rng_acc_feed_available_pool))
            {
                /* Simply increment storage index */
                rng_acc_feed_available_byte_index++;
            }
            else
            {
                rng_acc_feed_available_bytes_in_pool++;
            }            
            
            /* How many extra bits we had to fille that uint8_t */
            uint16_t extra_bits = nb_extracted_bits + current_bit_offset - sizeof(uint8_t);
            
            /* Store the remaining bits in the current byte */
            current_byte = extracted_bits >> (nb_extracted_bits - extra_bits);      
            
            /* Update current bit offset */
            current_bit_offset -= sizeof(uint8_t);      
        }
    }
}