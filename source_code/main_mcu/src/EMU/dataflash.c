#include "dataflash.h"
#include "emu_dataflash.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

static int bundle_fd = -1;

void emu_dataflash_init(const char *path)
{
    int i;
    const char *bundle_paths[] = {
        "/usr/share/misc/miniblebundle.img",
        "miniblebundle.img",
        "../../scripts/python_framework/bundle.img",
        "bundle.img",
        NULL
    };


    if(path && *path) {
        bundle_paths[0] = path;
        bundle_paths[1] = NULL;
    }

    for(i = 0; bundle_paths[i] != NULL;i++) {
        bundle_fd = open(bundle_paths[i], O_RDONLY);
        if(bundle_fd >= 0)
            return;
    }

    fprintf(stderr, "Failed to open bundle file, tried:\n");
    for(i = 0; bundle_paths[i] != NULL;i++)
        fprintf(stderr, "\t%s\n", bundle_paths[i]);

}

void dataflash_write_array_to_memory(spi_flash_descriptor_t* descriptor_pt, uint32_t address, uint8_t* data, uint32_t length){}
void dataflash_read_data_array(spi_flash_descriptor_t* descriptor_pt, uint32_t address, uint8_t* data, uint32_t length) 
{
    lseek(bundle_fd, address, SEEK_SET);
    read(bundle_fd, data, length);
}

void dataflash_read_bytes_from_opened_transfer(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length) {
    read(bundle_fd, data, length);
}

void dataflash_send_command(spi_flash_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length){}
void dataflash_send_single_byte_command(spi_flash_descriptor_t* descriptor_pt, uint8_t command){}
void dataflash_read_data_array_start(spi_flash_descriptor_t* descriptor_pt, uint32_t address) {
    lseek(bundle_fd, address, SEEK_SET);
}

void dataflash_erase_64kb_block(spi_flash_descriptor_t* descriptor_pt, uint32_t address){}
void dataflash_bulk_erase_without_wait(spi_flash_descriptor_t* descriptor_pt){}
uint8_t dataflash_read_status_register(spi_flash_descriptor_t* descriptor_pt){return 0;}
void dataflash_stop_ongoing_transfer(spi_flash_descriptor_t* descriptor_pt){}
void dataflash_bulk_erase_with_wait(spi_flash_descriptor_t* descriptor_pt){}
RET_TYPE dataflash_check_presence(spi_flash_descriptor_t* descriptor_pt){return RETURN_OK;}
void dataflash_send_write_enable(spi_flash_descriptor_t* descriptor_pt){}
void dataflash_wait_for_not_busy(spi_flash_descriptor_t* descriptor_pt){}
void dataflash_exit_power_down(spi_flash_descriptor_t* descriptor_pt){}
RET_TYPE dataflash_is_busy(spi_flash_descriptor_t* descriptor_pt){ return RETURN_OK; }
void dataflash_power_down(spi_flash_descriptor_t* descriptor_pt){}
