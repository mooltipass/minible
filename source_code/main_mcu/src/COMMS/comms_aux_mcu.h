/*!  \file     comms_aux_mcu.h
*    \brief    Communications with aux MCU
*    Created:  03/03/2018
*    Author:   Mathieu Stephan
*/


#ifndef COMMS_AUX_MCU_H_
#define COMMS_AUX_MCU_H_

#include "comms_aux_mcu_defines.h"
#include "comms_bootloader_msg.h"
#include "comms_hid_msgs.h"

/* Defines */
// Aux MCU Message Type
#define AUX_MCU_MSG_TYPE_USB        0x0000
#define AUX_MCU_MSG_TYPE_BLE        0x0001
#define AUX_MCU_MSG_TYPE_BOOTLOADER 0x0002

/* Typedefs */
typedef struct
{
    uint16_t message_type;
    uint16_t payload_length1;
    union
    {
        aux_mcu_bootloader_message_t bootloader_message;
        hid_message_t hid_message;
        uint8_t payload[AUX_MCU_MSG_PAYLOAD_LENGTH];
        uint32_t payload_as_uint32[AUX_MCU_MSG_PAYLOAD_LENGTH/4];    
    };
    uint16_t payload_length2;
    uint16_t payload_valid_flag;
} aux_mcu_message_t;

/* Prototypes */
BOOL comms_aux_mcu_get_received_packet(aux_mcu_message_t** message, BOOL arm_new_rx);
aux_mcu_message_t* comms_aux_mcu_get_temp_tx_message_object_pt(void);
void comms_aux_mcu_routine(void);
void comms_aux_init(void);


#endif /* COMMS_AUX_MCU_H_ */
