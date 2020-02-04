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
/*!  \file     fido2_values_defines.h
*    \brief    FIDO2 general values
*    Created:  19/01/2020
*    Author:   Mathieu Stephan
*/


#ifndef FIDO2_VALUES_DEFINES_H_
#define FIDO2_VALUES_DEFINES_H_


#define FIDO2_RPID_LEN 252                                      //RP ID length
#define FIDO2_USER_HANDLE_LEN 64                                    //User id length
#define FIDO2_USER_NAME_LEN 65                                  //User name length
#define FIDO2_DISPLAY_NAME_LEN 65                               //Display name length
#define FIDO2_CLIENT_DATA_HASH_LEN (SHA256_OUTPUT_LENGTH)       //Client data hash length
#define FIDO2_RPID_HASH_LEN (SHA256_OUTPUT_LENGTH)              //RP ID hash length
#define FIDO2_PUB_KEY_X_LEN 32                                  //Public key X part length
#define FIDO2_PUB_KEY_Y_LEN 32                                  //Public key Y part length
#define FIDO2_ATTEST_SIG_LEN 64                                 //Attested signature length
#define FIDO2_AAGUID_LEN 16                                     //AAGUID length
#define FIDO2_ENC_PUB_KEY_LEN 100                               //Encryped public key length
#define FIDO2_PRIV_KEY_LEN 32                                   //Private key length
#define FIDO2_CREDENTIAL_ID_LENGTH 16                           // Credential id length

#define FIDO2_UP_BIT (1 << 0)       // User Present
#define FIDO2_UV_BIT (1 << 2)       // User Verified
#define FIDO2_AT_BIT (1 << 6)       // Attested Credential Data Included
#define FIDO2_ED_BIT (1 << 7)       // Extension Data Included

#define FIDO2_CBOR_CONTAINER_START 0xA0

#define FIDO2_CREDENTIAL_EXISTS     1
#define FIDO2_NEW_CREDENTIAL        1

#define COSE_KEY_LABEL_KTY      1
#define COSE_KEY_LABEL_ALG      3
#define COSE_KEY_LABEL_CRV      -1
#define COSE_KEY_LABEL_X        -2
#define COSE_KEY_LABEL_Y        -3

#define COSE_KEY_KTY_EC2        2
#define COSE_KEY_CRV_P256       1

#define COSE_ALG_ES256            -7
#define COSE_ALG_ECDH_ES_HKDF_256 -25


#endif /* FIDO2_VALUES_DEFINES_H_ */
