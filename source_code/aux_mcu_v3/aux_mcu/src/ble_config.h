/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file or main.c
 * to avoid loosing it when reconfiguring.
 */
#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define CONSOLE_UART TARGET_IO

#define DEVICE_INFORMATION_SERVICE

#define HID_SERVICE

#define HID_DEVICE
#define HID_GATT_SERVER
#define BLE_DEVICE_ROLE BLE_ROLE_PERIPHERAL
#define NENABLE_PTS
#define ATT_DB_MEMORY

#define ENABLE_POWER_SAVE
#define NEW_EVT_HANDLER
#define ATT_DB_MEMORY

#define UART_FLOWCONTROL_6WIRE_MODE true
#define UART_FLOWCONTROL_4WIRE_MODE false

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // BLE_CONFIG_H
