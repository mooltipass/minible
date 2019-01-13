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
    SH1122_CMD_SET_ROW_ADDR,                    1, 0x00,                // Row Address Mode Setting
    SH1122_CMD_SET_HIGH_COLUMN_ADDR,            0,                      // Set Higher Column Address
    SH1122_CMD_SET_LOW_COLUMN_ADDR,             0,                      // Set Lower Column Address
    SH1122_CMD_SET_CLOCK_DIVIDER,               1, 0x50,                // Set Display Clock Divide Ratio / Oscillator Frequency: default fosc (512khz) and divide ratio of 1 > Fframe = 512 / 64 / 64 / 1 = 125Hz
    SS1122_CMD_SET_DISCHARGE_PRECHARGE_PERIOD,  1, 0x22,                // Set Discharge/Precharge Period: 2DCLK & 2DCLK (default)
    SH1122_CMD_SET_DISPLAY_START_LINE | 32,     0,                      // Set Display Start Line To 32 (not sure why, but it's the right value when flipping the display...)
    SH1122_CMD_SET_CONTRAST_CURRENT,            1, 0x80,                // Contrast Control Mode Set (up to 0xFF)
    SH1122_CMD_SET_SEGMENT_REMAP | 0x01,        0,                      // Set Segment Re-map to Reverse Direction
    SH1122_CMD_SET_SCAN_DIRECTION | 0x08,       0,                      // Scam from COM0 to COM[N-1]
    SH1122_CMD_SET_DISPLAY_OFF_ON | 0x00,       0,                      // Normal Display Status, Not Forced to ON
    SH1122_CMD_SET_NORMAL_DISPLAY,              0,                      // Display Bits Normally Interpreted
    SH1122_CMD_SET_MULTIPLEX_RATIO,             1, 0x3F,                // Mutiplex Ratio To 64
    SH1122_CMD_SET_DCDC_SETTING,                0,                      // Start Configuring Onboard DCDC
    SH1122_CMD_SET_DCDC_DISABLE,                0,                      // Disable DCDC
    SH1122_CMD_SET_DISPLAY_OFFSET,              1, 0x00,                // No Display Offset
    SH1122_CMD_SET_VCOM_DESELECT_LEVEL,         1, 0x30,                // VCOMH = (0.430+ A[7:0] X 0.006415) X VREF
    SH1122_CMD_SET_VSEGM_LEVEL,                 1, 0x00,                // VSEGM = (0.430+ A[7:0] X 0.006415) X VREF
    SH1122_CMD_SET_DISCHARGE_VSL_LEVEL | 0x01,  0                       // VSL = 0.1*Vref
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
    sh1122_write_single_data(oled_descriptor, contrast_current);    
}

/*! \fn     sh1122_set_master_current(sh1122_descriptor_t* oled_descriptor, uint8_t contrast_current)
*   \brief  Set master current for display
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  master_current      Master current (up to 0x0F)
*/
void sh1122_set_master_current(sh1122_descriptor_t* oled_descriptor, uint8_t master_current)
{
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_CONTRAST_CURRENT);
    sh1122_write_single_data(oled_descriptor, master_current & 0x0F);    
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
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_DISPLAY_START_LINE);
    sh1122_write_single_data(oled_descriptor, (uint8_t)offset);   
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

/*! \fn     sh1122_reset_max_text_x(sh1122_descriptor_t* oled_descriptor)
*   \brief  Reset maximum text X position
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_reset_max_text_x(sh1122_descriptor_t* oled_descriptor)
{
    oled_descriptor->max_text_x = SH1122_OLED_WIDTH;
}

/*! \fn     sh1122_flip_displayed_buffer(sh1122_descriptor_t* oled_descriptor)
*   \brief  Switch from active / inactive buffer
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_flip_displayed_buffer(sh1122_descriptor_t* oled_descriptor)
{
    sh1122_move_display_start_line(oled_descriptor, SH1122_OLED_HEIGHT);
}

/*! \fn     sh1122_flip_buffers(sh1122_descriptor_t* oled_descriptor, oled_scroll_te scroll_mode, uint32_t delay)
*   \brief  Flip buffer using scrolling
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  scroll_mode         Scrolling mode (see enum)
*   \param  delay               Delay in ms between line scrolls
*/
void sh1122_flip_buffers(sh1122_descriptor_t* oled_descriptor, oled_scroll_te scroll_mode, uint32_t delay)
{
    int16_t offset = (scroll_mode == OLED_SCROLL_UP ? 1 : -1);
    
    for (uint8_t ind = 0; ind < SH1122_OLED_HEIGHT; ind++)
    {
        sh1122_move_display_start_line(oled_descriptor, offset);
        timer_delay_ms(delay);
    }
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

/*! \fn     sh1122_check_for_flush_and_terminate(sh1122_descriptor_t* oled_descriptor)
*   \brief  Check if a flush is in progress, and wait for its completion if so
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
#ifdef OLED_INTERNAL_FRAME_BUFFER
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
        dma_oled_init_transfer((void*)&oled_descriptor->sercom_pt->SPI.DATA.reg, (void*)&oled_descriptor->frame_buffer[0][0], sizeof(oled_descriptor->frame_buffer), oled_descriptor->dma_trigger_id);
        oled_descriptor->frame_buffer_flush_in_progress = TRUE;
    }
    else if (oled_descriptor->loaded_transition == OLED_LEFT_RIGHT_TRANS)
    {
        /* Left to right */
        for (uint16_t x = 0; x < SH1122_OLED_WIDTH/2; x++)
        {
            for (uint16_t y = 0; y < SH1122_OLED_HEIGHT; y++)
            {
                sh1122_display_horizontal_pixel_line(oled_descriptor, x*2, y, 2, &(oled_descriptor->frame_buffer[y][x]));
            }
        }
    }
    else if (oled_descriptor->loaded_transition == OLED_RIGHT_LEFT_TRANS)
    {
        /* Right to left */
        for (int16_t x = (SH1122_OLED_WIDTH/2)-1; x >= 0; x--)
        {
            for (uint16_t y = 0; y < SH1122_OLED_HEIGHT; y++)
            {
                sh1122_display_horizontal_pixel_line(oled_descriptor, x*2, y, 2, &(oled_descriptor->frame_buffer[y][x]));
            }
        }
    }
    else if (oled_descriptor->loaded_transition == OLED_TOP_BOT_TRANS)
    {
        /* Top to bottom */
        for (uint16_t y = 0; y < SH1122_OLED_HEIGHT; y++)
        {
            sh1122_display_horizontal_pixel_line(oled_descriptor, 0, y, SH1122_OLED_WIDTH, &(oled_descriptor->frame_buffer[y][0]));
            DELAYMS(3);
        }
    }
    else if (oled_descriptor->loaded_transition == OLED_BOT_TOP_TRANS)
    {
        /* Bottom to top */
        for (int16_t y = SH1122_OLED_HEIGHT-1; y >= 0; y--)
        {
            sh1122_display_horizontal_pixel_line(oled_descriptor, 0, y, SH1122_OLED_WIDTH, &(oled_descriptor->frame_buffer[y][0]));
            DELAYMS(3);
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
                sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos, y, 2, &(oled_descriptor->frame_buffer[y][x_pos/2]));
                sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos2, y, 2, &(oled_descriptor->frame_buffer[y][x_pos2/2]));
            }
            sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos, low_y, x_pos2-x_pos, &(oled_descriptor->frame_buffer[low_y][x_pos/2]));
            sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos, high_y, x_pos2-x_pos, &(oled_descriptor->frame_buffer[high_y][x_pos/2]));
            
            if ((i & 0x01) == 0)
            {
                low_y--;
                high_y++;
            }
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
                sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos, y, 2, &(oled_descriptor->frame_buffer[y][x_pos/2]));
                sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos2, y, 2, &(oled_descriptor->frame_buffer[y][x_pos2/2]));
            }
            sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos, low_y, x_pos2-x_pos, &(oled_descriptor->frame_buffer[low_y][x_pos/2]));
            sh1122_display_horizontal_pixel_line(oled_descriptor, x_pos, high_y, x_pos2-x_pos, &(oled_descriptor->frame_buffer[high_y][x_pos/2]));
            
            if ((i & 0x01) == 1)
            {
                low_y++;
                high_y--;
            }
        }
    }
    
    /* Reset transition */
    oled_descriptor->loaded_transition = OLED_TRANS_NONE;
}

/*! \fn     sh1122_clear_frame_buffer(sh1122_descriptor_t* oled_descriptor)
*   \brief  Clear frame buffer
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_clear_frame_buffer(sh1122_descriptor_t* oled_descriptor)
{    
    sh1122_check_for_flush_and_terminate(oled_descriptor);
    memset((void*)oled_descriptor->frame_buffer, 0x00, sizeof(oled_descriptor->frame_buffer));
}
#endif

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
    timer_delay_ms(100);
}

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

/*! \fn     sh1122_refresh_used_font(void)
*   \brief  Refreshed used font (in case of init or language change)
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_refresh_used_font(sh1122_descriptor_t* oled_descriptor)
{
    if (custom_fs_get_file_address(DEFAULT_FONT_ID, &oled_descriptor->currentFontAddress, CUSTOM_FS_FONTS_TYPE) == RETURN_NOK)
    {
        oled_descriptor->currentFontAddress = 0;
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
    }    
}

/*! \fn     sh1122_init_display(sh1122_descriptor_t oled_descriptor)
*   \brief  Initialize a SSD1322 display
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_init_display(sh1122_descriptor_t* oled_descriptor)
{
    /* Vars init : should already be to 0 but you never know... */
    oled_descriptor->carriage_return_allowed = FALSE;
    oled_descriptor->line_feed_allowed = FALSE;
    oled_descriptor->currentFontAddress = 0;
    oled_descriptor->max_text_x = SH1122_OLED_WIDTH;
    oled_descriptor->min_text_x = 0;
    
    /* Send the initialization sequence through SPI */
    for (uint16_t ind = 0; ind < sizeof(sh1122_init_sequence);)
    {
        /* nCS set */
        PORT->Group[oled_descriptor->sh1122_cs_pin_group].OUTCLR.reg = oled_descriptor->sh1122_cs_pin_mask;
        PORT->Group[oled_descriptor->sh1122_cd_pin_group].OUTCLR.reg = oled_descriptor->sh1122_cd_pin_mask;
        
        /* First byte: command */
        sercom_spi_send_single_byte(oled_descriptor->sercom_pt, sh1122_init_sequence[ind++]);
        
        /* Second byte: payload length */
        uint16_t dataSize = sh1122_init_sequence[ind++];
        
        /* If different than 0, send payload */
        while (dataSize--)
        {
            sercom_spi_send_single_byte(oled_descriptor->sercom_pt, sh1122_init_sequence[ind++]);
        }
        
        /* nCS release */
        PORT->Group[oled_descriptor->sh1122_cs_pin_group].OUTSET.reg = oled_descriptor->sh1122_cs_pin_mask;
        asm("NOP");asm("NOP");
    }

    /* Clear display */
    sh1122_clear_current_screen(oled_descriptor);
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    memset((void*)oled_descriptor->frame_buffer, 0x00, sizeof(oled_descriptor->frame_buffer));
    oled_descriptor->frame_buffer_flush_in_progress = FALSE;
    #endif

    /* Switch screen on */    
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_DISPLAY_ON);
    oled_descriptor->oled_on = TRUE;
    
    /* Set emergency font by default */
    sh1122_set_emergency_font(oled_descriptor);
    
    /* From datasheet : wait 100ms */
    timer_delay_ms(100);
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
    /   TODO: compare sh1122_draw_full_screen_image_from_bitstream performance with sh1122_draw_aligned_image_from_bitstream
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
        dma_oled_init_transfer((void*)&oled_descriptor->sercom_pt->SPI.DATA.reg, (void*)pixel_buffer[buffer_sel], sizeof(pixel_buffer[0]), oled_descriptor->dma_trigger_id);
        
        for (uint32_t i = 0; i < (SH1122_OLED_WIDTH*SH1122_OLED_HEIGHT) - sizeof(pixel_buffer[0]); i+=sizeof(pixel_buffer[0])*2)
        {            
            /* Read from bitstream in the next buffer */
            bitstream_bitmap_array_read(bitstream, pixel_buffer[(buffer_sel+1)&0x01], sizeof(pixel_buffer[0])*2);
            
            /* Wait for transfer done */
            while(dma_oled_check_and_clear_dma_transfer_flag() == FALSE);
            
            /* Init DMA transfer */
            buffer_sel = (buffer_sel+1) & 0x01;
            dma_oled_init_transfer((void*)&oled_descriptor->sercom_pt->SPI.DATA.reg, (void*)pixel_buffer[buffer_sel], sizeof(pixel_buffer[0]), oled_descriptor->dma_trigger_id);
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

/*! \fn     sh1122_draw_aligned_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_bitmap_t* bitstream, BOOL write_to_buffer)
*   \brief  Draw a 2 pixels-aligned picture from a bitstream
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  bitstream           Pointer to the bistream
*   \param  write_to_buffer     Set to true to write to internal buffer
*/
void sh1122_draw_aligned_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_bitmap_t* bitstream, BOOL write_to_buffer)
{
    uint16_t height = bitstream->height;
    uint16_t width = bitstream->width;
    
    /* Frame buffer: we only support DMA transfers when not writing to buffer */
    #ifdef OLED_INTERNAL_FRAME_BUFFER
        /* Depending if we write in the frame buffer or not */
        if (write_to_buffer != FALSE)
        {
            /* Scan Y */
            for (uint16_t j = 0; j < height; j++)
            {
                /* Scan X */
                for (uint16_t i = 0; i < width/2; i++)
                {
                    uint8_t pixels = bitstream_bitmap_two_pixel_read(bitstream);
                    oled_descriptor->frame_buffer[y+j][x/2+i] |= pixels;
                }
            }
        } 
        else
        {
            /* Wait for a possible ongoing previous flush */
            sh1122_check_for_flush_and_terminate(oled_descriptor);
            
            /* Buffer large enough to contain a display line in order to trig one DMA transfer */
            uint8_t pixel_buffer[2][SH1122_OLED_WIDTH/2];
            uint32_t buffer_sel = 0;
            
            /* Trigger first buffer fill: if we asked more data, the bitstream will return 0s */
            bitstream_bitmap_array_read(bitstream, pixel_buffer[buffer_sel], width);
            
            /* Scan Y */
            for (uint16_t j = 0; j < height; j++)
            {
                /* Set pixel write window */
                sh1122_set_row_address(oled_descriptor, y+j);
                sh1122_set_column_address(oled_descriptor, x/2);
                
                /* Start filling the SSD1322 RAM */
                sh1122_start_data_sending(oled_descriptor);
                
                /* Trigger DMA transfer for the complete width */
                dma_oled_init_transfer((void*)&oled_descriptor->sercom_pt->SPI.DATA.reg, (void*)pixel_buffer[buffer_sel], width/2, oled_descriptor->dma_trigger_id);
                
                /* Flip buffer, start fetching next line while the transfer is happening */
                if (j != height-1)
                {
                    buffer_sel = (buffer_sel+1) & 0x01;
                    bitstream_bitmap_array_read(bitstream, pixel_buffer[buffer_sel], width);
                }
                
                /* Wait for transfer done */
                while(dma_oled_check_and_clear_dma_transfer_flag() == FALSE);
                
                /* Wait for spi buffer to be sent */
                sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
                
                /* Stop sending data */
                sh1122_stop_data_sending(oled_descriptor);
            }
        }
    #else    
        /* Depending if we use DMA transfers */
        #ifdef OLED_DMA_TRANSFER        
            /* Buffer large enough to contain a display line in order to trig one DMA transfer */
            uint8_t pixel_buffer[2][SH1122_OLED_WIDTH/2];
            uint32_t buffer_sel = 0;
        
            /* Trigger first buffer fill: if we asked more data, the bitstream will return 0s */
            bitstream_bitmap_array_read(bitstream, pixel_buffer[buffer_sel], width);
        
            /* Scan Y */
            for (uint16_t j = 0; j < height; j++)
            {
                /* Set pixel write window */
                sh1122_set_row_address(oled_descriptor, y+j);
                sh1122_set_column_address(oled_descriptor, x/2);
            
                /* Start filling the SSD1322 RAM */
                sh1122_start_data_sending(oled_descriptor);
            
                /* Trigger DMA transfer for the complete width */
                dma_oled_init_transfer((void*)&oled_descriptor->sercom_pt->SPI.DATA.reg, (void*)pixel_buffer[buffer_sel], width/2, oled_descriptor->dma_trigger_id);  
            
                /* Flip buffer, start fetching next line while the transfer is happening */   
                if (j != height-1)
                {                
                    buffer_sel = (buffer_sel+1) & 0x01;
                    bitstream_bitmap_array_read(bitstream, pixel_buffer[buffer_sel], width);
                }
            
                /* Wait for transfer done */
                while(dma_oled_check_and_clear_dma_transfer_flag() == FALSE);
            
                /* Wait for spi buffer to be sent */
                sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
            
                /* Stop sending data */
                sh1122_stop_data_sending(oled_descriptor);
            }
        #else        
            uint8_t pixel_buffer[16];
            uint32_t pixel_ind = 0;
        
            /* Read from bitstream */
            bitstream_bitmap_array_read(bitstream, pixel_buffer, sizeof(pixel_buffer)*2);
        
            /* Scan Y */
            for (uint16_t j = 0; j < height; j++)
            {
                /* Set pixel write window */
                sh1122_set_row_address(oled_descriptor, y+j);
                sh1122_set_column_address(oled_descriptor, x/2);
            
                /* Start filling the SSD1322 RAM */
                sh1122_start_data_sending(oled_descriptor);
            
                /* Scan X */
                for (uint16_t i = 0; i < width; i+=2)
                {
                    sercom_spi_send_single_byte_without_receive_wait(oled_descriptor->sercom_pt, pixel_buffer[pixel_ind++]);     
                
                    /* Check for empty buffer */
                    if (pixel_ind == sizeof(pixel_buffer))
                    {
                        bitstream_bitmap_array_read(bitstream, pixel_buffer, sizeof(pixel_buffer)*2);
                        pixel_ind = 0;
                    }
                }
            
                /* Wait for spi buffer to be sent */
                sercom_spi_wait_for_transmit_complete(oled_descriptor->sercom_pt);
            
                /* Stop sending data */
                sh1122_stop_data_sending(oled_descriptor);
            }
        #endif   
    #endif 
        
    /* Close bitstream */
    bitstream_bitmap_close(bitstream);    
}   

/*! \fn     sh1122_display_horizontal_pixel_line(sh1122_descriptor_t* oled_descriptor, uint16_t x, uint16_t y, uint16_t width, uint8_t* pixels)
*   \brief  Display adjacent pixels at a given Y
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   X position
*   \param  y                   Y position
*   \param  width               Line width
*   \param  pixels              Pointer to data buffer
*   \note   We only support odd X positions for the moment
*/
void sh1122_display_horizontal_pixel_line(sh1122_descriptor_t* oled_descriptor, uint16_t x, uint16_t y, uint16_t width, uint8_t* pixels)
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

/*! \fn     sh1122_draw_non_aligned_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_bitmap_t* bitstream, BOOL write_to_buffer)
*   \brief  Draw a 2 pixels non aligned picture from a bitstream
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  bitstream           Pointer to the bistream
*   \param  write_to_buffer    Set to true to write to internal buffer
*/
void sh1122_draw_non_aligned_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_bitmap_t* bitstream, BOOL write_to_buffer)
{
    uint16_t height = bitstream->height;
    uint16_t width = bitstream->width;
    uint16_t xoff = x - (x / 2) * 2;
    
    #ifdef OLED_INTERNAL_FRAME_BUFFER
    if (write_to_buffer != FALSE)
    {
        for (uint16_t yind=0; yind < height; yind++)
        {
            uint16_t xind = 0;
            uint16_t pixels = 0;

            /* Start x not a multiple of 2 */
            if (xoff != 0)
            {
                /* Set xind to 1 as we're writing a pixel */
                xind = 1;
                
                /* Fetch one pixel */
                pixels = bitstream_bitmap_read(bitstream, 1);

                /* Fill frame buffer */
                oled_descriptor->frame_buffer[y+yind][x/2] |= pixels;
            }
            
            /* Start x multiple of 2, start filling */
            for (; xind < width; xind+=2)
            {
                if ((xind+2) <= width)
                {
                    pixels = bitstream_bitmap_read(bitstream, 2);
                }
                else
                {
                    pixels = bitstream_bitmap_read(bitstream, 1) << 4;
                }                

                /* Fill frame buffer */
                oled_descriptor->frame_buffer[y+yind][(x+xind)/2] |= pixels;
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
            
            /* Fetch one pixel */
            pixels = bitstream_bitmap_read(bitstream, 1);

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
                pixels = bitstream_bitmap_read(bitstream, 2);
            }
            else
            {
                pixels = bitstream_bitmap_read(bitstream, 1) << 4;
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
    
    /* Close bitstream */
    bitstream_bitmap_close(bitstream);
}    

/*! \fn     sh1122_draw_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_t* bs, BOOL write_to_buffer)
*   \brief  Draw a picture from a bitstream
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  bitstream           Pointer to the bistream
*   \param  write_to_buffer     Set to true to write to internal buffer
*/
void sh1122_draw_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_bitmap_t* bitstream, BOOL write_to_buffer)
{
    if ((x == 0) && (y == 0) && (bitstream->width == SH1122_OLED_WIDTH) && (bitstream->height == SH1122_OLED_HEIGHT))
    {
        /* Dedicated code to allow faster update */
        //sh1122_draw_aligned_image_from_bitstream(oled_descriptor, x, y, bitstream);
        sh1122_draw_full_screen_image_from_bitstream(oled_descriptor, bitstream);        
    }
    else if ((bitstream->width % 2 == 0) && (x % 2 == 0))
    {
        /* If we're 2 pixels aligned, call a dedicated function for fast processing */
        sh1122_draw_aligned_image_from_bitstream(oled_descriptor, x, y, bitstream, write_to_buffer);
    } 
    else
    {
        sh1122_draw_non_aligned_image_from_bitstream(oled_descriptor, x, y, bitstream, write_to_buffer);
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

/*! \fn     sh1122_draw_rectangle(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
*   \brief  Draw a rectangle on the screen
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  width               Width
*   \param  height              Height
*   \param  color               4 bits color
*/
void sh1122_draw_rectangle(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
    uint16_t xoff = x - (x / 2) * 2;

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
}

/*! \fn     sh1122_get_string_width(sh1122_descriptor_t* oled_descriptor, const char* str)
*   \brief  Return the pixel width of the string.
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  str                 String to get width of
*   \return Width of string in pixels based on current font
*/
uint16_t sh1122_get_string_width(sh1122_descriptor_t* oled_descriptor, const cust_char_t* str)
{
    uint16_t width=0;
    
    for (nat_type_t ind=0; (str[ind] != 0) && (str[ind] != '\r'); ind++)
    {
        width += sh1122_get_glyph_width(oled_descriptor, str[ind]);
    }
    
    return width;    
}

/*! \fn     sh1122_get_glyph_width(sh1122_descriptor_t* oled_descriptor, char ch)
*   \brief  Return the width of the specified character in the current font
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  ch                  Character
*   \return width of the glyph
*/
uint16_t sh1122_get_glyph_width(sh1122_descriptor_t* oled_descriptor, cust_char_t ch)
{
    uint16_t glyph_desc_pt_offset = 0;
    uint16_t interval_start = 0;
    font_glyph_t glyph;
    uint16_t gind;
    
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
            return (glyph.xrect >> 1); // space character is always too large...
        }
        else
        {
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
        glyph_width = glyph.xrect >> 1; // space character is always too large...
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
*   \return success status
*/
RET_TYPE sh1122_put_char(sh1122_descriptor_t* oled_descriptor, cust_char_t ch, BOOL write_to_buffer)
{
    /* Have we actually selected a font? */
    if (oled_descriptor->currentFontAddress == 0)
    {
        return RETURN_NOK;
    }
    
    if ((ch == '\n') && (oled_descriptor->line_feed_allowed != FALSE))
    {
        oled_descriptor->cur_text_y += oled_descriptor->current_font_header.height;
        oled_descriptor->cur_text_x = 0;
    }
    else if ((ch == '\r') && (oled_descriptor->carriage_return_allowed != FALSE))
    {
        oled_descriptor->cur_text_x = 0;
    }
    else
    {
        uint16_t width = sh1122_get_glyph_width(oled_descriptor, ch);
        
        /* Check if we're not larger than the screen */
        if ((width + oled_descriptor->cur_text_x) > oled_descriptor->max_text_x)
        {
            if (oled_descriptor->line_feed_allowed != FALSE)
            {
                oled_descriptor->cur_text_y += oled_descriptor->current_font_header.height;
                oled_descriptor->cur_text_x = 0;
            }
            else
            {
                return RETURN_NOK;
            }
        }
        
        /* Check that we're not writing text after the screen edge */
        if ((oled_descriptor->cur_text_y + oled_descriptor->current_font_header.height) > SH1122_OLED_HEIGHT)
        {
            return RETURN_NOK;
        }
        
        // Display the text
        oled_descriptor->cur_text_x += sh1122_glyph_draw(oled_descriptor, oled_descriptor->cur_text_x, oled_descriptor->cur_text_y, ch, write_to_buffer);
    }
    
    return RETURN_OK;
}

/*! \fn     sh1122_put_string(sh1122_descriptor_t* oled_descriptor, const char* str, BOOL write_to_buffer)
*   \brief  Print string at current x y
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  str                 String to print
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return Number of printed chars
*/
uint16_t sh1122_put_string(sh1122_descriptor_t* oled_descriptor, const cust_char_t* str, BOOL write_to_buffer)
{
    uint16_t nb_printed_chars = 0;
    
    // Write chars until we find final 0
    while (*str)
    {
        if(sh1122_put_char(oled_descriptor, *str++, write_to_buffer) != RETURN_OK)
        {
            return nb_printed_chars;
        }
        else
        {
            nb_printed_chars++;
        }
    }
    
    return nb_printed_chars;
}

/*! \fn     sh1122_put_string_xy(sh1122_descriptor_t* oled_descriptor, int16_t x, uint8_t y, oled_align_te justify, const char* string)
*   \brief  Display an error string on the screen (X0Y0, centered)
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  string              Null terminated string
*   \return How many characters were printed
*/
uint16_t sh1122_put_error_string(sh1122_descriptor_t* oled_descriptor, const cust_char_t* string)
{
    return sh1122_put_string_xy(oled_descriptor, 0, 0, OLED_ALIGN_CENTER, string, FALSE);
}

/*! \fn     sh1122_put_string_xy(sh1122_descriptor_t* oled_descriptor, int16_t x, uint8_t y, oled_align_te justify, const char* string, BOOL write_to_buffer) 
*   \brief  Display a string on the screen
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  justify             String justify (see enum)
*   \param  string              Null terminated string
*   \param  write_to_buffer     Set to true to write to internal buffer
*   \return How many characters were printed
*/
uint16_t sh1122_put_string_xy(sh1122_descriptor_t* oled_descriptor, int16_t x, uint8_t y, oled_align_te justify, const cust_char_t* string, BOOL write_to_buffer) 
{
    uint16_t width = sh1122_get_string_width(oled_descriptor, string);
    int16_t max_text_x_copy = oled_descriptor->max_text_x;
    uint16_t return_val;

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