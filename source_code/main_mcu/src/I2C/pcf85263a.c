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
/*!  \file     pcf85263a.h
*    \brief    Driver for the pcf85263a
*    Created:  05/11/2024
*    Author:   Mathieu Stephan
*/
#include "platform_defines.h"
#ifdef MINIBLE_V2

#include "driver_i2c.h"
#include "pcf85263a.h"


/*! \fn     pcf85263a_init(void)
*   \brief  Initialize the pcf85263a
*   \return Initialization status
*/
RET_TYPE pcf85263a_init(void)
{
    /* Try writing the RAM byte */
    if (sercom_i2c_host_write_register_in_device(I2C_SERCOM, PCF85263A_ADDR, 0x2C, 0xAB) != RETURN_OK)
    {
        return RETURN_NOK;
    }
    
    /* Check for written value */
    if (sercom_i2c_read_register_from_device(I2C_SERCOM, PCF85263A_ADDR, 0x2C) != 0xAB)
    {
        return RETURN_NOK;
    }
    
    /* Return success */
    return RETURN_OK;
}

/*! \fn     pcf85263a_get_time(pcf85263a_time_structure_t* time_struct_pt)
*   \brief  Get the current time
*   \param  time_struct_pt  A pointer to a pcf85263a time structure
*/
void pcf85263a_get_time(pcf85263a_time_structure_t* time_struct_pt)
{
    sercom_i2c_read_array_from_device(I2C_SERCOM, PCF85263A_ADDR, 0x00, (uint8_t*)time_struct_pt, (uint32_t)sizeof(pcf85263a_time_structure_t));
    time_struct_pt->hundredth_seconds = DDIGITS_TO_U8(time_struct_pt->hundredth_seconds);
    time_struct_pt->seconds = DDIGITS_TO_U8(time_struct_pt->seconds & 0x7F);
    time_struct_pt->minutes = DDIGITS_TO_U8(time_struct_pt->minutes & 0x7F);
    time_struct_pt->hours = DDIGITS_TO_U8(time_struct_pt->hours);
    time_struct_pt->days = DDIGITS_TO_U8(time_struct_pt->days);
    time_struct_pt->months = DDIGITS_TO_U8(time_struct_pt->months);
    time_struct_pt->years = DDIGITS_TO_U8(time_struct_pt->years);
}


#endif