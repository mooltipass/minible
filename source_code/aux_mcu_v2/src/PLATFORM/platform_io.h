/*!  \file     platform_io.h
*    \brief    Platform IO related functions
*    Created:  22/08/2018
*    Author:   Mathieu Stephan
*/

#ifndef PLATFORM_IO_H_
#define PLATFORM_IO_H_

/* Prototypes */
uint32_t platform_io_get_cursense_conversion_result(BOOL trigger_conversion);
BOOL platform_io_is_current_sense_conversion_result_ready(void);
void platform_io_update_step_down_voltage(uint16_t voltage);
void platform_io_enable_battery_charging_ports(void);
void platform_io_prepare_ports_for_sleep_exit(void);
void platform_io_enable_step_down(uint16_t voltage);
void platform_io_prepare_ports_for_sleep(void);
void platform_io_disable_charge_mosfets(void);
void platform_io_enable_charge_mosfets(void);
void platform_io_disable_no_comms_int(void);
void platform_io_enable_no_comms_int(void);
void platform_io_init_no_comms_input(void);
void platform_io_disable_main_comms(void);
void platform_io_enable_main_comms(void);
void platform_io_init_aux_comms(void);
void platform_io_init_usb_ports(void);
void platform_io_enable_eic(void);
void platform_io_init_ports(void);


#endif /* PLATFORM_IO_H_ */