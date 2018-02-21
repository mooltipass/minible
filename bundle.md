## [](#header-1) Mooltipass Graphics Bundle Composition
This page explains in details the graphics bundle data structure.
   
## [](#header-2) Bundle Header

| bytes             | name       | description |
|:-------------------|:---------------|:----------|
| 0-3   | magic_header | 0x12345678 |
| 4-7   | total_size | Bundle total size |
| 8-11  | crc32 | Bundle CRC32 starting from byte 12 |
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
| 116-119 | language_map_item_count | Number of language map items |
| 120-123 | language_map_offset | Start address to find language map items |
| 124-127 | language_bitmap_starting_id | Starting index for language bitmaps |
   
   
## [](#header-2) Bitmap File

| bytes             | name       | description |
|:-------------------|:---------------|:----------|
| 0-1 | width | Bitmap width |
| 2 | height | Bitmap height |
| 3 | xpos | Recommended X display position |
| 4 | ypos | Recommended Y display position |
| 5 | depth | Number of bits per pixel |
| 6-7 | flags | Flags defining data format |
| 8-9 | dataSize | Payload datasize |
| 10-... | data | Bitmap data |
  
  
## [](#header-2) Font File

| bytes             | description |
|:-------------------|:----------|
| 0-5 | Font header |
| 6-(6+last_chr_val*2) | Font header |
  
  
## [](#header-2) Font Header

| bytes             | name       | description |
|:-------------------|:---------------|:----------|
| 0 | height | Font height |
| 1 | depth | Number of bits per pixel |
| 2-3 | last_chr_val | Unicode value of last character |
| 4-5 | chr_count | Number of characters in this font |
  
  
  ## [](#header-2) Font Glyph

| bytes             | name       | description |
|:-------------------|:---------------|:----------|
| 0 | xrect | x width of rectangle |
| 1 | yrect | y height of rectangle |
| 2 | xoffset | x offset of glyph in rectangle |
| 3 | yoffset | y offset of glyph in rectangle |
| 4-7 | glyph_data_offset | offset from beginning of pixel data for this glyph data |

