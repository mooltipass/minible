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

BOOL emu_get_lefthanded(void);

int emu_get_failure_flags(void);
/* Keep this in sync with EmuWindow::createFailuresUi */
enum {
    EMU_FAIL_SMARTCARD_INSECURE=1,
    EMU_FAIL_DBFLASH_FULL=2,
    EMU_FAIL_EEPROM_FULL=4,
};

#ifdef __cplusplus
}
#endif

#endif
