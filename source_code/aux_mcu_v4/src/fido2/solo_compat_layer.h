#include "ctap.h"
#include "stdarg.h"
#include "stdint.h"
#include "ctap.h"

uint32_t millis(void);
void usbhid_send(uint8_t * msg);
void ctaphid_write_block(uint8_t * data);
void device_wink(void);

void device_set_status(uint32_t status);
int timestamp(void);

void dump_hex(uint8_t * buf, int size);
#define dump_hex1(tag,data,len) dump_hex(data, len);

#if !defined DEBUG_LOG_DISABLED
void comms_usb_debug_printf(const char *fmt, ...);
#define printf1(tag,fmt, ...) comms_usb_debug_printf(fmt, ##__VA_ARGS__)
#define printf2(tag,fmt, ...) comms_usb_debug_printf(fmt, ##__VA_ARGS__)
#define printf3(tag,fmt, ...) comms_usb_debug_printf(fmt, ##__VA_ARGS__)
#else
#define printf1(tag,fmt, ...)
#define printf2(tag,fmt, ...)
#define printf3(tag,fmt, ...)
#endif

#define TAG_HID 0
#define TAG_ERR 0
#define TAG_TIME 0

