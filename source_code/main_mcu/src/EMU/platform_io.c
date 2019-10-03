#include "platform_io.h"
#include <stdlib.h>

uint16_t platform_io_get_voledinmv_conversion_result_and_trigger_conversion(void){ return 0;}
uint16_t platform_io_get_voledin_conversion_result_and_trigger_conversion(void){ return 0;}
oled_stepup_pwr_source_te platform_io_get_voled_stepup_pwr_source(void){return OLED_STEPUP_SOURCE_NONE; }
void platform_io_disable_scroll_wheel_wakeup_interrupts(void){}
void platform_io_enable_scroll_wheel_wakeup_interrupts(void){}
void platform_io_disable_no_comms_as_wakeup_interrupt(void){}
BOOL platform_io_is_voledin_conversion_result_ready(void){ return TRUE; }
void platform_io_prepare_acc_ports_for_sleep_exit(void){}
void platform_io_set_no_comms_as_wakeup_interrupt(void){}
void platform_io_disable_aux_tx_wakeup_interrupt(void){}
void platform_io_enable_usb_3v3_wakeup_interrupt(void){}
void platform_io_enable_aux_tx_wakeup_interrupt(void){}
void platform_io_prepare_ports_for_sleep_exit(void){}
void platform_io_disable_vbat_to_oled_stepup(void){}
void platform_io_disable_scroll_wheel_ports(void){}
void platform_io_enable_vbat_to_oled_stepup(void){}
void platform_io_disable_3v3_to_oled_stepup(void){}
void platform_io_enable_3v3_to_oled_stepup(void){}
void platform_io_init_bat_adc_measurements(void){}
void platform_io_set_wheel_click_pull_down(void){}
void platform_io_init_accelerometer_ports(void){}
void platform_io_prepare_ports_for_sleep(void){}
void platform_io_init_scroll_wheel_ports(void){}
void platform_io_power_up_oled(BOOL power_3v3){}
void platform_io_disable_switch_and_die(void){ abort(); }
void platform_io_smc_inserted_function(void){}
void platform_io_init_no_comms_signal(void){}
void platform_io_smc_remove_function(void){}
void platform_io_set_wheel_click_low(void){}
BOOL platform_io_is_usb_3v3_present(void){ return TRUE; }
void platform_io_release_aux_reset(void){}
void platform_io_smc_switch_to_spi(void){}
void platform_io_assert_oled_reset(void){}
void platform_io_disable_aux_comms(void){}
void platform_io_enable_aux_comms(void){}
void platform_io_smc_switch_to_bb(void){}
void platform_io_init_flash_ports(void){}
void platform_io_init_power_ports(void){}
void platform_io_init_oled_ports(void){}
void platform_io_power_down_oled(void){}
void platform_io_init_smc_ports(void){}
void platform_io_init_aux_comms(void){}
void platform_io_clear_no_comms(void){}
void platform_io_enable_switch(void){}
void platform_io_set_no_comms(void){}
void platform_io_disable_ble(void){}
void platform_io_init_ports(void){}
void platform_io_enable_ble(void){}
