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

/*! \fn     sh1122_oled_off(sh1122_descriptor_t* oled_descriptor)
*   \brief  Switch on the screen
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*/
void sh1122_oled_off(sh1122_descriptor_t* oled_descriptor)
{
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_DISPLAY_OFF);
    oled_descriptor->oled_on = FALSE;
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

    /* Switch screen on */    
    sh1122_write_single_command(oled_descriptor, SH1122_CMD_SET_DISPLAY_ON);
    oled_descriptor->oled_on = TRUE;
    
    /* Try to set default font */
    sh1122_refresh_used_font(oled_descriptor);
    
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

/*! \fn     sh1122_draw_aligned_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_bitmap_t* bitstream)
*   \brief  Draw a 2 pixels-aligned picture from a bitstream
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  bitstream           Pointer to the bistream
*/
void sh1122_draw_aligned_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_bitmap_t* bitstream)
{
    uint16_t height = bitstream->height;
    uint16_t width = bitstream->width;
    
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
        
    /* Close bitstream */
    bitstream_bitmap_close(bitstream);    
}   

/*! \fn     sh1122_draw_non_aligned_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_bitmap_t* bitstream)
*   \brief  Draw a 2 pixels non aligned picture from a bitstream
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  bitstream           Pointer to the bistream
*/
void sh1122_draw_non_aligned_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_bitmap_t* bitstream)
{
    uint16_t height = bitstream->height;
    uint16_t width = bitstream->width;
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
    
    /* Close bitstream */
    bitstream_bitmap_close(bitstream);
}    

/*! \fn     sh1122_draw_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_t* bs)
*   \brief  Draw a picture from a bitstream
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  bitstream           Pointer to the bistream
*/
void sh1122_draw_image_from_bitstream(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, bitstream_bitmap_t* bitstream)
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
        sh1122_draw_aligned_image_from_bitstream(oled_descriptor, x, y, bitstream);
    } 
    else
    {
        sh1122_draw_non_aligned_image_from_bitstream(oled_descriptor, x, y, bitstream);
    }    
}

/*! \fn     sh1122_display_bitmap_from_flash(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, uint32_t file_id)
*   \brief  Display a bitmap stored in the external flash
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  file_id             Bitmap file ID
*/
RET_TYPE sh1122_display_bitmap_from_flash(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, uint32_t file_id)
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
    sh1122_draw_image_from_bitstream(oled_descriptor, x, y, &bitstream);
    
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
    font_glyph_t glyph;
    uint16_t gind;
    
    /* Check that a font was actually chosen */
    if (oled_descriptor->currentFontAddress != 0)
    {
        /* We only support characters after ' ' */
        if (ch < ' ')
        {
            return 0;
        }
        
        /* Check if the char isn't above the one we actually support */
        if (ch > (oled_descriptor->current_font_header.last_chr_val + ' '))
        {
            if ('?' > (oled_descriptor->current_font_header.last_chr_val + ' '))
            {
                return 0;
            }
            else
            {
                ch = '?';                
            }
        }
        
        /* Convert character to glyph index */
        custom_fs_read_from_flash((uint8_t*)&gind, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + (ch - ' ')*sizeof(gind), sizeof(gind));
        
        // Check that we know this glyph
        if(gind == 0xFFFF)
        {
            // If we don't know this character, try again with '?'
            if ('?' > (oled_descriptor->current_font_header.last_chr_val + ' '))
            {
                return 0;
            }
            else
            {
                ch = '?';
            }            
            custom_fs_read_from_flash((uint8_t*)&gind, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + (ch - ' ')*sizeof(gind), sizeof(gind));
            
            // If we still don't know it, return 0
            if (gind == 0xFFFF)
            {
                return 0;
            }
        }

        // Read the beginning of the glyph
        custom_fs_read_from_flash((uint8_t*)&glyph, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + (oled_descriptor->current_font_header.last_chr_val+1)*sizeof(gind) + gind*sizeof(glyph), sizeof(glyph));

        if (glyph.glyph == 0xFFFFFFFF)
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

 /*! \fn     sh1122_glyph_draw(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, char ch)
 *   \brief  Draw a character glyph on the screen at x,y.
 *   \param  oled_descriptor    Pointer to a sh1122 descriptor struct
 *   \param  x                  x position to start glyph
 *   \param  y                  y position to start glyph
 *   \param  ch                 Character to draw
 *   \return width of the glyph
 */
uint16_t sh1122_glyph_draw(sh1122_descriptor_t* oled_descriptor, int16_t x, int16_t y, cust_char_t ch)
{
    bitstream_bitmap_t bs;              // Character bitstream
    uint8_t glyph_width;                // Glyph width
    font_glyph_t glyph;                 // Glyph header
    uint16_t gind;                      // Glyph index

    /* Check for selected font */
    if (oled_descriptor->currentFontAddress == 0)
    {
        return 0;
    }
    
    /* We only support characters after ' ' */
    if (ch < ' ')
    {
        return 0;
    }
    
    /* Check if the char isn't above the one we actually support */
    if (ch > (oled_descriptor->current_font_header.last_chr_val + ' '))
    {
        if ('?' > (oled_descriptor->current_font_header.last_chr_val + ' '))
        {
            return 0;
        }
        else
        {
            ch = '?';
        }
    }
    
    /* Convert character to glyph index */
    custom_fs_read_from_flash((uint8_t*)&gind, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + (ch - ' ')*sizeof(gind), sizeof(gind));
     
    // Check that we know this glyph
    if(gind == 0xFFFF)
    {
        // If we don't know this character, try again with '?'
        if ('?' > (oled_descriptor->current_font_header.last_chr_val + ' '))
        {
            return 0;
        }
        else
        {
            ch = '?';
        }
        custom_fs_read_from_flash((uint8_t*)&gind, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + (ch - ' ')*sizeof(gind), sizeof(gind));
         
        // If we still don't know it, return 0
        if (gind == 0xFFFF)
        {
            return 0;
        }
    }
    
    /* Read glyph data */
    custom_fs_read_from_flash((uint8_t*)&glyph, oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + (oled_descriptor->current_font_header.last_chr_val+1)*sizeof(gind) + gind*sizeof(glyph), sizeof(glyph));

    if (glyph.glyph == 0xFFFFFFFF)
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
        custom_fs_address_t gaddr = oled_descriptor->currentFontAddress + sizeof(oled_descriptor->current_font_header) + (oled_descriptor->current_font_header.last_chr_val+1)*sizeof(gind) + (oled_descriptor->current_font_header.chr_count) * sizeof(glyph) + glyph.glyph;
        
        // Initialize bitstream & draw the character
        bitstream_glyph_bitmap_init(&bs, &oled_descriptor->current_font_header, &glyph, gaddr, TRUE);
        sh1122_draw_image_from_bitstream(oled_descriptor, x, y, &bs);
    }
    
    return (uint8_t)(glyph_width + glyph.xoffset) + 1;
}

/*! \fn     sh1122_put_char(sh1122_descriptor_t* oled_descriptor, char ch)
*   \brief  Print char on display
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  ch                  Char to display
*   \return success status
*/
RET_TYPE sh1122_put_char(sh1122_descriptor_t* oled_descriptor, cust_char_t ch)
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
        oled_descriptor->cur_text_x += sh1122_glyph_draw(oled_descriptor, oled_descriptor->cur_text_x, oled_descriptor->cur_text_y, ch);
    }
    
    return RETURN_OK;
}

/*! \fn     sh1122_put_string(sh1122_descriptor_t* oled_descriptor, const char* str)
*   \brief  Print string at current x y
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  str                 String to print
*   \return Number of printed chars
*/
uint16_t sh1122_put_string(sh1122_descriptor_t* oled_descriptor, const cust_char_t* str)
{
    uint16_t nb_printed_chars = 0;
    
    // Write chars until we find final 0
    while (*str)
    {
        if(sh1122_put_char(oled_descriptor, *str++) != RETURN_OK)
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
*   \brief  Display a string on the screen
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  justify             String justify (see enum)
*   \param  string              Null terminated culotte
*   \return How many characters were printed
*/
uint16_t sh1122_put_string_xy(sh1122_descriptor_t* oled_descriptor, int16_t x, uint8_t y, oled_align_te justify, const cust_char_t* string) 
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
    return_val = sh1122_put_string(oled_descriptor, string);
    oled_descriptor->max_text_x = max_text_x_copy;
    
    // Return the number of characters printed
    return return_val;
}

#ifdef OLED_PRINTF_ENABLED
/*! \fn     sh1122_printf_xy(sh1122_descriptor_t* oled_descriptor, int16_t x, uint8_t y, uint8_t justify, const char *fmt, ...) 
*   \brief  Printf string on the display
*   \param  oled_descriptor     Pointer to a sh1122 descriptor struct
*   \param  x                   Starting x
*   \param  y                   Starting y
*   \param  justify             String justify (see enum)
*   \return How many characters were printed
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
uint16_t sh1122_printf_xy(sh1122_descriptor_t* oled_descriptor, int16_t x, uint8_t y, oled_align_te justify, const char *fmt, ...) 
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
    return_val = sh1122_put_string(oled_descriptor, u16buf);
    oled_descriptor->max_text_x = max_text_x_copy;
    
    // Return the number of characters printed
    return return_val;
}
#pragma GCC diagnostic pop

#endif