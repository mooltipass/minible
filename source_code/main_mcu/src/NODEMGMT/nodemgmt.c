/*!  \file     nodemgmt.c
*    \brief    Node management library
*    Created:  14/12/2018
*    Author:   Mathieu Stephan
*/
#include <assert.h>
#include <string.h>
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
    _Static_assert(NODEMGMT_ADDR_PAGE_BITSHIFT == 1, "Addressing scheme doesn't fit 1 or 2 base node size per page");
    
    #if (BYTES_PER_PAGE == BASE_NODE_SIZE)
        /* One node per page */
        return 0;
    #else
        return (addr & NODEMGMT_ADDR_NODE_MASK);
    #endif
}

/*! \fn     getIncrementedAddress(uint16_t addr)
*   \brief  Get next address for a given address
*   \param  addr   The base address
*   \return The next address for our addressing scheme
 */
uint16_t getIncrementedAddress(uint16_t addr)
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

/*! \fn     checkUserPermissionFromFlags(uint16_t flags)
*   \brief  Check that the user has the right to read/write a node
*   \param  flags       Flags contents
*   \return OK / NOK
*/
RET_TYPE checkUserPermissionFromFlags(uint16_t flags)
{
    // Either the node belongs to us or it is invalid, check that the address is after sector 1 (upper check done at the flashread/write level)
    if (((nodemgmt_current_handle.currentUserId == userIdFromFlags(flags)) && (correctFlagsBitFromFlags(flags) == NODEMGMT_VBIT_VALID)) || (validBitFromFlags(flags) == NODEMGMT_VBIT_INVALID))
    {
        return RETURN_OK;
    }
    else
    {
        return RETURN_NOK;
    }
}

/*! \fn     checkUserPermissionFromFlagsAndLock(uint16_t flags)
*   \brief  Check that the user has the right to read/write a node, lock if not
*   \param  flags       Flags contents
*/
void checkUserPermissionFromFlagsAndLock(uint16_t flags)
{
    if (checkUserPermissionFromFlags(flags) == RETURN_NOK)
    {
        while(1);
    }
}

/*! \fn     checkUserPermission(uint16_t node_addr)
*   \brief  Check that the user has the right to read/write a node
*   \param  node_addr   Node address
*   \return OK / NOK
*   \note   Scanning a 8Mb Flash memory contents with that function was timed at 56ms in Debug mode.
*/
RET_TYPE checkUserPermission(uint16_t node_addr)
{
    // Future node flags
    uint16_t temp_flags;
    // Node Page
    uint16_t page_addr = pageNumberFromAddress(node_addr);
    // Node byte address
    uint16_t byte_addr = BASE_NODE_SIZE * nodeNumberFromAddress(node_addr);
    
    // Fetch the flags
    dbflash_read_data_from_flash(&dbflash_descriptor, page_addr, byte_addr, sizeof(temp_flags), (void*)&temp_flags);
    
    // Check permission and memory boundaries (high boundary done on the lower level)
    if ((page_addr >= PAGE_PER_SECTOR) && (checkUserPermissionFromFlags(temp_flags) == RETURN_OK))
    {
        return RETURN_OK;        
    } 
    else
    {
        return RETURN_NOK;
    }
}

/*! \fn     writeParentNodeDataBlockToFlash(uint16_t address, parent_node_t* parent_node)
*   \brief  Write a parent node data block to flash
*   \param  address     Where to write
*   \param  parent_node Pointer to the node
*/
void writeParentNodeDataBlockToFlash(uint16_t address, parent_node_t* parent_node)
{
    _Static_assert(BASE_NODE_SIZE == sizeof(*parent_node), "Parent node isn't the size of base node size");
    dbflash_write_data_to_flash(&dbflash_descriptor, pageNumberFromAddress(address), BASE_NODE_SIZE * nodeNumberFromAddress(address), BASE_NODE_SIZE, (void*)parent_node->node_as_bytes);
}

/*! \fn     writeChildNodeDataBlockToFlash(uint16_t address, child_node_t* child_node)
*   \brief  Write a child node data block to flash
*   \param  address     Where to write
*   \param  parent_node Pointer to the node
*/
void writeChildNodeDataBlockToFlash(uint16_t address, child_node_t* child_node)
{
    _Static_assert(2*BASE_NODE_SIZE == sizeof(*child_node), "Child node isn't twice the size of base node size");
    dbflash_write_data_to_flash(&dbflash_descriptor, pageNumberFromAddress(address), BASE_NODE_SIZE * nodeNumberFromAddress(address), BASE_NODE_SIZE, (void*)child_node->node_as_bytes);
    dbflash_write_data_to_flash(&dbflash_descriptor, pageNumberFromAddress(getIncrementedAddress(address)), BASE_NODE_SIZE * nodeNumberFromAddress(getIncrementedAddress(address)), BASE_NODE_SIZE, (void*)(&child_node->node_as_bytes[BASE_NODE_SIZE]));
}

/*! \fn     readParentNodeDataBlockFromFlash(uint16_t address, parent_node_t* parent_node)
*   \brief  Read a parent node data block to flash
*   \param  address     Where to read
*   \param  parent_node Pointer to the node
*/
void readParentNodeDataBlockFromFlash(uint16_t address, parent_node_t* parent_node)
{
    dbflash_read_data_from_flash(&dbflash_descriptor, pageNumberFromAddress(address), BASE_NODE_SIZE * nodeNumberFromAddress(address), sizeof(parent_node->node_as_bytes), (void*)parent_node->node_as_bytes);
}

/*! \fn     readChildNodeDataBlockFromFlash(uint16_t address, child_node_t* child_node)
*   \brief  Read a parent node data block to flash
*   \param  address     Where to read
*   \param  parent_node Pointer to the node
*/
void readChildNodeDataBlockFromFlash(uint16_t address, child_node_t* child_node)
{
    dbflash_read_data_from_flash(&dbflash_descriptor, pageNumberFromAddress(address), BASE_NODE_SIZE * nodeNumberFromAddress(address), sizeof(child_node->node_as_bytes), (void*)child_node->node_as_bytes);
}

/*! \fn     userProfileStartingOffset(uint8_t uid, uint16_t *page, uint16_t *pageOffset)
    \brief  Obtains page and page offset for a given user id
    \param  uid             The id of the user to perform that profile page and offset calculation (0 up to NODE_MAX_UID)
    \param  page            The page containing the user profile
    \param  pageOffset      The offset of the page that indicates the start of the user profile
 */
void userProfileStartingOffset(uint16_t uid, uint16_t *page, uint16_t *pageOffset)
{
    if(uid >= NB_MAX_USERS)
    {
        /* No debug... no reason it should get stuck here as the data format doesn't allow such values */
        while(1);
    }

    /* Check for bad surprises */    
    _Static_assert(NODEMGMT_USER_PROFILE_SIZE == sizeof(nodemgmt_userprofile_t), "User profile isn't the right size");
    
    #if BYTES_PER_PAGE == NODEMGMT_USER_PROFILE_SIZE
        /* One node per page: just return the UID */
        *page = uid;
        *pageOffset = 0;
    #elif BYTES_PER_PAGE == 2*NODEMGMT_USER_PROFILE_SIZE
        /* One node per page: bitmask and division by 2 */
        *page = uid >> NODEMGMT_ADDR_PAGE_BITSHIFT;
        *pageOffset = (uid & NODEMGMT_ADDR_PAGE_BITSHIFT) * NODEMGMT_USER_PROFILE_SIZE;
    #else
        #error "User profile isn't a multiple of page size"
    #endif
}

/*! \fn     formatUserProfileMemory(uint8_t uid)
 *  \brief  Formats the user profile flash memory of user uid.
 *  \param  uid    The id of the user to format profile memory
 */
void formatUserProfileMemory(uint16_t uid)
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
    userProfileStartingOffset(uid, &temp_page, &temp_offset);
    dbflash_write_data_pattern_to_flash(&dbflash_descriptor, temp_page, temp_offset, sizeof(nodemgmtHandle_t), 0x00);
}

/*! \fn     getCurrentUserID(void)
*   \brief  Get the current user ID
*   \return The user ID
*/
uint16_t getCurrentUserID(void)
{
    return nodemgmt_current_handle.currentUserId;
}

/*! \fn     getFreeNodeAddress(void)
*   \brief  Get next free node address
*   \return The address
*/
uint16_t getFreeNodeAddress(void)
{
    return nodemgmt_current_handle.nextFreeNode;
}

/*! \fn     getStartingParentAddress(void)
 *  \brief  Gets the users starting parent node from the user profile memory portion of flash
 *  \return The address
 */
uint16_t getStartingParentAddress(void)
{
    nodemgmt_userprofile_t* const dirty_address_finding_trick = (nodemgmt_userprofile_t*)0;
    uint16_t temp_address;
    
    // Each user profile is within a page, data starting parent node is at the end of the favorites
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)&(dirty_address_finding_trick->main_data.cred_start_address), sizeof(temp_address), &temp_address);    
    
    return temp_address;
}

/*! \fn     getStartingDataParentAddress(void)
 *  \brief  Gets the users starting data parent node from the user profile memory portion of flash
 *  \return The address
 */
uint16_t getStartingDataParentAddress(void)
{
    nodemgmt_userprofile_t* const dirty_address_finding_trick = (nodemgmt_userprofile_t*)0;
    uint16_t temp_address;
    
    // Each user profile is within a page, data starting parent node is at the end of the favorites
    dbflash_read_data_from_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)&(dirty_address_finding_trick->main_data.data_start_address), sizeof(temp_address), &temp_address);    
    
    return temp_address;
}

/*! \fn     setStartingParentAddress(uint16_t parentAddress)
 *  \brief  Sets the users starting parent node both in the handle and user profile memory portion of flash
 *  \param  parentAddress   The constructed address of the users starting parent node
 */
void setStartingParentAddress(uint16_t parentAddress)
{
    nodemgmt_userprofile_t* const dirty_address_finding_trick = (nodemgmt_userprofile_t*)0;
    
    // update handle
    nodemgmt_current_handle.firstParentNode = parentAddress;
    
    // Write parent address in the user profile page
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)&(dirty_address_finding_trick->main_data.cred_start_address), sizeof(parentAddress), &parentAddress);
}

/*! \fn     setDataStartingParentAddress(uint16_t dataParentAddress)
 *  \brief  Sets the users starting data parent node both in the handle and user profile memory portion of flash
 *  \param  dataParentAddress   The constructed address of the users starting parent node
 */
void setDataStartingParentAddress(uint16_t dataParentAddress)
{
    nodemgmt_userprofile_t* const dirty_address_finding_trick = (nodemgmt_userprofile_t*)0;
    
    // update handle
    nodemgmt_current_handle.firstDataParentNode = dataParentAddress;
    
    // Write data parent address in the user profile page
    dbflash_write_data_to_flash(&dbflash_descriptor, nodemgmt_current_handle.pageUserProfile, nodemgmt_current_handle.offsetUserProfile + (size_t)&(dirty_address_finding_trick->main_data.data_start_address), sizeof(dataParentAddress), &dataParentAddress);
}

/*! \fn     findFreeNodes(uint8_t nbNodes, uint16_t* array)
*   \brief  Find Free Nodes inside our external memory
*   \param  nbNodes     Number of nodes we want to find
*   \param  nodeArray   An array where to store the addresses
*   \param  startPage   Page where to start the scanning
*   \param  startNode   Scan start node address inside the start page
*   \return the number of nodes found
*/
uint8_t findFreeNodes(uint8_t nbNodes, uint16_t* nodeArray, uint16_t startPage, uint8_t startNode)
{
    uint8_t nbNodesFound = 0;
    uint16_t nodeFlags;
    uint16_t pageItr;
    uint8_t nodeItr;
    
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
                if (nbNodesFound < nbNodes)
                {
                    nodeArray[nbNodesFound++] = constructAddress(pageItr, nodeItr);
                }
                else
                {
                    return nbNodesFound;
                }
            }
        }
        startNode = 0;
    }    
    
    return nbNodesFound;
}

/*! \fn     initNodeManagementHandle(uint16_t userIdNum)
 *  \brief  Initializes the Node Management Handle, scans memory for the next free node
 *  \param  userIdNum   The user id to initialize the handle for
 */
void initNodeManagementHandle(uint16_t userIdNum)
{
    if(userIdNum >= NB_MAX_USERS)
    {
        /* No debug... no reason it should get stuck here as the data format doesn't allow such values */
        while(1);
    }
            
    // fill current user id, first parent node address, user profile page & offset 
    userProfileStartingOffset(userIdNum, &nodemgmt_current_handle.pageUserProfile, &nodemgmt_current_handle.offsetUserProfile);
    nodemgmt_current_handle.firstDataParentNode = getStartingDataParentAddress();
    nodemgmt_current_handle.firstParentNode = getStartingParentAddress();
    nodemgmt_current_handle.currentUserId = userIdNum;
    nodemgmt_current_handle.datadbChanged = FALSE;
    nodemgmt_current_handle.dbChanged = FALSE;
    
    // scan for next free parent and child nodes from the start of the memory
    if (findFreeNodes(1, &nodemgmt_current_handle.nextFreeNode, NODE_ADDR_NULL, NODE_ADDR_NULL) == 0)
    {
        nodemgmt_current_handle.nextFreeNode = NODE_ADDR_NULL;
    }
    
    // To think about: the old service LUT from the mini isn't needed as we support unicode now
}

/*! \fn     scanNodeUsage(void)
*   \brief  Scan memory to find empty slots
*/
void scanNodeUsage(void)
{
    // Find one free node. If we don't find it, set the next to the null addr, we start looking from the just taken node
    if (findFreeNodes(1, &nodemgmt_current_handle.nextFreeNode, pageNumberFromAddress(nodemgmt_current_handle.nextFreeNode), nodeNumberFromAddress(nodemgmt_current_handle.nextFreeNode)) == 0)
    {
        nodemgmt_current_handle.nextFreeNode = NODE_ADDR_NULL;
    }
}

/*! \fn     deleteCurrentUserFromFlash(void)
*   \brief  Delete user data from flash
*/
void deleteCurrentUserFromFlash(void)
{
    uint16_t next_parent_addr = nodemgmt_current_handle.firstParentNode;
    uint16_t next_child_addr;
    uint16_t temp_buffer[4];
    uint16_t temp_address;
    parent_data_node_t* parent_node_pt = (parent_data_node_t*)temp_buffer;
    child_cred_node_t* child_node_pt = (child_cred_node_t*)temp_buffer;
    
    /* Boundary checks for the buffer we'll use to store start of node data */
    parent_data_node_t* const parent_node_san_checks = 0;
    _Static_assert(sizeof(temp_buffer) >= ((size_t)&(parent_node_san_checks->nextChildAddress)) + sizeof(parent_node_san_checks->nextChildAddress), "Buffer not long enough to store first bytes");
    child_cred_node_t* const child_node_san_checks = 0;
    _Static_assert(sizeof(temp_buffer) >= ((size_t)&(child_node_san_checks->nextChildAddress)) + sizeof(child_node_san_checks->nextChildAddress), "Buffer not long enough to store first bytes");
        
    // Delete user profile memory
    formatUserProfileMemory(nodemgmt_current_handle.currentUserId);
    
    // Then browse through all the credentials to delete them
    for (uint16_t i = 0; i < 2; i++)
    {
        while (next_parent_addr != NODE_ADDR_NULL)
        {
            // Read current parent node
            dbflash_read_data_from_flash(&dbflash_descriptor, pageNumberFromAddress(next_parent_addr), BASE_NODE_SIZE * nodeNumberFromAddress(next_parent_addr), sizeof(temp_buffer), (void*)parent_node_pt);
            checkUserPermissionFromFlagsAndLock(parent_node_pt->flags);
            
            // Read his first child
            next_child_addr = parent_node_pt->nextChildAddress;
            
            // Browse through all children
            while (next_child_addr != NODE_ADDR_NULL)
            {
                // Read child node
                dbflash_read_data_from_flash(&dbflash_descriptor, pageNumberFromAddress(next_child_addr), BASE_NODE_SIZE * nodeNumberFromAddress(next_child_addr), sizeof(temp_buffer), (void*)child_node_pt);
                checkUserPermissionFromFlagsAndLock(child_node_pt->flags);
                
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
                dbflash_write_data_pattern_to_flash(&dbflash_descriptor, pageNumberFromAddress(next_child_addr), BASE_NODE_SIZE * nodeNumberFromAddress(next_child_addr), BASE_NODE_SIZE, 0xFF);
                dbflash_write_data_pattern_to_flash(&dbflash_descriptor, pageNumberFromAddress(getIncrementedAddress(next_child_addr)), BASE_NODE_SIZE * nodeNumberFromAddress(getIncrementedAddress(next_child_addr)), BASE_NODE_SIZE, 0xFF);
                
                // Set correct next address
                next_child_addr = temp_address;
            }
            
            // Store the next parent address in temp
            temp_address = parent_node_pt->nextParentAddress;
            
            // Delete parent data block
            dbflash_write_data_pattern_to_flash(&dbflash_descriptor, pageNumberFromAddress(next_parent_addr), BASE_NODE_SIZE * nodeNumberFromAddress(next_parent_addr), BASE_NODE_SIZE, 0xFF);
            
            // Set correct next address
            next_parent_addr = temp_address;
        }
        // First loop done, remove data nodes
        next_parent_addr = nodemgmt_current_handle.firstDataParentNode;
    }
}

/*! \fn     createParentNode(parent_node_t* p, service_type_te type)
 *  \brief  Writes a parent node to memory (next free via handle) (in alphabetical order)
 *  \param  p               The parent node to write to memory (nextFreeParentNode)
 *  \param  type            Type of context (data or credential)
 *  \return success status
 *  \note   Handles necessary doubly linked list management
 */
RET_TYPE createParentNode(parent_node_t* p, service_type_te type)
{
    uint16_t temp_address, first_parent_addr;
    RET_TYPE temprettype;
    
    // Set flags to 0, added bonus: set valid flag
    p->cred_parent.flags = 0;
    
    // Set the first parent address depending on the type
    if (type == SERVICE_CRED_TYPE)
    {
        first_parent_addr = nodemgmt_current_handle.firstParentNode;
    } 
    else
    {
        first_parent_addr = nodemgmt_current_handle.firstDataParentNode;
    }
    
    // This is particular to parent nodes...
    p->cred_parent.nextChildAddress = NODE_ADDR_NULL;
    
    if (type == SERVICE_CRED_TYPE)
    {
        p->cred_parent.flags |= (NODE_TYPE_PARENT << NODEMGMT_TYPE_FLAG_BITSHIFT);
    }
    else
    {
        p->cred_parent.flags |= (NODE_TYPE_PARENT_DATA << NODEMGMT_TYPE_FLAG_BITSHIFT);
    }
    
    // Call createGenericNode to add a node
    //temprettype = createGenericNode((gNode*)p, first_parent_addr, &temp_address, PNODE_COMPARISON_FIELD_OFFSET, NODE_PARENT_SIZE_OF_SERVICE);
    
    // If the return is ok & we changed the first node address
    if ((temprettype == RETURN_OK) && (first_parent_addr != temp_address))
    {
        if (type == SERVICE_CRED_TYPE)
        {
            setStartingParentAddress(temp_address);
        }
        else
        {
            setDataStartingParentAddress(temp_address);
        }
    }
    
    return temprettype;
}