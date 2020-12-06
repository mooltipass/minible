/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2019 Stephan Mathieu
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*!  \file     fuses.c
*    \brief    Functions dedicated to fuse checks
*    Created:  20/06/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "platform_defines.h"
#include "fuses.h"

/* Configuration define */
#define BOOTPROT			(0x07LU << 0)       // Bootloader protected zone. 7 = 0B, 6 = 512B, 5 = 1024B, 4 = 2048B...
#define RESERVEDA			(0x01LU << 3)       // Reserved
#define EEPROM				(0x01LU << 4)       // Allocated EEPROM zone. 7 = 0B, 6 = 256B, 5 = 512B, 4 = 1024B..., 1 = 8192B
#define RESERVEDB			(0x01LU << 7)       // Reserved
#define BOD33_LEVEL			(39LU << 8)         // Brownout level, 39 = 2.84V
/* BOD is disabled on boards not having the battery, as direct 3V to Voled switch enables triggered the BOD */
#ifdef BOD_NOT_ENABLED
    #define BOD33_ENABLE	(0x00LU << 14)      // Brownout enable (1 = enabled)
#else
    #define BOD33_ENABLE	(0x01LU << 14)      // Brownout disabled (1 = enabled)
#endif
#define BOD33_ACTION		(0x01LU << 15)      // Brownout action (1 = reset)
#define RESERVEDC			(0xFFLU << 17)      // Reserved
#define WDT_ENABLE			(0x00LU << 25)      // WDT enable (1 = enabled)
#define WDT_ALWAYS_ON		(0x00LU << 26)      // WDT always on
#define WDT_PERIOD			(0x0BLU << 27)      // WDT timeout period. 0 = 8clk, 1 = 16clk, 2 = 32clk... until 0xB
#define WDT_WINDOW_LBIT		(0x01LU << 31)      // WDT window mode timeout period 1LSB
#define WDT_WINDOW_HBITS	(0x02LU << 0)       // WDT window mode timeout period 2MSB (LSB&MSB: 5 = 256clocks)
#define WDT_EWOFFSET		(0x0BLU << 3)       // Number of GCLK_WDT clocks in the offset from the start of the watchdog time-out period to when the Early Warning interrupt is generated
#define WDT_WEN				(0x00LU << 7)       // WDT Timer Window Mode Enable at power on
#define BOD33_HYS			(0x00LU << 8)       // BOD33 Hysteresis configuration at power on (0: no hysteresis)
#define RESERVEDD			(0x01LU << 9)       // Reserved
#define RESERVEDE			(0x3FLU << 10)      // Reserved
#define LOCK				(0xFFFFLU << 16)    // NVM Region Lock Bits.

/* Compose aux word from individual registers */
#define USER_WORD_0			(WDT_WINDOW_LBIT | WDT_PERIOD | WDT_ALWAYS_ON | WDT_ENABLE | BOD33_ACTION | BOD33_ENABLE | BOD33_LEVEL | EEPROM | BOOTPROT)
#define USER_WORD_0_MASK    (RESERVEDC | RESERVEDB | RESERVEDA)
#define USER_WORD_1			(LOCK | BOD33_HYS | WDT_WEN | WDT_EWOFFSET | WDT_WINDOW_HBITS)
#define USER_WORD_1_MASK    (RESERVEDE | RESERVEDD)

/* MASK Lock and Reserve Bits */
#define USER_MASK_0 0x01FE0088
#define USER_MASK_1 0x0000FE00

/* Sanity checks */
#if USER_MASK_0 != USER_WORD_0_MASK
    #error "User mask 0 error"
#endif
#if USER_MASK_1 != USER_WORD_1_MASK
    #error "User mask 1 error"
#endif

/* Command words redefines */
#define NVM_COMMAND_ERASE_AUX_ROW NVMCTRL_CTRLA_CMD_EAR
#define NVM_COMMAND_WRITE_AUX_ROW NVMCTRL_CTRLA_CMD_WAP
#define NVM_COMMAND_PAGE_BUFFER_CLEAR NVMCTRL_CTRLA_CMD_PBC

/*! \fn     fuses_check_program(BOOL flash_fuses)
*   \brief  Check for correct fuse settings (and flash)
*   \param  flash_fuses Set to TRUE to allow fuse programming
*   \return If the check / flash was correctly performed
*/
RET_TYPE fuses_check_program(BOOL flash_fuses)
{
    /* Get current words */
    uint32_t userWord0 = *((uint32_t *)NVMCTRL_AUX0_ADDRESS);
    uint32_t userWord1 = *(((uint32_t *)NVMCTRL_AUX0_ADDRESS) + 1);
    
    /* Check current words and security bit */
    #ifdef NO_SECURITY_BIT_CHECK
    if (((userWord0 & ~USER_MASK_0) == USER_WORD_0) && ((userWord1 & ~USER_MASK_1) == USER_WORD_1))
    #else
    if (((userWord0 & ~USER_MASK_0) == USER_WORD_0) && ((userWord1 & ~USER_MASK_1) == USER_WORD_1) && (NVMCTRL->STATUS.reg & NVMCTRL_STATUS_SB))
    #endif
    {
        /* Conf words & security bit ok */
        return RETURN_OK;
    }
    
    /* Incorrect words, check if we are allowed to flash them */
    if (flash_fuses == FALSE)
    {
        return RETURN_NOK;
    }
    
    /* We are allowed to program, check for security bit */
    if (NVMCTRL->STATUS.reg & NVMCTRL_STATUS_SB)
    {
        return RETURN_NOK;
    }
    
    /* Backup NVMCTRL CTRLB settings */
    uint32_t temp = NVMCTRL->CTRLB.reg;
    
    /* Automatic write, disable caching */
    NVMCTRL->CTRLB.bit.MANW = 0;
    NVMCTRL->CTRLB.bit.CACHEDIS = 1;

    /* Clear error flags */
    NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK;

    /* Set address, command will be issued elsewhere */
    NVMCTRL->ADDR.reg = NVMCTRL_AUX0_ADDRESS / 2;

    /* Erase the user page */
    NVMCTRL->CTRLA.reg = NVM_COMMAND_ERASE_AUX_ROW | NVMCTRL_CTRLA_CMDEX_KEY;

    /* Wait for NVM command to complete */
    while (!(NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY));

    /* Clear error flags */
    NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK;

    /* Set address, command will be issued elsewhere */
    NVMCTRL->ADDR.reg = NVMCTRL_AUX0_ADDRESS / 2;

    /* Erase the page buffer before buffering new data */
    NVMCTRL->CTRLA.reg = NVM_COMMAND_PAGE_BUFFER_CLEAR | NVMCTRL_CTRLA_CMDEX_KEY;

    /* Wait for NVM command to complete */
    while (!(NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY));

    /* Clear error flags */
    NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK;

    /* Set address, command will be issued elsewhere */
    NVMCTRL->ADDR.reg = NVMCTRL_AUX0_ADDRESS / 2;
    
    /* Prepare words to be written */
    userWord0 &= USER_MASK_0;
    userWord1 &= USER_MASK_1;
    userWord0 |= USER_WORD_0;
    userWord1 |= USER_WORD_1;

    /* write the fuses values to the memory buffer */
    *((uint32_t *)NVMCTRL_AUX0_ADDRESS) = userWord0;
    *(((uint32_t *)NVMCTRL_AUX0_ADDRESS) + 1) = userWord1;

    /* Write the user page */
    NVMCTRL->CTRLA.reg = NVM_COMMAND_WRITE_AUX_ROW | NVMCTRL_CTRLA_CMDEX_KEY;

    /* Wait for NVM command to complete */
    while (!(NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY));
    
    /* Set security bit */
    #ifndef NO_SECURITY_BIT_CHECK
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_SSB | NVMCTRL_CTRLA_CMDEX_KEY;
    while (!(NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY));
    #endif
    
    /* Restore NVM CTRLB settings */
    NVMCTRL->CTRLB.reg = temp;
    
    /* Reboot, needed to reload security bit status */
    NVIC_SystemReset();
    while(1);
    
    /* To avoid warnings */
    return RETURN_NOK;
}
