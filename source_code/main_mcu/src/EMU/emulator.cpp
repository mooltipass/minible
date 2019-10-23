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
#include <QPainter>
#include <QSemaphore>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QMutex>
#include <QTime>
#include <QLocalSocket>

#include "qt_metacall_helper.h"

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

// see INPUTS/inputs.c
extern volatile int16_t inputs_wheel_cur_increment;
extern volatile uint16_t inputs_wheel_click_duration_counter;
extern volatile det_ret_type_te inputs_wheel_click_return;

static void pseudo_irq(void)
{
    irq_mutex.lock();
    timer_ms_tick();

    /* Smartcard detect */
//            smartcard_lowlevel_detect();

    /* Scan buttons */
    inputs_scan();
    
    /* Power logic */
//            logic_power_ms_tick();

    irq_mutex.unlock();
}

class OLEDWidget: public QWidget {
public:
    QImage display;
    OLEDWidget(): display(256, 64, QImage::Format_RGB888) {
        setMinimumSize(display.size());
        setMaximumSize(display.size());
    }
    ~OLEDWidget() {
	// deliver the update events before they cause us trouble
	// in the QWidget destructor
	QApplication::removePostedEvents(this);
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

    virtual void wheelEvent(QWheelEvent *event) {
        int delta = event->angleDelta().y()/120;
        irq_mutex.lock();
        inputs_wheel_cur_increment += delta;
        irq_mutex.unlock();
    }

    virtual void mousePressEvent(QMouseEvent *event) {
        irq_mutex.lock();
        if (inputs_wheel_click_return != RETURN_JRELEASED)
            inputs_wheel_click_return = RETURN_JDETECT;
        irq_mutex.unlock();
    }

    virtual void mouseReleaseEvent(QMouseEvent *event) {
        irq_mutex.lock();
        if (inputs_wheel_click_return == RETURN_DET)
            inputs_wheel_click_return = RETURN_JRELEASED;
        irq_mutex.unlock();
    }
};

extern "C" void minible_main();

class AppThread: public QThread {
private:
    QMutex appexit_mutex;
    bool app_exiting = false;
    QSemaphore app_thread_blocked;

    QLocalSocket *aux;

    bool reconnect_aux() {
        if(aux->state() != QLocalSocket::ConnectedState) {
            aux->connectToServer("moolticuted_local_dev");
            aux->waitForConnected(10);
        }
        
        return aux->state() == QLocalSocket::ConnectedState;
    }

public:
    void run() {
        aux = new QLocalSocket;
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

    void send_aux(char *data, int size) {
        if(!reconnect_aux())
            return;

        while(size > 0) {
            int nb = aux->write(data, size);
            if(nb <= 0)
                break;

            data += nb;
            size -= nb;

            if(size > 0)
                aux->waitForBytesWritten();
        }
    }

    int rcv_aux(char *data, int size) {
        test_stop();
        if(!reconnect_aux())
            return 0;

        aux->waitForReadyRead(0);
        int nb = aux->read(data, size);
        return nb > 0 ? nb : 0;
    }
};

AppThread app_thread;

void emu_appexit_test(void) {
    app_thread.test_stop();
}

static OLEDWidget *oled;

// we need to limit the number of "framebuffer update" events between the threads
// otherwise the process quickly exhausts all RAM
QSemaphore display_queue(3);

void emu_update_display(uint8_t *fb)
{
    QByteArray fb_copy(reinterpret_cast<const char*>(fb), 256*64);
    while(!display_queue.tryAcquire(1, 100)) 
        emu_appexit_test();

    postToObject([=]() {
        oled->update_display(reinterpret_cast<const uint8_t*>(fb_copy.constData())); 
        display_queue.release();
    }, oled);
}

void emu_send_aux(char *data, int size)
{
    app_thread.send_aux(data, size);
}

int emu_rcv_aux(char *data, int size)
{
    return app_thread.rcv_aux(data, size);
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
    oled->show();

    app_thread.start();

    app.exec();

    app_thread.stop();

    delete oled;
    return 0;
}
