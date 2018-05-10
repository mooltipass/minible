#include <asf.h>
#include "port_manager.h"
#include "driver_sercom.h"
#include "power_manager.h"
#include "clock_manager.h"
#include "dma.h"
#include "comm.h"
#include "nvm.h"
#include "bootloader.h"
#include "driver_sercom.h"

/* 349 ms max wait time */
#define BOOTLOADER_TIMEOUT_MS       (349)
#define BOOTLOADER_TIMEOUT_TICKS    ((BOOTLOADER_TIMEOUT_MS*48000)-1)
#define APP_START_ADDR              (0x2000) /* Bootloader size 8k */


/* State Machine */
typedef enum {
    WAIT,
    PROGRAM,
    START_APP,
} T_boot_state;

/* State Machine */
typedef enum {
    ENTER_PROGRAMMING,
    WRITE
} T_boot_commands;

/* Binary Image info */
typedef struct{
    uint16_t cmd;
    uint16_t reserved;
    uint32_t size;
    uint32_t crc;
} T_image_info;

/* Write Data */
typedef struct{
    uint16_t cmd;
    uint16_t size;
    uint32_t crc;
    uint32_t* data;
} T_write_info;


/* Variables */
T_image_info image_info;
T_boot_state current_state = WAIT;
T_boot_state next_state;
bool entry_flg = true;
bool enter_programming = false;
bool start_app = false;
uint32_t write_addr = 0u;
uint32_t current_len = 0u;
uint32_t current_addr = 0u;


/**
 * \brief Function to start the application.
 */
static void start_application(void)
{
    /* Pointer to the Application Section */
    void (*application_code_entry)(void);

    /* Load the Reset Handler address of the application */
    application_code_entry = (void (*)(void))(unsigned *)(*(unsigned *)(APP_START_ADDR + 4));

    /* Jump to application if startaddr different than erased */
    if( *(uint32_t*)(APP_START_ADDR + 4) != 0xFFFFFFFF){
        /* Rebase the Stack Pointer */
        __set_MSP(*(uint32_t *)APP_START_ADDR);

        /* Rebase the vector table base address */
        SCB->VTOR = ((uint32_t)APP_START_ADDR & SCB_VTOR_TBLOFF_Msk);

        /* Jump to user Reset Handler in the application */
        application_code_entry();
    }
}

static void timeout_init(void){
    /* Disable SysTick CTRL */
    SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk);
    SysTick->LOAD = 0u;
    SysTick->VAL = 0u;
    /* Perform Dummy Read to clear COUNTFLAG */
    (void)SysTick->CTRL;

    /* Initialize systick (use processor freq) */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    #if BOOTLOADER_TIMEOUT_TICKS < 0xFFFFFF
        SysTick->LOAD = BOOTLOADER_TIMEOUT_TICKS;
    #else
        SysTick->LOAD = 0xFFFFFFu;
    #endif /* BOOTLOADER_TIMEOUT_TICKS < 0xFFFFFF */

    SysTick->VAL = 0u;
}

/**
 * \brief Function to start the application.
 * \return true - tiemout expired
 * \return false - timeout not expired
 */
static bool timeout_expired(void){
    return ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0u);
}
T_comm_msg internaltest = {
    .msg_type = 2,
    .payload_valid = 1,
    .payload_len = 10,
    .payload = {0x00,0x00,0x01,0x02,0x00,0x02,0x00,0x00,0x22,0x33,0x44,0x55},
};

/*! \brief Main function. Execution starts here.
 */
int main(void) {
    irq_initialize_vectors();
    cpu_irq_enable();

    // System init
    system_init();

    // Port init
    port_manager_init();

    // Power Manager init
    power_manager_init();

    // Clock Manager init
    clock_manager_init();

    // DMA init
    dma_init();

    // Init Serial communications
    comm_init();

    // send something through sercom
    //sercom_send_single_byte(SERCOM1, 0x55);
    //dma_aux_mcu_init_tx_transfer(&internaltest, sizeof(internaltest));


    // The main loop
    while (true) {
        switch(current_state){
            case WAIT:
                /* First action to execute when entering WAIT state */
                if(entry_flg){
                    timeout_init();
                }

                /* Process Programming Command */
                comm_task();

                /* programming mode ?? */
                if( enter_programming ){
                    next_state = PROGRAM;
                    //dma_aux_mcu_init_tx_transfer(&internaltest, sizeof(internaltest));
                }
                /* timeout expired ?? */
                else if(timeout_expired()){
                    next_state = START_APP;
                }
                break;

            case PROGRAM:
                comm_task();

                /* Start Application Required ?? */
                if(start_app){
                    next_state = START_APP;
                }
                break;

            case START_APP:
                start_application();
                /* this line shall not be reached */
            default:
                next_state = WAIT;
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


void bootloader_enter_programming(T_image_info info){
    /* Store program information */
    image_info = info;

    /* Variables to be used when writing to NVM */
    current_len = 0u;
    current_addr = APP_START_ADDR;

    /* Enter into programming */
    enter_programming = true;
#if 0
    //__ASM("label: B label");
    internaltest.payload[0] = WRITE;
    internaltest.payload[1] = 0x00;

    internaltest.payload[2] = 0x00;
    internaltest.payload[3] = 0x02;

    internaltest.payload[4] = 0x00;
    internaltest.payload[5] = 0x00;
    internaltest.payload[6] = 0x00;
    internaltest.payload[7] = 0x00;

    internaltest.payload[8] = 0x58;
    internaltest.payload[9] = 0x17;
    internaltest.payload[10] = 0x00;
    internaltest.payload[11] = 0x20;

    internaltest.payload[12] = 0x09;
    internaltest.payload[13] = 0x20;
    internaltest.payload[14] = 0x00;
    internaltest.payload[15] = 0x00;

    internaltest.payload[16] = 0xfe;
    internaltest.payload[17] = 0xe7;
    internaltest.payload[18] = 0xfe;
    internaltest.payload[19] = 0xe7;
    internaltest.payload[20] = 0xfe;
    internaltest.payload[21] = 0xe7;
    internaltest.payload[22] = 0xfe;
    internaltest.payload[23] = 0xe7;
#endif
}


void bootloader_write(uint32_t* src, uint32_t len){

    if(current_state != PROGRAM){
        return;
    }

    /* Checks for a later phase */
    nvm_write_buffer((uint32_t*)current_addr, src, len);

    /* Increment lenght and address */
    current_len += len;
    current_addr += len;

    if(image_info.size <= current_len){
        start_app = true;
    }
}


bool bootloader_process_msg(uint8_t* buff, uint16_t buff_len){
    T_boot_commands cmd = (T_boot_commands)((buff[1] << 8) + buff[0]);
    T_image_info *info = (T_image_info *)buff;
    T_write_info *write = (T_write_info *)buff;
    bool operation_ok = false;

    switch(cmd){
        case ENTER_PROGRAMMING:
            bootloader_enter_programming(*info);
            operation_ok = true;
            break;

        case WRITE:
            if( enter_programming ){
                bootloader_write(&write->data, write->size);
                operation_ok = true;
            } else{
                operation_ok = false;
            }
            break;
        default:
            break;
    }

    return operation_ok;
}


