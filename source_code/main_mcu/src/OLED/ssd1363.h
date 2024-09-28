/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2024 Stephan Mathieu
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
/*!  \file     ssd1363.h
*    \brief    SSD1363 OLED driver
*    Created:  24/07/2024
*    Author:   Mathieu Stephan
*/


#ifndef SSD1363_H_
#define SSD1363_H_

#include "platform_defines.h"
#ifndef MINIBLE_V1

#include <asf.h>
#include "custom_bitstream.h"
#include "custom_fs.h"


/* Command defines */
#define SSD1363_CMD_SET_DISPLAY_OFF                      0xAE
#define SSD1363_SET_COMMAND_LOCK                         0xFD
#define SSD1363_CMD_SET_SEGMENT_REMAP                    0xA0
#define SSD1363_CMD_SET_DISPLAY_START_LINE               0xA1
#define SSD1363_CMD_SET_DISPLAY_OFFSET                   0xA2
#define SSD1363_CMD_SET_NORMAL_DISPLAY                   0xA6
#define SSD1363_CMD_SET_INVERSE_DISPLAY                  0xA7
#define SSD1363_CMD_SET_DISCHARGE_PRECHARGE_PERIOD       0xB1
#define SSD1363_CMD_SET_CLOCK_DIVIDER_AND_OSC_FREQ       0xB3
#define SSD1363_CMD_SET_SECOND_PRECHARGE_PERIOD          0xB6
#define SSD1363_CMD_SET_DEFAULT_LINEAR_GRAY_SCALE        0xB9
#define SSD1363_CMD_SET_PRECHARGE_VOLTAGE                0xBB
#define SSD1363_CMD_SET_VCOM_DESELECT_LEVEL              0xBE
#define SSD1363_CMD_SET_CONTRAST_CURRENT                 0xC1
#define SSD1363_CMD_SET_MULTIPLEX_RATIO                  0xCA
#define SSD1363_CMD_SET_DISPLAY_ON                       0xAF
#define SSD1363_CMD_SET_ROW_ADDR                         0x75
#define SSD1363_CMD_SET_COLUMN_ADDR                      0x15
#define SSD1363_CMD_WRITE_RAM                            0x5C

/* Screen defines */
#define SSD1363_RAM_X_PIXELS         320
#define SSD1363_RAM_Y_PIXELS         160
#define SSD1363_OLED_WIDTH           264
#define SSD1363_OLED_HEIGHT          96
#define SSD1363_DISPLAY_X_OFFSET     7
#define SSD1363_OLED_BPP             4
#define SSD1363_OLED_PPB             (8/SSD1363_OLED_BPP)
#define SSD1363_OLED_PIX_PER_COL     4

/* Transition defines */
#define SSD1363_TRANSITION_PIXEL     0x03

/* Enums */
typedef enum {OLED_TRANS_NONE, OLED_LEFT_RIGHT_TRANS, OLED_RIGHT_LEFT_TRANS, OLED_TOP_BOT_TRANS, OLED_BOT_TOP_TRANS, OLED_IN_OUT_TRANS, OLED_OUT_IN_TRANS} oled_transition_te;
typedef enum {OLED_SCROLL_NONE = 0, OLED_SCROLL_UP = 1, OLED_SCROLL_DOWN = 2, OLED_SCROLL_FLIP = 3} oled_scroll_te;
typedef enum {OLED_ALIGN_LEFT = 0, OLED_ALIGN_RIGHT = 1, OLED_ALIGN_CENTER = 2} oled_align_te;

/* Structs */
// pixel buffer to allow merging of adjacent image data.
// To conserve memory, only one GDDRAM word is kept per display line.
// The library assumes the display will be filled left to right, and
// hence it only needs to merge the rightmost data with the next write
// on that line.
/* Solution compiled with -Wpacked */
typedef struct
{
    int16_t xaddr;
    uint16_t pixels;
} gddram_px_t;

typedef struct
{
    Sercom* sercom_pt;
    uint16_t dma_trigger_id;
    pin_group_te cs_pin_group;
    PIN_MASK_T cs_pin_mask;
    pin_group_te cd_pin_group;
    PIN_MASK_T cd_pin_mask;
    gddram_px_t gddram_pixel[SSD1363_OLED_HEIGHT];       // Buffer to merge adjascent pixels
    custom_fs_address_t currentFontAddress;             // Current font address
    font_header_t current_font_header;                  // Current font header
    unicode_interval_desc_t current_unicode_inters[15]; // Current unicode interval descriptors
    BOOL question_mark_support_described;               // If this font describes '?' support
    #ifdef MINIBLE_V2_TO_TACKLE
    BOOL screen_wrapping_allowed;                       // If we are allowing screen wrapping
    #endif
    BOOL carriage_return_allowed;                       // If we are allowing \r
    BOOL line_feed_allowed;                             // If we are allowing \n
    BOOL allow_text_partial_y_draw;                     // Allow drawing of text if they go over max Y
    BOOL allow_text_partial_x_draw;                     // Allow drawing of text if they go over max X
    BOOL screen_inverted;                               // If the screen is inverted
    uint16_t min_disp_y;                                // Min display Y
    uint16_t max_disp_y;                                // Max display Y
    uint16_t max_disp_x;                                // Max display X
    int16_t min_text_x;                                 // Min text X
    int16_t max_text_x;                                 // Max text X
    oled_align_te new_line_justify;                     // Justification for new lines
    int16_t new_line_x;                                 // X position when return occurs
    int16_t cur_text_x;                                 // Current x for writing text
    int16_t cur_text_y;                                 // Current y for writing text
    BOOL oled_on;                                       // Know if oled is on
    oled_transition_te loaded_transition;               // Loaded transition for full frame switch
    uint16_t onscreen_log_entry_ind;                    // Index to the next available log entry
    const cust_char_t* onscreen_log_entry_strings[7];          // Array containing pointers to the log strings currently displayed
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    union
    {
        uint8_t frame_buffer[SSD1363_OLED_HEIGHT][SSD1363_OLED_WIDTH/(8/SSD1363_OLED_BPP)];
        uint16_t frame_buffer_16b[SSD1363_OLED_HEIGHT][SSD1363_OLED_WIDTH/(16/SSD1363_OLED_BPP)];
    };
    BOOL frame_buffer_flush_in_progress;
    #endif
} oled_descriptor_t;

/* Prototypes */
int16_t ssd1363_put_string_xy(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, oled_align_te justify, const cust_char_t* string, BOOL write_to_buffer);
void ssd1363_draw_rectangle(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color, BOOL write_to_buffer);
int16_t ssd1363_get_start_x_for_string_based_on_alignment(oled_descriptor_t* oled_descriptor, int16_t x, oled_align_te justify, const cust_char_t* string);
void ssd1363_draw_image_from_bitstream(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_bitmap_t* bitstream, BOOL write_to_buffer);
void ssd1363_draw_vertical_line(oled_descriptor_t* oled_descriptor, int16_t x, int16_t ystart, int16_t yend, uint8_t color, BOOL write_to_buffer);
RET_TYPE ssd1363_display_bitmap_from_flash_at_recommended_position(oled_descriptor_t* oled_descriptor, uint32_t file_id, BOOL write_to_buffer);
RET_TYPE ssd1363_display_bitmap_from_flash(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, uint32_t file_id, BOOL write_to_buffer);
void ssd1363_init_display(oled_descriptor_t* oled_descriptor, BOOL leave_internal_logic_and_reflush_frame_buffer, uint8_t master_current);
uint16_t ssd1363_get_number_of_printable_characters_for_string(oled_descriptor_t* oled_descriptor, int16_t x, const cust_char_t* string);
uint16_t ssd1363_put_centered_string(oled_descriptor_t* oled_descriptor, uint8_t y, const cust_char_t* string, BOOL write_to_buffer);
void ssd1363_write_single_command_with_two_data(oled_descriptor_t* oled_descriptor, uint8_t command, uint8_t data, uint8_t data2);
void ssd1363_put_centered_char(oled_descriptor_t* oled_descriptor, int16_t x, uint16_t y, cust_char_t c, BOOL write_to_buffer);
uint16_t ssd1363_glyph_draw(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, cust_char_t ch, BOOL write_to_buffer);
void ssd1363_erase_screen_and_put_top_left_emergency_string(oled_descriptor_t* oled_descriptor, const cust_char_t* string);
void ssd1363_draw_full_screen_image_from_bitstream(oled_descriptor_t* oled_descriptor, bitstream_bitmap_t* bitstream);
void ssd1363_write_single_command_with_data(oled_descriptor_t* oled_descriptor, uint8_t command, uint8_t data);
void ssd1363_set_column_address_without_offset(oled_descriptor_t* oled_descriptor, uint8_t start, uint8_t end);
int16_t ssd1363_put_string(oled_descriptor_t* oled_descriptor, const cust_char_t* str, BOOL write_to_buffer);
uint16_t ssd1363_get_glyph_width(oled_descriptor_t* oled_descriptor, cust_char_t ch, uint16_t* glyph_height);
void ssd1363_add_on_screen_entry_log_entry(oled_descriptor_t* oled_descriptor, const cust_char_t* string);
void ssd1363_fade_into_darkness(oled_descriptor_t* oled_descriptor, oled_transition_te transition);
int16_t ssd1363_put_char(oled_descriptor_t* oled_descriptor, cust_char_t ch, BOOL write_to_buffer);
uint16_t ssd1363_put_error_string(oled_descriptor_t* oled_descriptor, const cust_char_t* string);
void ssd1363_set_contrast_current(oled_descriptor_t* oled_descriptor, uint8_t contrast_current);
void ssd1363_set_column_address(oled_descriptor_t* oled_descriptor, uint8_t start, uint8_t end);
void ssd1363_load_transition(oled_descriptor_t* oled_descriptor, oled_transition_te transition);
void ssd1363_set_discharge_charge_periods(oled_descriptor_t* oled_descriptor, uint8_t periods);
uint16_t ssd1363_get_string_width(oled_descriptor_t* oled_descriptor, const cust_char_t* str);
void ssd1363_set_row_address(oled_descriptor_t* oled_descriptor, uint8_t start, uint8_t end);
void ssd1363_set_screen_invert(oled_descriptor_t* oled_descriptor, BOOL screen_inverted);
void ssd1363_move_display_start_line(oled_descriptor_t* oled_descriptor, int16_t offset);
RET_TYPE ssd1363_refresh_used_font(oled_descriptor_t* oled_descriptor, uint16_t font_id);
void ssd1363_set_colors_invert(oled_descriptor_t* oled_descriptor, BOOL colors_inverted);
void ssd1363_add_emergency_dot_to_current_position(oled_descriptor_t* oled_descriptor);
void ssd1363_write_single_command(oled_descriptor_t* oled_descriptor, uint8_t command);
void ssd1363_set_vcomh_level(oled_descriptor_t* oled_descriptor, uint8_t vcomh);
void ssd1363_set_max_display_y(oled_descriptor_t* oled_descriptor, uint16_t y);
void ssd1363_set_min_display_y(oled_descriptor_t* oled_descriptor, uint16_t y);
void ssd1363_set_xy(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y);
void ssd1363_fill_screen(oled_descriptor_t* oled_descriptor, uint16_t color);
void ssd1363_set_max_text_x(oled_descriptor_t* oled_descriptor, int16_t x);
void ssd1363_set_min_text_x(oled_descriptor_t* oled_descriptor, int16_t x);
void ssd1363_prevent_partial_text_y_draw(oled_descriptor_t* oled_descriptor);
void ssd1363_prevent_partial_text_x_draw(oled_descriptor_t* oled_descriptor);
uint8_t ssd1363_get_current_font_height(oled_descriptor_t* oled_descriptor);
void ssd1363_allow_partial_text_y_draw(oled_descriptor_t* oled_descriptor);
void ssd1363_allow_partial_text_x_draw(oled_descriptor_t* oled_descriptor);
void ssd1363_clear_current_screen(oled_descriptor_t* oled_descriptor);
void ssd1363_reset_lim_display_y(oled_descriptor_t* oled_descriptor);
void ssd1363_set_emergency_font(oled_descriptor_t* oled_descriptor);
void ssd1363_start_data_sending(oled_descriptor_t* oled_descriptor);
BOOL ssd1363_is_screen_inverted(oled_descriptor_t* oled_descriptor);
void ssd1363_prevent_line_feed(oled_descriptor_t* oled_descriptor);
void ssd1363_stop_data_sending(oled_descriptor_t* oled_descriptor);
void ssd1363_reset_max_text_x(oled_descriptor_t* oled_descriptor);
void ssd1363_reset_min_text_x(oled_descriptor_t* oled_descriptor);
void ssd1363_allow_line_feed(oled_descriptor_t* oled_descriptor);
BOOL ssd1363_is_oled_on(oled_descriptor_t* oled_descriptor);
void ssd1363_oled_off(oled_descriptor_t* oled_descriptor);
void ssd1363_oled_on(oled_descriptor_t* oled_descriptor);

/* Depending on enabled features */
#ifdef OLED_INTERNAL_FRAME_BUFFER
void ssd1363_flush_frame_buffer_window(oled_descriptor_t* oled_descriptor, uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y);
void ssd1363_check_for_flush_and_terminate(oled_descriptor_t* oled_descriptor);
void ssd1363_flush_frame_buffer(oled_descriptor_t* oled_descriptor);
void ssd1363_clear_frame_buffer(oled_descriptor_t* oled_descriptor);
#endif

/* ifdef prototypes */
#ifdef OLED_PRINTF_ENABLED
    uint16_t ssd1363_printf_xy(oled_descriptor_t* oled_descriptor, int16_t x, uint8_t y, oled_align_te justify, BOOL write_to_buffer, const char *fmt, ...);
#endif


#endif /* MINIBLE_V1 */
#endif /* SSD1363_H_ */
