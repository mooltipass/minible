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
 * custom_bitstream.h
 *
 * Created: 16/05/2017 15:42:40
 *  Author: stephan
 */ 


#ifndef CUSTOM_BITSTREAM_H_
#define CUSTOM_BITSTREAM_H_

#include "custom_fs.h"
#include "defines.h"

/* Typedefs */
typedef struct
{
    uint8_t mask;               //*< pixel mask for returned data
    uint16_t width;             //*< number of pixels wide
    uint8_t height;             //*< number of pixels high
    uint8_t bitsPerPixel;       //*< number of bits per pixel
    uint16_t _size;             //*< number of pixels
    uint8_t _bits;              //*< current offset in data word
    uint16_t _word;             //*< current bitmap word
    uint16_t _count;            //*< number of bytes / words read
    uint8_t _pixel;             //*< current pixel for RLE decompress
    uint8_t _flags;		        //*< format flags.  E.g. RLE=1
    custom_fs_address_t addr;	//*< address of data in SPI FLASH store.
    uint8_t buf[2][32];	        //*< FLASH read-ahead buffer
    uint32_t bufInd;	        //*< read-ahead buffer index
    uint32_t bufSel;            //*< specify which of the 2 buffers we're using
    BOOL _exclusive_transfer;   //*< boolean to specify if no other bitmap transfer will take place at the same time
    BOOL _dma_transfer;         //*< boolean to specify if we're using DMA transfers (only convenient for big bitmaps)
} bitstream_bitmap_t;

/* Prototypes */
void bitstream_glyph_bitmap_init(bitstream_bitmap_t* bs, font_header_t* font, font_glyph_t* glyph, custom_fs_address_t address, BOOL exclusive);
void bitstream_bitmap_init(bitstream_bitmap_t* bs, bitmap_t* bitmap, custom_fs_address_t address, BOOL exclusive);
void bitstream_bitmap_array_read(bitstream_bitmap_t* bs, uint8_t* data, uint16_t nb_pixels);
uint16_t bitstream_bitmap_read(bitstream_bitmap_t* bs, uint16_t nb_pixels);
uint8_t bitstream_bitmap_two_pixel_read(bitstream_bitmap_t* bs);
void bitstream_bitmap_close(bitstream_bitmap_t* bs);

#endif /* CUSTOM_BITSTREAM_H_ */