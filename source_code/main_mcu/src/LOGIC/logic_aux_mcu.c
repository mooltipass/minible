/*!  \file     logic_aux_mcu.c
*    \brief    Aux MCU logic
*    Created:  09/05/2018
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include <asf.h>
#include "logic_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "custom_fs.h"
#include "defines.h"
#include "dma.h"


/*! \fn     logic_aux_mcu_flash_firmware_update(void)
*   \brief  Flash update firmware for aux MCU
*   \return something >= 0 if an answer needs to be sent, otherwise -1
*/
RET_TYPE logic_aux_mcu_flash_firmware_update(void)
{
    /* No need to worry about other aux messages coming in as the aux MCU is only running the bootloader */
    aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_temp_tx_message_object_pt();
    memset((void*)temp_tx_message_pt, 0, sizeof(*temp_tx_message_pt));
    aux_mcu_message_t* temp_rx_message_pt;
    
    /* Look for update file address */
    custom_fs_address_t fw_file_address;
    custom_fs_binfile_size_t fw_file_size;
    if (custom_fs_get_file_address(1, &fw_file_address, CUSTOM_FS_FW_UPDATE_TYPE) == RETURN_NOK)
    {
        /* We couldn't find the update file */
        return RETURN_NOK;
    }
    
    /* Read file size and compute binary data address */
    custom_fs_read_from_flash((uint8_t*)&fw_file_size, fw_file_address, sizeof(fw_file_size));
    fw_file_address += sizeof(fw_file_size);
    
    /* Prepare programming command */
    temp_tx_message_pt->message_type = AUX_MCU_MSG_TYPE_BOOTLOADER;
    temp_tx_message_pt->bootloader_message.command = BOOTLOADER_PROGRAMMING_COMMAND;
    temp_tx_message_pt->bootloader_message.programming_command.image_length = fw_file_size;
    temp_tx_message_pt->payload_length1 = sizeof(temp_tx_message_pt->bootloader_message.command) + sizeof(temp_tx_message_pt->bootloader_message.programming_command);
    temp_tx_message_pt->payload_length2 = sizeof(temp_tx_message_pt->bootloader_message.command) + sizeof(temp_tx_message_pt->bootloader_message.programming_command);
    temp_tx_message_pt->tx_reply_request_flag = 0x0001;
    
    /* Send message */
    dma_aux_mcu_init_tx_transfer((void*)&AUXMCU_SERCOM->USART.DATA.reg, (void*)temp_tx_message_pt, sizeof(*temp_tx_message_pt));
    dma_wait_for_aux_mcu_packet_sent();  
    
    /* Wait for answer... TODO: timeout? */
    while (comms_aux_mcu_get_received_packet(&temp_rx_message_pt, TRUE) == FALSE);
    
    /* Check for valid answer */
    if ((temp_rx_message_pt->message_type != AUX_MCU_MSG_TYPE_BOOTLOADER) || (temp_rx_message_pt->bootloader_message.command != BOOTLOADER_PROGRAMMING_COMMAND))
    {
        return RETURN_NOK;
    }
    
    /* Send bytes by blocks of 512 */
    uint32_t nb_bytes_to_read;
    while (fw_file_size > 0)
    {
        /* Compute number of bytes to send */
        nb_bytes_to_read = fw_file_size;
        if (nb_bytes_to_read > 512)
        {
            nb_bytes_to_read = 512;
        }
        
        /* Prepare write command */
        temp_tx_message_pt->message_type = AUX_MCU_MSG_TYPE_BOOTLOADER;
        temp_tx_message_pt->bootloader_message.command = BOOTLOADER_WRITE_COMMAND;
        temp_tx_message_pt->bootloader_message.write_command.size = 512;
        temp_tx_message_pt->payload_length1 = sizeof(temp_tx_message_pt->bootloader_message.command) + sizeof(temp_tx_message_pt->bootloader_message.write_command);
        temp_tx_message_pt->payload_length2 = sizeof(temp_tx_message_pt->bootloader_message.command) + sizeof(temp_tx_message_pt->bootloader_message.write_command);
        temp_tx_message_pt->tx_reply_request_flag = 0x0001;        
        
        /* Fill payload */
        custom_fs_read_from_flash((uint8_t*)temp_tx_message_pt->bootloader_message.write_command.payload, fw_file_address, nb_bytes_to_read);
        
        /* Update vars */
        fw_file_address += nb_bytes_to_read;
        fw_file_size -= nb_bytes_to_read;     
        
        /* Padding for last packet */
        if ((fw_file_size == 0) && (nb_bytes_to_read != 512))
        {
            memset((void*)&temp_tx_message_pt->bootloader_message.write_command.payload[nb_bytes_to_read], 0, 512-nb_bytes_to_read);
        }           
        
        /* Send message */
        dma_aux_mcu_init_tx_transfer((void*)&AUXMCU_SERCOM->USART.DATA.reg, (void*)temp_tx_message_pt, sizeof(*temp_tx_message_pt));
        dma_wait_for_aux_mcu_packet_sent();
        
        /* Wait for answer... TODO: timeout? */
        if (fw_file_size != 0)
        {
            while (comms_aux_mcu_get_received_packet(&temp_rx_message_pt, TRUE) == FALSE);
            
            /* Check for valid answer */
            if ((temp_rx_message_pt->message_type != AUX_MCU_MSG_TYPE_BOOTLOADER) || (temp_rx_message_pt->bootloader_message.command != BOOTLOADER_WRITE_COMMAND))
            {
                return RETURN_NOK;
            }            
        }   
    }      
    
    return RETURN_OK;
}