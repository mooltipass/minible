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
/*!  \file     logic_fido2.h
*    \brief    FIDO2 general logic
*    Created:  19/01/2020
*    Author:   0x0ptr
*/
#ifndef LOGIC_FIDO2_H_
#define LOGIC_FIDO2_H_

#include "comms_aux_mcu.h"
#include "defines.h"

#ifdef EMULATOR_BUILD
    #ifdef WIN32
        #define cpu_to_be32 __builtin_bswap32 
    #else
        #define cpu_to_be32 htonl
    #endif
#endif

/* Enums */
typedef enum
{
    FIDO2_SUCCESS = 0,
    FIDO2_OPERATION_DENIED = 1,
    FIDO2_USER_NOT_PRESENT = 2,
    FIDO2_STORAGE_EXHAUSTED = 3,
    FIDO2_CRED_NOT_FOUND = 4,
    FIDO2_NO_CREDENTIALS = 5,
} fido2_return_code_te;

/* Prototypes */
void logic_fido2_process_make_credential(fido2_make_credential_req_message_t* request, fido2_make_credential_rsp_message_t* response);
void logic_fido2_process_get_assertion(fido2_get_assertion_req_message_t* request, fido2_get_assertion_rsp_message_t* response);
void logic_fido2_process_exclude_list_item(fido2_auth_cred_req_message_t* request, fido2_auth_cred_rsp_message_t* response);

#endif /* FIDO2_H_ */
