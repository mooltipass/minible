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
// Value indicating that there are no files within a descriptor
#define CUSTOM_FS_MAX_FILE_COUNT            0xFFFFFFFFUL
// Offset in the external memory for the bundle
#define CUSTOM_FS_FILES_ADDR_OFFSET         0x0000
// Magic address for the emergency font file
#define CUSTOM_FS_EMERGENCY_FONT_FILE_ADDR  0x80000000UL
// Magic number at the beginning of the flash header
#define CUSTOM_FS_MAGIC_HEADER              0x12345678UL
// Custom file flags
#define CUSTOM_FS_BITMAP_RLE_FLAG           0x01
// Flag to use provisioned key
#define  CUSTOM_FS_PROV_KEY_FLAG            0x91

/* Settings IDs */
#define NB_DEVICE_SETTINGS                  64
#define SETTING_RESERVED_ID                 0
#define SETTING_RANDOM_PIN_ID               1
#define SETTING_USER_INTERACTION_TIMEOUT_ID 2
#define SETTING_FLASH_SCREEN_ID             3
#define SETTING_DEVICE_DEFAULT_LANGUAGE     4

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
// language map items : number of language map items
// language map offset: starting adress at which to find the address of the language map
// language specific start id : starting ID at which a language specific bitmap is needed
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
    custom_fs_file_count_t language_map_item_count;
    custom_fs_address_t language_map_offset;
    uint32_t language_bitmap_starting_id;
} custom_file_flash_header_t;

// Platform settings: do not store anything critical here in case of glitching
typedef struct  
{
    uint8_t device_settings[NB_DEVICE_SETTINGS];
    uint8_t reserved[184];
    uint16_t powered_off_due_to_battery_flag;
    uint16_t first_boot_flag;
    uint32_t start_upgrade_flag;
} custom_platform_settings_t;

// Bitmap header
typedef struct
{
    uint16_t width;     //*< width of image in pixels
    uint8_t height;     //*< height of image in pixels
    uint8_t xpos;       //*< recommended X position
    uint8_t ypos;       //*< recommended Y position
    uint8_t depth;      //*< Number of bits per pixel
    uint16_t flags;     //*< Flags defining data format
    uint16_t dataSize;  //*< number of words in data
    uint16_t data[];    //*< pointer to the image data
} bitmap_t;

// Font header
typedef struct
{
    uint8_t height;                 //*< height of font
    uint8_t depth;                  //*< Number of bits per pixel
    uint16_t described_chr_count;   //*< Number of described unicode chars (supported or not)
    uint16_t chr_count;             //*< Number of characters in this font
} font_header_t;

// Unicode interval descriptor
typedef struct 
{
    uint16_t interval_start;
    uint16_t interval_end;
} unicode_interval_desc_t;

// Glyph struct
typedef struct
{
    uint8_t xrect;                          // x width of rectangle
    uint8_t yrect;                          // y height of rectangle
    int8_t xoffset;                         // x offset of glyph in rectangle
    int8_t yoffset;                         // y offset of glyph in rectangle
    custom_fs_address_t glyph_data_offset;  // offset to glyph data
} font_glyph_t;

// Language map entry
typedef struct
{
    cust_char_t language_descr[18]; // Language description in unicode
    uint16_t string_file_index;     // String file for that language
    uint16_t starting_font;         // Starting font for that language
    uint16_t starting_bitmap;       // Starting bitmap for that language
    uint16_t keyboard_layout_id;    // Recommended keyboard layout ID
} language_map_entry_t;

// CPZ LUT entry
typedef struct
{
    uint8_t user_id;
    uint8_t use_provisioned_key_flag;
    uint8_t cards_cpz[8];
    uint8_t nonce[AES_BLOCK_SIZE/8];
    uint8_t provisioned_key[AES_KEY_LENGTH/8];
    uint8_t reserved[6];
} cpz_lut_entry_t;

/* Prototypes */
RET_TYPE custom_fs_continuous_read_from_flash(uint8_t* datap, custom_fs_address_t address, uint32_t size, BOOL use_dma);
RET_TYPE custom_fs_get_file_address(uint32_t file_id, custom_fs_address_t* address, custom_fs_file_type_te file_type);
RET_TYPE custom_fs_get_string_from_file(uint32_t string_id, cust_char_t** string_pt, BOOL lock_on_fail);
RET_TYPE custom_fs_read_from_flash(uint8_t* datap, custom_fs_address_t address, uint32_t size);
void custom_fs_write_256B_at_internal_custom_storage_slot(uint32_t slot_id, void* array);
void custom_fs_read_256B_at_internal_custom_storage_slot(uint32_t slot_id, void* array);
RET_TYPE custom_fs_get_cpz_lut_entry(uint8_t* cpz, cpz_lut_entry_t** cpz_entry_pt);
uint16_t custom_fs_get_nb_free_cpz_lut_entries(uint8_t* first_available_user_id);
RET_TYPE custom_fs_update_cpz_entry(cpz_lut_entry_t* cpz_entry, uint8_t user_id);
RET_TYPE custom_fs_store_cpz_entry(cpz_lut_entry_t* cpz_entry, uint8_t user_id);
void custom_fs_erase_256B_at_internal_custom_storage_slot(uint32_t slot_id);
void custom_fs_define_powered_off_due_to_battery_voltage(BOOL set_flag);
void custom_fs_set_dataflash_descriptor(spi_flash_descriptor_t* desc);
uint8_t custom_fs_settings_get_device_setting(uint16_t setting_id);
uint32_t custom_fs_get_custom_storage_slot_addr(uint32_t slot_id);
RET_TYPE custom_fs_compute_and_check_external_bundle_crc32(void);
ret_type_te custom_fs_set_current_language(uint8_t language_id);
void custom_fs_settings_store_dump(uint8_t* settings_buffer);
cust_char_t* custom_fs_get_current_language_text_desc(void);
BOOL custom_fs_is_powered_off_due_to_battery_voltage(void);
uint16_t custom_fs_settings_get_dump(uint8_t* dump_buffer);
void custom_fs_detele_user_cpz_lut_entry(uint8_t user_id);
custom_fs_init_ret_type_te custom_fs_settings_init(void);
void custom_fs_stop_continuous_read_from_flash(void);
BOOL custom_fs_settings_check_fw_upgrade_flag(void);
void custom_fs_settings_clear_first_boot_flag(void);
void custom_fs_settings_clear_fw_upgrade_flag(void);
void custom_fs_settings_set_fw_upgrade_flag(void);
uint32_t custom_fs_get_number_of_languages(void);
uint8_t custom_fs_get_current_language_id(void);
void custom_fs_settings_set_defaults(void);
BOOL custom_fs_is_first_boot(void);
ret_type_te custom_fs_init(void);

/* Global vars, for debug only */
#if defined(DEBUG_MENU_ENABLED)
    extern custom_file_flash_header_t custom_fs_flash_header;
    extern language_map_entry_t custom_fs_cur_language_entry;
#endif   

#endif /* CUSTOM_FS_H_ */
