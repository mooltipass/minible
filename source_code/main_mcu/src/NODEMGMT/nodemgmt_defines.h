/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2020 Stephan Mathieu
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
/*!  \file     nodemgmt_defines.h
*    \brief    Defines for structs that may be used in other.h files
*    Created:  20/05/2020
*    Author:   Mathieu Stephan
*/


#ifndef NODEMGMT_DEFINES_H_
#define NODEMGMT_DEFINES_H_

// Bluetooth bonding information
typedef struct
{
    uint16_t zero_to_be_valid;
    uint8_t address_resolv_type;
    uint8_t mac_address[6];
    uint8_t auth_type;
    uint8_t peer_ltk_key[16];
    uint16_t peer_ltk_ediv;
    uint8_t peer_ltk_random_nb[8];
    uint16_t peer_ltk_key_size;
    uint8_t peer_csrk_key[16];
    uint8_t peer_irk_key[16];
    uint8_t peer_irk_resolv_type;
    uint8_t peer_irk_address[6];
    uint8_t peer_irk_reserved;
    uint8_t host_ltk_key[16];
    uint16_t host_ltk_ediv;
    uint8_t host_ltk_random_nb[8];
    uint16_t host_ltk_key_size;
    uint8_t host_csrk_key[16];
    uint8_t reserved[10];
} nodemgmt_bluetooth_bonding_information_t;

#endif /* NODEMGMT_DEFINES_H_ */