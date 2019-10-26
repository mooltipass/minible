#include "emulator_ui.h"

#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSlider>
#include <QMenu>
#include <QFileDialog>
#include <QMutex>
#include <QMutexLocker>

#include "emu_smartcard.h"

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
    if(emu_is_smartcard_inserted())
        btn_insert->setEnabled(false);
    else
        btn_remove->setEnabled(false);

    auto btn_insert_menu = new QMenu();
    auto act_invalid = btn_insert_menu->addAction("Invalid");
    QObject::connect(act_invalid, &QAction::triggered, this, [=]() {
        if(emu_insert_smartcard(QString())) {
            btn_remove->setEnabled(true);
            btn_insert->setEnabled(false);
        }
    });

    auto act_broken = btn_insert_menu->addAction("Broken");
    QObject::connect(act_broken, &QAction::triggered, this, [=]() {
        if(emu_insert_smartcard("!broken")) {
            btn_remove->setEnabled(true);
            btn_insert->setEnabled(false);
        }
    });


    auto act_new = btn_insert_menu->addAction("New (blank)");
    QObject::connect(act_new, &QAction::triggered, this, [=]() {
        auto fileName = QFileDialog::getSaveFileName(this, "Create smartcard file", "", "Smartcard Image Files (*.smc)");
        if(fileName.isEmpty())
            return;

        if(emu_insert_smartcard(fileName, true)) {
            btn_remove->setEnabled(true);
            btn_insert->setEnabled(false);
        }
    });

    auto act_existing = btn_insert_menu->addAction("Existing");
    QObject::connect(act_existing, &QAction::triggered, this, [=]() {
        auto fileName = QFileDialog::getOpenFileName(this, "Select smartcard file", "", "Smartcard Image Files (*.smc)");
        if(fileName.isEmpty())
            return;

        if(emu_insert_smartcard(fileName)) {
            btn_remove->setEnabled(true);
            btn_insert->setEnabled(false);
        }
    });

    btn_insert->setMenu(btn_insert_menu);

    QObject::connect(btn_remove, &QPushButton::clicked, this, [=]() {
        emu_remove_smartcard();
        btn_remove->setEnabled(false);
        btn_insert->setEnabled(true);
    });

    smartcard_layout->addWidget(btn_insert);
    smartcard_layout->addWidget(btn_remove);

    return row_smartcard;
}
