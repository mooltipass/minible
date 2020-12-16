#include <asf.h>
#include "platform_defines.h"
#include "logic_bluetooth.h"
#include "comms_main_mcu.h"
#include "logic_battery.h"
#include "driver_clocks.h"
#include "comms_raw_hid.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "ble_manager.h"
#include "logic_sleep.h"
#include "defines.h"
#include "ctaphid.h"
#include "logic.h"
#include "fuses.h"
#include "debug.h"
#include "main.h"
#include "ctap.h"
#include "dma.h"
#include "usb.h"
#include "udc.h"
/* Bootloader flag at a given address */
volatile uint32_t bootloader_flag __attribute__((used,section (".bootloader_flag")));

extern uint32_t _estack; //Start of stack as defined by linker
extern uint32_t _sstack; //End of stack as defined by linker

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
    platform_io_init_no_comms_pullup_port();
    
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
    ctaphid_init();                                                     // Initialize CTAP HID communucation layer
    ctap_init();                                                        // Initialize CTAP layer
    logic_battery_init();                                               // Initialize battery logic code
}    

/*! \fn     main_standby_sleep(BOOL startup_run, volatile BOOL* reenable_comms)
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
    
    /* Small delay */
    DELAYUS(10);
    
    /* Disable no comms interrupt */
    platform_io_disable_no_comms_int();
    
    /* Prepare ports for sleep exit */
    platform_io_prepare_ports_for_sleep_exit();
    
    /* Damn errata... enable interrupts */
    cpu_irq_leave_critical();
}

int main(void)
{
    /* Initialize stack usage tracking */
    main_init_stack_tracking();

    /* Enable trace buffer */
    //debug_init_trace_buffer();
    
    /* Initialize our platform */
    main_platform_init();
    
    while(TRUE)
    {
        logic_battery_task();
        comms_usb_communication_routine();
        
        /* We can only communicate with main MCU when platform sleep isn't requested */
        if (logic_sleep_is_full_platform_sleep_requested() == FALSE)
        {
            comms_main_mcu_routine(FALSE, 0, FALSE);
        }
        
        /* ADC watchdog */
        if ((logic_battery_is_using_adc() != FALSE) && (timer_has_timer_expired(TIMER_ADC_WATCHDOG, TRUE) == TIMER_EXPIRED))
        {
            platform_io_get_cursense_conversion_result(TRUE);
        }
        
        /* Do we need to enable bluetooth? */
        uint8_t* mac_address;
        if ((logic_is_ble_enabled() == FALSE) && (logic_get_and_clear_bluetooth_to_be_enabled(&mac_address) != FALSE))
        {
            logic_bluetooth_start_bluetooth(mac_address);
            logic_set_ble_enabled();
            comms_main_mcu_send_simple_event(AUX_MCU_EVENT_BLE_ENABLED);
            dma_wait_for_main_mcu_packet_sent();
        }
        
        /* If BLE enabled: deal with events */
        if (logic_is_ble_enabled() != FALSE)
        {
           logic_bluetooth_routine();
        }
    }
}

#if defined(STACK_MEASURE_ENABLED)

/*! \fn     main_get_SP(void)
*   \brief  Get current active Stack Pointer (SP/r13)
*   \return Stack Pointer
*/
__attribute__( ( always_inline ) ) static inline uint32_t main_get_SP(void)
{
  register uint32_t sp;

  asm volatile ("MOV %0, SP\n" : "=r" (sp) );
  return(sp);
}

uint32_t main_stack_low_water_mark = ~0U;

/*! \fn     main_check_stack_usage(void)
*   \brief  check the stack usage
*   \return current low water mark
*/
uint32_t main_check_stack_usage(void)
{
    uint32_t stack_start;
    uint32_t stack_end;
    uint32_t i;
    uint32_t curr_low_water_mark;

    /*
     * Get the pointers to the start and end of the stack. These are symbols
     * available from the compiler/linker.
     */
    stack_start = (uint32_t) &_estack;
    stack_end = (uint32_t) &_sstack;

    for (i = stack_end; i < stack_start; ++i)
    {
        uint8_t *ptr = (uint8_t *) i;
        if (*ptr != DEBUG_STACK_TRACKING_COOKIE)
        {
            break;
        }
    }
    curr_low_water_mark = i - stack_end;

    if (curr_low_water_mark < main_stack_low_water_mark)
    {
        main_stack_low_water_mark = curr_low_water_mark;
    }
    return main_stack_low_water_mark;
}

/*! \fn     main_init_stack_tracking(void)
*   \brief  Initialize stack tracking
*/
void main_init_stack_tracking(void)
{
    uint32_t stack_start;
    uint32_t stack_end;
    uint8_t *ptr;

    stack_start = main_get_SP() - sizeof(uint32_t);
    stack_end = (uint32_t) &_sstack;

    //Inline implementation of memset since we we *might* be destroying
    //our own stack when calling the memset function.
    for (ptr = stack_end; ptr < stack_start; ++ptr)
    {
        *ptr = DEBUG_STACK_TRACKING_COOKIE;
    }
}

#else

/*! \fn     util_check_stack_usage(void)
*   \brief  check the stack usage
*   \return current low water mark
*/
uint32_t main_check_stack_usage(void)
{
    return 0;
}

/*! \fn     main_init_stack_tracking(void)
*   \brief  Initialize stack tracking
*/
void main_init_stack_tracking(void) { }

#endif
