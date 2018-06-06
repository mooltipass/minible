/**
 * \file    comm_bootloader.h
 * \author  MBorregoTrujillo
 * \date    29-May-2018
 * \brief   Communications Manager for Bootloader
 */

#ifndef COMM_BOOTLOADER_H_
#define COMM_BOOTLOADER_H_

#include <asf.h>

/* Defines */
#define BOOTLOADER_PROGRAMMING_COMMAND  (0x0000)
#define BOOTLOADER_WRITE_COMMAND        (0x0001)

/* Magic key equal to BOOT, will be stored in reserved space of
 * BOOTLOADER_ENTER_PROGRAMMING_COMMAND
 */
#define BOOTLOADER_MAGIC_KEY            (0x8007)

COMPILER_PACK_SET(1)

/* Binary Image info */
typedef struct{
    uint16_t reserved;
    uint32_t size;
    uint32_t crc;
} T_comm_bootloader_enter_programming;

/* Write Data */
typedef struct{
    uint16_t size;
    uint32_t crc;
    uint32_t data[128];
} T_comm_bootloader_write;


/* Bootloader Message */
typedef struct{
    uint16_t command;
    union {
        T_comm_bootloader_enter_programming programming_command;
        T_comm_bootloader_write write_command;
    };
} T_comm_bootloader_message;

COMPILER_PACK_RESET()

/* Prototypes */
bool comm_bootloader_process_msg(T_comm_bootloader_message *msg);
bool comm_bootloader_enter_programming_required(void);


#endif /* COMM_BOOTLOADER_H_ */