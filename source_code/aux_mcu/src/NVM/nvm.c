/*!  \file     nvm.c
 *   \brief    Low level driver for NVM
 *   Created:  04/05/2018
 *   Author:   MBorregoTrujillo
 */
#include "nvm.h"

#define wait_nvm_ready()   while(NVMCTRL->INTFLAG.bit.READY == 0)


/*! \fn     nvm_erase_row
 *  \brief  Erase row from NVM (non volatile memory, flash)
 *          1 Row = 4 Pages = 256 bytes
 *  \param  Pointer to the address to be erased
 *          (8 lower bits are discarded)
 */
static void nvm_erase_row(uint32_t* addr){
    /* Clear error flags */
    NVMCTRL->STATUS.reg = NVMCTRL_STATUS_MASK;
    /* Set address */
    NVMCTRL->ADDR.reg = ((uint32_t)addr) / 2;
    /* Execute Erase Row command */
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
    /* Wait */
    wait_nvm_ready();
};

/*! \fn     nvm_write_buffer
 *  \brief  Writes buffer contents into destination address and erase the row before
 *          1 Row = 4 Pages = 256 bytes
 *  \param
 */
T_nvm_err nvm_write_buffer(uint32_t* dst, uint32_t* src, uint32_t len){
    T_nvm_err err = NVM_NO_ERR;
    uint16_t i,j;
    /* Check length, has to be multiple of row size (256) */
    if(len % (NVMCTRL_ROW_PAGES*NVMCTRL_PAGE_SIZE)){
        err = NVM_BAD_LEN;
    }
    else if( (uint32_t)dst % (NVMCTRL_ROW_PAGES*NVMCTRL_PAGE_SIZE)){
        err = NVM_BAD_ADDR;
    } else{
        /* Wait */
        wait_nvm_ready();
        /* Erase And Write n pages */
        for(i=0; i<len; i+=NVMCTRL_PAGE_SIZE){
            /* Erase row if aligned to row */
            if(((uint32_t)dst % (NVMCTRL_ROW_PAGES*NVMCTRL_PAGE_SIZE)) == 0){
                nvm_erase_row(dst);
            }
            /* Erase the page buffer before buffering new data */
            NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_PBC | NVMCTRL_CTRLA_CMDEX_KEY;
            /* Force-wait for the buffer clear to complete */
            wait_nvm_ready();
            /* Clear error flags */
            NVMCTRL->STATUS.reg = NVMCTRL_STATUS_MASK;
            /* load data into page */
            for(j=0; j<NVMCTRL_PAGE_SIZE/4; j++){
                *dst = *src;
                dst++;
                src++;
            }
            /* Write Page */
            NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
            /* Force-wait for the page write to complete */
            wait_nvm_ready();
        }
     }

    return err;
}
