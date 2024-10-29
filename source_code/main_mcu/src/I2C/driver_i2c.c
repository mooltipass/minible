/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2024 Stephan Mathieu
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*!  \file     driver_i2c.c
*    \brief    Low level driver i2c comms
*    Created:  07/07/2024
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "driver_timer.h"
#include "driver_i2c.h"
/* I2C error flag */
BOOL sercom_i2c_error_flag = FALSE;


/*! \fn     sercom_i2c_get_error_flag(void)
*   \brief  Get possible I2C error flag
*   \return The flag
*/
BOOL sercom_i2c_get_error_flag(void)
{
    return sercom_i2c_error_flag;
}

/*! \fn     sercom_i2c_host_init(Sercom* sercom_pt, i2c_speed_mode_te speed_mode)
*   \brief  Initialize a SERCOM in I2C host mode
*   \param  sercom_pt       Pointer to a sercom module
*   \param  speed_mode      I2C speed mode
*   \note   I2C won't run in standby, high-speed mode not supported at the moment
*/
void sercom_i2c_host_init(Sercom* sercom_pt, i2c_speed_mode_te speed_mode)
{
    SERCOM_I2CM_CTRLA_Type ctrla_reg;
    
    /* CTRLA settings */
    ctrla_reg.reg = 0;                  // Register clear
    ctrla_reg.bit.SPEED = speed_mode;   // Speed mode specified by parameter
    ctrla_reg.bit.SDAHOLD = 2;          // 450ns SDA hold, recommended by atmel guide
    ctrla_reg.bit.MODE = SERCOM_I2CM_CTRLA_MODE_I2C_MASTER_Val;
    if (speed_mode == I2C_HS_MODE)
    {
        ctrla_reg.bit.SCLSM = 1;        // Clock stretch mode mandatory for high speed
    }
    sercom_pt->I2CM.CTRLA = ctrla_reg;  // Write CTRLA register value
    
    /* CTRLB settings: enable smart mode to not have to manually issue (N)ACKs */
    sercom_pt->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;
    
    /* Wait for sync */
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);
    
    /* BAUDLOW is non-zero, and baud register is loaded */
    if (speed_mode == I2C_FAST_MODE)
        //sercom_pt->I2CM.BAUD.reg = SERCOM_I2CM_BAUD_BAUD(32) | SERCOM_I2CM_BAUD_BAUDLOW(64);    // 400kHz
        sercom_pt->I2CM.BAUD.reg = SERCOM_I2CM_BAUD_BAUD(128) | SERCOM_I2CM_BAUD_BAUDLOW(255);    // 100kHz
    else if (speed_mode == I2C_FAST_MODEPLUS)
        sercom_pt->I2CM.BAUD.reg = SERCOM_I2CM_BAUD_BAUD(11) | SERCOM_I2CM_BAUD_BAUDLOW(22);    // 1MHz
    else
        sercom_pt->I2CM.BAUD.reg = SERCOM_I2CM_BAUD_BAUD(1) | SERCOM_I2CM_BAUD_BAUDLOW(2);      // less than 3.4MHz
    
    /* Wait for sync */
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);
    
    /* Enable SERCOM */
    ctrla_reg.reg |= SERCOM_I2CM_CTRLA_ENABLE;
    sercom_pt->I2CM.CTRLA = ctrla_reg;
    
    /* SERCOM enable synchronization busy wait */
    while((sercom_pt->I2CM.SYNCBUSY.reg & SERCOM_I2CM_SYNCBUSY_ENABLE));
    
    /* Bus state is forced into idle state */
    sercom_pt->I2CM.STATUS.bit.BUSSTATE = 0x1;
    
    /* Wait for sync */
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);
}

/*! \fn     sercom_i2c_check_for_errors(Sercom* sercom_pt)
*   \brief  Check for I2C sercom errors
*   \return success code
*/
RET_TYPE sercom_i2c_check_for_errors(Sercom* sercom_pt)
{
    /* Check for errors and NACKs */
    if ((sercom_pt->I2CM.STATUS.reg & (~SERCOM_I2CM_STATUS_BUSSTATE_Msk) & (~SERCOM_I2CM_STATUS_CLKHOLD)) != 0)
    {
        sercom_i2c_error_flag = TRUE;
        return RETURN_NOK;
    }

    /* Check for bus state error */
    if ((sercom_pt->I2CM.STATUS.bit.BUSSTATE != 1) && (sercom_pt->I2CM.STATUS.bit.BUSSTATE != 2))
    {
        sercom_i2c_error_flag = TRUE;
        return RETURN_NOK;
    }

    /* All good! */
    return RETURN_OK;
}

/*! \fn     sercom_i2c_host_write_byte_to_device(Sercom* sercom_pt, uint8_t addr_7b, uint8_t data)
*   \brief  Write a single byte to a given I2C device
*   \param  sercom_pt       Pointer to a sercom module
*   \param  addr_7b         Device 7bits address
*   \param  data            Data to be written
*   \return success code
*/
RET_TYPE sercom_i2c_host_write_byte_to_device(Sercom* sercom_pt, uint8_t addr_7b, uint8_t data)
{
    /* Check for bus errors */
    if (sercom_i2c_check_for_errors(sercom_pt) != RETURN_OK)
    {
        return RETURN_NOK;
    }

    sercom_pt->I2CM.ADDR.reg = (addr_7b << 1) | 0;  // Put address on bus
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);      // Wait for sync
    while(sercom_pt->I2CM.INTFLAG.bit.MB == 0);     // Wait for transmit

    /* Check for bus errors */
    if (sercom_i2c_check_for_errors(sercom_pt) != RETURN_OK)
    {
        return RETURN_NOK;
    }

    sercom_pt->I2CM.DATA.reg = data;                // Send data
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);      // Wait for sync
    while(sercom_pt->I2CM.INTFLAG.bit.MB == 0);     // Wait for transmit

    /* Check for bus errors */
    if (sercom_i2c_check_for_errors(sercom_pt) != RETURN_OK)
    {
        return RETURN_NOK;
    }

    sercom_pt->I2CM.CTRLB.bit.CMD = 0x03;           // Issue stop condition
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);      // Wait for sync
    
    return RETURN_OK;
}

/*! \fn     sercom_i2c_host_write_register_in_device(Sercom* sercom_pt, uint8_t addr_7b, uint8_t reg_addr, uint8_t data)
*   \brief  Write a single byte to a register in a given I2C device
*   \param  sercom_pt       Pointer to a sercom module
*   \param  addr_7b         Device 7bits address
*   \param  reg_addr        Register address
*   \param  data            Data to be written
*   \return success code
*/
RET_TYPE sercom_i2c_host_write_register_in_device(Sercom* sercom_pt, uint8_t addr_7b, uint8_t reg_addr, uint8_t data)
{
    /* Check for bus errors */
    if (sercom_i2c_check_for_errors(sercom_pt) != RETURN_OK)
    {
        return RETURN_NOK;
    }

    sercom_pt->I2CM.ADDR.reg = (addr_7b << 1) | 0;  // Put address on bus
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);      // Wait for sync
    while(sercom_pt->I2CM.INTFLAG.bit.MB == 0);     // Wait for transmit

    /* Check for bus errors */
    if (sercom_i2c_check_for_errors(sercom_pt) != RETURN_OK)
    {
        return RETURN_NOK;
    }

    sercom_pt->I2CM.DATA.reg = reg_addr;            // Send register address
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);      // Wait for sync
    while(sercom_pt->I2CM.INTFLAG.bit.MB == 0);     // Wait for transmit

    /* Check for bus errors */
    if (sercom_i2c_check_for_errors(sercom_pt) != RETURN_OK)
    {
        return RETURN_NOK;
    }

    sercom_pt->I2CM.DATA.reg = data;                // Send data
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);      // Wait for sync
    while(sercom_pt->I2CM.INTFLAG.bit.MB == 0);     // Wait for transmit

    /* Check for bus errors */
    if (sercom_i2c_check_for_errors(sercom_pt) != RETURN_OK)
    {
        return RETURN_NOK;
    }

    sercom_pt->I2CM.CTRLB.bit.CMD = 0x03;           // Issue stop condition
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);      // Wait for sync

    return RETURN_OK;
}

/*! \fn     sercom_i2c_host_write_array_to_device(Sercom* sercom_pt, uint8_t addr_7b, uint8_t* data, uint32_t data_length)
*   \brief  Write a single byte to a register in a given I2C device
*   \param  sercom_pt       Pointer to a sercom module
*   \param  addr_7b         Device 7bits address
*   \param  data            Data to be written
*   \param  data_length     Number of bytes to write
*   \return success code
*/
RET_TYPE sercom_i2c_host_write_array_to_device(Sercom* sercom_pt, uint8_t addr_7b, uint8_t* data, uint32_t data_length)
{
    /* Check for bus errors */
    if (sercom_i2c_check_for_errors(sercom_pt) != RETURN_OK)
    {
        return RETURN_NOK;
    }

    sercom_pt->I2CM.ADDR.reg = (addr_7b << 1) | 0;  // Put address on bus
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);      // Wait for sync
    while(sercom_pt->I2CM.INTFLAG.bit.MB == 0);     // Wait for transmit
    for (uint16_t i = 0; i < data_length; i++)
    {
        /* Check for bus errors */
        if (sercom_i2c_check_for_errors(sercom_pt) != RETURN_OK)
        {
            return RETURN_NOK;
        }

        sercom_pt->I2CM.DATA.reg = data[i++];       // Send data
        while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);  // Wait for sync
        while(sercom_pt->I2CM.INTFLAG.bit.MB == 0); // Wait for transmit
    }

    /* Check for bus errors */
    if (sercom_i2c_check_for_errors(sercom_pt) != RETURN_OK)
    {
        return RETURN_NOK;
    }

    sercom_pt->I2CM.CTRLB.bit.CMD = 0x03;           // Issue stop condition
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);      // Wait for sync
    
    return RETURN_OK;
}

/*! \fn     sercom_i2c_read_register_from_device(Sercom* sercom_pt, uint8_t addr_7b, uint8_t reg_addr)
*   \brief  Write a single byte to a register in a given I2C device
*   \param  sercom_pt       Pointer to a sercom module
*   \param  addr_7b         Device 7bits address
*   \param  reg_addr        Register address
*   \param  data_rcv        Pointer to where to store the received data
*   \return The register contents
*   \note   Call sercom_i2c_get_error_flag to check for errors
*/
uint8_t sercom_i2c_read_register_from_device(Sercom* sercom_pt, uint8_t addr_7b, uint8_t reg_addr)
{
    uint8_t return_value = 0x00;
    
    /* Check for bus errors */
    if (sercom_i2c_check_for_errors(sercom_pt) != RETURN_OK)
    {
        return 0x00;
    }

    sercom_pt->I2CM.ADDR.reg = (addr_7b << 1) | 0;  // Put address on bus
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);      // Wait for sync
    while(sercom_pt->I2CM.INTFLAG.bit.MB == 0);     // Wait for transmit

    /* Check for bus errors */
    if (sercom_i2c_check_for_errors(sercom_pt) != RETURN_OK)
    {
        return 0x00;
    }

    sercom_pt->I2CM.DATA.reg = reg_addr;            // Send register address
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);      // Wait for sync
    while(sercom_pt->I2CM.INTFLAG.bit.MB == 0);     // Wait for transmit   

    /* Check for bus errors */
    if (sercom_i2c_check_for_errors(sercom_pt) != RETURN_OK)
    {
        return 0x00;
    }

    sercom_pt->I2CM.ADDR.reg = (addr_7b << 1) | 1;  // Issue restart condition, put address on bus with read
    sercom_pt->I2CM.CTRLB.bit.ACKACT = 1;           // Send NACK upon DATA read
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);      // Wait for sync 
    timer_start_timer(TIMER_I2C_TIMEOUT, 2);        // Arm I2C timeout timer
    while (sercom_pt->I2CM.INTFLAG.bit.SB == 0)     // Wait for receive
    {
        if (timer_has_timer_expired(TIMER_I2C_TIMEOUT, TRUE) == TIMER_EXPIRED)
        {
            return RETURN_NOK;
        }
    }
    sercom_pt->I2CM.CTRLB.bit.CMD = 0x03;           // Issue stop condition
    while(sercom_pt->I2CM.SYNCBUSY.bit.SYSOP);      // Wait for sync
    return_value = sercom_pt->I2CM.DATA.reg;        // Read return data

    return return_value;
}
