/*!  \file     logic_security.c
*    \brief    General logic for security (credentials etc)
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/
#include "logic_security.h"
#include "defines.h"


/*! \fn     logic_security_clear_security_bools(void)
*   \brief  Clear all security booleans
*/
void logic_security_clear_security_bools(void)
{
    // TODO
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
    // TODO        
    /*
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
    smartcard_inserted_unlocked = TRUE;
    }
    */
}