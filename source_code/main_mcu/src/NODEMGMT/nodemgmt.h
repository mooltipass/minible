/*!  \file     nodemgmt.h
*    \brief    Node management library
*    Created:  10/12/2018
*    Author:   Mathieu Stephan
*/


#ifndef NODEMGMT_H_
#define NODEMGMT_H_

#include "defines.h"
#include "dbflash.h"

/* Typedefs */
typedef enum    {NODE_TYPE_PARENT = 0, NODE_TYPE_CHILD = 1, NODE_TYPE_PARENT_DATA = 2, NODE_TYPE_DATA = 3, NODE_TYPE_NULL = 4 /* Not a valid flag combination */} node_type_te;
    
/* Old gen defines */
#define NODEMGMT_OLD_GEN_ASCII_PWD_LENGTH           32

/* Defines */
#define NODE_ADDR_NULL                              0x0000
#define NB_MAX_USERS                                128
#define BASE_NODE_SIZE                              264
#define NODEMGMT_USER_PROFILE_SIZE                  264
#define NODEMGMT_TYPE_FLAG_BITSHIFT                 14
#define NODEMGMT_TYPE_FLAG_BITMASK                  0xC000
#define NODEMGMT_TYPE_FLAG_BITMASK_FINAL            0x0003
#define NODEMGMT_VALID_BIT_BITSHIFT                 13
#define NODEMGMT_VALID_BIT_BITMASK                  0x2000
#define NODEMGMT_VALID_BIT_MASK_FINAL               0x0001
#define NODEMGMT_CORRECT_FLAGS_BIT_BITSHIFT         5
#define NODEMGMT_CORRECT_FLAGS_BIT_BITMASK_FINAL    0x0001
#define NODEMGMT_PREVGEN_BIT_BITMASK                0x0010
#define NODEMGMT_YEAR_SHT                           9
#define NODEMGMT_YEAR_MASK                          0xFE00
#define NODEMGMT_YEAR_MASK_FINAL                    0x007F
#define NODEMGMT_MONTH_SHT                          5
#define NODEMGMT_MONTH_MASK                         0x03E0
#define NODEMGMT_MONTH_MASK_FINAL                   0x000F
#define NODEMGMT_DAY_MASK_FINAL                     0x001F
#define NODEMGMT_ADDR_PAGE_BITSHIFT                 1
#define NODEMGMT_ADDR_PAGE_MASK                     0xfffe
#define NODEMGMT_ADDR_PAGE_MASK_FINAL               0x7fff
#define NODEMGMT_ADDR_NODE_MASK                     0x0001
#define NODEMGMT_USERID_MASK                        0x1FC0
#define NODEMGMT_USERID_BITSHIFT                    6
#define NODEMGMT_USERID_MASK_FINAL                  0x007f
#define NODEMGMT_ADDR_NULL                          0x0000
#define NODEMGMT_VBIT_VALID                         0
#define NODEMGMT_VBIT_INVALID                       1


/* Structs */
// Parent node, see: https://mooltipass.github.io/minible/database_model
typedef struct
{
    uint16_t flags;
    uint16_t prevParentAddress;     // Previous parent node address (Alphabetically)
    uint16_t nextParentAddress;     // Next parent node address (Alphabetically)
    uint16_t nextChildAddress;      // Parent node first child address
    cust_char_t service[126];       // Unicode BMP text describing service, used for sorting and searching
    uint8_t reserved[4];            // Reserved for future use
} parent_cred_node_t;

// Parent node, see: https://mooltipass.github.io/minible/database_model
typedef struct
{
    uint16_t flags;
    uint16_t prevParentAddress;     // Previous parent node address (Alphabetically)
    uint16_t nextParentAddress;     // Next parent node address (Alphabetically)
    uint16_t nextChildAddress;      // Parent node first child address
    cust_char_t service[126];       // Unicode BMP text describing service, used for sorting and searching
    uint8_t reserved;               // Reserved for future use
    uint8_t startDataCtr[3];        // Encryption counter
} parent_data_node_t;

// Child data node, see: https://mooltipass.github.io/minible/database_model
typedef struct
{
    uint16_t flags;
    uint16_t nextDataAddress;       // Next data node in sequence
    uint16_t data_length;           // Encrypted data length
    uint8_t data[256];              // Encrypted data (256B)
    uint8_t reserved[2];            // Reserved for future use
    uint16_t fakeFlags;             // Same as flags but with bit 5 set to 1
    uint8_t data2[256];             // Encrypted data (256B)
    uint8_t reserved2[6];           // Reserved for future use
} child_data_node_t;

// Child credential node, see: https://mooltipass.github.io/minible/database_model
typedef struct
{
    uint16_t flags;
    uint16_t prevChildAddress;      // Previous child node address (Alphabetically)
    uint16_t nextChildAddress;      // Next child node address (Alphabetically)
    uint16_t mirroredChildAddress;  // If different than 0, pointer to the mirrored node
    uint16_t dateCreated;           // The date the child node was added to the DB
                                    /* Date Encoding:
                                    * 15 dn 9 -> Year (2010 + value)
                                    * 8 dn 5 -> Month
                                    * 4 dn 0 -> Day
                                    */
    uint16_t dateLastUsed;          // The date the child node was last used
                                    /* Date Encoding:
                                    * 15 dn 9 -> Year (2010 + value)
                                    * 8 dn 5 -> Month
                                    * 4 dn 0 -> Day
                                    */
    cust_char_t login[64];          // Unicode BMP login
    cust_char_t description[24];    // Unicode BMP description
    cust_char_t thirdField[36];     // Unicode BMP third field   
    uint16_t keyAfterLogin;         // Typed key after login
    uint16_t keyAfterPassword;      // Typed key after password
    uint16_t fakeFlags;             // Same as flags but with bit 5 set to 1
    uint8_t reserved;               // Reserved
    uint8_t ctr[3];                 // Encryption counter
    uint8_t password[128];          // Encrypted password
    uint8_t TBD[130];               // TBD
} child_cred_node_t;

// Struct that includes the common first 3 fields for parent nodes & child cred node
typedef struct
{
    uint16_t flags;
    uint16_t prevAddress;
    uint16_t nextAddress;  
} node_common_first_three_fields_t;

// Parent node genetic typedef
typedef struct
{
    union
    {
        parent_cred_node_t cred_parent;
        parent_data_node_t data_parent;
        uint8_t node_as_bytes[BASE_NODE_SIZE];
    };    
} parent_node_t;

// Child node genetic typedef
typedef struct
{
    union
    {
        child_cred_node_t cred_child;
        child_data_node_t data_child;
        uint8_t node_as_bytes[2*BASE_NODE_SIZE];
    };
} child_node_t;

// Generic node typedef
typedef struct
{
    union
    {
        parent_cred_node_t cred_parent;
        parent_data_node_t data_parent;
        child_cred_node_t cred_child;
        child_data_node_t data_child;
    };
} generic_node_t;

// Favorite address
typedef struct
{
    uint16_t parent_addr;
    uint16_t child_addr;
} favorite_addr_t;

// List of favorite for a given category
typedef struct
{
    favorite_addr_t favorite[10];
} favorites_for_category_t;

// User profile main data
typedef struct
{
    uint16_t cred_start_address;
    uint16_t data_start_address[16];
    uint8_t reserved[19];
    uint8_t current_ctr[3];
    uint32_t cred_change_number;
    uint32_t data_change_number;    
} nodemgmt_profile_main_data_t;

// User profile
typedef struct
{
    nodemgmt_profile_main_data_t main_data;
    favorites_for_category_t category_favorites[5];
} nodemgmt_userprofile_t;

// Node management handle
typedef struct
{
    BOOL datadbChanged;                     // Boolean to indicate if the user data DB has changed since user login
    BOOL dbChanged;                         // Boolean to indicate if the user DB has changed since user login
    uint16_t currentUserId;                 // The users ID
    uint16_t pageUserProfile;               // The page of the user profile
    uint16_t offsetUserProfile;             // The offset of the user profile
    uint16_t pageUserFavStrings;            // The page of the user favorite strings
    uint16_t offsetUserFavStrings;          // The offset of the user favorite strings
    uint16_t firstParentNode;               // The address of the users first parent node (read from flash. eg cache)
    uint16_t firstDataParentNode[16];       // The addresses of the users first data parent nodes (read from flash. eg cache)
    uint16_t nextParentFreeNode;            // The address of the next free parent node
    uint16_t nextChildFreeNode;             // The address of the next free child node
    parent_node_t temp_parent_node;         // Temp parent node to be used when needed
} nodemgmtHandle_t;

/* Inlines */

/*! \fn     nodemgmt_user_id_to_flags(uint16_t *flags, uint8_t uid)
*   \brief  Sets the user id to flags  
*   \param  flags           The flags field of a node
*   \return uid             The user id to set in flags (0 up to NODE_MAX_UID)
*/
static inline void nodemgmt_user_id_to_flags(uint16_t *flags, uint8_t uid)
{
    *flags = (*flags & (~NODEMGMT_USERID_MASK)) | ((uint16_t)uid << NODEMGMT_USERID_BITSHIFT);
}

/*! \fn     nodemgmt_page_from_address(uint16_t addr)
*   \brief  Extracts a page number from a constructed address
*   \param  addr    The constructed address used for extraction
*   \return A page number in flash memory (uin16_t)
*   \note   See design notes for address format
*   \note   Max Page Number varies per flash size
 */
static inline uint16_t nodemgmt_page_from_address(uint16_t addr)
{
    return (addr >> NODEMGMT_ADDR_PAGE_BITSHIFT) & NODEMGMT_ADDR_PAGE_MASK_FINAL;
}

/*! \fn     nodemgmt_node_from_address(uint16_t addr)
*   \brief  Extracts a node number from a constructed address
*   \param  addr   The constructed address used for extraction
*   \return A node number of a node in a page in flash memory
*   \note   See design notes for address format
*   \note   Max Node Number varies per flash size
 */
static inline uint16_t nodemgmt_node_from_address(uint16_t addr)
{
    _Static_assert(NODEMGMT_ADDR_PAGE_BITSHIFT == 1, "Addressing scheme doesn't fit 1 or 2 base node size per page");
    
    #if (BYTES_PER_PAGE == BASE_NODE_SIZE)
        /* One node per page */
        return 0;
    #else
        return (addr & NODEMGMT_ADDR_NODE_MASK);
    #endif
}

/* Prototypes */
uint16_t nodemgmt_find_free_nodes(uint16_t nbParentNodes, uint16_t* parentNodeArray, uint16_t nbChildtNodes, uint16_t* childNodeArray, uint16_t startPage, uint16_t startNode);
RET_TYPE nodemgmt_create_generic_node(generic_node_t* g, node_type_te node_type, uint16_t firstNodeAddress, uint16_t* newFirstNodeAddress, uint16_t* storedAddress);
RET_TYPE nodemgmt_create_parent_node(parent_node_t* p, service_type_te type, uint16_t* storedAddress, uint16_t typeId);
void nodemgmt_read_favorite(uint16_t categoryId, uint16_t favId, uint16_t* parentAddress, uint16_t* childAddress);
void nodemgmt_set_favorite(uint16_t categoryId, uint16_t favId, uint16_t parentAddress, uint16_t childAddress);
void nodemgmt_get_user_fav_names_starting_offset(uint16_t uid, uint16_t *page, uint16_t *pageOffset);
void nodemgmt_get_user_profile_starting_offset(uint16_t uid, uint16_t *page, uint16_t *pageOffset);
RET_TYPE nodemgmt_create_child_node(uint16_t pAddr, child_cred_node_t* c, uint16_t* storedAddress);
void nodemgmt_read_parent_node_data_block_from_flash(uint16_t address, parent_node_t* parent_node);
void nodemgmt_write_parent_node_data_block_to_flash(uint16_t address, parent_node_t* parent_node);
void nodemgmt_read_child_node_data_block_from_flash(uint16_t address, child_node_t* child_node);
void nodemgmt_read_cred_child_node_except_pwd(uint16_t address, child_cred_node_t* child_node);
void nodemgmt_read_parent_node(uint16_t address, parent_node_t* parent_node, BOOL data_clean);
void nodemgmt_write_child_node_block_to_flash(uint16_t address, child_node_t* child_node);
RET_TYPE nodemgmt_check_user_permission(uint16_t node_addr, node_type_te* node_type);
void nodemgmt_read_cred_child_node(uint16_t address, child_cred_node_t* child_node);
void nodemgmt_set_data_start_address(uint16_t dataParentAddress, uint16_t typeId);
uint16_t nodemgmt_construct_date(uint16_t year, uint16_t month, uint16_t day);
void nodemgmt_check_user_perm_from_flags_and_lock(uint16_t flags);
uint16_t nodemgmt_get_start_addresses(uint16_t* addresses_array);
uint16_t nodemgmt_get_starting_data_parent_addr(uint16_t typeId);
RET_TYPE nodemgmt_check_user_perm_from_flags(uint16_t flags);
void nodemgmt_set_start_addresses(uint16_t* addresses_array);
void nodemgmt_set_cred_start_address(uint16_t parentAddress);
void nodemgmt_set_data_change_number(uint32_t changeNumber);
void nodemgmt_set_cred_change_number(uint32_t changeNumber);
uint16_t nodemgmt_get_favorites(uint16_t* addresses_array);
uint16_t nodemgmt_get_incremented_address(uint16_t addr);
void nodemgmt_user_db_changed_actions(BOOL dataChanged);
void nodemgmt_delete_current_user_from_flash(void);
uint16_t nodemgmt_get_starting_parent_addr(void);
void nodemgmt_format_user_profile(uint16_t uid);
uint32_t nodemgmt_get_cred_change_number(void);
uint32_t nodemgmt_get_data_change_number(void);
void nodemgmt_init_context(uint16_t userIdNum);
void nodemgmt_set_current_date(uint16_t date);
void nodemgmt_read_profile_ctr(void* buf);
void nodemgmt_set_profile_ctr(void* buf);
void nodemgmt_scan_node_usage(void);

#endif /* NODEMGMT_H_ */
