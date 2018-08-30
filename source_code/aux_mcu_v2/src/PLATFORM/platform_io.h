/*!  \file     platform_io.h
*    \brief    Platform IO related functions
*    Created:  22/08/2018
*    Author:   Mathieu Stephan
*/

#ifndef PLATFORM_IO_H_
#define PLATFORM_IO_H_

/* Prototypes */
void platform_io_prepare_ports_for_sleep_exit(void);
void platform_io_prepare_ports_for_sleep(void);
void platform_io_disable_main_comms(void);
void platform_io_enable_main_comms(void);
void platform_io_init_aux_comms(void);
void platform_io_init_usb_ports(void);
void platform_io_init_ports(void);


#endif /* PLATFORM_IO_H_ */