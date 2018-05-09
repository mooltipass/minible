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
    volatile aux_mcu_message_t temp_message;
    memset((void*)&temp_message, 0, sizeof(temp_message));
    
    /* Look for update file address */
    custom_fs_address_t fw_file_address;
    custom_fs_binfile_size_t fw_file_size;
    if (custom_fs_get_file_address(1, &fw_file_address, CUSTOM_FS_FW_UPDATE_TYPE) == RETURN_NOK)
    {
        /* If we couldn't find the update file */
        return RETURN_NOK;
    }
    
    /* Read file size and compute binary data address */
    custom_fs_read_from_flash((uint8_t*)&fw_file_size, fw_file_address, sizeof(fw_file_size));
    fw_file_address += sizeof(fw_file_size);
    
    /* Prepare programming command */
    temp_message.message_type = AUX_MCU_MSG_TYPE_BOOTLOADER;
    temp_message.bootloader_message.command = BOOTLOADER_PROGRAMMING_COMMAND;
    temp_message.bootloader_message.programming_command.image_length = fw_file_size;
    temp_message.payload_length1 = sizeof(temp_message.bootloader_message.command) + sizeof(temp_message.bootloader_message.programming_command);
    temp_message.payload_length2 = sizeof(temp_message.bootloader_message.command) + sizeof(temp_message.bootloader_message.programming_command);
    temp_message.payload_valid_flag = 0x0001;
    
    /* Send message */
    dma_aux_mcu_init_tx_transfer((void*)&AUXMCU_SERCOM->USART.DATA.reg, (void*)&temp_message, sizeof(temp_message));
    dma_wait_for_aux_mcu_packet_sent();    
    
    return RETURN_OK;
}