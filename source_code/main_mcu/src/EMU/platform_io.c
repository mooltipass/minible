#include "platform_io.h"
#include "emulator.h"
#include "emu_smartcard.h"
#include "platform_defines.h"
#include <stdlib.h>

static int vbat_lerp(int bat, int lo, int hi, int vbatlo, int vbathi) 
{
    return vbatlo + (vbathi - vbatlo) * (bat - lo) / (hi - lo);
}

uint16_t platform_io_get_voledin_conversion_result_and_trigger_conversion(void){
    int bat = emu_get_battery_level();
    if(bat <= 0) 
        return BATTERY_ADC_OUT_CUTOUT-1;
    else if(bat <= 20)
        return vbat_lerp(bat, 0, 20, BATTERY_ADC_OUT_CUTOUT-1, BATTERY_ADC_20PCT_VOLTAGE);
    else if(bat <= 40)
        return vbat_lerp(bat, 20, 40, BATTERY_ADC_20PCT_VOLTAGE, BATTERY_ADC_40PCT_VOLTAGE);
    else if(bat <= 60)
        return vbat_lerp(bat, 40, 60, BATTERY_ADC_40PCT_VOLTAGE, BATTERY_ADC_60PCT_VOLTAGE);
    else if(bat <= 80)
        return vbat_lerp(bat, 60, 80, BATTERY_ADC_60PCT_VOLTAGE, BATTERY_ADC_80PCT_VOLTAGE);
    else if(bat <= 100)
        return vbat_lerp(bat, 80, 100, BATTERY_ADC_80PCT_VOLTAGE, BATTERY_ADC_OVER_VOLTAGE-1);

    return BATTERY_ADC_OVER_VOLTAGE-1;
}

uint16_t platform_io_get_voledin_conversion_result(void)
{
    int bat = emu_get_battery_level();
    if(bat <= 0) 
        return BATTERY_ADC_OUT_CUTOUT-1;
    else if(bat <= 20)
        return (BATTERY_ADC_OUT_CUTOUT+BATTERY_ADC_20PCT_VOLTAGE)/2;
    else if(bat <= 40)
        return (BATTERY_ADC_20PCT_VOLTAGE+BATTERY_ADC_40PCT_VOLTAGE)/2;
    else if(bat <= 60)
        return (BATTERY_ADC_40PCT_VOLTAGE+BATTERY_ADC_60PCT_VOLTAGE)/2;
    else if(bat <= 80)
        return (BATTERY_ADC_60PCT_VOLTAGE+BATTERY_ADC_80PCT_VOLTAGE)/2;
    else if(bat <= 100)
        return (BATTERY_ADC_80PCT_VOLTAGE+BATTERY_ADC_OVER_VOLTAGE)/2;

    return BATTERY_ADC_OVER_VOLTAGE-1;
}

static oled_stepup_pwr_source_te platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_NONE;
oled_stepup_pwr_source_te platform_io_get_voled_stepup_pwr_source(void){return platform_io_oled_stepup_power_source; }
BOOL platform_io_is_voledin_conversion_result_ready(void){ return TRUE; }
void platform_io_set_no_comms_as_wakeup_interrupt(void){}
void platform_io_disable_vbat_to_oled_stepup(void){
    platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_NONE;
}
void platform_io_enable_vbat_to_oled_stepup(void){
    platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_VBAT;
}
void platform_io_disable_3v3_to_oled_stepup(void){
    platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_3V3;
}
void platform_io_init_bat_adc_measurements(void){}
void platform_io_set_wheel_click_pull_down(void){}
void platform_io_power_up_oled(BOOL power_3v3){
    if(power_3v3)
        platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_3V3;
    else
        platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_VBAT;
}
void platform_io_disable_switch_and_die(void){ abort(); }
void platform_io_init_no_comms_signal(void){}
void platform_io_smc_remove_function(void){ emu_reset_smartcard(); }
void platform_io_set_wheel_click_low(void){}
BOOL platform_io_is_usb_3v3_present(void){ return emu_get_usb_charging(); }
BOOL platform_io_is_usb_3v3_present_raw(void){ return emu_get_usb_charging(); }
void platform_io_release_aux_reset(void){}
void platform_io_bypass_3v3_detection_debounce(void){}
void platform_io_assert_oled_reset(void){}
void platform_io_set_voled_vin_as_pulldown(void){}
void platform_io_set_voled_vin_as_adc_input(void){}
void platform_io_init_flash_ports(void){}
void platform_io_init_power_ports(void){}
void platform_io_power_down_oled(void){
    platform_io_oled_stepup_power_source = OLED_STEPUP_SOURCE_NONE;
}
void platform_io_clear_no_comms(void){}
void platform_io_enable_switch(void){}
void platform_io_set_no_comms(void){}
void platform_io_disable_ble(void){}
void platform_io_init_ports(void){}
void platform_io_enable_ble(void){}

/*
uint16_t platform_io_get_voledinmv_conversion_result_and_trigger_conversion(void){ return 0;}
void platform_io_disable_scroll_wheel_wakeup_interrupts(void){}
void platform_io_enable_scroll_wheel_wakeup_interrupts(void){}
void platform_io_disable_no_comms_as_wakeup_interrupt(void){}
void platform_io_prepare_acc_ports_for_sleep_exit(void){}
void platform_io_disable_aux_tx_wakeup_interrupt(void){}
void platform_io_enable_usb_3v3_wakeup_interrupt(void){}
void platform_io_bypass_3v3_detection_debounce(void){}
void platform_io_enable_aux_tx_wakeup_interrupt(void){}
void platform_io_prepare_ports_for_sleep_exit(void){}
void platform_io_disable_scroll_wheel_ports(void){}
void platform_io_enable_3v3_to_oled_stepup(void){}
void platform_io_init_accelerometer_ports(void){}
void platform_io_prepare_ports_for_sleep(void){}
void platform_io_init_scroll_wheel_ports(void){}
void platform_io_smc_inserted_function(void){}
void platform_io_smc_switch_to_spi(void){}
void platform_io_disable_aux_comms(void){}
void platform_io_enable_aux_comms(void){}
void platform_io_smc_switch_to_bb(void){}
void platform_io_init_oled_ports(void){}
void platform_io_init_smc_ports(void){}
void platform_io_init_aux_comms(void){}
*/
