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

/*! \fn     logic_database_search_login_in_service(uint16_t parent_addr, cust_char_t* login)
*   \brief  Find a given login for a given parent
*   \param  parent_addr Parent node address
*   \param  login       Login
*   \return Address of the found node, NODE_ADDR_NULL otherwise
*/
uint16_t logic_database_search_login_in_service(uint16_t parent_addr, cust_char_t* login)
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
        if (utils_custchar_strncmp(login, temp_half_cnode_pt->login, ARRAY_SIZE(temp_half_cnode_pt->login)) == 0)
        {
            return next_node_addr;
        }
        next_node_addr = temp_half_cnode_pt->nextChildAddress;
    }
    while (next_node_addr != NODE_ADDR_NULL);
    
    /* We didn't find the login */
    return NODE_ADDR_NULL;
}

/*! \fn     logic_database_get_number_of_creds_for_service(uint16_t parent_addr, uint16_t& fnode_addr)
*   \brief  Get number of credentials for a given service
*   \param  parent_addr Parent node address
*   \param  fnode_addr  Where to store first node address
*   \return Number of credentials for service
*/
uint16_t logic_database_get_number_of_creds_for_service(uint16_t parent_addr, uint16_t* fnode_addr)
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
    *fnode_addr = next_node_addr;
    
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
        
        /* Increment counter */
        return_val++;
        
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

/*! \fn     logic_database_update_credential(uint16_t child_addr, cust_char_t* desc, cust_char_t* third, uint8_t* password)
*   \brief  Update existing credential
*   \param  child_addr  Child address
*   \param  desc        Pointer to description string, or 0 if not specified
*   \param  third       Pointer to arbitrary third field, or 0 if not specified
*   \param  password    Pointer to encrypted password, or 0 if not specified
*/
void logic_database_update_credential(uint16_t child_addr, cust_char_t* desc, cust_char_t* third, uint8_t* password)
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
        memcpy(temp_cnode.password, password, sizeof(temp_cnode.password));
    }
    
    /* Then write back to flash at same address */
    nodemgmt_write_child_node_block_to_flash(child_addr, (child_node_t*)&temp_cnode);
}    

/*! \fn     logic_database_add_credential_for_service(uint16_t service_addr, cust_char_t* login, cust_char_t* desc, cust_char_t* third, uint8_t* password)
*   \brief  Add a new credential for a given service to our database
*   \param  service_addr    Service address
*   \param  login           Pointer to login string
*   \param  desc            Pointer to description string, or 0 if not specified
*   \param  third           Pointer to arbitrary third field, or 0 if not specified
*   \param  passwordPointer to encrypted password, or 0 if not specified
*   \return Success status
*/
RET_TYPE logic_database_add_credential_for_service(uint16_t service_addr, cust_char_t* login, cust_char_t* desc, cust_char_t* third, uint8_t* password)
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
        memcpy(temp_cnode.password, password, sizeof(temp_cnode.password));
    }

    /* Then create node */
    return nodemgmt_create_child_node(service_addr, &temp_cnode, &storage_addr);
}

/*! \fn     logic_database_fill_get_cred_message_answer(uint16_t child_node_addr, hid_message_t* send_msg, uint8_t* cred_ctr)
*   \brief  Fill a get cred message packet
*   \param  child_node_addr Child node address
*   \param  send_msg        Pointer to send message
*   \param  cred_ctr        Where to store credential CTR
*   \return Payload size
*/
uint16_t logic_database_fill_get_cred_message_answer(uint16_t child_node_addr, hid_message_t* send_msg, uint8_t* cred_ctr)
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
                    + sizeof(temp_cnode.password) + 4 \
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
    memcpy(&(send_msg->get_credential_answer.concatenated_strings[current_index]), temp_cnode.password, sizeof(temp_cnode.password));
    current_index += (sizeof(temp_cnode.password)/sizeof(cust_char_t)) + 1;
    
    /* Copy CTR */
    memcpy(cred_ctr, temp_cnode.ctr, sizeof(temp_cnode.ctr));
    
    return current_index*sizeof(cust_char_t) + sizeof(send_msg->get_credential_answer.login_name_index) + sizeof(send_msg->get_credential_answer.description_index) + sizeof(send_msg->get_credential_answer.third_field_index) + sizeof(send_msg->get_credential_answer.password_index);
}