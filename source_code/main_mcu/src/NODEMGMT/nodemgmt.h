/*!  \file     nodemgmt.h
*    \brief    Node management library
*    Created:  10/12/2018
*    Author:   Mathieu Stephan
*/


#ifndef NODEMGMT_H_
#define NODEMGMT_H_

#include "defines.h"

/* Typedefs */
typedef enum    {NODE_TYPE_PARENT = 0, NODE_TYPE_CHILD = 1, NODE_TYPE_PARENT_DATA = 2, NODE_TYPE_DATA = 3} node_type_te;

/* Defines */

/* Structs */
// Parent node, see: https://mooltipass.github.io/minible/database_model
typedef struct
{
    uint16_t flags;
    uint16_t prevParentAddress;     // Previous parent node address (Alphabetically)
    uint16_t nextParentAddress;     // Next parent node address (Alphabetically)
    uint16_t nextChildAddress;      // Parent node first child address
    cust_char_t service[126];       // Unicode BMP text describing service, used for sorting and searching
    uint8_t reserved;               // Reserved for future use
    uint8_t startDataCtr[3];        // Encryption counter in case the child is a data node
} parent_node_t;

// Child data node, see: https://mooltipass.github.io/minible/database_model
typedef struct
{
    uint16_t flags;
    uint16_t nextDataAddress;       // Next data node in sequence 
    uint8_t data[512];              // Encrypted data (512B)
    uint16_t data_length;           // Encrypted data length
    uint8_t reserved[10];           // Reserved for future use
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
    uint16_t fakeFlags;             // Set to 0b010xxxxx 0bxx1xxxxx to help db algorithm
    uint8_t reserved;               // Reserved
    uint8_t ctr[3];                 // Encryption counter
    uint8_t password[128];          // Encrypted password
    uint8_t TBD[130];               // TBD
} child_cred_node_t;


#endif /* NODEMGMT_H_ */