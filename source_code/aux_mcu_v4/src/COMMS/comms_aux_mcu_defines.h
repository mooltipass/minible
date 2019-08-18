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

//FIDO2 related lengths/sizes
#define RPID_LEN 252
#define USER_ID_LEN 65
#define USER_NAME_LEN 65
#define DISPLAY_NAME_LEN 32
#define CLIENT_DATA_HASH_LEN 32
#define RPID_HASH_LEN 32
#define TAG_LEN 32
#define PUB_KEY_X_LEN 32
#define PUB_KEY_Y_LEN 32
#define ATTEST_SIG_LEN 64
#define AAGUID_LEN 16
#define CRED_ID_LEN (TAG_LEN)
#define ENC_PUB_KEY_LEN 100
#define PRIV_KEY_LEN 32

#endif /* COMMS_AUX_MCU_DEFINES_H_ */
