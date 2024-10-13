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
/*!  \file     mp2710.h
*    \brief    Driver for the MP2710
*    Created:  13/10/2024
*    Author:   Mathieu Stephan
*/
#include "platform_defines.h"
#ifdef MINIBLE_V2

#include "driver_i2c.h"
#include "mp2710.h" 

/*! \fn     mp2710_init(void)
*   \brief  Initialize the MP2710
*   \return Initialization status
*/
RET_TYPE mp2710_init(void)
{
    //if (sercom_i2c_host_write_byte_to_device(I2C_SERCOM, uint8_t addr_7b, uint8_t data) != RETURN_OK)
    {
    }
}
#endif