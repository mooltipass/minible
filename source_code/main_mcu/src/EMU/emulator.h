#ifndef _EMULATOR_H
#define _EMULATOR_H
#include <inttypes.h>

#ifdef __cplusplus

class QMutex;
extern QMutex irq_mutex;

class OLEDWidget;
extern OLEDWidget *oled;

extern "C" {
#endif

void emu_appexit_test(void);
void emu_send_hid(char *data, int size);
int emu_rcv_hid(char *data, int size);

#ifdef __cplusplus
}
#endif

#endif
