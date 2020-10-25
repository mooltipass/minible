/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2019 Stephan Mathieu
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
/*
 * custom_fs.h
 *
 * Created: 16/05/2017 11:39:40
 *  Author: stephan
 */ 


#ifndef CUSTOM_FS_H_
#define CUSTOM_FS_H_

#include "custom_fs_defines.h"
#include "platform_defines.h"
#include "defines.h"

/* Defines */
#define START_OF_SIGNED_DATA_IN_DATA_FLASH    (CUSTOM_FS_FILES_ADDR_OFFSET + offsetof(custom_file_flash_header_t, signing_key_update_bool))

/* Prototypes */
ret_type_te custom_fs_get_keyboard_symbols_for_unicode_string(cust_char_t* string_pt, uint16_t* buffer, BOOL usb_layout);
RET_TYPE custom_fs_continuous_read_from_flash(uint8_t* datap, custom_fs_address_t address, uint32_t size, BOOL use_dma);
RET_TYPE custom_fs_get_file_address(uint32_t file_id, custom_fs_address_t* address, custom_fs_file_type_te file_type);
void custom_fs_get_other_data_from_continuous_read_from_flash(uint8_t* datap, uint32_t size, BOOL use_dma);
RET_TYPE custom_fs_get_string_from_file(uint32_t string_id, cust_char_t** string_pt, BOOL lock_on_fail);
ret_type_te custom_fs_get_keyboard_descriptor_string(uint8_t keyboard_id, cust_char_t* string_pt);
RET_TYPE custom_fs_read_from_flash(uint8_t* datap, custom_fs_address_t address, uint32_t size);
void custom_fs_define_nb_ms_since_last_full_charge(uint32_t nb_ms, uint16_t vbat_measurement);
ret_type_te custom_fs_get_language_description(uint8_t language_id, cust_char_t* string_pt);
void custom_fs_write_256B_at_internal_custom_storage_slot(uint32_t slot_id, void* array);
void custom_fs_read_256B_at_internal_custom_storage_slot(uint32_t slot_id, void* array);
void custom_fs_stop_continuous_read_from_flash(BOOL was_using_emergency_bundle_data);
ret_type_te custom_fs_set_current_keyboard_id(uint8_t keyboard_id, BOOL usb_layout);
RET_TYPE custom_fs_get_cpz_lut_entry(uint8_t* cpz, cpz_lut_entry_t** cpz_entry_pt);
uint16_t custom_fs_get_nb_free_cpz_lut_entries(uint8_t* first_available_user_id);
RET_TYPE custom_fs_update_cpz_entry(cpz_lut_entry_t* cpz_entry, uint8_t user_id);
RET_TYPE custom_fs_store_cpz_entry(cpz_lut_entry_t* cpz_entry, uint8_t user_id);
void custom_fs_set_device_flag_value(custom_fs_flag_id_te flag_id, BOOL value);
void custom_fs_set_settings_value(uint8_t settings_id, uint8_t setting_value);
void custom_fs_erase_256B_at_internal_custom_storage_slot(uint32_t slot_id);
custom_file_flash_header_t* custom_fs_get_buffered_flash_header_pt(void);
RET_TYPE custom_fs_get_user_id_for_cpz(uint8_t* cpz, uint8_t* user_id);
void custom_fs_set_dataflash_descriptor(spi_flash_descriptor_t* desc);
custom_fs_address_t custom_fs_get_start_address_of_signed_data(void);
uint8_t custom_fs_get_recommended_layout_for_current_language(void);
uint16_t custom_fs_get_last_vbat_measurement_before_poweroff(void);
BOOL custom_fs_get_device_flag_value(custom_fs_flag_id_te flag_id);
uint8_t custom_fs_settings_get_device_setting(uint16_t setting_id);
RET_TYPE custom_fs_compute_and_check_external_bundle_crc32(void);
ret_type_te custom_fs_set_current_language(uint8_t language_id);
void custom_fs_set_device_default_language(uint8_t language_id);
void* custom_fs_get_custom_storage_slot_ptr(uint32_t slot_id);
void custom_fs_settings_store_dump(uint8_t* settings_buffer);
cust_char_t* custom_fs_get_current_language_text_desc(void);
uint16_t custom_fs_settings_get_dump(uint8_t* dump_buffer);
void custom_fs_detele_user_cpz_lut_entry(uint8_t user_id);
uint32_t custom_fs_get_nb_ms_since_last_full_charge(void);
custom_fs_init_ret_type_te custom_fs_settings_init(void);
uint8_t custom_fs_get_current_layout_id(BOOL usb_layout);
BOOL custom_fs_settings_check_fw_upgrade_flag(void);
void custom_fs_settings_clear_fw_upgrade_flag(void);
uint32_t custom_fs_get_number_of_keyb_layouts(void);
void custom_fs_get_debug_bt_addr(uint8_t* bt_addr);
void custom_fs_settings_set_fw_upgrade_flag(void);
uint32_t custom_fs_get_number_of_languages(void);
uint8_t custom_fs_get_current_language_id(void);
void custom_fs_set_undefined_settings(void);
void custom_fs_hard_reset_settings(void);
ret_type_te custom_fs_init(void);

/* Global vars, for debug only */
#if defined(DEBUG_MENU_ENABLED)
    extern custom_file_flash_header_t custom_fs_flash_header;
    extern language_map_entry_t custom_fs_cur_language_entry;
#endif   

#endif /* CUSTOM_FS_H_ */
