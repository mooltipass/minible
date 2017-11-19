/*!  \file     platform_io.h
*    \brief    Platform IO related functions
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/

#ifndef PLATFORM_IO_H_
#define PLATFORM_IO_H_

/* Prototypes */
void platform_io_init_flash_ports(void);
void platform_io_init_power_ports(void);
void platform_io_init_oled_ports(void);
void platform_io_init_ports(void);

#endif /* PLATFORM_IO_H_ */