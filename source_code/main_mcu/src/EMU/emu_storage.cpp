#include "emu_storage.h"

#include <QDebug>
#include <QFile>

static QFile eeprom("eeprom.bin");
static QFile dbflash("dbflash.bin");
static void emu_open_flash(QFile & flashFile)
{
    if(!flashFile.isOpen() && !flashFile.fileName().isEmpty()) {
        if(!flashFile.open(QIODevice::ReadWrite)) {
            qWarning() << "Failed to open emulated flash" << flashFile.fileName();
            flashFile.setFileName(QString()); // mark the file as unusable
        }
    }
}

static void emu_extend_flash(QFile & flashFile, int size)
{
    flashFile.seek(flashFile.size());

    while(flashFile.size() < size) 
        flashFile.putChar('\xff');
    
    flashFile.flush();
}

static void emu_flash_read(QFile & flashFile, int offset, uint8_t *buf, int length) 
{
    emu_open_flash(flashFile);

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

void emu_eeprom_read(int offset, uint8_t *buf, int length)
{
    return emu_flash_read(eeprom, offset, buf, length);
}

void emu_eeprom_write(int offset, uint8_t *buf, int length)
{
    return emu_flash_write(eeprom, offset, buf, length);
}

void emu_dbflash_read(int offset, uint8_t *buf, int length)
{
    return emu_flash_read(dbflash, offset, buf, length);
}

void emu_dbflash_write(int offset, uint8_t *buf, int length)
{
    return emu_flash_write(dbflash, offset, buf, length);
}

