#ifndef EMU_SMARTCARD_H
#define EMU_SMARTCARD_H
#include <inttypes.h>
#include "defines.h"

#ifdef __cplusplus
#include <QString>
extern "C" {
#endif

/* this is not an accurate model of the card, but it works for our purposes */
struct emu_smartcard_storage_t {
    uint8_t smc[1568];
    uint8_t fuses[3];
    uint8_t type;
};

struct emu_smartcard_t {
    struct emu_smartcard_storage_t storage;
    BOOL unlocked;
};

// NULL means no card present
struct emu_smartcard_t *emu_open_smartcard(void);
void emu_close_smartcard(BOOL written);

enum { EMU_SMARTCARD_REGULAR, EMU_SMARTCARD_INVALID, EMU_SMARTCARD_BROKEN };
void emu_init_smartcard(struct emu_smartcard_storage_t *smartcard, int smartcard_type);
void emu_reset_smartcard();

#ifdef __cplusplus

bool emu_insert_smartcard(QString filePath);
bool emu_insert_new_smartcard(QString filePath, int smartcard_type = EMU_SMARTCARD_REGULAR);
void emu_remove_smartcard();
bool emu_is_smartcard_inserted();


}
#endif

#endif
