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
/*!  \file     nodemgmt.c
*    \brief    Node management library
*    Created:  14/12/2018
*    Author:   Mathieu Stephan
*/
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include "comms_hid_msgs_debug.h"
#include "nodemgmt.h"
#include "dbflash.h"
#include "utils.h"
#include "main.h"

// Current node management handle
nodemgmtHandle_t nodemgmt_current_handle;
// Current date
uint16_t nodemgmt_current_date;


/*! \fn     nodemgmt_set_current_date(uint16_t date)
*   \brief  Sets current date
*   \param  date    The date (see format in documentation)
*/
void nodemgmt_set_current_date(uint16_t date)
{
    nodemgmt_current_date = date;
}

/*! \fn     nodemgmt_get_incremented_address(uint16_t addr)
*   \brief  Get next address for a given address
*   \param  addr   The base address
*   \return The next address for our addressing scheme
 */
uint16_t nodemgmt_get_incremented_address(uint16_t addr)
{
    _Static_assert((BYTES_PER_PAGE == BASE_NODE_SIZE) || (BYTES_PER_PAGE == 2*BASE_NODE_SIZE), "Page size isn't 1 or 2 base node size");
    _Static_assert(NODEMGMT_ADDR_PAGE_BITSHIFT == 1, "Addressing scheme doesn't fit 1 or 2 base node size per page");
    
    #if (BYTES_PER_PAGE == BASE_NODE_SIZE)
        /* One node per page, change page */
        return addr + (1 << NODEMGMT_ADDR_PAGE_BITSHIFT);
    #else
        /* 2 nodes per page */
        return addr + 1;
    #endif
}

/*! \fn     constructAddress(uint16_t pageNumber, uint8_t nodeNumber)
*   \brief  Constructs an address for node storage in memory consisting of a page number and node number
*   \param  pageNumber      The page number to be encoded
*   \param  nodeNumber      The node number to be encoded
*   \return address         The constructed / encoded address
*   \note   No error checking is performed
*   \note   See design notes for address format
*   \note   Max Page Number and Node Number vary per flash size
 */
static inline uint16_t constructAddress(uint16_t pageNumber, uint8_t nodeNumber)
{
    return ((pageNumber << NODEMGMT_ADDR_PAGE_BITSHIFT) | ((uint16_t)nodeNumber & NODEMGMT_ADDR_NODE_MASK));
}

/*! \fn     nodeTypeFromFlags(uint16_t flags)
*   \brief  Gets nodeType from flags  
*   \param  flags           The flags field of a node
*   \return nodeType        See enum
*/
static inline node_type_te nodeTypeFromFlags(uint16_t flags)
{
    return (flags >> NODEMGMT_TYPE_FLAG_BITSHIFT) & NODEMGMT_TYPE_FLAG_BITMASK_FINAL;
}

/*! \fn     validBitFromFlags(uint16_t flags)
*   \brief  Gets the node valid bit from flags  
*   \return The valid 
*/
static inline uint16_t validBitFromFlags(uint16_t flags)
{
    return ((flags >> NODEMGMT_VALID_BIT_BITSHIFT) & NODEMGMT_VALID_BIT_MASK_FINAL);
}

/*! \fn     correctFlagsBitFromFlags(uint16_t flags)
*   \brief  Gets the correct flags valid bit from flags  
*   \return The valid 
*/
static inline uint16_t correctFlagsBitFromFlags(uint16_t flags)
{
    return ((flags >> NODEMGMT_CORRECT_FLAGS_BIT_BITSHIFT) & NODEMGMT_CORRECT_FLAGS_BIT_BITMASK_FINAL);
}

 /*! \fn     userIdFromFlags(uint16_t flags)
 *   \brief  Gets the user id from flags
 *   \return User ID
 */
static inline uint16_t userIdFromFlags(uint16_t flags)
{
    return ((flags >> NODEMGMT_USERID_BITSHIFT) & NODEMGMT_USERID_MASK_FINAL);
}

/*! \fn     nodemgmt_construct_date(uint16_t year, uint16_t month, uint16_t day)
*   \brief  Packs a uint16_t type with a date code in format YYYYYYYMMMMDDDDD. Year Offset from 2010
*   \param  year            The year to pack into the uint16_t
*   \param  month           The month to pack into the uint16_t
*   \param  day             The day to pack into the uint16_t
*   \return date            The constructed / encoded date in uint16_t
*/
uint16_t nodemgmt_construct_date(uint16_t year, uint16_t month, uint16_t day)
{
    /* Why swap16 you ask? well... it's an old error that propagated until now... */
    return swap16(day | (((month-1) << NODEMGMT_MONTH_SHT) & NODEMGMT_MONTH_MASK) | (((year-2010) << NODEMGMT_YEAR_SHT) & NODEMGMT_YEAR_MASK));
}

/*! \fn     extractDate(uint16_t date, uint8_t *year, uint8_t *month, uint8_t *day)
*   \brief  Unpacks a unint16_t to extract the year, month, and day information in format of YYYYYYYMMMMDDDDD. Year Offset from 2010
*   \param  year            The unpacked year
*   \param  month           The unpacked month
*   \param  day             The unpacked day
*   \return success status
*/
/*RET_TYPE extractDate(uint16_t date, uint8_t *year, uint8_t *month, uint8_t *day)
{
    *year = ((date >> NODEMGMT_YEAR_SHT) & NODEMGMT_YEAR_MASK_FINAL);
    *month = ((date >> NODEMGMT_MONTH_SHT) & NODEMGMT_MONTH_MASK_FINAL);
    *day = (date & NODEMGMT_DAY_MASK_FINAL);
    return RETURN_OK;
}*/

/*! \fn     nodemgmt_check_user_perm_from_flags(uint16_t flags)
*   \brief  Check that the user has the right to read/write a node
*   \param  flags       Flags contents
*   \return OK / NOK
*/
RET_TYPE nodemgmt_check_user_perm_from_flags(uint16_t flags)
{
    // Either the node belongs to us or it is invalid, check that the address is after sector 1 (upper check done at the flashread/write level)
    if ((nodemgmt_current_handle.currentUserId == userIdFromFlags(flags)) || (validBitFromFlags(flags) == NODEMGMT_VBIT_INVALID))
    {
        return RETURN_OK;
    }
    else
    {
        return RETURN_NOK;
    }
}

/*! \fn     nodemgmt_check_user_perm_from_flags_and_lock(uint16_t flags)
*   \brief  Check that the user has the right to read/write a node, lock if not
*   \param  flags       Flags contents
*/
void nodemgmt_check_user_perm_from_flags_and_lock(uint16_t flags)
{
    if (nodemgmt_check_user_perm_from_flags(flags) != RETURN_OK)
    {
        while(1);
    }
}

/*! \fn     nodemgmt_check_user_permission(uint16_t node_addr, node_type_te* node_type)
*   \brief  Check that the user has the right to read/write a node
*   \param  node_addr   Node address
*   \param  node_type   Where to store the node type (see enum)
*   \return OK / NOK
*   \note   Scanning a 8Mb Flash memory contents with that function was timed at 56ms in Debug mode.
*/
RET_TYPE nodemgmt_check_user_permission(uint16_t node_addr, node_type_te* node_type)
{
    // Future node flags
    uint16_t temp_flags;
    // Node Page
    uint16_t page_addr = nodemgmt_page_from_address(node_addr);
    // Node byte address
    uint16_t byte_addr = BASE_NODE_SIZE * nodemgmt_node_from_address(node_addr);
    
    // Fetch the flags
    dbflash_read_data_from_flash(&dbflash_descriptor, page_addr, byte_addr, sizeof(temp_flags), (void*)&temp_flags);
    
    // Check permission and memory boundaries (high boundary done on the lower level)
    if ((page_addr >= PAGE_PER_SECTOR) && (nodemgmt_check_user_perm_from_flags(temp_flags) == RETURN_OK))
    {
        /* Store node type */
        if (validBitFromFlags(temp_flags) == NODEMGMT_VBIT_VALID)
        {
            *node_type = nodeTypeFromFlags(temp_flags);
        } 
        else
        {
            *node_type = NODE_TYPE_NULL;
        }
        
        return RETURN_OK;        
    } 
    else
    {
        return RETURN_NOK;
    }
}

/*! \fn     nodemgmt_write_parent_node_data_block_to_flash(uint16_t address, parent_node_t* parent_node)
*   \brief  Write a parent node data block to flash
*   \param  address     Where to write
*   \param  parent_node Pointer to the node
*/
void nodemgmt_write_parent_node_data_block_to_flash(uint16_t address, parent_node_t* parent_node)
{
    _Static_assert(BASE_NODE_SIZE == sizeof(*parent_node), "Parent node isn't the size of base node size");    
    nodemgmt_user_id_to_flags(&(parent_node->cred_parent.flags), nodemgmt_current_handle.currentUserId);
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_page_from_address(address), BASE_NODE_SIZE * nodemgmt_node_from_address(address), BASE_NODE_SIZE, (void*)parent_node->node_as_bytes);
}

/*! \fn     nodemgmt_write_child_node_block_to_flash(uint16_t address, child_node_t* child_node, BOOL write_category)
*   \brief  Write a child node data block to flash
*   \param  address         Where to write
*   \param  parent_node     Pointer to the node
*   \param  write_category  Set to TRUE to write category to flags
*/
void nodemgmt_write_child_node_block_to_flash(uint16_t address, child_node_t* child_node, BOOL write_category)
{
    /* Enforce user ID */
    _Static_assert(2*BASE_NODE_SIZE == sizeof(*child_node), "Child node isn't twice the size of base node size");
    nodemgmt_user_id_to_flags(&(child_node->cred_child.flags), nodemgmt_current_handle.currentUserId);
    nodemgmt_user_id_to_flags(&(child_node->cred_child.fakeFlags), nodemgmt_current_handle.currentUserId);
    child_node->cred_child.fakeFlags |= (NODEMGMT_VBIT_INVALID << NODEMGMT_CORRECT_FLAGS_BIT_BITSHIFT);
    
    /* Write category flags if we're asked */
    if (write_category != FALSE)
    {
        // CATSEARCHLOGIC
        nodemgmt_categoryflags_to_flags(&(child_node->cred_child.flags), nodemgmt_current_handle.currentCategoryFlags);
        nodemgmt_categoryflags_to_flags(&(child_node->cred_child.fakeFlags), nodemgmt_current_handle.currentCategoryFlags);
    }
    
    /* Write to flash */
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_page_from_address(address), BASE_NODE_SIZE * nodemgmt_node_from_address(address), BASE_NODE_SIZE, (void*)child_node->node_as_bytes);
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_page_from_address(nodemgmt_get_incremented_address(address)), BASE_NODE_SIZE * nodemgmt_node_from_address(nodemgmt_get_incremented_address(address)), BASE_NODE_SIZE, (void*)(&child_node->node_as_bytes[BASE_NODE_SIZE]));
}

/*! \fn     nodemgmt_read_parent_node_data_block_from_flash(uint16_t address, parent_node_t* parent_node)
*   \brief  Read a parent node data block to flash
*   \param  address     Where to read
*   \param  parent_node Pointer to the node
*/
void nodemgmt_read_parent_node_data_block_from_flash(uint16_t address, parent_node_t* parent_node)
{
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_page_from_address(address), BASE_NODE_SIZE * nodemgmt_node_from_address(address), sizeof(parent_node->node_as_bytes), (void*)parent_node->node_as_bytes);
}

/*! \fn     nodemgmt_read_parent_node(uint16_t address, parent_node_t* parent_node, BOOL data_clean)
*   \brief  Read a parent node
*   \param  address     Where to read
*   \param  parent_node Pointer to the node
*   \param  data_clean  Clean the strings
*   \note   what's different from function above: sec checks
*/
void nodemgmt_read_parent_node(uint16_t address, parent_node_t* parent_node, BOOL data_clean)
{
    nodemgmt_read_parent_node_data_block_from_flash(address, parent_node);
    nodemgmt_check_user_perm_from_flags_and_lock(parent_node->cred_parent.flags);
    
    if (data_clean != FALSE)
    {
        parent_node->cred_parent.service[(sizeof(parent_node->cred_parent.service)/sizeof(parent_node->cred_parent.service[0]))-1] = 0;
    }
}

/*! \fn     nodemgmt_read_child_node_data_block_from_flash(uint16_t address, child_node_t* child_node)
*   \brief  Read a parent node data block to flash
*   \param  address     Where to read
*   \param  parent_node Pointer to the node
*/
void nodemgmt_read_child_node_data_block_from_flash(uint16_t address, child_node_t* child_node)
{
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_page_from_address(address), BASE_NODE_SIZE * nodemgmt_node_from_address(address), sizeof(child_node->node_as_bytes), (void*)child_node->node_as_bytes);
}

/*! \fn     nodemgmt_read_cred_child_node(uint16_t address, child_cred_node_t* child_node)
*   \brief  Read a child node
*   \param  address     Where to read
*   \param  child_node  Pointer to the node
*   \note   what's different from function above: sec checks & timestamp updates
*/
void nodemgmt_read_cred_child_node(uint16_t address, child_cred_node_t* child_node)
{
    nodemgmt_read_child_node_data_block_from_flash(address, (child_node_t*)child_node);
    nodemgmt_check_user_perm_from_flags_and_lock(child_node->flags);
    
    // If we have a date, update last used field
    if ((nodemgmt_current_date != 0x0000) && (child_node->dateLastUsed != nodemgmt_current_date))
    {
        // Just update the good field and write at the same place
        child_node->dateLastUsed = nodemgmt_current_date;
        nodemgmt_write_child_node_block_to_flash(address, (child_node_t*)child_node, FALSE);
    }
    
    // String cleaning
    child_node->pwdTerminatingZero = 0;
    child_node->login[(sizeof(child_node->login)/sizeof(child_node->login[0]))-1] = 0;
    child_node->thirdField[(sizeof(child_node->thirdField)/sizeof(child_node->thirdField[0]))-1] = 0;
    child_node->description[(sizeof(child_node->description)/sizeof(child_node->description[0]))-1] = 0;
}

/*! \fn     nodemgmt_read_cred_child_node_except_pwd(uint16_t address, child_cred_node_t* child_node)
*   \brief  Read a child node but not the password fields
*   \param  address     Where to read
*   \param  child_node  Pointer to the node
*/
void nodemgmt_read_cred_child_node_except_pwd(uint16_t address, child_cred_node_t* child_node)
{
    nodemgmt_read_parent_node_data_block_from_flash(address, (parent_node_t*)child_node);
    nodemgmt_check_user_perm_from_flags_and_lock(child_node->flags);
    
    // String cleaning
    child_node->login[(sizeof(child_node->login)/sizeof(child_node->login[0]))-1] = 0;
    child_node->thirdField[(sizeof(child_node->thirdField)/sizeof(child_node->thirdField[0]))-1] = 0;
    child_node->description[(sizeof(child_node->description)/sizeof(child_node->description[0]))-1] = 0;
}

/*! \fn     nodemgmt_get_user_profile_starting_offset(uint8_t uid, uint16_t *page, uint16_t *pageOffset)
    \brief  Obtains page and page offset for a given user id
    \param  uid             The id of the user to perform that profile page and offset calculation (0 up to NODE_MAX_UID)
    \param  page            The page containing the user profile
    \param  pageOffset      The offset of the page that indicates the start of the user profile
 */
void nodemgmt_get_user_profile_starting_offset(uint16_t uid, uint16_t *page, uint16_t *pageOffset)
{
    if(uid >= NB_MAX_USERS)
    {
        /* No debug... no reason it should get stuck here as the rest of the code shouldn't allow this */
        while(1);
    }

    /* Check for bad surprises */    
    _Static_assert(NODEMGMT_USER_PROFILE_SIZE == sizeof(nodemgmt_userprofile_t), "User profile isn't the right size");
    
    #if BYTES_PER_PAGE == NODEMGMT_USER_PROFILE_SIZE
        *page = uid*2;
        *pageOffset = 0;
    #elif BYTES_PER_PAGE == 2*NODEMGMT_USER_PROFILE_SIZE
        *page = uid;
        *pageOffset = 0;
    #else
        #error "User profile isn't a multiple of page size"
    #endif
}

/*! \fn     nodemgmt_get_bluetooth_bonding_info_starting_offset(uint8_t uid, uint16_t *page, uint16_t *pageOffset)
    \brief  Obtains page and page offset for a bonding information id
    \param  uid             The id of the bonding information to perform that profile page and offset calculation
    \param  page            The page containing the bonding information
    \param  pageOffset      The offset of the page that indicates the start of the bonding information
 */
void nodemgmt_get_bluetooth_bonding_info_starting_offset(uint16_t uid, uint16_t *page, uint16_t *pageOffset)
{
    if(uid >= NB_MAX_BONDING_INFORMATION)
    {
        /* No debug... no reason it should get stuck here as the rest of the code shouldn't allow this */
        while(1);
    }

    /* Check for bad surprises */
    _Static_assert(NODEMGMT_USER_PROFILE_SIZE == sizeof(nodemgmt_userprofile_t), "User profile isn't the right size");
    _Static_assert(NODEMGMT_USER_PROFILE_SIZE == 2*sizeof(nodemgmt_bluetooth_bonding_information_t), "Bluetooth bonding information isn't the right size");
    
    /* Compute the offset: after the last user profile */
    #if BYTES_PER_PAGE == NODEMGMT_USER_PROFILE_SIZE
        *page = NODEMGMT_BTBONDINFO_VUSER_SLOT_START*2;
        *pageOffset = 0;
    #elif BYTES_PER_PAGE == 2*NODEMGMT_USER_PROFILE_SIZE
        *page = NODEMGMT_BTBONDINFO_VUSER_SLOT_START;
        *pageOffset = 0;
    #else
        #error "User profile isn't a multiple of page size"
    #endif
    
    /* Then add from there */
    #if BYTES_PER_PAGE == NODEMGMT_USER_PROFILE_SIZE
        *page += uid >> 1;
        *pageOffset += (uid & 0x0001)*sizeof(nodemgmt_bluetooth_bonding_information_t);
    #elif BYTES_PER_PAGE == 2*NODEMGMT_USER_PROFILE_SIZE
        *page += uid >> 2;
        *pageOffset += (uid & 0x0003)*sizeof(nodemgmt_bluetooth_bonding_information_t);
    #else
        #error "User profile isn't a multiple of page size"
    #endif
}

/*! \fn     nodemgmt_get_user_category_names_starting_offset(uint8_t uid, uint16_t *page, uint16_t *pageOffset)
    \brief  Obtains page and page offset for a given user id favorite strings
    \param  uid             The id of the user to perform that profile page and offset calculation (0 up to NODE_MAX_UID)
    \param  page            The page containing the user favorite strings
    \param  pageOffset      The offset of the page that indicates the start of the user favorite strings
 */
void nodemgmt_get_user_category_names_starting_offset(uint16_t uid, uint16_t *page, uint16_t *pageOffset)
{
    if(uid >= NB_MAX_USERS)
    {
        /* No debug... no reason it should get stuck here as the data format doesn't allow such values */
        while(1);
    }

    /* Check for bad surprises */    
    _Static_assert(NODEMGMT_USER_PROFILE_SIZE == sizeof(nodemgmt_userprofile_t), "User profile isn't the right size");
    
    #if BYTES_PER_PAGE == NODEMGMT_USER_PROFILE_SIZE
        *page = uid*2 + 1;
        *pageOffset = 0;
    #elif BYTES_PER_PAGE == 2*NODEMGMT_USER_PROFILE_SIZE
        *page = uid;
        *pageOffset = NODEMGMT_USER_PROFILE_SIZE;
    #else
        #error "User profile isn't a multiple of page size"
    #endif
}

/*! \fn     nodemgmt_format_user_profile(uint16_t uid, uint16_t secPreferences, uint16_t languageId)
 *  \brief  Formats the user profile flash memory of user uid.
 *  \param  uid             The id of the user to format profile memory
 *  \param  secPreferences  User security preferences
 *  \param  languageId      User language
 *  \param  keyboardId      Keyboard ID
 */
void nodemgmt_format_user_profile(uint16_t uid, uint16_t secPreferences, uint16_t languageId, uint16_t keyboardId)
{
    /* Page & offset for this UID */
    uint16_t temp_page, temp_offset;
    
    if(uid >= NB_MAX_USERS)
    {
        /* No debug... no reason it should get stuck here as the data format doesn't allow such values */
        while(1);
    }
    
    #if NODE_ADDR_NULL != 0x0000
        #error "NODE_ADDR_NULL != 0x0000"
    #endif
    
    // Set buffer to all 0's.
    nodemgmt_get_user_profile_starting_offset(uid, &temp_page, &temp_offset);
    dbflash_write_data_pattern_to_flash(&dbflash_descriptor, temp_page, temp_offset, sizeof(nodemgmt_userprofile_t), 0x00);
    dbflash_write_data_to_flash(&dbflash_descriptor, temp_page, temp_offset + (size_t)offsetof(nodemgmt_userprofile_t, main_data.sec_preferences), sizeof(secPreferences), (void*)&secPreferences);
    dbflash_write_data_to_flash(&dbflash_descriptor, temp_page, temp_offset + (size_t)offsetof(nodemgmt_userprofile_t, main_data.language_id), sizeof(languageId), (void*)&languageId);
    dbflash_write_data_to_flash(&dbflash_descriptor, temp_page, temp_offset + (size_t)offsetof(nodemgmt_userprofile_t, main_data.layout_id), sizeof(keyboardId), (void*)&keyboardId);
}

/*! \fn     nodemgmt_store_bluetooth_bonding_information(nodemgmt_bluetooth_bonding_information_t* bonding_information)
 *  \brief  Store bluetooth bonding information
 *  \param  bonding_information Pointer to a bonding information struct
 *  \return RETURN_OK if we could store bonding information
 */
RET_TYPE nodemgmt_store_bluetooth_bonding_information(nodemgmt_bluetooth_bonding_information_t* bonding_information)
{
    bonding_information->zero_to_be_valid = 0x0000;
    uint16_t zero_to_be_valid_read_from_flash;
    uint16_t temp_page, temp_page_offset;
    BOOL found_empty_slot = FALSE;
    uint16_t temp_uid;
    
    /* Check for bad surprises */
    _Static_assert(BASE_NODE_SIZE == 2*sizeof(nodemgmt_bluetooth_bonding_information_t), "Bonding information struct isn't the right size");
    
    /* Find an available slot */
    for (temp_uid = 0; temp_uid < NB_MAX_BONDING_INFORMATION; temp_uid++)
    {
        /* Get page and offset */
        nodemgmt_get_bluetooth_bonding_info_starting_offset(temp_uid, &temp_page, &temp_page_offset);
        
        /* Check for empty slot */
        dbflash_read_data_from_flash(&dbflash_descriptor, temp_page, temp_page_offset + (size_t)offsetof(nodemgmt_bluetooth_bonding_information_t, zero_to_be_valid), sizeof(zero_to_be_valid_read_from_flash), &zero_to_be_valid_read_from_flash);
        
        /* Empty? */
        if (zero_to_be_valid_read_from_flash != 0x0000)
        {
            found_empty_slot = TRUE;
            break;
        }
    }
    
    /* Did we find a slot? */
    if (found_empty_slot == FALSE)
    {
        return RETURN_NOK;
    } 
    else
    {
        /* Then store the bonding information */
        dbflash_write_data_to_flash(&dbflash_descriptor, temp_page, temp_page_offset, sizeof(nodemgmt_bluetooth_bonding_information_t), (void*)bonding_information);
        return RETURN_OK;
    }    
}

/*! \fn     nodemgmt_get_bluetooth_bonding_information_for_mac_addr(uint8_t* mac_address, nodemgmt_bluetooth_bonding_information_t* bonding_information)
 *  \brief  Get a possible bluetooth bonding information for a given mac address
 *  \param  bonding_information Pointer to a where to store bonding information struct if found
 *  \return RETURN_OK if we found bonding information
 */
RET_TYPE nodemgmt_get_bluetooth_bonding_information_for_mac_addr(uint8_t* mac_address, nodemgmt_bluetooth_bonding_information_t* bonding_information)
{
    uint8_t mac_address_read[MEMBER_ARRAY_SIZE(nodemgmt_bluetooth_bonding_information_t, mac_address)];
    uint16_t zero_to_be_valid_read_from_flash;
    uint16_t temp_page, temp_page_offset;
    uint16_t temp_uid;
    
    /* Find an available slot */
    for (temp_uid = 0; temp_uid < NB_MAX_BONDING_INFORMATION; temp_uid++)
    {
        /* Get page and offset */
        nodemgmt_get_bluetooth_bonding_info_starting_offset(temp_uid, &temp_page, &temp_page_offset);
        
        /* Check for filled slot */
        dbflash_read_data_from_flash(&dbflash_descriptor, temp_page, temp_page_offset + (size_t)offsetof(nodemgmt_bluetooth_bonding_information_t, zero_to_be_valid), sizeof(zero_to_be_valid_read_from_flash), &zero_to_be_valid_read_from_flash);
        
        /* Read mac address */
        dbflash_read_data_from_flash(&dbflash_descriptor, temp_page, temp_page_offset + (size_t)offsetof(nodemgmt_bluetooth_bonding_information_t, mac_address), sizeof(mac_address_read), mac_address_read);
        
        /* Found it? */
        if ((zero_to_be_valid_read_from_flash == 0x0000) && (memcmp(mac_address_read, mac_address, sizeof(mac_address_read)) == 0))
        {
            dbflash_read_data_from_flash(&dbflash_descriptor, temp_page, temp_page_offset, sizeof(nodemgmt_bluetooth_bonding_information_t), (void*)bonding_information);
            return RETURN_OK;
        }
    }
    
    /* Didn't find what we were looking for */
    return RETURN_NOK;   
}

/*! \fn     nodemgmt_store_user_sec_preferences(uint16_t sec_preferences)
 *  \brief  Store user security preferences
 *  \param  sec_preferences Security preferences bitfield
 */
void nodemgmt_store_user_sec_preferences(uint16_t sec_preferences)
{
    // Write data parent address in the user profile page
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.sec_preferences), sizeof(sec_preferences), (void*)&sec_preferences);
}

/*! \fn     nodemgmt_get_user_sec_preferences(void)
 *  \brief  Get user security preferences
 *  \return User security preferences bitfield
 */
uint16_t nodemgmt_get_user_sec_preferences(void)
{
    uint16_t user_sec_flags;
    
    // Each user profile is within a page, data starting parent node is at the end of the favorites
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.sec_preferences), sizeof(user_sec_flags), &user_sec_flags);
    
    return user_sec_flags;
}

/*! \fn     nodemgmt_store_user_language(uint16_t languageId)
 *  \brief  Store user language
 *  \param  languageId  User language ID
 */
void nodemgmt_store_user_language(uint16_t languageId)
{
    // Write data parent address in the user profile page
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.language_id), sizeof(languageId), (void*)&languageId);
}

/*! \fn     nodemgmt_store_user_layout(uint16_t layoutId)
 *  \brief  Store user layout
 *  \param  layoutId    User layout ID
 */
void nodemgmt_store_user_layout(uint16_t layoutId)
{
    // Write data parent address in the user profile page
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.layout_id), sizeof(layoutId), (void*)&layoutId);
}

/*! \fn     nodemgmt_store_user_ble_layout(uint16_t layoutId)
 *  \brief  Store user BLE layout
 *  \param  layoutId    User layout ID
 */
void nodemgmt_store_user_ble_layout(uint16_t layoutId)
{
    nodemgmt_userprofile_t* const dirty_address_finding_trick = (nodemgmt_userprofile_t*)0;
    
    // Write data parent address in the user profile page
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)&(dirty_address_finding_trick->main_data.ble_layout_id), sizeof(layoutId), (void*)&layoutId);
}

/*! \fn     nodemgmt_get_user_language(void)
 *  \brief  Get user language
 *  \return User language ID
 */
uint16_t nodemgmt_get_user_language(void)
{
    uint16_t language_id;
    
    // Each user profile is within a page, data starting parent node is at the end of the favorites
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.language_id), sizeof(language_id), &language_id);
    
    return language_id;
}

/*! \fn     nodemgmt_get_user_layout(void)
 *  \brief  Get user layout
 *  \return User layout ID
 */
uint16_t nodemgmt_get_user_layout(void)
{
    uint16_t layout_id;
    
    // Each user profile is within a page, data starting parent node is at the end of the favorites
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.layout_id), sizeof(layout_id), &layout_id);
    
    return layout_id;
}

/*! \fn     nodemgmt_get_user_ble_layout(void)
 *  \brief  Get user BLE layout
 *  \return User layout ID
 */
uint16_t nodemgmt_get_user_ble_layout(void)
{
    nodemgmt_userprofile_t* const dirty_address_finding_trick = (nodemgmt_userprofile_t*)0;
    uint16_t layout_id;
    
    // Each user profile is within a page, data starting parent node is at the end of the favorites
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)&(dirty_address_finding_trick->main_data.ble_layout_id), sizeof(layout_id), &layout_id);
    
    return layout_id;
}

/*! \fn     nodemgmt_get_prev_child_node_for_cur_category(uint16_t search_start_child_addr)
 *  \brief  Gets the prev child node for the current category
 *  \param  search_start_child_addr     The child address from which to start looking.
 *  \return The address or NODE_ADDR_NULL
 */
uint16_t nodemgmt_get_prev_child_node_for_cur_category(uint16_t search_start_child_addr)
{
    uint16_t prev_child_node_addr_to_scan = search_start_child_addr;
    uint16_t child_read_buffer[4];
    
    /* Sanity check for this hack */
    _Static_assert(0 == offsetof(child_cred_node_t, flags), "Incorrect buffer for flags & addr read");
    _Static_assert(2 == offsetof(child_cred_node_t, prevChildAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(4 == offsetof(child_cred_node_t, nextChildAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(6 == offsetof(child_cred_node_t, mirroredChildAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(sizeof(child_read_buffer) == MEMBER_SIZE(child_cred_node_t, flags) + MEMBER_SIZE(child_cred_node_t, prevChildAddress) + MEMBER_SIZE(child_cred_node_t, nextChildAddress) + MEMBER_SIZE(child_cred_node_t, mirroredChildAddress), "Incorrect buffer for flags & addr read");
    
    /* Hack to read flags & prev / next address */
    child_cred_node_t* child_node_pt = (child_cred_node_t*)child_read_buffer;
    
    /* Nothing before nothing :D */
    if (search_start_child_addr == NODE_ADDR_NULL)
    {
        return NODE_ADDR_NULL;
    }
    
    /* Read flags and prev/next address */
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_page_from_address(prev_child_node_addr_to_scan), BASE_NODE_SIZE*nodemgmt_node_from_address(prev_child_node_addr_to_scan), sizeof(child_read_buffer), &child_read_buffer);
    prev_child_node_addr_to_scan = child_node_pt->prevChildAddress;
    
    /* Loop */
    while (prev_child_node_addr_to_scan != NODE_ADDR_NULL)
    {
        /* Read flags and prev/next address */
        dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_page_from_address(prev_child_node_addr_to_scan), BASE_NODE_SIZE*nodemgmt_node_from_address(prev_child_node_addr_to_scan), sizeof(child_read_buffer), &child_read_buffer);

        /* Check if it is of the current selected category */
        if ((nodemgmt_current_handle.currentCategoryFlags == 0) || (categoryFromFlags(child_node_pt->flags) == nodemgmt_current_handle.currentCategoryFlags))
        {
            // CATSEARCHLOGIC
            return prev_child_node_addr_to_scan;
        }
        
        /* Store next address to scan */
        prev_child_node_addr_to_scan = child_node_pt->prevChildAddress;
    }
    
    return NODE_ADDR_NULL;
}    

/*! \fn     nodemgmt_get_next_child_node_for_cur_category(uint16_t search_start_child_addr)
 *  \brief  Gets the next child node for the current category
 *  \param  search_start_child_addr     The child address from which to start looking.
 *  \return The address or NODE_ADDR_NULL
 */
uint16_t nodemgmt_get_next_child_node_for_cur_category(uint16_t search_start_child_addr)
{
    /* We already have a function that does that, but the address it takes is inclusive...*/
    uint16_t child_read_buffer[4];
        
    /* Sanity check for this hack */
    _Static_assert(0 == offsetof(child_cred_node_t, flags), "Incorrect buffer for flags & addr read");
    _Static_assert(2 == offsetof(child_cred_node_t, prevChildAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(4 == offsetof(child_cred_node_t, nextChildAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(6 == offsetof(child_cred_node_t, mirroredChildAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(sizeof(child_read_buffer) == MEMBER_SIZE(child_cred_node_t, flags) + MEMBER_SIZE(child_cred_node_t, prevChildAddress) + MEMBER_SIZE(child_cred_node_t, nextChildAddress) + MEMBER_SIZE(child_cred_node_t, mirroredChildAddress), "Incorrect buffer for flags & addr read");
        
    /* Hack to read flags & prev / next address */
    child_cred_node_t* child_node_pt = (child_cred_node_t*)child_read_buffer;
        
    /* Read flags and prev/next address */
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_page_from_address(search_start_child_addr), BASE_NODE_SIZE*nodemgmt_node_from_address(search_start_child_addr), sizeof(child_read_buffer), &child_read_buffer);
    search_start_child_addr = child_node_pt->nextChildAddress;

    /* Use the other function */
    return nodemgmt_check_for_logins_with_category_in_parent_node(search_start_child_addr, nodemgmt_current_handle.currentCategoryFlags);
}    

/*! \fn     nodemgmt_check_for_logins_with_category_in_parent_node(uint16_t start_child_addr, uint16_t category_flags)
 *  \brief  See if a parent node contains children that have the desired category
 *  \param  start_child_addr    Address of the first child
 *  \param  category_flags      Desired category flags
 *  \return The address of the first child that has the desired category or NODE_ADDR_NULL
 */
uint16_t nodemgmt_check_for_logins_with_category_in_parent_node(uint16_t start_child_addr, uint16_t category_flags)
{
    uint16_t next_child_node_addr_to_scan = start_child_addr;
    uint16_t child_read_buffer[4];
    
    /* Sanity check for this hack */
    _Static_assert(0 == offsetof(child_cred_node_t, flags), "Incorrect buffer for flags & addr read");
    _Static_assert(2 == offsetof(child_cred_node_t, prevChildAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(4 == offsetof(child_cred_node_t, nextChildAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(6 == offsetof(child_cred_node_t, mirroredChildAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(sizeof(child_read_buffer) == MEMBER_SIZE(child_cred_node_t, flags) + MEMBER_SIZE(child_cred_node_t, prevChildAddress) + MEMBER_SIZE(child_cred_node_t, nextChildAddress) + MEMBER_SIZE(child_cred_node_t, mirroredChildAddress), "Incorrect buffer for flags & addr read");
    
    /* Hack to read flags & prev / next address */
    child_cred_node_t* child_node_pt = (child_cred_node_t*)child_read_buffer;
    
    /* Loop in children */
    while (next_child_node_addr_to_scan != NODE_ADDR_NULL)
    {
        /* Read flags and prev/next address */
        dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_page_from_address(next_child_node_addr_to_scan), BASE_NODE_SIZE*nodemgmt_node_from_address(next_child_node_addr_to_scan), sizeof(child_read_buffer), &child_read_buffer);
        
        /* Check if it is of the current selected category */
        if ((category_flags == 0) || (categoryFromFlags(child_node_pt->flags) == category_flags))
        {
            // CATSEARCHLOGIC
            return next_child_node_addr_to_scan;
        }
        
        /* Go to next child if there's any */
        next_child_node_addr_to_scan = child_node_pt->nextChildAddress;
    }
    
    return NODE_ADDR_NULL;
}

/*! \fn     nodemgmt_get_prev_parent_node_for_cur_category(uint16_t search_start_parent_addr)
 *  \brief  Gets the prev parent node for the current category
 *  \param  search_start_parent_addr    The parent address from which to start looking.
 *  \return The address or NODE_ADDR_NULL
 */
uint16_t nodemgmt_get_prev_parent_node_for_cur_category(uint16_t search_start_parent_addr)
{
    uint16_t prev_parent_node_addr_to_scan = nodemgmt_current_handle.firstParentNode;
    uint16_t parent_read_buffer[4];
    
    /* Sanity check for this hack */
    _Static_assert(0 == offsetof(parent_cred_node_t, flags), "Incorrect buffer for flags & addr read");
    _Static_assert(2 == offsetof(parent_cred_node_t, prevParentAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(4 == offsetof(parent_cred_node_t, nextParentAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(6 == offsetof(parent_cred_node_t, nextChildAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(sizeof(parent_read_buffer) == MEMBER_SIZE(parent_cred_node_t, flags) + MEMBER_SIZE(parent_cred_node_t, prevParentAddress) + MEMBER_SIZE(parent_cred_node_t, nextParentAddress) + MEMBER_SIZE(parent_cred_node_t, nextChildAddress), "Incorrect buffer for flags & addr read");
    
    /* Hack to read flags & prev / next address */
    parent_cred_node_t* parent_node_pt = (parent_cred_node_t*)parent_read_buffer;
    
    /* Nothing before nothing :D */
    if (search_start_parent_addr == NODE_ADDR_NULL)
    {
        return NODE_ADDR_NULL;
    }
    
    /* Read flags and prev/next address */
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_page_from_address(search_start_parent_addr), BASE_NODE_SIZE*nodemgmt_node_from_address(search_start_parent_addr), sizeof(parent_read_buffer), &parent_read_buffer);
    prev_parent_node_addr_to_scan = parent_node_pt->prevParentAddress;
    
    /* Loop */
    while (prev_parent_node_addr_to_scan != NODE_ADDR_NULL)
    {
        /* Read flags and prev/next address */
        dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_page_from_address(prev_parent_node_addr_to_scan), BASE_NODE_SIZE*nodemgmt_node_from_address(prev_parent_node_addr_to_scan), sizeof(parent_read_buffer), &parent_read_buffer);

        /* Check for logins with desired category */
        if (nodemgmt_check_for_logins_with_category_in_parent_node(parent_node_pt->nextChildAddress, nodemgmt_current_handle.currentCategoryFlags) != NODE_ADDR_NULL)
        {
            return prev_parent_node_addr_to_scan;
        }
        
        /* Store next address to scan */
        prev_parent_node_addr_to_scan = parent_node_pt->prevParentAddress;
    }
    
    return NODE_ADDR_NULL;
}

/*! \fn     nodemgmt_get_next_parent_node_for_cur_category(uint16_t search_start_parent_addr)
 *  \brief  Gets the next parent node for the current category
 *  \param  search_start_parent_addr    The parent address from which to start looking. Use NODE_ADDR_NULL to start from the beginning
 *  \return The address or NODE_ADDR_NULL
 */
uint16_t nodemgmt_get_next_parent_node_for_cur_category(uint16_t search_start_parent_addr)
{
    uint16_t next_parent_node_addr_to_scan = nodemgmt_current_handle.firstParentNode;
    uint16_t parent_read_buffer[4];
    
    /* Sanity check for this hack */
    _Static_assert(0 == offsetof(parent_cred_node_t, flags), "Incorrect buffer for flags & addr read");
    _Static_assert(2 == offsetof(parent_cred_node_t, prevParentAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(4 == offsetof(parent_cred_node_t, nextParentAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(6 == offsetof(parent_cred_node_t, nextChildAddress), "Incorrect buffer for flags & addr read");
    _Static_assert(sizeof(parent_read_buffer) == MEMBER_SIZE(parent_cred_node_t, flags) + MEMBER_SIZE(parent_cred_node_t, prevParentAddress) + MEMBER_SIZE(parent_cred_node_t, nextParentAddress) + MEMBER_SIZE(parent_cred_node_t, nextChildAddress), "Incorrect buffer for flags & addr read");
    
    /* Hack to read flags & prev / next address */
    parent_cred_node_t* parent_node_pt = (parent_cred_node_t*)parent_read_buffer;
    
    /* Get next address to check for for the below loop in case we're not starting from the very beginning */
    if (search_start_parent_addr != NODE_ADDR_NULL)
    {
        /* Read flags and prev/next address */
        dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_page_from_address(search_start_parent_addr), BASE_NODE_SIZE*nodemgmt_node_from_address(search_start_parent_addr), sizeof(parent_read_buffer), &parent_read_buffer);
        next_parent_node_addr_to_scan = parent_node_pt->nextParentAddress;
    }
    
    /* Loop */
    while (next_parent_node_addr_to_scan != NODE_ADDR_NULL)
    {
        /* Read flags and prev/next address */
        dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_page_from_address(next_parent_node_addr_to_scan), BASE_NODE_SIZE*nodemgmt_node_from_address(next_parent_node_addr_to_scan), sizeof(parent_read_buffer), &parent_read_buffer);

        /* Check for logins with desired category */
        if (nodemgmt_check_for_logins_with_category_in_parent_node(parent_node_pt->nextChildAddress, nodemgmt_current_handle.currentCategoryFlags) != NODE_ADDR_NULL)
        {
            return next_parent_node_addr_to_scan;
        }
        
        /* Store next address to scan */
        next_parent_node_addr_to_scan = parent_node_pt->nextParentAddress;
    }
    
    return NODE_ADDR_NULL;
}

/*! \fn     nodemgmt_get_starting_parent_addr_for_category(void)
 *  \brief  Gets the users starting parent node for the current category
 *  \return The address
 */
uint16_t nodemgmt_get_starting_parent_addr_for_category(void)
{
    return nodemgmt_get_next_parent_node_for_cur_category(NODE_ADDR_NULL);
}

/*! \fn     nodemgmt_get_starting_parent_addr(void)
 *  \brief  Gets the users starting parent node from the user profile memory portion of flash
 *  \return The address
 */
uint16_t nodemgmt_get_starting_parent_addr(void)
{
    uint16_t temp_address;
    
    // Each user profile is within a page, data starting parent node is at the end of the favorites
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + offsetof(nodemgmt_userprofile_t, main_data.cred_start_address), sizeof(temp_address), &temp_address);    
    
    return temp_address;
}

/*! \fn     nodemgmt_get_starting_data_parent_addr(uint16_t typeId)
 *  \brief  Gets the users starting data parent node from the user profile memory portion of flash
 *  \param  typeId  Type ID
 *  \return The address
 */
uint16_t nodemgmt_get_starting_data_parent_addr(uint16_t typeId)
{
    uint16_t temp_address;
    
    // type id check
    if (typeId >= (MEMBER_ARRAY_SIZE(nodemgmt_userprofile_t, main_data.data_start_address)))
    {
        return NODE_ADDR_NULL;
    }
    
    // Each user profile is within a page, data starting parent node is at the end of the favorites
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.data_start_address[typeId]), sizeof(temp_address), &temp_address);    
    
    return temp_address;
}

/*! \fn     nodemgmt_get_start_addresses(uint16_t* addresses_array)
 *  \brief  Get all start addresses at once
 *  \return Number of start addresses
 */
uint16_t nodemgmt_get_start_addresses(uint16_t* addresses_array)
{    
    // Write addresses in the user profile page. Possible as the credential start address & data start addresses are contiguous in memory
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.cred_start_address), sizeof(uint16_t) + MEMBER_SIZE(nodemgmt_profile_main_data_t, data_start_address), addresses_array);

    return 1 + MEMBER_ARRAY_SIZE(nodemgmt_profile_main_data_t, data_start_address);
}

/*! \fn     nodemgmt_get_cred_change_number(void)
 *  \brief  Gets the users change number from the user profile memory portion of flash
 *  \return The address
 */
uint32_t nodemgmt_get_cred_change_number(void)
{
    uint32_t change_number;
    
    // Each user profile is within a page, data starting parent node is at the end of the favorites
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.cred_change_number), sizeof(change_number), (void*)&change_number);    
    
    return change_number;
}

/*! \fn     nodemgmt_get_data_change_number(void)
 *  \brief  Gets the users data change number from the user profile memory portion of flash
 *  \return The address
 */
uint32_t nodemgmt_get_data_change_number(void)
{
    uint32_t change_number;
    
    // Each user profile is within a page, data starting parent node is at the end of the favorites
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.data_change_number), sizeof(change_number), (void*)&change_number);    
    
    return change_number;
}

/*! \fn     nodemgmt_set_cred_start_address(uint16_t parentAddress)
 *  \brief  Sets the users starting parent node both in the handle and user profile memory portion of flash
 *  \param  parentAddress   The constructed address of the users starting parent node
 */
void nodemgmt_set_cred_start_address(uint16_t parentAddress)
{
    // update handle
    nodemgmt_current_handle.firstParentNode = parentAddress;
    
    // Write parent address in the user profile page
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.cred_start_address), sizeof(parentAddress), &parentAddress);
}

/*! \fn     nodemgmt_set_data_start_address(uint16_t dataParentAddress, uint16_t typeId)
 *  \brief  Sets the users starting data parent node both in the handle and user profile memory portion of flash
 *  \param  dataParentAddress   The constructed address of the users starting parent node
 *  \param  typeId              The type ID
 */
void nodemgmt_set_data_start_address(uint16_t dataParentAddress, uint16_t typeId)
{
    // type id check
    if (typeId >= (MEMBER_ARRAY_SIZE(nodemgmt_userprofile_t, main_data.data_start_address)))
    {
        return;
    }
    
    // update handle
    nodemgmt_current_handle.firstDataParentNode[typeId] = dataParentAddress;
    
    // Write data parent address in the user profile page
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.data_start_address[typeId]), sizeof(dataParentAddress), &dataParentAddress);
}

/*! \fn     nodemgmt_set_start_addresses(uint16_t* addresses_array)
 *  \brief  Set all start addresses at once
 *  \param  addresses_array     An array containing the credential start address followed by all the data start addresses
 */
void nodemgmt_set_start_addresses(uint16_t* addresses_array)
{
    // update handle    
    nodemgmt_current_handle.firstParentNode = addresses_array[0];
    memcpy(nodemgmt_current_handle.firstDataParentNode, &(addresses_array[1]), MEMBER_SIZE(nodemgmt_profile_main_data_t, data_start_address));

    // Write addresses in the user profile page. Possible as the credential start address & data start addresses are contiguous in memory
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.cred_start_address), sizeof(uint16_t) + MEMBER_SIZE(nodemgmt_profile_main_data_t, data_start_address), addresses_array);
}

/*! \fn     nodemgmt_set_cred_change_number(uint32_t changeNumber)
 *  \brief  Set the credential change number
 *  \param  changeNumber    The new change number
 */
void nodemgmt_set_cred_change_number(uint32_t changeNumber)
{
    // Write data parent address in the user profile page
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.cred_change_number), sizeof(changeNumber), (void*)&changeNumber);
}

/*! \fn     nodemgmt_set_data_change_number(uint32_t changeNumber)
 *  \brief  Set the data change number
 *  \param  changeNumber    The new change number
 */
void nodemgmt_set_data_change_number(uint32_t changeNumber)
{
    // Write data parent address in the user profile page
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.data_change_number), sizeof(changeNumber), (void*)&changeNumber);
}

/*! \fn     nodemgmt_set_favorite(uint16_t categoryId, uint16_t favId, uint16_t parentAddress, uint16_t childAddress)
 *  \brief  Sets a user favorite in the user profile
 *  \param  categoryId      Category number of the fav record
 *  \param  favId           The id number of the fav record
 *  \param  parentAddress   The parent node address of the fav
 *  \param  childAddress    The child node address of the fav
 */
void nodemgmt_set_favorite(uint16_t categoryId, uint16_t favId, uint16_t parentAddress, uint16_t childAddress)
{
    favorite_addr_t favorite = {parentAddress, childAddress};
    
    if (categoryId >= (MEMBER_ARRAY_SIZE(nodemgmt_userprofile_t, category_favorites)))
    {
        while(1);
    }
    
    if(favId >= (MEMBER_ARRAY_SIZE(nodemgmt_userprofile_t, category_favorites[0].favorite)))
    {
        while(1);
    }

    // Write to flash    
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, category_favorites[categoryId].favorite[favId]), sizeof(favorite), (void*)&favorite);
}

/*! \fn     nodemgmt_read_favorite(uint16_t categoryId, uint16_t favId, uint16_t parentAddress, uint16_t childAddress)
 *  \brief  Reads a user favorite in the user profile
 *  \param  categoryId      Category number of the fav record
 *  \param  favId           The id number of the fav record
 *  \param  parentAddress   The parent node address of the fav
 *  \param  childAddress    The child node address of the fav
 */
void nodemgmt_read_favorite(uint16_t categoryId, uint16_t favId, uint16_t* parentAddress, uint16_t* childAddress)
{
    favorite_addr_t favorite;
        
    if (categoryId >= (MEMBER_ARRAY_SIZE(nodemgmt_userprofile_t, category_favorites)))
    {
        while(1);
    }
    
    if(favId >= (MEMBER_ARRAY_SIZE(nodemgmt_userprofile_t, category_favorites[0].favorite)))
    {
        while(1);
    }
    
    // Read from flash
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, category_favorites[categoryId].favorite[favId]), sizeof(favorite), (void*)&favorite);
    
    // return values to user
    *parentAddress = favorite.parent_addr;
    *childAddress = favorite.child_addr;
}

/*! \fn     nodemgmt_read_favorite_for_current_category(uint16_t favId, uint16_t* parentAddress, uint16_t* childAddress)
 *  \brief  Reads a user favorite in the user profile
 *  \param  favId           The id number of the fav record
 *  \param  parentAddress   The parent node address of the fav
 *  \param  childAddress    The child node address of the fav
 */
void nodemgmt_read_favorite_for_current_category(uint16_t favId, uint16_t* parentAddress, uint16_t* childAddress)
{
    favorite_addr_t favorite;
    
    if(favId >= (MEMBER_ARRAY_SIZE(nodemgmt_userprofile_t, category_favorites[0].favorite)))
    {
        while(1);
    }
    
    // Read from flash
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, category_favorites[nodemgmt_current_handle.currentCategoryId].favorite[favId]), sizeof(favorite), (void*)&favorite);
    
    // return values to user
    *parentAddress = favorite.parent_addr;
    *childAddress = favorite.child_addr;
}

/*! \fn     nodemgmt_get_next_non_null_favorite_after_index(uint16_t favId)
 *  \brief  Get a non-null favorite ID after a given one
 *  \param  favId           The id number at which we should start looking for favs
 *  \return A favorite ID or -1
 */
int16_t nodemgmt_get_next_non_null_favorite_after_index(uint16_t favId)
{
    favorite_addr_t favorite;
    
    /* Sanity checks */
    if(favId >= (MEMBER_ARRAY_SIZE(nodemgmt_userprofile_t, category_favorites[0].favorite)))
    {
        return -1;
    }
    
    /* Start looking */
    for (uint16_t i = favId; i < MEMBER_ARRAY_SIZE(favorites_for_category_t, favorite); i++)
    {
        // Read from flash
        dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, category_favorites[nodemgmt_current_handle.currentCategoryId].favorite[i]), sizeof(favorite), (void*)&favorite);

        // Valid favorite?
        if ((favorite.child_addr != NODE_ADDR_NULL) && (favorite.parent_addr != NODE_ADDR_NULL))
        {
            return i;
        }
    }
    
    return -1;
}

/*! \fn     nodemgmt_get_next_non_null_favorite_before_index(uint16_t favId)
 *  \brief  Get a non-null favorite ID after a given one
 *  \param  favId           The id number at which we should start looking for favs
 *  \return A favorite ID or -1
 */
int16_t nodemgmt_get_next_non_null_favorite_before_index(uint16_t favId)
{
    favorite_addr_t favorite;
    
    /* Sanity checks */
    if(favId >= (MEMBER_ARRAY_SIZE(nodemgmt_userprofile_t, category_favorites[0].favorite)))
    {
        return -1;
    }
    
    /* Start looking */
    for (int16_t i = favId; i >= 0; i--)
    {
        // Read from flash
        dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, category_favorites[nodemgmt_current_handle.currentCategoryId].favorite[i]), sizeof(favorite), (void*)&favorite);

        // Valid favorite?
        if ((favorite.child_addr != NODE_ADDR_NULL) && (favorite.parent_addr != NODE_ADDR_NULL))
        {
            return i;
        }
    }
    
    return -1;
}

/*! \fn     nodemgmt_get_favorites(uint16_t* addresses_array)
 *  \brief  Get all favorites at once
 *  \param  addresses_array     Where to store them
 *  \return Number of favorites written
 */
uint16_t nodemgmt_get_favorites(uint16_t* addresses_array)
{
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, category_favorites), MEMBER_SIZE(nodemgmt_userprofile_t,category_favorites), (void*)addresses_array);
    return MEMBER_SIZE(nodemgmt_userprofile_t,category_favorites)/sizeof(favorite_addr_t);
}

/*! \fn     nodemgmt_read_profile_ctr(void* buf)
 *  \brief  Reads the users base CTR from the user profile flash memory
 *  \param  buf             The buffer to store the read CTR
 */
void nodemgmt_read_profile_ctr(void* buf)
{
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.current_ctr), MEMBER_SIZE(nodemgmt_userprofile_t, main_data.current_ctr), buf);
}

/*! \fn     nodemgmt_set_profile_ctr(void* buf)
 *  \brief  Sets the user DB change number in the user profile flash memory
 *  \param  buf             The buffer containing the new CTR
 */
void nodemgmt_set_profile_ctr(void* buf)
{
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)offsetof(nodemgmt_userprofile_t, main_data.current_ctr), MEMBER_SIZE(nodemgmt_userprofile_t, main_data.current_ctr), buf);
}

/*! \fn     nodemgmt_get_category_strings(nodemgmt_user_category_strings_t* strings_pt)
 *  \brief  Get all category strings at once
 *  \param  strings_pt  Where to store the strings
 */
void nodemgmt_get_category_strings(nodemgmt_user_category_strings_t* strings_pt)
{
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserCategoryStrings, nodemgmt_current_handle.offsetUserCategoryStrings, sizeof(nodemgmt_user_category_strings_t), strings_pt);
    strings_pt->category_strings[0][MEMBER_SUB_ARRAY_SIZE(nodemgmt_user_category_strings_t, category_strings)-1] = 0;
    strings_pt->category_strings[1][MEMBER_SUB_ARRAY_SIZE(nodemgmt_user_category_strings_t, category_strings)-1] = 0;
    strings_pt->category_strings[2][MEMBER_SUB_ARRAY_SIZE(nodemgmt_user_category_strings_t, category_strings)-1] = 0;
    strings_pt->category_strings[3][MEMBER_SUB_ARRAY_SIZE(nodemgmt_user_category_strings_t, category_strings)-1] = 0;
}

/*! \fn     nodemgmt_set_category_strings(nodemgmt_user_category_strings_t* strings_pt)
 *  \brief  Set all category strings at once
 *  \param  strings_pt  Where the strings are stored
 */
void nodemgmt_set_category_strings(nodemgmt_user_category_strings_t* strings_pt)
{
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserCategoryStrings, nodemgmt_current_handle.offsetUserCategoryStrings, sizeof(nodemgmt_user_category_strings_t), strings_pt);
}

/*! \fn     nodemgmt_get_category_string(uint16_t string_id, cust_char_t* string_pt)
 *  \brief  Get a given user category string
 *  \param  category_id     Category ID
 *  \param  string_pt       Where to store the category string   
 */
void nodemgmt_get_category_string(uint16_t category_id, cust_char_t* string_pt)
{
    if (category_id >= MEMBER_ARRAY_SIZE(nodemgmt_user_category_strings_t, category_strings))
    {
        return;
    }
    
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserCategoryStrings, nodemgmt_current_handle.offsetUserCategoryStrings + (size_t)offsetof(nodemgmt_user_category_strings_t, category_strings[category_id]), MEMBER_SIZE(nodemgmt_user_category_strings_t, category_strings[0]), string_pt);
    string_pt[MEMBER_SUB_ARRAY_SIZE(nodemgmt_user_category_strings_t, category_strings)-1] = 0;
}

/*! \fn     nodemgmt_set_category_string(uint16_t string_id, cust_char_t* string_pt)
 *  \brief  Set a given user category string
 *  \param  category_id     Category ID
 *  \param  string_pt       Where to store the category string 
 *  \note   Input buffer must have at least the same size as the filesystem category string field  
 */
void nodemgmt_set_category_string(uint16_t category_id, cust_char_t* string_pt)
{
    if (category_id >= MEMBER_ARRAY_SIZE(nodemgmt_user_category_strings_t, category_strings))
    {
        return;
    }
    
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserCategoryStrings, nodemgmt_current_handle.offsetUserCategoryStrings + (size_t)offsetof(nodemgmt_user_category_strings_t, category_strings[category_id]), MEMBER_SIZE(nodemgmt_user_category_strings_t, category_strings[0]), string_pt);
}

/*! \fn     nodemgmt_find_free_nodes(uint16_t nbParentNodes, uint16_t* parentNodeArray, uint16_t nbChildtNodes, uint16_t* childNodeArray, uint16_t startPage, uint16_t startNode)
*   \brief  Find Free Nodes inside our external memory
*   \param  nbParentNodes   Number of parent nodes we want to find
*   \param  parentNodeArray An array where to store the addresses
*   \param  nbChildtNodes   Number of child nodes we want to find
*   \param  childNodeArray  An array where to store the addresses
*   \param  startPage       Page where to start the scanning
*   \param  startNode       Scan start node address inside the start page
*   \return the number of nodes found
*/
uint16_t nodemgmt_find_free_nodes(uint16_t nbParentNodes, uint16_t* parentNodeArray, uint16_t nbChildtNodes, uint16_t* childNodeArray, uint16_t startPage, uint16_t startNode)
{
    uint16_t prevFreeAddressFound = NODE_ADDR_NULL;
    uint16_t nbParentNodesFound = 0;
    uint16_t nbChildNodesFound = 0;
    uint16_t nodeFlags;
    uint16_t pageItr;
    uint16_t nodeItr;
    
    // Check the start page
    if (startPage < PAGE_PER_SECTOR)
    {
        startPage = PAGE_PER_SECTOR;
    }

    // for each page
    for(pageItr = startPage; pageItr < PAGE_COUNT; pageItr++)
    {
        // for each possible parent node in the page (changes per flash chip)
        for(nodeItr = startNode; nodeItr < BYTES_PER_PAGE/BASE_NODE_SIZE; nodeItr++)
        {
            // read node flags (2 bytes - fixed size)
            dbflash_read_data_from_flash(&dbflash_descriptor, pageItr, BASE_NODE_SIZE*nodeItr, sizeof(nodeFlags), &nodeFlags);
            
            // If this slot is OK
            if(validBitFromFlags(nodeFlags) == NODEMGMT_VBIT_INVALID)
            {
                // fill parent nodes first (only one block)
                if (nbParentNodesFound != nbParentNodes)
                {
                    parentNodeArray[nbParentNodesFound++] = constructAddress(pageItr, nodeItr);
                    
                    // check for end
                    if ((nbChildtNodes == 0) && (nbParentNodesFound == nbParentNodes))
                    {
                        return nbChildNodesFound+nbParentNodesFound;
                    }
                } 
                else
                {
                    if (prevFreeAddressFound == NODE_ADDR_NULL)
                    {
                        // Store address if the next free block found is available
                        prevFreeAddressFound = constructAddress(pageItr, nodeItr);
                    } 
                    else
                    {
                        childNodeArray[nbChildNodesFound++] = prevFreeAddressFound;
                        prevFreeAddressFound = NODE_ADDR_NULL;
                        
                        // check for end
                        if (nbChildNodesFound == nbChildtNodes)
                        {
                            return nbChildNodesFound+nbParentNodesFound;
                        }
                    }
                }
            }
            else
            {
                // block found isn't available, reset flag
                prevFreeAddressFound = NODE_ADDR_NULL;
            }
        }
        startNode = 0;
    }    
    
    return nbChildNodesFound+nbParentNodesFound;
}

/*! \fn     nodemgmt_scan_node_usage(void)
*   \brief  Scan memory to find empty slots
*/
void nodemgmt_scan_node_usage(void)
{
    // Find one free node. If we don't find it, set the next to the null addr, we start looking from the just taken node
    if (nodemgmt_find_free_nodes(1, &nodemgmt_current_handle.nextParentFreeNode, 1, &nodemgmt_current_handle.nextChildFreeNode, nodemgmt_page_from_address(nodemgmt_current_handle.nextParentFreeNode), nodemgmt_node_from_address(nodemgmt_current_handle.nextParentFreeNode)) != 2)
    {
        nodemgmt_current_handle.nextParentFreeNode = NODE_ADDR_NULL;
        nodemgmt_current_handle.nextChildFreeNode = NODE_ADDR_NULL;
    }
}

/*! \fn     nodemgmt_get_current_category_flags(void)
 *  \brief  Get current selected category ID in flag form
 *  \return The category in flag form
 */
uint16_t nodemgmt_get_current_category_flags(void)
{
    return nodemgmt_current_handle.currentCategoryFlags;
}

/*! \fn     nodemgmt_get_current_category(void)
 *  \brief  Get current selected category ID
 *  \return The category
 */
uint16_t nodemgmt_get_current_category(void)
{
    return nodemgmt_current_handle.currentCategoryId;    
}

/*! \fn     nodemgmt_set_current_category_id(uint16_t catId)
 *  \brief  Set current selected category ID
 *  \param  catId   The category ID
 */
void nodemgmt_set_current_category_id(uint16_t catId)
{
    if (catId < NODEMGMT_NB_MAX_CATEGORIES)
    {        
        nodemgmt_current_handle.currentCategoryId = catId;
        
        // Compute flags from category id
        if (catId == 0)
        {
            nodemgmt_current_handle.currentCategoryFlags = 0;
        } 
        else
        {
            nodemgmt_current_handle.currentCategoryFlags = 1 << (catId-1);
        }
    }
}

/*! \fn     nodemgmt_init_context(uint16_t userIdNum, uint16_t* userSecFlags, uint16_t* userLanguage, uint16_t* userLayout, uint16_t* userBLELayout)
 *  \brief  Initializes the Node Management Handle, scans memory for the next free node
 *  \param  userIdNum       The user id to initialize the handle for
 *  \param  userSecFlags    Pointer to where to store user security flags
 *  \param  userLanguage    Pointer to where to store user language
 *  \param  userLayout      Pointer to where to store user keyboard layout
 *  \param  userBLELayout   Pointer to where to store user BLE keyboard layout
 */
void nodemgmt_init_context(uint16_t userIdNum, uint16_t* userSecFlags, uint16_t* userLanguage, uint16_t* userLayout, uint16_t* userBLELayout)
{
    if(userIdNum >= NB_MAX_USERS)
    {
        /* No debug... no reason it should get stuck here as the data format doesn't allow such values */
        while(1);
    }
            
    // fill current user id, first parent node address, user profile page & offset
    nodemgmt_get_user_category_names_starting_offset(userIdNum, &nodemgmt_current_handle.pageUserCategoryStrings, &nodemgmt_current_handle.offsetUserCategoryStrings);
    nodemgmt_get_user_profile_starting_offset(userIdNum, &nodemgmt_current_handle.pageUserProfile, &nodemgmt_current_handle.offsetUserProfile);
    nodemgmt_current_handle.firstParentNode = nodemgmt_get_starting_parent_addr();
    nodemgmt_current_handle.currentUserId = userIdNum;
    nodemgmt_current_handle.currentCategoryFlags = 0;
    nodemgmt_current_handle.currentCategoryId = 0;
    nodemgmt_current_handle.datadbChanged = FALSE;
    nodemgmt_current_handle.dbChanged = FALSE;
    
    // Get starting data parents
    for (uint16_t i = 0; i < (sizeof(nodemgmt_current_handle.firstDataParentNode)/sizeof(nodemgmt_current_handle.firstDataParentNode[0])); i++)
    {        
        nodemgmt_current_handle.firstDataParentNode[i] = nodemgmt_get_starting_data_parent_addr(i);
    }
    
    // scan for next free parent and child nodes from the start of the memory
    nodemgmt_scan_node_usage();

    // Store user security preference and language
    *userSecFlags = nodemgmt_get_user_sec_preferences();
    *userBLELayout = nodemgmt_get_user_ble_layout();
    *userLanguage = nodemgmt_get_user_language();
    *userLayout = nodemgmt_get_user_layout();
}

/*! \fn     nodemgmt_user_db_changed_actions(BOOL dataChanged)
 *  \brief  Function called to inform that the DB has been changed
 *  \param  dataChanged  FALSE when a standard credential is changed, something else when it is a data node that is changed
 *  \note   Currently called on password change & add, 32B data write
 */
void nodemgmt_user_db_changed_actions(BOOL dataChanged)
{
    // Cred db change number
    if ((nodemgmt_current_handle.dbChanged == FALSE) && (dataChanged == FALSE))
    {
        uint32_t current_cred_change_number = nodemgmt_get_cred_change_number();
        current_cred_change_number++;
        nodemgmt_current_handle.dbChanged = TRUE;
        nodemgmt_set_cred_change_number(current_cred_change_number);
    }
    
    // Data db change number
    if ((nodemgmt_current_handle.datadbChanged == FALSE) && (dataChanged != FALSE))
    {
        uint32_t current_data_change_number = nodemgmt_get_data_change_number();
        current_data_change_number++;
        nodemgmt_current_handle.datadbChanged = TRUE;
        nodemgmt_set_data_change_number(current_data_change_number);        
    }
}

/*! \fn     nodemgmt_delete_current_user_from_flash(void)
*   \brief  Delete user data from flash
*/
void nodemgmt_delete_current_user_from_flash(void)
{
    uint16_t next_parent_addr = nodemgmt_current_handle.firstParentNode;
    uint16_t next_child_addr;
    uint16_t temp_buffer[4];
    uint16_t temp_address;
    parent_data_node_t* parent_node_pt = (parent_data_node_t*)temp_buffer;
    child_cred_node_t* child_node_pt = (child_cred_node_t*)temp_buffer;
    
    /* Boundary checks for the buffer we'll use to store start of node data */
    _Static_assert(sizeof(temp_buffer) >= offsetof(parent_data_node_t, nextChildAddress) + sizeof(parent_node_pt->nextChildAddress), "Buffer not long enough to store first bytes");
    _Static_assert(sizeof(temp_buffer) >= offsetof(child_cred_node_t, nextChildAddress) + sizeof(child_node_pt->nextChildAddress), "Buffer not long enough to store first bytes");
        
    // Delete user profile memory
    nodemgmt_format_user_profile(nodemgmt_current_handle.currentUserId, 0, 0, 0);
    
    // Then browse through all the credentials to delete them
    for (uint16_t i = 0; i < 1 + (sizeof(nodemgmt_current_handle.firstDataParentNode)/sizeof(nodemgmt_current_handle.firstDataParentNode[0])); i++)
    {
        while (next_parent_addr != NODE_ADDR_NULL)
        {
            // Read current parent node
            dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_page_from_address(next_parent_addr), BASE_NODE_SIZE * nodemgmt_node_from_address(next_parent_addr), sizeof(temp_buffer), (void*)parent_node_pt);
            nodemgmt_check_user_perm_from_flags_and_lock(parent_node_pt->flags);
            
            // Read his first child
            next_child_addr = parent_node_pt->nextChildAddress;
            
            // Browse through all children
            while (next_child_addr != NODE_ADDR_NULL)
            {
                // Read child node
                dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_page_from_address(next_child_addr), BASE_NODE_SIZE * nodemgmt_node_from_address(next_child_addr), sizeof(temp_buffer), (void*)child_node_pt);
                nodemgmt_check_user_perm_from_flags_and_lock(child_node_pt->flags);
                
                // Store the next child address in temp
                if (i == 0)
                {
                    // First loop is cnode
                    temp_address = child_node_pt->nextChildAddress;
                }
                else
                {
                    // Second loop is dnode
                    child_data_node_t* temp_dnode_ptr = (child_data_node_t*)child_node_pt;
                    temp_address = temp_dnode_ptr->nextDataAddress;
                }
                
                // Delete child data block
                dbflash_write_data_pattern_to_flash(&dbflash_descriptor, nodemgmt_page_from_address(next_child_addr), BASE_NODE_SIZE * nodemgmt_node_from_address(next_child_addr), BASE_NODE_SIZE, 0xFF);
                dbflash_write_data_pattern_to_flash(&dbflash_descriptor, nodemgmt_page_from_address(nodemgmt_get_incremented_address(next_child_addr)), BASE_NODE_SIZE * nodemgmt_node_from_address(nodemgmt_get_incremented_address(next_child_addr)), BASE_NODE_SIZE, 0xFF);
                
                // Set correct next address
                next_child_addr = temp_address;
            }
            
            // Store the next parent address in temp
            temp_address = parent_node_pt->nextParentAddress;
            
            // Delete parent data block
            dbflash_write_data_pattern_to_flash(&dbflash_descriptor, nodemgmt_page_from_address(next_parent_addr), BASE_NODE_SIZE * nodemgmt_node_from_address(next_parent_addr), BASE_NODE_SIZE, 0xFF);
            
            // Set correct next address
            next_parent_addr = temp_address;
        }
        
        // First loop done, remove data nodes
        if (i < (sizeof(nodemgmt_current_handle.firstDataParentNode)/sizeof(nodemgmt_current_handle.firstDataParentNode[0])))
        {
            next_parent_addr = nodemgmt_current_handle.firstDataParentNode[i];
        }
    }
}

/*! \fn     nodemgmt_create_generic_node(generic_node_t* g, node_type_te node_type, uint16_t firstNodeAddress, uint16_t* newFirstNodeAddress, uint16_t* storedAddress)
 *  \brief  Writes a generic node to memory (next free via handle) (in alphabetical order)
 *  \param  g                       The node to write to memory (nextFreeParentNode)
 *  \param  node_type               The node type (see enum)
 *  \param  firstNodeAddress        Address of the first node of its kind
 *  \param  newFirstNodeAddress     If the firstNodeAddress changed, this var will store the new value
 *  \param  storedAddress           Where to store the address at which the node was stored
 *  \return success status
 *  \note   Handles necessary doubly linked list management
 *  \note   Not called for child data node
 */
RET_TYPE nodemgmt_create_generic_node(generic_node_t* g, node_type_te node_type, uint16_t firstNodeAddress, uint16_t* newFirstNodeAddress, uint16_t* storedAddress)
{
    /* Sanity checks */
    _Static_assert(offsetof(parent_cred_node_t, prevParentAddress) == offsetof(parent_data_node_t, prevParentAddress), "Next / Prev fields do not match across parent & child nodes");
    _Static_assert(offsetof(parent_cred_node_t, prevParentAddress) == offsetof(child_cred_node_t, prevChildAddress), "Next / Prev fields do not match across parent & child nodes");
    _Static_assert(offsetof(parent_cred_node_t, nextParentAddress) == offsetof(parent_data_node_t, nextParentAddress), "Next / Prev fields do not match across parent & child nodes");
    _Static_assert(offsetof(parent_cred_node_t, nextParentAddress) == offsetof(child_cred_node_t, nextChildAddress), "Next / Prev fields do not match across parent & child nodes");
        
    // Local vars
    node_common_first_three_fields_t* temp_first_three_fields_pt = (node_common_first_three_fields_t*)&nodemgmt_current_handle.temp_parent_node;
    child_cred_node_t* temp_half_child_node_pt = (child_cred_node_t*)&nodemgmt_current_handle.temp_parent_node;
    parent_cred_node_t* temp_parent_node_pt = (parent_cred_node_t*)&nodemgmt_current_handle.temp_parent_node;
    node_common_first_three_fields_t* g_first_three_fields_pt = (node_common_first_three_fields_t*)g;
    uint16_t freeNodeAddress = NODE_ADDR_NULL;
    uint16_t addr = NODE_ADDR_NULL;
    int16_t res = 0;
    
    // We handle everything except data nodes...
    if (node_type == NODE_TYPE_DATA)
    {
        while(1);
    }
    
    // Set newFirstNodeAddress to firstNodeAddress by default
    *newFirstNodeAddress = firstNodeAddress;
    
    // Select correct free address based on node type
    if (node_type == NODE_TYPE_CHILD)
    {
        freeNodeAddress = nodemgmt_current_handle.nextChildFreeNode;
    }
    else
    {
        freeNodeAddress = nodemgmt_current_handle.nextParentFreeNode;
    }
    
    // Check space in flash
    if (freeNodeAddress == NODE_ADDR_NULL)
    {
        return RETURN_NOK;
    }
    
    // Set flags to 0, added bonus: set valid flags
    g_first_three_fields_pt->flags = 0;
    
    // Set node type    
    g_first_three_fields_pt->flags |= (node_type << NODEMGMT_TYPE_FLAG_BITSHIFT);
    
    // Set correct user id to the node
    g_first_three_fields_pt->flags |= (nodemgmt_current_handle.currentUserId << NODEMGMT_USERID_BITSHIFT);
    
    // Child nodes: set second flags
    if (node_type == NODE_TYPE_CHILD)
    {
        g->data_child.fakeFlags = g->data_child.flags | (NODEMGMT_VBIT_INVALID << NODEMGMT_CORRECT_FLAGS_BIT_BITSHIFT);
    }

    // clear next/prev address
    g_first_three_fields_pt->prevAddress = NODE_ADDR_NULL;
    g_first_three_fields_pt->nextAddress = NODE_ADDR_NULL;
    
    // if user has no nodes. this node is the first node
    if(firstNodeAddress == NODE_ADDR_NULL)
    {
        // Write node
        if (node_type == NODE_TYPE_CHILD)
        {
            nodemgmt_write_child_node_block_to_flash(freeNodeAddress, (child_node_t*)g, TRUE);
        }
        else
        {
            nodemgmt_write_parent_node_data_block_to_flash(freeNodeAddress, (parent_node_t*)g);
        }
        
        // set new first node address
        *newFirstNodeAddress = freeNodeAddress;
    }
    else
    {        
        // set first node address
        addr = firstNodeAddress;
        while(addr != NODE_ADDR_NULL)
        {
            // read node: use read parent node function as all the fields always are in the first 264B
            nodemgmt_read_parent_node(addr, (parent_node_t*)temp_parent_node_pt, FALSE);
            
            // compare nodes (alphabetically)
            if (node_type == NODE_TYPE_CHILD)
            {
                res = utils_custchar_strncmp(g->cred_child.login, temp_half_child_node_pt->login, sizeof(temp_half_child_node_pt->login)/sizeof(temp_half_child_node_pt->login[0]));
            }
            else
            {
                res = utils_custchar_strncmp(g->cred_parent.service, temp_parent_node_pt->service, sizeof(temp_parent_node_pt->service)/sizeof(temp_parent_node_pt->service[0]));
            }
            
            // Check comparison result
            if(res > 0)
            {
                // to add parent node comes after current node in memory.. go to next node
                if(temp_first_three_fields_pt->nextAddress == NODE_ADDR_NULL)
                {
                    // end of linked list. Set to write node prev and next addr's
                    g_first_three_fields_pt->prevAddress = addr; // current memNode Addr
                    
                    // write new node to flash
                    if (node_type == NODE_TYPE_CHILD)
                    {
                        nodemgmt_write_child_node_block_to_flash(freeNodeAddress, (child_node_t*)g, TRUE);
                    }
                    else
                    {
                        nodemgmt_write_parent_node_data_block_to_flash(freeNodeAddress, (parent_node_t*)g);
                    }
                    
                    // set previous last node to point to new node
                    temp_first_three_fields_pt->nextAddress = freeNodeAddress;
                    
                    // even if the previous node is of type child, we only need to write the first 264B!
                    nodemgmt_write_parent_node_data_block_to_flash(addr, (parent_node_t*)temp_parent_node_pt);
                                        
                    // set loop exit case
                    addr = NODE_ADDR_NULL; 
                }
                else
                {
                    // loop and read next node
                    addr = temp_first_three_fields_pt->nextAddress;
                }
            }
            else if(res < 0)
            {
                // to add parent node comes before current node in memory. Previous node is already not a memcmp match .. write node                
                // set node to write next parent to current node in mem, set prev parent to current node in mems prev parent
                g_first_three_fields_pt->nextAddress = addr;
                g_first_three_fields_pt->prevAddress = temp_first_three_fields_pt->prevAddress;
                
                // write new node to flash
                if (node_type == NODE_TYPE_CHILD)
                {
                    nodemgmt_write_child_node_block_to_flash(freeNodeAddress, (child_node_t*)g, TRUE);
                }
                else
                {
                    nodemgmt_write_parent_node_data_block_to_flash(freeNodeAddress, (parent_node_t*)g);
                }
                
                // update current node in mem. set prev parent to address node to write was written to.
                temp_first_three_fields_pt->prevAddress = freeNodeAddress;                
                
                // even if the previous node is of type child, we only need to write the first 264B!
                nodemgmt_write_parent_node_data_block_to_flash(addr, (parent_node_t*)temp_parent_node_pt);
                
                if(g_first_three_fields_pt->prevAddress != NODE_ADDR_NULL)
                {
                    // read p->prev node: use read parent node function as all the fields always are in the first 264B
                    nodemgmt_read_parent_node(g_first_three_fields_pt->prevAddress, (parent_node_t*)temp_parent_node_pt, FALSE);
                
                    // update prev node to point next parent to addr of node to write node
                    temp_first_three_fields_pt->nextAddress = freeNodeAddress;
                    
                    // even if the previous node is of type child, we only need to write the first 264B!
                    nodemgmt_write_parent_node_data_block_to_flash(g_first_three_fields_pt->prevAddress, (parent_node_t*)temp_parent_node_pt);
                }                
                
                if(addr == firstNodeAddress)
                {
                    // new node comes before current address and current address in first node.
                    // new node should be first node
                    *newFirstNodeAddress = freeNodeAddress;
                }
                
                // set exit case
                addr = NODE_ADDR_NULL;
            }
            else
            {
                // services match, return nok. Same parent node
                return RETURN_NOK;
            } // end cmp results
        } // end while
    } // end if first parent
    
    // Rescan node usage
    nodemgmt_scan_node_usage();
    
    // Store the address
    *storedAddress = freeNodeAddress;
    
    return RETURN_OK;
}

/*! \fn     nodemgmt_create_parent_node(parent_node_t* p, service_type_te type, uint16_t* storedAddress)
 *  \brief  Writes a parent node to memory (next free via handle) (in alphabetical order)
 *  \param  p               The parent node to write to memory (nextFreeParentNode)
 *  \param  type            Type of context (data or credential)
 *  \param  storedAddress   Where to store the address at which the node was stored
 *  \param  typeId          In case of data parent, typeId
 *  \return success status
 *  \note   Handles necessary doubly linked list management
 */
RET_TYPE nodemgmt_create_parent_node(parent_node_t* p, service_type_te type, uint16_t* storedAddress, uint16_t typeId)
{
    uint16_t temp_address, first_parent_addr;
    RET_TYPE temprettype;
    
    // Set the first parent address depending on the type
    if (type == SERVICE_CRED_TYPE)
    {
        first_parent_addr = nodemgmt_current_handle.firstParentNode;
    } 
    else
    {
        first_parent_addr = nodemgmt_current_handle.firstDataParentNode[typeId];
    }
    
    // This is particular to parent nodes...
    p->cred_parent.nextChildAddress = NODE_ADDR_NULL;
    
    // Call nodemgmt_create_generic_node to add a node
    if (type == SERVICE_CRED_TYPE)
    {
        temprettype = nodemgmt_create_generic_node((generic_node_t*)p, NODE_TYPE_PARENT, first_parent_addr, &temp_address, storedAddress);
    }
    else
    {
        temprettype = nodemgmt_create_generic_node((generic_node_t*)p, NODE_TYPE_PARENT_DATA, first_parent_addr, &temp_address, storedAddress);
    }
    
    // If the return is ok & we changed the first node address
    if ((temprettype == RETURN_OK) && (first_parent_addr != temp_address))
    {
        if (type == SERVICE_CRED_TYPE)
        {
            nodemgmt_set_cred_start_address(temp_address);
        }
        else
        {
            nodemgmt_set_data_start_address(temp_address, typeId);
        }
    }
    
    return temprettype;
}

/*! \fn     nodemgmt_create_child_node(uint16_t pAddr, child_cred_node_t* c, uint16_t* storedAddress)
 *  \brief  Writes a child node to memory (next free via handle) (in alphabetical order)
 *  \param  pAddr           The parent node address of the child
 *  \param  c               The child node to write to memory (nextFreeChildNode)
 *  \param  storedAddress           Where to store the address at which the node was stored
 *  \return success status
 *  \note   Handles necessary doubly linked list management
 */
RET_TYPE nodemgmt_create_child_node(uint16_t pAddr, child_cred_node_t* c, uint16_t* storedAddress)
{
    uint16_t childFirstAddress, temp_address;
    RET_TYPE temprettype;
    
    // Write date created & used fields
    c->dateCreated = nodemgmt_current_date;
    c->dateLastUsed = nodemgmt_current_date;
    
    // Set "master setting" for keys pressed after login & password entering
    c->keyAfterPassword = 0xFFFF;
    c->keyAfterLogin = 0xFFFF;
    
    // Read parent to get the first child address
    nodemgmt_read_parent_node(pAddr, &nodemgmt_current_handle.temp_parent_node, FALSE);
    childFirstAddress = nodemgmt_current_handle.temp_parent_node.cred_parent.nextChildAddress;
    
    // Call nodemgmt_create_generic_node to add a node
    temprettype = nodemgmt_create_generic_node((generic_node_t*)c, NODE_TYPE_CHILD, childFirstAddress, &temp_address, storedAddress);
    
    // If the return is ok & we changed the first child address
    if ((temprettype == RETURN_OK) && (childFirstAddress != temp_address))
    {
        nodemgmt_read_parent_node(pAddr, &nodemgmt_current_handle.temp_parent_node, FALSE);
        nodemgmt_current_handle.temp_parent_node.cred_parent.nextChildAddress = temp_address;
        nodemgmt_write_parent_node_data_block_to_flash(pAddr, &nodemgmt_current_handle.temp_parent_node);
    }
    
    return temprettype;
}  
