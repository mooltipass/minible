#ifndef EMU_STORAGE_H
#define EMU_STORAGE_H
#include <inttypes.h>
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL emu_eeprom_open(void);
void emu_eeprom_read(int offset, uint8_t *buf, int length);
void emu_eeprom_write(int offset, uint8_t *buf, int length);

BOOL emu_dbflash_open(void);
void emu_dbflash_read(int offset, uint8_t *buf, int length);
void emu_dbflash_write(int offset, uint8_t *buf, int length);

#ifdef __cplusplus
}
#endif


#endif
