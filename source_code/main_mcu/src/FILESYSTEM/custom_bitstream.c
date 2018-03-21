/*
 * custom_bitstream.c
 *
 * Created: 16/05/2017 15:42:53
 *  Author: stephan
 */
#include "platform_defines.h"
#include "custom_bitstream.h"
#include "custom_fs.h"
#include "dma.h"


/*! \fn     bitstream_bitmap_init(bitstream_bitmap_t* bs, bitmap_t* bitmap, custom_fs_address_t address, BOOL exclusive)
*   \brief  Initialize a bitmap bitstream
*   \param  bs          Pointer to a bitmap bitstream structure
*   \param  bitmap      Pointer to a bitmap header structure
*   \param  address     Bitmap data address in flash
*   \param  exclusive   Bool to specify if this transfer is exclusive on the bus
*/
void bitstream_bitmap_init(bitstream_bitmap_t* bs, bitmap_t* bitmap, custom_fs_address_t address, BOOL exclusive)
{
    bs->bitsPerPixel = bitmap->depth;
    bs->width = bitmap->width;
    bs->height = bitmap->height;
    bs->_size = bitmap->dataSize;
    bs->mask = (1 << bs->bitsPerPixel) - 1;
    bs->_bits = 0;
    bs->_word = 0xAA55;
    bs->_count = 0;
    bs->_flags = bitmap->flags;
    bs->addr = address;
    bs->bufSel = 0;
    bs->_exclusive_transfer = exclusive;

    /* In case you want to implement a DMA enabling strategy... */
    #ifdef FLASH_ALONE_ON_SPI_BUS
        #ifdef FLASH_DMA_FETCHES
            /* Implement your logic below */
            BOOL dma_enabled = TRUE;
        #else
            /* Implement your logic below */
            BOOL dma_enabled = FALSE;
        #endif
    #endif
       
    /* DMA transfers can't be enabled if the flash isn't alone on the bus */ 
    #ifdef FLASH_ALONE_ON_SPI_BUS
        if (dma_enabled != FALSE)
        {
            bs->bufInd = 0;
            bs->_dma_transfer = TRUE;
            
            /* Start filling our buffer */
            custom_fs_continuous_read_from_flash(bs->buf[bs->bufSel], bs->addr, sizeof(bs->buf[0]), bs->_dma_transfer);
            while(dma_custom_fs_check_and_clear_dma_transfer_flag() == FALSE);

            /* Increment address counter */
            bs->addr += sizeof(bs->buf[0]);

            /* Trigger filling of next buffer */
            custom_fs_continuous_read_from_flash(bs->buf[(bs->bufSel+1)&0x01], bs->addr, sizeof(bs->buf[0]), bs->_dma_transfer);

            /* Increment address counter */
            bs->addr += sizeof(bs->buf[0]);                
        } 
        else
        {
            bs->bufInd = sizeof(bs->buf[0]);
            bs->_dma_transfer = FALSE;
        }
    #else
        bs->bufInd = sizeof(bs->buf[0]);
        bs->_dma_transfer = FALSE;
    #endif
}

/*! \fn     bitstream_glyph_bitmap_init(bitstream_bitmap_t* bs, font_header_t* font, font_glyph_t* glyph, custom_fs_address_t address, BOOL exclusive)
*   \brief  Initialize a glyph bitstream
*   \param  bs          Pointer to a bitmap bitstream structure
*   \param  font        Pointer to a font structure
*   \param  glyph       Pointer to a glyph structure
*   \param  address     Bitmap data address in flash
*   \param  exclusive   Bool to specify if this transfer is exclusive on the bus
*/
void bitstream_glyph_bitmap_init(bitstream_bitmap_t* bs, font_header_t* font, font_glyph_t* glyph, custom_fs_address_t address, BOOL exclusive)
{
    bs->bitsPerPixel = font->depth;
    bs->width = glyph->xrect;
    bs->height = glyph->yrect;
    bs->_size = ((bs->width*bs->bitsPerPixel+7)/8) * bs->height;
    bs->mask = (1 << bs->bitsPerPixel) - 1;
    bs->_bits = 0;
    bs->_word = 0xAA55;
    bs->_count = 0;
    bs->_flags = 0;
    bs->addr = address;
    bs->bufSel = 0;
    bs->_exclusive_transfer = exclusive;

    /* In case you want to implement a DMA enabling strategy... */
    #ifdef FLASH_ALONE_ON_SPI_BUS
        #ifdef FLASH_DMA_FETCHES
            /* Implement your logic below: by default, not enabled for glyphs */
            BOOL dma_enabled = FALSE;
        #else
            /* Implement your logic below */
            BOOL dma_enabled = FALSE;
       #endif
   #endif
   
    /* DMA transfers can't be enabled if the flash isn't alone on the bus */
    #ifdef FLASH_ALONE_ON_SPI_BUS
        if (dma_enabled)
        {
            bs->bufInd = 0;
            bs->_dma_transfer = TRUE;

            /* Start filling our buffer */
            custom_fs_continuous_read_from_flash(bs->buf[bs->bufSel], bs->addr, sizeof(bs->buf[0]), bs->_dma_transfer);
            while(dma_custom_fs_check_and_clear_dma_transfer_flag() == FALSE);

            /* Increment address counter */
            bs->addr += sizeof(bs->buf[0]);

            /* Trigger filling of next buffer */
            custom_fs_continuous_read_from_flash(bs->buf[(bs->bufSel+1)&0x01], bs->addr, sizeof(bs->buf[0]), bs->_dma_transfer);

            /* Increment address counter */
            bs->addr += sizeof(bs->buf[0]);
        }
        else
        {
            bs->bufInd = sizeof(bs->buf[0]);
            bs->_dma_transfer = FALSE;
        }
    #else
        bs->bufInd = sizeof(bs->buf[0]);
        bs->_dma_transfer = FALSE;
    #endif
}

/*! \fn     bitstream_bitmap_get_next_byte(bitstream_bitmap_t* bs)
*   \brief  Get the next byte of a bitmap bitstream
*   \param  bs          Pointer to a bitmap bitstream structure
*   \return The next byte, or 0 if we already read too many bytes
*/
static inline uint8_t bitstream_bitmap_get_next_byte(bitstream_bitmap_t* bs)
{
    /* Check if didn't read too much data */
    if (bs->_count < bs->_size) 
    {
        /* Increment read counter */
        bs->_count++;
        
        /* If we have used all our internal buffer, read additional bytes from flash */
        if (bs->bufInd < sizeof(bs->buf[0])) 
        {
            return bs->buf[bs->bufSel][bs->bufInd++];
        }
        else 
        {
            #ifdef FLASH_ALONE_ON_SPI_BUS
                if (bs->_exclusive_transfer == FALSE)
                {
                    custom_fs_read_from_flash(bs->buf[bs->bufSel], bs->addr, sizeof(bs->buf[0]));
                } 
                else
                {
                    if (bs->_dma_transfer != FALSE)
                    {
                        /* Trigger a new DMA transfer on current buffer and switch to the new one */
                        while(dma_custom_fs_check_and_clear_dma_transfer_flag() == FALSE);
                        custom_fs_continuous_read_from_flash(bs->buf[bs->bufSel], bs->addr, sizeof(bs->buf[0]), bs->_dma_transfer);
                        bs->bufSel = (bs->bufSel+1)&0x01;
                    }
                    else
                    {
                        custom_fs_continuous_read_from_flash(bs->buf[bs->bufSel], bs->addr, sizeof(bs->buf[0]), bs->_dma_transfer);
                    }
                }
            #else
                custom_fs_read_from_flash(bs->buf[bs->bufSel], bs->addr, sizeof(bs->buf[0]));
            #endif
            bs->addr += sizeof(bs->buf[0]);
            bs->bufInd = 0;
            return bs->buf[bs->bufSel][bs->bufInd++];
        }
    }
    else
    {
        return 0;
    }
}

/*! \fn     bitstream_bitmap_array_read(bitstream_bitmap_t* bs, uint8_t* data, uint16_t nb_pixels)
*   \brief  Read continuous pixel data
*   \param  bs          Pointer to a bitmap bitstream structure
*   \param  data        Pointer to where to store the data
*   \param  nb_pixels   Number of pixels to be read (multiple of 4!)
*/
void bitstream_bitmap_array_read(bitstream_bitmap_t* bs, uint8_t* data, uint16_t nb_pixels)
{
    if (bs->_flags & CUSTOM_FS_BITMAP_RLE_FLAG)
    {
        while (nb_pixels != 0)
        {
            if (bs->_bits == 0)
            {
                /* We have read all pixels of the same color */
                uint8_t byte = bitstream_bitmap_get_next_byte(bs);
                bs->_bits = (byte >> 4) + 1;
                bs->_pixel = byte & 0x0F;
            }
            
            /* We have pixels of the same color to store */
            *data = bs->_pixel << 4;
            bs->_bits--;
            nb_pixels--;
                
            if (bs->_bits == 0)
            {
                /* We have read all pixels of the same color */
                uint8_t byte = bitstream_bitmap_get_next_byte(bs);
                bs->_bits = (byte >> 4) + 1;
                bs->_pixel = byte & 0x0F;
            }
            
            /* We have pixels of the same color to store */
            *data |= bs->_pixel;
            bs->_bits--;
            nb_pixels--;
            data++;
        }
    }
    else
    {
        while (nb_pixels != 0)
        {
            *data = 0;
            for (uint16_t i = 0; i < 2; i++)
            {
                *data <<= 4;
                if (bs->_bits == 0)
                {
                    /* We have processed all data inside _word */
                    bs->_word = bitstream_bitmap_get_next_byte(bs);
                    bs->_bits = 8;
                }
                if (bs->_bits >= bs->bitsPerPixel)
                {
                    /* Move pixel data from _word to data */
                    bs->_bits -= bs->bitsPerPixel;
                    *data |= (((bs->_word >> bs->_bits) & bs->mask) * 15) / bs->mask;
                }
                else
                {
                    /* Pixel depth not aligned with word */
                    uint8_t offset = bs->bitsPerPixel - bs->_bits;
                    *data |= (bs->_word << offset & bs->mask);
                    bs->_bits += 8 - bs->bitsPerPixel;
                    bs->_word = bitstream_bitmap_get_next_byte(bs);
                    *data |= ((bs->_word >> bs->_bits) * 15) / bs->mask;
                }                
            }
            nb_pixels-=2;
            data++;
        }
    }        
}

/*! \fn     bitstream_bitmap_close(bitstream_bitmap_t* bs)
*   \brief  Close an ongoing bitstream
*   \param  bs          Pointer to a bitmap bitstream structure
*/
void bitstream_bitmap_close(bitstream_bitmap_t* bs)
{
    #ifdef FLASH_ALONE_ON_SPI_BUS
        if (bs->_dma_transfer != FALSE)
        {        
            while(dma_custom_fs_check_and_clear_dma_transfer_flag() == FALSE);
        }
        custom_fs_stop_continuous_read_from_flash();
    #endif    
}

 /*! \fn     bitstream_bitmap_two_pixel_read(bitstream_bitmap_t* bs)
 *   \brief  Get 2 4-bit pixels
 *   \param  bs          Pointer to a bitmap bitstream structure
 *   \return An uint8_t containing 2 4-bit pixels
 */
uint8_t bitstream_bitmap_two_pixel_read(bitstream_bitmap_t* bs)
{
    uint8_t data = 0;
    
    if (bs->_flags & CUSTOM_FS_BITMAP_RLE_FLAG) 
    {
        /* for speed, no for loop */
        if (bs->_bits == 0) 
        {
            /* We have read all pixels of the same color */
            uint8_t byte = bitstream_bitmap_get_next_byte(bs);
            bs->_bits = (byte >> 4) + 1;
            bs->_pixel = byte & 0x0F;
        }
        if (bs->_bits) 
        {
            /* We still have pixels of the same color to send */
            data |= bs->_pixel;
            bs->_bits--;
        }
        data <<= 4;
        if (bs->_bits == 0)
        {
            /* We have read all pixels of the same color */
            uint8_t byte = bitstream_bitmap_get_next_byte(bs);
            bs->_bits = (byte >> 4) + 1;
            bs->_pixel = byte & 0x0F;
        }
        if (bs->_bits)
        {
            /* We still have pixels of the same color to send */
            data |= bs->_pixel;
            bs->_bits--;
        }
    }
    else
    {
        for(uint32_t i = 0; i < 2; i++)
        {
            data <<= 4;
            if (bs->_bits == 0) 
            {
                /* We have processed all data inside _word */ 
                bs->_word = bitstream_bitmap_get_next_byte(bs);
                bs->_bits = 8;
            }
            if (bs->_bits >= bs->bitsPerPixel) 
            {
                /* Move pixel data from _word to data */
                bs->_bits -= bs->bitsPerPixel;
                data |= (((bs->_word >> bs->_bits) & bs->mask) * 15) / bs->mask;
            }
            else 
            {
                /* Pixel depth not aligned with word */
                uint8_t offset = bs->bitsPerPixel - bs->_bits;
                data |= (bs->_word << offset & bs->mask);
                bs->_bits += 8 - bs->bitsPerPixel;
                bs->_word = bitstream_bitmap_get_next_byte(bs);
                data |= ((bs->_word >> bs->_bits) * 15) / bs->mask;
            }
        }
    }
    
    return data;
}

 /*! \fn     bitstream_bitmap_read(bitstream_bitmap_t* bs, uint8_t numPixels)
 *   \brief  Get up to 4 4-bit pixels from the bitstream
 *   \param  bs          Pointer to a bitmap bitstream structure
 *   \param  nb_pixels   Number of pixels to be read (max 4!)
 *   \return An uint16_t containing up to 4 4-bit pixels
 */
uint16_t bitstream_bitmap_read(bitstream_bitmap_t* bs, uint16_t nb_pixels)
{
    uint16_t data = 0;
    
    if (bs->_flags & CUSTOM_FS_BITMAP_RLE_FLAG) 
    {
        while (nb_pixels--) 
        {
            data <<= 4;
            if (bs->_bits == 0) 
            {
                /* We have read all pixels of the same color */
                uint8_t byte = bitstream_bitmap_get_next_byte(bs);
                bs->_bits = (byte >> 4) + 1;
                bs->_pixel = byte & 0x0F;
            }
            if (bs->_bits) 
            {
                /* We still have pixels of the same color to send */
                data |= bs->_pixel;
                bs->_bits--;
            }
        }
    }
    else
    {
        while (nb_pixels--) 
        {
            data <<= 4;
            if (bs->_bits == 0) 
            {
                /* We have processed all data inside _word */ 
                bs->_word = bitstream_bitmap_get_next_byte(bs);
                bs->_bits = 8;
            }
            if (bs->_bits >= bs->bitsPerPixel) 
            {
                /* Move pixel data from _word to data */
                bs->_bits -= bs->bitsPerPixel;
                data |= (((bs->_word >> bs->_bits) & bs->mask) * 15) / bs->mask;
            }
            else 
            {
                /* Pixel depth not aligned with word */
                uint8_t offset = bs->bitsPerPixel - bs->_bits;
                data |= (bs->_word << offset & bs->mask);
                bs->_bits += 8 - bs->bitsPerPixel;
                bs->_word = bitstream_bitmap_get_next_byte(bs);
                data |= ((bs->_word >> bs->_bits) * 15) / bs->mask;
            }
        }
    }
    
    return data;
}
