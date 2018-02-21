## [](#header-1) Mooltipass Graphics Bundle Composition
This page explains in details the graphics bundle data structure.
   
## [](#header-2) Bundle Header

| bytes             | name       | explanation |
|:-------------------|:---------------|:----------|
| 0-3   | magic_header | 0x12345678 |
| 4-7   | total_size | Bundle total size |
| 8-11  | crc32 | Bundle CRC32 starting from byte 16 |
| 12-75 | signed_hash | TBD |
| 76-79 | update_file_count | Number of update files |
| 80-83 | update_file_offset | Start address to find update file addresses |
| 84-87 | string_file_count | Number of string files |
| 88-91 | string_file_offset | Start address to find string file addresses |
| 92-95 | fonts_file_count | Number of font files |
| 96-99 | fonts_file_offset | Start address to find font file addresses |
| 100-103 | bitmap_file_count | Number of bitmap files |
| 104-107 | bitmap_file_offset | Start address to find bitmap file addresses |
| 108-111 | binary_img_file_count | Number of binary files (keyboard LUTs) |
| 112-115 | binary_img_file_offset | Start address to find binary file addresses |
| 108-111 | language_map_item_count | Number of language map items |
| 112-115 | language_map_offset | Start address to find language map items |
   
## [](#header-2) Mooltipass Commands

0x00: Debug Message
-------------------

| byte 0 | byte 1-2                    | bytes 3-X                          |
|:-------|:----------------------------|:-----------------------------------|
| 0x00   | hmstrlen(debug_message) + 2 | Debug Message + terminating 0x0000 |

Can be sent from both the device or the computer. **Does not require an answer.** 
