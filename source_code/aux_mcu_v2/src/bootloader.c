/*
 * bootloader.c
 *
 * Created: 02/10/2018 22:12:51
 * Author : limpkin
 */ 
#include "platform_defines.h"
#include "driver_clocks.h"
#include "sam.h"
/* Bootloader flag at a given address */
volatile uint32_t bootloader_flag __attribute__((used,section (".bootloader_flag")));

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
    /*
    dma_init();                                                         // Initialize the DMA controller
    timer_initialize_timebase();                                        // Initialize the platform time base
    platform_io_init_ports();                                           // Initialize platform IO ports
    comms_main_init_rx();                                               // Initialize communication handling with main MCU
    usb_init();                                                         // Initialize USB stack
    logic_battery_init();                                               // Initialize battery logic code
    */

    /* Replace with your application code */
    while (1) 
    {
        //uint16_t fw_file_address = NVMCTRL_ROW_SIZE;
    }
}
