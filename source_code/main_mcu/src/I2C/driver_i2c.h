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
/*!  \file     driver_i2c.h
*    \brief    Low level driver i2c comms
*    Created:  07/07/2024
*    Author:   Mathieu Stephan
*/


#ifndef I2C_H_
#define I2C_H_

#include "defines.h"
#include "asf.h"

/* Enums */
typedef enum {I2C_FAST_MODE = 0, I2C_FAST_MODEPLUS = 1, I2C_HS_MODE = 2} i2c_speed_mode_te;

/* Prototypes */
void sercom_i2c_read_array_from_device(Sercom* sercom_pt, uint8_t addr_7b, uint8_t reg_addr, uint8_t* data, uint32_t data_length);
RET_TYPE sercom_i2c_host_write_array_to_device(Sercom* sercom_pt, uint8_t addr_7b, uint8_t* data, uint32_t data_length);
RET_TYPE sercom_i2c_host_write_register_in_device(Sercom* sercom_pt, uint8_t addr_7b, uint8_t reg_addr, uint8_t data);
uint8_t sercom_i2c_read_register_from_device(Sercom* sercom_pt, uint8_t addr_7b, uint8_t reg_addr);
RET_TYPE sercom_i2c_host_write_byte_to_device(Sercom* sercom_pt, uint8_t addr_7b, uint8_t data);
void sercom_i2c_host_init(Sercom* sercom_pt, i2c_speed_mode_te speed_mode);
RET_TYPE sercom_i2c_check_for_errors(Sercom* sercom_pt);
BOOL sercom_i2c_get_error_flag(void);


#endif /* I2C_H_ */