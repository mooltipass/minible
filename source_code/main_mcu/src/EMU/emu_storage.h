#ifndef EMU_STORAGE_H
#define EMU_STORAGE_H
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

void emu_eeprom_read(int offset, uint8_t *buf, int length);
void emu_eeprom_write(int offset, uint8_t *buf, int length);

#ifdef __cplusplus
}
#endif


#endif
