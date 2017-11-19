/*
 * custom_fs.h
 *
 * Created: 16/05/2017 11:39:40
 *  Author: stephan
 */ 


#ifndef CUSTOM_FS_H_
#define CUSTOM_FS_H_

#include "platform_defines.h"
#include "defines.h"

/* Defines */
#define CUSTOM_FS_MAX_FILE_COUNT    0xFFFFFFFFUL
#define CUSTOM_FS_FILES_ADDR_OFFSET 0x0000
// Custom file flags
#define CUSTOM_FS_BITMAP_RLE_FLAG   0x01

/* Typedefs */
typedef uint32_t custom_fs_file_count_t;
typedef uint32_t custom_fs_address_t;
typedef uint16_t custom_fs_file_type_t;
typedef uint16_t custom_fs_string_count_t;
typedef uint16_t custom_fs_string_length_t;
typedef uint16_t custom_fs_string_offset_t;
typedef uint32_t custom_fs_binfile_size_t;

/* Enums */
typedef enum {CUSTOM_FS_STRING_TYPE = 0, CUSTOM_FS_FONTS_TYPE = 1, CUSTOM_FS_BITMAP_TYPE = 2, CUSTOM_FS_BINARY_TYPE = 3, CUSTOM_FS_FW_UPDATE_TYPE = 4} custom_fs_file_type_te;
    
/* Structs */

// First bytes inside the external flash:
// magic number: to indicate the presence of something in the flash
// total size: total size of the bundle
// crc32: crc32 of what is after the signed hash
// signed hash: guess what it is...
// string file count: number of string files
// string file offset: starting address at which to find the address of each string file
// same for fonts, bitmaps, binary imgs...
typedef struct
{
    uint32_t magic_header;
    uint32_t total_size;
    uint32_t crc32;
    uint8_t signed_hash[64];
    custom_fs_file_count_t update_file_count;
    custom_fs_address_t update_file_offset;
    custom_fs_file_count_t string_file_count;
    custom_fs_address_t string_file_offset;
    custom_fs_file_count_t fonts_file_count;
    custom_fs_address_t fonts_file_offset;
    custom_fs_file_count_t bitmap_file_count;
    custom_fs_address_t bitmap_file_offset;
    custom_fs_file_count_t binary_img_file_count;
    custom_fs_address_t binary_img_file_offset;
    uint8_t reserved[12];
} custom_file_flash_header_t;

// Platform settings
typedef struct  
{
    uint8_t reserved[252];
    uint32_t start_upgrade_flag;
} custom_platform_settings_t;

// Bitmap header
typedef struct
{
    uint16_t width;     //*< width of image in pixels
    uint8_t height;     //*< height of image in pixels
    uint8_t depth;      //*< Number of bits per pixel
    uint16_t flags;     //*< Flags defining data format
    uint16_t dataSize;  //*< number of words in data
    uint16_t data[];    //*< pointer to the image data
} bitmap_t;

// Font header
typedef struct
{
    uint8_t height;         //*< height of font
    uint8_t depth;          //*< Number of bits per pixel
    uint16_t last_chr_val;  //*< Unicode value of last character
    uint16_t chr_count;     //*< Number of characters in this font
} font_header_t;

// Glyph struct
typedef struct
{
    uint8_t xrect;                  // x width of rectangle
    uint8_t yrect;                  // y height of rectangle
    int8_t xoffset;                 // x offset of glyph in rectangle
    int8_t yoffset;                 // y offset of glyph in rectangle
    custom_fs_address_t glyph;      // offset to glyph data
} font_glyph_t;

/* Prototypes */
RET_TYPE custom_fs_continuous_read_from_flash(uint8_t* datap, custom_fs_address_t address, uint32_t size, BOOL use_dma);
RET_TYPE custom_fs_get_file_address(uint32_t file_id, custom_fs_address_t* address, custom_fs_file_type_te file_type);
RET_TYPE custom_fs_get_string_from_file(uint32_t text_file_id, uint32_t string_id, cust_char_t** string_pt);
RET_TYPE custom_fs_read_from_flash(uint8_t* datap, custom_fs_address_t address, uint32_t size);
void custom_fs_write_256B_at_internal_custom_storage_slot(uint32_t slot_id, void* array);
void custom_fs_read_256B_at_internal_custom_storage_slot(uint32_t slot_id, void* array);
uint32_t custom_fs_get_custom_storage_slot_addr(uint32_t slot_id);
RET_TYPE custom_fs_compute_and_check_external_bundle_crc32(void);
void custom_fs_stop_continuous_read_from_flash(void);
BOOL custom_fs_settings_check_fw_upgrade_flag(void);
void custom_fs_settings_clear_fw_upgrade_flag(void);
void custom_fs_settings_set_fw_upgrade_flag(void);
void custom_fs_init(spi_flash_descriptor_t* desc);
void custom_fs_settings_init(void);

#endif /* CUSTOM_FS_H_ */