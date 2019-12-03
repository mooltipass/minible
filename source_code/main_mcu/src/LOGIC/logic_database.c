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
/*!  \file     logic_database.c
*    \brief    General logic for database operations
*    Created:  17/03/2019
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include "logic_database.h"
#include "gui_dispatcher.h"
#include "nodemgmt.h"
#include "utils.h"


/*! \fn     logic_database_get_prev_2_fletters_services(uint16_t start_address, cust_char_t start_char, cust_char_t* char_array)
*   \brief  Get the previous 2 services with different first letters
*   \param  start_address       Address at which we should start looking
*   \param  start_char          The current first char
*   \param  char_array          An array for 2 chars
*   \return Address of the next node having a different first letter
*/
uint16_t logic_database_get_prev_2_fletters_services(uint16_t start_address, cust_char_t start_char, cust_char_t* char_array)
{
    uint16_t previous_address = start_address;
    uint16_t return_value = NODE_ADDR_NULL;
    BOOL skip_first_change_bool = TRUE;
    cust_char_t cur_char = start_char;
    uint16_t current_node_addr;
    int16_t storage_index = 1;
    parent_node_t temp_pnode;
    
    /* To start with the loop below */
    temp_pnode.cred_parent.prevParentAddress = start_address;
    char_array[0] = ' '; char_array[1] = ' ';
    
    do
    {
        /* Update current node address */
        current_node_addr = temp_pnode.cred_parent.prevParentAddress;
        
        /* Read Node */
        nodemgmt_read_parent_node(current_node_addr, &temp_pnode, FALSE);
        
        /* Check if the fchar changed */
        if ((temp_pnode.cred_parent.service[0] != cur_char) && (nodemgmt_check_for_logins_with_category_in_parent_node(temp_pnode.cred_parent.nextChildAddress, nodemgmt_get_current_category_flags()) != NODE_ADDR_NULL))
        {            
            if (skip_first_change_bool == FALSE)
            {
                char_array[storage_index--] = cur_char;
                
                /* First next letter, store address */
                if (storage_index == 0)
                {
                    return_value = previous_address;
                }
                
                /* Did we fill the array? */
                if (storage_index == -1)
                {
                    break;
                }
            } 
            else
            {
                skip_first_change_bool = FALSE;
            }
            
            previous_address = current_node_addr;
            cur_char = temp_pnode.cred_parent.service[0];
        }    
    } while (temp_pnode.cred_parent.prevParentAddress != NODE_ADDR_NULL);
    
    /* Check if we are at the first parent node and therefore need to store address & chars */
    if (temp_pnode.cred_parent.prevParentAddress == NODE_ADDR_NULL)
    {
        if (((storage_index == 1) && (temp_pnode.cred_parent.service[0] != start_char)) && (nodemgmt_check_for_logins_with_category_in_parent_node(temp_pnode.cred_parent.nextChildAddress, nodemgmt_get_current_category_flags()) != NODE_ADDR_NULL))
        {
            char_array[1] = temp_pnode.cred_parent.service[0];
            return_value = current_node_addr;
        } 
        else if (((storage_index == 0) && (temp_pnode.cred_parent.service[0] != char_array[1])) && (nodemgmt_check_for_logins_with_category_in_parent_node(temp_pnode.cred_parent.nextChildAddress, nodemgmt_get_current_category_flags()) != NODE_ADDR_NULL))
        {
            char_array[0] = temp_pnode.cred_parent.service[0];
        }
    }
    
    return return_value;
}

/*! \fn     logic_database_get_next_2_fletters_services(uint16_t start_address, cust_char_t cur_char, cust_char_t* char_array)
*   \brief  Get the next 2 services with different first letters
*   \param  start_address       Address at which we should start looking
*   \param  cur_char            The current first char
*   \param  char_array          An array for 2 chars
*   \return Address of the next node having a different first letter
*/
uint16_t logic_database_get_next_2_fletters_services(uint16_t start_address, cust_char_t cur_char, cust_char_t* char_array)
{
    uint16_t return_value = NODE_ADDR_NULL;
    uint16_t current_node_addr;
    uint16_t storage_index = 0;
    parent_node_t temp_pnode;
    
    /* To start with the loop below */
    temp_pnode.cred_parent.nextParentAddress = start_address;
    char_array[0] = ' '; char_array[1] = ' ';
    
    do 
    {
        /* Update current node address */
        current_node_addr = temp_pnode.cred_parent.nextParentAddress;
        
        /* Read Node */
        nodemgmt_read_parent_node(current_node_addr, &temp_pnode, FALSE);
        
        /* Check if the fchar changed */
        if ((temp_pnode.cred_parent.service[0] != cur_char) && (nodemgmt_check_for_logins_with_category_in_parent_node(temp_pnode.cred_parent.nextChildAddress, nodemgmt_get_current_category_flags()) != NODE_ADDR_NULL))
        {
            char_array[storage_index++] = temp_pnode.cred_parent.service[0];
            cur_char = temp_pnode.cred_parent.service[0];
            
            /* First next letter, store address */
            if (storage_index == 1)
            {
                return_value = current_node_addr;
            }
            
            /* Did we fill the array? */
            if (storage_index == 2)
            {
                break;
            }
        }
        
    } while (temp_pnode.cred_parent.nextParentAddress != NODE_ADDR_NULL);
    
    return return_value;
}

/*! \fn     logic_database_search_service(cust_char_t* name, service_compare_mode_te compare_type, BOOL cred_type, uint16_t data_category_id)
*   \brief  Find a given service name
*   \param  name                    Name of the service / website
*   \param  compare_type            Mode of comparison (see enum)
*   \param  cred_type               set to TRUE to search for credential, FALSE for data 
*   \param  data_category_id        If cred_type is set to FALSE, the data category ID
*   \return Address of the found node, NODE_ADDR_NULL otherwise
*/
uint16_t logic_database_search_service(cust_char_t* name, service_compare_mode_te compare_type, BOOL cred_type, uint16_t data_category_id)
{
    parent_node_t temp_pnode;
    uint16_t next_node_addr;
    int16_t compare_result;
    
    /* Get start node */
    if (cred_type != FALSE)
    {
        next_node_addr = nodemgmt_get_starting_parent_addr();
    }
    else
    {
        next_node_addr = nodemgmt_get_starting_data_parent_addr(data_category_id);
    }
    
    /* Check for presence of at least one parent node */
    if (next_node_addr == NODE_ADDR_NULL)
    {
        return NODE_ADDR_NULL;
    }
    else
    {
        /* Start going through the nodes */
        do
        {
            /* Read parent node */
            nodemgmt_read_parent_node(next_node_addr, &temp_pnode, TRUE);
            
            /* Compare its service name with the name that was provided */
            if (compare_type == COMPARE_MODE_MATCH)
            {
                compare_result = utils_custchar_strncmp(name, temp_pnode.cred_parent.service, ARRAY_SIZE(temp_pnode.cred_parent.service));
                
                if (compare_result == 0)
                {
                    /* Result found */
                    return next_node_addr;
                }
                else if (compare_result < 0)
                {
                    /* Nodes are alphabetically sorted, escape if we went over */
                    return NODE_ADDR_NULL;
                }
            }
            else if ((compare_type == COMPARE_MODE_COMPARE) && (utils_custchar_strncmp(name, temp_pnode.cred_parent.service, ARRAY_SIZE(temp_pnode.cred_parent.service)) < 0))
            {
                return next_node_addr;
            }
            next_node_addr = temp_pnode.cred_parent.nextParentAddress;
        }
        while (next_node_addr != NODE_ADDR_NULL);
        
        if ((cred_type != FALSE) && (compare_type == COMPARE_MODE_COMPARE))
        {
            /* We didn't find the service, return first node */
            return nodemgmt_get_starting_parent_addr();
        }
        else
        {
            return NODE_ADDR_NULL;
        }
    }    
}

/*! \fn     logic_database_search_login_in_service(uint16_t parent_addr, cust_char_t* login, BOOL category_filter)
*   \brief  Find a given login for a given parent
*   \param  parent_addr     Parent node address
*   \param  login           Login
*   \param  category_filter Set to TRUE to filter categories
*   \return Address of the found node, NODE_ADDR_NULL otherwise
*/
uint16_t logic_database_search_login_in_service(uint16_t parent_addr, cust_char_t* login, BOOL category_filter)
{
    child_cred_node_t* temp_half_cnode_pt;
    parent_node_t temp_pnode;
    uint16_t next_node_addr;
    
    /* Dirty trick */
    temp_half_cnode_pt = (child_cred_node_t*)&temp_pnode;
    
    /* Read parent node and get first child address */
    nodemgmt_read_parent_node(parent_addr, &temp_pnode, TRUE);
    next_node_addr = temp_pnode.cred_parent.nextChildAddress;
    
    /* Check that there's actually a child node */
    if (next_node_addr == NODE_ADDR_NULL)
    {
        return NODE_ADDR_NULL;
    }
    
    /* Start going through the nodes */
    do
    {
        /* Read child node */
        nodemgmt_read_cred_child_node_except_pwd(next_node_addr, temp_half_cnode_pt);
        
        /* Compare login with the provided name */        
        if ((utils_custchar_strncmp(login, temp_half_cnode_pt->login, ARRAY_SIZE(temp_half_cnode_pt->login)) == 0) && ((category_filter == FALSE) || (nodemgmt_get_current_category_flags() == 0) || (categoryFromFlags(temp_half_cnode_pt->flags) == nodemgmt_get_current_category_flags())))
        {
            // CATSEARCHLOGIC
            return next_node_addr;
        }
        next_node_addr = temp_half_cnode_pt->nextChildAddress;
    }
    while (next_node_addr != NODE_ADDR_NULL);
    
    /* We didn't find the login */
    return NODE_ADDR_NULL;
}

/*! \fn     logic_database_get_login_for_address(uint16_t child_addr, cust_char_t** login)
*   \brief  Get the login at a given address
*   \param  child_addr  Child address
*   \param  login       Where to store the login (check size!)
*/
void logic_database_get_login_for_address(uint16_t child_addr, cust_char_t** login)
{
    child_cred_node_t* temp_half_cnode_pt;
    parent_node_t temp_pnode;
    
    /* Dirty trick */
    temp_half_cnode_pt = (child_cred_node_t*)&temp_pnode;
    
    /* Read child node */
    nodemgmt_read_cred_child_node_except_pwd(child_addr, temp_half_cnode_pt);
    
    /* Copy string */
    utils_strncpy(*login, temp_half_cnode_pt->login, MEMBER_ARRAY_SIZE(child_cred_node_t, login));
}

/*! \fn     logic_database_get_number_of_creds_for_service(uint16_t parent_addr, uint16_t* fnode_addr, BOOL category_filter)
*   \brief  Get number of credentials for a given service
*   \param  parent_addr     Parent node address
*   \param  fnode_addr      Where to store first node address
*   \param  category_filter Set to TRUE to filter categories
*   \return Number of credentials for service
*/
uint16_t logic_database_get_number_of_creds_for_service(uint16_t parent_addr, uint16_t* fnode_addr, BOOL category_filter)
{
    child_cred_node_t* temp_half_cnode_pt;
    parent_node_t temp_pnode;
    uint16_t next_node_addr;
    uint16_t return_val = 0;
    
    /* Dirty trick */
    temp_half_cnode_pt = (child_cred_node_t*)&temp_pnode;
    
    /* Read parent node and get first child address */
    nodemgmt_read_parent_node(parent_addr, &temp_pnode, TRUE);
    next_node_addr = temp_pnode.cred_parent.nextChildAddress;
    *fnode_addr = NODE_ADDR_NULL;
    
    /* Check that there's actually a child node */
    if (next_node_addr == NODE_ADDR_NULL)
    {
        return return_val;
    }
    
    /* Start going through the nodes */
    do
    {
        /* Read child node */
        nodemgmt_read_cred_child_node_except_pwd(next_node_addr, temp_half_cnode_pt);
        
        /* Check for category */
        if ((category_filter == FALSE) || (nodemgmt_get_current_category_flags() == 0) || (categoryFromFlags(temp_half_cnode_pt->flags) == nodemgmt_get_current_category_flags()))
        {            
            // CATSEARCHLOGIC            
            /* Store first node */
            if (*fnode_addr == NODE_ADDR_NULL)
            {
                *fnode_addr = next_node_addr;
            }
            
            /* Increment counter */
            return_val++;
        }
            
        /* Go to next node */
        next_node_addr = temp_half_cnode_pt->nextChildAddress;
    }
    while (next_node_addr != NODE_ADDR_NULL);
    
    /* Return counter value */
    return return_val;    
}

/*! \fn     logic_database_add_service(cust_char_t* service, BOOL cred_type, uint16_t data_category_id)
*   \brief  Add a new service to our database
*   \param  service                 Name of the service / website
*   \param  cred_type               Service type (see enum)
*   \param  data_category_id        If cred_type is set to FALSE, the data category ID
*   \return Address of the found node, NODE_ADDR_NULL if fail
*   \note   Please call logic_database_search_service before calling this
*/
uint16_t logic_database_add_service(cust_char_t* service, service_type_te cred_type, uint16_t data_category_id)
{
    uint16_t storage_addr = NODE_ADDR_NULL;
    parent_node_t temp_pnode;
    
    /* Clear node */
    memset((void*)&temp_pnode, 0, sizeof(temp_pnode));
    
    /* Set field */
    utils_strncpy(temp_pnode.cred_parent.service, service, sizeof(temp_pnode.cred_parent.service)/sizeof(cust_char_t));
    
    /* Create parent node, function handles flag setting etc */
    if (nodemgmt_create_parent_node(&temp_pnode, cred_type, &storage_addr, data_category_id) == RETURN_OK)
    {
        return storage_addr;
    }
    else
    {
        return NODE_ADDR_NULL;
    }
}

/*! \fn     logic_database_update_credential(uint16_t child_addr, cust_char_t* desc, cust_char_t* third, uint8_t* password, uint8_t* ctr)
*   \brief  Update existing credential
*   \param  child_addr  Child address
*   \param  desc        Pointer to description string, or 0 if not specified
*   \param  third       Pointer to arbitrary third field, or 0 if not specified
*   \param  password    Pointer to encrypted password, or 0 if not specified
*   \param  ctr         CTR value
*/
void logic_database_update_credential(uint16_t child_addr, cust_char_t* desc, cust_char_t* third, uint8_t* password, uint8_t* ctr)
{
    child_cred_node_t temp_cnode;
    
    /* Read node, ownership checks are done within */
    nodemgmt_read_cred_child_node(child_addr, &temp_cnode);
    
    /* Update fields that are required */
    if (desc != 0)
    {
        utils_strncpy(temp_cnode.description, desc, sizeof(temp_cnode.description)/sizeof(cust_char_t));
    }
    if (third != 0)
    {
        utils_strncpy(temp_cnode.thirdField, third, sizeof(temp_cnode.thirdField)/sizeof(cust_char_t));
    }
    if (password != 0)
    {
        memcpy(temp_cnode.ctr, ctr, MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr));
        memcpy(temp_cnode.password, password, sizeof(temp_cnode.password));
        temp_cnode.fakeFlags &= ~NODEMGMT_PREVGEN_BIT_BITMASK;
        temp_cnode.flags &= ~NODEMGMT_PREVGEN_BIT_BITMASK;
    }
    
    /* Then write back to flash at same address */
    nodemgmt_write_child_node_block_to_flash(child_addr, (child_node_t*)&temp_cnode, FALSE);
    nodemgmt_user_db_changed_actions(FALSE);
}    

/*! \fn     logic_database_add_credential_for_service(uint16_t service_addr, cust_char_t* login, cust_char_t* desc, cust_char_t* third, uint8_t* password, uint8_t* ctr)
*   \brief  Add a new credential for a given service to our database
*   \param  service_addr    Service address
*   \param  login           Pointer to login string
*   \param  desc            Pointer to description string, or 0 if not specified
*   \param  third           Pointer to arbitrary third field, or 0 if not specified
*   \param  password        Pointer to encrypted password, or 0 if not specified
*   \param  ctr             CTR value
*   \return Success status
*/
RET_TYPE logic_database_add_credential_for_service(uint16_t service_addr, cust_char_t* login, cust_char_t* desc, cust_char_t* third, uint8_t* password, uint8_t* ctr)
{
    uint16_t storage_addr = NODE_ADDR_NULL;
    child_cred_node_t temp_cnode;
    
    /* Clear node */
    memset((void*)&temp_cnode, 0, sizeof(temp_cnode));
    
    /* Update fields that are required */
    utils_strncpy(temp_cnode.login, login, sizeof(temp_cnode.login)/sizeof(cust_char_t));
    if (desc != 0)
    {
        utils_strncpy(temp_cnode.description, desc, sizeof(temp_cnode.description)/sizeof(cust_char_t));
    }
    if (third != 0)
    {
        utils_strncpy(temp_cnode.thirdField, third, sizeof(temp_cnode.thirdField)/sizeof(cust_char_t));
    }
    if (password != 0)
    {
        memcpy(temp_cnode.ctr, ctr, MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr));
        memcpy(temp_cnode.password, password, sizeof(temp_cnode.password));
    }

    /* Then create node */
    ret_type_te ret_val = nodemgmt_create_child_node(service_addr, &temp_cnode, &storage_addr);
    if (ret_val == RETURN_OK)
    {
        nodemgmt_user_db_changed_actions(FALSE);
    }

    /* Return success status */
    return ret_val;
}

/*! \fn     logic_database_fetch_encrypted_password(uint16_t child_node_addr, uint8_t* password, uint8_t* cred_ctr, BOOL* prev_gen_credential_flag)
*   \brief  Fill a get cred message packet
*   \param  child_node_addr             Child node address
*   \param  password                    Where to store the encrypted password
*   \param  cred_ctr                    Where to store credential CTR
*   \param  prev_gen_credential_flag    Where to store flag for previous gen cred
*/
void logic_database_fetch_encrypted_password(uint16_t child_node_addr, uint8_t* password, uint8_t* cred_ctr, BOOL* prev_gen_credential_flag)
{
    child_cred_node_t temp_cnode;
    
    /* Read node, ownership checks and text fields sanitizing are done within */
    nodemgmt_read_cred_child_node(child_node_addr, &temp_cnode);

    /* Copy encrypted password */
    memcpy(password, temp_cnode.password, sizeof(temp_cnode.password));
    
    /* Copy CTR */
    memcpy(cred_ctr, temp_cnode.ctr, sizeof(temp_cnode.ctr));
    
    /* Set prev gen bool */
    if ((temp_cnode.flags & NODEMGMT_PREVGEN_BIT_BITMASK) != 0)
    {
        *prev_gen_credential_flag = TRUE;
    }
    else
    {
        *prev_gen_credential_flag = FALSE;
    }
}

/*! \fn     logic_database_fill_get_cred_message_answer(uint16_t child_node_addr, hid_message_t* send_msg, uint8_t* cred_ctr, BOOL* prev_gen_credential_flag)
*   \brief  Fill a get cred message packet
*   \param  child_node_addr             Child node address
*   \param  send_msg                    Pointer to send message
*   \param  cred_ctr                    Where to store credential CTR
*   \param  prev_gen_credential_flag    Where to store flag for previous gen cred
*   \return Payload size without pwd
*/
uint16_t logic_database_fill_get_cred_message_answer(uint16_t child_node_addr, hid_message_t* send_msg, uint8_t* cred_ctr, BOOL* prev_gen_credential_flag)
{
    child_cred_node_t temp_cnode;    
    uint16_t current_index = 0;
    
    /* Read node, ownership checks and text fields sanitizing are done within */
    nodemgmt_read_cred_child_node(child_node_addr, &temp_cnode);
    
    /* Clear send_msg */
    memset(send_msg->payload, 0x00, sizeof(send_msg->payload));
    
    /* The strcpy below can be done as the message is way bigger than all fields combined */
    _Static_assert( sizeof(temp_cnode.login) \
                    + sizeof(temp_cnode.description) \
                    + sizeof(temp_cnode.thirdField) \
                    + sizeof(temp_cnode.password) 
                    + sizeof(temp_cnode.pwdTerminatingZero) + 4 \
                    < \
                    sizeof(send_msg->payload)
                    - sizeof(send_msg->get_credential_answer.login_name_index) \
                    - sizeof(send_msg->get_credential_answer.description_index) \
                    - sizeof(send_msg->get_credential_answer.third_field_index) \
                    - sizeof(send_msg->get_credential_answer.password_index), "Not enough space for send cred contents");
    
    /* Login field */
    send_msg->get_credential_answer.login_name_index = current_index;
    current_index += utils_strcpy(&(send_msg->get_credential_answer.concatenated_strings[current_index]), temp_cnode.login) + 1;
    
    /* Description field */
    send_msg->get_credential_answer.description_index = current_index;
    current_index += utils_strcpy(&(send_msg->get_credential_answer.concatenated_strings[current_index]), temp_cnode.description) + 1;
    
    /* Third field */
    send_msg->get_credential_answer.third_field_index = current_index;
    current_index += utils_strcpy(&(send_msg->get_credential_answer.concatenated_strings[current_index]), temp_cnode.thirdField) + 1;
    
    /* Password field */
    send_msg->get_credential_answer.password_index = current_index;
    memcpy(&(send_msg->get_credential_answer.concatenated_strings[current_index]), temp_cnode.password, sizeof(temp_cnode.password) + sizeof(temp_cnode.pwdTerminatingZero));
    
    /* Copy CTR */
    memcpy(cred_ctr, temp_cnode.ctr, sizeof(temp_cnode.ctr));
    
    /* Set prev gen bool */
    if ((temp_cnode.flags & NODEMGMT_PREVGEN_BIT_BITMASK) != 0)
    {
        *prev_gen_credential_flag = TRUE;
    } 
    else
    {
        *prev_gen_credential_flag = FALSE;
    }
    
    return current_index*sizeof(cust_char_t) + sizeof(send_msg->get_credential_answer.login_name_index) + sizeof(send_msg->get_credential_answer.description_index) + sizeof(send_msg->get_credential_answer.third_field_index) + sizeof(send_msg->get_credential_answer.password_index);
}
