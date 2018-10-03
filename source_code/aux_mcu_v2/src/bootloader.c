/*
 * bootloader.c
 *
 * Created: 02/10/2018 22:12:51
 * Author : limpkin
 */ 
#include <string.h>
#include "platform_defines.h"
#include "driver_clocks.h"
#include "platform_io.h"
#include "dma.h"
#include "sam.h"
/* Bootloader flag at a given address */
volatile uint32_t bootloader_flag __attribute__((used,section (".bootloader_flag")));
/* Message about to be sent to main MCU */
aux_mcu_message_t main_mcu_send_message;

/**
 * \brief Function to start the application.
 */
static void start_application(void)
{
    /* Pointer to the Application Section */
    void (*application_code_entry)(void);

    /* Rebase the Stack Pointer */
    __set_MSP(*(uint32_t *)APP_START_ADDR);

    /* Rebase the vector table base address */
    SCB->VTOR = ((uint32_t)APP_START_ADDR & SCB_VTOR_TBLOFF_Msk);

    /* Load the Reset Handler address of the application */
    application_code_entry = (void (*)(void))(unsigned *)(*(unsigned *)(APP_START_ADDR + 4));

    /* Jump to user Reset Handler in the application */
    application_code_entry();
}

int main(void)
{
    /* Initialize the SAM system */
    SystemInit();
    
    /* Check for boot flag */
    if (bootloader_flag != BOOTLOADER_FLAG)
    {
        start_application();
    }
    
    /* Initialize system */
    clocks_start_48MDFLL();                                             // Switch to 48M main clock
    dma_init();                                                         // Initialize the DMA controller
    platform_io_init_aux_comms();                                       // Initialize aux comms   
    dma_main_mcu_init_rx_transfer();                                    // Initialize communication handling with main MCU
    
    /* Send message to main MCU to let it know we're ready to receive data */
    memset((void*)&main_mcu_send_message, 0x00, sizeof(aux_mcu_message_t));
    main_mcu_send_message.message_type = AUX_MCU_MSG_TYPE_BOOTLOADER;
    main_mcu_send_message.bootloader_message.command = BOOTLOADER_START_PROGRAMMING_COMMAND;
    main_mcu_send_message.payload_length1 = sizeof(main_mcu_send_message.bootloader_message.command);
    dma_main_mcu_init_tx_transfer((void*)&AUXMCU_SERCOM->USART.DATA.reg, (void*)&main_mcu_send_message, sizeof(aux_mcu_message_t)); 
    
    /* Automatic write, disable caching */
    NVMCTRL->CTRLB.bit.MANW = 0;
    NVMCTRL->CTRLB.bit.CACHEDIS = 1;
    
    /* Replace with your application code */
    while (1) 
    {
        /* Did we receive a message? */
        if (dma_main_mcu_other_msg_received != FALSE)
        {
            /* Check for write message and payload is a multiple of the nvm row size */
            if ((dma_main_mcu_temp_rcv_message.message_type == AUX_MCU_MSG_TYPE_BOOTLOADER) && (dma_main_mcu_temp_rcv_message.bootloader_message.command == BOOTLOADER_WRITE_COMMAND) && ((dma_main_mcu_temp_rcv_message.bootloader_message.write_command.size & (NVMCTRL_ROW_SIZE - 1)) == 0))
            {
                /* Program one or more rows */
                for (uint16_t offset = 0; offset < dma_main_mcu_temp_rcv_message.bootloader_message.write_command.size; offset+=NVMCTRL_ROW_SIZE)
                {
                    /* Compute write address */
                    uint32_t write_adddress = APP_START_ADDR + dma_main_mcu_temp_rcv_message.bootloader_message.write_command.address + offset;
                    
                    /* Erase complete row, composed of 4 pages */
                    while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
                    NVMCTRL->ADDR.reg  = write_adddress / 2;
                    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;
                    
                    /* Flash bytes */
                    for (uint32_t j = 0; j < 4; j++)
                    {
                        /* Flash 4 consecutive pages */
                        while ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) == 0);
                        for(uint32_t i = 0; i < NVMCTRL_ROW_SIZE/4; i+=2)
                        {                            
                            NVM_MEMORY[(write_adddress+j*(NVMCTRL_ROW_SIZE/4)+i)/2] = dma_main_mcu_temp_rcv_message.bootloader_message.write_command.payload_as_uint16_t[(offset+j*(NVMCTRL_ROW_SIZE/4)+i)/2];
                        }
                    }
                }
                
                /* Send message to main MCU to let it know we're ready to receive data */
                memset((void*)&main_mcu_send_message, 0x00, sizeof(aux_mcu_message_t));
                main_mcu_send_message.message_type = AUX_MCU_MSG_TYPE_BOOTLOADER;
                main_mcu_send_message.bootloader_message.command = BOOTLOADER_WRITE_COMMAND;
                main_mcu_send_message.payload_length1 = sizeof(main_mcu_send_message.bootloader_message.command);
                dma_main_mcu_init_tx_transfer((void*)&AUXMCU_SERCOM->USART.DATA.reg, (void*)&main_mcu_send_message, sizeof(aux_mcu_message_t)); 
            }
            
            /* Start app? */
            if ((dma_main_mcu_temp_rcv_message.message_type == AUX_MCU_MSG_TYPE_BOOTLOADER) && (dma_main_mcu_temp_rcv_message.bootloader_message.command == BOOTLOADER_START_APP_COMMAND))
            {
                bootloader_flag = 0;
                __disable_irq();
                __DMB();
                NVIC_SystemReset();
            }  
            
            /* Clear bool */
            dma_main_mcu_other_msg_received = FALSE; 
        }
    }
}
