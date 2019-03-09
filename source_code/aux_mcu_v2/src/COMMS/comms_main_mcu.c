/*!  \file     comms_aux_mcu.c
*    \brief    Communications with aux MCU
*    Created:  03/03/2018
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include <asf.h>
#include "comms_hid_msgs_debug.h"
#include "platform_defines.h"
#include "hid_keyboard_app.h"
#include "comms_hid_msgs.h"
#include "comms_main_mcu.h"
#include "logic_battery.h"
#include "driver_timer.h"
#include "at_ble_api.h"
#include "comms_usb.h"
#include "ble_utils.h"
#include "defines.h"
#include "logic.h"
#include "main.h"
#include "dma.h"
#include "udc.h"
/* Message about to be sent to main MCU */
aux_mcu_message_t main_mcu_send_message;
/* Temporary message, used when dealing with message shorter than max size */
volatile aux_mcu_message_t comms_main_mcu_temp_message;
/* Flag set if we have treated a message by only looking at its first bytes */
volatile BOOL comms_main_mcu_usb_msg_answered_using_first_bytes = FALSE;
volatile BOOL comms_main_mcu_ble_msg_answered_using_first_bytes = FALSE;
volatile BOOL comms_main_mcu_other_msg_answered_using_first_bytes = FALSE;

/*! \fn     comms_main_init_rx(void)
*   \brief  Init communications with aux MCU
*/
void comms_main_init_rx(void)
{
    dma_main_mcu_init_rx_transfer();
}

/*! \fn     comms_main_mcu_get_temp_tx_message_object_pt(void)
*   \brief  Get a pointer to our temporary tx message object
*/
aux_mcu_message_t* comms_main_mcu_get_temp_tx_message_object_pt(void)
{
    return (aux_mcu_message_t*)&main_mcu_send_message;
}

/*! \fn     comms_main_mcu_send_message(aux_mcu_message_t* message, uint16_t message_length)
*   \brief  Send a message to the MCU
*   \param  message         Pointer to the message to send
*   \param  message_length  Message length
*   \note   Transfer is done through DMA so data will be accessed after this function returns
*/
void comms_main_mcu_send_message(aux_mcu_message_t* message, uint16_t message_length)
{    
    /* The function below does wait for a previous transfer to finish and does check for no comms */
    dma_main_mcu_init_tx_transfer((void*)&AUXMCU_SERCOM->USART.DATA.reg, (void*)message, sizeof(aux_mcu_message_t));    
}

/*! \fn     comms_main_mcu_deal_with_non_usb_non_ble_message(void)
*   \brief  Routine dealing with non USB & non BLE messages
*   \param  message Message to deal with
*/
void comms_main_mcu_deal_with_non_usb_non_ble_message(aux_mcu_message_t* message)
{
    //comms_usb_debug_printf("Received main MCU other message: %i", message->message_type);
    
    if (message->message_type == AUX_MCU_MSG_TYPE_PLAT_DETAILS)
    {
        /* Status request */
        memset((void*)&main_mcu_send_message, 0x00, sizeof(aux_mcu_message_t));
        main_mcu_send_message.message_type = message->message_type;
        main_mcu_send_message.payload_length1 = sizeof(aux_plat_details_message_t);
        main_mcu_send_message.aux_details_message.aux_fw_ver_major = FW_MAJOR;
        main_mcu_send_message.aux_details_message.aux_fw_ver_minor = FW_MINOR;
        main_mcu_send_message.aux_details_message.aux_did_register = DSU->DID.reg;
        main_mcu_send_message.aux_details_message.aux_uid_registers[0] = *(uint32_t*)0x0080A00C;
        main_mcu_send_message.aux_details_message.aux_uid_registers[1] = *(uint32_t*)0x0080A040;
        main_mcu_send_message.aux_details_message.aux_uid_registers[2] = *(uint32_t*)0x0080A044;
        main_mcu_send_message.aux_details_message.aux_uid_registers[3] = *(uint32_t*)0x0080A048;
        
        /* Check if BLE is enabled */
        if (logic_is_ble_enabled() != FALSE)
        {
            /* Blusdk lib version */
            main_mcu_send_message.aux_details_message.blusdk_lib_maj = BLE_SDK_MAJOR_NO(BLE_SDK_VERSION);
            main_mcu_send_message.aux_details_message.blusdk_lib_min = BLE_SDK_MINOR_NO(BLE_SDK_VERSION);
            
            /* Try to get fw version */
            uint32_t blusdk_fw_ver;
            if(at_ble_firmware_version_get(&blusdk_fw_ver) == AT_BLE_SUCCESS)
            {
                main_mcu_send_message.aux_details_message.blusdk_fw_maj = BLE_SDK_MAJOR_NO(blusdk_fw_ver);
                main_mcu_send_message.aux_details_message.blusdk_fw_min = BLE_SDK_MINOR_NO(blusdk_fw_ver);
                main_mcu_send_message.aux_details_message.blusdk_fw_build = BLE_SDK_BUILD_NO(blusdk_fw_ver);
                
                /* Getting RF version */
                at_ble_rf_version_get(&main_mcu_send_message.aux_details_message.atbtlc_rf_ver);
                
                /* ATBTLC1000 chip ID */
                at_ble_chip_id_get(&main_mcu_send_message.aux_details_message.atbtlc_chip_id);
                
                /* ATBTLC address */
                at_ble_addr_t atbtlc_address;
                atbtlc_address.type = AT_BLE_ADDRESS_PUBLIC;
                at_ble_addr_get(&atbtlc_address);
                memcpy((void*)main_mcu_send_message.aux_details_message.atbtlc_address, (void*)atbtlc_address.addr, sizeof(atbtlc_address.addr));
            }
        }
        
        /* Send message */
        comms_main_mcu_send_message((void*)&main_mcu_send_message, (uint16_t)sizeof(main_mcu_send_message));
    }
    else if (message->message_type == AUX_MCU_MSG_TYPE_NIMH_CHARGE)
    {
        /* Return charging status */
        memset((void*)&main_mcu_send_message, 0x00, sizeof(aux_mcu_message_t));
        main_mcu_send_message.message_type = message->message_type;
        main_mcu_send_message.payload_length1 = sizeof(nimh_charge_message_t);
        main_mcu_send_message.nimh_charge_message.charge_status = logic_battery_get_charging_status();
        main_mcu_send_message.nimh_charge_message.battery_voltage = logic_battery_get_vbat();
        main_mcu_send_message.nimh_charge_message.charge_current = logic_battery_get_charging_current();
        
        /* Send message */
        comms_main_mcu_send_message((void*)&main_mcu_send_message, (uint16_t)sizeof(main_mcu_send_message));
    }
    else if (message->message_type == AUX_MCU_MSG_TYPE_BOOTLOADER)
    {
        if (message->bootloader_message.command == BOOTLOADER_START_PROGRAMMING_COMMAND)
        {
            main_set_bootloader_flag();
            cpu_irq_disable();
            NVIC_SystemReset();
        }
    }        
    else if (message->message_type == AUX_MCU_MSG_TYPE_MAIN_MCU_CMD)
    {
        switch(message->main_mcu_command_message.command)
        {
            case MAIN_MCU_COMMAND_SLEEP:
            {
                /* Wait for interrupt to clear this flag if set (wait for full packet receive) */
                while (comms_main_mcu_other_msg_answered_using_first_bytes != FALSE);                
                main_standby_sleep(FALSE);
                break;
            }
            case MAIN_MCU_COMMAND_PING:
            {
                /* Resend same message */
                comms_main_mcu_send_message((void*)message, (uint16_t)sizeof(aux_mcu_message_t));
                break;
            }
            case MAIN_MCU_COMMAND_ATTACH_USB:
            {
                /* Attach USB resistors */
                udc_attach();
                break;
            }
            case MAIN_MCU_COMMAND_ENABLE_BLE:
            {
                /* Enable BLE */
                if (logic_is_ble_enabled() == FALSE)
                {
                    logic_set_ble_enabled();
                    mini_ble_init();
                    message->message_type = AUX_MCU_MSG_TYPE_AUX_MCU_EVENT;
                    message->aux_mcu_event_message.event_id = AUX_MCU_EVENT_BLE_ENABLED;
                    comms_main_mcu_send_message((void*)message, (uint16_t)sizeof(aux_mcu_message_t));
                }
                else
                {
                    message->message_type = AUX_MCU_MSG_TYPE_AUX_MCU_EVENT;
                    message->aux_mcu_event_message.event_id = AUX_MCU_EVENT_BLE_ENABLED;
                    comms_main_mcu_send_message((void*)message, (uint16_t)sizeof(aux_mcu_message_t));
                }
                break;
            }
            case MAIN_MCU_COMMAND_NIMH_CHARGE:
            {
                /* Charge NiMH battery */
                logic_battery_start_charging(NIMH_12C_CHARGING);
                break;
            }
            case MAIN_MCU_COMMAND_NO_COMMS_UNAV:
            {
                /* No comms signal unavailable */
                logic_set_nocomms_unavailable();
                break;
            }
        }
    }
}

/*! \fn     comms_main_mcu_routine(void)
*   \brief  Routine dealing with main mcu comms
*/
void comms_main_mcu_routine(void)
{	
    /* First: deal with fully received messages */
    if (dma_main_mcu_usb_msg_received != FALSE)
    {
        /* Set bool and do necessary action: no point in setting the bool after the function call as the dma receiver will overwrite the packet anyways */
        dma_main_mcu_usb_msg_received = FALSE;
        
        if (comms_main_mcu_usb_msg_answered_using_first_bytes == FALSE)
        {
            comms_usb_send_hid_message((aux_mcu_message_t*)&dma_main_mcu_usb_rcv_message);
        }
    }
    if (dma_main_mcu_ble_msg_received != FALSE)
    {
        /* Set bool and do necessary action: no point in setting the bool after the function call as the dma receiver will overwrite the packet anyways */
        dma_main_mcu_ble_msg_received = FALSE;
        
        if (comms_main_mcu_ble_msg_answered_using_first_bytes == FALSE)
        {
            // TBD
        }
    }
    if (dma_main_mcu_other_msg_received != FALSE)
    {
        /* Set bool and do necessary action: no point in setting the bool after the function call as the dma receiver will overwrite the packet anyways */
        dma_main_mcu_other_msg_received = FALSE;
        
        if (comms_main_mcu_other_msg_answered_using_first_bytes == FALSE)
        {
            comms_main_mcu_deal_with_non_usb_non_ble_message((aux_mcu_message_t*)&dma_main_mcu_other_message);
        }
    }
    
    /* Second: see if we could deal with a packet in advance */
    /* Ongoing RX transfer received bytes */
    uint16_t nb_received_bytes_for_ongoing_transfer = sizeof(dma_main_mcu_temp_rcv_message) - dma_main_mcu_get_remaining_bytes_for_rx_transfer();
    
    /* Depending on the message type, set the correct bool pointers */
    volatile BOOL* answered_with_the_first_bytes_pointer = &comms_main_mcu_other_msg_answered_using_first_bytes;
    volatile BOOL* packet_fully_received_in_the_mean_time_pointer = &dma_main_mcu_other_msg_received;
    if (dma_main_mcu_temp_rcv_message.message_type == AUX_MCU_MSG_TYPE_USB)
    {
        answered_with_the_first_bytes_pointer = &comms_main_mcu_usb_msg_answered_using_first_bytes;
        packet_fully_received_in_the_mean_time_pointer = &dma_main_mcu_usb_msg_received;
    } 
    else if (dma_main_mcu_temp_rcv_message.message_type == AUX_MCU_MSG_TYPE_BLE)
    {
        answered_with_the_first_bytes_pointer = &comms_main_mcu_ble_msg_answered_using_first_bytes;
        packet_fully_received_in_the_mean_time_pointer = &dma_main_mcu_ble_msg_received;
    }
    
    /* Check if we should deal with this packet */    
    cpu_irq_enter_critical();
    BOOL should_deal_with_packet = FALSE;
    
    /* Conditions: received more bytes than the payload length, didn't already reply using this method, received flag didn't arrive in the mean time */
    if ((nb_received_bytes_for_ongoing_transfer >= sizeof(dma_main_mcu_temp_rcv_message.message_type) + sizeof(dma_main_mcu_temp_rcv_message.payload_length1) + dma_main_mcu_temp_rcv_message.payload_length1) && (*answered_with_the_first_bytes_pointer == FALSE) && ((sizeof(aux_mcu_message_t) - nb_received_bytes_for_ongoing_transfer) > 20) && (*packet_fully_received_in_the_mean_time_pointer == FALSE))
    {
        should_deal_with_packet = TRUE;
        
        /* Set bool and do necessary action: no point in setting the bool after the function call as the dma receiver will overwrite the packet anyways */
        *answered_with_the_first_bytes_pointer = TRUE;
        
        /* Copy message into dedicated buffer, as the message currently is in the dma temporary buffer. */
        memset((void*)&comms_main_mcu_temp_message, 0, sizeof(comms_main_mcu_temp_message));
        memcpy((void*)&comms_main_mcu_temp_message, (void*)&dma_main_mcu_temp_rcv_message, nb_received_bytes_for_ongoing_transfer);
    }
    cpu_irq_leave_critical();
    
    /* First part of message */
    if (should_deal_with_packet != FALSE)
    {        
        if (comms_main_mcu_temp_message.message_type == AUX_MCU_MSG_TYPE_USB)
        {
            comms_usb_send_hid_message((aux_mcu_message_t*)&comms_main_mcu_temp_message);
        }
        else if (comms_main_mcu_temp_message.message_type == AUX_MCU_MSG_TYPE_BLE)
        {
            // TBD
        }
        else
        {
            comms_main_mcu_deal_with_non_usb_non_ble_message((aux_mcu_message_t*)&comms_main_mcu_temp_message);  
        }
    }
}