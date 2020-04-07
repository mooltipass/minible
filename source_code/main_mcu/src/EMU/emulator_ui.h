#ifndef EMULATOR_UI_H
#define EMULATOR_UI_H

#include <QWidget>

class EmuWindow: public QWidget {
public:
    EmuWindow();

private:
    QWidget *createSmartcardUi();
    QWidget *createBatteryUi();
    QWidget *createChargerUi();
    QWidget *createAccelerometerUi();
};

#endif
