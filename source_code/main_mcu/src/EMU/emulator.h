#ifndef _EMULATOR_H
#define _EMULATOR_H
#include <inttypes.h>

void emu_update_display(uint8_t *fb);
void emu_appexit_test(void);

void emu_send_aux(char *data, int size);
int emu_rcv_aux(char *data, int size);

#endif
