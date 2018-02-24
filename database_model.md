## [](#header-1) Mooltipass Database Model
This page details the database model for the new Mooltipass Mini.  
All strings use uint16_t as a character length.  
   
## [](#header-2) 2 Bytes Long Flags

**Parent Node**

| bits | description | on the previous mini DB model |
|:-----|:------------|-------------------------------|
| 15->14 | 0b00: specifies parent node type | same |
| 13->13 | not valid bit | same |
| 12->8 | user ID MSB (5b) | userID |
| 7->4 | user ID LSB (4b) | reserved |
| 3->0 | reserved | not used |

**Child Node**

| bits | description | on the previous mini DB model |
|:-----|:------------|-------------------------------|
| 15->14 | 0b01: credentials, 0b10: data start | same |
| 13->13 | not valid bit | same |
| 12->8 | user ID MSB (5b) | userID |
| 7->4 | user ID LSB (4b) | not used |
| 3->0 | credential category UID | not used |

**Backward compatibility**: specify a 0b1111 category for a password imported from an old mini DB (ascii flag).  
**Forward compatibility**: use a bit in the 7->0 field to set unicode flag.  

**Data Node**

| bits | description | on the previous mini DB model |
|:-----|:------------|-------------------------------|
| 15->14 | 0b11: specified data node | same |
| 13->13 | not valid bit | same |
| 12->8 | user ID MSB (5b) | userID |
| 7->4 | user ID LSB (4b) | payload length MSB (4b) |
| 3->0 | data category UID | payload length LSB (4b) |

## [](#header-2) Nodes

**Parent Node**

| bytes | description | on the previous mini DB model |
|:------|:------------|-------------------------------|
| 0->1 | flags | same |
| 2->3 | previous parent address | same |
| 4->5 | next parent address | same |
| 6->7 | first child address | same |
| 8->259 | service name | 8->128 service, 129->131 ctr |
| 260->260 | reserved | out of bounds |
| 261->263 | CTR value for data child | out of bounds |


**Data Node**

| bytes | description | on the previous mini DB model |
|:------|:------------|-------------------------------|
| 0->1 | flags | same |
| 2->3 | next data address | same |
| 4->515 | 512B of encrypted data | 4->131 encrypted data |
| 516->519 | encrypted data length | out of bounds |
| 520->527 | reserved | out of bounds |
