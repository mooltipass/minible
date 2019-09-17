# [](#header-1) Mooltipass Graphics Bundle Composition
This page explains in details the graphics bundle data structure.
  
  
## [](#header-2) Bundle Composition

| Contents |
|:---------|
| Bundle Header |
| File addresses |
| Update file(s) |
| String file(s) |
| Font file(s) |
| Bitmap file(s) |
| Binary file(s) |
| Language map(s) |

   
## [](#header-2) Bundle Header

| bytes             | name       | description |
|:-------------------|:---------------|:----------|
| 0->3   | magic_header | 0x12345678 |
| 4->7   | total_size | Bundle total size |
| 8->11  | crc32 | Bundle CRC32 starting from byte 12 |
| 12->75 | signed_hash | TBD |
| 76->79 | update_file_count | Number of update files |
| 80->83 | update_file_offset | Start address to find update file addresses |
| 84->87 | string_file_count | Number of string files |
| 88->91 | string_file_offset | Start address to find string file addresses |
| 92->95 | fonts_file_count | Number of font files |
| 96->99 | fonts_file_offset | Start address to find font file addresses |
| 100->103 | bitmap_file_count | Number of bitmap files |
| 104->107 | bitmap_file_offset | Start address to find bitmap file addresses |
| 108->111 | binary_img_file_count | Number of binary files (keyboard LUTs) |
| 112->115 | binary_img_file_offset | Start address to find binary file addresses |
| 116->119 | language_map_item_count | Number of language map items |
| 120->123 | language_map_offset | Start address to find language map items |
| 124->127 | language_bitmap_starting_id | Starting index for language bitmaps |
   
   
## [](#header-2) Bitmap File

| bytes             | name       | description |
|:-------------------|:---------------|:----------|
| 0->1 | width | Bitmap width |
| 2 | height | Bitmap height |
| 3 | xpos | Recommended X display position |
| 4 | ypos | Recommended Y display position |
| 5 | depth | Number of bits per pixel |
| 6->7 | flags | Flags defining data format |
| 8->9 | dataSize | Payload datasize |
| 10->... | data | Bitmap data |
  
  
## [](#header-2) Font File

| bytes             | description |
|:-------------------|:----------|
| 0->5 | Font header |
| 6->65 | 15x uint16_t (interval_start-interval_end) of unicode chars for which we provide support description |
| 66->(66+described_chr_countx2) | Uint16_t array of glyph indexes (set to an index when we support the char, 0xFFFF otherwise) |
| (66+described_chr_countx2)->(66+described_chr_countx2+chr_countx8) | Glyph array |
| (66+described_chr_countx2+chr_countx8)->... | Pixel data for glyphs |
  
  
## [](#header-2) Font Header

| bytes             | name       | description |
|:-------------------|:---------------|:----------|
| 0 | height | Font height |
| 1 | depth | Number of bits per pixel |
| 2->3 | described_chr_count | Number of characters for which we describe support |
| 4->5 | chr_count | Number of characters in this font |
  
  
## [](#header-2) Font Glyph

| bytes             | name       | description |
|:-------------------|:---------------|:----------|
| 0 | xrect | x width of rectangle |
| 1 | yrect | y height of rectangle |
| 2 | xoffset | x offset of glyph in rectangle |
| 3 | yoffset | y offset of glyph in rectangle |
| 4->7 | glyph_data_offset | offset from beginning of pixel data for this glyph data |
  
    
## [](#header-2) Text File

| bytes             | description |
|:-------------------|:----------|
| 0->1 | Number of strings in text file (nb_strings) |
| 2->2+2xnb_strings | Array of offset addresses for the nb_strings (addra, addrb, addrc...) |
| addra->addra+1 | Length of string #0 (length0), including final 0 |
| addra+2->addra+2+length0 | String #0 |
| addrb->addrb+1 | Length of string #1 (length1), including final 0 |
| addrb+2->addrb+2+length1 | String #1 |
  
  
## [](#header-2) Language Map Entry

| bytes             | description |
|:-------------------|:----------|
| 0->35 | Unicode string of language description, with terminating 0x0000 |
| 36->37 | String file ID for that language |
| 38->39 | Starting font ID for that language |
| 40->41 | Starting bitmap ID for that language |
| 42->43 | Recommended keyboard layout ID |
  
  
## [](#header-2) Keyboard File Entry

| bytes             | description |
|:-------------------|:----------|
| 0->39 | Unicode string of layout description, with terminating 0x0000 |
| 40->99 | 15x uint16_t (interval_start-interval_end) of unicode chars for which we provide support description |
| 100->... | Array of uint16_t, each describing how to type a glyph (see below) |

| bits               | description |
|:-------------------|:----------|
| 15 | If 14->8 is 0: dead key bitmask |
| 14->8 | 0: only one key is required, 0x3f: not implemented, else: first key to type |
| 7->0 | HID key to type (0x80: shift bitmask, 0x40: alt bitmask, 0x03: europe key) |
