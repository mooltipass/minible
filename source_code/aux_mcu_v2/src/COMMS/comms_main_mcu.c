/*!  \file     comms_aux_mcu.c
*    \brief    Communications with aux MCU
*    Created:  03/03/2018
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include <asf.h>
#include "comms_hid_msgs_debug.h"
#include "platform_defines.h"
#include "comms_hid_msgs.h"
#include "comms_main_mcu.h"
#include "driver_timer.h"
#include "at_ble_api.h"
#include "comms_usb.h"
#include "ble_utils.h"
#include "defines.h"
#include "logic.h"
#include "main.h"
#include "dma.h"
#include "udc.h"
/* Received and sent MCU messages */
aux_mcu_message_t main_mcu_send_message;
/* Flag set if we have treated a message by only looking at its first bytes */
BOOL comms_main_mcu_usb_msg_answered_using_first_bytes = FALSE;
BOOL comms_main_mcu_ble_msg_answered_using_first_bytes = FALSE;
BOOL comms_main_mcu_other_msg_answered_using_first_bytes = FALSE;


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
    return &main_mcu_send_message;
}

/*! \fn     comms_main_mcu_send_message(aux_mcu_message_t* message, uint16_t message_length)
*   \brief  Send a message to the MCU
*   \param  message         Pointer to the message to send
*   \param  message_length  Message length
*   \note   Transfer is done through DMA so data will be accessed after this function returns
*/
void comms_main_mcu_send_message(aux_mcu_message_t* message, uint16_t message_length)
{
    /* To implement here in the future: no comms check */
    
    /* The function below does wait for a previous transfer to finish */
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
        memset(&main_mcu_send_message, 0x00, sizeof(aux_mcu_message_t));
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
                memcpy(main_mcu_send_message.aux_details_message.atbtlc_address, atbtlc_address.addr, sizeof(atbtlc_address.addr));
            }
        }
        
        /* Send message */
        comms_main_mcu_send_message((void*)&main_mcu_send_message, (uint16_t)sizeof(main_mcu_send_message));
    }
    else if (message->message_type == AUX_MCU_MSG_TYPE_MAIN_MCU_CMD)
    {
        switch(message->main_mcu_command_message.command)
        {
            case MAIN_MCU_COMMAND_SLEEP:
            {
                main_standby_sleep(TRUE);
                break;
            }
            case MAIN_MCU_COMMAND_PING:
            {
                comms_main_mcu_send_message((void*)message, (uint16_t)sizeof(aux_mcu_message_t));
                break;
            }
            case MAIN_MCU_COMMAND_ATTACH_USB:
            {
                udc_attach();
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
        if (comms_main_mcu_usb_msg_answered_using_first_bytes == FALSE)
        {
            comms_usb_send_hid_message((aux_mcu_message_t*)&dma_main_mcu_usb_rcv_message);
        }
        comms_main_mcu_usb_msg_answered_using_first_bytes = FALSE;
        dma_main_mcu_usb_msg_received = FALSE;
    }
    if (dma_main_mcu_ble_msg_received != FALSE)
    {
        if (comms_main_mcu_ble_msg_answered_using_first_bytes == FALSE)
        {
            // TBD
        }
        comms_main_mcu_ble_msg_answered_using_first_bytes = FALSE;
        dma_main_mcu_ble_msg_received = FALSE;
    }
    if (dma_main_mcu_other_msg_received != FALSE)
    {
        if (comms_main_mcu_other_msg_answered_using_first_bytes == FALSE)
        {
            comms_main_mcu_deal_with_non_usb_non_ble_message((aux_mcu_message_t*)&dma_main_mcu_other_message);
        }
        comms_main_mcu_other_msg_answered_using_first_bytes = FALSE;
        dma_main_mcu_other_msg_received = FALSE;
    }
    
    /* Second: see if we could deal with a packet in advance */
    /* Ongoing RX transfer received bytes */
    uint16_t nb_received_bytes_for_ongoing_transfer = sizeof(dma_main_mcu_temp_rcv_message) - dma_main_mcu_get_remaining_bytes_for_rx_transfer();
    
    /* Depending on the message type, set the correct bool pointer */
    BOOL* answered_with_the_first_bytes_pointer = &comms_main_mcu_other_msg_answered_using_first_bytes;
    if (dma_main_mcu_temp_rcv_message.message_type == AUX_MCU_MSG_TYPE_USB)
    {
        answered_with_the_first_bytes_pointer = &comms_main_mcu_usb_msg_answered_using_first_bytes;
    } 
    else if (dma_main_mcu_temp_rcv_message.message_type == AUX_MCU_MSG_TYPE_BLE)
    {
        answered_with_the_first_bytes_pointer = &comms_main_mcu_ble_msg_answered_using_first_bytes;
    }
    
    /* First part of message */
    if ((nb_received_bytes_for_ongoing_transfer >= sizeof(dma_main_mcu_temp_rcv_message.message_type) + sizeof(dma_main_mcu_temp_rcv_message.payload_length1) + dma_main_mcu_temp_rcv_message.payload_length1) && (*answered_with_the_first_bytes_pointer == FALSE))
    {
        /* Set bool and do necessary action */
        *answered_with_the_first_bytes_pointer = TRUE;
        if (dma_main_mcu_temp_rcv_message.message_type == AUX_MCU_MSG_TYPE_USB)
        {
            comms_usb_send_hid_message((aux_mcu_message_t*)&dma_main_mcu_temp_rcv_message);
        }
        else if (dma_main_mcu_temp_rcv_message.message_type == AUX_MCU_MSG_TYPE_BLE)
        {
            // TBD
        }
        else
        {
            comms_main_mcu_deal_with_non_usb_non_ble_message((aux_mcu_message_t*)&dma_main_mcu_temp_rcv_message);            
        }
    }
}