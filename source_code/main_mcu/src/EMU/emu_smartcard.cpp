#include "emu_smartcard.h"

#include <QMutex>
#include <QMutexLocker>
#include <QFile>

static QMutex smc_mutex;
static emu_smartcard_t card;
static bool card_present = false;
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
        smartcardFile.flush();
    }
    smc_mutex.unlock();
}

bool emu_insert_smartcard(QString filePath)
{
    QMutexLocker locker(&smc_mutex);
    smartcardFile.close();
    
    memset(&card, 0, sizeof(card));

    smartcardFile.setFileName(filePath);
    if(!smartcardFile.exists() || !smartcardFile.open(QIODevice::ReadWrite))
        return false;

    smartcardFile.read((char*)&card.storage, sizeof(card.storage));
    card_present = true;

    return true;
}

bool emu_insert_new_smartcard(QString filePath, int smartcard_type)
{
    QMutexLocker locker(&smc_mutex);
    smartcardFile.close();
    
    memset(&card, 0, sizeof(card));

    card_present = true;
    emu_init_smartcard(&card.storage, smartcard_type);

    if(!filePath.isEmpty()) {
        smartcardFile.setFileName(filePath);
        if(!smartcardFile.open(QIODevice::ReadWrite))
            return false;

        smartcardFile.write((char*)&card.storage, sizeof(card.storage));
        smartcardFile.flush();
    }

    card_present = true;
    return true;
}

void emu_remove_smartcard() {
    QMutexLocker locker(&smc_mutex);
    smartcardFile.close();
    card_present = false;
}

void emu_reset_smartcard() {
    QMutexLocker locker(&smc_mutex);
    card.unlocked = FALSE;
}

bool emu_is_smartcard_inserted()
{
    return card_present;
}
