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
/*!  \file     nodemgmt.h
*    \brief    Node management library
*    Created:  10/12/2018
*    Author:   Mathieu Stephan
*/


#ifndef NODEMGMT_H_
#define NODEMGMT_H_

#include "nodemgmt_defines.h"
#include "platform_defines.h"
#include "defines.h"
#include "dbflash.h"

/* Typedefs */
typedef enum    {NODE_TYPE_PARENT = 0, NODE_TYPE_CHILD = 1, NODE_TYPE_PARENT_DATA = 2, NODE_TYPE_DATA = 3, NODE_TYPE_NULL = 4 /* Not a valid flag combination */} node_type_te;
    
/* Old gen defines */
#define NODEMGMT_OLD_GEN_ASCII_PWD_LENGTH           32
#define NODEMGMG_OLD_GEN_DATA_BLOCK_LENGTH          128

/* Defines */
#define NB_MAX_USERS                                MAX_NUMBER_OF_USERS
#define NODE_ADDR_NULL                              0x0000
#define BASE_NODE_SIZE                              264
#define NODEMGMT_NB_MAX_CATEGORIES                  5
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
#define NODEMGMT_CAT_MASK_FINAL                     0x000F
#define NODEMGMT_CAT_MASK                           0x000F
#define NODEMGMT_CAT_BITSHIFT                       0

/* User security settings flags */
#define USER_SEC_FLG_LOGIN_CONF             0x01
#define USER_SEC_FLG_PIN_FOR_MMM            0x02
#define USER_SEC_FLG_CRED_SAVE_PROMPT_MMM   0x04
#define USER_SEC_FLG_ADVANCED_MENU          0x08
#define USER_SEC_FLG_BLE_ENABLED            0x10
#define USER_SEC_FLG_KNOCK_DET_DISABLED     0x20
#define USER_SEC_FLG_ALL_FLAGS_MASK         0x3F

/*  The user limit is set to something smaller than what our DB can actually store.              */
/*  We use the space freed by these non-used user profile to store bluetooth bonding information */
#define NODEMGMT_BTBONDINFO_VUSER_SLOT_START        MAX_NUMBER_OF_USERS
#define NODEMGMT_BTBONDINFO_VUSER_SLOT_STOP         128
#define NODEMGMT_BTBONDINFO_SIZE                    (NODEMGMT_USER_PROFILE_SIZE/2)
#define NB_MAX_BONDING_INFORMATION_TH               (NODEMGMT_BTBONDINFO_VUSER_SLOT_STOP-NODEMGMT_BTBONDINFO_VUSER_SLOT_START)*2*2  // For each user, we have one profile + user category names
#define NB_MAX_BONDING_INFORMATION                  32
#if NB_MAX_BONDING_INFORMATION > NB_MAX_BONDING_INFORMATION_TH
    #error "Max number of bonding information too high"
#endif

/* Credential types IDs */
typedef enum    {NODEMGMT_STANDARD_CRED_TYPE_ID = 0, NODEMGMT_WEBAUTHN_CRED_TYPE_ID = 1} nodemgmt_cred_type_te;
/* Data types IDs */
typedef enum    {NODEMGMT_STANDARD_DATA_TYPE_ID = 0, NODEMGMT_NOTES_DATA_TYPE_ID = 1} nodemgmt_data_category_te;

/* Structs */
// Parent node, see: https://mooltipass.github.io/minible/database_model
typedef struct
{
    uint16_t flags;
    uint16_t prevParentAddress;                 // Previous parent node address (Alphabetically)
    uint16_t nextParentAddress;                 // Next parent node address (Alphabetically)
    uint16_t nextChildAddress;                  // Parent node first child address
    cust_char_t service[SERVICE_NAME_MAX_LEN];  // Unicode BMP text describing service, used for sorting and searching
    uint16_t last_cnode_used_addr;              // Address of the child node last used for that parent
    uint8_t reserved[2];                        // Reserved for future use
} parent_cred_node_t;

// Parent node, see: https://mooltipass.github.io/minible/database_model
typedef struct
{
    uint16_t flags;
    uint16_t prevParentAddress;                 // Previous parent node address (Alphabetically)
    uint16_t nextParentAddress;                 // Next parent node address (Alphabetically)
    uint16_t nextChildAddress;                  // Parent node first child address
    cust_char_t service[SERVICE_NAME_MAX_LEN];  // Unicode BMP text describing service, used for sorting and searching
    uint8_t reserved;                           // Reserved for future use
    uint8_t startDataCtr[3];                    // Encryption counter
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

typedef struct
{
    uint16_t fakeFlags;             // Same as flags but with bit 5 set to 1
    uint8_t data2[256];             // Encrypted data (256B)
    uint8_t reserved2[6];           // Reserved for future use    
} child_data_node_second_half_t;

typedef struct TOTP_cred_node_s
{
    union
    {
        uint8_t TOTPsecret[TOTP_SECRET_MAX_LEN];    // Encrypted secret
        uint8_t TOTPsecret_ct[TOTP_SECRET_MAX_LEN]; // Decrypted (cleartext) secret
    };
    uint16_t TOTPsecretLen;        // Length of TOTPsecret
    uint8_t TOTPtimeStep;          // Time step to calculate TOTP value over (default: 30s, max: 99s)
    uint8_t TOTP_SHA_ver;          // SHA version used for TOTP (0 - SHA1 (default), 1 = SHA256, 2 = SHA512)
    uint8_t TOTPnumDigits;         // Number of digits for TOTP value
    uint8_t TOTPsecret_ctr[3];     // Encryption counter
    uint8_t reserved1;             // Reserved for future use
    uint8_t reserved2;             // Reserved for future use
} TOTP_cred_node_t;

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
    cust_char_t login[LOGIN_NAME_MAX_LEN];  // Unicode BMP login
    cust_char_t description[24];            // Unicode BMP description
    cust_char_t thirdField[36];             // Unicode BMP third field
    uint16_t keyAfterLogin;                 // Typed key after login
    uint16_t keyAfterPassword;              // Typed key after password
    uint16_t fakeFlags;                     // Same as flags but with bit 5 set to 1
    uint8_t passwordBlankFlag;              // No password flag
    uint8_t ctr[3];                         // Encryption counter
    union
    {
        uint8_t password[128];              // Encrypted password
        cust_char_t cust_char_password[64];
    };
    cust_char_t pwdTerminatingZero; // Set to 0
    TOTP_cred_node_t TOTP;          // TOTP entry
    uint8_t TBD[54];                // TBD
} child_cred_node_t;

// Webauthn credential
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
    /* As the user name is next, this is what will be used for alphabetical sorting! */
    cust_char_t user_name[64];      // A human readable identifier for a user account (for example, "alexm", "alex.p.mueller@example.com" or "+14255551234".). Intended for display only, stored as BMP.
    cust_char_t user_name_t0;       // Terminating 0
    uint8_t credential_id[16];      // Credential identifier. A probabilistically-unique byte sequence identifying a public key credential source (tag in solokey). Generated by device
    uint8_t user_handle[64];        // An opaque byte sequence with a maximum size of 64 bytes. User handles are not meant to be displayed to users. Generated by RP
    uint8_t user_handle_len;        // Actual length of user handle
    uint8_t reserved[2];            // Reserved
    uint8_t ctr[3];                 // Encryption counter
    uint8_t private_key[32];        // Encrypted private key
    uint16_t signature_counter_msb; // Signature counter msb
    uint16_t signature_counter_lsb; // Signature counter lsb
    uint16_t fakeFlags;             // Same as flags but with bit 5 set to 1
    cust_char_t display_name[64];   // A human readable name for a user account (for example, "Alex P. Muller"). Intended for display only, stored as BMP.
    cust_char_t display_name_t0;    // Terminating 0
    uint8_t TBD[132];               // TBD
} child_webauthn_node_t;

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
        child_webauthn_node_t webauthn_child;
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
        child_webauthn_node_t webauthn_child;
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
    uint16_t cred_start_addresses[10];
    uint16_t data_start_addresses[7];
    uint16_t sec_preferences;
    uint16_t language_id;
    uint16_t layout_id;
    uint16_t ble_layout_id;
    uint16_t nb_languages_known;
    uint16_t nb_keyboards_layout_known;    
    uint8_t reserved[7];
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

// User favorite strings
typedef struct
{
    cust_char_t category_strings[4][33];
} nodemgmt_user_category_strings_t;

// Node management handle
typedef struct
{
    BOOL datadbChanged;                     // Boolean to indicate if the user data DB has changed since user login
    BOOL dbChanged;                         // Boolean to indicate if the user DB has changed since user login
    uint16_t currentUserId;                 // The users ID
    uint16_t pageUserProfile;               // The page of the user profile
    uint16_t offsetUserProfile;             // The offset of the user profile
    uint16_t pageUserCategoryStrings;       // The page of the user favorite strings
    uint16_t offsetUserCategoryStrings;     // The offset of the user favorite strings
    uint16_t firstCredParentNodes[10];      // The address of the users first cred parent node (read from flash. eg cache)
    uint16_t firstDataParentNodes[7];       // The addresses of the users first data parent nodes (read from flash. eg cache)
    uint16_t nextParentFreeNode;            // The address of the next free parent node
    uint16_t nextChildFreeNode;             // The address of the next free child node
    parent_node_t temp_parent_node;         // Temp parent node to be used when needed
    uint16_t currentCategoryId;             // Current category ID
    uint16_t currentCategoryFlags;          // Current category flags
    uint16_t lastCredParentNodes[10];      // The address of the users last cred parent node (read from flash. eg cache)
    uint16_t lastDataParentNodes[7];       // The addresses of the users last data parent nodes (read from flash. eg cache)
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

/*! \fn     nodemgmt_categoryflags_to_flags(uint16_t* flags, uint16_t cat_flag)
*   \brief  Sets the category id to flags
*   \param  flags           The flags field of a node
*   \return uid             The category id to set in flags
*/
static inline void nodemgmt_categoryflags_to_flags(uint16_t* flags, uint16_t cat_flag)
{
    *flags = (*flags & (~NODEMGMT_CAT_MASK)) | (cat_flag << NODEMGMT_CAT_BITSHIFT);    
}

 /*! \fn     categoryFromFlags(uint16_t flags)
 *   \brief  Gets the category from flags
 *   \return category
 */
static inline uint16_t categoryFromFlags(uint16_t flags)
{
    return ((flags >> NODEMGMT_CAT_BITSHIFT) & NODEMGMT_CAT_MASK_FINAL);
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

/*! \fn     nodeTypeFromFlags(uint16_t flags)
*   \brief  Gets nodeType from flags  
*   \param  flags           The flags field of a node
*   \return nodeType        See enum
*/
static inline node_type_te nodeTypeFromFlags(uint16_t flags)
{
    return (flags >> NODEMGMT_TYPE_FLAG_BITSHIFT) & NODEMGMT_TYPE_FLAG_BITMASK_FINAL;
}

/* Prototypes */
RET_TYPE nodemgmt_create_generic_node(generic_node_t* g, node_type_te node_type, uint16_t firstNodeAddress, uint16_t* newFirstNodeAddress, uint16_t* storedAddress, uint16_t* newLastNodeAddress);
void nodemgmt_get_prev_favorite_and_category_index(int16_t category_index, int16_t favorite_index, int16_t* new_cat_index, int16_t* new_fav_index, BOOL navigate_across_categories);
void nodemgmt_get_next_favorite_and_category_index(int16_t category_index, int16_t favorite_index, int16_t* new_cat_index, int16_t* new_fav_index, BOOL navigate_across_categories);
RET_TYPE nodemgmt_get_bluetooth_bonding_information_for_mac_addr(uint8_t address_resolv_type, uint8_t* mac_address, nodemgmt_bluetooth_bonding_information_t* bonding_information);
uint16_t nodemgmt_find_free_nodes(uint16_t nbParentNodes, uint16_t* parentNodeArray, uint16_t nbChildtNodes, uint16_t* childNodeArray, uint16_t startPage, uint16_t startNode);
void nodemgmt_read_webauthn_child_node_except_display_name(uint16_t address, child_webauthn_node_t* child_node, BOOL update_date_and_increment_preinc_count);
void nodemgmt_init_context(uint16_t userIdNum, uint16_t* userSecFlags, uint16_t* userLanguage, uint16_t* userLayout, uint16_t* userBLELayout);
RET_TYPE nodemgmt_get_bluetooth_bonding_information_for_irk(uint8_t* irk_key, nodemgmt_bluetooth_bonding_information_t* bonding_information);
void nodemgmt_format_user_profile(uint16_t uid, uint16_t secPreferences, uint16_t languageId, uint16_t keyboardId, uint16_t bleKeyboardId);
void nodemgmt_read_webauthn_child_node(uint16_t address, child_webauthn_node_t* child_node, BOOL update_date_and_increment_preinc_count);
uint16_t nodemgmt_get_data_parent_next_child_address_ctr_and_prev_gen_flag(uint16_t parent_address, uint8_t* ctr, BOOL* prev_gen_flag);
void nodemgmt_update_data_parent_ctr_and_first_child_address(uint16_t parent_address, uint8_t* ctr_val, uint16_t first_child_address);
int32_t nodemgmt_get_next_non_null_favorite_before_index(uint16_t favId, uint16_t category_id, BOOL navigate_across_categories);
int32_t nodemgmt_get_next_non_null_favorite_after_index(uint16_t favId, uint16_t category_id, BOOL navigate_across_categories);
uint16_t nodemgmt_get_encrypted_data_from_data_node(uint16_t data_child_address, uint8_t* buffer, uint16_t* nb_bytes_written);
uint16_t nodemgmt_get_prev_parent_node_for_cur_category(uint16_t search_start_parent_addr, uint16_t credential_type_id);
uint16_t nodemgmt_get_next_parent_node_for_cur_category(uint16_t search_start_parent_addr, uint16_t credential_type_id);
RET_TYPE nodemgmt_create_parent_node(parent_node_t* p, service_type_te type, uint16_t* storedAddress, uint16_t typeId);
RET_TYPE nodemgmt_store_bluetooth_bonding_information(nodemgmt_bluetooth_bonding_information_t* bonding_information);
uint16_t nodemgmt_check_for_logins_with_category_in_parent_node(uint16_t start_child_addr, uint16_t category_flags);
void nodemgmt_read_favorite(uint16_t categoryId, uint16_t favId, uint16_t* parentAddress, uint16_t* childAddress);
void nodemgmt_read_favorite_for_current_category(uint16_t favId, uint16_t* parentAddress, uint16_t* childAddress);
void nodemgmt_write_child_node_block_to_flash(uint16_t address, child_node_t* child_node, BOOL write_category);
void nodemgmt_set_favorite(uint16_t categoryId, uint16_t favId, uint16_t parentAddress, uint16_t childAddress);
void nodemgmt_get_bluetooth_bonding_info_starting_offset(uint16_t uid, uint16_t *page, uint16_t *pageOffset);
RET_TYPE nodemgmt_read_parent_node_permissive(uint16_t address, parent_node_t* parent_node, BOOL data_clean);
void nodemgmt_get_user_category_names_starting_offset(uint16_t uid, uint16_t *page, uint16_t *pageOffset);
void nodemgmt_get_bluetooth_bonding_information_irks(uint16_t* nb_keys, uint8_t* aggregated_keys_buffer);
void nodemgmt_update_child_data_node_with_next_address(uint16_t child_address, uint16_t next_address);
void nodemgmt_set_last_used_child_node_for_service(uint16_t parent_address, uint16_t child_address);
void nodemgmt_get_user_profile_starting_offset(uint16_t uid, uint16_t *page, uint16_t *pageOffset);
RET_TYPE nodemgmt_create_child_node(uint16_t pAddr, child_cred_node_t* c, uint16_t* storedAddress);
void nodemgmt_read_parent_node_data_block_from_flash(uint16_t address, parent_node_t* parent_node);
void nodemgmt_write_parent_node_data_block_to_flash(uint16_t address, parent_node_t* parent_node);
void nodemgmt_read_child_node_data_block_from_flash(uint16_t address, child_node_t* child_node);
void nodemgmt_read_cred_child_node_except_pwd(uint16_t address, child_cred_node_t* child_node);
void nodemgmt_read_parent_node(uint16_t address, parent_node_t* parent_node, BOOL data_clean);
void nodemgmt_delete_data_parent_and_its_children(uint16_t parent_address, uint16_t typeId);
void nodemgmt_extract_date(uint16_t date, uint16_t* year, uint16_t* month, uint16_t* day);
void nodemgmt_set_cred_start_address(uint16_t parentAddress, uint16_t credential_type_id);
uint16_t nodemgmt_get_prev_child_node_for_cur_category(uint16_t search_start_child_addr);
uint16_t nodemgmt_get_next_child_node_for_cur_category(uint16_t search_start_child_addr);
uint16_t nodemgmt_get_last_parent_addr(BOOL data_parent, uint16_t credential_type_id);
uint16_t nodemgmt_get_starting_parent_addr_for_category(uint16_t credential_type_id);
RET_TYPE nodemgmt_check_user_permission(uint16_t node_addr, node_type_te* node_type);
void nodemgmt_read_cred_child_node(uint16_t address, child_cred_node_t* child_node);
RET_TYPE nodemgmt_store_data_node(child_data_node_t* node, uint16_t* storedAddress);
void nodemgmt_delete_children_list(uint16_t first_children_addr, BOOL data_child);
void nodemgmt_set_data_start_address(uint16_t dataParentAddress, uint16_t typeId);
void nodemgmt_get_category_strings(nodemgmt_user_category_strings_t* strings_pt);
void nodemgmt_set_category_strings(nodemgmt_user_category_strings_t* strings_pt);
void nodemgmt_get_category_string(uint16_t category_id, cust_char_t* string_pt);
void nodemgmt_set_category_string(uint16_t category_id, cust_char_t* string_pt);
uint16_t nodemgmt_construct_date(uint16_t year, uint16_t month, uint16_t day);
uint16_t nodemgmt_get_starting_parent_addr(uint16_t credential_type_id);
uint16_t nodemgmt_get_sec_preference_for_user_id(uint16_t userIdNum);
uint16_t nodemgmt_get_user_language_for_user_id(uint16_t userIdNum);
uint16_t nodemgmt_get_first_child_address(uint16_t parent_address);
void nodemgmt_store_user_sec_preferences(uint16_t sec_preferences);
void nodemgmt_check_address_validity_and_lock(uint16_t node_addr);
void nodemgmt_check_user_perm_from_flags_and_lock(uint16_t flags);
uint16_t nodemgmt_get_start_addresses(uint16_t* addresses_array);
uint16_t nodemgmt_get_starting_data_parent_addr(uint16_t typeId);
void nodemgmt_delete_all_bluetooth_bonding_information(void);
RET_TYPE nodemgmt_check_address_validity(uint16_t node_addr);
RET_TYPE nodemgmt_check_user_perm_from_flags(uint16_t flags);
void nodemgmt_set_start_addresses(uint16_t* addresses_array);
void nodemgmt_set_data_change_number(uint32_t changeNumber);
void nodemgmt_set_cred_change_number(uint32_t changeNumber);
uint16_t nodemgmt_get_user_nb_known_keyboard_layouts(void);
uint16_t nodemgmt_get_favorites(uint16_t* addresses_array);
uint16_t nodemgmt_get_incremented_address(uint16_t addr);
void nodemgmt_user_db_changed_actions(BOOL dataChanged);
void nodemgmt_store_user_language(uint16_t languageId);
void nodemgmt_store_user_ble_layout(uint16_t layoutId);
void nodemgmt_set_current_category_id(uint16_t catId);
void nodemgmt_allow_new_change_number_increment(void);
uint16_t nodemgmt_get_user_nb_known_languages(void);
void nodemgmt_delete_current_user_from_flash(void);
uint16_t nodemgmt_get_current_category_flags(void);
void nodemgmt_store_user_layout(uint16_t layoutId);
void nodemgmt_trigger_db_ext_changed_actions(void);
uint16_t nodemgmt_get_user_sec_preferences(void);
uint32_t nodemgmt_get_cred_change_number(void);
uint32_t nodemgmt_get_data_change_number(void);
void nodemgmt_scan_for_last_parent_nodes(void);
void nodemgmt_set_current_date(uint16_t date);
uint16_t nodemgmt_get_current_category(void);
uint16_t nodemgmt_get_user_ble_layout(void);
uint16_t nodemgmt_get_user_language(void);
void nodemgmt_read_profile_ctr(void* buf);
void nodemgmt_set_profile_ctr(void* buf);
uint16_t nodemgmt_get_current_date(void);
uint16_t nodemgmt_get_user_layout(void);
void nodemgmt_scan_node_usage(void);

#endif /* NODEMGMT_H_ */
