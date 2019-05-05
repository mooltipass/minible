/*!  \file     logic_security.c
*    \brief    General logic for security (credentials etc)
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include "logic_security.h"
/* Inserted card unlocked */
volatile BOOL logic_security_smartcard_inserted_unlocked = FALSE;
/* Memory management mode */
volatile BOOL logic_security_management_mode = FALSE;


/*! \fn     logic_security_clear_security_bools(void)
*   \brief  Clear all security booleans
*/
void logic_security_clear_security_bools(void)
{
    logic_security_smartcard_inserted_unlocked = FALSE;
    logic_security_management_mode = FALSE;
    // TODO2
    /*
    context_valid_flag = FALSE;
    selected_login_flag = FALSE;
    login_just_added_flag = FALSE;
    leaveMemoryManagementMode();
    data_context_valid_flag = FALSE;
    current_adding_data_flag = FALSE;
    activateTimer(TIMER_CREDENTIALS, 0);
    smartcard_inserted_unlocked = FALSE;
    currently_writing_first_block = FALSE;
    */
}

/*! \fn     logic_security_smartcard_unlocked_actions(void)
*   \brief  Actions when smartcard is unlocked
*/
void logic_security_smartcard_unlocked_actions(void)
{    
    cpu_irq_enter_critical();
    logic_security_smartcard_inserted_unlocked = TRUE;
    logic_security_management_mode = FALSE;
    cpu_irq_leave_critical();
}

/*! \fn     logic_security_is_smc_inserted_unlocked(void)
*   \brief  Know if inserted smartcard is unlocked
*   \return The boolean
*/
BOOL logic_security_is_smc_inserted_unlocked(void)
{
    return logic_security_smartcard_inserted_unlocked;
}

/*! \fn     logic_security_set_management_mode(void)
*   \brief  Set device into management mode
*/
void logic_security_set_management_mode(void)
{
    logic_security_management_mode = TRUE;
}

/*! \fn     logic_security_clear_management_mode(void)
*   \brief  Exit management mode
*/
void logic_security_clear_management_mode(void)
{
    logic_security_management_mode = FALSE;
}

/*! \fn     logic_security_is_management_mode_set(void)
*   \brief  Check if device is in MMM
*   \return The boolean
*/
BOOL logic_security_is_management_mode_set(void)
{
    return logic_security_management_mode;
}