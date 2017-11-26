/* CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at src/license_cddl-1.0.txt
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at src/license_cddl-1.0.txt
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*! \file   smartcard.c
*   \brief  Smart Card low level functions
*   Copyright [2014] [Mathieu Stephan]
*/
//#include "smart_card_higher_level_functions.h"
//#include "logic_aes_and_comms.h"
//#include <util/delay_basic.h>
//#include "logic_smartcard.h"
//#include <avr/interrupt.h>
//#include "timer_manager.h"
//#include <util/atomic.h>
#include "smartcard_highlevel.h"
#include "smartcard_lowlevel.h"
#include "platform_defines.h"
#include "driver_sercom.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "defines.h"

/** Counter for successive card detects **/
volatile uint16_t card_detect_counter = 0;
/** Current detection state, see detect_return_t */
volatile det_ret_type_te button_return;
/** Smartcard power state **/
volatile uint8_t card_powered = FALSE;


/*! \fn     smartcard_lowlevel_hpulse_delay(void)
*   \brief  2us half pulse delay, specified by datasheet (min 3.3us/2)
*/
void smartcard_lowlevel_hpulse_delay(void)
{
    DELAYUS(2);
}

/*! \fn     smartcard_lowlevel_tchp_delay(void)
*   \brief  Tchp delay (3.0ms min)
*/
static inline void smartcard_lowlevel_tchp_delay(void)
{
    timer_delay_ms(4);
}

/*! \fn     smartcard_lowlevel_clock_pulse(void)
*   \brief  Send a 4us H->L clock pulse (datasheet: min 3.3us)
*/
void smartcard_lowlevel_clock_pulse(void)
{
    PORT->Group[SMC_SCK_GROUP].OUTSET.reg = SMC_SCK_MASK;
    smartcard_lowlevel_hpulse_delay();
    PORT->Group[SMC_SCK_GROUP].OUTCLR.reg = SMC_SCK_MASK;
    smartcard_lowlevel_hpulse_delay();
}

/*! \fn     smartcard_lowlevel_inverted_clock_pulse(void)
*   \brief  Send a 4us L->H clock pulse (datasheet: min 3.3us)
*/
void smartcard_lowlevel_inverted_clock_pulse(void)
{
    PORT->Group[SMC_SCK_GROUP].OUTCLR.reg = SMC_SCK_MASK;
    smartcard_lowlevel_hpulse_delay();
    PORT->Group[SMC_SCK_GROUP].OUTSET.reg = SMC_SCK_MASK;
    smartcard_lowlevel_hpulse_delay();
}

/*! \fn     smartcard_lowlevel_write_nerase(uint8_t is_write)
*   \brief  Perform a write or erase operation on the smart card
*   \param  is_write    Boolean to indicate if it is a write
*/
void smartcard_lowlevel_write_nerase(uint8_t is_write)
{
    /* Set programming control signal */
    PORT->Group[SMC_PGM_GROUP].OUTSET.reg = SMC_PGM_MASK;
    smartcard_lowlevel_hpulse_delay();

    /* Set data according to write / erase */
    if (is_write != FALSE)
    {
        PORT->Group[SMC_MOSI_GROUP].OUTSET.reg = SMC_MOSI_MASK;
    }
    else
    {
        PORT->Group[SMC_MOSI_GROUP].OUTCLR.reg = SMC_MOSI_MASK;
    }
    smartcard_lowlevel_hpulse_delay();

    /* Set clock */
    PORT->Group[SMC_SCK_GROUP].OUTSET.reg = SMC_SCK_MASK;
    smartcard_lowlevel_hpulse_delay();

    /* Release program signal and data, wait for tchp */
    PORT->Group[SMC_PGM_GROUP].OUTCLR.reg = SMC_PGM_MASK;
    smartcard_lowlevel_tchp_delay();

    /* Release clock */
    PORT->Group[SMC_SCK_GROUP].OUTCLR.reg = SMC_SCK_MASK;
    smartcard_lowlevel_hpulse_delay();

    /* Release data */
    PORT->Group[SMC_MOSI_GROUP].OUTCLR.reg = SMC_MOSI_MASK;
    smartcard_lowlevel_hpulse_delay();
}

/*! \fn     smartcard_lowlevel_blow_fuse(uint8_t fuse_name)
*   \brief  Blow the manufacturer or issuer fuse
*   \param  fuse_name    Which fuse to blow
*/
void smartcard_lowlevel_blow_fuse(uint8_t fuse_name)
{
    uint16_t i;

    /* Set the index to write */
    if (fuse_name == MAN_FUSE)
    {
        i = 1460;
    }
    else if (fuse_name == ISSUER_FUSE)
    {
        i = 1560;
    }
    else if (fuse_name == EC2EN_FUSE)
    {
        i = 1529;
    }
    else
    {
        i = 0;
    }

    /* Switch to bit banging */
    platform_io_smc_switch_to_bb();
    smartcard_lowlevel_hpulse_delay();
    smartcard_lowlevel_clear_pgmrst_signals();

    /* Get to the good index */
    while(i--)smartcard_lowlevel_clock_pulse();

    /* Set RST signal */
    PORT->Group[SMC_RST_GROUP].OUTSET.reg = SMC_RST_MASK;

    /* Perform a write */
    smartcard_lowlevel_write_nerase(TRUE);

    /* Set PGM / RST signals to standby mode */
    smartcard_lowlevel_set_pgmrst_signals();

    /* Switch to SPI mode */
    platform_io_smc_switch_to_spi();
}

/*! \fn     smartcard_lowlevel_is_card_plugged(void)
*   \brief  Know if a card is plugged
*   \return just released/pressed, (non)detected
*/
det_ret_type_te smartcard_lowlevel_is_card_plugged(void)
{
    // This copy is an atomic operation
    volatile det_ret_type_te return_val = button_return;

    if ((return_val != RETURN_DET) && (return_val != RETURN_REL))
    {
        cpu_irq_enter_critical();

        if (button_return == RETURN_JDETECT)
        {
            button_return = RETURN_DET;
        }
        else if (button_return == RETURN_JRELEASED)
        {
            button_return = RETURN_REL;
        }
        
        cpu_irq_leave_critical();
    }

    return return_val;
}

/*! \fn     smartcard_lowlevel_detect(void)
*   \brief  Card detect debounce called by 1ms interrupt
*/
void smartcard_lowlevel_detect(void)
{
    if ((PORT->Group[SMC_DET_GROUP].IN.reg & SMC_DET_MASK) == 0)
    {
        if (card_detect_counter == 250)
        {
            // We must make sure the user detected that the smartcard was removed before setting it as detected!
            if (button_return != RETURN_JRELEASED)
            {
                button_return = RETURN_JDETECT;
                card_detect_counter++;
            }
        }
        else if (card_detect_counter != 0xFF)
        {
            card_detect_counter++;            
        }
    }
    else
    {
        // Smartcard remove functions
        if ((card_detect_counter != 0) && (card_powered != FALSE))
        {
            platform_io_smc_remove_function();
            card_powered = FALSE;
            //clearSmartCardInsertedUnlocked();
        }
        if (button_return == RETURN_DET)
        {
            button_return = RETURN_JRELEASED;
        }
        else if (button_return != RETURN_JRELEASED)
        {
            button_return = RETURN_REL;
        }
        card_detect_counter = 0;
    }
}

/*! \fn     smartcard_lowlevel_first_detect_function(void)
*   \brief  functions performed once the smart card is detected
*   \return The detection result (see card_detect_return_t)
*/
card_detect_return_te smartcard_lowlevel_first_detect_function(void)
{
    uint16_t data_buffer;
    uint16_t temp_uint;   

    /* Initialize IOs */
    platform_io_smc_inserted_function();
    card_powered = TRUE;

    /* Let the card come online */
    timer_delay_ms(300);

    /* Check smart card FZ */
    smartcard_highlevel_read_fab_zone((uint8_t*)&data_buffer);
    if ((swap16(data_buffer)) != SMARTCARD_FABRICATION_ZONE)
    {
        return RETURN_CARD_NDET;
    }

    /* Perform test write on MTZ */
    smartcard_highlevel_read_mem_test_zone((uint8_t*)&temp_uint);
    temp_uint = temp_uint + 5;
    smartcard_highlevel_write_mem_test_zone((uint8_t*)&temp_uint);
    smartcard_highlevel_read_mem_test_zone((uint8_t*)&data_buffer);
    if (data_buffer != temp_uint)
    {
        return RETURN_CARD_TEST_PB;
    }

    /* Read security code attempts counter */
    switch(smartcard_highlevel_get_nb_sec_tries_left())
    {
        case 4: return RETURN_CARD_4_TRIES_LEFT;
        case 3: return RETURN_CARD_3_TRIES_LEFT;
        case 2: return RETURN_CARD_2_TRIES_LEFT;
        case 1: return RETURN_CARD_1_TRIES_LEFT;
        case 0: return RETURN_CARD_0_TRIES_LEFT;
        default: return RETURN_CARD_0_TRIES_LEFT;
    }
}

#ifdef dtc
/*! \fn     eraseApplicationZone1NZone2SMC(uint8_t zone1_nzone2)
*   \brief  Set E1 or E2 flag by presenting the correct erase key (always FFFF...) and erase the AZ1 or AZ2
*   \param  zone1_nzone2    Zone 1 or Not Zone 2
*/
void eraseApplicationZone1NZone2SMC(uint8_t zone1_nzone2)
{
    #ifdef SMARTCARD_FUSE_V1        
        uint8_t temp_bool;
    #endif
    uint16_t i;

    /* Which index to go to */
    if (zone1_nzone2 == FALSE)
    {
        i = 1248;
    }
    else
    {
        i = 688;
    }

    #if SPI_SMARTCARD == SPI_NATIVE
        /* Switch to bit banging */
        platform_io_smc_switch_to_bb();
        smartcard_lowlevel_hpulse_delay();
        smartcard_lowlevel_clear_pgmrst_signals();

        /* Get to the good EZx */
        while(i--) smartcard_lowlevel_inverted_clock_pulse();

        /* How many bits to compare */
        if (zone1_nzone2 == FALSE)
        {
            i = 32;
        }
        else
        {
            i = 48;
        }

        /* Clock is at high level now, as input must be switched during this time */
        /* Enter the erase key */
        smartcardHPulseDelay();
        while(i--)
        {
            // The code is always FFFF...
            smartcardHPulseDelay();

            /* Inverted clock pulse */
            smartcard_lowlevel_inverted_clock_pulse();
        }

        /* Bring clock and data low */
        PORT_SPI_NATIVE &= ~(1 << SCK_SPI_NATIVE);
        smartcardHPulseDelay();smartcardHPulseDelay();
        PORT_SPI_NATIVE &= ~(1 << MOSI_SPI_NATIVE);
        smartcardHPulseDelay();smartcardHPulseDelay();
        
        /* In smart card fuse V1 (early versions sent to beta testers), EC2EN is not blown so we're limited to 128 erase operations... */
        #ifdef SMARTCARD_FUSE_V1
            if (zone1_nzone2 == FALSE)
            {               
                i = 0;
                temp_bool = TRUE;
                /* Write one of the four SCAC bits to 0 and check if successful */
                while((temp_bool == TRUE) && (i < 128))
                {
                    /* If one of the four bits is at 1, write a 0 */
                    if (PINB & (1 << MISO_SPI_NATIVE))
                    {
                        /* Set write command */
                        smartcard_lowlevel_write_nerase(TRUE);

                        /* Wait for the smart card to output a 0 */
                        while(PINB & (1 << MISO_SPI_NATIVE));

                        /* Exit loop */
                        temp_bool = FALSE;
                    }
                    else
                    {
                        /* Clock pulse */
                        smartcard_lowlevel_clock_pulse();
                        i++;
                    }
                }
            }
        #endif

        /* Erase AZ1/AZ2 */
        smartcard_lowlevel_write_nerase(FALSE);

        /* Set PGM / RST signals to standby mode */
        setPgmRstSignals();

        /* Switch to SPI mode */
        setSPIModeSMC();
    #else
        #error "SPI not supported"
    #endif
}
#endif

/*! \fn     smartcard_lowlevel_validate_code(volatile uint16_t* code)
*   \brief  Check security code
*   \param  code    The code
*   \return success_status (see card_detect_return_t)
*/
pin_check_return_te smartcard_lowlevel_validate_code(volatile uint16_t* code)
{
    pin_check_return_te return_val = RETURN_PIN_NOK_0;
    BOOL temp_bool;
    uint16_t i;

    /* Switch to bit banging */
    platform_io_smc_switch_to_bb();
    smartcard_lowlevel_hpulse_delay();
    smartcard_lowlevel_clear_pgmrst_signals();

    /* Get to the SC */
    for(i = 0; i < 80; i++)
    {
        smartcard_lowlevel_inverted_clock_pulse();
    }

    /* Clock is at high level now, as input must be switched during this time */
    /* Enter the SC */
    smartcard_lowlevel_hpulse_delay();
    for(i = 0; i < 16; i++)
    {
        if (((*code >> (15-i)) & 0x0001) != 0x0000)
        {
            PORT->Group[SMC_MOSI_GROUP].OUTCLR.reg = SMC_MOSI_MASK;
        }
        else
        {
            PORT->Group[SMC_MOSI_GROUP].OUTSET.reg = SMC_MOSI_MASK;
        }
        smartcard_lowlevel_hpulse_delay();

        /* Inverted clock pulse */
        smartcard_lowlevel_inverted_clock_pulse();
    }

    /* Bring clock and data low */
    PORT->Group[SMC_SCK_GROUP].OUTCLR.reg = SMC_SCK_MASK;
    smartcard_lowlevel_hpulse_delay();
    smartcard_lowlevel_hpulse_delay();
    PORT->Group[SMC_MOSI_GROUP].OUTCLR.reg = SMC_MOSI_MASK;
    smartcard_lowlevel_hpulse_delay();
    smartcard_lowlevel_hpulse_delay();

    i = 0;
    temp_bool = TRUE;
    /* Write one of the four SCAC bits to 0 and check if successful */
    while((temp_bool == TRUE) && (i < 4))
    {
        /* If one of the four bits is at 1, write a 0 */
        if ((PORT->Group[SMC_MISO_GROUP].IN.reg & SMC_MISO_MASK) != 0)
        {
            /* Set write command */
            smartcard_lowlevel_write_nerase(TRUE);

            /* Wait for the smart card to output a 0 */
            while((PORT->Group[SMC_MISO_GROUP].IN.reg & SMC_MISO_MASK) != 0);

            /* Now, erase SCAC */
            smartcard_lowlevel_write_nerase(FALSE);

            /* Were we successful? */
            if ((PORT->Group[SMC_MISO_GROUP].IN.reg & SMC_MISO_MASK) != 0)
            {
                // Success !
                return_val = RETURN_PIN_OK;
            }
            else
            {
                // Wrong pin, return number of tries left
                if (i == 0)
                {
                    return_val = RETURN_PIN_NOK_3;
                }
                else if (i == 1)
                {
                    return_val = RETURN_PIN_NOK_2;
                }
                else if (i == 2)
                {
                    return_val = RETURN_PIN_NOK_1;
                }
                else if (i == 3)
                {
                    return_val = RETURN_PIN_NOK_0;
                }
            }

            /* Clock pulse */
            smartcard_lowlevel_clock_pulse();

            /* Exit loop */
            temp_bool = FALSE;
        }
        else
        {
            /* Clock pulse */
            smartcard_lowlevel_clock_pulse();
            i++;
        }
    }

    /* If we couldn't find a spot to write, no tries left */
    if (i == 4)
    {
        return_val = RETURN_PIN_NOK_0;
    }

    /* Set PGM / RST signals to standby mode */
    smartcard_lowlevel_set_pgmrst_signals();

    /* Switch to SPI mode */
    platform_io_smc_switch_to_spi();

    return return_val;
}

/*! \fn     smartcard_lowlevel_write_smc(uint16_t start_index_bit, uint16_t nb_bits, uint8_t* data_to_write)
*   \brief  Write bits to the smart card
*   \param  start_index_bit         Where to start writing bits
*   \param  nb_bits                 Number of bits to write
*   \param  data_to_write           Pointer to the buffer
*/
void smartcard_lowlevel_write_smc(uint16_t start_index_bit, uint16_t nb_bits, uint8_t* data_to_write)
{
    uint16_t current_written_bit = 0;
    uint16_t masked_bit_to_write = 0;
    uint16_t i;

    /* Switch to bit banging */
    platform_io_smc_switch_to_bb();
    smartcard_lowlevel_hpulse_delay();
    smartcard_lowlevel_clear_pgmrst_signals();

    /* Try to not erase AZ1 if EZ1 is 0xFFFFFFF... and we're writing the first bit of the AZ2 */
    if (start_index_bit >= SMARTCARD_AZ2_BIT_START)
    {
        /* Clock pulses until AZ2 start - 1 */
        for(i = 0; i < SMARTCARD_AZ2_BIT_START - 1; i++)
        {
            smartcard_lowlevel_clock_pulse();            
        }
        PORT->Group[SMC_MOSI_GROUP].OUTSET.reg = SMC_MOSI_MASK;
        smartcard_lowlevel_clock_pulse();
        PORT->Group[SMC_MOSI_GROUP].OUTCLR.reg = SMC_MOSI_MASK;
        /* Clock for the rest */
        for(i = 0; i < (start_index_bit - SMARTCARD_AZ2_BIT_START); i++)
        {
            smartcard_lowlevel_clock_pulse();            
        }                
    }
    else
    {
        /* Get to the good index, clock pulses */
        for(i = 0; i < start_index_bit; i++)
        {
            smartcard_lowlevel_clock_pulse();
        }                
    }

    /* Start writing */
    for(current_written_bit = 0; current_written_bit < nb_bits; current_written_bit++)
    {
        /* If we are at the start of a 16bits word or writing our first bit, erase the word */
        if ((((start_index_bit+current_written_bit) & 0x000F) == 0) || (current_written_bit == 0))
        {
            smartcard_lowlevel_write_nerase(FALSE);
        }

        /* Get good bit to write */
        masked_bit_to_write = (data_to_write[(current_written_bit>>3)] >> (7 - (current_written_bit & 0x0007))) & 0x01;

        /* Write only if the data is a 0 */
        if (masked_bit_to_write == 0x00)
        {
            smartcard_lowlevel_write_nerase(TRUE);
        }

        /* Go to next address */
        smartcard_lowlevel_clock_pulse();
    }

    /* Set PGM / RST signals to standby mode */
    smartcard_lowlevel_set_pgmrst_signals();

    /* Switch to SPI mode */
    platform_io_smc_switch_to_spi();
}

/*! \fn     smartcard_lowlevel_clear_pgmrst_signals(void)
*   \brief  Clear PGM / RST signal for normal operation mode
*/
void smartcard_lowlevel_clear_pgmrst_signals(void)
{
    PORT->Group[SMC_PGM_GROUP].OUTCLR.reg = SMC_PGM_MASK;
    PORT->Group[SMC_RST_GROUP].OUTCLR.reg = SMC_RST_MASK;
    smartcard_lowlevel_hpulse_delay();
    smartcard_lowlevel_hpulse_delay();
}

/*! \fn     smartcard_lowlevel_set_pgmrst_signals(void)
*   \brief  Set PGM / RST signal for standby mode
*/
void smartcard_lowlevel_set_pgmrst_signals(void)
{
    PORT->Group[SMC_RST_GROUP].OUTSET.reg = SMC_RST_MASK;
    PORT->Group[SMC_PGM_GROUP].OUTCLR.reg = SMC_PGM_MASK;
    smartcard_lowlevel_hpulse_delay();
}

/*! \fn     smartcard_lowlevel_read_smc(uint16_t nb_bytes_total_read, uint16_t start_record_index, uint8_t* data_to_receive)
*   \brief  Read bytes from the smart card
*   \param  nb_bytes_total_read     The number of bytes to be read
*   \param  start_record_index      The index at which we start recording the answer
*   \param  data_to_receive        Pointer to the buffer
*   \return The buffer
*/
uint8_t* smartcard_lowlevel_read_smc(uint16_t nb_bytes_total_read, uint16_t start_record_index, uint8_t* data_to_receive)
{
    uint8_t* return_val = data_to_receive;
    uint8_t i;

    /* Set PGM / RST signals for operation */
    smartcard_lowlevel_clear_pgmrst_signals();

    for(i = 0; i < nb_bytes_total_read; i++)
    {
        /* Start transmission */
        uint8_t data_byte = sercom_spi_send_single_byte(SMARTCARD_SERCOM, 0x00);

        /* Store data in buffer or discard it*/
        if (i >= start_record_index)
        {
            *(data_to_receive++) = data_byte;
        }
    }

    /* Set PGM / RST signals to standby mode */
    smartcard_lowlevel_set_pgmrst_signals();

    return return_val;
}