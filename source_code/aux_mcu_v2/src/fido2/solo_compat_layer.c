#include "driver_timer.h"
#include "solo_compat_layer.h"
#include "comms_raw_hid.h"
#include "platform_defines.h"

uint32_t millis(void)
{
    return timer_get_systick();
}

void usbhid_send(uint8_t * msg)
{
    hid_packet_t* send_buf_ptr = comms_raw_hid_get_send_buffer(CTAP_INTERFACE);

    memcpy(send_buf_ptr, msg, USB_RAWHID_RX_SIZE);

    //comms_usb_debug_printf("Output buffer3:\n");
    //comms_usb_debug_printf("0x%02x 0x%02x 0x%02x 0x%02x\n", msg[0], msg[1], msg[2], msg[3]);
    //comms_usb_debug_printf("0x%02x 0x%02x 0x%02x 0x%02x\n", msg[4], msg[5], msg[6], msg[7]);
    //comms_usb_debug_printf("0x%02x 0x%02x 0x%02x 0x%02x\n", msg[8], msg[9], msg[10], msg[11]);
    //comms_usb_debug_printf("0x%02x 0x%02x 0x%02x 0x%02x\n", msg[12], msg[13], msg[14], msg[15]);

    comms_raw_hid_send_packet(CTAP_INTERFACE, send_buf_ptr, TRUE, USB_RAWHID_RX_SIZE);
}

void ctaphid_write_block(uint8_t * data)
{
    usbhid_send(data);
}

void device_wink()
{
    //TODO: 0x0ptr
    //main_mcu message to flash something on display?
}

#if defined DEBUG_LOG_DISABLED

void dump_hex(uint8_t * buf, uint32_t size)
{
}

#else

void dump_hex(uint8_t * buf, uint32_t size)
{
    uint32_t i;
    char tmp[50];
    uint32_t offset = 0;
    uint32_t byteno = 0;

    platform_io_uart_debug_printf("DATA [len: %d]", size);

    for (i = 0; i < size; ++i) {
        if ((i > 0) && (i % 16) == 0) {
            tmp[offset] = '\0';
            platform_io_uart_debug_printf("[%03X]: %s", byteno, tmp);
            offset = 0;
            byteno += 16;
        }
        offset += sprintf(&tmp[offset], "%02X ", buf[i]);
    }
    tmp[offset] = '\0';
    platform_io_uart_debug_printf("[%03X]: %s", byteno, tmp);
}

#endif

int timestamp(void)
{
    return millis();
}

