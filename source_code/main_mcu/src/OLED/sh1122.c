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
/*!  \file     sh1122.c
*    \brief    SH1122 OLED driver
*    Created:  30/12/2017
*    Author:   Mathieu Stephan
*/
#include <stdarg.h>
#include <string.h>
#include <asf.h>
#include "custom_bitstream.h"
#include "driver_sercom.h"
#include "driver_timer.h"
#include "custom_fs.h"
#include "sh1122.h"
#include "dma.h"

#ifdef EMULATOR_BUILD
#include "../EMU/emu_oled.h"
#else
static void emu_oled_flush(void) {}
#endif

/* SH1122 initialization sequence */
static const uint8_t sh1122_init_sequence[] = 
{
    // Interesting theoretical litterature: https://www.osram-os.com/Graphics/XPic2/00032223_0.pdf/4-Bit%20Driver%20Basic%20Register%20Setup.pdf
    // How these values were tweaked: 
    // 1) contrast current set by displaying mooltipass mini picture and matching the 100% intensity with the standard mini display
    // 2) precharge voltage set by displaying greyscale picture and getting nice gamma >> moved to 0 (might want to change back if there's a problem in the future)
    // 3) readjusted contrast current using 1)
    // 4) changing vcomh values doesn't lead to noticeable improvements
    // 5) VSL set by displaying greyscale picture and getting nice gamma >> moved to 1 (might want to change back if there's a problem in the future)
    // 6) readjusted contrast current using 1)
    SH1122_CMD_SET_DISPLAY_OFF,                 0,                      // Set Display Off
    SH1122_CMD_SET_CLOCK_DIVIDER,               1, 0x50,                // Set Display Clock Divide Ratio / Oscillator Frequency: default fosc (512khz) and divide ratio of 1 > Fframe = 512 / 64 / 64 / 1 = 125Hz
    SH1122_CMD_SET_MULTIPLEX_RATIO,             1, 0x3F,                // Mutiplex Ratio To 64
    SH1122_CMD_SET_DISPLAY_OFFSET,              1, 0x00,                // No Display Offset
    SH1122_CMD_SET_ROW_ADDR,                    1, 0x00,                // Row Address Mode Setting
    SH1122_CMD_SET_DISPLAY_START_LINE | 32,     0,                      // Set Display Start Line To 32
    SH1122_CMD_SET_DISCHARGE_VSL_LEVEL | 0x00,  0,                      // VSL = 0.0*Vref
    SH1122_CMD_SET_DCDC_SETTING,                1, 0x80,                // Start Configuring Onboard DCDC
    SH1122_CMD_SET_SEGMENT_REMAP | 0x01,        0,                      // Set Segment Re-map to Reverse Direction
    SH1122_CMD_SET_SCAN_DIRECTION | 0x08,       0,                      // Scan from COM0 to COM[N-1]
    SH1122_CMD_SET_CONTRAST_CURRENT,            1, 0x90,                // Contrast Control Mode Set (up to 0xFF)
    SS1122_CMD_SET_DISCHARGE_PRECHARGE_PERIOD,  1, 0x28,                // Set Discharge/Precharge Period (4bits each)
    SH1122_CMD_SET_VCOM_DESELECT_LEVEL,         1, 0x30,                // VCOMH = (0.430+ A[7:0] X 0.006415) X VREF
    SH1122_CMD_SET_VSEGM_LEVEL,                 1, 0x1e,                // VSEGM = (0.430+ A[7:0] X 0.006415) X VREF
    SH1122_CMD_SET_NORMAL_DISPLAY,              0,                      // Display Bits Normally Interpreted
    SH1122_CMD_SET_HIGH_COLUMN_ADDR,            0,                      // Set Higher Column Address
    SH1122_CMD_SET_LOW_COLUMN_ADDR,             0,                      // Set Lower Column Address
    SH1122_CMD_SET_DISPLAY_OFF_ON | 0x00,       0                       // Normal Display Status, Not Forced to ON
};
static const uint8_t sh1122_init_sequence_inverted[] = 
{
    // Interesting theoretical litterature: https://www.osram-os.com/Graphics/XPic2/00032223_0.pdf/4-Bit%20Driver%20Basic%20Register%20Setup.pdf
    // How these values were tweaked: 
    // 1) contrast current set by displaying mooltipass mini picture and matching the 100% intensity with the standard mini display
    // 2) precharge voltage set by displaying greyscale picture and getting nice gamma >> moved to 0 (might want to change back if there's a problem in the future)
    // 3) readjusted contrast current using 1)
    // 4) changing vcomh values doesn't lead to noticeable improvements
    // 5) VSL set by displaying greyscale picture and getting nice gamma >> moved to 1 (might want to change back if there's a problem in the future)
    // 6) readjusted contrast current using 1)
    SH1122_CMD_SET_DISPLAY_OFF,                 0,                      // Set Display Off
    SH1122_CMD_SET_CLOCK_DIVIDER,               1, 0x50,                // Set Display Clock Divide Ratio / Oscillator Frequency: default fosc (512khz) and divide ratio of 1 > Fframe = 512 / 64 / 64 / 1 = 125Hz
    SH1122_CMD_SET_MULTIPLEX_RATIO,             1, 0x3F,                // Mutiplex Ratio To 64
    SH1122_CMD_SET_DISPLAY_OFFSET,              1, 0x00,                // No Display Offset
    SH1122_CMD_SET_ROW_ADDR,                    1, 0x00,                // Row Address Mode Setting
    SH1122_CMD_SET_DISPLAY_START_LINE,     0,                           // Set Display Start Line To 0
    SH1122_CMD_SET_DISCHARGE_VSL_LEVEL | 0x00,  0,                      // VSL = 0.0*Vref
    SH1122_CMD_SET_DCDC_SETTING,                1, 0x80,                // Start Configuring Onboard DCDC
    SH1122_CMD_SET_SEGMENT_REMAP,        0,                             // Set Segment Re-map to Normal Direction
    SH1122_CMD_SET_SCAN_DIRECTION,       0,                             // Scan from COM[N-1] to COM0
    SH1122_CMD_SET_CONTRAST_CURRENT,            1, 0x90,                // Contrast Control Mode Set (up to 0xFF)
    SS1122_CMD_SET_DISCHARGE_PRECHARGE_PERIOD,  1, 0x28,                // Set Discharge/Precharge Period (4bits each)
    SH1122_CMD_SET_VCOM_DESELECT_LEVEL,         1, 0x30,                // VCOMH = (0.430+ A[7:0] X 0.006415) X VREF
    SH1122_CMD_SET_VSEGM_LEVEL,                 1, 0x1e,                // VSEGM = (0.430+ A[7:0] X 0.006415) X VREF
    SH1122_CMD_SET_NORMAL_DISPLAY,              0,                      // Display Bits Normally Interpreted
    SH1122_CMD_SET_HIGH_COLUMN_ADDR,            0,                      // Set Higher Column Address
    SH1122_CMD_SET_LOW_COLUMN_ADDR,             0,                      // Set Lower Column Address
    SH1122_CMD_SET_DISPLAY_OFF_ON | 0x00,       0                       // Normal Display Status, Not Forced to ON
};


/*! \fn     sh1122_write_single_command(sh1122_descriptor_t* oled_descriptor, uint8_t reg)
*   \brief  Write a single command byte through the SPI
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  data                Byte to be sent
*/
void sh1122_write_single_command(sh1122_descriptor_t* oled_descriptor, uint8_t reg)
{
    PORT->Group[oled_descriptor->sh1122_cs_pin_group].OUTCLR.reg = oled_descriptor->sh1122_cs_pin_mask;
    PORT->Group[oled_descriptor->sh1122_cd_pin_group].OUTCLR.reg = oled_descriptor->sh1122_cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, reg);
    PORT->Group[oled_descriptor->sh1122_cs_pin_group].OUTSET.reg = oled_descriptor->sh1122_cs_pin_mask;
}

/*! \fn     sh1122_write_single_data(sh1122_descriptor_t* oled_descriptor, uint8_t data)
*   \brief  Write a single data byte through the SPI
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  data                Byte to be sent
*/
void sh1122_write_single_data(sh1122_descriptor_t* oled_descriptor, uint8_t data)
{
    PORT->Group[oled_descriptor->sh1122_cs_pin_group].OUTCLR.reg = oled_descriptor->sh1122_cs_pin_mask;
    PORT->Group[oled_descriptor->sh1122_cd_pin_group].OUTSET.reg = oled_descriptor->sh1122_cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, data);
    PORT->Group[oled_descriptor->sh1122_cs_pin_group].OUTSET.reg = oled_descriptor->sh1122_cs_pin_mask;
}

/*! \fn     sh1122_write_single_word(sh1122_descriptor_t* oled_descriptor, uint16_t data)
*   \brief  Write a single word byte through the SPI
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  data                uint16_t to be sent
*/
void sh1122_write_single_word(sh1122_descriptor_t* oled_descriptor, uint16_t data)
{
    PORT->Group[oled_descriptor->sh1122_cs_pin_group].OUTCLR.reg = oled_descriptor->sh1122_cs_pin_mask;
    PORT->Group[oled_descriptor->sh1122_cd_pin_group].OUTSET.reg = oled_descriptor->sh1122_cd_pin_mask;
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, (uint8_t)(data>>8));
    sercom_spi_send_single_byte(oled_descriptor->sercom_pt, (uint8_t)(data&0x00FF));    
    PORT->Group[oled_descriptor->sh1122_cs_pin_group].OUTSET.reg = oled_descriptor->sh1122_cs_pin_mask;
}

/*! \fn     sh1122_start_data_sending(sh1122_descriptor_t* oled_descriptor)
*   \brief  Start data sending mode: assert nCS & CD pin
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_start_data_sending(sh1122_descriptor_t* oled_descriptor)
{
    PORT->Group[oled_descriptor->sh1122_cs_pin_group].OUTCLR.reg = oled_descriptor->sh1122_cs_pin_mask;
    PORT->Group[oled_descriptor->sh1122_cd_pin_group].OUTSET.reg = oled_descriptor->sh1122_cd_pin_mask;    
}

/*! \fn     sh1122_stop_data_sending(sh1122_descriptor_t* oled_descriptor)
*   \brief  Start data sending mode: de-assert nCS
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_stop_data_sending(sh1122_descriptor_t* oled_descriptor)
{
    PORT->Group[oled_descriptor->sh1122_cs_pin_group].OUTSET.reg = oled_descriptor->sh1122_cs_pin_mask;  
}

/*! \fn     sh1122_set_contrast_current(sh1122_descriptor_t* oled_descriptor, uint8_t contrast_current)
*   \brief  Set contrast current for display
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  contrast_current    Contrast current (up to 0xFF)
*/
void sh1122_set_contrast_current(sh1122_descriptor_t* oled_descriptor, uint8_t contrast_current)
{
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_CONTRAST_CURRENT);
    sh1122_write_single_command(oled_descriptor, contrast_current);    
}

/*! \fn     sh1122_set_vcomh_level(sh1122_descriptor_t* oled_descriptor, uint8_t vcomh)
*   \brief  Set vcomh level
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  vcomh               VCOMH level (up to 0xFF)
*/
void sh1122_set_vcomh_level(sh1122_descriptor_t* oled_descriptor, uint8_t vcomh)
{
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_VCOM_DESELECT_LEVEL);
    sh1122_write_single_command(oled_descriptor, vcomh);    
}

/*! \fn     sh1122_set_vsegm_level(sh1122_descriptor_t* oled_descriptor, uint8_t vsegm)
*   \brief  Set vsegm level
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  vsegm               VSEGM level (up to 0xFF)
*/
void sh1122_set_vsegm_level(sh1122_descriptor_t* oled_descriptor, uint8_t vsegm)
{
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_VSEGM_LEVEL);
    sh1122_write_single_command(oled_descriptor, vsegm);    
}

/*! \fn     sh1122_set_discharge_charge_periods(sh1122_descriptor_t* oled_descriptor, uint8_t periods)
*   \brief  Set discharge & charge periods
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  periods             4 bits for discharge, 4 bits for charge
*/
void sh1122_set_discharge_charge_periods(sh1122_descriptor_t* oled_descriptor, uint8_t periods)
{
    sh1122_write_single_command(oled_descriptor, SS1122_CMD_SET_DISCHARGE_PRECHARGE_PERIOD);
    sh1122_write_single_command(oled_descriptor, periods);    
}

/*! \fn     sh1122_set_discharge_vsl_level(sh1122_descriptor_t* oled_descriptor, uint8_t vsl_level)
*   \brief  Set discharge vsl level
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  vsl_level           Discharge VSL level (4 bits max)
*/
void sh1122_set_discharge_vsl_level(sh1122_descriptor_t* oled_descriptor, uint8_t vsl_level)
{
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_DISCHARGE_VSL_LEVEL | vsl_level);
}

/*! \fn     sh1122_set_column_address(sh1122_descriptor_t* oled_descriptor, uint8_t start)
*   \brief  Set a selected column address range
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  start               Start column
*   \param  end                 End column
*/
void sh1122_set_column_address(sh1122_descriptor_t* oled_descriptor, uint8_t start)
{
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_HIGH_COLUMN_ADDR | (start >> 4));
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_LOW_COLUMN_ADDR | (start & 0x0F));
}

/*! \fn     sh1122_set_row_address(sh1122_descriptor_t* oled_descriptor, uint8_t start)
*   \brief  Set a selected column address range
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  start               Start row
*/
void sh1122_set_row_address(sh1122_descriptor_t* oled_descriptor, uint8_t start)
{
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_ROW_ADDR);
    sh1122_write_single_command(oled_descriptor, start);
}

/*! \fn     sh1122_move_display_start_line(sh1122_descriptor_t* oled_descriptor, int16_t offset)
*   \brief  Shift the display start line
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  offset              Y offset for shift
*/
void sh1122_move_display_start_line(sh1122_descriptor_t* oled_descriptor, int16_t offset)
{   
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_DISPLAY_START_LINE | (uint8_t)offset);
}

/*! \fn     sh1122_is_oled_on(sh1122_descriptor_t* oled_descriptor)
*   \brief  Know if OLED is ON
*   \return A boolean
*/
BOOL sh1122_is_oled_on(sh1122_descriptor_t* oled_descriptor)
{
    return oled_descriptor->oled_on;
}

/*! \fn     sh1122_oled_off(sh1122_descriptor_t* oled_descriptor)
*   \brief  Switch on the screen
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_oled_off(sh1122_descriptor_t* oled_descriptor)
{
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_DISPLAY_OFF);
    oled_descriptor->oled_on = FALSE;
}

/*! \fn     sh1122_oled_on(sh1122_descriptor_t* oled_descriptor)
*   \brief  Switch on the screen
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_oled_on(sh1122_descriptor_t* oled_descriptor)
{
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_DISPLAY_ON);
    oled_descriptor->oled_on = TRUE;
}

/*! \fn     sh1122_load_transition(sh1122_descriptor_t* oled_descriptor, oled_transition_te transition)
*   \brief  Load transition when the next frame buffer flush occurs
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  transition          The transition
*/
void sh1122_load_transition(sh1122_descriptor_t* oled_descriptor, oled_transition_te transition)
{
    oled_descriptor->loaded_transition = transition;
}

/*! \fn     sh1122_fade_into_darkness(sh1122_descriptor_t* oled_descriptor, oled_transition_te transition)
*   \brief  Fade display into black
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  transition          The transition
*/
void sh1122_fade_into_darkness(sh1122_descriptor_t* oled_descriptor, oled_transition_te transition)
{
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    sh1122_load_transition(oled_descriptor, transition);
    sh1122_clear_frame_buffer(oled_descriptor);
    sh1122_flush_frame_buffer(oled_descriptor);
    #endif
}

/*! \fn     sh1122_set_min_text_x(sh1122_descriptor_t* oled_descriptor, int16_t x)
*   \brief  Set maximum text X position
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Min text x
*/
void sh1122_set_min_text_x(sh1122_descriptor_t* oled_descriptor, int16_t x)
{
    oled_descriptor->min_text_x = x;
}

/*! \fn     sh1122_set_max_text_x(sh1122_descriptor_t* oled_descriptor, int16_t x)
*   \brief  Set maximum text X position
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Max text x
*/
void sh1122_set_max_text_x(sh1122_descriptor_t* oled_descriptor, int16_t x)
{
    oled_descriptor->max_text_x = x;
}

/*! \fn     sh1122_reset_min_text_x(sh1122_descriptor_t* oled_descriptor)
*   \brief  Reset minimum text X position
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_reset_min_text_x(sh1122_descriptor_t* oled_descriptor)
{
    oled_descriptor->min_text_x = 0;
}

/*! \fn     sh1122_reset_max_text_x(sh1122_descriptor_t* oled_descriptor)
*   \brief  Reset maximum text X position
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_reset_max_text_x(sh1122_descriptor_t* oled_descriptor)
{
    oled_descriptor->max_text_x = SH1122_OLED_WIDTH;
}

/*! \fn     sh1122_set_min_display_y(sh1122_descriptor_t* oled_descriptor, uint16_t y)
*   \brief  Set minimum Y display
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  y                   Min y
*/
void sh1122_set_min_display_y(sh1122_descriptor_t* oled_descriptor, uint16_t y)
{
    if (y > SH1122_OLED_HEIGHT)
    {
        y = SH1122_OLED_HEIGHT;
    }
    oled_descriptor->min_disp_y = y;
}

/*! \fn     sh1122_set_max_display_y(sh1122_descriptor_t* oled_descriptor, uint16_t y)
*   \brief  Set maximum Y display
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  y                   Max y
*/
void sh1122_set_max_display_y(sh1122_descriptor_t* oled_descriptor, uint16_t y)
{
    if (y > SH1122_OLED_HEIGHT)
    {
        y = SH1122_OLED_HEIGHT;
    }
    oled_descriptor->max_disp_y = y;
}

/*! \fn     sh1122_reset_lim_display_y(sh1122_descriptor_t* oled_descriptor)
*   \brief  Reset min/max Y display
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_reset_lim_display_y(sh1122_descriptor_t* oled_descriptor)
{
    oled_descriptor->min_disp_y = 0;
    oled_descriptor->max_disp_y = SH1122_OLED_HEIGHT;
}

/*! \fn     sh1122_allow_line_feed(sh1122_descriptor_t* oled_descriptor)
*   \brief  Allow line feed
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_allow_line_feed(sh1122_descriptor_t* oled_descriptor)
{
    oled_descriptor->line_feed_allowed = TRUE;
}

/*! \fn     sh1122_prevent_line_feed(sh1122_descriptor_t* oled_descriptor)
*   \brief  Prevent line feed
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_prevent_line_feed(sh1122_descriptor_t* oled_descriptor)
{
    oled_descriptor->line_feed_allowed = FALSE;
}

/*! \fn     sh1122_allow_partial_text_y_draw(sh1122_descriptor_t* oled_descriptor)
*   \brief  Allow partial drawing of text in Y
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_allow_partial_text_y_draw(sh1122_descriptor_t* oled_descriptor)
{
    oled_descriptor->allow_text_partial_y_draw = TRUE;
}

/*! \fn     sh1122_prevent_partial_text_x_draw(sh1122_descriptor_t* oled_descriptor)
*   \brief  Prevent partial drawing of text in X
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_prevent_partial_text_x_draw(sh1122_descriptor_t* oled_descriptor)
{
    oled_descriptor->allow_text_partial_x_draw = FALSE;
}

/*! \fn     sh1122_allow_partial_text_x_draw(sh1122_descriptor_t* oled_descriptor)
*   \brief  Allow partial drawing of text in X
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_allow_partial_text_x_draw(sh1122_descriptor_t* oled_descriptor)
{
    oled_descriptor->allow_text_partial_x_draw = TRUE;
}

/*! \fn     sh1122_is_screen_inverted(sh1122_descriptor_t* oled_descriptor)
*   \brief  Know if the screen is inverted
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \return The right boolean
*/
BOOL sh1122_is_screen_inverted(sh1122_descriptor_t* oled_descriptor)
{
    return oled_descriptor->screen_inverted;
}

/*! \fn     sh1122_set_screen_invert(sh1122_descriptor_t* oled_descriptor, BOOL screen_inverted)
*   \brief  Invert the screen
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  screen_inverted     Boolean to invert or not the screen
*/
void sh1122_set_screen_invert(sh1122_descriptor_t* oled_descriptor, BOOL screen_inverted)
{
    if (screen_inverted == FALSE)
    {
        sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_DISPLAY_START_LINE | 32);
        sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_SEGMENT_REMAP | 0x01);
        sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_SCAN_DIRECTION | 0x08);
    } 
    else
    {
        sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_DISPLAY_START_LINE);
        sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_SEGMENT_REMAP);
        sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_SCAN_DIRECTION);
    }
    oled_descriptor->screen_inverted = screen_inverted;
}

/*! \fn     sh1122_prevent_partial_text_y_draw(sh1122_descriptor_t* oled_descriptor)
*   \brief  Prevent partial drawing of text in Y
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_prevent_partial_text_y_draw(sh1122_descriptor_t* oled_descriptor)
{
    oled_descriptor->allow_text_partial_y_draw = FALSE;
}

/*! \fn     sh1122_fill_screen(sh1122_descriptor_t* oled_descriptor, uint8_t color)
*   \brief  Fill the sh1122 screen with a given color
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  color               Color (4 bits value)
*   \note   timed at 8.3ms
*/
void sh1122_fill_screen(sh1122_descriptor_t* oled_descriptor, uint16_t color)
{
    uint8_t fill_color = (uint8_t)((color & 0x000F) | (color << 4));
    uint32_t i;
    
    /* Select a square that fits the complete screen */
    sh1122_set_row_address(oled_descriptor, 0);
    sh1122_set_column_address(oled_descriptor, 0);

    /* Start filling the SSD1322 RAM */
    sh1122_start_data_sending(oled_descriptor);
    for (i = 0; i < SH1122_OLED_HEIGHT * SH1122_OLED_WIDTH / 2; i++)
    {
        sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, fill_color);
    }   
    sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
    sh1122_stop_data_sending(oled_descriptor);
}

/*! \fn     sh1122_clear_current_screen(sh1122_descriptor_t* oled_descriptor)
*   \brief  Clear current selected screen (active or inactive)
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_clear_current_screen(sh1122_descriptor_t* oled_descriptor)
{
    /* Fill screen with 0 pixels */
    sh1122_fill_screen(oled_descriptor, 0);

    /* clear gddram pixels */
    for (uint16_t ind=0; ind < SH1122_OLED_HEIGHT; ind++)
    {
        oled_descriptor->gddram_pixel[ind].xaddr = 0;
        oled_descriptor->gddram_pixel[ind].pixels = 0;
    }

    /* Reset current x & y */
    oled_descriptor->cur_text_x = 0;
    oled_descriptor->cur_text_y = 0;
}

#ifdef OLED_INTERNAL_FRAME_BUFFER
/*! \fn     sh1122_clear_frame_buffer(sh1122_descriptor_t* oled_descriptor)
*   \brief  Clear frame buffer
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_clear_frame_buffer(sh1122_descriptor_t* oled_descriptor)
{
    sh1122_check_for_flush_and_terminate(oled_descriptor);
    memset((void*)oled_descriptor->frame_buffer, 0x00, sizeof(oled_descriptor->frame_buffer));
}

/*! \fn     sh1122_clear_y_frame_buffer(sh1122_descriptor_t* oled_descriptor)
*   \brief  Clear frame buffer between two Y
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  ystart              Start Y
*   \param  yend                End Y (exclusive)
*/
void sh1122_clear_y_frame_buffer(sh1122_descriptor_t* oled_descriptor, uint16_t ystart, uint16_t yend)
{
    if ((ystart >= SH1122_OLED_HEIGHT) || (yend > SH1122_OLED_HEIGHT))
    {
        return;
    }
    
    sh1122_check_for_flush_and_terminate(oled_descriptor);
    memset((void*)&oled_descriptor->frame_buffer[ystart][0], 0x00, (yend-ystart)*SH1122_OLED_WIDTH/2);
}

/*! \fn     sh1122_check_for_flush_and_terminate(sh1122_descriptor_t* oled_descriptor)
*   \brief  Check if a flush is in progress, and wait for its completion if so
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_check_for_flush_and_terminate(sh1122_descriptor_t* oled_descriptor)
{
    /* Check for in progress flush */
    if (oled_descriptor->frame_buffer_flush_in_progress != FALSE)
    {        
        /* Wait for data to be transferred */
        while(dma_oled_check_and_clear_dma_transfer_flag() == FALSE);

        /* Wait for spi buffer to be sent */
        sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
        
        /* Stop sending data */
        sh1122_stop_data_sending(oled_descriptor);
        
        /* Clear bool */
        oled_descriptor->frame_buffer_flush_in_progress = FALSE;
    }
}

/*! \fn     sh1122_flush_frame_buffer_window(sh1122_descriptor_t* oled_descriptor, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
*   \brief  Only flush a small part of our frame buffer
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Window X
*   \param  y                   Window Y
*   \param  width               Box width
*   \param  height              Box height
*   \note   will force x & width to be even
*/
void sh1122_flush_frame_buffer_window(sh1122_descriptor_t* oled_descriptor, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    x = ((x)/2)*2;
    width = ((width+1)/2)*2;
    
    /* Sanity checks */
    if (x >= SH1122_OLED_WIDTH)
    {
        x = SH1122_OLED_WIDTH-1;
    }
    if (y >= SH1122_OLED_HEIGHT)
    {
        y = SH1122_OLED_HEIGHT-1;
    }
    if (x+width > SH1122_OLED_WIDTH)
    {
        width = SH1122_OLED_WIDTH-x;
    }
    if (y+height > SH1122_OLED_HEIGHT)
    {
        height = SH1122_OLED_HEIGHT-y;
    }
    
    /* Display! */
    for (uint16_t i = y; i < y+height; i++)
    {
        sh1122_display_horizontal_pixel_line(oled_descriptor, x, i, width, &oled_descriptor->frame_buffer[i][x/2], FALSE);
    }
}

/*! \fn     sh1122_flush_frame_buffer_y_window(sh1122_descriptor_t* oled_descriptor, uint16_t ystart, uint16_t yend)
*   \brief  Flush frame buffer between two Y
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  ystart              Start Y
*   \param  yend                End Y (exclusive)
*/
void sh1122_flush_frame_buffer_y_window(sh1122_descriptor_t* oled_descriptor, uint16_t ystart, uint16_t yend)
{
    /* Sanity checks */
    if (ystart >= SH1122_OLED_HEIGHT)
    {
        ystart = SH1122_OLED_HEIGHT-1;
    }
    if (yend > SH1122_OLED_HEIGHT)
    {
        yend = SH1122_OLED_HEIGHT;
    }
    
    /* Set pixel write window */
    sh1122_set_row_address(oled_descriptor, ystart);
    sh1122_set_column_address(oled_descriptor, 0);
    
    /* Start filling the SSD1322 RAM */
    sh1122_start_data_sending(oled_descriptor);
    
    /* Send buffer! */
    #ifdef OLED_DMA_TRANSFER        
        dma_oled_init_transfer(oled_descriptor->sercom_pt, (void*)&oled_descriptor->frame_buffer[ystart][0], (yend-ystart)*SH1122_OLED_WIDTH/2, oled_descriptor->dma_trigger_id);
        oled_descriptor->frame_buffer_flush_in_progress = TRUE;
    #else
        for (uint32_t y = ystart; y < yend; y++) 
        {
            for (uint32_t x = 0; x < SH1122_OLED_WIDTH/2; x++) {
                sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, oled_descriptor->frame_buffer[y][x]);
            }
        }
    #endif
}

/*! \fn     sh1122_flush_frame_buffer(sh1122_descriptor_t* oled_descriptor)
*   \brief  Flush frame buffer to screen
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_flush_frame_buffer(sh1122_descriptor_t* oled_descriptor)
{
    /* Wait for a possible ongoing previous flush */
    sh1122_check_for_flush_and_terminate(oled_descriptor);
    
    if (oled_descriptor->loaded_transition == OLED_TRANS_NONE)
    {        
        /* Set pixel write window */
        sh1122_set_row_address(oled_descriptor, 0);
        sh1122_set_column_address(oled_descriptor, 0);
        
        /* Start filling the SSD1322 RAM */
        sh1122_start_data_sending(oled_descriptor);
        
        /* Send buffer! */
        #ifdef OLED_DMA_TRANSFER        
            dma_oled_init_transfer(oled_descriptor->sercom_pt, (void*)&oled_descriptor->frame_buffer[0][0], sizeof(oled_descriptor->frame_buffer), oled_descriptor->dma_trigger_id);
            oled_descriptor->frame_buffer_flush_in_progress = TRUE;
        #else
            for (uint32_t y = 0; y < SH1122_OLED_HEIGHT; y++) 
            {
                for (uint32_t x = 0; x < SH1122_OLED_WIDTH/2; x++) {
                    sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, oled_descriptor->frame_buffer[y][x]);
                }
            }
        #endif
    }
    else if (oled_descriptor->loaded_transition == OLED_LEFT_RIGHT_TRANS)
    {
        uint8_t pixel_data[2];
        
        /* Left to right */
        for (uint16_t x = 0; x < SH1122_OLED_WIDTH/2; x++)
        {
            for (uint16_t y = 0; y < SH1122_OLED_HEIGHT; y++)
            {
                pixel_data[0] = (oled_descriptor->frame_buffer[y][x]);
                pixel_data[1] = SH1122_TRANSITION_PIXEL;
                
                if (x*2 + 2 < SH1122_OLED_WIDTH)
                {
                    sh1122_display_horizontal_pixel_line(oled_descriptor, x*2, y, 4, pixel_data, FALSE);
                }
                else
                {
                    sh1122_display_horizontal_pixel_line(oled_descriptor, x*2, y, 2, pixel_data, FALSE);                    
                }
            }
            emu_oled_flush();
        }
    }
    else if (oled_descriptor->loaded_transition == OLED_RIGHT_LEFT_TRANS)
    {
        uint8_t pixel_data[2];
        
        /* Right to left */
        for (int16_t x = (SH1122_OLED_WIDTH/2)-2; x >= -1; x--)
        {
            for (uint16_t y = 0; y < SH1122_OLED_HEIGHT; y++)
            {
                pixel_data[0] = SH1122_TRANSITION_PIXEL<<4;
                pixel_data[1] = (oled_descriptor->frame_buffer[y][x+1]);
                
                if (x > 0)
                {
                    sh1122_display_horizontal_pixel_line(oled_descriptor, x*2, y, 4, pixel_data, FALSE);
                }
                else
                {
                    sh1122_display_horizontal_pixel_line(oled_descriptor, x*2 + 2, y, 2, pixel_data+1, FALSE);
                }
            }
            emu_oled_flush();
        }
    }
    else if (oled_descriptor->loaded_transition == OLED_TOP_BOT_TRANS)
    {
        /* Top to bottom */
        for (uint16_t y = 0; y < SH1122_OLED_HEIGHT; y++)
        {
            sh1122_display_horizontal_pixel_line(oled_descriptor, 0, y, SH1122_OLED_WIDTH, &(oled_descriptor->frame_buffer[y][0]), FALSE);
            if (y+2 < SH1122_OLED_HEIGHT)
            {                
                /* Dotted line */
                sh1122_draw_rectangle(oled_descriptor, 0, y+2, SH1122_OLED_WIDTH, 1, SH1122_TRANSITION_PIXEL, FALSE);
            }
            emu_oled_flush();
            DELAYMS(2);
        }
    }
    else if (oled_descriptor->loaded_transition == OLED_BOT_TOP_TRANS)
    {
        /* Bottom to top */
        for (int16_t y = SH1122_OLED_HEIGHT-1; y >= 0; y--)
        {
            sh1122_display_horizontal_pixel_line(oled_descriptor, 0, y, SH1122_OLED_WIDTH, &(oled_descriptor->frame_buffer[y][0]), FALSE);
            if (y-2 >= 0)
            {
                /* Dotted line */
                sh1122_draw_rectangle(oled_descriptor, 0, y-2, SH1122_OLED_WIDTH, 1, SH1122_TRANSITION_PIXEL, FALSE);
            }
            emu_oled_flush();
            DELAYMS(2);
        }
    }
    else if (oled_descriptor->loaded_transition == OLED_IN_OUT_TRANS)
    {
        uint16_t low_y = SH1122_OLED_HEIGHT/2 - 1;
        uint16_t high_y = SH1122_OLED_HEIGHT/2;
        
        /* Window IN to OUT */
        for (uint16_t i = 1; i <= SH1122_OLED_WIDTH/4; i++)
        {
            uint16_t x_pos = (SH1122_OLED_WIDTH/2)-2*i;
            uint16_t x_pos2 = (SH1122_OLED_WIDTH/2)+2*i-2;
            for (uint16_t y = low_y; y <= high_y; y++)
            {
                sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos, y, 2, &(oled_descriptor->frame_buffer[y][x_pos/2]), FALSE);
                sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos2, y, 2, &(oled_descriptor->frame_buffer[y][x_pos2/2]), FALSE);
            }
            sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos, low_y, x_pos2-x_pos, &(oled_descriptor->frame_buffer[low_y][x_pos/2]), FALSE);
            sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos, high_y, x_pos2-x_pos, &(oled_descriptor->frame_buffer[high_y][x_pos/2]), FALSE);
            
            if ((i & 0x01) == 0)
            {
                low_y--;
                high_y++;
            }
            emu_oled_flush();
        }
    }
    else if (oled_descriptor->loaded_transition == OLED_OUT_IN_TRANS)
    {
        uint16_t low_y = 0;
        uint16_t high_y = SH1122_OLED_HEIGHT-1;
        
        /* Window IN to OUT */
        for (uint16_t i = SH1122_OLED_WIDTH/4; i > 0; i--)
        {
            uint16_t x_pos = (SH1122_OLED_WIDTH/2)-2*i;
            uint16_t x_pos2 = (SH1122_OLED_WIDTH/2)+2*i-2;
            for (uint16_t y = low_y; y <= high_y; y++)
            {
                sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos, y, 2, &(oled_descriptor->frame_buffer[y][x_pos/2]), FALSE);
                sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos2, y, 2, &(oled_descriptor->frame_buffer[y][x_pos2/2]), FALSE);
            }
            sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos, low_y, x_pos2-x_pos, &(oled_descriptor->frame_buffer[low_y][x_pos/2]), FALSE);
            sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos, high_y, x_pos2-x_pos, &(oled_descriptor->frame_buffer[high_y][x_pos/2]), FALSE);
            
            if ((i & 0x01) == 1)
            {
                low_y++;
                high_y--;
            }
            emu_oled_flush();
        }
    }
    
    /* Reset transition */
    oled_descriptor->loaded_transition = OLED_TRANS_NONE;
    emu_oled_flush();
}
#endif

/*! \fn     sh1122_set_emergency_font(void)
*   \brief  Use the flash-stored emergency font (ascii only)
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_set_emergency_font(sh1122_descriptor_t* oled_descriptor)
{
    oled_descriptor->currentFontAddress = CUSTOM_FS_EMERGENCY_FONT_FILE_ADDR;
    custom_fs_read_from_flash((uint8_t*)&oled_descriptor->current_font_header, oled_descriptor->currentFontAddress, sizeof(oled_descriptor->current_font_header));
    custom_fs_read_from_flash((uint8_t*)&oled_descriptor->current_unicode_inters, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header), sizeof(oled_descriptor->current_unicode_inters));
}

/*! \fn     sh1122_refresh_used_font(sh1122_descriptor_t* oled_descriptor, uint16_t font_id)
*   \brief  Refreshed used font (in case of init or language change)
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  font_id             Font ID to use
*   \return Success status
*/
RET_TYPE sh1122_refresh_used_font(sh1122_descriptor_t* oled_descriptor, uint16_t font_id)
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

/*! \fn     sh1122_get_current_font_height(sh1122_descriptor_t oled_descriptor)
*   \brief  Get current font height
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \return Font height
*/
uint8_t sh1122_get_current_font_height(sh1122_descriptor_t* oled_descriptor)
{
    return oled_descriptor->current_font_header.height;
}

/*! \fn     sh1122_init_display(sh1122_descriptor_t oled_descriptor, BOOL leave_internal_logic_and_reflush_frame_buffer)
*   \brief  Initialize a SSD1322 display
*   \param  oled_descriptor                                 Pointer to a sh1122 descriptor struct
*   \param  leave_internal_logic_and_reflush_frame_buffer   Set to TRUE to do what it says...
*/
void sh1122_init_display(sh1122_descriptor_t* oled_descriptor, BOOL leave_internal_logic_and_reflush_frame_buffer)
{
    /* Vars init : should already be to 0 but you never know... */
    if (leave_internal_logic_and_reflush_frame_buffer == FALSE)
    {
        oled_descriptor->allow_text_partial_y_draw = FALSE;
        oled_descriptor->allow_text_partial_x_draw = FALSE;
        oled_descriptor->screen_wrapping_allowed = FALSE;
        oled_descriptor->carriage_return_allowed = FALSE;
        oled_descriptor->line_feed_allowed = FALSE;
        oled_descriptor->currentFontAddress = 0;
        oled_descriptor->max_text_x = SH1122_OLED_WIDTH;
        oled_descriptor->min_text_x = 0;
        oled_descriptor->max_disp_x = SH1122_OLED_WIDTH;
        oled_descriptor->max_disp_y = SH1122_OLED_HEIGHT;
        oled_descriptor->min_disp_y = 0;
    }
    
    /* Different init sequences based on screen inversion */
    const uint8_t* init_seq = sh1122_init_sequence;
    if (oled_descriptor->screen_inverted != FALSE)
    {
        init_seq = sh1122_init_sequence_inverted;
    }
    
    /* Send the initialization sequence through SPI */
    for (uint16_t ind = 0; ind < sizeof(sh1122_init_sequence);)
    {
        /* nCS set */
        PORT->Group[oled_descriptor->sh1122_cs_pin_group].OUTCLR.reg = oled_descriptor->sh1122_cs_pin_mask;
        PORT->Group[oled_descriptor->sh1122_cd_pin_group].OUTCLR.reg = oled_descriptor->sh1122_cd_pin_mask;
        
        /* First byte: command */
        sercom_spi_send_single_byte(oled_descriptor->sercom_pt, init_seq[ind++]);
        
        /* Second byte: payload length */
        uint16_t dataSize = init_seq[ind++];
        
        /* If different than 0, send payload */
        while (dataSize--)
        {
            sercom_spi_send_single_byte(oled_descriptor->sercom_pt, init_seq[ind++]);
        }
        
        /* nCS release */
        PORT->Group[oled_descriptor->sh1122_cs_pin_group].OUTSET.reg = oled_descriptor->sh1122_cs_pin_mask;
        asm("NOP");asm("NOP");
    }

    /* Clear display */
    if (leave_internal_logic_and_reflush_frame_buffer == FALSE)
    {
        sh1122_clear_current_screen(oled_descriptor);
        #ifdef OLED_INTERNAL_FRAME_BUFFER
        memset((void*)oled_descriptor->frame_buffer, 0x00, sizeof(oled_descriptor->frame_buffer));
        oled_descriptor->frame_buffer_flush_in_progress = FALSE;
        #endif
    }
    else
    {
        /* Fill screen with 0 pixels */
        sh1122_fill_screen(oled_descriptor, 0);
    }
    
    /* Set emergency font by default */
    sh1122_set_emergency_font(oled_descriptor);
    
    /* From datasheet : wait 100ms */
    timer_delay_ms(100);

    /* Switch screen on */
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_DISPLAY_ON);
    oled_descriptor->oled_on = TRUE;
    
    /* Reflush frame buffer if asked to */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    if (leave_internal_logic_and_reflush_frame_buffer != FALSE)
    {
        timer_delay_ms(50);
        oled_descriptor->loaded_transition = OLED_TRANS_NONE;
        sh1122_flush_frame_buffer(oled_descriptor);
        sh1122_check_for_flush_and_terminate(oled_descriptor);
    }
    #endif
}

/*! \fn     sh1122_draw_vertical_line(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t ystart, int16_t yend, uint8_t color, BOOL write_to_buffer)
*   \brief  Draw a vertical line on the display
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  ystart              Starting y
*   \param  yend                Ending y
*   \param  color               4 bits Color
*   \param  write_to_buffer     Set to true to write to internal buffer
*/
void sh1122_draw_vertical_line(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t ystart, int16_t yend, uint8_t color, BOOL write_to_buffer)
{
    uint16_t xoff = x - (x / 2) * 2;
    ystart = ystart<0?0:ystart;
    
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    if (write_to_buffer != FALSE)
    {
        for (int16_t y=ystart; y<=yend; y++)
        {
            uint8_t pixels = color << 4;

            /* Start x not a multiple of 2 */
            if (xoff != 0)
            {
                pixels = color;
            }
            
            /* Fill frame buffer */
            oled_descriptor->frame_buffer[y][x/2] |= pixels;
        }
    } 
    else
    {
    #endif
    for (int16_t y=ystart; y<=yend; y++)
    {
        uint8_t pixels = color << 4;
        
        /* Set pixel write window */
        sh1122_set_row_address(oled_descriptor, y);
        sh1122_set_column_address(oled_descriptor, x/2);
        
        /* Start filling the SSD1322 RAM */
        sh1122_start_data_sending(oled_descriptor);

        /* Start x not a multiple of 2 */
        if (xoff != 0)
        {
            pixels = color;

            /* Fill existing pixels if available */
            if ((x/2) == oled_descriptor->gddram_pixel[y].xaddr)
            {
                pixels |= oled_descriptor->gddram_pixel[y].pixels;
            }
        }

        /* Send the 2 pixels to the display */
        sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)(pixels & 0x00FF));
        
        /* Store pixel data in our gddram buffer for later merging */
        oled_descriptor->gddram_pixel[y].pixels = (uint8_t)pixels;
        oled_descriptor->gddram_pixel[y].xaddr = x/2;
            
        /* Wait for spi buffer to be sent */
        sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
            
        /* Stop sending data */
        sh1122_stop_data_sending(oled_descriptor);
    }
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    }
    #endif
} 

/*! \fn     sh1122_display_horizontal_pixel_line(sh1122_descriptor_t* oled_descriptor, uint16_t x, uint16_t y, uint16_t width, uint8_t* pixels, BOOL write_to_buffer)
*   \brief  Display adjacent pixels at a given position, handles wrapping around the screen
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   X position
*   \param  y                   Y position
*   \param  width               Line width
*   \param  pixels              Pointer to data buffer
*   \param  write_to_buffer     Set to something else than FALSE to write to buffer
*   \note   Boundary checks on X are supposed to be done before calling this function. X must be between -SH1122_OLED_WIDTH and SH1122_OLED_WIDTH
*   \note   When not writing to buffer, only odd X are supported and between 0 & SH1122_OLED_WIDTH!
*/
void sh1122_display_horizontal_pixel_line(sh1122_descriptor_t* oled_descriptor, int16_t x, uint16_t y, uint16_t width, uint8_t* pixels, BOOL write_to_buffer)
{    
    /* Check for correct Y */
    if ((y < oled_descriptor->min_disp_y) || (y >= oled_descriptor->max_disp_y))
    {
        return;
    }
    
#ifdef OLED_INTERNAL_FRAME_BUFFER
    if (write_to_buffer != FALSE)
    {
        /* Previous pixels in case we are shifted */
        uint8_t prev_pixels = 0x00;
        
        /* Boolean to mention if pixel to be written is the first one in the buffer */
        BOOL pixel_shift = FALSE;
        
        /* Check for negative x */
        if (x < 0)
        {
            /* nb pixels to write */
            uint16_t nb_pixels_to_be_written = -x>=width?width:-x;
            
            /* Wrapping allowed? */
            if (oled_descriptor->screen_wrapping_allowed != FALSE)
            {
                /* Check for 2 pixels alignment */
                if ((x%2) == 0)
                {
                    memcpy(&oled_descriptor->frame_buffer[y][x/2+SH1122_OLED_WIDTH/2], pixels, (nb_pixels_to_be_written/2));
                    pixels -= x/2;
                }
                else
                {
                    /* We get pixel shift! */
                    pixel_shift = TRUE;

                    /* Not 2 pixels aligned, loop */
                    for (int16_t i = x; i < x+nb_pixels_to_be_written; i+=2)
                    {
                        oled_descriptor->frame_buffer[y][(i+SH1122_OLED_WIDTH)/2] = (*pixels >> 4) | (prev_pixels << 4);
                        prev_pixels = *pixels;
                        pixels++;
                    }
                }
                
                /* Did we miss the last pixel? */
                if ((x + nb_pixels_to_be_written)%2 != 0)
                {
                    if (pixel_shift != FALSE)
                    {
                        oled_descriptor->frame_buffer[y][(x+nb_pixels_to_be_written+SH1122_OLED_WIDTH)/2] = prev_pixels << 4;
                    } 
                    else
                    {
                        oled_descriptor->frame_buffer[y][(x+nb_pixels_to_be_written+SH1122_OLED_WIDTH)/2] = *pixels;
                    }
                }
            } 
            else
            {
                /* remove off screen pixels */
                pixels -= (x-1)/2;
                if ((x%2) != 0)
                {
                    pixel_shift = TRUE;
                    prev_pixels = *(pixels-1);
                }
            }     
            
            /* Check for end of display */
            if (nb_pixels_to_be_written == width)
            {
                return;
            }
                   
            /* Update x to 0, reduce width */
            width += x;
            x = 0;
        }
        
        /* Now x is 0 or less than SH1122_OLED_WIDTH */
        uint16_t nb_pixels_to_be_written = (x+width)>=oled_descriptor->max_disp_x?(oled_descriptor->max_disp_x-x):width;
        
        /* Check for 2 pixels alignment */
        if ((x%2 == 0) && (pixel_shift == FALSE))
        {            
            memcpy(&oled_descriptor->frame_buffer[y][x/2], pixels, nb_pixels_to_be_written/2);
            pixels += nb_pixels_to_be_written/2;
        } 
        else
        {
            /* We get pixel shift! */
            pixel_shift = TRUE;
            
            /* Not 2 pixels aligned, loop */
            for (int16_t i = x; i < x+nb_pixels_to_be_written; i+=2)
            {
                oled_descriptor->frame_buffer[y][i/2] = (*pixels >> 4) | (prev_pixels << 4);
                prev_pixels = *pixels;
                pixels++;
            }
        }
        
        /* Did we miss the last pixel? */
        if ((x + nb_pixels_to_be_written)%2 != 0)
        {
            if (pixel_shift != FALSE)
            {
                if (x != 0)
                {
                    /* I wish I could explain you... */
                    oled_descriptor->frame_buffer[y][(x+nb_pixels_to_be_written)/2] = prev_pixels << 4;
                }
            } 
            else
            {
                oled_descriptor->frame_buffer[y][(x+nb_pixels_to_be_written)/2] = *pixels;
            }
        }
            
        /* Update x and width */
        width -= nb_pixels_to_be_written;
        x += nb_pixels_to_be_written;
        
        /* Out of screen and wrapping? */
        if ((width > 0) && (oled_descriptor->screen_wrapping_allowed != FALSE))
        {
            x-= SH1122_OLED_WIDTH;
            nb_pixels_to_be_written = (x+width)>=SH1122_OLED_WIDTH?(SH1122_OLED_WIDTH-x-1):width;
            
            /* Check for 2 pixels alignment */
            if (pixel_shift == FALSE)
            {
                memcpy(&oled_descriptor->frame_buffer[y][0], pixels, nb_pixels_to_be_written/2);
                pixels += nb_pixels_to_be_written/2;
            } 
            else
            {
                /* Not 2 pixels aligned, loop */
                for (int16_t i = 0; i < width; i+=2)
                {
                    oled_descriptor->frame_buffer[y][i/2] = (*pixels >> 4) | (prev_pixels << 4);
                    prev_pixels = *pixels;
                    pixels++;
                }
            }
        
            /* Did we miss the last pixel? */
            if ((x + nb_pixels_to_be_written)%2 != 0)
            {
                if (pixel_shift != FALSE)
                {
                    oled_descriptor->frame_buffer[y][(x+nb_pixels_to_be_written)/2] = prev_pixels << 4;
                } 
                else
                {
                    oled_descriptor->frame_buffer[y][(x+nb_pixels_to_be_written)/2] = *pixels;
                }
            }
        }
    } 
    else
#endif
    {
        /* Set pixel write window */
        sh1122_set_row_address(oled_descriptor, y);
        sh1122_set_column_address(oled_descriptor, x/2);
        
        /* Start filling the SSD1322 RAM */
        sh1122_start_data_sending(oled_descriptor);

        /* Send line */
        for (uint16_t i = 0; i < width/2; i++)
        {
            sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, *pixels);
            pixels++;
        }
        
        /* Wait for spi buffer to be sent */
        sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
        
        /* Stop sending data */
        sh1122_stop_data_sending(oled_descriptor);
    }
}   

/*! \fn     sh1122_draw_rectangle(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color, BOOL write_to_buffer)
*   \brief  Draw a rectangle on the screen
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  width               Width
*   \param  height              Height
*   \param  color               4 bits color
*   \param  write_to_buffer     Set to something else than FALSE to write to buffer
*   \note   No checks done on X & Y & width & height!
*/
void sh1122_draw_rectangle(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color, BOOL write_to_buffer)
{
    uint16_t xoff = x - (x / 2) * 2;

    #ifdef OLED_INTERNAL_FRAME_BUFFER
    if (write_to_buffer != FALSE)
    {
        for (uint16_t yind = 0; yind < height; yind++)
        {
            uint16_t xind = 0;

            /* Start x not a multiple of 2 */
            if (xoff != 0)
            {
                /* Set xind to 1 as we're writing a pixel */
                xind = 1;
                
                /* one pixel */
                oled_descriptor->frame_buffer[y+yind][(x+xind)/2] &= 0xF0;
                oled_descriptor->frame_buffer[y+yind][(x+xind)/2] |= color;
            }
            
            /* Start x multiple of 2, start filling */
            for (; xind < width; xind+=2)
            {
                if ((xind+2) <= width)
                {
                    oled_descriptor->frame_buffer[y+yind][(x+xind)/2] = color | (color << 4);
                }
                else
                {
                    oled_descriptor->frame_buffer[y+yind][(x+xind)/2] &= 0x0F;
                    oled_descriptor->frame_buffer[y+yind][(x+xind)/2] |= color << 4;
                }
            }
        }
    }
    else
    {
        #endif
        for (uint16_t yind=0; yind < height; yind++)
        {
            uint16_t xind = 0;
            uint16_t pixels = 0;
        
            /* Set pixel write window */
            sh1122_set_row_address(oled_descriptor, y+yind);
            sh1122_set_column_address(oled_descriptor, x/2);
        
            /* Start filling the SSD1322 RAM */
            sh1122_start_data_sending(oled_descriptor);

            /* Start x not a multiple of 2 */
            if (xoff != 0)
            {
                /* Set xind to 1 as we're writing a pixel */
                xind = 1;
            
                /* one pixel */
                pixels = color;

                /* Fill existing pixels if available */
                if ((x/2) == oled_descriptor->gddram_pixel[y+yind].xaddr)
                {
                    pixels |= oled_descriptor->gddram_pixel[y+yind].pixels;
                }

                /* Send the 2 pixels to the display */
                sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)(pixels & 0x00FF));
            }
        
            /* Start x multiple of 2, start filling */
            for (; xind < width; xind+=2)
            {
                if ((xind+2) <= width)
                {
                    pixels = color | (color << 4);
                }
                else
                {
                    pixels = color << 4;
                }
            
                // Send 2 pixels to the display
                sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)(pixels & 0x00FF));
            }
        
            /* Store pixel data in our gddram buffer for later merging */
            if (pixels != 0)
            {
                oled_descriptor->gddram_pixel[y+yind].pixels = (uint8_t)pixels;
                oled_descriptor->gddram_pixel[y+yind].xaddr = (x+width-1)/2;
            }
        
            /* Wait for spi buffer to be sent */
            sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
        
            /* Stop sending data */
            sh1122_stop_data_sending(oled_descriptor);
        }
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    }
    #endif
}

/*! \fn     sh1122_draw_full_screen_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, bitstream_bitmap_t* bitstream)
*   \brief  Draw a full screen picture from a bitstream
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  bitstream           Pointer to the bistream
*/
void sh1122_draw_full_screen_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, bitstream_bitmap_t* bitstream)
{
    /*  So, here's a quick overview if you were to wonder what has been done to improve display speeds:
    /   Note: the FPS count mentioned here highly depends on the picture itself due to RLE compression
    /   Using internal flash :
    /   1) No defines: 28fps
    /   2) OLED_DMA_TRANSFER: 43fps
    /   Using external flash :
    /   1) FLASH_ALONE_ON_SPI_BUS: when defined, reads from the flash are done in a continuous manner, rather than doing multiple reads for different chunks of data: 20fps
    /   2) FLASH_DMA_FETCHES: when defined, reads from the flash are done using the DMA controller: 26fps
    /   3) OLED_DMA_TRANSFER: when defined, writes to the oled are done using the DMA controller: 40fps
    /   Note: more or less no performance improvements have been found by overclocking oled spi clk
    */

    #ifdef OLED_INTERNAL_FRAME_BUFFER
    /* Wait for a possible ongoing previous flush */
    sh1122_check_for_flush_and_terminate(oled_descriptor);
    #endif

    /* Set pixel write window */
    sh1122_set_row_address(oled_descriptor, 0);
    sh1122_set_column_address(oled_descriptor, 0);
    
    /* Start filling the SSD1322 RAM */
    sh1122_start_data_sending(oled_descriptor);
    
    /* Depending if we use DMA transfers */
    #ifdef OLED_DMA_TRANSFER        
        uint8_t pixel_buffer[2][32];
        uint32_t buffer_sel = 0;
        
        /* Get things going: start first transfer then enter the for(), as we need to wait for OLED DMA after inside the loop */
        bitstream_bitmap_array_read(bitstream, pixel_buffer[buffer_sel], sizeof(pixel_buffer[0])*2);
        dma_oled_init_transfer(oled_descriptor->sercom_pt, (void*)pixel_buffer[buffer_sel], sizeof(pixel_buffer[0]), oled_descriptor->dma_trigger_id);
        
        for (uint32_t i = 0; i < (SH1122_OLED_WIDTH*SH1122_OLED_HEIGHT) - sizeof(pixel_buffer[0])*2; i+=sizeof(pixel_buffer[0])*2)
        {            
            /* Read from bitstream in the next buffer */
            bitstream_bitmap_array_read(bitstream, pixel_buffer[(buffer_sel+1)&0x01], sizeof(pixel_buffer[0])*2);
            
            /* Wait for transfer done */
            while(dma_oled_check_and_clear_dma_transfer_flag() == FALSE);
            
            /* Init DMA transfer */
            buffer_sel = (buffer_sel+1) & 0x01;
            dma_oled_init_transfer(oled_descriptor->sercom_pt, (void*)pixel_buffer[buffer_sel], sizeof(pixel_buffer[0]), oled_descriptor->dma_trigger_id);
        }
        
        /* Wait for data to be transferred */
        while(dma_oled_check_and_clear_dma_transfer_flag() == FALSE);
    #else        
        uint8_t pixel_buffer[16];
        
        /* Send all pixels */
        for (uint32_t i = 0; i < (SH1122_OLED_WIDTH*SH1122_OLED_HEIGHT); i+=sizeof(pixel_buffer)*2)
        {
            /* Read from bitstream */
            bitstream_bitmap_array_read(bitstream, pixel_buffer, sizeof(pixel_buffer)*2);
            
            /* Send pixels */
            for (uint32_t j = 0; j < sizeof(pixel_buffer); j++)
            {
                sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, pixel_buffer[j]);
            }
        }
    #endif
    
    /* Wait for spi buffer to be sent */
    sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
    
    /* Stop sending data */
    sh1122_stop_data_sending(oled_descriptor);
    
    /* Close bitstream */
    bitstream_bitmap_close(bitstream);
}

/*! \fn     sh1122_draw_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_t* bs, BOOL write_to_buffer)
*   \brief  Draw a picture from a bitstream
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  bitstream           Pointer to the bitstream
*   \param  write_to_buffer     Set to true to write to internal buffer
*/
void sh1122_draw_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_bitmap_t* bitstream, BOOL write_to_buffer)
{
    /* Check for off screen line on the left */
    if (((x < 0) && (-x >= bitstream->width) && (oled_descriptor->screen_wrapping_allowed == FALSE)) || (x < -SH1122_OLED_WIDTH))
    {
        return;
    }
    
    /* X off screen, remove one OLED width if wrap enabled */
    if ((x >= oled_descriptor->max_disp_x) && (oled_descriptor->screen_wrapping_allowed != FALSE))
    {
        x -= oled_descriptor->max_disp_x;
    }
    
    /* Check for off screen line on the right */
    if (x >= oled_descriptor->max_disp_x)
    {
        return;
    }

    /* Use different drawing methods if it's a full screen picture and if we are 2 pixels aligned */
    if ((x == 0) && (y == 0) && (bitstream->width == SH1122_OLED_WIDTH) && (bitstream->height == SH1122_OLED_HEIGHT) && (oled_descriptor->max_disp_y == SH1122_OLED_HEIGHT) && (write_to_buffer == FALSE))
    {
        /* Dedicated code to allow faster write to display */
        sh1122_draw_full_screen_image_from_bitstream(oled_descriptor, bitstream);        
    }
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    else if (write_to_buffer != FALSE)
    {
        /* Buffer large enough to contain a display line */
        uint8_t pixel_buffer[(SH1122_OLED_WIDTH/2)+1];
        
        /* Check for buffer overflow */
        if ((bitstream->width/2) > sizeof(pixel_buffer) - 1)
        {
            return;
        }
        
        /* In some cases the routine does an extra read, depending on alignment */
        pixel_buffer[bitstream->width/2] = 0;

        /* Wait for a possible ongoing previous flush */
        sh1122_check_for_flush_and_terminate(oled_descriptor);
        
        /* Lines loop */
        for (int16_t i = 0; i < bitstream->height; i++)
        {            
            bitstream_bitmap_array_read(bitstream, pixel_buffer, bitstream->width);
            
            /* Check for on screen */
            if ((y+i >= oled_descriptor->min_disp_y) && (y+i < oled_descriptor->max_disp_y))
            {
                sh1122_display_horizontal_pixel_line(oled_descriptor, x, y+i, bitstream->width, pixel_buffer, write_to_buffer);
            }
        }
        
        /* Close bitstream */
        bitstream_bitmap_close(bitstream);    
    }
    #endif
    #ifdef OLED_DMA_TRANSFER
    else if ((bitstream->width % 2 == 0) && (x % 2 == 0) && (oled_descriptor->max_disp_x % 2 == 0))
    {
        /* Buffer large enough to contain a display line in order to trig one DMA transfer */
        uint8_t pixel_buffer[2][SH1122_OLED_WIDTH/2];
        uint16_t buffer_sel = 0;

        /* X offset for display in case of wrapping */
        int16_t x_offset_wrap = 0;

        /* Offset in data array */
        uint16_t pixel_array_offset = 0;

        /* Number of pixels to send per line */
        uint16_t nb_pixels_to_send = bitstream->width;

        /* Negative X */
        if (x < 0)
        {
            if (oled_descriptor->screen_wrapping_allowed != FALSE)
            {
                /* Dirty trick: let the screen do the wrapping for you */
                x_offset_wrap = SH1122_OLED_WIDTH;
            }
            else
            {
                pixel_array_offset = -x/2;
                nb_pixels_to_send += x;
                x = 0;
            }
        }

        /* Bitmap over screen edge on the right */
        if ((x + nb_pixels_to_send > oled_descriptor->max_disp_x) && (oled_descriptor->screen_wrapping_allowed == FALSE))
        {
            nb_pixels_to_send = oled_descriptor->max_disp_x-x;
        }

        #ifdef OLED_INTERNAL_FRAME_BUFFER
        /* Wait for a possible ongoing previous flush */
        sh1122_check_for_flush_and_terminate(oled_descriptor);
        #endif

        /* Trigger first buffer fill: if we asked more data, the bitstream will return 0s */
        bitstream_bitmap_array_read(bitstream, pixel_buffer[buffer_sel], bitstream->width);

        /* Scan Y */
        for (uint16_t j = 0; j < bitstream->height; j++)
        {            
            if ((y+j >= oled_descriptor->min_disp_y) && (y+j <= oled_descriptor->max_disp_y))
            {
                /* Set pixel write window */
                sh1122_set_row_address(oled_descriptor, y+j);
                sh1122_set_column_address(oled_descriptor, (x+x_offset_wrap)/2);
                
                /* Start filling the SSD1322 RAM */
                sh1122_start_data_sending(oled_descriptor);
                
                /* Trigger DMA transfer for the complete width */
                dma_oled_init_transfer(oled_descriptor->sercom_pt, (void*)&pixel_buffer[buffer_sel][pixel_array_offset], nb_pixels_to_send/2, oled_descriptor->dma_trigger_id);
            }
                
            /* Flip buffer, start fetching next line while the transfer is happening */
            if (j != bitstream->height-1)
            {
                buffer_sel = (buffer_sel+1) & 0x01;
                bitstream_bitmap_array_read(bitstream, pixel_buffer[buffer_sel], bitstream->width);
            }
               
            if ((y+j >= oled_descriptor->min_disp_y) && (y+j <= oled_descriptor->max_disp_y))
            { 
                /* Wait for transfer done */
                while(dma_oled_check_and_clear_dma_transfer_flag() == FALSE);
                
                /* Wait for spi buffer to be sent */
                sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
                
                /* Stop sending data */
                sh1122_stop_data_sending(oled_descriptor);
            }
        }
        
        /* Close bitstream */
        bitstream_bitmap_close(bitstream);   
    } 
    #endif
    else
    {
        /* Negative X */
        if ((x < 0) && (oled_descriptor->screen_wrapping_allowed != FALSE))
        {
            /* Dirty trick: let the screen do the wrapping for you */
            x += SH1122_OLED_WIDTH;
        }    
        
        /* Y loop */
        for (uint16_t yind=0; yind < bitstream->height; yind++)
        {
            uint16_t xind = 0;
            uint16_t pixels = 0;
               
            /* If y is off screen, simply continue looping */
            if ((y+yind < oled_descriptor->min_disp_y) || (y+yind >= oled_descriptor->max_disp_y))
            {
                for (uint16_t i = 0; i < bitstream->width; i++)
                {
                    bitstream_bitmap_read(bitstream, 1);
                }
                continue;
            }

            /* Set pixel write window */
            sh1122_set_row_address(oled_descriptor, y+yind);
            if (x < 0)
            {
                sh1122_set_column_address(oled_descriptor, 0);
            }
            else
            {
                sh1122_set_column_address(oled_descriptor, x/2);
            }
            
            /* Start filling the SSD1322 RAM */
            sh1122_start_data_sending(oled_descriptor);

            /* Start x not a multiple of 2 */
            if (x%2 != 0)
            {
                /* Set xind to 1 as we're writing a pixel */
                xind = 1;
                
                /* Fetch one pixel */
                pixels = bitstream_bitmap_read(bitstream, 1);

                /* Fill existing pixels if available */
                if ((x/2) == oled_descriptor->gddram_pixel[y+yind].xaddr)
                {
                    pixels |= oled_descriptor->gddram_pixel[y+yind].pixels;
                }

                /* Send the 2 pixels to the display */
                if (x >= 0)
                {
                    sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)(pixels & 0x00FF));
                }
            }
            
            /* Start x multiple of 2, start filling */
            for (; xind < bitstream->width; xind+=2)
            {
                if ((xind+2) <= bitstream->width)
                {
                    pixels = bitstream_bitmap_read(bitstream, 2);
                }
                else
                {
                    pixels = bitstream_bitmap_read(bitstream, 1) << 4;
                }
                
                /* Send the 2 pixels to the display */
                if ((x+xind >= 0) && ((x+xind <= oled_descriptor->max_disp_x) || (oled_descriptor->screen_wrapping_allowed != FALSE)))
                {
                    sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, (uint8_t)(pixels & 0x00FF));
                }
            }
            
            /* Store pixel data in our gddram buffer for later merging */
            if (pixels != 0)
            {
                oled_descriptor->gddram_pixel[y+yind].pixels = (uint8_t)pixels;
                oled_descriptor->gddram_pixel[y+yind].xaddr = (x+bitstream->width-1)/2;
            }
            
            /* Wait for spi buffer to be sent */
            sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
            
            /* Stop sending data */
            sh1122_stop_data_sending(oled_descriptor);
        }
        
        /* Close bitstream */
        bitstream_bitmap_close(bitstream);   
    }    
}

/*! \fn     sh1122_display_bitmap_from_flash_at_recommended_position(sh1122_descriptor_t* oled_descriptor, uint32_t file_id, BOOL write_to_buffer)
*   \brief  Display a bitmap stored in the external flash, at its recommended position
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  file_id             Bitmap file ID
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return success status
*/
RET_TYPE sh1122_display_bitmap_from_flash_at_recommended_position(sh1122_descriptor_t* oled_descriptor, uint32_t file_id, BOOL write_to_buffer)
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
    sh1122_draw_image_from_bitstream(oled_descriptor, bitmap.xpos, bitmap.ypos, &bitstream, write_to_buffer);
    
    return RETURN_OK;    
}

/*! \fn     sh1122_display_bitmap_from_flash(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, uint32_t file_id, BOOL write_to_buffer)
*   \brief  Display a bitmap stored in the external flash
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  file_id             Bitmap file ID
*   \param  write_to_buffer    Set to true to write to internal buffer
*   \return success status
*/
RET_TYPE sh1122_display_bitmap_from_flash(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, uint32_t file_id, BOOL write_to_buffer)
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
    sh1122_draw_image_from_bitstream(oled_descriptor, x, y, &bitstream, write_to_buffer);
    
    return RETURN_OK;  
} 

/*! \fn     sh1122_get_string_width(sh1122_descriptor_t* oled_descriptor, const char* str)
*   \brief  Return the pixel width of the string.
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  str                 String to get width of
*   \return Width of string in pixels based on current font
*/
uint16_t sh1122_get_string_width(sh1122_descriptor_t* oled_descriptor, const cust_char_t* str)
{
    uint16_t temp_uint16 = 0;
    uint16_t width=0;
    
    for (nat_type_t ind=0; (str[ind] != 0) && (str[ind] != '\r'); ind++)
    {
        width += sh1122_get_glyph_width(oled_descriptor, str[ind], &temp_uint16);
    }
    
    return width;    
}

/*! \fn     sh1122_get_glyph_width(sh1122_descriptor_t* oled_descriptor, char ch, uint16_t* glyph_height)
*   \brief  Return the width of the specified character in the current font
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  ch                  Character
*   \param  glyph_height        Where to store the glyph height (added bonus)
*   \return width of the glyph
*/
uint16_t sh1122_get_glyph_width(sh1122_descriptor_t* oled_descriptor, cust_char_t ch, uint16_t* glyph_height)
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
            return glyph.xrect;
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

 /*! \fn     sh1122_glyph_draw(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, char ch, BOOL write_to_buffer)
 *   \brief  Draw a character glyph on the screen at x,y.
 *   \param  oled_descriptor    Pointer to a sh1122 descriptor struct
 *   \param  x                  x position to start glyph
 *   \param  y                  y position to start glyph
 *   \param  ch                 Character to draw
 *   \param  write_to_buffer    Set to true to write to internal buffer
 *   \return width of the glyph
 */
uint16_t sh1122_glyph_draw(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, cust_char_t ch, BOOL write_to_buffer)
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
        sh1122_draw_image_from_bitstream(oled_descriptor, x, y, &bs, write_to_buffer);
    }
    
    return (uint8_t)(glyph_width + glyph.xoffset) + 1;
}

/*! \fn     sh1122_put_char(sh1122_descriptor_t* oled_descriptor, char ch, BOOL write_to_buffer)
*   \brief  Print char on display
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  ch                  Char to display
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return Pixel width of the printed char, -1 if too wide for display
*/
int16_t sh1122_put_char(sh1122_descriptor_t* oled_descriptor, cust_char_t ch, BOOL write_to_buffer)
{
    uint16_t glyph_height = 0;
    
    /* Have we actually selected a font? */
    if (oled_descriptor->currentFontAddress == 0)
    {
        return -1;
    }
    
    if (ch == '\n')
    {
        if (oled_descriptor->line_feed_allowed != FALSE)
        {
            oled_descriptor->cur_text_y += oled_descriptor->current_font_header.height;
            oled_descriptor->cur_text_x = 0;            
        }
    }
    else if (ch == '\r')
    {
        if (oled_descriptor->carriage_return_allowed != FALSE)
        {
            oled_descriptor->cur_text_x = 0;
        }
    }
    else
    {
        uint16_t width = sh1122_get_glyph_width(oled_descriptor, ch, &glyph_height);
        
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
        oled_descriptor->cur_text_x += sh1122_glyph_draw(oled_descriptor, oled_descriptor->cur_text_x, oled_descriptor->cur_text_y, ch, write_to_buffer);
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
        if (oled_descriptor->cur_text_x < SH1122_OLED_WIDTH)
        {
            /* Normal use case */
            return width;
        } 
        else
        {
            /* Part of glyph is displayed */
            return width - (oled_descriptor->cur_text_x - SH1122_OLED_WIDTH);
        }
    }
    
    return 0;
}

/*! \fn     sh1122_put_string(sh1122_descriptor_t* oled_descriptor, const char* str, BOOL write_to_buffer)
*   \brief  Print string at current x y
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  str                 String to print
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return Pixel width of the printed string, -1 if too wide for display
*/
int16_t sh1122_put_string(sh1122_descriptor_t* oled_descriptor, const cust_char_t* str, BOOL write_to_buffer)
{
    int16_t string_width = 0;
    
    // Write chars until we find final 0
    while (*str)
    {
        /* Check for line feed */
        if ((*str == '\n') &&  (oled_descriptor->line_feed_allowed == FALSE))
        {
            return string_width;
        }
        
        int16_t pixel_width = sh1122_put_char(oled_descriptor, *str++, write_to_buffer);
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

/*! \fn     sh1122_put_string_xy(sh1122_descriptor_t* oled_descriptor, int16_t x, uint8_t y, oled_align_te justify, const char* string)
*   \brief  Display an error string on the screen (X0Y0, centered)
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  string              Null terminated string
*   \return How many characters were printed
*/
uint16_t sh1122_put_error_string(sh1122_descriptor_t* oled_descriptor, const cust_char_t* string)
{
    sh1122_put_string_xy(oled_descriptor, 0, 0, OLED_ALIGN_CENTER, string, TRUE);
    return sh1122_put_string_xy(oled_descriptor, 0, 0, OLED_ALIGN_CENTER, string, FALSE);
}

/*! \fn     sh1122_put_centered_string(sh1122_descriptor_t* oled_descriptor, uint8_t y, const cust_char_t* string, BOOL write_to_buffer)
*   \brief  Display a centered string on the screen
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  y                   Starting y
*   \param  string              Null terminated string
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return Width of the printed string
*/
uint16_t sh1122_put_centered_string(sh1122_descriptor_t* oled_descriptor, uint8_t y, const cust_char_t* string, BOOL write_to_buffer) 
{
     return sh1122_put_string_xy(oled_descriptor, 0, y, OLED_ALIGN_CENTER, string, write_to_buffer);
}

/*! \fn     void sh1122_put_centered_char(sh1122_descriptor_t* oled_descriptor, int16_t x, uint16_t y, cust_char_t c, BOOL write_to_buffer)
*   \brief  Display a char centered around an X position
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Centered around this x
*   \param  y                   Starting y
*   \param  c                   Char to print
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return How many characters were printed
*/
void sh1122_put_centered_char(sh1122_descriptor_t* oled_descriptor, int16_t x, uint16_t y, cust_char_t c, BOOL write_to_buffer) 
{
    uint16_t glyph_height;
    int16_t cur_text_x = oled_descriptor->cur_text_x;
    int16_t cur_text_y = (int16_t)oled_descriptor->cur_text_y;
    uint16_t width = sh1122_get_glyph_width(oled_descriptor, c, &glyph_height);
    
    /* Store cur text x & y */
    oled_descriptor->cur_text_x = x-((width+1)/2);
    oled_descriptor->cur_text_y = y;
   
    /* Display char */
    sh1122_put_char(oled_descriptor, c, write_to_buffer);

    /* Restore x & y */
    oled_descriptor->cur_text_x = cur_text_x;
    oled_descriptor->cur_text_y = cur_text_y;
}

/*! \fn     sh1122_set_xy(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y)
*   \brief  Set current text X & Y
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x   X
*   \param  y   Y
*/
void sh1122_set_xy(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y)
{
    oled_descriptor->cur_text_x = x;
    oled_descriptor->cur_text_y = y;
}

/*! \fn     sh1122_get_number_of_printable_characters_for_string(sh1122_descriptor_t* oled_descriptor, int16_t x, const cust_char_t* string) 
*   \brief  Get the number of characters of a given string that can be printed on the screen
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  string              Null terminated string
*   \return Number of characters that can be printed
*/
uint16_t sh1122_get_number_of_printable_characters_for_string(sh1122_descriptor_t* oled_descriptor, int16_t x, const cust_char_t* string) 
{
    uint16_t nb_characters = 0;
    uint16_t temp_uint16 = 0;
    
    for (nat_type_t ind=0; (string[ind] != 0) && (string[ind] != '\r'); ind++)
    {
        x += sh1122_get_glyph_width(oled_descriptor, string[ind], &temp_uint16);
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

/*! \fn     sh1122_put_string_xy(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, oled_align_te justify, const char* string, BOOL write_to_buffer) 
*   \brief  Display a string on the screen
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  justify             String justify (see enum)
*   \param  string              Null terminated string
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return Pixel width of the printed string, -1 if too wide for display
*/
int16_t sh1122_put_string_xy(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, oled_align_te justify, const cust_char_t* string, BOOL write_to_buffer) 
{
    uint16_t width = sh1122_get_string_width(oled_descriptor, string);
    int16_t max_text_x_copy = oled_descriptor->max_text_x;
    uint16_t return_val;

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
        if (x < oled_descriptor->max_text_x)
        {
            oled_descriptor->max_text_x = x;
        }
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
    
    /* Store cur text x & y */
    oled_descriptor->cur_text_x = x;
    oled_descriptor->cur_text_y = y;
    
    /* Display string */
    return_val = sh1122_put_string(oled_descriptor, string, write_to_buffer);
    oled_descriptor->max_text_x = max_text_x_copy;
    
    // Return the number of characters printed
    return return_val;
}

#ifdef OLED_PRINTF_ENABLED
/*! \fn     sh1122_printf_xy(sh1122_descriptor_t* oled_descriptor, int16_t x, uint8_t y, uint8_t justify, BOOL write_to_buffer, const char *fmt, ...) 
*   \brief  Printf string on the display
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  justify             String justify (see enum)
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return How many characters were printed
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
uint16_t sh1122_printf_xy(sh1122_descriptor_t* oled_descriptor, int16_t x, uint8_t y, oled_align_te justify, BOOL write_to_buffer, const char *fmt, ...) 
{
    int16_t max_text_x_copy = oled_descriptor->max_text_x;
    uint16_t return_val;
    uint16_t width;
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
        width = sh1122_get_string_width(oled_descriptor, u16buf);
    }
    else
    {
        va_end(ap);
        return 0;
    }

    if (justify == OLED_ALIGN_CENTER)
    {
        if ((x + oled_descriptor->min_text_x + width) < oled_descriptor->max_text_x)
        {
            x = (oled_descriptor->max_text_x + x + oled_descriptor->min_text_x - width)/2;
        }
    }
    else if (justify == OLED_ALIGN_RIGHT)
    {
        if (x < oled_descriptor->max_text_x)
        {
            oled_descriptor->max_text_x = x;
        }
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
    
    /* Store cur text x & y */
    oled_descriptor->cur_text_x = x;
    oled_descriptor->cur_text_y = y;
    
    /* Display string */
    return_val = sh1122_put_string(oled_descriptor, u16buf, write_to_buffer);
    oled_descriptor->max_text_x = max_text_x_copy;
    
    // Return the number of characters printed
    return return_val;
}
#pragma GCC diagnostic pop

#endif
