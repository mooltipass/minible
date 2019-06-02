#include "interrupt_sam_nvic.h"
#include "platform_defines.h"
#include "logic_bluetooth.h"
#include "comms_main_mcu.h"
#include "logic_battery.h"
#include "driver_clocks.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "comms_usb.h"
#include "defines.h"
#include "logic.h"
#include "fuses.h"
#include "main.h"
#include "dma.h"
#include "usb.h"
#include "udc.h"
/* Bootloader flag at a given address */
volatile uint32_t bootloader_flag __attribute__((used,section (".bootloader_flag")));

/****************************************************************************/
/* The blob of code below is aimed at facilitating our development process  */
/* To understand this, you need to know that:                               */
/* - the Cortex M0 expects the stack pointer address to be at addr 0x0000   */
/* - the Cortex M0 expects the reset handler address to be at addr 0x0004   */
/*                                                                          */
/* So this is what we do:                                                   */
/* - put the sram base address + something at addr0 using the array below   */
/* - then put the address of a custom function made to boot the main code   */
/*                                                                          */
/* This is made possible by:                                                */
/* - adding .flash_start_addr=0x0 to linker option                          */
/* - adding .start_app_function_addr=0x200 (matches the 2nd element in array*/
/* - adding --undefined=jump_to_application_function to linker option       */
/* - adding --undefined=jump_to_application_function_addr to linker option  */
/*                                                                          */
/* To move the application address, change APP_START_ADDR & .text           */
/* What I don't understand: why the "+1" in the second array element        */
/****************************************************************************/
const uint32_t jump_to_application_function_addr[2] __attribute__((used,section (".flash_start_addr"))) = {HMCRAMC0_ADDR+100,0x200+1};
void jump_to_application_function(void) __attribute__((used,section (".start_app_function_addr")));
void jump_to_application_function(void)
{
    /* Overwriting the default value of the NVMCTRL.CTRLB.MANW bit (errata reference 13134) */
    NVMCTRL->CTRLB.bit.MANW = 1;
    
    /* Pointer to the Application Section */
    void (*application_code_entry)(void);
    
    /* Rebase the Stack Pointer */
    __set_MSP(*(uint32_t*)APP_START_ADDR);
    
    /* Rebase the vector table base address */
    SCB->VTOR = ((uint32_t)APP_START_ADDR & SCB_VTOR_TBLOFF_Msk);
    
    /* Load the Reset Handler address of the application */
    application_code_entry = (void (*)(void))(unsigned *)(*(unsigned *)(APP_START_ADDR + 4));
    
    /* Jump to user Reset Handler in the application */
    application_code_entry();
}

/*! \fn     main_set_bootloader_flag(void)
*   \brief  Set bootloader flag
*/
void main_set_bootloader_flag(void)
{
    bootloader_flag = BOOTLOADER_FLAG;
}

/*! \fn     main_platform_init(void)
*   \brief  Initialize our platform
*/
void main_platform_init(void)
{
    /* Initialization results vars */
    RET_TYPE fuses_ok;
    
    /* Enable EIC interrupts and no comms input */
    platform_io_enable_eic();
    platform_io_init_no_comms_input();
    
    /* Main MCU boot delay, so it can set the interrupt line high */
    DELAYMS_8M(10);
    
    /* Sleep until main MCU tells us to wake up */
    main_standby_sleep(TRUE);
    
    /* Check fuses */
    fuses_ok = fuses_check_program(TRUE);                               // Check fuses and program them if incorrectly set
    while(fuses_ok == RETURN_NOK);
    
    /* Perform Initializations */
    clocks_start_48MDFLL();                                             // Switch to 48M main clock
    dma_init();                                                         // Initialize the DMA controller
    timer_initialize_timebase();                                        // Initialize the platform time base
    platform_io_init_ports();                                           // Initialize platform IO ports
    comms_main_init_rx();                                               // Initialize communication handling with main MCU    
    usb_init();                                                         // Initialize USB stack
    logic_battery_init();                                               // Initialize battery logic code
}    

/*! \fn     main_standby_sleep(BOOL startup_run)
*   \brief  Go to sleep
*   \param  startup_run     Set to TRUE when this routine is called at startup
*/
void main_standby_sleep(BOOL startup_run)
{    
    /* Disable DMA transfers */
    if (startup_run == FALSE)
    {
        dma_main_mcu_disable_transfer();
    }
    
    /* Errata 10416: disable interrupt routines */
    cpu_irq_enter_critical();
        
    /* Prepare the ports for sleep */
    platform_io_prepare_ports_for_sleep();
    
    /* Enable no comms interrupt on low level */
    platform_io_enable_no_comms_int();
    
    /* Enter deep sleep */
    SCB->SCR = SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
    
    /* Disable no comms interrupt */
    platform_io_disable_no_comms_int();
    
    /* Prepare ports for sleep exit */
    platform_io_prepare_ports_for_sleep_exit();
    
    /* Damn errata... enable interrupts */
    cpu_irq_leave_critical();
    
    /* Re-enable DMA transfers */
    if (startup_run == FALSE)
    {
        DELAYMS(1);
        comms_main_init_rx();
    }
}

int main(void)
{
    /* Initialize our platform */
    main_platform_init();
        
    /* Test code: remove later */
    /*udc_attach();
    timer_start_timer(TIMER_TIMEOUT_FUNCTS, 5000);
    while (timer_has_timer_expired(TIMER_TIMEOUT_FUNCTS, TRUE) == TIMER_RUNNING)
    {
        comms_main_mcu_routine();
        comms_usb_communication_routine();
    }
    logic_bluetooth_start_bluetooth();*/
    
    /* Infinite loop */
    while(TRUE)
    {
        logic_battery_task();
        comms_main_mcu_routine();
        comms_usb_communication_routine();
        
        /* If BLE enabled: deal with events */
        if (logic_is_ble_enabled() != FALSE)
        {
            logic_bluetooth_routine();
        }
    }
}
