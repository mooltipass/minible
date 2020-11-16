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
/*
 * timer.c
 *
 * Created: 19/04/2017 09:39:16
 *  Author: stephan
 */ 
#include <asf.h>
#include "smartcard_lowlevel.h"
#include "platform_defines.h"
#include "driver_clocks.h"
#include "driver_timer.h"
#include "logic_device.h"
#include "logic_power.h"
#include "platform_io.h"
#include "logic_user.h"
#include "inputs.h"
#include "main.h"

#ifdef EMULATOR_BUILD
#include "emulator.h"
#include <time.h>
/* Difference between system time and emulated RTC
 * rtc_time = time(NULL) + rtc_offset
 */
static int rtc_offset;
#endif
/* Timer array */
#define NUMBER_OF_ALLOCATABLE_TIMERS    3
volatile allocatedTimerEntry_t context_allocatable_timers[NUMBER_OF_ALLOCATABLE_TIMERS];
volatile timerEntry_t context_timers[TOTAL_NUMBER_OF_TIMERS];
/* Inactivity timer remaining ticks */
volatile uint16_t timer_inactivity_30mins_tick_remain = 0;
/* Bool set when MCU systic expired */
volatile BOOL timer_systick_expired = TRUE;
/* System tick */
volatile uint32_t sysTick;
/* timestamp set at the last "set date" message */
uint32_t timer_last_set_timestamp = 0;
/* Default value for 32K oscillator calibration */
volatile uint32_t timer_default_OSCULP32K_calib_val;
/* Fine adjustment for our time base */
volatile int32_t timer_accumulated_corrections = 0;
volatile uint32_t timer_nb_30mins_interrupts = 0;
volatile BOOL timer_fine_adjust_positive;
volatile uint32_t timer_fine_adjust = 0;
#ifdef EMULATOR_BUILD
uint32_t timer_emulator_fake_rtc_cnt = 0;
#endif


#ifndef EMULATOR_BUILD
/*! \fn     TCC0_Handler(void)
*   \brief  Called every ms by interrupt
*/
void TCC0_Handler(void)
{
    #ifndef BOOTLOADER    
        if (TCC0->INTFLAG.reg & TCC_INTFLAG_OVF)
        {
            /* Overflow interrupt: clear flag */
            TCC0->INTFLAG.reg = TCC_INTFLAG_OVF;
        
            /* Timer ms tick */
            timer_ms_tick();

            /* Smartcard detect */
            smartcard_lowlevel_detect();
        
            /* Scan buttons */
            inputs_scan();
            
            /* Scan 3v3 input */
            platform_io_scan_3v3();
            
            /* Power logic */
            logic_power_ms_tick();
        }
    #endif
}

/*! \fn     TCC2_Handler(void)
*   \brief  Called every 20minutes by interrupt
*/
void TCC2_Handler(void)
{
    #ifndef BOOTLOADER
    if (TCC2->INTFLAG.reg & TCC_INTFLAG_OVF)
    {
        /* Overflow interrupt: clear flag */
        TCC2->INTFLAG.reg = TCC_INTFLAG_OVF;
        
        /* In case this wakes up device, setup the flag */
        logic_device_set_wakeup_reason(WAKEUP_REASON_30M_TIMER);
        
        /* Logic power 30min tick */
        logic_power_30m_tick();
        
        /* Deal with inactivity logoff timer */
        if (timer_inactivity_30mins_tick_remain != 0)
        {
            if (timer_inactivity_30mins_tick_remain-- == 1)
            {
                logic_user_set_user_to_be_logged_off_flag();
            }
        }
        
        /* Timestamp adjustment */
        if (((timer_nb_30mins_interrupts & 0x01) == 0) && ((timer_fine_adjust & 0x01) != 0))
        {
            if (timer_fine_adjust_positive == FALSE)
            {
                timer_accumulated_corrections -= 1;
            } 
            else
            {
                timer_accumulated_corrections += 1;
            }
        }
        if (timer_fine_adjust_positive == FALSE)
        {
            timer_accumulated_corrections -= timer_fine_adjust/2;
        } 
        else
        {
            timer_accumulated_corrections += timer_fine_adjust/2;
        }
        timer_nb_30mins_interrupts++;
    }
    #endif    
}

#endif

/*! \fn     timer_get_fine_adjust(void)
*   \brief  Get current 30mins sec adjusts
*   \return By how many secs we adjust the rtc every 30s
*/
int32_t timer_get_fine_adjust(void)
{
    if (timer_fine_adjust_positive == FALSE)
    {
        return -((int32_t)timer_fine_adjust);
    }
    else
    {
        return (int32_t)timer_fine_adjust;
    }
}

/*! \fn     timer_start_logoff_timer(uint16_t nb_30mins_ticks_before_lock)
*   \brief  Start inactivity logoff timer
*   \param  nb_30mins_ticks_before_lock     Number of 30mins ticks before lock
*/
void timer_start_logoff_timer(uint16_t nb_30mins_ticks_before_lock)
{
    cpu_irq_enter_critical();
    timer_inactivity_30mins_tick_remain = nb_30mins_ticks_before_lock + 1;
    cpu_irq_leave_critical();
}

/*! \fn     timer_initialize_timebase(void)
*   \brief  Initialize the platform time base
*   \note   Will use GCLK3 for a 1.024KHz and uses the RTC module in calendar mode
*/
void timer_initialize_timebase(void)
{
#ifndef EMULATOR_BUILD            
    /* Assign internal 32kHz to GCLK3, divide it by 32 */
    GCLK_GENDIV_Type gendiv_reg;                                        // Gendiv struct
    gendiv_reg.bit.ID = GCLK_CLKCTRL_GEN_GCLK3_Val;                     // Select gclk3
    gendiv_reg.bit.DIV = 32;                                            // Divide by 32
    while ((GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) != 0);             // Wait for sync
    GCLK->GENDIV = gendiv_reg;                                          // Write register
    GCLK_GENCTRL_Type genctrl;                                          // Genctrl struct
    genctrl.reg = 0;                                                    // Reset temp var
    genctrl.bit.ID = GCLK_CLKCTRL_GEN_GCLK3_Val;                        // Select gclk3
    genctrl.bit.SRC = GCLK_GENCTRL_SRC_OSCULP32K_Val;                   // Assign ultra low power 32kHz
    genctrl.bit.GENEN = 1;                                              // Enable generator
    genctrl.bit.DIVSEL = 0;                                             // Divide clock by gendiv.div
    genctrl.bit.OE = 1;                                                 // Do not output clock
    genctrl.bit.RUNSTDBY = 1;                                           // Run in standby
    while ((GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) != 0);             // Wait for sync
    GCLK->GENCTRL = genctrl;                                            // Write register
    
    /* Set GCLK Multiplexer 4 (for GCLK_RTC) to 1kHz GCLK3 */
    GCLK_CLKCTRL_Type clkctrl;                                          // Clkctrl struct
    clkctrl.reg = GCLK_CLKCTRL_ID_RTC;                                  // Disable RTC clock generator
    clkctrl.bit.GEN = GCLK_CLKCTRL_GEN_GCLK2_Val;                       // Default value
    while ((GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) != 0);             // Wait for sync
    GCLK->CLKCTRL = clkctrl;                                            // Write register    
    while ((GCLK->CLKCTRL.reg & GCLK_CLKCTRL_CLKEN) != 0);              // Wait for clock disable
    clkctrl.reg = 0;                                                    // Reset temp var
    clkctrl.bit.ID = GCLK_CLKCTRL_ID_RTC_Val;                           // Select RTC input
    clkctrl.bit.GEN = GCLK_CLKCTRL_GEN_GCLK3_Val;                       // Select gclk3
    clkctrl.bit.CLKEN = 1;                                              // Enable generator
    while ((GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) != 0);             // Wait for sync
    GCLK->CLKCTRL = clkctrl;                                            // Write register
    
    /* Setup RTC module in 32bits counter mode */
    while((RTC->MODE0.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);          // Wait for sync
    RTC->MODE0.CTRL.reg = 0;                                            // Disable RTC
    while((RTC->MODE0.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);          // Wait for sync
    RTC->MODE0.CTRL.reg = RTC_MODE0_CTRL_SWRST;                         // Reset RTC
    while((RTC->MODE0.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);          // Wait for sync
    while((RTC->MODE0.CTRL.reg & RTC_MODE0_CTRL_SWRST) != 0);           // Wait for end of reset
    RTC_MODE0_CTRL_Type rtc_ctrl_reg;                                   // rtc ctrl struct
    rtc_ctrl_reg.reg = RTC_MODE0_CTRL_ENABLE;                           // Enable module
    rtc_ctrl_reg.bit.MODE = RTC_MODE0_CTRL_MODE_COUNT32_Val;            // 32bits counter mode
    rtc_ctrl_reg.bit.PRESCALER = RTC_MODE0_CTRL_PRESCALER_DIV1024_Val;  // Divide 1.024kHz signal by 1024
    while((RTC->MODE0.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);          // Wait for sync
    RTC->MODE0.CTRL = rtc_ctrl_reg;                                     // Write register
    //RTC->MODE0.DBGCTRL.bit.DBGRUN = 1;                                  // Allow normal operation during debug mode
    
    /* Set GCLK Multiplexer 0x1A (for GCLK_TCC0) to 48MHz GCLK0 */
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, GCLK_CLKCTRL_ID_TCC0_TCC1_Val);
    
    /* Set GCLK 0x1B (for GCLK_TCC2) to 1kHz GCLK3 */
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_32K, GCLK_CLKCTRL_ID_TCC2_TC3_Val);
    
    /* Setup TCC0 for 1ms interrupt */
    PM->APBCMASK.bit.TCC0_ = 1;                                         // Enable APBC clock for TCC0
    while(TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_PER);                       // Wait for sync
    TCC0->PER.reg = TCC_PER_PER(48000-1);                               // Set period to be 48M/48000 = 1k
    TCC_CTRLA_Type tcc_ctrl_reg;                                        // tcc ctrl reg
    tcc_ctrl_reg.reg = TCC_CTRLA_ENABLE;                                // Enable tcc0
    tcc_ctrl_reg.bit.RUNSTDBY = 0;                                      // Do not run during standby
    tcc_ctrl_reg.bit.PRESCALER = TCC_CTRLA_PRESCALER_DIV1_Val;          // No prescaling
    while(TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE);                    // Wait for sync
    TCC0->CTRLA = tcc_ctrl_reg;                                         // Write register
    TCC0->INTENSET.reg = TCC_INTENSET_OVF;                              // Enable overflow interrupt
    NVIC_EnableIRQ(TCC0_IRQn);                                          // Enable int
    
    /* Setup TCC2 for 34 minutes (!) interrupt */
    PM->APBCMASK.bit.TCC2_ = 1;                                         // Enable APBC clock for TCC2
    while(TCC2->SYNCBUSY.reg & TCC_SYNCBUSY_PER);                       // Wait for sync
    TCC2->PER.reg = TCC_PER_PER((SET_DATE_MSG_INTERVAL_S/2)-1);         // Set period to be 1024/1024/2048 = 34mins
    tcc_ctrl_reg.reg = TCC_CTRLA_ENABLE;                                // Enable tcc0
    tcc_ctrl_reg.bit.RUNSTDBY = 1;                                      // Run during standby
    tcc_ctrl_reg.bit.PRESCALER = TCC_CTRLA_PRESCALER_DIV1024_Val;       // 1024 prescaling to have a 1sec tick
    while(TCC2->SYNCBUSY.reg & TCC_SYNCBUSY_ENABLE);                    // Wait for sync
    TCC2->CTRLA = tcc_ctrl_reg;                                         // Write register
    TCC2->INTENSET.reg = TCC_INTENSET_OVF;                              // Enable overflow interrupt
    NVIC_EnableIRQ(TCC2_IRQn);                                          // Enable int
    
    /* Store default osculp32k calibration value */
    timer_default_OSCULP32K_calib_val = SYSCTRL->OSCULP32K.bit.CALIB;
#endif
}

/*!	\fn		driver_timer_get_rtc_timestamp_uin32t(void)
*	\brief	Get current RTC timestamp
*   \return Something sad
*/
uint32_t driver_timer_get_rtc_timestamp_uint32t(void)
{
#ifndef EMULATOR_BUILD
    cpu_irq_enter_critical();
    
    /* Get current time stamp */
    while((RTC->MODE0.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);
    uint32_t current_timestamp = RTC->MODE0.COUNT.reg;
    
    /* Add timestamp from 1/1/2020 */
    current_timestamp += 1577836800;
    
    /* Add correction: coarse */
    current_timestamp += timer_accumulated_corrections;
    
    /* Add correction: fine */
    while(TCC2->SYNCBUSY.reg & TCC_SYNCBUSY_COUNT);
    uint32_t timer_counter_val = (uint32_t)TCC2->COUNT.bit.COUNT;
    
    /* Scale it down */
    timer_counter_val = (timer_counter_val * timer_fine_adjust) / SET_DATE_MSG_INTERVAL_S;
    if (timer_fine_adjust_positive == FALSE)
    {
        current_timestamp -= timer_counter_val;
    } 
    else
    {
        current_timestamp += timer_counter_val;
    }
    
    cpu_irq_leave_critical();
    return current_timestamp;
#else
    return timer_emulator_fake_rtc_cnt + 1577836800;
#endif
}

/*!	\fn		driver_timer_get_rtc_timestamp_uin32t(void)
*	\brief	Get current RTC timestamp
*   \return Something sad
*/
uint64_t driver_timer_get_rtc_timestamp_uint64t(void)
{
#ifndef EMULATOR_BUILD
    cpu_irq_enter_critical();

    /* Get current time stamp */
    while((RTC->MODE0.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);
    uint64_t current_timestamp = RTC->MODE0.COUNT.reg;
    
    /* Add timestamp from 1/1/2020 */
    current_timestamp += 1577836800;
    
    /* Add correction: coarse */
    current_timestamp += timer_accumulated_corrections;
    
    /* Add correction: fine */
    while(TCC2->SYNCBUSY.reg & TCC_SYNCBUSY_COUNT);
    uint32_t timer_counter_val = (uint32_t)TCC2->COUNT.bit.COUNT;
    
    /* Scale it down */
    timer_counter_val = (timer_counter_val * timer_fine_adjust) / SET_DATE_MSG_INTERVAL_S;
    if (timer_fine_adjust_positive == FALSE)
    {
        current_timestamp -= timer_counter_val;
    }
    else
    {
        current_timestamp += timer_counter_val;
    }
    
    cpu_irq_leave_critical();
    return current_timestamp;
#else
    return (uint64_t)timer_emulator_fake_rtc_cnt + 1577836800;
#endif
}

/*!	\fn		driver_timer_set_rtc_timestamp(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second)
*	\brief	Set our RTC timestamp (kinda)
*   \param  year    Current year
*   \param  month   Current month
*   \param  day     Current day
*   \param  hour    Current hour
*   \param  minute  Current minute
*   \param  second  Current second
*/
void driver_timer_set_rtc_timestamp(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second)
{
    uint8_t is_curr_year_leap = IS_LEAP_YEAR(year);
    uint32_t num_days = 0;
    
#ifndef EMULATOR_BUILD
    /* Get current time stamp */
    while((RTC->MODE0.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);
    uint32_t current_timestamp = RTC->MODE0.COUNT.reg;
#endif

    /* UNIX time does NOT include LEAP seconds. See https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_xbd_chap04.html#tag_21_04_16 */
    /* use RTC timer to count number of seconds since 1/1/2020 */
    for (uint16_t i = 2020; i < year; ++i)
    {
        num_days += IS_LEAP_YEAR(i) ? 366 : 365;
    }

    /* Add up days in all months */
    for (uint16_t i = 0; i < month - 1; ++i)
    {
        num_days += driver_timer_dph[i];
        num_days += ((i == LEAP_MONTH) && is_curr_year_leap) ? 1 : 0;
    }

    /* Add days in month */
    num_days += day - 1;

    /* Add the days hours, minutes, and seconds */
    uint32_t kinda_unix_time = num_days * SEC_IN_DAY;
    kinda_unix_time += hour * SEC_IN_HOUR;
    kinda_unix_time += minute * 60;
    kinda_unix_time += second;
    
#ifndef EMULATOR_BUILD
    /* Store timestamp */
    while((RTC->MODE0.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);
    RTC->MODE0.COUNT.reg = kinda_unix_time;
    
    /* Compute difference between our timebase and the one just sent */
    int32_t nb_seconds_difference = (int32_t)(kinda_unix_time - current_timestamp);
    
    /* Check for message sent every 4096s */
    if ((timer_last_set_timestamp + 4094 < kinda_unix_time) && (timer_last_set_timestamp + 4098 > kinda_unix_time))
    {        
        /* Sanity check */
        if ((nb_seconds_difference > -300) && (nb_seconds_difference < 300))
        {
            /* Coarse adjustment: calib register */
            int32_t calib_register_offset = nb_seconds_difference/113;
            SYSCTRL->OSCULP32K.bit.CALIB += calib_register_offset;
            nb_seconds_difference -= calib_register_offset*113;
            
            /* Pause interrupts */
            cpu_irq_enter_critical();
            
            /* Get counter value before realign */
            while(TCC2->SYNCBUSY.reg & TCC_SYNCBUSY_COUNT);
            uint32_t counter_before_realign = TCC2->COUNT.bit.COUNT;
            
            /* Re-align the TCC2 count as it has a period of 4096/2s and this is be called every hour */
            while(TCC2->SYNCBUSY.reg & TCC_SYNCBUSY_COUNT);
            TCC2->COUNT.bit.COUNT = 0;
            
            /* If realignment is making us miss an interrupt, trigger the code */
            if (counter_before_realign > 1000)
            {
                /* Logic power 30min tick */
                logic_power_30m_tick();
                
                /* Deal with inactivity logoff timer */
                if (timer_inactivity_30mins_tick_remain != 0)
                {
                    if (timer_inactivity_30mins_tick_remain-- == 1)
                    {
                        logic_user_set_user_to_be_logged_off_flag();
                    }
                }
            }
            
            /* Overflow interrupt: clear potential flag */
            TCC2->INTFLAG.reg = TCC_INTFLAG_OVF;
            
            /* Resume interrupts */
            cpu_irq_leave_critical();
            
            /* Adjust fine adjustment */
            timer_fine_adjust_positive = TRUE;
            timer_fine_adjust = (uint32_t)nb_seconds_difference;
            if (nb_seconds_difference < 0)
            {
                timer_fine_adjust_positive = FALSE;
                timer_fine_adjust = (uint32_t)(-nb_seconds_difference);
            }
        }
        else
        {
            /* Reset default calibration value */
            SYSCTRL->OSCULP32K.bit.CALIB = timer_default_OSCULP32K_calib_val;
            timer_fine_adjust = 0;
        }
    }
    else
    {
    }
#endif
    
    /* Store last set timestamp */
    timer_last_set_timestamp = kinda_unix_time;
    
    /* Pause interrupts */
    cpu_irq_enter_critical();
    
    /* Reset accumulated correction count */
    timer_accumulated_corrections = 0;
    timer_nb_30mins_interrupts = 0;
    
    /* Resume interrupts */
    cpu_irq_leave_critical();
    
#ifdef EMULATOR_BUILD
    timer_emulator_fake_rtc_cnt = timer_last_set_timestamp;
#endif
}

/*!	\fn		timer_ms_tick(void)
*	\brief	Function called by interrupt every ms
*/
void timer_ms_tick(void)
{
    uint32_t i;
    sysTick++;
    
    // Loop through the timers
    for (i = 0; i < TOTAL_NUMBER_OF_TIMERS; i++)
    {
        if (context_timers[i].timer_val != 0)
        {
            if (context_timers[i].timer_val-- == 1)
            {
                context_timers[i].flag = TIMER_EXPIRED;
            }
        }
    }
    
    // Loop through the allocated timers
    for (i = 0; i < NUMBER_OF_ALLOCATABLE_TIMERS; i++)
    {
        if (context_allocatable_timers[i].timer_val != 0)
        {
            if (context_allocatable_timers[i].timer_val-- == 1)
            {
                context_allocatable_timers[i].flag = TIMER_EXPIRED;
            }
        }
    }
    
    #ifdef EMULATOR_BUILD
    timer_emulator_fake_rtc_cnt++;
    #endif
}

#ifndef EMULATOR_BUILD
/*!	\fn		SysTick_Handler(void)
*	\brief	Called by MCU systick at timeout
*/
void SysTick_Handler(void)
{
    /* Disable systick */
    SysTick->CTRL = 0;
    timer_systick_expired = TRUE;
}
#endif

/*!	\fn		timer_wait_for_aux_tx_flood_protection(void)
*	\brief	Wait for the MCU systick timeout
*/
void timer_wait_for_aux_tx_flood_protection(void)
{
    while(timer_systick_expired == FALSE);
}

/*!	\fn		timer_arm_mcu_systick_for_aux_tx_flood_protection(void)
*	\brief	Arm MCU systick in countdown mode to implement a timeout
*   \note   This function is called by interrupt from the DMA TX done
*/
void timer_arm_mcu_systick_for_aux_tx_flood_protection(void)
{
    #ifndef EMULATOR_BUILD
    /* Reset Booleans */
    timer_systick_expired = FALSE;
    
    /* Set defined timeout value */
    SysTick->LOAD = MCU_SYSTICK_VAL_FOR_AUX_RX_TO;
    SysTick->VAL = 0;
    
    /* Use reference clock, generate interrupt and enable counter */
    SysTick->CTRL = 0x03;
    #endif
}

/*!	\fn		timer_get_mcu_systick(uint32_t* value)
*	\brief	Get MCU systick
*   \param  value   Pointer to where to store the value
*   \return Bool indicating if the counter has overflowed
*/
BOOL timer_get_mcu_systick(uint32_t* value)
{
#ifndef EMULATOR_BUILD
    *value = SysTick->VAL & 0x00FFFFFF;
    
    if ((SysTick->CTRL & 0x10000) != 0)
    {
        return TRUE;
    } 
    else
    {
        return FALSE;
    }
#else
    /* for portability, use qt's timers */
    return emu_get_systick(value);
#endif
}

/*!	\fn		timer_get_systick(void)
*	\brief	Get system timer
*   \return The system time in ms since boot
*/
uint32_t timer_get_systick(void)
{
    return sysTick;
}

/*!	\fn		timer_has_timer_expired(timer_id_te uid, BOOL clear)
*	\brief	Know if a timer expired and clear the flag if so
*   \param  uid     Unique ID
*   \param  clear   Boolean to say if we clear the flag
*   \return TIMER_EXPIRED or TIMER_RUNNING (see enum)
*/
timer_flag_te timer_has_timer_expired(timer_id_te uid, BOOL clear)
{
    // Compare & write is done in one cycle
    if (context_timers[uid].flag == TIMER_EXPIRED)
    {
        if (clear == TRUE)
        {
            context_timers[uid].flag = TIMER_RUNNING;
        }
        return TIMER_EXPIRED;
    }
    else
    {
        return TIMER_RUNNING;
    }
}

/*! \fn     timer_has_allocated_timer_expired(uint16_t uid, BOOL clear)
*   \brief  Know if a timer expired and clear the flag if so
*   \param  uid     Unique ID
*   \param  clear   Boolean to say if we clear the flag
*   \return TIMER_EXPIRED or TIMER_RUNNING (see enum)
*/
timer_flag_te timer_has_allocated_timer_expired(uint16_t uid, BOOL clear)
{
    // Check for valid uid
    if (uid >= NUMBER_OF_ALLOCATABLE_TIMERS)
    {
        main_reboot();
    }
    
    // Compare & write is done in one cycle
    if (context_allocatable_timers[uid].flag == TIMER_EXPIRED)
    {
        if (clear == TRUE)
        {
            context_allocatable_timers[uid].flag = TIMER_RUNNING;
        }
        return TIMER_EXPIRED;
    }
    else
    {
        return TIMER_RUNNING;
    }
}

/*! \fn     timer_rearm_allocated_timer(uint16_t uid, BOOL clear)
*   \brief	Rearm an allocated timer
*   \param  uid Allocated timer id
*   \param  val Delay in ms
*/
void timer_rearm_allocated_timer(uint16_t uid, uint32_t val)
{
    // Check for valid uid
    if (uid >= NUMBER_OF_ALLOCATABLE_TIMERS)
    {
        main_reboot();
    }
    
    // Compare is done in one cycle
    if (context_allocatable_timers[uid].timer_val != val)
    {
        cpu_irq_enter_critical();
        
        context_allocatable_timers[uid].timer_val = val;
        if (val == 0)
        {
            context_allocatable_timers[uid].flag = TIMER_EXPIRED;
        }
        else
        {
            context_allocatable_timers[uid].flag = TIMER_RUNNING;
        }
        
        cpu_irq_leave_critical();
    }
}

/*! \fn     timer_get_and_start_timer(uint32_t val)
*   \brief	Get and start a timer
*   \param  val Delay in ms
*   \return A timer number, to be freed later
*   \note   To not f*ck everything up, allocate and free in the same calling function
*/
uint16_t timer_get_and_start_timer(uint32_t val)
{
    /* Loop over our timers */
    for (uint16_t i = 0; i < NUMBER_OF_ALLOCATABLE_TIMERS; i++)
    {
        /* Check for allocation */
        if (context_allocatable_timers[i].allocated == FALSE)
        {
            // Compare is done in one cycle
            if (context_allocatable_timers[i].timer_val != val)
            {
                cpu_irq_enter_critical();
                
                context_allocatable_timers[i].timer_val = val;
                if (val == 0)
                {
                    context_allocatable_timers[i].flag = TIMER_EXPIRED;
                }
                else
                {
                    context_allocatable_timers[i].flag = TIMER_RUNNING;
                }
                
                cpu_irq_leave_critical();
            }
            
            /* Set allocated flag, return uid */
            context_allocatable_timers[i].allocated = TRUE;
            return i;
        }
    }
    
    /* No timer found, reboot */
    main_reboot();
    return 0;
}

/*!	\fn		timer_deallocate_timer(uint16_t timer_id)
*	\brief	Deallocate previously provided timer
*   \param  timer_id    The timer id
*/
void timer_deallocate_timer(uint16_t timer_id)
{
    // Check for valid uid
    if (timer_id >= NUMBER_OF_ALLOCATABLE_TIMERS)
    {
        main_reboot();
    }
    
    // Reset flag
    context_allocatable_timers[timer_id].allocated = FALSE;
}

/*!	\fn		timer_start_timer(timer_id_te uid, uint32_t val)
*	\brief	Start timer
*   \param  uid Unique ID
*   \param  val Delay in ms
*/
void timer_start_timer(timer_id_te uid, uint32_t val)
{    
    // Compare is done in one cycle
    if (context_timers[uid].timer_val != val)
    {
        cpu_irq_enter_critical();
        
        context_timers[uid].timer_val = val;
        if (val == 0)
        {
            context_timers[uid].flag = TIMER_EXPIRED;
        }
        else
        {
            context_timers[uid].flag = TIMER_RUNNING;
        }
        
        cpu_irq_leave_critical();
    }
}

/*!	\fn		timer_get_timer_val(timer_id_te uid)
*	\brief	Get current timer val
*   \param  uid     Unique ID
*   \return the timer val
*/
uint32_t timer_get_timer_val(timer_id_te uid)
{
    return context_timers[uid].timer_val;
}

/*!	\fn		timer_delay_ms(uint32_t ms)
*	\brief	Timer based ms delay
*   \param  ms  Number of ms
*/
void timer_delay_ms(uint32_t ms)
{
#ifndef BOOTLOADER
    timer_start_timer(TIMER_WAITING_FUNCT, ms+1);
    while(timer_has_timer_expired(TIMER_WAITING_FUNCT, TRUE) != TIMER_EXPIRED);
#else
    DELAYMS(ms);
#endif
}
