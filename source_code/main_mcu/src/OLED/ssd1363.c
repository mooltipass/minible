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
/*!  \file     ssd1363.c
*    \brief    SSD1363 OLED driver
*    Created:  24/07/2024
*    Author:   Mathieu Stephan
*/
#include "platform_defines.h"
#ifndef MINIBLE_V1

#include <stdarg.h>
#include <string.h>
#include <asf.h>
#include "custom_bitstream.h"
#include "driver_sercom.h"
#include "driver_timer.h"
#include "custom_fs.h"
#include "ssd1363.h"
#include "dma.h"

#ifdef EMULATOR_BUILD
#include "../EMU/emu_oled.h"
#define emul_delay_ms(...)  timer_delay_ms(...)
#elif !defined(BOOTLOADER)
static void emu_oled_flush(void) {}
static void emul_delay_ms(uint16_t delay) {(void)delay;}
#endif

/* SSD1363 initialization sequence */
static const uint8_t ssd1363_init_sequence[] = 
{
    // Display frame frequency = Fosc / (D*K*Multiplex ratio) with Fosc = 1.365MHz, D = 2, K = Phase 1 + phase 2 periods (0xb1 command) + 72 = 8+15+72, MUX ratio = 96: 75Hz
    SSD1363_CMD_SET_DISPLAY_OFF,                 0,                 // Set Display Off
    SSD1363_SET_COMMAND_LOCK,                    1, 0x12,           // Unlock OLED driver IC MCU interface from entering command
    SSD1363_CMD_SET_SEGMENT_REMAP,               2, 0x32, 0x00,     // Set Segment Re-map: Enable COM Split Odd Even, Scan from COM[N-1] to COM0, Column address 79 is mapped to SEG0, Horizontal address increment, Disable Dual COM mode
    SSD1363_CMD_SET_DISPLAY_START_LINE,          1, 0,              // Set display start line
    SSD1363_CMD_SET_DISPLAY_OFFSET,              1, 0x40,           // Set display offset
    SSD1363_CMD_SET_NORMAL_DISPLAY,              0,                 // Display Bits Normally Interpreted
    SSD1363_CMD_SET_DISCHARGE_PRECHARGE_PERIOD,  1, 0x8F,           // Set Discharge/Precharge Period (4bits each)
    SSD1363_CMD_SET_CLOCK_DIVIDER_AND_OSC_FREQ,  1, 0xA1,           // Clock divider of 2, oscillator frequency of 1.365MHz (I guess?)
    SSD1363_CMD_SET_SECOND_PRECHARGE_PERIOD,     1, 0x08,           // Second pre-charge period of 8 DCLK
    SSD1363_CMD_SET_DEFAULT_LINEAR_GRAY_SCALE,   0,                 // Select Default Linear Gray Scale table
    SSD1363_CMD_SET_PRECHARGE_VOLTAGE,           1, 0x1F,           // Set precharge voltage to 0.5133xVcc (max)
    SSD1363_CMD_SET_VCOM_DESELECT_LEVEL,         1, 0x07,           // Set COM deselect voltage to 0.86*Vcc (max)
    SSD1363_CMD_SET_MULTIPLEX_RATIO,             1, 0x5F,           // Set multiplex ratio to 96
};
static const uint8_t ssd1363_init_sequence_inverted[] = 
{
    // Display frame frequency = Fosc / (D*K*Multiplex ratio) with Fosc = 1.365MHz, D = 2, K = Phase 1 + phase 2 periods (0xb1 command) + 72 = 8+15+72, MUX ratio = 96: 75Hz
    SSD1363_CMD_SET_DISPLAY_OFF,                 0,                 // Set Display Off
    SSD1363_SET_COMMAND_LOCK,                    1, 0x12,           // Unlock OLED driver IC MCU interface from entering command
    SSD1363_CMD_SET_SEGMENT_REMAP,               2, 0x20, 0x00,     // Set Segment Re-map: Enable COM Split Odd Even, Scan from COM0 to COM[N–1], Column address 0 is mapped to SEG0, Horizontal address increment, Disable Dual COM mode
    SSD1363_CMD_SET_DISPLAY_START_LINE,          1, 0,              // Set display start line
    SSD1363_CMD_SET_DISPLAY_OFFSET,              1, 0x60,           // Set display offset
    SSD1363_CMD_SET_NORMAL_DISPLAY,              0,                 // Display Bits Normally Interpreted
    SSD1363_CMD_SET_DISCHARGE_PRECHARGE_PERIOD,  1, 0x8F,           // Set Discharge/Precharge Period (4bits each)
    SSD1363_CMD_SET_CLOCK_DIVIDER_AND_OSC_FREQ,  1, 0xA1,           // Clock divider of 2, oscillator frequency of 1.365MHz (I guess?)
    SSD1363_CMD_SET_SECOND_PRECHARGE_PERIOD,     1, 0x08,           // Second pre-charge period of 8 DCLK
    SSD1363_CMD_SET_DEFAULT_LINEAR_GRAY_SCALE,   0,                 // Select Default Linear Gray Scale table
    SSD1363_CMD_SET_PRECHARGE_VOLTAGE,           1, 0x1F,           // Set precharge voltage to 0.5133xVcc (max)
    SSD1363_CMD_SET_VCOM_DESELECT_LEVEL,         1, 0x07,           // Set COM deselect voltage to 0.86*Vcc (max)
    SSD1363_CMD_SET_MULTIPLEX_RATIO,             1, 0x5F,           // Set multiplex ratio to 96
};


/*! \fn     ssd1363_write_single_command(oled_descriptor_t* oled_descriptor, uint8_t command)
*   \brief  Write a single command byte through SPI
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  command             Command to be sent
*/
void ssd1363_write_single_command(oled_descriptor_t* oled_descriptor, uint8_t command)
{
    PORT->Group[oled_descriptor->cs_pin_group].OUTCLR.reg = oled_descriptor->cs_pin_mask;
    PORT->Group[oled_descriptor->cd_pin_group].OUTCLR.reg = oled_descriptor->cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, command);
    PORT->Group[oled_descriptor->cs_pin_group].OUTSET.reg = oled_descriptor->cs_pin_mask;
}

/*! \fn     ssd1363_write_single_command_with_data(oled_descriptor_t* oled_descriptor, uint8_t command, uint8_t data)
*   \brief  Write a single command byte with accompanying data byte through SPI
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  command             Command to be sent
*   \param  data                Data byte
*/
void ssd1363_write_single_command_with_data(oled_descriptor_t* oled_descriptor, uint8_t command, uint8_t data)
{
    PORT->Group[oled_descriptor->cs_pin_group].OUTCLR.reg = oled_descriptor->cs_pin_mask;
    PORT->Group[oled_descriptor->cd_pin_group].OUTCLR.reg = oled_descriptor->cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, command);
    PORT->Group[oled_descriptor->cd_pin_group].OUTSET.reg = oled_descriptor->cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, data);
    PORT->Group[oled_descriptor->cs_pin_group].OUTSET.reg = oled_descriptor->cs_pin_mask;
}

/*! \fn     ssd1363_write_single_command_with_two_data(oled_descriptor_t* oled_descriptor, uint8_t command, uint8_t data, uint8_t data2)
*   \brief  Write a single command byte with accompanying 2 data bytes through SPI
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  command             Command to be sent
*   \param  data                Data byte
*   \param  data2               Second data byte
*/
void ssd1363_write_single_command_with_two_data(oled_descriptor_t* oled_descriptor, uint8_t command, uint8_t data, uint8_t data2)
{
    PORT->Group[oled_descriptor->cs_pin_group].OUTCLR.reg = oled_descriptor->cs_pin_mask;
    PORT->Group[oled_descriptor->cd_pin_group].OUTCLR.reg = oled_descriptor->cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, command);
    PORT->Group[oled_descriptor->cd_pin_group].OUTSET.reg = oled_descriptor->cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, data);
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, data2);
    PORT->Group[oled_descriptor->cs_pin_group].OUTSET.reg = oled_descriptor->cs_pin_mask;
}

/*! \fn     ssd1363_start_data_sending(oled_descriptor_t* oled_descriptor)
*   \brief  Start data sending mode: assert nCS & CD pin
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_start_data_sending(oled_descriptor_t* oled_descriptor)
{
    PORT->Group[oled_descriptor->cs_pin_group].OUTCLR.reg = oled_descriptor->cs_pin_mask;
    PORT->Group[oled_descriptor->cd_pin_group].OUTSET.reg = oled_descriptor->cd_pin_mask;    
}

/*! \fn     ssd1363_stop_data_sending(oled_descriptor_t* oled_descriptor)
*   \brief  Start data sending mode: de-assert nCS
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_stop_data_sending(oled_descriptor_t* oled_descriptor)
{
    PORT->Group[oled_descriptor->cs_pin_group].OUTSET.reg = oled_descriptor->cs_pin_mask;  
}

/*! \fn     ssd1363_set_contrast_current(oled_descriptor_t* oled_descriptor, uint8_t contrast_current)
*   \brief  Set contrast current for display
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  contrast_current    Contrast current (up to 0xFF)
*/
void ssd1363_set_contrast_current(oled_descriptor_t* oled_descriptor, uint8_t contrast_current)
{
    ssd1363_write_single_command_with_data(oled_descriptor, SSD1363_CMD_SET_CONTRAST_CURRENT, contrast_current);
}

/*! \fn     ssd1363_set_vcomh_level(oled_descriptor_t* oled_descriptor, uint8_t vcomh)
*   \brief  Set vcomh level
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  vcomh               VCOMH level (up to 0x07)
*/
void ssd1363_set_vcomh_level(oled_descriptor_t* oled_descriptor, uint8_t vcomh)
{
    ssd1363_write_single_command_with_data(oled_descriptor, SSD1363_CMD_SET_VCOM_DESELECT_LEVEL, vcomh); 
}

/*! \fn     ssd1363_set_discharge_charge_periods(oled_descriptor_t* oled_descriptor, uint8_t periods)
*   \brief  Set discharge & charge periods
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  periods             4 bits for discharge, 4 bits for charge
*/
void ssd1363_set_discharge_charge_periods(oled_descriptor_t* oled_descriptor, uint8_t periods)
{
    ssd1363_write_single_command_with_data(oled_descriptor, SSD1363_CMD_SET_DISCHARGE_PRECHARGE_PERIOD, periods);
}

/*! \fn     ssd1363_set_column_address_without_offset(oled_descriptor_t* oled_descriptor, uint8_t start, uint8_t end)
*   \brief  Set a selected column address range without adding the display x offset
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  start               Start column
*   \param  end                 End column
*/
void ssd1363_set_column_address_without_offset(oled_descriptor_t* oled_descriptor, uint8_t start, uint8_t end)
{
    PORT->Group[oled_descriptor->cs_pin_group].OUTCLR.reg = oled_descriptor->cs_pin_mask;
    PORT->Group[oled_descriptor->cd_pin_group].OUTCLR.reg = oled_descriptor->cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, SSD1363_CMD_SET_COLUMN_ADDR);
    PORT->Group[oled_descriptor->cd_pin_group].OUTSET.reg = oled_descriptor->cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, start);
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, end);
    PORT->Group[oled_descriptor->cs_pin_group].OUTSET.reg = oled_descriptor->cs_pin_mask;
}

/*! \fn     ssd1363_set_column_address(oled_descriptor_t* oled_descriptor, uint8_t start, uint8_t end)
*   \brief  Set a selected column address range
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  start               Start column
*   \param  end                 End column
*/
void ssd1363_set_column_address(oled_descriptor_t* oled_descriptor, uint8_t start, uint8_t end)
{
    PORT->Group[oled_descriptor->cs_pin_group].OUTCLR.reg = oled_descriptor->cs_pin_mask;
    PORT->Group[oled_descriptor->cd_pin_group].OUTCLR.reg = oled_descriptor->cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, SSD1363_CMD_SET_COLUMN_ADDR);
    PORT->Group[oled_descriptor->cd_pin_group].OUTSET.reg = oled_descriptor->cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, start+SSD1363_DISPLAY_X_OFFSET);
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, end+SSD1363_DISPLAY_X_OFFSET);
    PORT->Group[oled_descriptor->cs_pin_group].OUTSET.reg = oled_descriptor->cs_pin_mask;
}

/*! \fn     ssd1363_set_row_address(oled_descriptor_t* oled_descriptor, uint8_t start, uint8_t end)
*   \brief  Set a selected column address range
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  start               Start row
*/
void ssd1363_set_row_address(oled_descriptor_t* oled_descriptor, uint8_t start, uint8_t end)
{
    PORT->Group[oled_descriptor->cs_pin_group].OUTCLR.reg = oled_descriptor->cs_pin_mask;
    PORT->Group[oled_descriptor->cd_pin_group].OUTCLR.reg = oled_descriptor->cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, SSD1363_CMD_SET_ROW_ADDR);
    PORT->Group[oled_descriptor->cd_pin_group].OUTSET.reg = oled_descriptor->cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, start);
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, end);
    PORT->Group[oled_descriptor->cs_pin_group].OUTSET.reg = oled_descriptor->cs_pin_mask;
}

/*! \fn     ssd1363_move_display_start_line(oled_descriptor_t* oled_descriptor, int16_t offset)
*   \brief  Shift the display start line
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  offset              Y offset for shift
*/
void ssd1363_move_display_start_line(oled_descriptor_t* oled_descriptor, int16_t offset)
{
    PORT->Group[oled_descriptor->cs_pin_group].OUTCLR.reg = oled_descriptor->cs_pin_mask;
    PORT->Group[oled_descriptor->cd_pin_group].OUTCLR.reg = oled_descriptor->cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, SSD1363_CMD_SET_DISPLAY_START_LINE);
    PORT->Group[oled_descriptor->cd_pin_group].OUTSET.reg = oled_descriptor->cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, (uint8_t)offset);
    PORT->Group[oled_descriptor->cs_pin_group].OUTSET.reg = oled_descriptor->cs_pin_mask;
}

/*! \fn     ssd1363_is_oled_on(oled_descriptor_t* oled_descriptor)
*   \brief  Know if OLED is ON
*   \return A boolean
*/
BOOL ssd1363_is_oled_on(oled_descriptor_t* oled_descriptor)
{
    return oled_descriptor->oled_on;
}

/*! \fn     ssd1363_oled_off(oled_descriptor_t* oled_descriptor)
*   \brief  Switch on the screen
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_oled_off(oled_descriptor_t* oled_descriptor)
{
    ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_SET_DISPLAY_OFF);
    oled_descriptor->oled_on = FALSE;
}

/*! \fn     ssd1363_oled_on(oled_descriptor_t* oled_descriptor)
*   \brief  Switch on the screen
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_oled_on(oled_descriptor_t* oled_descriptor)
{
    ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_SET_DISPLAY_ON);
    oled_descriptor->oled_on = TRUE;
}

/*! \fn     ssd1363_load_transition(oled_descriptor_t* oled_descriptor, oled_transition_te transition)
*   \brief  Load transition when the next frame buffer flush occurs
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  transition          The transition
*/
void ssd1363_load_transition(oled_descriptor_t* oled_descriptor, oled_transition_te transition)
{
    oled_descriptor->loaded_transition = transition;
}

/*! \fn     ssd1363_fade_into_darkness(oled_descriptor_t* oled_descriptor, oled_transition_te transition)
*   \brief  Fade display into black
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  transition          The transition
*/
void ssd1363_fade_into_darkness(oled_descriptor_t* oled_descriptor, oled_transition_te transition)
{
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    ssd1363_load_transition(oled_descriptor, transition);
    ssd1363_clear_frame_buffer(oled_descriptor);
    ssd1363_flush_frame_buffer(oled_descriptor);
    #endif
}

/*! \fn     ssd1363_set_min_text_x(oled_descriptor_t* oled_descriptor, int16_t x)
*   \brief  Set maximum text X position
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  x                   Min text x
*/
void ssd1363_set_min_text_x(oled_descriptor_t* oled_descriptor, int16_t x)
{
    oled_descriptor->min_text_x = x;
}

/*! \fn     ssd1363_set_max_text_x(oled_descriptor_t* oled_descriptor, int16_t x)
*   \brief  Set maximum text X position
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  x                   Max text x
*/
void ssd1363_set_max_text_x(oled_descriptor_t* oled_descriptor, int16_t x)
{
    oled_descriptor->max_text_x = x;
}

/*! \fn     ssd1363_reset_min_text_x(oled_descriptor_t* oled_descriptor)
*   \brief  Reset minimum text X position
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_reset_min_text_x(oled_descriptor_t* oled_descriptor)
{
    oled_descriptor->min_text_x = 0;
}

/*! \fn     ssd1363_reset_max_text_x(oled_descriptor_t* oled_descriptor)
*   \brief  Reset maximum text X position
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_reset_max_text_x(oled_descriptor_t* oled_descriptor)
{
    oled_descriptor->max_text_x = SSD1363_OLED_WIDTH;
}

/*! \fn     ssd1363_set_min_display_y(oled_descriptor_t* oled_descriptor, uint16_t y)
*   \brief  Set minimum Y display
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  y                   Min y
*/
void ssd1363_set_min_display_y(oled_descriptor_t* oled_descriptor, uint16_t y)
{
    if (y > SSD1363_OLED_HEIGHT)
    {
        y = SSD1363_OLED_HEIGHT;
    }
    oled_descriptor->min_disp_y = y;
}

/*! \fn     ssd1363_set_max_display_y(oled_descriptor_t* oled_descriptor, uint16_t y)
*   \brief  Set maximum Y display
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  y                   Max y
*/
void ssd1363_set_max_display_y(oled_descriptor_t* oled_descriptor, uint16_t y)
{
    if (y > SSD1363_OLED_HEIGHT)
    {
        y = SSD1363_OLED_HEIGHT;
    }
    oled_descriptor->max_disp_y = y;
}

/*! \fn     ssd1363_reset_lim_display_y(oled_descriptor_t* oled_descriptor)
*   \brief  Reset min/max Y display
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_reset_lim_display_y(oled_descriptor_t* oled_descriptor)
{
    oled_descriptor->min_disp_y = 0;
    oled_descriptor->max_disp_y = SSD1363_OLED_HEIGHT;
}

/*! \fn     ssd1363_allow_line_feed(oled_descriptor_t* oled_descriptor)
*   \brief  Allow line feed
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_allow_line_feed(oled_descriptor_t* oled_descriptor)
{
    oled_descriptor->line_feed_allowed = TRUE;
}

/*! \fn     ssd1363_prevent_line_feed(oled_descriptor_t* oled_descriptor)
*   \brief  Prevent line feed
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_prevent_line_feed(oled_descriptor_t* oled_descriptor)
{
    oled_descriptor->line_feed_allowed = FALSE;
}

/*! \fn     ssd1363_allow_partial_text_y_draw(oled_descriptor_t* oled_descriptor)
*   \brief  Allow partial drawing of text in Y
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_allow_partial_text_y_draw(oled_descriptor_t* oled_descriptor)
{
    oled_descriptor->allow_text_partial_y_draw = TRUE;
}

/*! \fn     ssd1363_prevent_partial_text_x_draw(oled_descriptor_t* oled_descriptor)
*   \brief  Prevent partial drawing of text in X
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_prevent_partial_text_x_draw(oled_descriptor_t* oled_descriptor)
{
    oled_descriptor->allow_text_partial_x_draw = FALSE;
}

/*! \fn     ssd1363_allow_partial_text_x_draw(oled_descriptor_t* oled_descriptor)
*   \brief  Allow partial drawing of text in X
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_allow_partial_text_x_draw(oled_descriptor_t* oled_descriptor)
{
    oled_descriptor->allow_text_partial_x_draw = TRUE;
}

/*! \fn     ssd1363_is_screen_inverted(oled_descriptor_t* oled_descriptor)
*   \brief  Know if the screen is inverted
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \return The right boolean
*/
BOOL ssd1363_is_screen_inverted(oled_descriptor_t* oled_descriptor)
{
    return oled_descriptor->screen_inverted;
}

/*! \fn     ssd1363_set_screen_invert(oled_descriptor_t* oled_descriptor, BOOL screen_inverted)
*   \brief  Invert the screen
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  screen_inverted     Boolean to invert or not the screen
*/
void ssd1363_set_screen_invert(oled_descriptor_t* oled_descriptor, BOOL screen_inverted)
{
    if (screen_inverted == FALSE)
    {
        ssd1363_write_single_command_with_two_data(oled_descriptor, SSD1363_CMD_SET_SEGMENT_REMAP, 0x32, 0x00);
        ssd1363_write_single_command_with_data(oled_descriptor, SSD1363_CMD_SET_DISPLAY_OFFSET, 0x40);
    } 
    else
    {
        ssd1363_write_single_command_with_two_data(oled_descriptor, SSD1363_CMD_SET_SEGMENT_REMAP, 0x20, 0x00);
        ssd1363_write_single_command_with_data(oled_descriptor, SSD1363_CMD_SET_DISPLAY_OFFSET, 0x60);
    }
    oled_descriptor->screen_inverted = screen_inverted;
}

/*! \fn     ssd1363_prevent_partial_text_y_draw(oled_descriptor_t* oled_descriptor)
*   \brief  Prevent partial drawing of text in Y
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_prevent_partial_text_y_draw(oled_descriptor_t* oled_descriptor)
{
    oled_descriptor->allow_text_partial_y_draw = FALSE;
}

/*! \fn     ssd1363_set_colors_invert(oled_descriptor_t* oled_descriptor, BOOL colors_inverted)
*   \brief  Invert the screen colors
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  screen_inverted     Boolean to invert or not the colors
*/
void ssd1363_set_colors_invert(oled_descriptor_t* oled_descriptor, BOOL colors_inverted)
{
    if (colors_inverted == FALSE)
    {
        ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_SET_NORMAL_DISPLAY);
    }
    else
    {
        ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_SET_INVERSE_DISPLAY);
    }
}

/*! \fn     ssd1363_fill_screen(oled_descriptor_t* oled_descriptor, uint8_t color)
*   \brief  Fill the ssd1363 screen with a given color
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  color               Color (4 bits value)
*/
void ssd1363_fill_screen(oled_descriptor_t* oled_descriptor, uint16_t color)
{
    uint8_t fill_color = (uint8_t)((color & 0x000F) | (color << 4));
    uint32_t i;
    
    /* Select a square that fits the complete screen */
    ssd1363_set_row_address(oled_descriptor, 0, SSD1363_OLED_HEIGHT-1);
    ssd1363_set_column_address(oled_descriptor, 0, SSD1363_OLED_WIDTH/SSD1363_OLED_PIX_PER_COL-1);
    ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_WRITE_RAM);

    /* Start filling the SSD1322 RAM */
    ssd1363_start_data_sending(oled_descriptor);
    for (i = 0; i < SSD1363_OLED_HEIGHT * SSD1363_OLED_WIDTH / SSD1363_OLED_PIX_PER_COL; i++)
    {
        sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, fill_color);
        sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, fill_color);
    }   
    sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
    ssd1363_stop_data_sending(oled_descriptor);
}

/*! \fn     ssd1363_clear_current_screen(oled_descriptor_t* oled_descriptor)
*   \brief  Clear current selected screen (active or inactive)
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_clear_current_screen(oled_descriptor_t* oled_descriptor)
{
    /* Fill screen with 0 pixels */
    ssd1363_fill_screen(oled_descriptor, 0);

    /* clear gddram pixels */
    for (uint16_t ind=0; ind < SSD1363_OLED_HEIGHT; ind++)
    {
        oled_descriptor->gddram_pixel[ind].xaddr = 0;
        oled_descriptor->gddram_pixel[ind].pixels = 0;
    }

    /* Reset current x & y */
    oled_descriptor->cur_text_x = 0;
    oled_descriptor->cur_text_y = 0;
}

#ifdef OLED_INTERNAL_FRAME_BUFFER
/*! \fn     ssd1363_clear_frame_buffer(oled_descriptor_t* oled_descriptor)
*   \brief  Clear frame buffer
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_clear_frame_buffer(oled_descriptor_t* oled_descriptor)
{
    ssd1363_check_for_flush_and_terminate(oled_descriptor);
    memset((void*)oled_descriptor->frame_buffer, 0x00, sizeof(oled_descriptor->frame_buffer));
}

/*! \fn     ssd1363_check_for_flush_and_terminate(oled_descriptor_t* oled_descriptor)
*   \brief  Check if a flush is in progress, and wait for its completion if so
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_check_for_flush_and_terminate(oled_descriptor_t* oled_descriptor)
{
    /* Check for in progress flush */
    if (oled_descriptor->frame_buffer_flush_in_progress != FALSE)
    {        
        /* Wait for data to be transferred */
        while(dma_oled_check_and_clear_dma_transfer_flag() == FALSE);

        /* Wait for spi buffer to be sent */
        sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
        
        /* Stop sending data */
        ssd1363_stop_data_sending(oled_descriptor);
        
        /* Clear bool */
        oled_descriptor->frame_buffer_flush_in_progress = FALSE;
    }
}

/*! \fn     ssd1363_flush_frame_buffer_window(oled_descriptor_t* oled_descriptor, uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y)
*   \brief  Only flush a small part of our frame buffer
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  start_x             Window start x
*   \param  start_y             Window start y
*   \param  end_x               Window end x (included)
*   \param  end_y               Window end y (included)
*   \note   will force start & end x to be SSD1363_OLED_PIX_PER_COL aligned
*/
void ssd1363_flush_frame_buffer_window(oled_descriptor_t* oled_descriptor, uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y)
{
    start_x = (start_x/SSD1363_OLED_PIX_PER_COL)*SSD1363_OLED_PIX_PER_COL;
    end_x = (end_x/SSD1363_OLED_PIX_PER_COL)*SSD1363_OLED_PIX_PER_COL;
    
    /* Sanity checks */
    if (start_x >= SSD1363_OLED_WIDTH)
    {
        start_x = SSD1363_OLED_WIDTH-SSD1363_OLED_PIX_PER_COL;
    }
    if (start_y >= SSD1363_OLED_HEIGHT)
    {
        start_y = SSD1363_OLED_HEIGHT-1;
    }
    if (end_x > SSD1363_OLED_WIDTH)
    {
        end_x = SSD1363_OLED_WIDTH-SSD1363_OLED_PIX_PER_COL;
    }
    if (end_y > SSD1363_OLED_HEIGHT)
    {
        end_y = SSD1363_OLED_HEIGHT-1;
    }
    
    /* Set pixel write window */
    ssd1363_set_row_address(oled_descriptor, start_y, end_y);
    ssd1363_set_column_address(oled_descriptor, start_x/SSD1363_OLED_PIX_PER_COL, end_x/SSD1363_OLED_PIX_PER_COL);
    
    /* Start filling the SSD1322 RAM */
    ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_WRITE_RAM);
    ssd1363_start_data_sending(oled_descriptor);
    
    for (uint16_t y = start_y; y <= end_y; y++)
    {
        for (uint16_t x = start_x/SSD1363_OLED_PIX_PER_COL; x <= end_x/SSD1363_OLED_PIX_PER_COL; x++)
        {
            sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)oled_descriptor->frame_buffer_16b[y][x]);
            sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)(oled_descriptor->frame_buffer_16b[y][x] >> 8));
        }
    }

    /* We're done sending */
    sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
    ssd1363_stop_data_sending(oled_descriptor);
    emu_oled_flush();
}

/*! \fn     ssd1363_flush_frame_buffer(oled_descriptor_t* oled_descriptor)
*   \brief  Flush frame buffer to screen
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_flush_frame_buffer(oled_descriptor_t* oled_descriptor)
{
    /* Wait for a possible ongoing previous flush */
    ssd1363_check_for_flush_and_terminate(oled_descriptor);
    
    if (oled_descriptor->loaded_transition == OLED_TRANS_NONE)
    {
        /* Set pixel write window */
        ssd1363_set_row_address(oled_descriptor, 0, SSD1363_OLED_HEIGHT-1);
        ssd1363_set_column_address(oled_descriptor, 0, (SSD1363_OLED_WIDTH-1)/SSD1363_OLED_PIX_PER_COL);
        
        /* Start filling the SSD1322 RAM */
        ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_WRITE_RAM);
        ssd1363_start_data_sending(oled_descriptor);
        
        /* Send buffer! */
        #ifdef OLED_DMA_TRANSFER        
            dma_oled_init_transfer(oled_descriptor->sercom_pt, (void*)&oled_descriptor->frame_buffer[0][0], sizeof(oled_descriptor->frame_buffer), oled_descriptor->dma_trigger_id);
            oled_descriptor->frame_buffer_flush_in_progress = TRUE;
        #else
            for (uint32_t y = 0; y < SSD1363_OLED_HEIGHT; y++) 
            {
                for (uint32_t x = 0; x < SSD1363_OLED_WIDTH/2; x++) 
                {
                    sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, oled_descriptor->frame_buffer[y][x]);
                }
            }
            sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
            ssd1363_stop_data_sending(oled_descriptor);
        #endif
        emu_oled_flush();
    }
    else if (oled_descriptor->loaded_transition == OLED_LEFT_RIGHT_TRANS)
    {        
        /* Left to right */
        for (uint16_t x = 0; x < SSD1363_OLED_WIDTH; x+=4)
        {
            /* Flush that window */
            ssd1363_flush_frame_buffer_window(oled_descriptor, x, 0, x, SSD1363_OLED_HEIGHT-1);
            emul_delay_ms(1);
            DELAYUS(2500);
        }
    }
    else if (oled_descriptor->loaded_transition == OLED_RIGHT_LEFT_TRANS)
    {
        /* Right to left */
        for (int16_t x = SSD1363_OLED_WIDTH-4; x >= 0; x-=4)
        {
            /* Flush that window */
            ssd1363_flush_frame_buffer_window(oled_descriptor, x, 0, x, SSD1363_OLED_HEIGHT-1);
            emul_delay_ms(1);
            DELAYUS(2500);
        }
    }
    else if (oled_descriptor->loaded_transition == OLED_TOP_BOT_TRANS)
    {
        /* Top to bottom */
        for (uint16_t y = 0; y < SSD1363_OLED_HEIGHT; y++)
        {
            /* Flush that window */
            ssd1363_flush_frame_buffer_window(oled_descriptor, 0, y, SSD1363_OLED_WIDTH-1, y);
            emul_delay_ms(1);
            DELAYUS(2500);
        }
    }
    else if (oled_descriptor->loaded_transition == OLED_BOT_TOP_TRANS)
    {
        /* Bottom to top */
        for (int16_t y = SSD1363_OLED_HEIGHT-1; y >= 0; y--)
        {
            /* Flush that window */
            ssd1363_flush_frame_buffer_window(oled_descriptor, 0, (uint16_t)y, SSD1363_OLED_WIDTH-1, (uint16_t)y);
            emul_delay_ms(1);
            DELAYUS(2500);
        }
    }
    else if (oled_descriptor->loaded_transition == OLED_IN_OUT_TRANS)
    {
        uint16_t low_y = SSD1363_OLED_HEIGHT/2;
        uint16_t high_y = SSD1363_OLED_HEIGHT/2 - 1;
        uint16_t low_x = SSD1363_OLED_WIDTH/2;
        uint16_t high_x = SSD1363_OLED_WIDTH/2 - 4;
        
        /* Window IN to OUT */
        while (low_x != 0)
        {
            low_x -= 4;
            high_x += 4;
            ssd1363_flush_frame_buffer_window(oled_descriptor, low_x, low_y, low_x, high_y);
            ssd1363_flush_frame_buffer_window(oled_descriptor, high_x, low_y, high_x, high_y);
            if (low_y > 0)
            {
                low_y--;
                high_y++;
                if ((low_x % 3) == 0)
                {
                    low_y--;
                    high_y++;
                    ssd1363_flush_frame_buffer_window(oled_descriptor, low_x, low_y, high_x, low_y+1);
                    ssd1363_flush_frame_buffer_window(oled_descriptor, low_x, high_y-1, high_x, high_y);
                }
                else
                {
                    ssd1363_flush_frame_buffer_window(oled_descriptor, low_x, low_y, high_x, low_y);
                    ssd1363_flush_frame_buffer_window(oled_descriptor, low_x, high_y, high_x, high_y);
                }
            }
            emul_delay_ms(1);
            DELAYUS(2500);
        }
    }
    else if (oled_descriptor->loaded_transition == OLED_OUT_IN_TRANS)
    {
        int16_t low_y = -1;
        int16_t high_y = SSD1363_OLED_HEIGHT;
        int16_t low_x = -4;
        int16_t high_x = SSD1363_OLED_WIDTH;
        
        /* Window OUT to IN */
        while (low_x != SSD1363_OLED_WIDTH/2)
        {
            low_x += 4;
            high_x -= 4;
            low_y++;
            high_y--;
            ssd1363_flush_frame_buffer_window(oled_descriptor, low_x, low_y, low_x, high_y);
            ssd1363_flush_frame_buffer_window(oled_descriptor, high_x, low_y, high_x, high_y);
            if ((low_x % 3) == 0)
            {
                low_y++;
                high_y--;
                ssd1363_flush_frame_buffer_window(oled_descriptor, low_x, low_y-1, high_x, low_y);
                ssd1363_flush_frame_buffer_window(oled_descriptor, low_x, high_y, high_x, high_y+1);
            }
            else
            {
                ssd1363_flush_frame_buffer_window(oled_descriptor, low_x, low_y, high_x, low_y);
                ssd1363_flush_frame_buffer_window(oled_descriptor, low_x, high_y, high_x, high_y);
            }
            emul_delay_ms(1);
            DELAYUS(2500);
        }
    }
    
    /* Reset transition */
    oled_descriptor->loaded_transition = OLED_TRANS_NONE;
}
#endif

/*! \fn     ssd1363_set_emergency_font(void)
*   \brief  Use the flash-stored emergency font (ascii only)
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_set_emergency_font(oled_descriptor_t* oled_descriptor)
{
    oled_descriptor->currentFontAddress = CUSTOM_FS_EMERGENCY_FONT_FILE_ADDR;
    custom_fs_read_from_flash((uint8_t*)&oled_descriptor->current_font_header, oled_descriptor->currentFontAddress, sizeof(oled_descriptor->current_font_header));
    custom_fs_read_from_flash((uint8_t*)&oled_descriptor->current_unicode_inters, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header), sizeof(oled_descriptor->current_unicode_inters));
}

/*! \fn     ssd1363_refresh_used_font(oled_descriptor_t* oled_descriptor, uint16_t font_id)
*   \brief  Refreshed used font (in case of init or language change)
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  font_id             Font ID to use
*   \return Success status
*/
RET_TYPE ssd1363_refresh_used_font(oled_descriptor_t* oled_descriptor, uint16_t font_id)
{
    if (custom_fs_get_file_address(font_id, &oled_descriptor->currentFontAddress, CUSTOM_FS_FONTS_TYPE) != RETURN_OK)
    {
        oled_descriptor->currentFontAddress = 0;
        return RETURN_NOK;
    }
    else
    {
        /* Read font header */
        custom_fs_read_from_flash((uint8_t*)&oled_descriptor->current_font_header, oled_descriptor->currentFontAddress, sizeof(oled_descriptor->current_font_header));
        
        /* Read unicode chars support intervals */
        custom_fs_read_from_flash((uint8_t*)&oled_descriptor->current_unicode_inters, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header), sizeof(oled_descriptor->current_unicode_inters));
        
        /* Check for ? support */
        if (('?' < oled_descriptor->current_unicode_inters[0].interval_start) || ('?' > oled_descriptor->current_unicode_inters[0].interval_end))
        {
            oled_descriptor->question_mark_support_described = FALSE;
        } 
        else
        {
            oled_descriptor->question_mark_support_described = TRUE;
        }

        return RETURN_OK;
    }    
}

/*! \fn     ssd1363_get_current_font_height(oled_descriptor_t oled_descriptor)
*   \brief  Get current font height
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \return Font height
*/
uint8_t ssd1363_get_current_font_height(oled_descriptor_t* oled_descriptor)
{
    return oled_descriptor->current_font_header.height;
}

/*! \fn     ssd1363_init_display(oled_descriptor_t oled_descriptor, BOOL leave_internal_logic_and_reflush_frame_buffer, uint8_t master_current)
*   \brief  Initialize a SSD1363 display
*   \param  oled_descriptor                                 Pointer to a ssd1363 descriptor struct
*   \param  leave_internal_logic_and_reflush_frame_buffer   Set to TRUE to do what it says...
*   \param  master_current                                  Display master current
*/
void ssd1363_init_display(oled_descriptor_t* oled_descriptor, BOOL leave_internal_logic_and_reflush_frame_buffer, uint8_t master_current)
{
    /* Vars init : should already be to 0 but you never know... */
    if (leave_internal_logic_and_reflush_frame_buffer == FALSE)
    {
        oled_descriptor->allow_text_partial_y_draw = FALSE;
        oled_descriptor->allow_text_partial_x_draw = FALSE;
        oled_descriptor->carriage_return_allowed = FALSE;
        oled_descriptor->line_feed_allowed = FALSE;
        oled_descriptor->currentFontAddress = 0;
        oled_descriptor->max_text_x = SSD1363_OLED_WIDTH;
        oled_descriptor->min_text_x = 0;
        oled_descriptor->max_disp_x = SSD1363_OLED_WIDTH;
        oled_descriptor->max_disp_y = SSD1363_OLED_HEIGHT;
        oled_descriptor->min_disp_y = 0;
        oled_descriptor->onscreen_log_entry_ind = 0;
    }
    
    /* Different init sequences based on screen inversion */
    const uint8_t* init_seq = ssd1363_init_sequence;
    if (oled_descriptor->screen_inverted != FALSE)
    {
        init_seq = ssd1363_init_sequence_inverted;
    }
    
    /* Send the initialization sequence through SPI */
    for (uint16_t ind = 0; ind < sizeof(ssd1363_init_sequence);)
    {
        /* nCS set */
        PORT->Group[oled_descriptor->cs_pin_group].OUTCLR.reg = oled_descriptor->cs_pin_mask;
        PORT->Group[oled_descriptor->cd_pin_group].OUTCLR.reg = oled_descriptor->cd_pin_mask;
        
        /* First byte: command */
        sercom_spi_send_single_byte(oled_descriptor->sercom_pt, init_seq[ind++]);
        
        /* Second byte: payload length */
        uint16_t dataSize = init_seq[ind++];
        
        /* If different than 0, assert data signal and send payload */
        while (dataSize--)
        {
            PORT->Group[oled_descriptor->cd_pin_group].OUTSET.reg = oled_descriptor->cd_pin_mask;
            sercom_spi_send_single_byte(oled_descriptor->sercom_pt, init_seq[ind++]);
        }
        
        /* nCS release */
        PORT->Group[oled_descriptor->cs_pin_group].OUTSET.reg = oled_descriptor->cs_pin_mask;
        asm("NOP");asm("NOP");
    }

    /* Select whole display RAM for writing 0s... */
    ssd1363_set_column_address_without_offset(oled_descriptor, 0, SSD1363_RAM_X_PIXELS/SSD1363_OLED_PIX_PER_COL-1);
    ssd1363_set_row_address(oled_descriptor, 0, SSD1363_RAM_Y_PIXELS-1);
    ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_WRITE_RAM);

    /* Start filling the SSD1322 RAM */
    ssd1363_start_data_sending(oled_descriptor);
    for (uint16_t i = 0; i < SSD1363_RAM_X_PIXELS * SSD1363_RAM_Y_PIXELS / SSD1363_OLED_PPB; i++)
    {
        sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, 0);
    }
    sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
    ssd1363_stop_data_sending(oled_descriptor);

    #ifdef OLED_INTERNAL_FRAME_BUFFER
    if (leave_internal_logic_and_reflush_frame_buffer == FALSE)
    {
        /* Clear frame buffer if needed */
        memset((void*)oled_descriptor->frame_buffer, 0x00, sizeof(oled_descriptor->frame_buffer));
        oled_descriptor->frame_buffer_flush_in_progress = FALSE;
    }
    #endif
    
    /* Set emergency font by default */
    ssd1363_set_emergency_font(oled_descriptor);
    
    /* Set master current */
    ssd1363_set_contrast_current(oled_descriptor, master_current);
    timer_delay_ms(5);

    /* Switch screen on */
    ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_SET_DISPLAY_ON);
    oled_descriptor->oled_on = TRUE;
    
    /* Reflush frame buffer if asked to */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    if (leave_internal_logic_and_reflush_frame_buffer != FALSE)
    {
        timer_delay_ms(50);
        oled_descriptor->loaded_transition = OLED_TRANS_NONE;
        ssd1363_flush_frame_buffer(oled_descriptor);
        ssd1363_check_for_flush_and_terminate(oled_descriptor);
    }
    #endif
}

/*! \fn     ssd1363_draw_vertical_line(oled_descriptor_t* oled_descriptor, int16_t x, int16_t ystart, int16_t yend, uint8_t color, BOOL write_to_buffer)
*   \brief  Draw a vertical line on the display
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  x                   Starting x
*   \param  ystart              Starting y
*   \param  yend                Ending y
*   \param  color               4 bits Color
*   \param  write_to_buffer     Set to true to write to internal buffer
*/
void ssd1363_draw_vertical_line(oled_descriptor_t* oled_descriptor, int16_t x, int16_t ystart, int16_t yend, uint8_t color, BOOL write_to_buffer)
{
    uint16_t xoff4 = x - (x / 4) * 4;
    ystart = ystart<0?0:ystart;
    color &= 0x0F;
    
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    if (write_to_buffer != FALSE)
    {
        uint16_t pixels_4_color = (color) | (color << 4) | (color << 8) | (color << 12);
        uint16_t pixel_mapping[4] = {0x0F00 & pixels_4_color, 0xF000 & pixels_4_color, 0x000F & pixels_4_color, 0x00F0 & pixels_4_color};

        for (int16_t y=ystart; y<=yend; y++)
        {            
            /* Fill frame buffer */
            oled_descriptor->frame_buffer_16b[y][x/SSD1363_OLED_PIX_PER_COL] |= pixel_mapping[xoff4];
        }
    } 
    else
    {
    #endif
        /* Calculate pixel bitmask */
        uint16_t pixels = color << (4*xoff4);
    
        /* Set pixel write window */
        ssd1363_set_row_address(oled_descriptor, ystart, yend);
        ssd1363_set_column_address(oled_descriptor, x/SSD1363_OLED_PIX_PER_COL, x/SSD1363_OLED_PIX_PER_COL);
        ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_WRITE_RAM);

        /* Start filling the SSD1322 RAM */
        ssd1363_start_data_sending(oled_descriptor);

        /* Send the 4 pixels to the display */
        for (uint16_t y = ystart; y <= yend; y++)
        {
            uint16_t pixels2 = pixels;

            /* Merge adjacent data if available */
            if ((x/SSD1363_OLED_PIX_PER_COL) == oled_descriptor->gddram_pixel[y].xaddr)
            {
                oled_descriptor->gddram_pixel[y].pixels &= ~((0x0F)<<(4*xoff4));
                pixels2 |= oled_descriptor->gddram_pixel[y].pixels;
            }

            /* Send first 2 bytes */
            sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)((pixels2 >> 8) & 0x00FF));

            /* Store pixel data in our gddram buffer for later merging */
            oled_descriptor->gddram_pixel[y].xaddr = x/SSD1363_OLED_PIX_PER_COL;
            oled_descriptor->gddram_pixel[y].pixels = pixels2;

            /* Send next 2 bytes */
            sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)(pixels2 & 0x00FF));
        }
        
        /* Wait for spi buffer to be sent */
        sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
            
        /* Stop sending data */
        ssd1363_stop_data_sending(oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    }
    #endif
}  

/*! \fn     ssd1363_draw_rectangle(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color, BOOL write_to_buffer)
*   \brief  Draw a rectangle on the screen
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  width               Width
*   \param  height              Height
*   \param  color               4 bits color
*   \param  write_to_buffer     Set to something else than FALSE to write to buffer
*   \note   No checks done on X & Y & width & height!
*/
void ssd1363_draw_rectangle(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color, BOOL write_to_buffer)
{
    uint16_t pixels_4_color = (color) | (color << 4) | (color << 8) | (color << 12);

    #ifdef OLED_INTERNAL_FRAME_BUFFER
    if (write_to_buffer != FALSE)
    {
        uint16_t pixel_lfilling[4] = {0xFFFF, 0xF0FF, 0x00FF, 0x00F0};
        uint16_t pixel_rfilling[4] = {0x0000, 0x0F00, 0xFF00, 0xFF0F};
        uint16_t xoffset = x%SSD1363_OLED_PIX_PER_COL;

        for (uint16_t yind = 0; yind < height; yind++)
        {
            uint16_t xind = 0;

            /* Start x not a multiple of 4 */
            if (xoffset != 0)
            {
                /* Set xind to number of pixels we need to read to be a multiple of 4 */
                xind = SSD1363_OLED_PIX_PER_COL-x%SSD1363_OLED_PIX_PER_COL;
                
                /* one to three pixels */
                oled_descriptor->frame_buffer_16b[y+yind][x/SSD1363_OLED_PIX_PER_COL] &= ~pixel_lfilling[xoffset];
                oled_descriptor->frame_buffer_16b[y+yind][x/SSD1363_OLED_PIX_PER_COL] |= (pixel_lfilling[xoffset] & pixels_4_color);
            }
            
            /* Start x multiple of 4, start filling */
            for (; xind < width; xind+=SSD1363_OLED_PIX_PER_COL)
            {
                if ((xind+SSD1363_OLED_PIX_PER_COL) <= width)
                {
                    oled_descriptor->frame_buffer_16b[y+yind][(x+xind)/SSD1363_OLED_PIX_PER_COL] = pixels_4_color;
                }
                else
                {
                    oled_descriptor->frame_buffer_16b[y+yind][(x+xind)/SSD1363_OLED_PIX_PER_COL] &= ~pixel_rfilling[width-xind];
                    oled_descriptor->frame_buffer_16b[y+yind][(x+xind)/SSD1363_OLED_PIX_PER_COL] |= (pixel_rfilling[width-xind]& pixels_4_color);
                }
            }
        }
    }
    else
    {
        #endif
        /* Set pixel write window */
        ssd1363_set_row_address(oled_descriptor, y, y+height-1);
        ssd1363_set_column_address(oled_descriptor, x/SSD1363_OLED_PIX_PER_COL, (x+width-1)/SSD1363_OLED_PIX_PER_COL);

        /* Start filling the SSD1322 RAM */
        ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_WRITE_RAM);
        ssd1363_start_data_sending(oled_descriptor);
        
        /* Y loop */
        for (uint16_t yind=0; yind < height; yind++)
        {
            uint16_t xind = 0;
            uint16_t pixels = 0;

            /* Start x not a multiple of 4 */
            if (x%SSD1363_OLED_PIX_PER_COL != 0)
            {
                /* Set xind to number of pixels we need to read to be a multiple of 4 */
                xind = SSD1363_OLED_PIX_PER_COL-x%SSD1363_OLED_PIX_PER_COL;
                uint8_t pixels_to_display = xind>width?width:xind;
                
                /* Generate pixels, bitshift (remember that the pixel order is reversed) */
                pixels = color;
                for (uint16_t i = 1; i < pixels_to_display; i++)
                {
                    pixels <<= 4;
                    pixels |= color;
                }
                pixels <<= (SSD1363_OLED_PIX_PER_COL-xind)*SSD1363_OLED_BPP;

                /* Fill existing pixels if available */
                if ((x/SSD1363_OLED_PIX_PER_COL) == oled_descriptor->gddram_pixel[y+yind].xaddr)
                {
                    pixels |= oled_descriptor->gddram_pixel[y+yind].pixels;
                }

                /* Send the 4 pixels to the display */
                if (x >= 0)
                {
                    sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)((pixels>>8) & 0x00FF));
                    sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)(pixels & 0x00FF));
                }
            }
        
            /* Start x multiple of 4, start filling */
            for (; xind < width; xind+=SSD1363_OLED_PIX_PER_COL)
            {
                if ((xind+SSD1363_OLED_PIX_PER_COL) <= width)
                {
                    pixels = pixels_4_color;
                }
                else
                {
                    pixels = color;
                    for (uint16_t i = 1; i < width-xind; i++)
                    {
                        pixels <<= 4;
                        pixels |= color;
                    }
                }
            
                /* Send the 4 pixels to the display */
                sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)((pixels>>8) & 0x00FF));
                sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)(pixels & 0x00FF));
            }
        
            /* Store pixel data in our gddram buffer for later merging */
            if (pixels != 0)
            {
                oled_descriptor->gddram_pixel[y+yind].pixels = pixels;
                oled_descriptor->gddram_pixel[y+yind].xaddr = (x+width-1)/SSD1363_OLED_PIX_PER_COL;
            }
        }
        
        /* Wait for spi buffer to be sent */
        sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
        
        /* Stop sending data */
        ssd1363_stop_data_sending(oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    }
    #endif
}

/*! \fn     ssd1363_draw_full_screen_image_from_bitstream(oled_descriptor_t* oled_descriptor, bitstream_bitmap_t* bitstream)
*   \brief  Draw a full screen picture from a bitstream
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  bitstream           Pointer to the bistream
*/
void ssd1363_draw_full_screen_image_from_bitstream(oled_descriptor_t* oled_descriptor, bitstream_bitmap_t* bitstream)
{
    /*  So, here's a quick overview if you were to wonder what has been done to improve display speeds:
    /   Note: the FPS count mentioned here highly depends on the picture itself due to RLE compression
    /   For a non-RLE compressed & non-pixels inverted bitmap, solution compiled in release, OLED SPI at 12MHz, FLASH SPI at 24MHz:
    /   0) baseline: 10fps
    /   1) FLASH_ALONE_ON_SPI_BUS: when defined, reads from the flash are done in a continuous manner, rather than doing multiple reads for different chunks of data: 10fps
    /   2) FLASH_DMA_FETCHES: when defined, reads from the flash are done using the DMA controller: 12fps
    /   3) OLED_DMA_TRANSFER: when defined, writes to the oled are done using the DMA controller: 13fps
    /   For a non-RLE compressed & pixels inverted bitmap: 22fps / 24fps / 36fps / 47fps
    /   For a RLE compressed bitmap: 21fps / 21fps / 23fps / 27fps
    /   18% gain can be achieved by making bitstream_bitmap_array_read_rev without the last arg
    */

    #ifdef OLED_INTERNAL_FRAME_BUFFER
    /* Wait for a possible ongoing previous flush */
    ssd1363_check_for_flush_and_terminate(oled_descriptor);
    #endif

    /* Set pixel write window */
    ssd1363_set_row_address(oled_descriptor, 0, SSD1363_OLED_HEIGHT-1);
    ssd1363_set_column_address(oled_descriptor, 0, (SSD1363_OLED_WIDTH-1)/SSD1363_OLED_PIX_PER_COL);
    
    /* Start filling the SSD1322 RAM */
    ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_WRITE_RAM);
    ssd1363_start_data_sending(oled_descriptor);
    
    /* Depending if we use DMA transfers */
    #ifdef OLED_DMA_TRANSFER        
        uint16_t pixel_buffer[2][16];
        uint32_t buffer_sel = 0;
        
        /* Get things going: start first transfer then enter the for(), as we need to wait for OLED DMA after inside the loop */
        bitstream_bitmap_array_read_rev(bitstream, pixel_buffer[buffer_sel], sizeof(pixel_buffer[0])*2, 0);
        dma_oled_init_transfer(oled_descriptor->sercom_pt, (void*)pixel_buffer[buffer_sel], sizeof(pixel_buffer[0]), oled_descriptor->dma_trigger_id);
        
        for (uint32_t i = 0; i < (SSD1363_OLED_WIDTH*SSD1363_OLED_HEIGHT) - sizeof(pixel_buffer[0])*2; i+=sizeof(pixel_buffer[0])*2)
        {            
            /* Read from bitstream in the next buffer */
            bitstream_bitmap_array_read_rev(bitstream, pixel_buffer[(buffer_sel+1)&0x01], sizeof(pixel_buffer[0])*2, 0);
            
            /* Wait for transfer done */
            while(dma_oled_check_and_clear_dma_transfer_flag() == FALSE);
            
            /* Init DMA transfer */
            buffer_sel = (buffer_sel+1) & 0x01;
            dma_oled_init_transfer(oled_descriptor->sercom_pt, (void*)pixel_buffer[buffer_sel], sizeof(pixel_buffer[0]), oled_descriptor->dma_trigger_id);
        }
        
        /* Wait for data to be transferred */
        while(dma_oled_check_and_clear_dma_transfer_flag() == FALSE);
    #else        
        uint16_t pixel_buffer[16];
        
        /* Send all pixels */
        for (uint32_t i = 0; i < (SSD1363_OLED_WIDTH*SSD1363_OLED_HEIGHT); i+=sizeof(pixel_buffer)*2)
        {
            /* Read from bitstream */
            bitstream_bitmap_array_read_rev(bitstream, pixel_buffer, sizeof(pixel_buffer)*2, 0);
            
            /* Send pixels */
            for (uint16_t j = 0; j < ARRAY_SIZE(pixel_buffer); j++)
            {
                sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, pixel_buffer[j] & 0x00FF);
                sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (pixel_buffer[j] >> 8) & 0x00FF);
            }
        }
    #endif
    
    /* Wait for spi buffer to be sent */
    sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
    
    /* Stop sending data */
    ssd1363_stop_data_sending(oled_descriptor);
    
    /* Close bitstream */
    bitstream_bitmap_close(bitstream);
}

/*! \fn     ssd1363_draw_image_from_bitstream(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_t* bs, BOOL write_to_buffer)
*   \brief  Draw a picture from a bitstream
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  bitstream           Pointer to the bitstream
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \note   48x48px image going across the screen: 1950ms without defines (dbg), 1974ms adding FLASH_ALONE_ON_SPI_BUS (dbg), 1599ms adding FLASH_DMA_FETCHES (dbg), 1527ms adding OLED_DMA_TRANSFER (dbg)
*/
void ssd1363_draw_image_from_bitstream(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_bitmap_t* bitstream, BOOL write_to_buffer)
{
    /* Check for off screen line on the left */
    if (((x < 0) && (-x >= bitstream->width)) || (x < -SSD1363_OLED_WIDTH))
    {
        bitstream_bitmap_close(bitstream);   
        return;
    }
    
    /* Check for off screen line on the right */
    if (x >= oled_descriptor->max_disp_x)
    {
        bitstream_bitmap_close(bitstream);
        return;
    }

    /* Use different drawing methods if it's a full screen picture and if we are 4 pixels aligned */
    if ((x == 0) && (y == 0) && (bitstream->width == SSD1363_OLED_WIDTH) && (bitstream->height == SSD1363_OLED_HEIGHT) && (oled_descriptor->max_disp_y == SSD1363_OLED_HEIGHT) && (write_to_buffer == FALSE))
    {
        /* Dedicated code to allow faster write to display */
        ssd1363_draw_full_screen_image_from_bitstream(oled_descriptor, bitstream);        
    }
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    else if (write_to_buffer != FALSE)
    {        
        /* Number of pixels to send per line */
        uint16_t nb_pixels_to_write = bitstream->width;
        uint16_t rpixels_to_discard = 0;
        int16_t xstart = x>=0?x:0;

        /* Negative X */
        if (x < 0)
        {
            nb_pixels_to_write += x;
        }
        
        /* Bitmap over screen edge on the right */
        if (xstart + nb_pixels_to_write > oled_descriptor->max_disp_x)
        {
            rpixels_to_discard = xstart + nb_pixels_to_write - oled_descriptor->max_disp_x;
            nb_pixels_to_write = oled_descriptor->max_disp_x - xstart;
        }

        /* Wait for a possible ongoing previous flush */
        ssd1363_check_for_flush_and_terminate(oled_descriptor);

        /* Y loop */
        for (uint16_t yind=0; yind < bitstream->height; yind++)
        {
            uint16_t xind = 0;
            
            /* If y is off screen, simply continue looping */
            if ((y+yind < oled_descriptor->min_disp_y) || (y+yind >= oled_descriptor->max_disp_y))
            {
                bitstream_bitmap_discard_pixels(bitstream, bitstream->width);
                continue;
            }

            /* If x is negative, discard off-screen pixels */
            if (x < 0)
            {
                xind = -x;
                bitstream_bitmap_discard_pixels(bitstream, -x);
            }

            /* Overwrite framebuffer at the right slot */
            bitstream_bitmap_array_read_rev(bitstream, &oled_descriptor->frame_buffer_16b[y+yind][(x+xind)/SSD1363_OLED_PIX_PER_COL], nb_pixels_to_write, (x+xind)%SSD1363_OLED_PIX_PER_COL);
            
            /* Discard possible off-screen pixels */
            if (rpixels_to_discard > 0)
            {
                bitstream_bitmap_discard_pixels(bitstream, rpixels_to_discard);
            }            
        }
        
        /* Close bitstream */
        bitstream_bitmap_close(bitstream);    
    }
    #endif
    #ifdef OLED_DMA_TRANSFER
    else if ((bitstream->width % SSD1363_OLED_PIX_PER_COL == 0) && (x % SSD1363_OLED_PIX_PER_COL == 0) && (oled_descriptor->max_disp_x % SSD1363_OLED_PIX_PER_COL == 0) && (bitstream->width <= SSD1363_OLED_WIDTH))
    {
        /* Buffer large enough to contain a display line in order to trig one DMA transfer */
        uint16_t pixel_buffer[2][SSD1363_OLED_WIDTH/4];
        uint16_t buffer_sel = 0;

        /* Offset in data array */
        uint16_t pixel_array_offset = 0;

        /* Number of pixels to send per line */
        uint16_t nb_pixels_to_send = bitstream->width;

        /* Compute ystart and yend */
        int16_t xstart = x>=0?x:0;
        int16_t xend = (x+bitstream->width-1)>=oled_descriptor->max_disp_x?oled_descriptor->max_disp_x-1:(x+bitstream->width-1);
        int16_t ystart = y>=oled_descriptor->min_disp_y?y:oled_descriptor->min_disp_y;
        int16_t yend = (y+bitstream->height-1)>=oled_descriptor->max_disp_y?oled_descriptor->max_disp_y-1:(y+bitstream->height-1);

        /* Negative X */
        if (x < 0)
        {
            pixel_array_offset = -x/4;
            nb_pixels_to_send += x;
        }
        
        /* Bitmap over screen edge on the right */
        if (xstart + nb_pixels_to_send > oled_descriptor->max_disp_x)
        {
            nb_pixels_to_send = oled_descriptor->max_disp_x-xstart;
        }

        #ifdef OLED_INTERNAL_FRAME_BUFFER
        /* Wait for a possible ongoing previous flush */
        ssd1363_check_for_flush_and_terminate(oled_descriptor);
        #endif

        /* Set pixel write window */
        ssd1363_set_row_address(oled_descriptor, ystart, yend);
        ssd1363_set_column_address(oled_descriptor, xstart/SSD1363_OLED_PIX_PER_COL, xend/SSD1363_OLED_PIX_PER_COL);
        
        /* Start filling the SSD1322 RAM */
        ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_WRITE_RAM);
        ssd1363_start_data_sending(oled_descriptor);

        /* Trigger first buffer fill: if we asked more data, the bitstream will return 0s */
        bitstream_bitmap_array_read_rev(bitstream, pixel_buffer[buffer_sel], bitstream->width, 0);

        /* Scan Y */
        for (uint16_t j = 0; j < bitstream->height; j++)
        {            
            if ((y+j >= oled_descriptor->min_disp_y) && (y+j <= oled_descriptor->max_disp_y))
            {                
                /* Trigger DMA transfer for the complete width */
                dma_oled_init_transfer(oled_descriptor->sercom_pt, (void*)&pixel_buffer[buffer_sel][pixel_array_offset], nb_pixels_to_send/2, oled_descriptor->dma_trigger_id);
            }
                
            /* Flip buffer, start fetching next line while the transfer is happening */
            if (j != bitstream->height-1)
            {
                buffer_sel = (buffer_sel+1) & 0x01;
                bitstream_bitmap_array_read_rev(bitstream, pixel_buffer[buffer_sel], bitstream->width, 0);
            }
               
            if ((y+j >= oled_descriptor->min_disp_y) && (y+j <= oled_descriptor->max_disp_y))
            { 
                /* Wait for transfer done */
                while(dma_oled_check_and_clear_dma_transfer_flag() == FALSE);
            }
        }

        /* Wait for spi buffer to be sent */
        sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
            
        /* Stop sending data */
        ssd1363_stop_data_sending(oled_descriptor);
        
        /* Close bitstream */
        bitstream_bitmap_close(bitstream);   
    } 
    #endif
    else
    {
        /* Compute ystart and yend */
        int16_t xstart = x>=0?x:0;
        int16_t xend = (x+bitstream->width-1)>=oled_descriptor->max_disp_x?oled_descriptor->max_disp_x-1:(x+bitstream->width-1);
        int16_t ystart = y>=oled_descriptor->min_disp_y?y:oled_descriptor->min_disp_y;
        int16_t yend = (y+bitstream->height-1)>=oled_descriptor->max_disp_y?oled_descriptor->max_disp_y-1:(y+bitstream->height-1);

        #ifdef OLED_INTERNAL_FRAME_BUFFER
        /* Wait for a possible ongoing previous flush */
        ssd1363_check_for_flush_and_terminate(oled_descriptor);
        #endif

        /* Set pixel write window */
        ssd1363_set_row_address(oled_descriptor, ystart, yend);
        ssd1363_set_column_address(oled_descriptor, xstart/SSD1363_OLED_PIX_PER_COL, xend/SSD1363_OLED_PIX_PER_COL);
            
        /* Start filling the SSD1322 RAM */
        ssd1363_write_single_command(oled_descriptor, SSD1363_CMD_WRITE_RAM);
        ssd1363_start_data_sending(oled_descriptor);
        
        /* Y loop */
        for (uint16_t yind=0; yind < bitstream->height; yind++)
        {
            uint16_t xind = 0;
            uint16_t pixels = 0;
               
            /* If y is off screen, simply continue looping */
            if ((y+yind < oled_descriptor->min_disp_y) || (y+yind >= oled_descriptor->max_disp_y))
            {
                bitstream_bitmap_discard_pixels(bitstream, bitstream->width);
                continue;
            }

            /* If x is negative, discard off-screen pixels */
            if (x < 0)
            {
                xind = -x;
                bitstream_bitmap_discard_pixels(bitstream, -x);
            }
            else if (x%SSD1363_OLED_PIX_PER_COL != 0)
            {
                /* Start x not a multiple of 4: set xind to number of pixels we need to read to be a multiple of 4 */
                xind = SSD1363_OLED_PIX_PER_COL-x%SSD1363_OLED_PIX_PER_COL;
                uint8_t pixels_to_read = xind>bitstream->width?bitstream->width:xind;
                
                /* Fetch X pixels with bitshift */
                pixels = bitstream_bitmap_read_rev_with_shift(bitstream, pixels_to_read, 4-pixels_to_read);

                /* Fill existing pixels if available */
                if ((x/SSD1363_OLED_PIX_PER_COL) == oled_descriptor->gddram_pixel[y+yind].xaddr)
                {
                    pixels |= oled_descriptor->gddram_pixel[y+yind].pixels;
                }

                /* Send the 4 pixels to the display */
                if (x >= 0)
                {
                    sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)(pixels & 0x00FF));
                    sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)((pixels>>8) & 0x00FF));
                }
            }
            
            /* Start x multiple of 4, start filling */
            for (; xind < bitstream->width; xind+=SSD1363_OLED_PIX_PER_COL)
            {
                if ((xind+SSD1363_OLED_PIX_PER_COL) <= bitstream->width)
                {
                    pixels = bitstream_bitmap_read_rev(bitstream, SSD1363_OLED_PIX_PER_COL);
                }
                else
                {
                    pixels = bitstream_bitmap_read_rev(bitstream, bitstream->width-xind);
                }
                
                /* Send the 4 pixels to the display */
                if ((x+xind >= 0) && (x+xind < oled_descriptor->max_disp_x))
                {
                    sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)(pixels & 0x00FF));
                    sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)((pixels>>8) & 0x00FF));
                }
            }
            
            /* Store pixel data in our gddram buffer for later merging */
            if (pixels != 0)
            {
                oled_descriptor->gddram_pixel[y+yind].pixels = pixels;
                oled_descriptor->gddram_pixel[y+yind].xaddr = (x+bitstream->width-1)/SSD1363_OLED_PIX_PER_COL;
            }
        }

        /* Wait for spi buffer to be sent */
        sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
            
        /* Stop sending data */
        ssd1363_stop_data_sending(oled_descriptor);
        
        /* Close bitstream */
        bitstream_bitmap_close(bitstream);   
    }    
}

/*! \fn     ssd1363_display_bitmap_from_flash_at_recommended_position(oled_descriptor_t* oled_descriptor, uint32_t file_id, BOOL write_to_buffer)
*   \brief  Display a bitmap stored in the external flash, at its recommended position
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  file_id             Bitmap file ID
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return success status
*/
RET_TYPE ssd1363_display_bitmap_from_flash_at_recommended_position(oled_descriptor_t* oled_descriptor, uint32_t file_id, BOOL write_to_buffer)
{
    custom_fs_address_t file_adress;
    bitstream_bitmap_t bitstream;
    bitmap_t bitmap;

    /* Fetch file address */
    if (custom_fs_get_file_address(file_id, &file_adress, CUSTOM_FS_BITMAP_TYPE) != RETURN_OK)
    {
        return RETURN_NOK;
    }

    /* Read bitmap info data */
    custom_fs_read_from_flash((uint8_t *)&bitmap, file_adress, sizeof(bitmap));
    
    /* Init bitstream */
    bitstream_bitmap_init(&bitstream, &bitmap, file_adress + sizeof(bitmap), TRUE);
    
    /* Draw bitmap */
    ssd1363_draw_image_from_bitstream(oled_descriptor, bitmap.xpos, bitmap.ypos, &bitstream, write_to_buffer);
    
    return RETURN_OK;    
}

/*! \fn     ssd1363_display_bitmap_from_flash(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, uint32_t file_id, BOOL write_to_buffer)
*   \brief  Display a bitmap stored in the external flash
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  file_id             Bitmap file ID
*   \param  write_to_buffer    Set to true to write to internal buffer
*   \return success status
*/
RET_TYPE ssd1363_display_bitmap_from_flash(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, uint32_t file_id, BOOL write_to_buffer)
{
    custom_fs_address_t file_adress;
    bitstream_bitmap_t bitstream;
    bitmap_t bitmap;

    /* Fetch file address */
    if (custom_fs_get_file_address(file_id, &file_adress, CUSTOM_FS_BITMAP_TYPE) != RETURN_OK)
    {
        return RETURN_NOK;
    }    

    /* Read bitmap info data */
    custom_fs_read_from_flash((uint8_t *)&bitmap, file_adress, sizeof(bitmap));
    
    /* Init bitstream */
    bitstream_bitmap_init(&bitstream, &bitmap, file_adress + sizeof(bitmap), TRUE);
    
    /* Draw bitmap */
    ssd1363_draw_image_from_bitstream(oled_descriptor, x, y, &bitstream, write_to_buffer);
    
    return RETURN_OK;  
} 

/*! \fn     ssd1363_get_string_width(oled_descriptor_t* oled_descriptor, const char* str)
*   \brief  Return the pixel width of the string.
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  str                 String to get width of
*   \return Width of string in pixels based on current font
*/
uint16_t ssd1363_get_string_width(oled_descriptor_t* oled_descriptor, const cust_char_t* str)
{
    uint16_t temp_uint16 = 0;
    uint16_t width=0;
    
    for (nat_type_t ind=0; (str[ind] != 0) && (str[ind] != '\r'); ind++)
    {
        width += ssd1363_get_glyph_width(oled_descriptor, str[ind], &temp_uint16);
    }
    
    return width;    
}

/*! \fn     ssd1363_get_glyph_width(oled_descriptor_t* oled_descriptor, char ch, uint16_t* glyph_height)
*   \brief  Return the width of the specified character in the current font
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  ch                  Character
*   \param  glyph_height        Where to store the glyph height (added bonus)
*   \return width of the glyph
*/
uint16_t ssd1363_get_glyph_width(oled_descriptor_t* oled_descriptor, cust_char_t ch, uint16_t* glyph_height)
{
    uint16_t glyph_desc_pt_offset = 0;
    uint16_t interval_start = 0;
    font_glyph_t glyph;
    uint16_t gind;
    
    /* Set default value */
    *glyph_height = 0;
    
    /* Check that a font was actually chosen */
    if (oled_descriptor->currentFontAddress != 0)
    {
        /* Check that support for this char is described */
        BOOL char_support_described = FALSE;
        for (uint16_t i=0; i < sizeof(oled_descriptor->current_unicode_inters)/sizeof(oled_descriptor->current_unicode_inters[0]); i++)
        {
            /* Check if char is within this interval */
            if ((oled_descriptor->current_unicode_inters[i].interval_start != 0xFFFF) && (oled_descriptor->current_unicode_inters[i].interval_start <= ch) && (oled_descriptor->current_unicode_inters[i].interval_end >= ch))
            {
                interval_start = oled_descriptor->current_unicode_inters[i].interval_start;
                char_support_described = TRUE;
                break;
            }
            
            /* Add offset to descriptor */
            glyph_desc_pt_offset += oled_descriptor->current_unicode_inters[i].interval_end - oled_descriptor->current_unicode_inters[i].interval_start + 1;
        }
        
        /* Support not described, check if we could switch with ? */
        if (char_support_described == FALSE)
        {
            if (oled_descriptor->question_mark_support_described != FALSE)
            {
                interval_start = oled_descriptor->current_unicode_inters[0].interval_start;
                glyph_desc_pt_offset = 0;
                ch = '?';
            } 
            else
            {
                return 0;
            }
        }
        
        /* Convert character to glyph index */
        custom_fs_read_from_flash((uint8_t*)&gind, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + sizeof(oled_descriptor->current_unicode_inters) + glyph_desc_pt_offset*sizeof(gind) + (ch - interval_start)*sizeof(gind), sizeof(gind));

        /* Check that we know this glyph */
        if(gind == 0xFFFF)
        {
            // If we don't know this character, try again with '?'
            if (oled_descriptor->question_mark_support_described == FALSE)
            {
                return 0;
            }
            else
            {
                ch = '?';
            }            
            custom_fs_read_from_flash((uint8_t*)&gind, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + sizeof(oled_descriptor->current_unicode_inters) + glyph_desc_pt_offset*sizeof(gind) + (ch - interval_start)*sizeof(gind), sizeof(gind));
            
            // If we still don't know it, return 0
            if (gind == 0xFFFF)
            {
                return 0;
            }
        }

        // Read the beginning of the glyph
        custom_fs_read_from_flash((uint8_t*)&glyph, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + sizeof(oled_descriptor->current_unicode_inters) + (oled_descriptor->current_font_header.described_chr_count)*sizeof(gind) + gind*sizeof(glyph), sizeof(glyph));

        if (glyph.glyph_data_offset == 0xFFFFFFFF)
        {
            // If there's no glyph data, it is the space!
            return glyph.xrect + 1;
        }
        else
        {
            *glyph_height = glyph.yrect + glyph.yoffset;
            return glyph.xrect + glyph.xoffset + 1;
        }
    }
    else
    {
        return 0;
    }
}

 /*! \fn     ssd1363_glyph_draw(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, char ch, BOOL write_to_buffer)
 *   \brief  Draw a character glyph on the screen at x,y.
 *   \param  oled_descriptor    Pointer to a ssd1363 descriptor struct
 *   \param  x                  x position to start glyph
 *   \param  y                  y position to start glyph
 *   \param  ch                 Character to draw
 *   \param  write_to_buffer    Set to true to write to internal buffer
 *   \return width of the glyph
 */
uint16_t ssd1363_glyph_draw(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, cust_char_t ch, BOOL write_to_buffer)
{
    uint16_t glyph_desc_pt_offset = 0;  // Offset to the pointer of the glyph descriptor
    uint16_t interval_start = 0;        // Unicode code of the first char of the current unicode support interval
    bitstream_bitmap_t bs;              // Character bitstream
    uint8_t glyph_width;                // Glyph width
    font_glyph_t glyph;                 // Glyph header
    uint16_t gind;                      // Glyph index

    /* Check for selected font */
    if (oled_descriptor->currentFontAddress == 0)
    {
        return 0;
    }
    
    /* Check that support for this char is described */
    BOOL char_support_described = FALSE;
    for (uint16_t i=0; i < sizeof(oled_descriptor->current_unicode_inters)/sizeof(oled_descriptor->current_unicode_inters[0]); i++)
    {
        /* Check if char is within this interval */
        if ((oled_descriptor->current_unicode_inters[i].interval_start != 0xFFFF) && (oled_descriptor->current_unicode_inters[i].interval_start <= ch) && (oled_descriptor->current_unicode_inters[i].interval_end >= ch))
        {
            interval_start = oled_descriptor->current_unicode_inters[i].interval_start;
            char_support_described = TRUE;
            break;
        }
        
        /* Add offset to descriptor */
        glyph_desc_pt_offset += oled_descriptor->current_unicode_inters[i].interval_end - oled_descriptor->current_unicode_inters[i].interval_start + 1;
    }
    
    /* Support not described, check if we could switch with ? */
    if (char_support_described == FALSE)
    {
        if (oled_descriptor->question_mark_support_described != FALSE)
        {
            interval_start = oled_descriptor->current_unicode_inters[0].interval_start;
            glyph_desc_pt_offset = 0;
            ch = '?';
        }
        else
        {
            return 0;
        }
    }
    
    /* Convert character to glyph index */
    custom_fs_read_from_flash((uint8_t*)&gind, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + sizeof(oled_descriptor->current_unicode_inters) + glyph_desc_pt_offset*sizeof(gind) + (ch - interval_start)*sizeof(gind), sizeof(gind));

    /* Check that we know this glyph */
    if(gind == 0xFFFF)
    {
        // If we don't know this character, try again with '?'
        if (oled_descriptor->question_mark_support_described == FALSE)
        {
            return 0;
        }
        else
        {
            ch = '?';
        }
        custom_fs_read_from_flash((uint8_t*)&gind, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + sizeof(oled_descriptor->current_unicode_inters) + glyph_desc_pt_offset*sizeof(gind) + (ch - interval_start)*sizeof(gind), sizeof(gind));
        
        // If we still don't know it, return 0
        if (gind == 0xFFFF)
        {
            return 0;
        }
    }
    
    /* Read glyph data */
    custom_fs_read_from_flash((uint8_t*)&glyph, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + sizeof(oled_descriptor->current_unicode_inters) + (oled_descriptor->current_font_header.described_chr_count)*sizeof(gind) + gind*sizeof(glyph), sizeof(glyph));

    if (glyph.glyph_data_offset == 0xFFFFFFFF)
    {
        /* Space character, just fill in the gddram buffer and output background pixels */
        glyph.glyph_data_offset = 0;
        glyph_width = glyph.xrect;
    }
    else
    {
        /* Store glyph height and width, increment with offset */
        glyph_width = glyph.xrect;
        x += glyph.xoffset;
        y += glyph.yoffset;
        
        /* Compute glyph data address */
        custom_fs_address_t gaddr = oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + sizeof(oled_descriptor->current_unicode_inters) + (oled_descriptor->current_font_header.described_chr_count)*sizeof(gind) + (oled_descriptor->current_font_header.chr_count)*sizeof(glyph) + glyph.glyph_data_offset;
        
        // Initialize bitstream & draw the character
        bitstream_glyph_bitmap_init(&bs, &oled_descriptor->current_font_header, &glyph, gaddr, TRUE);
        ssd1363_draw_image_from_bitstream(oled_descriptor, x, y, &bs, write_to_buffer);
    }
    
    return (uint8_t)(glyph_width + glyph.xoffset) + 1;
}

/*! \fn     ssd1363_put_char(oled_descriptor_t* oled_descriptor, char ch, BOOL write_to_buffer)
*   \brief  Print char on display
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  ch                  Char to display
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return Pixel width of the printed char, -1 if too wide for display
*/
int16_t ssd1363_put_char(oled_descriptor_t* oled_descriptor, cust_char_t ch, BOOL write_to_buffer)
{
    uint16_t glyph_height = 0;
    
    /* Have we actually selected a font? */
    if (oled_descriptor->currentFontAddress == 0)
    {
        return -1;
    }
    
    if ((ch == '\n') || (ch == '\r'))
    {
        // Handled at the calling function level
    }
    else
    {
        uint16_t width = ssd1363_get_glyph_width(oled_descriptor, ch, &glyph_height);
        
        /* Check if we're not larger than the screen */
        if ((width + oled_descriptor->cur_text_x) > oled_descriptor->max_text_x)
        {
            if (oled_descriptor->line_feed_allowed != FALSE)
            {
                oled_descriptor->cur_text_y += oled_descriptor->current_font_header.height;
                oled_descriptor->cur_text_x = 0;

                /* Check for out of screen */
                if (oled_descriptor->cur_text_y >= oled_descriptor->max_disp_y)
                {
                    return -1;
                }
            }
            else if ((oled_descriptor->cur_text_x < oled_descriptor->max_text_x) && (oled_descriptor->allow_text_partial_x_draw != FALSE))
            {
                /* Special case: part of glyph displayed */
            }
            else
            {
                return -1;
            }
        }
        
        /* Same check but for Y */
        if ((glyph_height + oled_descriptor->cur_text_y > oled_descriptor->max_disp_y) && (oled_descriptor->allow_text_partial_y_draw == FALSE))
        {
            return -1;
        }
        
        /* Display the text */
        int16_t cur_text_x_copy = oled_descriptor->cur_text_x;
        uint16_t max_disp_x_copy = oled_descriptor->max_disp_x;
        oled_descriptor->max_disp_x = oled_descriptor->max_text_x;
        oled_descriptor->cur_text_x += ssd1363_glyph_draw(oled_descriptor, oled_descriptor->cur_text_x, oled_descriptor->cur_text_y, ch, write_to_buffer);
        oled_descriptor->max_disp_x = max_disp_x_copy;
        
        /* Return printed pixels width */
        if (cur_text_x_copy < 0)
        {
            /* Start of print X is off screen */
            if (oled_descriptor->cur_text_x > 0)
            {
                return oled_descriptor->cur_text_x;
            } 
            else
            {
                return 0;
            }
        }
        if (oled_descriptor->cur_text_x < SSD1363_OLED_WIDTH)
        {
            /* Normal use case */
            return width;
        } 
        else
        {
            /* Part of glyph is displayed */
            return width - (oled_descriptor->cur_text_x - SSD1363_OLED_WIDTH);
        }
    }
    
    return 0;
}

/*! \fn     ssd1363_put_string(oled_descriptor_t* oled_descriptor, const char* str, BOOL write_to_buffer)
*   \brief  Print string at current x y
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  str                 String to print
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return Pixel width of the printed string, -1 if too wide for display
*/
int16_t ssd1363_put_string(oled_descriptor_t* oled_descriptor, const cust_char_t* str, BOOL write_to_buffer)
{
    int16_t string_width = 0;
    
    // Write chars until we find final 0
    while (*str)
    {
        /* Check for line feed */
        if (*str == '\n')
        {
            if (oled_descriptor->line_feed_allowed == FALSE)
            {
                return string_width;
            } 
            else
            {
                /* Get to the new line */
                while ((*str == '\n') || (*str == '\r'))
                {
                    str++;
                }
                
                /* Compute new X & Y */
                oled_descriptor->cur_text_y += oled_descriptor->current_font_header.height;
                oled_descriptor->cur_text_x = ssd1363_get_start_x_for_string_based_on_alignment(oled_descriptor, oled_descriptor->new_line_x, oled_descriptor->new_line_justify, str);
            }
        }
        
        int16_t pixel_width = ssd1363_put_char(oled_descriptor, *str++, write_to_buffer);
        if(pixel_width < 0)
        {
            return -1;
        }
        else
        {
            string_width += pixel_width;
        }
    }
    
    return string_width;
}

/*! \fn     ssd1363_put_error_string(oled_descriptor_t* oled_descriptor, const cust_char_t* string)
*   \brief  Display an error string on the screen (X0Y0, centered)
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  string              Null terminated string
*   \return How many characters were printed
*/
uint16_t ssd1363_put_error_string(oled_descriptor_t* oled_descriptor, const cust_char_t* string)
{
    ssd1363_put_string_xy(oled_descriptor, 0, 0, OLED_ALIGN_CENTER, string, TRUE);
    return ssd1363_put_string_xy(oled_descriptor, 0, 0, OLED_ALIGN_CENTER, string, FALSE);
}


/*! \fn     ssd1363_add_on_screen_entry_log_entry(oled_descriptor_t* oled_descriptor, const cust_char_t* string)
*   \brief  Add an error string on the screen, scrolling down if needed
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  string              Null terminated string
*   \return How many characters were printed
*/
void ssd1363_add_on_screen_entry_log_entry(oled_descriptor_t* oled_descriptor, const cust_char_t* string)
{
    BOOL redraw_needed = FALSE;

    /* Check if we need to shift the log entries */
    if (oled_descriptor->onscreen_log_entry_ind == ARRAY_SIZE(oled_descriptor->onscreen_log_entry_strings))
    {
        for (uint16_t i = 0; i < ARRAY_SIZE(oled_descriptor->onscreen_log_entry_strings)-1; i++)
        {
            oled_descriptor->onscreen_log_entry_strings[i] = oled_descriptor->onscreen_log_entry_strings[i+1];
        }
        oled_descriptor->onscreen_log_entry_ind--;
        redraw_needed = TRUE;
    }

    /* Store the pointer to the string */
    oled_descriptor->onscreen_log_entry_strings[oled_descriptor->onscreen_log_entry_ind] = string;

    /* Can we just display the missing string? */
    if (redraw_needed == FALSE)
    {
        ssd1363_put_string_xy(oled_descriptor, 0, oled_descriptor->onscreen_log_entry_ind*oled_descriptor->current_font_header.height, OLED_ALIGN_LEFT, string, TRUE);
        ssd1363_put_string_xy(oled_descriptor, 0, oled_descriptor->onscreen_log_entry_ind*oled_descriptor->current_font_header.height, OLED_ALIGN_LEFT, string, FALSE);
    } 
    else
    {
        ssd1363_clear_current_screen(oled_descriptor);
        for (uint16_t i = 0; i < ARRAY_SIZE(oled_descriptor->onscreen_log_entry_strings); i++)
        {
            ssd1363_put_string_xy(oled_descriptor, 0, i*oled_descriptor->current_font_header.height, OLED_ALIGN_LEFT, oled_descriptor->onscreen_log_entry_strings[i], TRUE);
            ssd1363_put_string_xy(oled_descriptor, 0, i*oled_descriptor->current_font_header.height, OLED_ALIGN_LEFT, oled_descriptor->onscreen_log_entry_strings[i], FALSE);
        }
    }

    /* Increment index */
    oled_descriptor->onscreen_log_entry_ind++;
}

/*! \fn     ssd1363_put_centered_string(oled_descriptor_t* oled_descriptor, uint8_t y, const cust_char_t* string, BOOL write_to_buffer)
*   \brief  Display a centered string on the screen
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  y                   Starting y
*   \param  string              Null terminated string
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return Width of the printed string
*/
uint16_t ssd1363_put_centered_string(oled_descriptor_t* oled_descriptor, uint8_t y, const cust_char_t* string, BOOL write_to_buffer) 
{
     return ssd1363_put_string_xy(oled_descriptor, 0, y, OLED_ALIGN_CENTER, string, write_to_buffer);
}

/*! \fn     void ssd1363_put_centered_char(oled_descriptor_t* oled_descriptor, int16_t x, uint16_t y, cust_char_t c, BOOL write_to_buffer)
*   \brief  Display a char centered around an X position
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  x                   Centered around this x
*   \param  y                   Starting y
*   \param  c                   Char to print
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return How many characters were printed
*/
void ssd1363_put_centered_char(oled_descriptor_t* oled_descriptor, int16_t x, uint16_t y, cust_char_t c, BOOL write_to_buffer) 
{
    uint16_t glyph_height;
    int16_t cur_text_x = oled_descriptor->cur_text_x;
    int16_t cur_text_y = (int16_t)oled_descriptor->cur_text_y;
    uint16_t width = ssd1363_get_glyph_width(oled_descriptor, c, &glyph_height);
    
    /* Store cur text x & y */
    oled_descriptor->cur_text_x = x-((width+1)/2);
    oled_descriptor->cur_text_y = y;
   
    /* Display char */
    ssd1363_put_char(oled_descriptor, c, write_to_buffer);

    /* Restore x & y */
    oled_descriptor->cur_text_x = cur_text_x;
    oled_descriptor->cur_text_y = cur_text_y;
}

/*! \fn     ssd1363_set_xy(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y)
*   \brief  Set current text X & Y
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  x   X
*   \param  y   Y
*/
void ssd1363_set_xy(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y)
{
    oled_descriptor->cur_text_x = x;
    oled_descriptor->cur_text_y = y;
}

/*! \fn     ssd1363_get_number_of_printable_characters_for_string(oled_descriptor_t* oled_descriptor, int16_t x, const cust_char_t* string) 
*   \brief  Get the number of characters of a given string that can be printed on the screen
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  x                   Starting x
*   \param  string              Null terminated string
*   \return Number of characters that can be printed
*/
uint16_t ssd1363_get_number_of_printable_characters_for_string(oled_descriptor_t* oled_descriptor, int16_t x, const cust_char_t* string) 
{
    uint16_t nb_characters = 0;
    uint16_t temp_uint16 = 0;
    
    for (nat_type_t ind=0; (string[ind] != 0) && (string[ind] != '\r'); ind++)
    {
        x += ssd1363_get_glyph_width(oled_descriptor, string[ind], &temp_uint16);
        if (x >= oled_descriptor->max_text_x)
        {
            return nb_characters;
        }
        else
        {
            nb_characters++;
        }
    }
    
    return nb_characters;
}

/*! \fn     ssd1363_get_start_x_for_string_based_on_alignment(oled_descriptor_t* oled_descriptor, int16_t x, oled_align_te justify, const cust_char_t* string)
*   \brief  Get the start x for a given string for a given alignment
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  x                   Starting x
*   \param  justify             String justify (see enum)
*   \param  string              Null terminated string
*   \return Start X
*/
int16_t ssd1363_get_start_x_for_string_based_on_alignment(oled_descriptor_t* oled_descriptor, int16_t x, oled_align_te justify, const cust_char_t* string)
{
    uint16_t width = ssd1363_get_string_width(oled_descriptor, string);
    
    if (justify == OLED_ALIGN_CENTER)
    {
        if ((x + oled_descriptor->min_text_x + width) < oled_descriptor->max_text_x)
        {
            x = oled_descriptor->min_text_x + x + (oled_descriptor->max_text_x - oled_descriptor->min_text_x - x - width)/2;
        }
        else
        {
            x = oled_descriptor->min_text_x;
        }
    }
    else if (justify == OLED_ALIGN_RIGHT)
    {
        if (x >= (width + oled_descriptor->min_text_x))
        {
            x -= width;
        }
        else if ((width + oled_descriptor->min_text_x) >= oled_descriptor->max_text_x)
        {
            x = oled_descriptor->min_text_x;
        }
        else
        {
            x = oled_descriptor->max_text_x - width;
        }
    }
    
    return x;    
}

/*! \fn     ssd1363_put_string_xy(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, oled_align_te justify, const char* string, BOOL write_to_buffer) 
*   \brief  Display a string on the screen
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  justify             String justify (see enum)
*   \param  string              Null terminated string
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return Pixel width of the printed string, -1 if too wide for display
*/
int16_t ssd1363_put_string_xy(oled_descriptor_t* oled_descriptor, int16_t x, int16_t y, oled_align_te justify, const cust_char_t* string, BOOL write_to_buffer) 
{    
    /* Store cur text x & y */
    oled_descriptor->cur_text_x = ssd1363_get_start_x_for_string_based_on_alignment(oled_descriptor, x, justify, string);
    oled_descriptor->new_line_justify = justify;
    oled_descriptor->cur_text_y = y;
    oled_descriptor->new_line_x = x;
    
    /* Display string */
    int16_t return_val = ssd1363_put_string(oled_descriptor, string, write_to_buffer);
    
    /* Return the number of characters printed */
    return return_val;
}

/*! \fn     ssd1363_erase_screen_and_put_top_left_emergency_string(oled_descriptor_t* oled_descriptor, const cust_char_t* string)
*   \brief  Display an emergency string on the screen
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  string              Null terminated string
*/
void ssd1363_erase_screen_and_put_top_left_emergency_string(oled_descriptor_t* oled_descriptor, const cust_char_t* string)
{
    int16_t string_width = 0;
    oled_descriptor->cur_text_x = 0;
    oled_descriptor->cur_text_y = 0;
    
    /* Set emergency font, clear string */
    ssd1363_set_emergency_font(oled_descriptor);
    ssd1363_clear_current_screen(oled_descriptor);
    
    /* Use put char for smaller memory footprint */
    while (*string)
    {
        int16_t pixel_width = ssd1363_put_char(oled_descriptor, *string++, FALSE);
        if(pixel_width < 0)
        {
            return;
        }
        else
        {
            string_width += pixel_width;
        }
    }
}

/*! \fn     ssd1363_add_emergency_dot_to_current_position(oled_descriptor_t* oled_descriptor)
*   \brief  Add "." to the current position on screen, used for bootloader progress
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*/
void ssd1363_add_emergency_dot_to_current_position(oled_descriptor_t* oled_descriptor)
{
    oled_descriptor->line_feed_allowed = TRUE;
    ssd1363_put_char(oled_descriptor, '.', FALSE);
    oled_descriptor->line_feed_allowed = FALSE;
}

#ifdef OLED_PRINTF_ENABLED
/*! \fn     ssd1363_printf_xy(oled_descriptor_t* oled_descriptor, int16_t x, uint8_t y, uint8_t justify, BOOL write_to_buffer, const char *fmt, ...) 
*   \brief  Printf string on the display
*   \param  oled_descriptor     Pointer to a ssd1363 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  justify             String justify (see enum)
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return How many characters were printed
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
uint16_t ssd1363_printf_xy(oled_descriptor_t* oled_descriptor, int16_t x, uint8_t y, oled_align_te justify, BOOL write_to_buffer, const char *fmt, ...) 
{
    cust_char_t u16buf[64];
    char buf[64];    
    va_list ap;
    
    va_start(ap, fmt);

    if (vsnprintf(buf, sizeof(buf), fmt, ap) > 0)
    {
        va_end(ap);
        for (uint32_t i = 0; i < sizeof(buf); i++)
        {
            u16buf[i] = buf[i];
        }
    }
    else
    {
        va_end(ap);
        return 0;
    }
    
    /* Store cur text x & y */
    oled_descriptor->cur_text_x = ssd1363_get_start_x_for_string_based_on_alignment(oled_descriptor, x, justify, u16buf);
    oled_descriptor->cur_text_y = y;
    
    /* Display string */
    uint16_t return_val = ssd1363_put_string(oled_descriptor, u16buf, write_to_buffer);
    
    // Return the number of characters printed
    return return_val;
}
#pragma GCC diagnostic pop

#endif
#endif