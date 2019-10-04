#include <asf.h>
#include "emu_oled.h"
#include "emulator.h"
#include "platform_defines.h"
#include "sh1122.h"

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

void emu_oled_flush(void)
{
    emu_update_display(oled_fb);
}
