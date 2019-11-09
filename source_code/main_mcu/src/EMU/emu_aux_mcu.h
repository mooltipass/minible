#ifndef EMU_AUX_MCU_H
#define EMU_AUX_MCU_H

void emu_send_aux(char *data, int size);
int emu_rcv_aux(char *data, int size);

#endif
