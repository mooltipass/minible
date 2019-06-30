/*!  \file     defines.h
*    \brief    Generic type defines
*    Created:  22/08/2018
*    Author:   Mathieu Stephan
*/
#ifndef DEFINES_H_
#define DEFINES_H_

#include <stdio.h>

/* Macros */
#define XSTR(x) STR(x)
#define STR(x) #x

/* Standard defines */
#define FALSE                   0
#define TRUE                    (!FALSE)
#define BOOTLOADER_FLAG         0xDEADBEEF

/* Enums */
typedef enum    {USB_INTERFACE = 0, BLE_INTERFACE = 1, NB_HID_INTERFACES} hid_interface_te;
typedef enum    {RETURN_NOK = -1, RETURN_OK = 0} ret_type_te;

/* Typedefs */
typedef void (*void_function_ptr_type_t)(void);
typedef uint16_t cust_char_t;
typedef ret_type_te RET_TYPE;
typedef uint32_t nat_type_t;
typedef int32_t BOOL;

#endif /* DEFINES_H_ */