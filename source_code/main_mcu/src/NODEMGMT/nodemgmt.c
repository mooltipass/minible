/*!  \file     nodemgmt.c
*    \brief    Node management library
*    Created:  14/12/2018
*    Author:   Mathieu Stephan
*/
#include "comms_hid_msgs_debug.h"
#include "nodemgmt.h"
#include "dbflash.h"
#include "main.h"

// Current node management handle
nodemgmtHandle_t nodemgmt_current_handle;
// Current date
uint16_t nodemgmt_current_date;


/*! \fn     setCurrentDate(uint16_t date)
*   \brief  Sets current date
*   \param  date    The date (see format in documentation)
*/
void setCurrentDate(uint16_t date)
{
    nodemgmt_current_date = date;
}

/*! \fn     pageNumberFromAddress(uint16_t addr)
*   \brief  Extracts a page number from a constructed address
*   \param  addr    The constructed address used for extraction
*   \return A page number in flash memory (uin16_t)
*   \note   See design notes for address format
*   \note   Max Page Number varies per flash size
 */
static inline uint16_t pageNumberFromAddress(uint16_t addr)
{
    return (addr >> NODEMGMT_ADDR_PAGE_BITSHIFT) & NODEMGMT_ADDR_PAGE_MASK_FINAL;
}

/*! \fn     nodeNumberFromAddress(uint16_t addr)
*   \brief  Extracts a node number from a constructed address
*   \param  addr   The constructed address used for extraction
*   \return A node number of a node in a page in flash memory
*   \note   See design notes for address format
*   \note   Max Node Number varies per flash size
 */
static inline uint16_t nodeNumberFromAddress(uint16_t addr)
{
    return (addr & NODEMGMT_ADDR_NODE_MASK);
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

/*! \fn     constructDate(uint16_t year, uint16_t month, uint16_t day)
*   \brief  Packs a uint16_t type with a date code in format YYYYYYYMMMMDDDDD. Year Offset from 2010
*   \param  year            The year to pack into the uint16_t
*   \param  month           The month to pack into the uint16_t
*   \param  day             The day to pack into the uint16_t
*   \return date            The constructed / encoded date in uint16_t
*/
static inline uint16_t constructDate(uint16_t year, uint16_t month, uint16_t day)
{
    return (day | ((month << NODEMGMT_MONTH_SHT) & NODEMGMT_MONTH_MASK) | ((year << NODEMGMT_YEAR_SHT) & NODEMGMT_YEAR_MASK));
}

/*! \fn     extractDate(uint16_t date, uint8_t *year, uint8_t *month, uint8_t *day)
*   \brief  Unpacks a unint16_t to extract the year, month, and day information in format of YYYYYYYMMMMDDDDD. Year Offset from 2010
*   \param  year            The unpacked year
*   \param  month           The unpacked month
*   \param  day             The unpacked day
*   \return success status
*/
RET_TYPE extractDate(uint16_t date, uint8_t *year, uint8_t *month, uint8_t *day)
{
    *year = ((date >> NODEMGMT_YEAR_SHT) & NODEMGMT_YEAR_MASK_FINAL);
    *month = ((date >> NODEMGMT_MONTH_SHT) & NODEMGMT_MONTH_MASK_FINAL);
    *day = (date & NODEMGMT_DAY_MASK_FINAL);
    return RETURN_OK;
}

/*! \fn     checkUserPermission(uint16_t node_addr)
*   \brief  Check that the user has the right to read/write a node
*   \param  node_addr   Node address
*   \return OK / NOK
*/
RET_TYPE checkUserPermission(uint16_t node_addr)
{
    // Future node flags
    uint16_t temp_flags;
    // Node Page
    uint16_t page_addr = pageNumberFromAddress(node_addr);
    // Node byte address
    uint16_t byte_addr = BASE_NODE_SIZE * (uint16_t)nodeNumberFromAddress(node_addr);
    
    // Fetch the flags
    dbflash_read_data_from_flash(&dbflash_descriptor, page_addr, byte_addr, sizeof(temp_flags), (void*)&temp_flags);
    
    // Either the node belongs to us or it is invalid, check that the address is after sector 1 (upper check done at the flashread/write level)
    if ((((nodemgmt_current_handle.currentUserId == userIdFromFlags(temp_flags)) && (correctFlagsBitFromFlags(temp_flags) == NODEMGMT_VBIT_VALID)) || (validBitFromFlags(temp_flags) == NODEMGMT_VBIT_INVALID)) && (page_addr >= PAGE_PER_SECTOR))
    {
        return RETURN_OK;
    }
    else
    {
        return RETURN_NOK;
    }
}