#include "dbflash.h"
#include "emu_storage.h"

#include <stdlib.h>
#include <string.h>

void dbflash_write_data_pattern_to_flash(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber, uint16_t offset, uint16_t dataSize, uint8_t pattern)
{
    char *tmp = malloc(dataSize);
    memset(tmp, pattern, dataSize);
    dbflash_write_data_to_flash(descriptor_pt, pageNumber, offset, dataSize, tmp);
    free(tmp);
}

void dbflash_read_data_from_flash(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber, uint16_t offset, uint16_t dataSize, void *data)
{
    emu_dbflash_read(pageNumber * BYTES_PER_PAGE + offset, data, dataSize);
}

void dbflash_write_data_to_flash(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber, uint16_t offset, uint16_t dataSize, void *data)
{
    emu_dbflash_write(pageNumber * BYTES_PER_PAGE + offset, data, dataSize);
}

void dbflash_page_erase(spi_flash_descriptor_t* descriptor_pt, uint16_t pageNumber)
{
    char *tmp = malloc(BYTES_PER_PAGE);
    memset(tmp, 0xFF, BYTES_PER_PAGE);
    dbflash_write_data_to_flash(descriptor_pt, pageNumber, 0, BYTES_PER_PAGE, tmp);
    free(tmp);
}

static BOOL initialized = FALSE;

RET_TYPE dbflash_check_presence(spi_flash_descriptor_t* descriptor_pt)
{   
    if(!initialized) {
        initialized = TRUE;
        emu_dbflash_open();
    }

    return RETURN_OK;
}
