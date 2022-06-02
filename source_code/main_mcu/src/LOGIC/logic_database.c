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
#include "logic_encryption.h"
#include "logic_database.h"
#include "gui_dispatcher.h"
#include "nodemgmt.h"
#include "utils.h"


/*! \fn     logic_database_get_prev_2_fletters_services(uint16_t start_address, cust_char_t start_char, cust_char_t* char_array, uint16_t credential_type_id)
*   \brief  Get the previous 2 services with different first letters
*   \param  start_address       Address at which we should start looking
*   \param  start_char          The current first char
*   \param  char_array          An array for 2 chars
*   \param  category_id         Credential/Data category ID
*   \return Address of the next node having a different first letter
*/
uint16_t logic_database_get_prev_2_fletters_services(uint16_t start_address, cust_char_t start_char, cust_char_t* char_array, uint16_t credential_type_id)
{
    uint16_t last_seen_parent_node_that_fits_category = NODE_ADDR_NULL;
    uint16_t current_node_addr = start_address;
    uint16_t return_value = NODE_ADDR_NULL;
    BOOL skip_first_change_bool = TRUE;
    cust_char_t cur_char = start_char;
    BOOL loopback_detected = FALSE;
    BOOL first_loop_bool = TRUE;
    int16_t storage_index = 1;
    parent_node_t temp_pnode;
    
    /* To start with the loop below */
    temp_pnode.cred_parent.prevParentAddress = start_address;
    char_array[0] = ' '; char_array[1] = ' ';
    
    while(TRUE)
    {
        /* Update current node address */
        current_node_addr = temp_pnode.cred_parent.prevParentAddress;
        
        /* Handle wrapover */
        if (current_node_addr == NODE_ADDR_NULL)
        {
            current_node_addr = nodemgmt_get_prev_parent_node_for_cur_category(NODE_ADDR_NULL, credential_type_id);
        }
        
        /* Check for credential loop */
        if ((current_node_addr == start_address) && (first_loop_bool == FALSE))
        {
            loopback_detected = TRUE;
            break;
        }
        
        /* Reset bool */
        first_loop_bool = FALSE;
        
        /* Read Node */
        nodemgmt_read_parent_node(current_node_addr, &temp_pnode, FALSE);
        
        /* Part of current category? */
        if (nodemgmt_check_for_logins_with_category_in_parent_node(temp_pnode.cred_parent.nextChildAddress, nodemgmt_get_current_category_flags()) != NODE_ADDR_NULL)
        {
            /* Check if the fchar changed */
            if (temp_pnode.cred_parent.service[0] != cur_char)
            {
                if (skip_first_change_bool == FALSE)
                {
                    char_array[storage_index--] = cur_char;
                    
                    /* First next letter, store address */
                    if (storage_index == 0)
                    {
                        return_value = last_seen_parent_node_that_fits_category;
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
                
                cur_char = temp_pnode.cred_parent.service[0];
            }
            
            /* Store parent node address */
            last_seen_parent_node_that_fits_category = current_node_addr;
        }
    }
    
    /* We looped back, but didn't have the occasion to store the credential */
    if ((loopback_detected != FALSE) && (cur_char != start_char))
    {
        char_array[storage_index--] = cur_char;
        
        /* First next letter, store address */
        if (storage_index == 0)
        {
            return_value = last_seen_parent_node_that_fits_category;
        }
    }
    
    return return_value;
}

/*! \fn     logic_database_get_next_2_fletters_services(uint16_t start_address, cust_char_t cur_char, cust_char_t* char_array, uint16_t credential_type_id)
*   \brief  Get the next 2 services with different first letters
*   \param  start_address       Address at which we should start looking
*   \param  cur_char            The current first char
*   \param  char_array          An array for 2 chars
*   \param  category_id         Credential/Data category ID
*   \return Address of the next node having a different first letter
*/
uint16_t logic_database_get_next_2_fletters_services(uint16_t start_address, cust_char_t cur_char, cust_char_t* char_array, uint16_t credential_type_id)
{
    uint16_t current_node_addr = start_address;
    uint16_t return_value = NODE_ADDR_NULL;
    BOOL first_loop_bool = TRUE;
    uint16_t storage_index = 0;
    parent_node_t temp_pnode;
    
    /* To start with the loop below */
    temp_pnode.cred_parent.nextParentAddress = start_address;
    char_array[0] = ' '; char_array[1] = ' ';
    
    while(TRUE)
    {
        /* Check for credential loop */
        if ((temp_pnode.cred_parent.nextParentAddress == start_address) && (first_loop_bool == FALSE))
        {
            break;
        }
        
        /* Reset bool */
        first_loop_bool = FALSE;
        
        /* Update current node address */
        current_node_addr = temp_pnode.cred_parent.nextParentAddress;
        
        /* Read Node */
        nodemgmt_read_parent_node(current_node_addr, &temp_pnode, FALSE);
        
        /* Check if the fchar changed */
        if ((temp_pnode.cred_parent.service[0] != cur_char) && (nodemgmt_check_for_logins_with_category_in_parent_node(temp_pnode.cred_parent.nextChildAddress, nodemgmt_get_current_category_flags()) != NODE_ADDR_NULL))
        {            
            /* Store node */
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
        
        /* Handle wrapover */
        if (temp_pnode.cred_parent.nextParentAddress == NODE_ADDR_NULL)
        {
            temp_pnode.cred_parent.nextParentAddress = nodemgmt_get_next_parent_node_for_cur_category(NODE_ADDR_NULL, credential_type_id);
        }
    }
    
    return return_value;
}

/*! \fn     logic_database_search_for_next_data_parent_after_addr(uint16_t node_addr, nodemgmt_data_category_te data_type, cust_char_t* service_name)
*   \brief  Search for the next data parent after a given address
*   \param  node_addr           VALID parent address after which we need to look
*   \param  data_type           Data category (see enum)
*   \param  service_name        Where to store the service name
*   \return Address of the found node, NODE_ADDR_NULL otherwise
*   \note   This function goes through the complete linked list to make sure of ownership
*/
uint16_t logic_database_search_for_next_data_parent_after_addr(uint16_t node_addr, nodemgmt_data_category_te data_type, cust_char_t* service_name)
{
    uint16_t next_node_addr = nodemgmt_get_starting_data_parent_addr(data_type);
    BOOL node_addr_found = FALSE;
    parent_node_t temp_pnode;
    
    /* Start address wanted? */
    if (node_addr == NODE_ADDR_NULL)
    {
        node_addr_found = TRUE;
    }
    
    /* No files? */
    if (next_node_addr == NODE_ADDR_NULL)
    {
        return NODE_ADDR_NULL;
    }
    
    /* Start going through the nodes */
    do
    {
        /* Read parent node */
        nodemgmt_read_parent_node(next_node_addr, &temp_pnode, TRUE);
        
        /* Did we find the node we were looking for? */
        if (node_addr_found == FALSE)
        {
            /* Check if it is the node we're looking for */
            if (node_addr == next_node_addr)
            {
                node_addr_found = TRUE;
            }
        } 
        else
        {
            /* Check for correct data type */
            if (nodeTypeFromFlags(temp_pnode.data_parent.flags) != NODE_TYPE_PARENT_DATA)
            {
                return NODE_ADDR_NULL;
            }                
            
            /* Copy the service name, sanitized by previous nodemgmt_read_parent_node call */
            utils_strcpy(service_name, temp_pnode.data_parent.service);
            
            /* Node was read, therefore checking ownership */
            return next_node_addr;
        }        
        
        /* Load next address */
        next_node_addr = temp_pnode.data_parent.nextParentAddress;
    }
    while (next_node_addr != NODE_ADDR_NULL);
    
    /* We just don't have nodes */
    return NODE_ADDR_NULL;
}

/*! \fn     logic_database_search_service(cust_char_t* name, service_compare_mode_te compare_type, BOOL cred_type, uint16_t category_id)
*   \brief  Find a given service name
*   \param  name                    Name of the service / website
*   \param  compare_type            Mode of comparison (see enum)
*   \param  cred_type               set to TRUE to search for credential, FALSE for data 
*   \param  category_id             Credential/Data category ID
*   \return Address of the found node, NODE_ADDR_NULL otherwise
*   \note   Full 8Mb database search has been timed at 581ms
*/
uint16_t logic_database_search_service(cust_char_t* name, service_compare_mode_te compare_type, BOOL cred_type, uint16_t category_id)
{
    cust_char_t last_service_encountered[MEMBER_ARRAY_SIZE(parent_cred_node_t, service)];
    memset(last_service_encountered, 0, sizeof(last_service_encountered));
    BOOL mult_domain_possible = FALSE;
    uint16_t service_dot_index = 0;
    parent_node_t temp_pnode;
    uint16_t next_node_addr;
    int16_t compare_result;
    
    /* Check if we can actually use the multiple domain feature */
    if ((compare_type == COMPARE_MODE_MATCH) && (cred_type == TRUE) && (category_id == NODEMGMT_STANDARD_CRED_TYPE_ID))
    {
        /* Store index of the last '.' and check for terminating 0 */
        for (uint16_t i = 0; i < MEMBER_ARRAY_SIZE(parent_cred_node_t, service); i++)
        {
            /* Terminating 0 before end of array */
            if (name[i] == 0)
            {
                if (service_dot_index != 0)
                {
                    mult_domain_possible = TRUE;
                }
                break;
            } 
            else if (name[i] == '.')
            {
                service_dot_index = i;
            }
        }
    }
    
    /* Get start node */
    if (cred_type != FALSE)
    {
        next_node_addr = nodemgmt_get_starting_parent_addr(category_id);
    }
    else
    {
        next_node_addr = nodemgmt_get_starting_data_parent_addr(category_id);
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
            if (nodemgmt_read_parent_node_permissive(next_node_addr, &temp_pnode, TRUE) != RETURN_OK)
            {
                return NODE_ADDR_NULL;
            }
            
            /* Check for database loop */
            if (utils_custchar_strncmp(last_service_encountered, temp_pnode.cred_parent.service, ARRAY_SIZE(temp_pnode.cred_parent.service)) >= 0)
            {
                return NODE_ADDR_NULL;
            }
            memcpy(last_service_encountered, temp_pnode.cred_parent.service, sizeof(temp_pnode.cred_parent.service));
            
            /* Compare its service name with the name that was provided */
            if (compare_type == COMPARE_MODE_MATCH)
            {
                compare_result = utils_custchar_strncmp(name, temp_pnode.cred_parent.service, ARRAY_SIZE(temp_pnode.cred_parent.service));
                
                /* Hey future Mathieu! Data parent category filter could be setup here... but then each file name must be unique across all categories... */
                
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
            return nodemgmt_get_starting_parent_addr(category_id);
        }
        else
        {
            return NODE_ADDR_NULL;
        }
    }    
}

/*! \fn     logic_database_search_webauthn_userhandle_in_service(uint16_t parent_addr, uint8_t* user_handle, uint8_t user_handle_len)
*   \brief  Find a given userhandle for a given parent
*   \param  parent_addr Parent node address
*   \param  user_handle User handle
*   \param  user_handle_len Length of user handle
*   \return Address of the found node, NODE_ADDR_NULL otherwise
*/
uint16_t logic_database_search_webauthn_userhandle_in_service(uint16_t parent_addr, uint8_t* user_handle, uint8_t user_handle_len)
{
    child_webauthn_node_t* temp_half_cnode_pt;
    parent_node_t temp_pnode;
    uint16_t next_node_addr;
    
    /* Dirty trick */
    temp_half_cnode_pt = (child_webauthn_node_t*)&temp_pnode;
    
    /* Read parent node and get first child address */
    nodemgmt_read_parent_node(parent_addr, &temp_pnode, TRUE);
    next_node_addr = temp_pnode.cred_parent.nextChildAddress;
    
    /* Check that there's actually a child node */
    if (next_node_addr == NODE_ADDR_NULL)
    {
        return NODE_ADDR_NULL;
    }

    /* Sanitize the user handle length to prevent overflows */
    if (user_handle_len > MEMBER_SIZE(child_webauthn_node_t, user_handle))
    {
        user_handle_len = MEMBER_SIZE(child_webauthn_node_t, user_handle);
    }

    /* Start going through the nodes */
    do
    {
        /* Read child node */
        nodemgmt_read_webauthn_child_node_except_display_name(next_node_addr, temp_half_cnode_pt, FALSE);
        
        /* Compare with provided user handle */
        if (user_handle_len == temp_half_cnode_pt->user_handle_len && memcmp(temp_half_cnode_pt->user_handle, user_handle, user_handle_len) == 0)
        {
            return next_node_addr;
        }
        
        /* Go to next one */
        next_node_addr = temp_half_cnode_pt->nextChildAddress;
    }
    while (next_node_addr != NODE_ADDR_NULL);
    
    /* We didn't find the userhandle */
    return NODE_ADDR_NULL;
}

/*! \fn     logic_database_search_webauthn_credential_id_in_service(uint16_t parent_addr, uint8_t* credential_id)
*   \brief  Find a given credential id for a given parent
*   \param  parent_addr Parent node address
*   \param  credential_id Credential ID
*   \return Address of the found node, NODE_ADDR_NULL otherwise
*/
uint16_t logic_database_search_webauthn_credential_id_in_service(uint16_t parent_addr, uint8_t* credential_id)
{
    child_webauthn_node_t* temp_half_cnode_pt;
    parent_node_t temp_pnode;
    uint16_t next_node_addr;
    
    /* Dirty trick */
    temp_half_cnode_pt = (child_webauthn_node_t*)&temp_pnode;
    
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
        nodemgmt_read_webauthn_child_node_except_display_name(next_node_addr, temp_half_cnode_pt, FALSE);
        
        /* Compare with provided credential id */
        if (memcmp(temp_half_cnode_pt->credential_id, credential_id, MEMBER_SIZE(child_webauthn_node_t, credential_id)) == 0)
        {
            return next_node_addr;
        }
        
        /* Go to next one */
        next_node_addr = temp_half_cnode_pt->nextChildAddress;
    }
    while (next_node_addr != NODE_ADDR_NULL);
    
    /* We didn't find the credential id */
    return NODE_ADDR_NULL;
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

/*! \fn     logic_database_get_webauthn_username_for_address(uint16_t child_addr, cust_char_t* user_name)
*   \brief  Get the user_name at a given address
*   \param  child_addr  Child address
*   \param  user_name   Where to store the username
*/
void logic_database_get_webauthn_username_for_address(uint16_t child_addr, cust_char_t* user_name)
{
    child_webauthn_node_t* temp_half_cnode_pt;
    parent_node_t temp_pnode;
    
    /* Dirty trick */
    temp_half_cnode_pt = (child_webauthn_node_t*)&temp_pnode;
    
    /* Read child node */
    nodemgmt_read_webauthn_child_node_except_display_name(child_addr, temp_half_cnode_pt, FALSE);
    
    /* Copy string */
    utils_strncpy(user_name, temp_half_cnode_pt->user_name, MEMBER_ARRAY_SIZE(child_webauthn_node_t, user_name));
}

/*! \fn     logic_database_get_webauthn_userhandle_for_address(uint16_t child_addr, uint8_t* userhandle, uint8_t *user_handle_len)
*   \brief  Get the userhandle at a given address
*   \param  child_addr  Child address
*   \param  user_handle    Where to store the user handle 
*   \param  user_hanle_len Where to store the user handle length
*/
void logic_database_get_webauthn_userhandle_for_address(uint16_t child_addr, uint8_t* user_handle, uint8_t *user_handle_len)
{
    child_webauthn_node_t* temp_half_cnode_pt;
    parent_node_t temp_pnode;
    
    /* Dirty trick */
    temp_half_cnode_pt = (child_webauthn_node_t*)&temp_pnode;
    
    /* Read child node */
    nodemgmt_read_webauthn_child_node_except_display_name(child_addr, temp_half_cnode_pt, FALSE);

    /* Sanitize user_handle_len to prevent overflow */
    if (temp_half_cnode_pt->user_handle_len > MEMBER_SIZE(child_webauthn_node_t, user_handle))
    {
        temp_half_cnode_pt->user_handle_len = MEMBER_SIZE(child_webauthn_node_t, user_handle);
    }

    /* Copy userhandle */
    memcpy(user_handle, temp_half_cnode_pt->user_handle, temp_half_cnode_pt->user_handle_len);

    /* Store user handle length */
    *user_handle_len = temp_half_cnode_pt->user_handle_len;
}

/*! \fn     logic_database_get_webauthn_data_for_address(uint16_t child_addr, uint8_t* user_handle, uint8_t *user_handle_len, uint8_t* credential_id, uint8_t* key, uint32_t* count, uint8_t* ctr)
*   \brief  Get the webauthn data for given address and pre increment signing counter
*   \param  child_addr  Child address
*   \param  user_handle     Where to store the user handle
*   \param  user_handle_len Where to store the user handle length
*   \param  credential_id   Where to store the credential id
*   \param  key             Where to store the fetched key
*   \param  count           Where to store the sign count
*   \param  ctr             Where to store the ctr value
*   \param  keyType         Where to store the key type
*/
void logic_database_get_webauthn_data_for_address_and_inc_count(uint16_t child_addr, uint8_t* user_handle, uint8_t *user_handle_len, uint8_t* credential_id, uint8_t* key, uint32_t* count, uint8_t* ctr, uint8_t *keyType)
{
    child_webauthn_node_t* temp_half_cnode_pt;
    parent_node_t temp_pnode;
    
    /* Dirty trick */
    temp_half_cnode_pt = (child_webauthn_node_t*)&temp_pnode;
    
    /* Read child node */
    nodemgmt_read_webauthn_child_node_except_display_name(child_addr, temp_half_cnode_pt, TRUE);

    /* Sanitize user_handle_len to prevent overflow */
    if (temp_half_cnode_pt->user_handle_len > MEMBER_SIZE(child_webauthn_node_t, user_handle))
    {
        temp_half_cnode_pt->user_handle_len = MEMBER_SIZE(child_webauthn_node_t, user_handle);
    }

    /* Copy user handle */
    memcpy(user_handle, temp_half_cnode_pt->user_handle, temp_half_cnode_pt->user_handle_len);
    
    /* Store user handle length */
    *user_handle_len = temp_half_cnode_pt->user_handle_len;

    /* Copy credential id */
    memcpy(credential_id, temp_half_cnode_pt->credential_id, MEMBER_SIZE(child_webauthn_node_t, credential_id));
    
    /* Copy key */
    memcpy(key, temp_half_cnode_pt->private_key, MEMBER_SIZE(child_webauthn_node_t, private_key));
    
    /* Copy CTR */
    memcpy(ctr, temp_half_cnode_pt->ctr, MEMBER_SIZE(child_webauthn_node_t, ctr));
    
    /* Store signature count */
    *count = temp_half_cnode_pt->signature_counter_msb;
    *count = (*count << 16) & 0xFFFF0000;
    *count += temp_half_cnode_pt->signature_counter_lsb;

    /* Store key type */
    *keyType = temp_half_cnode_pt->keyType;
}

/*! \fn     logic_database_get_number_of_creds_for_service(uint16_t parent_addr, uint16_t* fnode_addr, uint16_t* lnode_used_addr, BOOL category_filter)
*   \brief  Get number of credentials for a given service
*   \param  parent_addr     Parent node address
*   \param  fnode_addr      Where to store first node address
*   \param  lnode_used_addr Where to store the address of the child node that was last used for that parent
*   \param  category_filter Set to TRUE to filter categories
*   \return Number of credentials for service
*/
uint16_t logic_database_get_number_of_creds_for_service(uint16_t parent_addr, uint16_t* fnode_addr, uint16_t* lnode_used_addr, BOOL category_filter)
{
    child_cred_node_t* temp_half_cnode_pt;
    uint16_t suggested_last_node_addr;
    parent_node_t temp_pnode;
    uint16_t next_node_addr;
    uint16_t return_val = 0;
    
    /* Set last node address to NULL in case */
    *lnode_used_addr = NODE_ADDR_NULL;
    
    /* Dirty trick */
    temp_half_cnode_pt = (child_cred_node_t*)&temp_pnode;
    
    /* Read parent node and get first child address */
    nodemgmt_read_parent_node(parent_addr, &temp_pnode, TRUE);
    suggested_last_node_addr = temp_pnode.cred_parent.last_cnode_used_addr;
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
            
            /* Check for last used match */
            if (suggested_last_node_addr == next_node_addr)
            {
                *lnode_used_addr = next_node_addr;
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

/*! \fn     logic_database_add_child_node_to_data_service(uint16_t logic_user_data_service_addr,uint16_t* logic_user_last_data_child_addr,hid_message_store_data_into_file_t* store_data_request)
*   \brief  Add data to a given data service
*   \param  logic_user_data_service_addr    The parent address
*   \param  logic_user_last_data_child_addr Pointer to where to read/store the address of the latest stored child address
*   \param  store_data_request              The store data request
*   \return success status
*/
RET_TYPE logic_database_add_child_node_to_data_service(uint16_t logic_user_data_service_addr, uint16_t* logic_user_last_data_child_addr, hid_message_store_data_into_file_t* store_data_request)
{
    _Static_assert(sizeof(hid_message_store_data_into_file_t) == sizeof(child_data_node_t), "Erroneous hid_message_store_data_into_file_t cast");
    uint8_t temp_cred_ctr_val_bis[MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr)];
    uint8_t temp_cred_ctr_val[MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr)];
    uint16_t stored_address = NODE_ADDR_NULL;
    
    /* Cast into node type */
    child_data_node_t* data_node_pt = (child_data_node_t*)store_data_request;
    
    /* Input sanitizing */
    memset(data_node_pt->reserved2, 0, sizeof(data_node_pt->reserved2));
    memset(data_node_pt->reserved, 0, sizeof(data_node_pt->reserved));
    data_node_pt->nextDataAddress = NODE_ADDR_NULL;
    data_node_pt->fakeFlags = 0;
    data_node_pt->flags = 0;    
    
    /* Encrypt chunks of data */
    logic_encryption_ctr_encrypt(data_node_pt->data, sizeof(data_node_pt->data), temp_cred_ctr_val);
    logic_encryption_ctr_encrypt(data_node_pt->data2, sizeof(data_node_pt->data2), temp_cred_ctr_val_bis);
    
    /* Try to store data node */
    if (nodemgmt_store_data_node(data_node_pt, &stored_address) != RETURN_OK)
    {
        return RETURN_NOK;
    }
    
    /* Update parent if this is the first block */
    if (*logic_user_last_data_child_addr == NODE_ADDR_NULL)
    {
        nodemgmt_update_data_parent_ctr_and_first_child_address(logic_user_data_service_addr, temp_cred_ctr_val, stored_address);
    }
    else
    {
        /* If not, update the previous data node */
        nodemgmt_update_child_data_node_with_next_address(*logic_user_last_data_child_addr, stored_address);
    }
    
    /* Store storage address */
    *logic_user_last_data_child_addr = stored_address;
    
    /* Updated actions */
    nodemgmt_user_db_changed_actions(TRUE);
    
    return RETURN_OK;
}

/*! \fn     logic_database_update_webauthn_credential(uint16_t child_address, cust_char_t* user_name, cust_char_t* display_name, uint8_t* private_key,  uint8_t* ctr, uint8_t* credential_id)
*   \brief  Update a webauthn credential for a given service in our database
*   \param  child_addr      Child address
*   \param  user_name       Pointer to user name string
*   \param  display_name    Pointer to display name sstring
*   \param  private_key     Pointer to encrypted private key
*   \param  ctr             CTR value
*   \param  credential_id   Pointer to credential_id buffer
*   \param  keyType         Key Type (ES256 or EDDSA)
*/
void logic_database_update_webauthn_credential(uint16_t child_address, cust_char_t* user_name, cust_char_t* display_name, uint8_t* private_key,  uint8_t* ctr, uint8_t* credential_id, uint8_t keyType)
{
    child_webauthn_node_t temp_cnode;
    
    /* Read node, ownership checks are done within */
    nodemgmt_read_webauthn_child_node(child_address, &temp_cnode, FALSE);
    
    /* Copy everything */
    utils_strncpy(temp_cnode.user_name, user_name, MEMBER_ARRAY_SIZE(child_webauthn_node_t, user_name));
    utils_strncpy(temp_cnode.display_name, display_name, MEMBER_ARRAY_SIZE(child_webauthn_node_t, display_name));
    memcpy(temp_cnode.private_key, private_key, sizeof(temp_cnode.private_key));
    memcpy(temp_cnode.ctr, ctr, sizeof(temp_cnode.ctr));
    memcpy(temp_cnode.credential_id, credential_id, sizeof(temp_cnode.credential_id));
    temp_cnode.keyType = keyType;
    
    /* Set signature count to 1 */
    temp_cnode.signature_counter_msb = 0;
    temp_cnode.signature_counter_lsb = 1;
    
    /* Update dates */
    temp_cnode.dateCreated = nodemgmt_get_current_date();
    temp_cnode.dateLastUsed = nodemgmt_get_current_date();

    /* Then write node */
    nodemgmt_write_child_node_block_to_flash(child_address, (child_node_t*)&temp_cnode, FALSE);
    nodemgmt_user_db_changed_actions(FALSE);
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
    uint16_t pted_to_pwd_totp_address = NODE_ADDR_NULL;
    node_type_te temp_node_type = NODE_TYPE_NULL;
    child_cred_node_t temp_cnode;
    
    /* Read node, ownership checks are done within */
    nodemgmt_read_cred_child_node(child_addr, &temp_cnode, FALSE);
    
    /* Check if this node uses pwd & totp pointing */
    if ((temp_cnode.ptedPwdChildAddress != UINT16_MAX) && (nodemgmt_check_user_permission(temp_cnode.ptedPwdChildAddress, &temp_node_type) == RETURN_OK) && (temp_node_type == NODE_TYPE_CHILD))
    {
        pted_to_pwd_totp_address = temp_cnode.ptedPwdChildAddress;
    }
    
    /* Update dates */
    if ((desc != 0) || (third != 0) || ((password != 0) && (pted_to_pwd_totp_address == NODE_ADDR_NULL)))
    {
        temp_cnode.dateCreated = nodemgmt_get_current_date();
        temp_cnode.dateLastUsed = nodemgmt_get_current_date();
    }
    
    /* Update fields that are required */
    if (desc != 0)
    {
        utils_strncpy(temp_cnode.description, desc, sizeof(temp_cnode.description)/sizeof(cust_char_t));
    }
    if (third != 0)
    {
        utils_strncpy(temp_cnode.thirdField, third, sizeof(temp_cnode.thirdField)/sizeof(cust_char_t));
    }
    if ((password != 0) && (pted_to_pwd_totp_address == NODE_ADDR_NULL))
    {
        memcpy(temp_cnode.ctr, ctr, MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr));
        memcpy(temp_cnode.password, password, sizeof(temp_cnode.password));
        temp_cnode.fakeFlags &= ~NODEMGMT_PREVGEN_BIT_BITMASK;
        temp_cnode.flags &= ~NODEMGMT_PREVGEN_BIT_BITMASK;
        temp_cnode.passwordBlankFlag = FALSE;
    }
    
    /* Then write back to flash at same address */
    nodemgmt_write_child_node_block_to_flash(child_addr, (child_node_t*)&temp_cnode, FALSE);
    nodemgmt_user_db_changed_actions(FALSE);
    
    /* Do we need to update password for a pointed to node? */
    if ((password != 0) && (pted_to_pwd_totp_address != NODE_ADDR_NULL))
    {
        /* Read node, ownership checks are done within */
        nodemgmt_read_cred_child_node(pted_to_pwd_totp_address, &temp_cnode, FALSE);
        
        /* Update dates */
        temp_cnode.dateCreated = nodemgmt_get_current_date();
        temp_cnode.dateLastUsed = nodemgmt_get_current_date();
        
        /* Update password related fields */
        memcpy(temp_cnode.ctr, ctr, MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr));
        memcpy(temp_cnode.password, password, sizeof(temp_cnode.password));
        temp_cnode.fakeFlags &= ~NODEMGMT_PREVGEN_BIT_BITMASK;
        temp_cnode.flags &= ~NODEMGMT_PREVGEN_BIT_BITMASK;
        temp_cnode.passwordBlankFlag = FALSE;
        
        /* Then write back to flash at same address */
        nodemgmt_write_child_node_block_to_flash(pted_to_pwd_totp_address, (child_node_t*)&temp_cnode, FALSE);
    }
}    

/*! \fn     logic_database_update_TOTP_credentials(uint16_t child_addr, TOTPcredentials_t const *TOTPcreds, uint8_t* ctr)
*   \brief  Update existing credential
*           NOTE: Assumes TOTPcreds are sanitized already
*   \param  child_addr  Child address
*   \param  TOTPcreds   Pointer to encrypted TOTPsecret and meta data
*   \param  ctr         CTR value
*   \return Success status
*/
RET_TYPE logic_database_update_TOTP_credentials(uint16_t child_addr, TOTPcredentials_t const *TOTPcreds, uint8_t* ctr)
{
    node_type_te temp_node_type = NODE_TYPE_NULL;
    child_cred_node_t temp_cnode;

    /* Read node, ownership checks are done within */
    nodemgmt_read_cred_child_node(child_addr, &temp_cnode, FALSE);
    
    /* Check if this node uses pwd & totp pointing */
    if ((temp_cnode.ptedPwdChildAddress != UINT16_MAX) && (nodemgmt_check_user_permission(temp_cnode.ptedPwdChildAddress, &temp_node_type) == RETURN_OK) && (temp_node_type == NODE_TYPE_CHILD))
    {
        /* It does... start working with the other node... */
        child_addr = temp_cnode.ptedPwdChildAddress;
        nodemgmt_read_cred_child_node(child_addr, &temp_cnode, FALSE);
    }

    /* Update dates */
    temp_cnode.dateCreated = nodemgmt_get_current_date();
    temp_cnode.dateLastUsed = nodemgmt_get_current_date();

    /* Update fields */

    /* TOTPsecretLen checked in calling function, copy all credential information into node */
    memcpy(temp_cnode.TOTP.TOTPsecret, TOTPcreds->TOTPsecret, TOTPcreds->TOTPsecretLen);
    memcpy(temp_cnode.TOTP.TOTPsecret_ctr, ctr, MEMBER_SIZE(TOTP_cred_node_t, TOTPsecret_ctr));
    temp_cnode.TOTP.TOTPsecretLen = TOTPcreds->TOTPsecretLen;

    /* All values sanitized in calling function */
    temp_cnode.TOTP.TOTPtimeStep = TOTPcreds->TOTPtimeStep;
    temp_cnode.TOTP.TOTP_SHA_ver = TOTPcreds->TOTP_SHA_ver;
    temp_cnode.TOTP.TOTPnumDigits = TOTPcreds->TOTPnumDigits;

    /* Then write back to flash at same address */
    nodemgmt_write_child_node_block_to_flash(child_addr, (child_node_t*)&temp_cnode, FALSE);
    nodemgmt_user_db_changed_actions(FALSE);

    return RETURN_OK;
}

/*! \fn     logic_database_add_webauthn_credential_for_service(uint16_t service_addr, uint8_t* user_handle, uint8_t user_handle_len, cust_char_t* user_name, cust_char_t* display_name, uint8_t* private_key,  uint8_t* ctr, uint8_t* credential_id)
*   \brief  Add a new webauthn credential for a given service to our database
*   \param  service_addr    Service address
*   \param  user_handle     user handle
*   \param  user_handle_len Length of user handle
*   \param  user_name       Pointer to user name string
*   \param  display_name    Pointer to display name sstring
*   \param  private_key     Pointer to encrypted private key
*   \param  ctr             CTR value
*   \param  credential_id   Pointer to credential_id buffer
*   \param  keyType         Key Type (ES256 or EDDSA)
*   \return Success status
*/
RET_TYPE logic_database_add_webauthn_credential_for_service(uint16_t service_addr, uint8_t* user_handle, uint8_t user_handle_len, cust_char_t* user_name, cust_char_t* display_name, uint8_t* private_key,  uint8_t* ctr, uint8_t* credential_id, uint8_t keyType)
{
    uint16_t storage_addr = NODE_ADDR_NULL;
    child_webauthn_node_t temp_cnode;
    
    /* Clear node */
    memset((void*)&temp_cnode, 0, sizeof(temp_cnode));

    /* Sanitize the user handle length to prevent overflows */
    if (user_handle_len > MEMBER_SIZE(child_webauthn_node_t, user_handle))
    {
        user_handle_len = MEMBER_SIZE(child_webauthn_node_t, user_handle);
    }

    /* Copy everything */
    temp_cnode.user_handle_len = user_handle_len;
    memcpy(temp_cnode.user_handle, user_handle, user_handle_len);
    utils_strncpy(temp_cnode.user_name, user_name, MEMBER_ARRAY_SIZE(child_webauthn_node_t, user_name));
    utils_strncpy(temp_cnode.display_name, display_name, MEMBER_ARRAY_SIZE(child_webauthn_node_t, display_name));
    memcpy(temp_cnode.private_key, private_key, sizeof(temp_cnode.private_key));
    memcpy(temp_cnode.ctr, ctr, sizeof(temp_cnode.ctr));
    memcpy(temp_cnode.credential_id, credential_id, sizeof(temp_cnode.credential_id));
    temp_cnode.keyType = keyType;
    
    /* Set signature count to 1 */
    temp_cnode.signature_counter_msb = 0;
    temp_cnode.signature_counter_lsb = 1;

    /* Then create node */
    ret_type_te ret_val = nodemgmt_create_child_node(service_addr, (child_cred_node_t*)&temp_cnode, &storage_addr);
    if (ret_val == RETURN_OK)
    {
        nodemgmt_user_db_changed_actions(FALSE);
    }

    /* Return success status */
    return ret_val;    
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
        temp_cnode.passwordBlankFlag = FALSE;
        memcpy(temp_cnode.ctr, ctr, MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr));
        memcpy(temp_cnode.password, password, sizeof(temp_cnode.password));
    }
    else
    {
        temp_cnode.passwordBlankFlag = TRUE;
    }
    
    /* Set "master setting" for keys pressed after login & password entering */
    temp_cnode.keyAfterPassword = 0xFFFF;
    temp_cnode.keyAfterLogin = 0xFFFF;

    /* Then create node */
    ret_type_te ret_val = nodemgmt_create_child_node(service_addr, &temp_cnode, &storage_addr);
    if (ret_val == RETURN_OK)
    {
        nodemgmt_user_db_changed_actions(FALSE);
    }

    /* Return success status */
    return ret_val;
}

/*! \fn     logic_database_add_TOTP_credential_for_service(uint16_t service_addr, cust_char_t* login, TOTPcredentials_t const *TOTPcreds, uint8_t *ctr)
*   \brief  Add a new TOTP credential for a given service to our database
*           NOTE: Assumes TOTPcreds are sanitized already
*   \param  service_addr    Service address
*   \param  login           Pointer to login string
*   \param  TOTPcreds       Pointer to the TOTP credentials
*   \param  ctr             CTR value
*   \return Success status
*/
RET_TYPE logic_database_add_TOTP_credential_for_service(uint16_t service_addr, cust_char_t* login, TOTPcredentials_t const *TOTPcreds, uint8_t *ctr)
{
    uint16_t storage_addr = NODE_ADDR_NULL;
    child_cred_node_t temp_cnode;

    /* Clear node */
    memset((void*)&temp_cnode, 0, sizeof(temp_cnode));

    /* Update fields that are required */
    utils_strncpy(temp_cnode.login, login, MEMBER_ARRAY_SIZE(child_cred_node_t, login));

    temp_cnode.passwordBlankFlag = TRUE;

    /* Set "master setting" for keys pressed after login & password entering */
    temp_cnode.keyAfterPassword = 0xFFFF;
    temp_cnode.keyAfterLogin = 0xFFFF;

    /* TOTPsecretLen is checked in calling function, copy all credential information into node */
    memcpy(temp_cnode.TOTP.TOTPsecret, TOTPcreds->TOTPsecret, TOTPcreds->TOTPsecretLen);
    memcpy(temp_cnode.TOTP.TOTPsecret_ctr, ctr, MEMBER_SIZE(TOTP_cred_node_t, TOTPsecret_ctr));
    temp_cnode.TOTP.TOTPsecretLen = TOTPcreds->TOTPsecretLen;
    /* All values sanitized in calling function */
    temp_cnode.TOTP.TOTPtimeStep = TOTPcreds->TOTPtimeStep;
    temp_cnode.TOTP.TOTP_SHA_ver = TOTPcreds->TOTP_SHA_ver;
    temp_cnode.TOTP.TOTPnumDigits = TOTPcreds->TOTPnumDigits;

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
    nodemgmt_read_cred_child_node(child_node_addr, &temp_cnode, TRUE);

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

/*! \fn     logic_database_fetch_encrypted_TOTPsecret(uint16_t child_node_addr, uint8_t* TOTPsecret, uint8_t *TOTPsecretLen, uint8_t* TOTP_ctr)
*   \brief  Fetch encrypted TOTP secret
*   \param  child_node_addr             Child node address
*   \param  TOTPsecret                  Where to store the encrypted secret
*   \param  TOTPsecretLen               Where to store the TOTP length (maximum secret length supported is 32 bytes)
*   \param  TOTP_ctr                    Where to store TOTP CTR
*/
void logic_database_fetch_encrypted_TOTPsecret(uint16_t child_node_addr, uint8_t* TOTPsecret, uint8_t *TOTPsecretLen, uint8_t* TOTP_ctr)
{
    child_cred_node_t temp_cnode;

    /* Read node, ownership checks and text fields sanitizing are done within */
    nodemgmt_read_cred_child_node(child_node_addr, &temp_cnode, TRUE);

    /* Copy encrypted password */
    memcpy(TOTPsecret, temp_cnode.TOTP.TOTPsecret, sizeof(temp_cnode.TOTP.TOTPsecret));

    /* Copy secret (non-encryped) TOTPsecret length */
    *TOTPsecretLen = temp_cnode.TOTP.TOTPsecretLen;
    
    /* Sanitize secret len */
    if (temp_cnode.TOTP.TOTPsecretLen > sizeof(temp_cnode.TOTP.TOTPsecret))
    {
        *TOTPsecretLen = sizeof(temp_cnode.TOTP.TOTPsecret);
    }

    /* Copy CTR */
    memcpy(TOTP_ctr, temp_cnode.TOTP.TOTPsecret_ctr, sizeof(temp_cnode.TOTP.TOTPsecret_ctr));
}

/*! \fn     logic_database_fill_get_cred_message_answer(uint16_t child_node_addr, hid_message_t* send_msg, uint8_t* cred_ctr, BOOL* prev_gen_credential_flag, BOOL* password_valid, BOOL* has_totp)
*   \brief  Fill a get cred message packet
*   \param  child_node_addr             Child node address
*   \param  send_msg                    Pointer to send message
*   \param  cred_ctr                    Where to store credential CTR
*   \param  prev_gen_credential_flag    Where to store flag for previous gen cred
*   \param  password_valid              Boolean set depending if the password is valid
*   \param  has_totp                    Boolean set depending if the credential has a TOTP
*   \return Payload size without pwd
*/
uint16_t logic_database_fill_get_cred_message_answer(uint16_t child_node_addr, hid_message_t* send_msg, uint8_t* cred_ctr, BOOL* prev_gen_credential_flag, BOOL* password_valid, BOOL* has_totp)
{
    child_cred_node_t temp_cnode;    
    uint16_t current_index = 0;
    
    /* Read node, ownership checks and text fields sanitizing are done within */
    nodemgmt_read_cred_child_node(child_node_addr, &temp_cnode, TRUE);
    
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
    
    /* Set password valid flag */
    if (temp_cnode.passwordBlankFlag == FALSE)
    {
        *password_valid = TRUE;
    } 
    else
    {
        *password_valid = FALSE;
    }
    
    /* Set TOTP flag */
    if (temp_cnode.TOTP.TOTPsecretLen > 0)
    {
        *has_totp = TRUE;
    } 
    else
    {
        *has_totp = FALSE;
    }
    
    return current_index*sizeof(cust_char_t) + sizeof(send_msg->get_credential_answer.login_name_index) + sizeof(send_msg->get_credential_answer.description_index) + sizeof(send_msg->get_credential_answer.third_field_index) + sizeof(send_msg->get_credential_answer.password_index);
}
