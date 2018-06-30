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
#include "defines.h"
#include "inputs.h"
/* Timer array */
volatile timerEntry_t context_timers[TOTAL_NUMBER_OF_TIMERS];
/* System tick */
volatile uint32_t sysTick;


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
        }
    #endif
}

/*! \fn     timer_initialize_timebase(void)
*   \brief  Initialize the platform time base
*   \note   Will use GCLK3 for a 1.024KHz and uses the RTC module in calendar mode
*/
void timer_initialize_timebase(void)
{        
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
    
    /* Setup RTC module in calendar mode */
    while((RTC->MODE2.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);          // Wait for sync
    RTC->MODE2.CTRL.reg = 0;                                            // Disable RTC
    while((RTC->MODE2.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);          // Wait for sync
    RTC->MODE2.CTRL.reg = RTC_MODE2_CTRL_SWRST;                         // Reset RTC
    while((RTC->MODE2.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);          // Wait for sync
    while((RTC->MODE2.CTRL.reg & RTC_MODE2_CTRL_SWRST) != 0);           // Wait for end of reset
    RTC_MODE2_CTRL_Type rtc_ctrl_reg;                                   // rtc ctrl struct
    rtc_ctrl_reg.reg = RTC_MODE1_CTRL_ENABLE;                           // Enable module
    rtc_ctrl_reg.bit.CLKREP = 0;                                        // 24 hour mode
    rtc_ctrl_reg.bit.MODE = RTC_MODE2_CTRL_MODE_CLOCK_Val;              // Calendar mode
    rtc_ctrl_reg.bit.PRESCALER = RTC_MODE2_CTRL_PRESCALER_DIV1024_Val;  // Divide 1.024kHz signal by 1024
    while((RTC->MODE2.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);          // Wait for sync
    RTC->MODE2.CTRL = rtc_ctrl_reg;                                     // Write register
    //RTC->MODE2.DBGCTRL.bit.DBGRUN = 1;                                  // Allow normal operation during debug mode
    
    /* Set GCLK Multiplexer 0x1A (for GCLK_TCC0) to 48MHz GCLK0 */
    clocks_map_gclk_to_peripheral_clock(GCLK_ID_48M, GCLK_CLKCTRL_ID_TCC0_TCC1_Val);
    
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
}

/*!	\fn		timer_get_calendar(void)
*	\brief	Get the current date
*   \param  calendar_pt Pointer to a calendar struct
*/
void timer_get_calendar(calendar_t* calendar_pt)
{
    while((RTC->MODE2.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);          // Wait for sync
    RTC->MODE2.READREQ.reg |= RTC_READREQ_RREQ;                         // Trigger read request
    while((RTC->MODE2.STATUS.reg & RTC_STATUS_SYNCBUSY) != 0);          // Wait for sync
    *calendar_pt = RTC->MODE2.CLOCK;                                    // Store current time
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
    timer_start_timer(TIMER_WAIT_FUNCTS, ms+1);
    while(timer_has_timer_expired(TIMER_WAIT_FUNCTS, TRUE) != TIMER_EXPIRED);
}
