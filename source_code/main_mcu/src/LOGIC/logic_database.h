/*!  \file     logic_database.h
*    \brief    General logic for database operations
*    Created:  17/03/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_DATABASE_H_
#define LOGIC_DATABASE_H_

#include "defines.h"


/* Prototypes */
uint16_t logic_database_search_service(cust_char_t* name, service_compare_mode_te compare_type, BOOL cred_type, uint16_t data_category_id);
uint16_t logic_database_search_login_in_service(uint16_t parent_addr, cust_char_t* login);


#endif /* LOGIC_DATABASE_H_ */