/*!  \file     comms_aux_mcu.h
*    \brief    Communications with aux MCU
*    Created:  03/03/2018
*    Author:   Mathieu Stephan
*/


#ifndef COMMS_AUX_MCU_H_
#define COMMS_AUX_MCU_H_

/* Typedefs */
typedef struct
{
    uint16_t message_type;
    uint16_t payload_length;
    uint8_t payload[532];
} aux_mcu_message_t;

/* Prototypes */
void comms_aux_mcu_routine(void);
void comms_aux_init(void);


#endif /* COMMS_AUX_MCU_H_ */