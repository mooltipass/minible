/*!  \file     dbflash.c
*    \brief    Low level driver for AT45DB* flash
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/

#ifndef DBFLASH_MEM_H_
#define DBFLASH_MEM_H_

#include "defines.h"

// To define in project solution later
#define DBFLASH_CHIP_8M

// Enable boundary checks
#define DBFLASH_MEMORY_BOUNDARY_CHECKS

/* Prototypes */
void dbflash_write_data_pattern_to_flash(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber, uint16_t offset, uint16_t dataSize, uint8_t pattern);
void dbflash_send_pattern_data_with_four_bytes_opcode(spi_flash_descriptor_t* descriptor_pt, uint8_t* opcode, uint8_t pattern, uint16_t nb_bytes);
void dbflash_read_data_from_flash(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber, uint16_t offset, uint16_t dataSize, void *data);
void dbflash_send_data_with_four_bytes_opcode(spi_flash_descriptor_t* descriptor_pt, uint8_t* opcode, uint8_t* buffer, uint16_t buffer_size);
void dbflash_write_data_to_flash(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber, uint16_t offset, uint16_t dataSize, void *data);
void dbflash_write_buffer(spi_flash_descriptor_t* descriptor_pt, uint8_t* datap, uint16_t offset, uint16_t size);
void dbflash_raw_read(spi_flash_descriptor_t* descriptor_pt, uint8_t* datap, uint16_t addr, uint16_t size);
void dbflash_load_page_to_internal_buffer(spi_flash_descriptor_t* descriptor_pt, uint16_t page_number);
void dbflash_send_command(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length);
void dbflash_flash_write_buffer_to_page(spi_flash_descriptor_t* descriptor_pt, uint16_t page);
void dbflash_sector_zero_erase(spi_flash_descriptor_t* descriptor_pt, uint8_t sectorNumber);
void dbflash_sector_erase(spi_flash_descriptor_t* descriptor_pt, uint8_t sectorNumber);
void dbflash_block_erase(spi_flash_descriptor_t* descriptor_pt, uint16_t blockNumber);
void dbflash_page_erase(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber);
void dbflash_enter_ultra_deep_power_down(spi_flash_descriptor_t* descriptor_pt);
RET_TYPE dbflash_check_presence(spi_flash_descriptor_t* descriptor_pt);
void dbflash_wait_for_not_busy(spi_flash_descriptor_t* descriptor_pt);
void dbflash_format_flash(spi_flash_descriptor_t* descriptor_pt);
void dbflash_chip_erase(spi_flash_descriptor_t* descriptor_pt);
void dbflash_memory_boundary_error_callblack(void);

/* Defines */
#if defined(DBFLASH_CHIP_1M)      // Used to identify a 1M Flash Chip (AT45DB011D)
    #define DBFLASH_CHIP 1
    #define DBFLASH_CHIP_STR  "\x01"
#elif defined(DBFLASH_CHIP_2M)    // Used to identify a 2M Flash Chip (AT45DB021E)
    #define DBFLASH_CHIP 2
    #define DBFLASH_CHIP_STR  "\x02"
#elif defined(DBFLASH_CHIP_4M)    // Used to identify a 4M Flash Chip (AT45DB041E)
    #define DBFLASH_CHIP 4
    #define DBFLASH_CHIP_STR  "\x04"
#elif defined(DBFLASH_CHIP_8M)    // Used to identify a 8M Flash Chip (AT45DB081E)
    #define DBFLASH_CHIP 8
    #define DBFLASH_CHIP_STR  "\x08"
#elif defined(DBFLASH_CHIP_16M)   // Used to identify a 16M Flash Chip (AT45DB161E)
    #define DBFLASH_CHIP 16
    #define DBFLASH_CHIP_STR  "\x10"
#elif defined(DBFLASH_CHIP_32M)   // Used to identify a 32M Flash Chip (AT45DB321E)
    #define DBFLASH_CHIP 32
    #define DBFLASH_CHIP_STR  "\x20"
#endif

#if defined(DBFLASH_CHIP_1M)
    #define MAN_FAM_DEN_VAL 0x22       // Used for Chip Identity (see datasheet)
    #define PAGE_COUNT 512             // Number of pages in the chip
    #define BYTES_PER_PAGE 264         // Bytes per page of the chip
    #define MAP_BYTES 192              // Bytes required to make 'node' usage (map) -> (((PAGE_COUNT - PAGE_PER_SECTOR) * BYTES_PER_PAGE) / NODE_SIZE_PARENT) / 8bits
    #define MAP_PAGES 1                // Pages required to hold 'node' usage (map) -> CEILING(MAP_BYTES / BYTES_PER_PAGE)
    #define NODE_PARENT_PER_PAGE 4     // Number of parent nodes per page -> BYTES_PER_PAGE / NODE_SIZE_PARENT
	#define NODE_CHILD_MAX_NODE 2	   // Last valid child node block (due to size of child = size of parent * 2)
	//#define NODE_PER_PAGE 2            // Number of nodes per page
    #define BLOCK_COUNT 64             // Number of blocks in the chip
    #define SECTOR_START 1             // The first whole sector number in the chip
    #define SECTOR_END 3               // The last whole sector number in the chip
    #define PAGE_PER_SECTOR 128        // Number of pages per sector in the chip

    #define SECTOR_ERASE_0_SHT_AMT 12  // The shift amount used for a sector zero part erase (see comments below)
    #define SECTOR_ERASE_N_SHT_AMT 16  // The shift amount used for a sector erase (see comments below)
    #define BLOCK_ERASE_SHT_AMT 12     // The shift amount used for a block erase (see comments below)
    #define PAGE_ERASE_SHT_AMT 9       // The shift amount used for a page erase (see comments below)
    #define WRITE_SHT_AMT 9            // The shift amount used for a write operation (see comments below)
    #define READ_OFFSET_SHT_AMT 9      // The shift amount used for a read operation (see comments below)

    // sector erase -> 264size -> OP: 0x7C -> 0a/0b -> 3 address bytes -> 5 D/C, 7 Page address bits (PA9-PA3), 12 D/C -> PA3=0 -> 0a, PA3=1 -> 0b
    // sector erase -> 264size -> OP: 0x7C -> 1/3 0> 3 address bytes -> 6 D/C, 2 Page address bits (PA8-PA7), 16 D/C
    // sector 0a -> 8 pages, sector 0b -> 120 pages, sector 1/3 -> 128 page
    // block erase -> 264size -> OP: 0x50 -> 3 address bytes -> 6 D/C, 6 Block Num, 12 D/C -> 64 Blocks
    // page erase -> 264size -> OP: 0x81 -> 3 address bytes -> 6 D/C, 9 Page Num, 9 D/C -> 512 Pages

    // Write_p1 -> 264size -> MMP to Buffer T -> OP: 0x53 -> 3 address bytes -> 6 D/C, 9 Page Address, 9 D/C
    // write_p2 -> 264size -> MMP PROG T Buffer -> OP: 0x82 -> 3 address bytes -> 6 D/C, 9 Page Address, 9 Buffer Address

    // Read -> 528size -> Low Freq Read -> 0P: 0x03 -> 3 address bytes -> 9 Page Address, 9 Offset, 6 D/C ?
#elif defined(DBFLASH_CHIP_2M)
    #define MAN_FAM_DEN_VAL 0x23       // Used for Chip Identity (see datasheet)
    #define PAGE_COUNT 1024            // Number of pages in the chip
    #define BYTES_PER_PAGE 264         // Bytes per page of the chip
    #define MAP_BYTES 448              // Bytes required to make 'node' usage (map) -> (((PAGE_COUNT - PAGE_PER_SECTOR) * BYTES_PER_PAGE) / NODE_SIZE_PARENT) / 8bits
    #define MAP_PAGES 2                // Pages required to hold 'node' usage (map) -> CEILING(MAP_BYTES / BYTES_PER_PAGE)
    #define NODE_PARENT_PER_PAGE 4     // Number of parent nodes per page -> BYTES_PER_PAGE / NODE_SIZE_PARENT
	#define NODE_CHILD_MAX_NODE 2	   // Last valid child node block (due to size of child = size of parent * 2)
    //#define NODE_PER_PAGE 2            // Number of nodes per page
    #define BLOCK_COUNT 128            // Number of blocks in the chip
    #define SECTOR_START 1             // The first whole sector number in the chip
    #define SECTOR_END 7               // The last whole sector number in the chip
    #define PAGE_PER_SECTOR 128        // Number of pages per sector in the chip

    #define SECTOR_ERASE_0_SHT_AMT 12  // The shift amount used for a sector zero part erase (see comments below)
    #define SECTOR_ERASE_N_SHT_AMT 16  // The shift amount used for a sector erase (see comments below)
    #define BLOCK_ERASE_SHT_AMT 12     // The shift amount used for a block erase (see comments below)
    #define PAGE_ERASE_SHT_AMT 9       // The shift amount used for a page erase (see comments below)
    #define WRITE_SHT_AMT 9            // The shift amount used for a write operation (see comments below)
    #define READ_OFFSET_SHT_AMT 9      // The shift amount used for a read operation (see comments below)

    // sector erase -> 264size -> OP: 0x7C -> 0a/0b -> 3 address bytes -> 5 D/C, 7 Sector Address (PA9-PA3), 12 D/C -> PA3=0 -> 0a, PA3=1 -> 0b
    // sector erase -> 264size -> OP: 0x7C -> 1/7 -> 3 address bytes -> 5 D/C, 3 Sector Address Bits, 16 D/C
    // sector 0a -> 8 pages, sector 0b -> 120 pages, sector 1/7 -> 128 pages
    // block erase -> 264size -> OP: 0x50 -> 3 address bytes -> 5 D/C, 7 Block Num, 12 D/C -> 128 Blocks
    // page erase -> 264size -> OP: 0x81 -> 3 address bytes -> 5 D/C, 10 Page Num, 9 D/C -> 1024 pages

    // Write_p1 -> 264size -> MMP to Buffer T -> OP: 0x53 -> 3 address bytes -> 5 D/C, 10 Page Address, 9 D/C
    // write_p2 -> 264size -> MMP PROG T Buffer -> OP: 0x82 -> 3 address bytes -> 5 D/C, 10 Page Address, 9 Buffer Address

    // Read -> 528size -> Low Freq Read -> 0P: 0x03 -> 3 address bytes -> 10 Page Address, 9 Offset, 5 D/C ?
#elif defined(DBFLASH_CHIP_4M)
    #define MAN_FAM_DEN_VAL 0x24       // Used for Chip Identity (see datasheet)
    #define PAGE_COUNT 2048            // Number of pages in the chip
    #define BYTES_PER_PAGE 264         // Bytes per page of the chip
    #define MAP_BYTES 896              // Bytes required to make 'node' usage (map) -> (((PAGE_COUNT - PAGE_PER_SECTOR) * BYTES_PER_PAGE) / NODE_SIZE_PARENT) / 8bits
    #define MAP_PAGES 4                // Pages required to hold 'node' usage (map) -> CEILING(MAP_BYTES / BYTES_PER_PAGE)
    #define NODE_PARENT_PER_PAGE 4     // Number of parent nodes per page -> BYTES_PER_PAGE / NODE_SIZE_PARENT
	#define NODE_CHILD_MAX_NODE 2	   // Last valid child node block (due to size of child = size of parent * 2)
	//#define NODE_PER_PAGE 2            // Number of nodes per page
    #define BLOCK_COUNT 256            // Number of blocks in the chip
    #define SECTOR_START 1             // The first whole sector number in the chip
    #define SECTOR_END 7               // The last whole sector number in the chip
    #define PAGE_PER_SECTOR 256        // Number of pages per sector in the chip

    #define SECTOR_ERASE_0_SHT_AMT 12  // The shift amount used for a sector zero part erase (see comments below)
    #define SECTOR_ERASE_N_SHT_AMT 17  // The shift amount used for a sector erase (see comments below)
    #define BLOCK_ERASE_SHT_AMT 12     // The shift amount used for a block erase (see comments below)
    #define PAGE_ERASE_SHT_AMT 9       // The shift amount used for a page erase (see comments below)
    #define WRITE_SHT_AMT 9            // The shift amount used for a write operation (see comments below)
    #define READ_OFFSET_SHT_AMT 9      // The shift amount used for a read operation (see comments below)

    // sector erase -> 264size -> OP: 0x7C -> 0a/0b -> 3 address bytes -> 4 D/C, 8 Sector Address (PA10-PA3), 12 D/C -> PA3=0 -> 0a, PA3=1 -> 0b
    // sector erase -> 264size -> OP: 0x7C -> 1/7 -> 3 address bytes -> 4 D/C, 3 Sector Address Bits, 17 D/C
    // sector 0a -> 8 pages, sector 0b -> 248 pages, sector 1/7 -> 256 pages
    // block erase -> 264size -> OP: 0x50 -> 3 address bytes -> 4 D/C, 8 Block Num, 12 D/C -> 256 Blocks
    // page erase -> 264size -> OP: 0x81 -> 3 address bytes -> 4 D/C, 11 Page Num, 9 D/C -> 2048 Pages

    // Write_p1 -> 264size -> MMP to Buffer T -> OP: 0x53 -> 3 address bytes -> 4 D/C, 11 Page Address, 9 D/C
    // write_p2 -> 264size -> MMP PROG T Buffer -> OP: 0x82 -> 3 address bytes -> 4 D/C, 11 Page Address, 9 Buffer Address

    // Read -> 528size -> Low Freq Read -> 0P: 0x03 -> 3 address bytes -> 11 Page Address, 9 Offset, 4 D/C ?
#elif defined(DBFLASH_CHIP_8M)
    #define MAN_FAM_DEN_VAL 0x25       // Used for Chip Identity (see datasheet)
    #define PAGE_COUNT 4096            // Number of pages in the chip
    #define BYTES_PER_PAGE 264         // Bytes per page of the chip
    #define MAP_BYTES 1920             // Bytes required to make 'node' usage (map) -> (((PAGE_COUNT - PAGE_PER_SECTOR) * BYTES_PER_PAGE) / NODE_SIZE_PARENT) / 8bits
    #define MAP_PAGES 8                // Pages required to hold 'node' usage (map) -> CEILING(MAP_BYTES / BYTES_PER_PAGE)
    #define NODE_PARENT_PER_PAGE 4     // Number of parent nodes per page -> BYTES_PER_PAGE / NODE_SIZE_PARENT
	#define NODE_CHILD_MAX_NODE 2	   // Last valid child node block (due to size of child = size of parent * 2)
	//#define NODE_PER_PAGE 2            // Number of nodes per page
    #define BLOCK_COUNT 512            // Number of blocks in the chip
    #define SECTOR_START 1             // The first whole sector number in the chip
    #define SECTOR_END 15               // The last whole sector number in the chip
    #define PAGE_PER_SECTOR 256        // Number of pages per sector in the chip

    #define SECTOR_ERASE_0_SHT_AMT 12  // The shift amount used for a sector zero part erase (see comments below)
    #define SECTOR_ERASE_N_SHT_AMT 17  // The shift amount used for a sector erase (see comments below)
    #define BLOCK_ERASE_SHT_AMT 12     // The shift amount used for a block erase (see comments below)
    #define PAGE_ERASE_SHT_AMT 9       // The shift amount used for a page erase (see comments below)
    #define WRITE_SHT_AMT 9            // The shift amount used for a write operation (see comments below)
    #define READ_OFFSET_SHT_AMT 9      // The shift amount used for a read operation (see comments below)

    // sector erase -> 264size -> OP: 0x7C -> 0a/0b -> 3 address bytes -> 3 D/C, 9 Sector Address (PA11-PA3), 12 D/C -> PA3=0 -> 0a, PA3=1 -> 0b
    // sector erase -> 264size -> OP: 0x7C -> 1/15 -> 3 address bytes -> 3 D/C, 4 Sector Address Bits, 17 D/C
    // sector 0a -> 8 pages, sector 0b -> 248 pages, sector 1/15 -> 256 pages
    // block erase -> 264size -> OP: 0x50 -> 3 address bytes -> 3 D/C, 9 Block Num, 12 D/C -> 512 Blocks
    // page erase -> 264size -> OP: 0x81 -> 3 address bytes -> 3 D/C, 12 Page Num, 9 D/C -> 4096 Pages

    // Write_p1 -> 264size -> MMP to Buffer T -> OP: 0x53 -> 3 address bytes -> 3 D/C, 12 Page Address, 9 D/C
    // write_p2 -> 264size -> MMP PROG T Buffer -> OP: 0x82 -> 3 address bytes -> 3 D/C, 12 Page Address, 9 Buffer Address

    // Read -> 528size -> Low Freq Read -> 0P: 0x03 -> 3 address bytes -> 12 Page Address, 9 Offset, 3 D/C ?
#elif defined(DBFLASH_CHIP_16M)
    #define MAN_FAM_DEN_VAL 0x26       // Used for Chip Identity (see datasheet)
    #define PAGE_COUNT 4096            // Number of pages in the chip
    #define BYTES_PER_PAGE 528         // Bytes per page of the chip
    #define MAP_BYTES 3840             // Bytes required to make 'node' usage (map) -> (((PAGE_COUNT - PAGE_PER_SECTOR) * BYTES_PER_PAGE) / NODE_SIZE_PARENT) / 8bits
    #define MAP_PAGES 8                // Pages required to hold 'node' usage (map) -> CEILING(MAP_BYTES / BYTES_PER_PAGE)
    #define NODE_PARENT_PER_PAGE 8     // Number of parent nodes per page -> BYTES_PER_PAGE / NODE_SIZE_PARENT
	#define NODE_CHILD_MAX_NODE 6	   // Last valid child node block (due to size of child = size of parent * 2)
	//#define NODE_PER_PAGE 4            // Number of nodes per page
    #define BLOCK_COUNT 512            // Number of blocks in the chip
    #define SECTOR_START 1             // The first whole sector number in the chip
    #define SECTOR_END 15              // The last whole sector number in the chip
    #define PAGE_PER_SECTOR 256        // Number of pages per sector in the chip

    #define SECTOR_ERASE_0_SHT_AMT 13  // The shift amount used for a sector zero part erase (see comments below)
    #define SECTOR_ERASE_N_SHT_AMT 18  // The shift amount used for a sector erase (see comments below)
    #define BLOCK_ERASE_SHT_AMT 13     // The shift amount used for a block erase (see comments below)
    #define PAGE_ERASE_SHT_AMT 10      // The shift amount used for a page erase (see comments below)
    #define WRITE_SHT_AMT 10           // The shift amount used for a write operation (see comments below)
    #define READ_OFFSET_SHT_AMT 10     // The shift amount used for a read operation (see comments below)

    // sector erase -> 528size -> OP: 0x7C -> 0a/0b -> 3 address bytes -> 2 D/C, 9 Sector Address (PA11-PA3), 13 D/C -> PA3=0 -> 0a, PA3=1 -> 0b
    // sector erase -> 528size -> OP: 0x7C -> 1/15 -> 3 address bytes -> 2 D/C, 4 Sector Address Bits, 18 D/C
    // sector 0a -> 8 pages, sector 0b -> 248 pages, sector 1/15 -> 256 pages
    // block erase -> 528size -> OP: 0x50 -> 3 address bytes -> 2 D/C, 9 Block Num, 13 D/C -> 512 Blocks
    // page erase -> 528size -> OP: 0x81 -> 3 address bytes -> 2 D/C, 12 Page Num, 10 D/C -> 4096 Pages

    // Write_p1 -> 528size -> MMP to Buffer T -> OP: 0x53 -> 3 address bytes -> 2 D/C, 12 Page Address, 10 D/C
    // write_p2 -> 528size -> MMP PROG T Buffer -> OP: 0x82 -> 3 address bytes -> 2 D/C, 12 Page Address, 10 Buffer Address

    // Read -> 528size -> Low Freq Read -> 0P: 0x03 -> 3 address bytes -> 12 Page Address, 10 Offset, 2 D/C ?
#elif defined(DBFLASH_CHIP_32M)
    #define MAN_FAM_DEN_VAL 0x27       // Used for Chip Identity (see datasheet)
    #define PAGE_COUNT 8192            // Number of pages in the chip
    #define BYTES_PER_PAGE 528         // Bytes per page of the chip
    #define MAP_BYTES 8064             // Bytes required to make 'node' usage (map) -> (((PAGE_COUNT - PAGE_PER_SECTOR) * BYTES_PER_PAGE) / NODE_SIZE_PARENT) / 8bits
    #define MAP_PAGES 16               // Pages required to hold 'node' usage (map) -> CEILING(MAP_BYTES / BYTES_PER_PAGE)
    #define NODE_PARENT_PER_PAGE 8     // Number of parent nodes per page -> BYTES_PER_PAGE / NODE_SIZE_PARENT
	#define NODE_CHILD_MAX_NODE 6	   // Last valid child node block (due to size of child = size of parent * 2)
	//#define NODE_PER_PAGE 4            // Number of nodes per page
    #define BLOCK_COUNT 1024           // Number of blocks in the chip
    #define SECTOR_START 1             // The first whole sector number in the chip
    #define SECTOR_END 63              // The last whole sector number in the chip
    #define PAGE_PER_SECTOR 128        // Number of pages per sector in the chip

    #define SECTOR_ERASE_0_SHT_AMT 13  // The shift amount used for a sector zero part erase (see comments below)
    #define SECTOR_ERASE_N_SHT_AMT 17  // The shift amount used for a sector erase (see comments below)
    #define BLOCK_ERASE_SHT_AMT 13     // The shift amount used for a block erase (see comments below)
    #define PAGE_ERASE_SHT_AMT 10      // The shift amount used for a page erase (see comments below)
    #define WRITE_SHT_AMT 10           // The shift amount used for a write operation (see comments below)
    #define READ_OFFSET_SHT_AMT 10     // The shift amount used for a read operation (see comments below)

    // sector erase -> 528size -> OP: 0x7C -> 0a/0b -> 3 address bytes -> 1 D/C, 10 Sector Address (PA12-PA3), 13 D/C -> PA3=0 -> 0a, PA3=1 -> 0b
    // sector erase -> 528size -> OP: 0x7C -> 1/63 -> 3 address bytes -> 1 D/C, 6 Sector Address Bits, 17 D/C
    // sector 0a -> 8 pages, sector 0b -> 120 pages, sector 1/63 -> 128 pages
    // block erase -> 528size -> OP: 0x50 -> 3 address bytes -> 1 D/C, 10 Block Num, 13 D/C -> 1024 Blocks
    // page erase -> 528size -> OP: 0x81 -> 3 address bytes -> 1 D/C, 13 Page Num, 10 D/C -> 8192 Pages

    // Write_p1 -> 528size -> MMP to Buffer T -> OP: 0x53 -> 3 address bytes -> 1 D/C, 13 Page Address, 10 D/C
    // write_p2 -> 528size -> MMP PROG T Buffer -> OP: 0x82 -> 3 address bytes -> 1 D/C, 13 Page Address, 10 Buffer Address

    // Read -> 528size -> Low Freq Read -> 0P: 0x03 -> 3 address bytes -> 13 Page Address, 10 Offset, 1 D/C ?
#endif

// Common for all flash chips
#define DBFLASH_MANUF_ID                    0x1F
#define DBFLASH_OPCODE_SECTOR_ERASE         0x7C  // Opcode to perform a sector erase
#define DBFLASH_OPCODE_BLOCK_ERASE          0x50  // Opcode to perform a block erase
#define DBFLASH_OPCODE_PAGE_ERASE           0x81  // Opcode to perform a page erase
#define DBFLASH_OPCODE_READ_STAT_REG        0xD7  // Opcode to perform a read of the status register
#define DBFLASH_OPCODE_MAINP_TO_BUF         0x53  // Opcode to perform a Main Memory Page to Buffer Transfer
#define DBFLASH_OPCODE_MMP_PROG_TBUF        0x82  // Opcode to perform a Main Memory Page Program Through Buffer
#define DBFLASH_OPCODE_LOWF_READ            0x03  // Opcode to perform a Continuous Array Read (Low Frequency)
#define DBFLASH_OPCODE_BUF_WRITE            0x84  // Opcode to write into buffer
#define DBFLASH_OPCODE_BUF_TO_PAGE          0x83  // Opcode to write buffer to given page
#define DBFLASH_OPCODE_READ_DEV_INFO        0x9F  // Opcode to perform a Manufacturer and Device ID Read
#define DBFLASH_OPCODE_UDEEP_PDOWN_ENTER    0x79  // Opcode to enter ultra deep powerdown
#define DBFLASH_READY_BITMASK               0x80  // Bitmask used to determine if the chip is ready (poll status register). Used with DBFLASH_OPCODE_READ_STAT_REG.
#define DBFLASH_SECTOR_ZER0_A_PAGES         8
#define DBFLASH_SECTOR_ZERO_A_CODE          0
#define DBFLASH_SECTOR_ZERO_B_CODE          1

// Flash Page Mappings
#define DBFLASH_PAGE_MAPPING_NODE_META_DATA  0  // Reserving two (2) pages for node management meta data
#define DBFLASH_PAGE_MAPPING_NODE_MAP_START  2  // Reserving two (2) pages for node management meta data
#define DBFLASH_PAGE_MAPPING_NODE_MAP_END    (DBFLASH_PAGE_MAPPING_NODE_MAP_START + MAP_PAGES)  // Last page used for node mapping
#define DBFLASH_PAGE_MAPPING_GFX_START       (DBFLASH_PAGE_MAPPING_NODE_MAP_END + 1)  // Start GFX Mapping
#define DBFLASH_PAGE_MAPPING_GFX_END         (PAGE_PER_SECTOR) // End GFX Mapping
#define DBFLASH_PAGE_MAPPING_GFX_SIZE        (DBFLASH_PAGE_MAPPING_GFX_END - DBFLASH_PAGE_MAPPING_GFX_START)

// Flash size defines
#define DBFLASH_SIZE          ((uint32_t)PAGE_COUNT * (uint32_t)BYTES_PER_PAGE)

#endif /* DBFLASH_MEM_H_ */