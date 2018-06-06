#include <asf.h>
#include "port_manager.h"
#include "driver_sercom.h"
#include "power_manager.h"
#include "clock_manager.h"
#include "dma.h"
#include "comm.h"
#include "comm_bootloader.h"
#include "nvm.h"
#include "bootloader.h"
#include "driver_sercom.h"

/* Configure max wait time */
#define BOOTLOADER_WAIT_TIME_MS     (500u)
#define BOOTLAODER_TICK_MS          (100u)
#define SYSTICK_100MS               ((4800000u) - 1)
#define APP_START_ADDR              (0x2000) /* Bootloader size 8k */

/* Private Variables */
static uint32_t image_size;
static uint32_t image_crc;
static T_boot_state current_state = BOOTLOADER_WAIT;
static T_boot_state next_state;
static bool entry_flg = true;
static bool enter_programming = false;
static bool exit_programming = false;
static uint32_t current_len = 0u;
static uint32_t current_addr = 0u;
static uint32_t timeout_ms = 0u;


/*! \fn     bootloader_start_application
 *  \brief  Function to start the application.
 */
static void bootloader_start_application(void)
{
    /* Pointer to the Application Section */
    void (*application_code_entry)(void);

    /* Load the Reset Handler address of the application */
    application_code_entry = (void (*)(void))(unsigned *)(*(unsigned *)(APP_START_ADDR + 4));

    /* Jump to application if startaddr different than erased */
    if( *(uint32_t*)(APP_START_ADDR + 4) != 0xFFFFFFFF){

        /* Deinitialize COMM */
        comm_deinit();

        /* Disable transfer aux mcu */
        dma_aux_mcu_disable_transfer();

        /* Rebase the Stack Pointer */
        __set_MSP(*(uint32_t *)APP_START_ADDR);

        /* Rebase the vector table base address */
        SCB->VTOR = ((uint32_t)APP_START_ADDR & SCB_VTOR_TBLOFF_Msk);

        /* Jump to user Reset Handler in the application */
        application_code_entry();
    }
}

/*! \fn     bootloader_timeout_init
 *  \brief  Configures SysTick timer with a reload value of 100ms
 */
static void bootloader_timeout_init(void){
    /* Disable SysTick CTRL */
    SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk);
    SysTick->LOAD = 0u;
    SysTick->VAL = 0u;
    /* Perform Dummy Read to clear COUNTFLAG */
    (void)SysTick->CTRL;

    /* Initialize systick (use processor freq) */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    #if SYSTICK_100MS < 0xFFFFFF
        SysTick->LOAD = SYSTICK_100MS;
    #else
        SysTick->LOAD = 0xFFFFFFu;
    #endif /* BOOTLOADER_TIMEOUT_TICKS < 0xFFFFFF */

    SysTick->VAL = 0u;
}

/*! \fn     bootloader_timeout_expired
 *  \brief  check if previously configured timeout has been expired
 *  \return true     Timeout expired
 *  \return false    Timeout not expired
 */
static bool bootloader_timeout_expired(void){

    /* COUNTFLAG is cleared after read */
    if ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0u){
        timeout_ms += BOOTLAODER_TICK_MS; /* 100 ms tick */
    }

    /* Check Counter, reset if expired */
    if( timeout_ms >= BOOTLOADER_WAIT_TIME_MS){
        timeout_ms = 0u;
        return true;
    }

    return false;
}

/*! \fn     bootloader_get_state
 *  \brief  Return current bootloader state
 */
T_boot_state bootloader_get_state(void){
    return current_state;
}

/*! \fn     bootloader_enter_programming
 *  \brief  Saves image info to be received from MAIN MCU and
 *          sets enter programming flag
 *  \param  size Image size
 *  \param  crc  Image CRC
 */
void bootloader_enter_programming(uint32_t size, uint32_t crc){
    /* Store program information */
    image_size = size;
    image_crc = crc;

    /* Variables to be used when writing to NVM */
    current_len = 0u;
    current_addr = APP_START_ADDR;

    /* Enter into programming */
    enter_programming = true;
}

/*! \fn     bootloader_write
 *  \brief  Writes src buffer to address defined by current_addr
 *          which is incremented by len on each write
 *  \param  src pointer to buffer with the data to write
 *  \param  len length of src pointer in bytes
 */
void bootloader_write(uint32_t* src, uint32_t len){
    /* Checks for a later phase */
    nvm_write_buffer((uint32_t*)current_addr, src, len);

    /* Increment length and address */
    current_len += len;
    current_addr += len;

    if(image_size <= current_len){
        exit_programming = true;
    }
}

/*! \fn     main
 *  \brief  Main function. Execution starts here.
 */
int main(void) {
    /* Irq initialization */
    irq_initialize_vectors();
    cpu_irq_enable();

    /* System init */
    system_init();

    /* Port init */
    port_manager_init();

    /* Power Manager init */
    power_manager_init();

    /* Clock Manager init */
    clock_manager_init();

    /* DMA init */
    dma_init();

    /* Init Serial communications */
    comm_init();

    /* Check if Programming is required
     * before entering main loop
     */
    if( comm_bootloader_enter_programming_required() ){
        current_state = BOOTLOADER_PROGRAM;
    } else{
        current_state = BOOTLOADER_WAIT;
    }

    /* The main loop */
    while (true) {
        switch(current_state){
            case BOOTLOADER_WAIT:
                /* First action to execute when entering WAIT state */
                if(entry_flg){
                    bootloader_timeout_init();
                }

                /* Process Programming Command */
                comm_task();

                /* programming mode ?? */
                if( enter_programming ){
                    next_state = BOOTLOADER_PROGRAM;
                }
                /* timeout expired ?? */
                else if(bootloader_timeout_expired()){
                    next_state = BOOTLOADER_START_APP;
                }
                break;

            case BOOTLOADER_PROGRAM:
                comm_task();

                /* Start Application Required ?? */
                if(exit_programming){
                    next_state = BOOTLOADER_LAST_TX_DMA;
                }
                break;

            case BOOTLOADER_LAST_TX_DMA:
                if(dma_aux_mcu_check_tx_dma_transfer_flag()){
                    next_state = BOOTLOADER_START_APP;
                }
                break;

            case BOOTLOADER_START_APP:
                bootloader_start_application();
                /* this line shall not be reached */
                next_state = BOOTLOADER_WAIT;
                break;

            default:
                next_state = BOOTLOADER_WAIT;
                break;

        }
        /* Set entry flag if state changes */
        if(next_state != current_state){
            entry_flg = true;
        } else{
            entry_flg = false;
        }
        current_state = next_state;
    }
}


