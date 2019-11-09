#include "emu_storage.h"

#include <stdlib.h>
#include <QDebug>
#include <QFile>

static QFile eeprom("eeprom.bin");
static QFile dbflash("dbflash.bin");
static bool emu_open_flash(QFile & flashFile)
{
    if(!flashFile.open(QIODevice::ReadWrite)) {
        qWarning() << "Failed to open emulated flash" << flashFile.fileName();
        abort();
    }

    return flashFile.size() > 0;
}

static void emu_extend_flash(QFile & flashFile, int size)
{
    if(flashFile.size() < size) {
        flashFile.seek(flashFile.size());
        int extend_size = size - flashFile.size();
        flashFile.write(QByteArray(extend_size, '\xff'));
    }
    
    flashFile.flush();
}

static void emu_flash_read(QFile & flashFile, int offset, uint8_t *buf, int length) 
{
    if(flashFile.isOpen()) {
        emu_extend_flash(flashFile, offset+length);
        flashFile.seek(offset);
        flashFile.read((char*)buf, length);

    } else {
        memset(buf, 0xff, length);
    }
}

static void emu_flash_write(QFile & flashFile, int offset, uint8_t *buf, int length)
{
    if(flashFile.isOpen()) {
        emu_extend_flash(flashFile, offset+length);
        flashFile.seek(offset);
        flashFile.write((char*)buf, length);
        flashFile.flush();
    }
}

BOOL emu_eeprom_open()
{
    return emu_open_flash(eeprom);
}

void emu_eeprom_read(int offset, uint8_t *buf, int length)
{
    return emu_flash_read(eeprom, offset, buf, length);
}

void emu_eeprom_write(int offset, uint8_t *buf, int length)
{
    return emu_flash_write(eeprom, offset, buf, length);
}

BOOL emu_dbflash_open()
{
    return emu_open_flash(dbflash);
}

void emu_dbflash_read(int offset, uint8_t *buf, int length)
{
    return emu_flash_read(dbflash, offset, buf, length);
}

void emu_dbflash_write(int offset, uint8_t *buf, int length)
{
    return emu_flash_write(dbflash, offset, buf, length);
}

