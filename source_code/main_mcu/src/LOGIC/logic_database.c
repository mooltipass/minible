/*!  \file     logic_database.c
*    \brief    General logic for database operations
*    Created:  17/03/2019
*    Author:   Mathieu Stephan
*/
#include "logic_database.h"
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