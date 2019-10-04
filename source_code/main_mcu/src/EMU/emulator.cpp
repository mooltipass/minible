extern "C" {
#include "asf.h"
#include "driver_timer.h"
#include "emulator.h"
}

#include <QApplication>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include <QPainter>

#include "qt_metacall_helper.h"

static struct emu_port_t _PORT;
struct emu_port_t *PORT=&_PORT;

class OLEDWidget: public QWidget {
public:
    QImage display;
    OLEDWidget(): display(256, 64, QImage::Format_RGB888) {
        setMinimumSize(display.size());
        setMaximumSize(display.size());
    }

    void update_display(const uint8_t *fb) {
        uint8_t *optr = display.bits();
        for(int y=0;y<64;y++)
            for(int x=0;x<256;x++) {
                optr[0] = optr[1] = optr[2] = *fb++;
                optr+=3;
            }
       
        repaint();
    }

protected:
    virtual void paintEvent(QPaintEvent *) {
        QPainter painter(this);
        painter.drawImage(0, 0, display);
    }
};

extern "C" void minible_main();

class AppThread: public QThread {
private:

public:
    void run() {
        minible_main();
    }


};

static OLEDWidget *oled;
void emu_update_display(uint8_t *fb)
{
    QByteArray fb_copy(reinterpret_cast<const char*>(fb), 256*64);
    postToObject([=]() { oled->update_display(reinterpret_cast<const uint8_t*>(fb_copy.constData())); }, oled);
}

int main(int ac, char ** av)
{
    // Qt needs to run on the main thread. We run the application code on a separate thread
    // (1) to ensure responsiveness when the main code blocks
    // (2) to have our input behave in an interrupt-like manner
    QApplication app(ac, av);
    QTimer ms_timer;
    ms_timer.setInterval(1);
    ms_timer.start();

    QObject::connect(&ms_timer, &QTimer::timeout, []() {
        // this will likely miss some ticks, FIXME later
        timer_ms_tick();

        /* Smartcard detect */
//            smartcard_lowlevel_detect();
    
        /* Scan buttons */
//            inputs_scan();
        
        /* Power logic */
//            logic_power_ms_tick();

    });

    oled = new OLEDWidget;
    oled->show();

    AppThread app_thread;
    app_thread.start();

    app.exec();

    delete oled;
    return 0;
}
