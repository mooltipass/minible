#ifndef EMU_SMARTCARD_H
#define EMU_SMARTCARD_H
#include <inttypes.h>
#include "defines.h"

#ifdef __cplusplus
#include <QString>

bool emu_insert_smartcard(QString filePath, bool createNew = false);
void emu_remove_smartcard();
bool emu_is_smartcard_inserted();

extern "C" {
#endif

struct emu_smartcard_t {
    uint8_t smc[1568];
    uint8_t fuses[3];
    uint8_t type;
};

// NULL means no card present
struct emu_smartcard_t *emu_open_smartcard(void);
void emu_close_smartcard(BOOL written);

enum { EMU_SMARTCARD_BLANK, EMU_SMARTCARD_INVALID, EMU_SMARTCARD_BROKEN };
void emu_init_smartcard(struct emu_smartcard_t *smartcard, int smartcard_type);

#ifdef __cplusplus
}
#endif

#endif
