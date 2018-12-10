## [](#header-1) Mooltipass Database Model
This page details the database model for the new Mooltipass Mini.  
- All strings use uint16_t as a character length.  
- UserID field is now 7 bits: the biggest DB flash has 32k 264B pages, so for a one page parent node and a 2 pages child node that's an average of 85 credentials per user.  
   
## [](#header-2) 2 Bytes Long Flags

**Parent Node**

| bits | description | on the previous mini DB model |
|:-----|:------------|-------------------------------|
| 15->14 | 0b00 (creds), 0b10 (data) | same |
| 13->13 | not valid bit | same |
| 12->8 | user ID MSbs (5b) | userID |
| 7->6 | user ID LSbs (2b) | reserved |
| 5->4 | reserved | reserved |
| 3->0 | data: data type | not used |

**Child Node**

| bits | description | on the previous mini DB model |
|:-----|:------------|-------------------------------|
| 15->14 | 0b01 | same |
| 13->13 | not valid bit | same |
| 12->8 | user ID MSbs (5b) | userID |
| 7->6 | user ID LSbs (2b) | not used |
| 5->4 | reserved | not used |
| 3->0 | credential category UID | not used |

**Backward compatibility**: specify a 0b1111 category for a password imported from an old mini DB (ascii flag).  
**Forward compatibility**: use a bit in the 7->0 field to set unicode flag.  

**Data Node**

| bits | description | on the previous mini DB model |
|:-----|:------------|-------------------------------|
| 15->14 | 0b11: specified data node | same |
| 13->13 | not valid bit | same |
| 12->8 | user ID MSbs (5b) | userID |
| 7->6 | user ID LSbs (2b) | payload length MSB (2b) |
| 5->0 | # of data nodes | payload length LSB (6b) |

## [](#header-2) Nodes

**Parent Node (264B)**

| bytes | description | on the previous mini DB model |
|:------|:------------|-------------------------------|
| 0->1 | flags (2B) | same |
| 2->3 | previous parent address (2B) | same |
| 4->5 | next parent address (2B) | same |
| 6->7 | first child address (2B) | same |
| 8->259 | service name (252B) | 8->128 service, 129->131 ctr |
| 260->260 | reserved (1B) | out of bounds |
| 261->263 | CTR value for data child (3B) | out of bounds |

**Data Node (528B)**

| bytes | description | on the previous mini DB model |
|:------|:------------|-------------------------------|
| 0->1 | flags (2B) | same |
| 2->3 | next data address (2B) | same |
| 4->515 | 512B of encrypted data | 4->131 encrypted data |
| 516->519 | encrypted data length (4B) | out of bounds |
| 520->527 | reserved (8B) | out of bounds |

**Child Node (528B)**

| bytes | description |
|:------|:------------|
| 0->1 | flags (2B) |
| 2->3 | previous child address (2B) |
| 4->5 | next child address (2B) |
| 6->7 | pointed to child address (0 when not a pointer) (2B) |
| 8->135 | plain text login (128B) |
| 136->199 | plain text description (64B) |
| 200->201 | last modified date #1 (2B) |
| 202->203 | last used date #1 (2B) |
| 204->206 | CTR value #1 (3B) |
| 207->334 | encrypted password #1 (128B) |
| 335->336 | last modified date #2 (2B) |
| 337->338 | last used date #2 (2B) |
| 339->341 | CTR value #2 (3B) |
| 342->469 | encrypted password #2 (128B) |
| 470->519 | arbitrary third field (50B) |
| 520->521 | key pressed after login typing (2B) |
| 522->523 | key press after password typing (2B) |
| 524->527 | reserved (4B) |

**For Information: Previous DB Child Node (132B)**

| bytes | description |
|:------|:------------|
| 0->1 | flags (2B) |
| 2->3 | previous child address (2B) |
| 4->5 | next child address (2B) |
| 6->29 | plain text description (24B) |
| 30->31 | last modified date (2B) |
| 32->33 | last used date (2B) |
| 34->36 | CTR value (3B) |
| 37->99 | plain text login (63B) |
| 100->131 | encrypted password (32B) |
| 524->527 | reserved (4B) |
