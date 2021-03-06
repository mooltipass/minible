/*!  \file     defines.h
*    \brief    Generic type defines
*    Created:  22/08/2018
*    Author:   Mathieu Stephan
*/
#ifndef DEFINES_H_
#define DEFINES_H_

#include <stdio.h>

/* Macros */
#define XSTR(x)                             STR(x)
#define STR(x)                              #x
#define ARRAY_SIZE(x)                       (sizeof((x)) / sizeof((x)[0]))
#define MEMBER_SIZE(type, member)           sizeof(((type*)0)->member)
#define MEMBER_ARRAY_SIZE(type, member)     (sizeof(((type*)0)->member) / sizeof(((type*)0)->member[0]))
#define MEMBER_SUB_ARRAY_SIZE(type, member) (sizeof(((type*)0)->member[0]) / sizeof(((type*)0)->member[0][0]))

/* Standard defines */
#define FALSE                   0
#define TRUE                    (!FALSE)
#define BOOTLOADER_FLAG         0xDEADBEEF

/* Debugging defines */
#define DEBUG_STACK_TRACKING_COOKIE 0x5D

/* Enums */
typedef enum    {LF_EN_MASK = 0x01, LF_ENT_KEY_MASK = 0x02, LF_LOGIN_MASK = 0x04, LF_WIN_L_SEND_MASK = 0x08, LF_CTRL_ALT_DEL_MASK = 0x10} lock_feature_te;
typedef enum    {USB_INTERFACE = 0, BLE_INTERFACE = 1, CTAP_INTERFACE = 2, NB_HID_INTERFACES} hid_interface_te;
typedef enum    {COMMS_USB_NO_RET = 0, COMMS_USB_TIMEOUT} comms_usb_ret_te;
typedef enum    {RETURN_NOK = -1, RETURN_OK = 0} ret_type_te;

/* Typedefs */
typedef void (*void_function_ptr_type_t)(void);
typedef uint16_t cust_char_t;
typedef ret_type_te RET_TYPE;
typedef uint32_t nat_type_t;
typedef int32_t BOOL;

#endif /* DEFINES_H_ */
