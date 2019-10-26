#include "emu_oled.h"
#include "emulator.h"
extern "C" {
#include <asf.h>
#include "platform_defines.h"
#include "sh1122.h"
}

#include "qt_metacall_helper.h"
#include <QMutex>
#include <QSemaphore>
#include <QApplication>
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>

#define FB_WIDTH (256)
#define FB_HEIGHT (64)

/// grayscale 8-bit
static uint8_t oled_fb[FB_WIDTH * FB_HEIGHT];
static int oled_col, oled_row;

void emu_oled_byte(uint8_t data)
{
    static int cmdargs = 0;
    static uint8_t last_cmd = 0;

    if(PORT->Group[OLED_CD_GROUP].OUTCLR.reg == OLED_CD_MASK) {
        // command byte
        //printf("Oled CMD: %02x\n", data);
        if(cmdargs == 0) {
            switch(data) {
            case SH1122_CMD_SET_HIGH_COLUMN_ADDR ... SH1122_CMD_SET_HIGH_COLUMN_ADDR+15:
                oled_col = (oled_col & 0xf) | ((data & 0xf) << 4);
                break;
            case SH1122_CMD_SET_LOW_COLUMN_ADDR ... SH1122_CMD_SET_LOW_COLUMN_ADDR+15:
                oled_col = (oled_col & 0xf0) | (data & 0xf);
                break;
            case SH1122_CMD_SET_ROW_ADDR:
                cmdargs = 1;
                break;
            case SH1122_CMD_SET_CLOCK_DIVIDER:
            case SS1122_CMD_SET_DISCHARGE_PRECHARGE_PERIOD:
            case SH1122_CMD_SET_CONTRAST_CURRENT:
            case SH1122_CMD_SET_DISPLAY_OFFSET:
            case SH1122_CMD_SET_VCOM_DESELECT_LEVEL:
            case SH1122_CMD_SET_VSEGM_LEVEL:
                cmdargs = 1;
                break;
            }

            last_cmd = data;

        } else {
            if(cmdargs == 1 && last_cmd == SH1122_CMD_SET_ROW_ADDR) {
                oled_row = data;
            }

            cmdargs--;
        }
    } else {
        // data byte
        //printf("Oled DATA @%d,%d: %02x\n", oled_col, oled_row, data);
        oled_fb[FB_WIDTH * oled_row + oled_col*2] = data & 0xf0;
        oled_fb[FB_WIDTH * oled_row + oled_col*2 + 1] = (data & 0x0f) << 4;
        
        if(oled_col == SH1122_OLED_Max_Column) {
            if(oled_row == SH1122_OLED_Max_Row) {
                oled_row = 0;

            } else {
                oled_row++;
            }
            oled_col = 0;
        } else {
            oled_col++;
        }
    }
}

// we need to limit the number of "framebuffer update" events between the threads
// otherwise the process quickly exhausts all RAM
QSemaphore display_queue(3);

void emu_oled_flush(void)
{
    QByteArray fb_copy(reinterpret_cast<const char*>(oled_fb), 256*64);
    while(!display_queue.tryAcquire(1, 100)) 
        emu_appexit_test();

    postToObject([=]() {
        oled->update_display(reinterpret_cast<const uint8_t*>(fb_copy.constData())); 
        display_queue.release();
    }, oled);
}

OLEDWidget::OLEDWidget(): display(256, 64, QImage::Format_RGB888) {
    setMinimumSize(display.size());
    setMaximumSize(display.size());
}

OLEDWidget::~OLEDWidget() {
    // deliver the update events before they cause us trouble
    // in the QWidget destructor
    QApplication::removePostedEvents(this);
}

void OLEDWidget::update_display(const uint8_t *fb) {
    uint8_t *optr = display.bits();

    for(int y=0;y<64;y++)
        for(int x=0;x<256;x++) {
            optr[0] = optr[1] = optr[2] = *fb++;
            optr+=3;
        }
   
    repaint();
}

void OLEDWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.drawImage(0, 0, display);
}

// see INPUTS/inputs.c
extern "C" volatile int16_t inputs_wheel_cur_increment;
extern "C" volatile uint16_t inputs_wheel_click_duration_counter;
extern "C" volatile det_ret_type_te inputs_wheel_click_return;


void OLEDWidget::wheelEvent(QWheelEvent *event) {
    int delta = event->angleDelta().y()/120;
    irq_mutex.lock();
    inputs_wheel_cur_increment += delta;
    irq_mutex.unlock();
}

void OLEDWidget::mousePressEvent(QMouseEvent *event) {
    irq_mutex.lock();
    if (inputs_wheel_click_return != RETURN_JRELEASED)
        inputs_wheel_click_return = RETURN_JDETECT;
    irq_mutex.unlock();
}

void OLEDWidget::mouseReleaseEvent(QMouseEvent *event) {
    irq_mutex.lock();
    if (inputs_wheel_click_return == RETURN_DET)
        inputs_wheel_click_return = RETURN_JRELEASED;
    irq_mutex.unlock();
}
