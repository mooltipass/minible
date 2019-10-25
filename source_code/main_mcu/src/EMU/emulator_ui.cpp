#include "emulator_ui.h"

#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSlider>
#include <QMenu>
#include <QFileDialog>
#include <QMutex>
#include <QMutexLocker>

#include "emulator.h"

// TODO this should not be in the UI file ?
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

static bool openSmartcard(QString filePath, bool createNew = false)
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

static void closeSmartcard() {
    QMutexLocker locker(&smc_mutex);
    smartcardFile.close();
    card_present = false;
}

EmuWindow::EmuWindow()
{
    auto layout = new QFormLayout(this);
    auto smartcard = createSmartcardUi();
    layout->addRow("Smartcard", smartcard);

    auto battery = new QSlider(Qt::Horizontal);
    battery->setMaximum(100);
    battery->setValue(75);
    layout->addRow("Battery", battery);
}

QWidget *EmuWindow::createSmartcardUi() 
{
    auto row_smartcard = new QWidget();
    auto smartcard_layout = new QHBoxLayout(row_smartcard);

    auto btn_insert = new QPushButton("Insert");
    auto btn_remove = new QPushButton("Remove");
    btn_remove->setEnabled(false);

    auto btn_insert_menu = new QMenu();
    auto act_invalid = btn_insert_menu->addAction("Invalid");
    QObject::connect(act_invalid, &QAction::triggered, this, [=]() {
        if(openSmartcard(QString())) {
            btn_remove->setEnabled(true);
            btn_insert->setEnabled(false);
        }
    });

    auto act_broken = btn_insert_menu->addAction("Broken");
    QObject::connect(act_broken, &QAction::triggered, this, [=]() {
        if(openSmartcard("!broken")) {
            btn_remove->setEnabled(true);
            btn_insert->setEnabled(false);
        }
    });


    auto act_new = btn_insert_menu->addAction("New (blank)");
    QObject::connect(act_new, &QAction::triggered, this, [=]() {
        auto fileName = QFileDialog::getSaveFileName(this, "Create smartcard file", "", "Smartcard Image Files (*.smc)");
        if(openSmartcard(fileName, true)) {
            btn_remove->setEnabled(true);
            btn_insert->setEnabled(false);
        }
    });

    auto act_existing = btn_insert_menu->addAction("Existing");
    QObject::connect(act_existing, &QAction::triggered, this, [=]() {
        auto fileName = QFileDialog::getOpenFileName(this, "Select smartcard file", "", "Smartcard Image Files (*.smc)");
        if(openSmartcard(fileName)) {
            btn_remove->setEnabled(true);
            btn_insert->setEnabled(false);
        }
    });

    btn_insert->setMenu(btn_insert_menu);

    QObject::connect(btn_remove, &QPushButton::clicked, this, [=]() {
        closeSmartcard();
        btn_remove->setEnabled(false);
        btn_insert->setEnabled(true);
    });

    smartcard_layout->addWidget(btn_insert);
    smartcard_layout->addWidget(btn_remove);

    return row_smartcard;
}
