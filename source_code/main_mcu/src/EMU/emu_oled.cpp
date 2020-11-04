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
#include <QKeyEvent>
#include <QThread>
#include <QTimer>

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
            case SH1122_CMD_SET_DISPLAY_ON:
                postToObject([]() { oled->set_display_on(true); }, oled);
                break;
            case SH1122_CMD_SET_DISPLAY_OFF:
                postToObject([]() { oled->set_display_on(false); }, oled);
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

static QMutex fb_update;
static uint8_t framebuffers[2][256*64];
static int fb_next=0, fb_pending=-1;

void emu_oled_flush(void)
{
    emu_appexit_test();
    fb_update.lock();
    if(fb_pending >= 0) {
        // an update is queued, just replace the contents
        memcpy(framebuffers[fb_pending], oled_fb, 256*64);

    } else {
        // request an update
        int fb_req = fb_next;
        memcpy(framebuffers[fb_next], oled_fb, 256*64);
        fb_pending = fb_req;
        fb_next = (fb_next+1)%2;

        // use a timer to coalesce multiple repaints 
        QTimer::singleShot(2, oled, [fb_req]() {
            fb_update.lock();
            if(fb_req == fb_pending)
                fb_pending = -1;
            fb_update.unlock();
            oled->update_display(framebuffers[fb_req]);
        });
    }

    fb_update.unlock();
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

void OLEDWidget::set_display_on(bool on) {
    display_on = on;
    repaint();
}

void OLEDWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    if(display_on)
        painter.drawImage(QRect(0, 0, width(), height()), display, QRect(0, 0, display.width(), display.height()));
    else
        painter.eraseRect(QRect(0, 0, width(), height()));
}

// see INPUTS/inputs.c

extern "C" volatile int16_t inputs_wheel_cur_increment;
extern "C" volatile uint16_t inputs_wheel_click_duration_counter;
extern "C" volatile det_ret_type_te inputs_wheel_click_return;
static bool emu_wheel_state;

static void set_emulated_wheel_state(bool state, int16_t nb_ms_press_override)
{
    emu_wheel_state = state;
    if (nb_ms_press_override > 0)
    {
        inputs_wheel_click_duration_counter = nb_ms_press_override;
    }
}

extern "C" void inputs_scan(void);

extern "C" void inputs_scan(void)
{
    if (inputs_wheel_click_return == RETURN_INV_DET)
    {
        if (!emu_wheel_state)
        {
            inputs_wheel_click_return = RETURN_REL;
        }
    }
    else
    {
        if (emu_wheel_state) {
            if(inputs_wheel_click_return == RETURN_REL || inputs_wheel_click_return == WHEEL_ACTION_NONE) {
                inputs_wheel_click_return = RETURN_JDETECT;
            }

        } else {
            if (inputs_wheel_click_return == RETURN_DET) {
                inputs_wheel_click_return = RETURN_JRELEASED;
            }
        }

        if ((inputs_wheel_click_return == RETURN_DET) || (inputs_wheel_click_return == RETURN_JDETECT))
        {
            inputs_wheel_click_duration_counter++;
        } else {
            inputs_wheel_click_duration_counter = 0;
        }
    }
}


void OLEDWidget::wheelEvent(QWheelEvent *evt) {
    int delta = evt->angleDelta().y()/120;
    irq_mutex.lock();
    inputs_wheel_cur_increment -= delta;
    irq_mutex.unlock();
}

void OLEDWidget::mousePressEvent(QMouseEvent *evt) {
    irq_mutex.lock();
    if((evt->button() == Qt::BackButton) || (evt->button() == Qt::RightButton))
    {
        set_emulated_wheel_state(true, 3000);
    }
    else if(evt->button() == Qt::LeftButton)
        set_emulated_wheel_state(true, -1);

    irq_mutex.unlock();
}

void OLEDWidget::mouseReleaseEvent(QMouseEvent *evt) {
    irq_mutex.lock();
    if((evt->button() == Qt::BackButton) || (evt->button() == Qt::RightButton))
        set_emulated_wheel_state(false, -1);
    else if(evt->button() == Qt::LeftButton)
        set_emulated_wheel_state(false, -1);
    irq_mutex.unlock();
}

void OLEDWidget::keyPressEvent(QKeyEvent *evt) {
    irq_mutex.lock();
    switch(evt->key()) {
    case Qt::Key_Up:
        inputs_wheel_cur_increment--;
        break;
    case Qt::Key_Down:
        inputs_wheel_cur_increment++;
        break;
    case Qt::Key_Right:
    case Qt::Key_Space:
    case Qt::Key_Enter:
    case Qt::Key_Return:
        set_emulated_wheel_state(true, -1);
        break;
    case Qt::Key_Left:
    case Qt::Key_Backspace:
        set_emulated_wheel_state(true, 3000);
        break;
    }
    irq_mutex.unlock();
}

void OLEDWidget::keyReleaseEvent(QKeyEvent *evt) {
    irq_mutex.lock();
    switch(evt->key()) {
    case Qt::Key_Right:
    case Qt::Key_Space:
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Left:
    case Qt::Key_Backspace:
        set_emulated_wheel_state(false, -1);
        break;
    }
    irq_mutex.unlock();

}
