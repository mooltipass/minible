#ifndef _EMULATOR_H
#define _EMULATOR_H
#include <inttypes.h>
#include "defines.h"

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

int emu_get_battery_level(void);
BOOL emu_get_usb_charging(void);
void emu_charger_enable(BOOL en);

BOOL emu_get_systick(uint32_t *value);

#ifdef __cplusplus
}
#endif

#endif
