extern "C" {
#include "asf.h"
#include "driver_timer.h"
#include "emulator.h"
#include "inputs.h"
}

#include <QApplication>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include <QSemaphore>
#include <QMutex>
#include <QTime>
#include <QLocalSocket>

#include "emu_oled.h"
#include "emu_smartcard.h"
#include "emulator_ui.h"

static struct emu_port_t _PORT;
struct emu_port_t *PORT=&_PORT;

QMutex irq_mutex;

void cpu_irq_enter_critical(void)
{
    irq_mutex.lock();
}

void cpu_irq_leave_critical(void)
{
    irq_mutex.unlock();
}

static void pseudo_irq(void)
{
    irq_mutex.lock();
    timer_ms_tick();

    /* Scan buttons */
    inputs_scan();
    
    /* Power logic */
//            logic_power_ms_tick();

    irq_mutex.unlock();
}

extern "C" void minible_main();

class AppThread: public QThread {
private:
    QMutex appexit_mutex;
    bool app_exiting = false;
    QSemaphore app_thread_blocked;

    QLocalSocket *hid;

    bool reconnect_hid() {
        if(hid->state() != QLocalSocket::ConnectedState) {
            hid->connectToServer("moolticuted_local_dev");
            hid->waitForConnected(10);
        }
        
        return hid->state() == QLocalSocket::ConnectedState;
    }

public:
    void run() {
        hid = new QLocalSocket;
        minible_main();
    }

    void stop() {
        appexit_mutex.lock();
        app_exiting = true;
        appexit_mutex.unlock();
        app_thread_blocked.acquire();
    }

    void test_stop(void) {
        appexit_mutex.lock();
        if(app_exiting) {
            appexit_mutex.unlock();
            app_thread_blocked.release();
            for(;;);
        }

        appexit_mutex.unlock();
    }

    void send_hid(char *data, int size) {
        if(!reconnect_hid())
            return;

        while(size > 0) {
            int nb = hid->write(data, size);
            if(nb <= 0)
                break;

            data += nb;
            size -= nb;

            if(size > 0)
                hid->waitForBytesWritten();
        }
    }

    int rcv_hid(char *data, int size) {
        test_stop();
        if(!reconnect_hid())
            return -1;

        hid->waitForReadyRead(0);
        int nb = hid->read(data, size);
        return nb > 0 ? nb : 0;
    }
};

AppThread app_thread;

void emu_appexit_test(void) {
    app_thread.test_stop();
}

OLEDWidget *oled;

void emu_send_hid(char *data, int size)
{
    app_thread.send_hid(data, size);
}

int emu_rcv_hid(char *data, int size)
{
    return app_thread.rcv_hid(data, size);
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

    QObject::connect(&ms_timer, &QTimer::timeout, [] () {
        // the OS will most likely not schedule our timer with 1ms frequency,
        // so we correct for this by running the "irq" function several times if needed
        static QTime timer = QTime::currentTime();
        for(int msPassed = timer.restart(); msPassed >= 0; --msPassed) 
            pseudo_irq();
    });

    oled = new OLEDWidget;

    if(av[1])
        emu_insert_smartcard(av[1]);

    EmuWindow emu_window;
    emu_window.show();

    oled->show();
    app_thread.start();

    app.exec();

    app_thread.stop();

    delete oled;
    return 0;
}
