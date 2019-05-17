## [](#header-1) Mooltipass Database Model
This page details the database model for the new Mooltipass Mini.  
- All strings use uint16_t as a character size to support Unicode BMP  
- UserID field is now 7 bits: the biggest DB flash has 32k 264B pages, so for a one page parent node and a 2 pages child node that's an average of 85 credentials per user  
   
## [](#header-2) 2 Bytes Long Flags

**Parent Node**

| bits | description | on the previous mini DB model |
|:-----|:------------|-------------------------------|
| 15->14 | 0b00 (creds), 0b10 (data) | same |
| 13->13 | not valid bit | same |
| 12->8 | user ID MSbs (5b) | userID |
| 7->6 | user ID LSbs (2b) | reserved |
| 5 | flags not valid bit: 0b0 | reserved |
| 4 | data parent: prev gen flag | reserved |
| 3->0 | not used | not used |

**Child Node**

| bits | description | on the previous mini DB model |
|:-----|:------------|-------------------------------|
| 15->14 | 0b01 | same |
| 13->13 | not valid bit | same |
| 12->8 | user ID MSbs (5b) | userID |
| 7->6 | user ID LSbs (2b) | not used |
| 5 | flags not valid bit: 0b0 | not used |
| 4 | ascii flag | not used |
| 3->0 | credential category bitfield | not used |

**Forward compatibility**: use a bit in the 7->0 field to set unicode flag.  
By setting bit 5 to 0 and the "matching" bit one page later to 1, this allows us to detect when scanning memory contents the child nodes first and second 264B blocks.

**Data Node**

| bits | description | on the previous mini DB model |
|:-----|:------------|-------------------------------|
| 15->14 | 0b11 | same |
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
| 188->259 | arbitrary third field (72B) |
| 260->261 | key pressed after login typing (2B) |
| 262->263 | key press after password typing (2B) |
| 264->265 | same as flags, but with bit 5 set to 1 |
| 266 | reserved |
| 267->269 | CTR value (3B) |
| 270->397 | encrypted password (128B) |
| 398->399 | set to 0 |
| 400->527 | TBD |

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

## [](#header-2) User Profile and DB Flash Layout

As with the previous mini device, sector 0a/b of the DB flash is reserved for user profiles.  
For the new mini, it can store up to 128 user profiles. Each user profile is composed of two contiguous 264B blocks whose contents are shown in the table below. As a result, the first 65kB of each DB flash are reserved for user profiles. For information, 8Mb/16Mb/32Mb/64Mb DB flashes respectively contain 65kB/131kB/65kB/262kB in their sector 0.  
Compared to our previous device, the number of favorites has been reduced to 10 (instead of 14) as users seem to not use more.  

**User Profile - First Block**

| bytes | description |
|:-----|:------------|
| 0->1 | credential start address |
| 2->33 | 16 data start addresses |
| 34->35 | security preferences |
| 36->37 | language id |
| 38->52 | reserved (15B) |
| 53->55 | current CTR |
| 56->59 | credential change number |
| 60->63 | data change number |
| 64->85 | 10 favorites (no category) |
| 104->143 | 10 favorites (category #1) |
| 144->183 | 10 favorites (category #2) |
| 184->223 | 10 favorites (category #3) |
| 224->263 | 10 favorites (category #4) |

**User Profile - Second Block**

| bytes | description |
|:-----|:------------|
| 0->65 | Description string for category #1 |
| 66->131 | Description string for category #2 |
| 132->197 | Description string for category #3 |
| 198->263 | Description string for category #4 |

## [](#header-2) CPZ Lookup Table

As with the Mooltipass Mini, AES-CTR is used to encrypt all user data on the device. The encryption key is stored on the Mooltipass card while the AES CTR nonce is stored inside the MCU flash. At the time of user creation, a unique identifier is programmed into the Mooltipass card Code Protected Zone (CPZ) in order to associate a given card (and its clones) with a user ID and an AES-CTR nonce.  

However, as the Mooltipass Mini BLE includes fleet management capabilities allowing a remote computer to manage the database of a given user, we now offer the possibility to provision the database AES key **on the device**. In this case, the AES key stored inside the Mooltipass Card is used to decrypt the AES key used to encrypt the user data. This has the following advantages:  
- the card **and** the device are required to access the database encryption key  
- the database AES key may be renewed without reprogramming the user card(s)  

As a result, the Mooltipass Mini lookup table needed to be modified to include this encrypted AES key. The new LUT entry format follows:  

| bytes | description |
|:-----|:------------|
| 0 | User ID |
| 1 | Flag: use provisioned AES key |
| 2->9 | Card CPZ |
| 10->25 | AES CTR nonce |
| 26->57 | Encrypted database AES key |
| 58->63 | Reserved |

This lookup table is stored inside the RWWEE part of our MCU, which is configured to be 8kB. The first 4 256B pages being used for other purposes, the maximum number of users is therefore limited to **112**.
