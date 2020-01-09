/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2019 Stephan Mathieu
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
/*!  \file     comms_aux_mcu_defines.h
*    \brief    Defines for aux mcu comms
*    Created:  09/05/2018
*    Author:   Mathieu Stephan
*/


#ifndef COMMS_AUX_MCU_DEFINES_H_
#define COMMS_AUX_MCU_DEFINES_H_

// AUX MCU Message payload length
#define AUX_MCU_MSG_PAYLOAD_LENGTH  552

// HID payload size
#define HID_PAYLOAD_SIZE            64

#define SHA256_OUTPUT_LENGTH 32

//FIDO2 related lengths/sizes
#define FIDO2_RPID_LEN 252                                      //RP ID length
#define FIDO2_USER_ID_LEN 65                                    //User id length
#define FIDO2_USER_NAME_LEN 65                                  //User name length
#define FIDO2_DISPLAY_NAME_LEN 65                               //Display name length
#define FIDO2_CLIENT_DATA_HASH_LEN (SHA256_OUTPUT_LENGTH)       //Client data hash length
#define FIDO2_RPID_HASH_LEN (SHA256_OUTPUT_LENGTH)              //RP ID hash length
#define FIDO2_TAG_LEN 16                                        //TAG length
#define FIDO2_PUB_KEY_X_LEN 32                                  //Public key X part length
#define FIDO2_PUB_KEY_Y_LEN 32                                  //Public key Y part length
#define FIDO2_ATTEST_SIG_LEN 64                                 //Attested signature length
#define FIDO2_AAGUID_LEN 16                                     //AAGUID length
#define FIDO2_CRED_ID_LEN (FIDO2_TAG_LEN)                       //Credential ID length (Should be TAG length)
#define FIDO2_ENC_PUB_KEY_LEN 100                               //Encryped public key length
#define FIDO2_PRIV_KEY_LEN 32                                   //Private key length

#endif /* COMMS_AUX_MCU_DEFINES_H_ */
