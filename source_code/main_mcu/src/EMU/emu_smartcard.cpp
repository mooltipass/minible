#include "emu_smartcard.h"

#include <QMutex>
#include <QMutexLocker>
#include <QFile>

static QMutex smc_mutex;
static emu_smartcard_t card;
static bool card_present;
static QFile smartcardFile;

struct emu_smartcard_t *emu_open_smartcard()
{
    smc_mutex.lock();
    if(card_present) {
        return &card;

    } else {
        smc_mutex.unlock();
        return NULL;
    }
}

void emu_close_smartcard(BOOL written)
{
    if(written) {
        smartcardFile.seek(0);
        smartcardFile.write((char*)&card, sizeof(card));
    }
    smc_mutex.unlock();
}

bool emu_insert_smartcard(QString filePath, bool createNew)
{
    QMutexLocker locker(&smc_mutex);
    smartcardFile.close();

    if(filePath.isEmpty()) {
        card_present = true;
        emu_init_smartcard(&card, EMU_SMARTCARD_INVALID);

    } else if(filePath == "!broken") {
        card_present = true;
        emu_init_smartcard(&card, EMU_SMARTCARD_BROKEN);

    } else {
        smartcardFile.close();
        smartcardFile.setFileName(filePath);
        if(createNew) {
            if(!smartcardFile.open(QIODevice::ReadWrite))
                return false;

            emu_init_smartcard(&card, EMU_SMARTCARD_BLANK);
            smartcardFile.write((char*)&card, sizeof(card));
            card_present = true;

        } else {
            if(!smartcardFile.exists() || !smartcardFile.open(QIODevice::ReadWrite))
                return false;

            smartcardFile.read((char*)&card, sizeof(card));
            card_present = true;
        }
    }

    return true;
}

void emu_remove_smartcard() {
    QMutexLocker locker(&smc_mutex);
    smartcardFile.close();
    card_present = false;
}

bool emu_is_smartcard_inserted()
{
    return card_present;
}
