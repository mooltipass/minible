#include "emu_storage.h"

#include <QFile>
static QFile eeprom("eeprom.bin");

void emu_eeprom_read(int offset, uint8_t *buf, int length)
{
    if(!eeprom.isOpen())
        eeprom.open(QIODevice::ReadWrite);

    memset(buf, 0xff, length);
    eeprom.seek(offset);
    eeprom.read((char*)buf, length); // ignore errors, missing data will read as 0xff
}

void emu_eeprom_write(int offset, uint8_t *buf, int length)
{
    if(!eeprom.isOpen())
        eeprom.open(QIODevice::ReadWrite);
    eeprom.seek(offset);
    eeprom.write((char*)buf, length);
    eeprom.flush();
}
