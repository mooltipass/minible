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
| 5 | flags not valid bit: 0b0 | reserved |
| 4 | reserved | reserved |
| 3->0 | data: data type | not used |

**Child Node**

| bits | description | on the previous mini DB model |
|:-----|:------------|-------------------------------|
| 15->14 | 0b01 | same |
| 13->13 | not valid bit | same |
| 12->8 | user ID MSbs (5b) | userID |
| 7->6 | user ID LSbs (2b) | not used |
| 5 | flags not valid: 0b0 | not used |
| 4 | ascii flag | not used |
| 3->0 | credential category bitfield | not used |

**Forward compatibility**: use a bit in the 7->0 field to set unicode flag.  
By setting bit 5 to 0 and the "matching" bit one page later to 1, this allows us to detect when scanning memory contents the child nodes first and second 264B blocks.

**Data Node**

| bits | description | on the previous mini DB model |
|:-----|:------------|-------------------------------|
| 15->14 | 0b11: specified data node | same |
| 13->13 | not valid bit | same |
| 12->8 | user ID MSbs (5b) | userID |
| 7->6 | user ID LSbs (2b) | payload length MSB (2b) |
| 5 | flags not valid bit: 0b0 | payload length LSB (1b) |
| 4->0 | # of data nodes | payload length LSB (5b) |

By setting bit 5 to 0 and the "matching" bit one page later to 1, this allows us to detect when scanning memory contents the child nodes first and second 264B blocks.  
The # of data nodes is meant for information only, not for actual size (capped at 16).  

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
| 4->5 | encrypted data length (2B) | 4->131 encrypted data |
| 6->261 | 256B of encrypted data | 4->131 encrypted data |
| 262->263 | reserved (2B) | out of bounds |
| 264->265 | same as flags, but with bit 5 set to 1 | out of bounds |
| 266->521 | next 256B of encrypted data | 4->131 encrypted data |
| 522->527 | reserved (6B) | out of bounds |

**Child Node (528B)**

| bytes | description |
|:------|:------------|
| 0->1 | flags (2B) |
| 2->3 | previous child address (2B) |
| 4->5 | next child address (2B) |
| 6->7 | pointed to child address (0 when not a pointer) (2B) |
| 8->9 | last modified date (2B) |
| 10->11 | last used date (2B) |
| 12->139 | plain text login (128B) |
| 140->187 | plain text description (48B) |
| 188->259 | arbitrary third field (36B) |
| 260->261 | key pressed after login typing (2B) |
| 262->263 | key press after password typing (2B) |
| 264->265 | same as flags, but with bit 5 set to 1 |
| 266->393 | encrypted password (128B) |
| 394 | reserved |
| 395->397 | CTR value (3B) |
| 398->527 | TBD |

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

## [](#header-2) User Profile and DB Flash Layout

As with the previous mini device, sector 0a/b of the DB flash is reserved for user profiles and potentially other graphics.  
For the mini ble, it will store up to 128 user profiles. Each user profile is 264B long and its contents are shown in the table below. As a result, the first 32kB in each DB flash are reserved for user profiles. For information, 8Mb/16Mb/32Mb/64Mb DB flashes respectively contain 65kB/131kB/65kB/262kB in their sector 0.  
Compared to our previous device, the number of favorites has been reduced to 12 (instead of 14)as users seem to not use more than 10 favorites.  

**User Profile**

| bytes | description |
|:-----|:------------|
| 0->1 | credential start address |
| 2->3 | data start address |
| 4 | reserved |
| 5->7 | current CTR |
| 8->11 | credential change number |
| 12->15 | data change number |
| 16->63 | 12 favorites (no category) |
| 64->111 | 12 favorites (category #1) |
| 112->159 | 12 favorites (category #2) |
| 160->207 | 12 favorites (category #3) |
| 208->255 | 12 favorites (category #4) |
| 256->263 | reserved |
