#ifndef _EMU_OLED_H
#define _EMU_OLED_H
#include <inttypes.h>

#ifdef __cplusplus

#include <QWidget>
#include <QImage>

class OLEDWidget: public QWidget {
public:
    QImage display;
    OLEDWidget();
    ~OLEDWidget();

    void update_display(const uint8_t *fb);

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void wheelEvent(QWheelEvent *evt);
    virtual void mousePressEvent(QMouseEvent *evt);
    virtual void mouseReleaseEvent(QMouseEvent *evt);
    virtual void keyPressEvent(QKeyEvent *evt);
    virtual void keyReleaseEvent(QKeyEvent *evt);
};

extern "C" {
#endif

void emu_oled_byte(uint8_t data);
void emu_oled_flush(void);

#ifdef __cplusplus
}
#endif


#endif
