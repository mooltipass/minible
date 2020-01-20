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
/*!  \file     fido2.h
*    \brief    FIDO2 general logic
*    Created:  19/01/2020
*    Author:   0x0ptr
*/
#ifndef FIDO2_H_
#define FIDO2_H_

#include "comms_aux_mcu.h"
#include "defines.h"

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

/* Typedefs */

typedef struct credential_DB_rec_s
{
    uint8_t rpID[FIDO2_RPID_LEN];                  //252
    uint8_t tag[FIDO2_CREDENTIAL_ID_LENGTH];                    //16
    uint8_t user_ID[FIDO2_USER_ID_LEN];            //65
    uint8_t user_name[FIDO2_USER_NAME_LEN];        //65
    uint8_t display_name[FIDO2_DISPLAY_NAME_LEN];  //65
    uint8_t spare;                                 //1 (padding)
    uint8_t priv_key[FIDO2_PRIV_KEY_LEN];          //32
    uint32_t count;                                //4
} credential_DB_rec_t;

/* Prototypes */
void fido2_process_make_auth_data(fido2_make_auth_data_req_message_t const* request, fido2_make_auth_data_rsp_message_t* response);
void fido2_process_exclude_list_item(fido2_auth_cred_req_message_t const* request, fido2_auth_cred_rsp_message_t* response);

#endif /* FIDO2_H_ */