#ifndef _EMULATOR_H
#define _EMULATOR_H
#include <inttypes.h>
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

void emu_update_display(uint8_t *fb);
void emu_appexit_test(void);

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

void emu_send_hid(char *data, int size);
int emu_rcv_hid(char *data, int size);

#ifdef __cplusplus
}
#endif

#endif
