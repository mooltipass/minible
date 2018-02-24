## [](#header-1) Mooltipass Database Model
This page details the database model for the new Mooltipass Mini
   
## [](#header-2) 2 Bytes Long Flags

**Parent Node**

| bits | description | on the previous mini DB model |
|:-----|:------------|-------------------------------|
| 15->14 | 0b00 : specifies parent node type | same |
| 13->13 | not valid bit | same |
| 12->4 | user ID (9b) | 12->8 reserved, 7->4 user ID |
   
