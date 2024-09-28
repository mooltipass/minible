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
/*!  \file     oled_wrapper.h
*    \brief    Wrapper functions for OLED screens
*    Created:  15/09/2024
*    Author:   Mathieu Stephan
*/


#ifndef OLED_WRAPPER_H_
#define OLED_WRAPPER_H_

#include "platform_defines.h"

/* Defines */

/* Function defines */
#ifndef MINIBLE_V2
    #include "sh1122.h"
    #define OLED_WIDTH  SH1122_OLED_WIDTH
    #define OLED_HEIGHT SH1122_OLED_HEIGHT
    
    #define oled_display_bitmap_from_flash_at_recommended_position    sh1122_display_bitmap_from_flash_at_recommended_position
    #define oled_erase_screen_and_put_top_left_emergency_string       sh1122_erase_screen_and_put_top_left_emergency_string
    #define oled_get_number_of_printable_characters_for_string        sh1122_get_number_of_printable_characters_for_string
    #define oled_get_start_x_for_string_based_on_alignment            sh1122_get_start_x_for_string_based_on_alignment
    #define oled_draw_full_screen_image_from_bitstream                sh1122_draw_full_screen_image_from_bitstream
    #define oled_add_emergency_dot_to_current_position                sh1122_add_emergency_dot_to_current_position
    #define oled_display_horizontal_pixel_line                        sh1122_display_horizontal_pixel_line
    #define oled_set_discharge_charge_periods                         sh1122_set_discharge_charge_periods
    #define oled_prevent_partial_text_y_draw                          sh1122_prevent_partial_text_y_draw
    #define oled_prevent_partial_text_x_draw                          sh1122_prevent_partial_text_x_draw
    #define oled_draw_image_from_bitstream                            sh1122_draw_image_from_bitstream
    #define oled_display_bitmap_from_flash                            sh1122_display_bitmap_from_flash
    #define oled_allow_partial_text_y_draw                            sh1122_allow_partial_text_y_draw
    #define oled_allow_partial_text_x_draw                            sh1122_allow_partial_text_x_draw
    #define oled_get_current_font_height                              sh1122_get_current_font_height
    #define oled_set_discharge_vsl_level                              sh1122_set_discharge_vsl_level
    #define oled_move_display_start_line                              sh1122_move_display_start_line
    #define oled_clear_current_screen                                 sh1122_clear_current_screen
    #define oled_write_single_command                                 sh1122_write_single_command
    #define oled_set_contrast_current                                 sh1122_set_contrast_current
    #define oled_put_centered_string                                  sh1122_put_centered_string
    #define oled_reset_lim_display_y                                  sh1122_reset_lim_display_y
    #define oled_set_emergency_font                                   sh1122_set_emergency_font
    #define oled_start_data_sending                                   sh1122_start_data_sending
    #define oled_is_screen_inverted                                   sh1122_is_screen_inverted
    #define oled_draw_vertical_line                                   sh1122_draw_vertical_line
    #define oled_fade_into_darkness                                   sh1122_fade_into_darkness
    #define oled_set_column_address                                   sh1122_set_column_address
    #define oled_put_centered_char                                    sh1122_put_centered_char
    #define oled_set_screen_invert                                    sh1122_set_screen_invert
    #define oled_refresh_used_font                                    sh1122_refresh_used_font
    #define oled_set_colors_invert                                    sh1122_set_colors_invert
    #define oled_set_max_display_y                                    sh1122_set_max_display_y
    #define oled_set_min_display_y                                    sh1122_set_min_display_y
    #define oled_prevent_line_feed                                    sh1122_prevent_line_feed
    #define oled_stop_data_sending                                    sh1122_stop_data_sending
    #define oled_write_single_word                                    sh1122_write_single_word
    #define oled_write_single_data                                    sh1122_write_single_data
    #define oled_put_error_string                                     sh1122_put_error_string
    #define oled_get_string_width                                     sh1122_get_string_width
    #define oled_reset_max_text_x                                     sh1122_reset_max_text_x
    #define oled_reset_min_text_x                                     sh1122_reset_min_text_x
    #define oled_get_glyph_width                                      sh1122_get_glyph_width
    #define oled_load_transition                                      sh1122_load_transition
    #define oled_allow_line_feed                                      sh1122_allow_line_feed
    #define oled_set_row_address                                      sh1122_set_row_address
    #define oled_set_vsegm_level                                      sh1122_set_vsegm_level
    #define oled_set_vcomh_level                                      sh1122_set_vcomh_level
    #define oled_set_max_text_x                                       sh1122_set_max_text_x
    #define oled_set_min_text_x                                       sh1122_set_min_text_x
    #define oled_draw_rectangle                                       sh1122_draw_rectangle
    #define oled_put_string_xy                                        sh1122_put_string_xy
    #define oled_init_display                                         sh1122_init_display
    #define oled_fill_screen                                          sh1122_fill_screen
    #define oled_glyph_draw                                           sh1122_glyph_draw
    #define oled_put_string                                           sh1122_put_string
    #define oled_is_oled_on                                           sh1122_is_oled_on
    #define oled_put_char                                             sh1122_put_char
    #define oled_set_xy                                               sh1122_set_xy
    #define oled_off                                                  sh1122_oled_off
    #define oled_on                                                   sh1122_oled_on
    
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    #define oled_check_for_flush_and_terminate                        sh1122_check_for_flush_and_terminate
    #define oled_flush_frame_buffer_y_window                          sh1122_flush_frame_buffer_y_window
    #define oled_flush_frame_buffer_window                            sh1122_flush_frame_buffer_window
    #define oled_clear_y_frame_buffer                                 sh1122_clear_y_frame_buffer
    #define oled_flush_frame_buffer                                   sh1122_flush_frame_buffer
    #define oled_clear_frame_buffer                                   sh1122_clear_frame_buffer
    #endif
    
    #ifdef OLED_PRINTF_ENABLED
    #define oled_printf_xy                                            sh1122_printf_xy
    #endif
#else
    #include "ssd1363.h"
    #define OLED_WIDTH  SSD1363_OLED_WIDTH
    #define OLED_HEIGHT SSD1363_OLED_HEIGHT
    
    #define oled_display_bitmap_from_flash_at_recommended_position    ssd1363_display_bitmap_from_flash_at_recommended_position
    #define oled_erase_screen_and_put_top_left_emergency_string       ssd1363_erase_screen_and_put_top_left_emergency_string
    #define oled_get_number_of_printable_characters_for_string        ssd1363_get_number_of_printable_characters_for_string
    #define oled_get_start_x_for_string_based_on_alignment            ssd1363_get_start_x_for_string_based_on_alignment
    #define oled_draw_full_screen_image_from_bitstream                ssd1363_draw_full_screen_image_from_bitstream
    #define oled_add_emergency_dot_to_current_position                ssd1363_add_emergency_dot_to_current_position
    #define oled_display_horizontal_pixel_line                        ssd1363_display_horizontal_pixel_line
    #define oled_set_discharge_charge_periods                         ssd1363_set_discharge_charge_periods
    #define oled_prevent_partial_text_y_draw                          ssd1363_prevent_partial_text_y_draw
    #define oled_prevent_partial_text_x_draw                          ssd1363_prevent_partial_text_x_draw
    #define oled_draw_image_from_bitstream                            ssd1363_draw_image_from_bitstream
    #define oled_display_bitmap_from_flash                            ssd1363_display_bitmap_from_flash
    #define oled_allow_partial_text_y_draw                            ssd1363_allow_partial_text_y_draw
    #define oled_allow_partial_text_x_draw                            ssd1363_allow_partial_text_x_draw
    #define oled_get_current_font_height                              ssd1363_get_current_font_height
    //#define oled_set_discharge_vsl_level                              ssd1363_set_discharge_vsl_level
    #define oled_move_display_start_line                              ssd1363_move_display_start_line
    #define oled_clear_current_screen                                 ssd1363_clear_current_screen
    #define oled_write_single_command                                 ssd1363_write_single_command
    #define oled_set_contrast_current                                 ssd1363_set_contrast_current
    #define oled_put_centered_string                                  ssd1363_put_centered_string
    #define oled_reset_lim_display_y                                  ssd1363_reset_lim_display_y
    #define oled_set_emergency_font                                   ssd1363_set_emergency_font
    #define oled_start_data_sending                                   ssd1363_start_data_sending
    #define oled_is_screen_inverted                                   ssd1363_is_screen_inverted
    #define oled_draw_vertical_line                                   ssd1363_draw_vertical_line
    #define oled_fade_into_darkness                                   ssd1363_fade_into_darkness
    #define oled_set_column_address                                   ssd1363_set_column_address
    #define oled_put_centered_char                                    ssd1363_put_centered_char
    #define oled_set_screen_invert                                    ssd1363_set_screen_invert
    #define oled_refresh_used_font                                    ssd1363_refresh_used_font
    #define oled_set_colors_invert                                    ssd1363_set_colors_invert
    #define oled_set_max_display_y                                    ssd1363_set_max_display_y
    #define oled_set_min_display_y                                    ssd1363_set_min_display_y
    #define oled_prevent_line_feed                                    ssd1363_prevent_line_feed
    #define oled_stop_data_sending                                    ssd1363_stop_data_sending
    #define oled_write_single_word                                    ssd1363_write_single_word
    #define oled_write_single_data                                    ssd1363_write_single_data
    #define oled_put_error_string                                     ssd1363_put_error_string
    #define oled_get_string_width                                     ssd1363_get_string_width
    #define oled_reset_max_text_x                                     ssd1363_reset_max_text_x
    #define oled_reset_min_text_x                                     ssd1363_reset_min_text_x
    #define oled_get_glyph_width                                      ssd1363_get_glyph_width
    #define oled_load_transition                                      ssd1363_load_transition
    #define oled_allow_line_feed                                      ssd1363_allow_line_feed
    #define oled_set_row_address                                      ssd1363_set_row_address
    #define oled_set_vsegm_level                                      ssd1363_set_vsegm_level
    #define oled_set_vcomh_level                                      ssd1363_set_vcomh_level
    #define oled_set_max_text_x                                       ssd1363_set_max_text_x
    #define oled_set_min_text_x                                       ssd1363_set_min_text_x
    #define oled_draw_rectangle                                       ssd1363_draw_rectangle
    #define oled_put_string_xy                                        ssd1363_put_string_xy
    #define oled_init_display                                         ssd1363_init_display
    #define oled_fill_screen                                          ssd1363_fill_screen
    #define oled_glyph_draw                                           ssd1363_glyph_draw
    #define oled_put_string                                           ssd1363_put_string
    #define oled_is_oled_on                                           ssd1363_is_oled_on
    #define oled_put_char                                             ssd1363_put_char
    #define oled_set_xy                                               ssd1363_set_xy
    #define oled_off                                                  ssd1363_oled_off
    #define oled_on                                                   ssd1363_oled_on
    
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    #define oled_check_for_flush_and_terminate                        ssd1363_check_for_flush_and_terminate
    #define oled_flush_frame_buffer_window                            ssd1363_flush_frame_buffer_window
    #define oled_flush_frame_buffer                                   ssd1363_flush_frame_buffer
    #define oled_clear_frame_buffer                                   ssd1363_clear_frame_buffer
    #endif
    
    #ifdef OLED_PRINTF_ENABLED
    #define oled_printf_xy                                            ssd1363_printf_xy
    #endif
#endif


#endif /* OLED_WRAPPER_H_ */