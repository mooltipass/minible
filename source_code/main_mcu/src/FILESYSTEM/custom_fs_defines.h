/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2020 Stephan Mathieu
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*!  \file     custom_fs_defines.h
*    \brief    Defines for our custom file system
*    Created:  20/05/2020
*    Author:   Mathieu Stephan
*/
#ifndef CUSTOM_FS_DEFINES_H_
#define CUSTOM_FS_DEFINES_H_

/* Includes */
#include <asf.h>
#include "defines.h"
#include "logic_power.h"

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

/* HID defines */
#define KEY_RETURN                          0x28
#define KEY_TAB                             0x2B

/* Fields sizes */
#define CUSTOM_FS_KEYBOARD_DESC_LGTH        20
#define CUSTOM_FS_KEYB_NB_INT_DESCRIBED     20

/* Settings IDs */
#define NB_DEVICE_SETTINGS                  64
#define SETTING_RESERVED_ID                 0
#define SETTING_RANDOM_PIN_ID               1
#define SETTING_USER_INTERACTION_TIMEOUT_ID 2
#define SETTING_FLASH_SCREEN_ID             3
#define SETTING_DEVICE_DEFAULT_LANGUAGE     4
#define SETTINGS_CHAR_AFTER_LOGIN_PRESS     5
#define SETTINGS_CHAR_AFTER_PASS_PRESS      6
#define SETTINGS_DELAY_BETWEEN_PRESSES      7
#define SETTINGS_BOOT_ANIMATION             8
#define SETTINGS_MASTER_CURRENT             9
#define SETTINGS_LOCK_ON_DISCONNECT         10
#define SETTINGS_KNOCK_DETECT_SENSITIVITY   11
#define SETTINGS_LEFT_HANDED_ON_BATTERY     12
#define SETTINGS_LEFT_HANDED_ON_USB         13
#define SETTINGS_PIN_SHOWN_WHEN_BACK        14
#define SETTINGS_UNLOCK_FEATURE_PARAM       15
#define SETTINGS_DEVICE_TUTORIAL            16
#define SETTINGS_SHOW_PIN_ON_ENTRY          17
#define SETTINGS_DISABLE_BLE_ON_CARD_REMOVE 18
#define SETTINGS_DISABLE_BLE_ON_LOCK        19
#define SETTINGS_NB_20MINS_TICKS_FOR_LOCK   20
#define SETTINGS_SWITCH_OFF_AFTER_USB_DISC  21
#define SETTINGS_HASH_DISPLAY_FEATURE       22
#define SETTINGS_INFORMATION_TIME_DELAY     23
#define SETTINGS_BLUETOOTH_SHORTCUTS        24
#define SETTINGS_SCREEN_SAVER_ID            25
/* Set to define the number of settings used */
#define SETTINGS_NB_USED                    26

/* Flags IDs */
#define NB_DEVICE_FLAGS                     32
#define FLAG_SET_BOOL_VALUE                 0x1212
typedef enum {PWR_OFF_DUE_TO_BATTERY_FLG_ID = 0, FUNCTIONAL_TEST_PASSED_FLAG_ID = 1, DEVICE_WENT_THROUGH_BOOTLOADER_FLAG_ID = 2, NOT_FIRST_BOOT_FLAG_ID = 3, RF_TESTING_PASSED_FLAG_ID = 4, SUCCESSFUL_UPDATE_FLAG_ID = 5} custom_fs_flag_id_te;

/* Typedefs */
#define CUSTOM_FS_ADDRESS_TMAX              UINT32_MAX
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
// signing key update bool: boolean to define if the signing key should be updated
// encrypted new signing key: this
// bundle version: the bundle version number
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
    uint8_t reserved[4];
    uint8_t signed_hash[16];
    uint16_t signing_key_update_bool;
    uint8_t encrypted_new_signing_key[AES_KEY_LENGTH/8];
    uint16_t bundle_version;
    uint8_t available_for_future_use[8];
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

// A struct contained in the last row of the bootloader section
typedef struct
{
    uint8_t bundle_signing_key[AES_KEY_LENGTH/8];
    uint8_t device_operations_key[AES_KEY_LENGTH/8];
    uint8_t available_signing_key2[AES_KEY_LENGTH/8];
    uint8_t available_signing_key3[AES_KEY_LENGTH/8];
    uint32_t platform_serial_number;
    uint8_t platform_ble_mac_addr[6];
    uint16_t current_bundle_version;
    uint8_t available_data[116];
} platform_unique_data_t;

typedef struct
{
    union
    {
        uint16_t row_data[NVMCTRL_ROW_SIZE/2];
        platform_unique_data_t platform_unique_data;
    };
} bl_section_last_row_t;

// Platform settings: do not store anything critical here in case of glitching
typedef struct  
{
    uint8_t device_settings[NB_DEVICE_SETTINGS];
    uint32_t reserved_uint32;
    uint32_t nb_settings_last_covered;
    uint32_t other_reserved_uint32;
    power_consumption_log_t power_log;
    uint8_t reserved_array[116];
    uint32_t device_auth_challenge_counter;
    uint8_t bluetooth_addr_padding[2];
    uint8_t dbg_bluetooth_addr[6];
    uint32_t start_upgrade_flag;
} custom_platform_settings_t;

// Platform flags
typedef struct
{
    uint16_t device_flags[NB_DEVICE_FLAGS];
    uint8_t reserved[192];
} custom_platform_flags_t;

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

#endif /* CUSTOM_FS_DEFINES_H_ */
