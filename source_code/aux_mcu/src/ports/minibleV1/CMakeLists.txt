# CMakelists.txt from tanche
# 
# Manage SAMD files

set(V1_SRC
  ASF/thirdparty/wireless/ble_sdk/ble_services/battery/battery.c
  ASF/common2/services/delay/sam0/systick_counter.c
  ASF/common/utils/interrupt/interrupt_sam_nvic.c
  ASF/sam0/boards/samd21_xplained_pro/board_init.c
  ASF/sam0/drivers/extint/extint_callback.c
  ASF/sam0/drivers/extint/extint_sam_d_r_h/extint.c
  ASF/sam0/drivers/port/port.c
  ASF/sam0/drivers/sercom/sercom.c
  ASF/sam0/drivers/sercom/sercom_interrupt.c
  ASF/sam0/drivers/sercom/usart/usart.c
  ASF/sam0/drivers/sercom/usart/usart_interrupt.c
  ASF/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/clock.c
  ASF/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/gclk.c
  ASF/sam0/drivers/system/interrupt/system_interrupt.c
  ASF/sam0/drivers/system/pinmux/pinmux.c
  ASF/sam0/drivers/system/system.c
  ASF/sam0/drivers/tc/tc_interrupt.c
  ASF/sam0/drivers/tc/tc_sam_d_r_h/tc.c
  ASF/sam0/utils/cmsis/samd21/source/gcc/startup_samd21.c
  ASF/sam0/utils/cmsis/samd21/source/system_samd21.c
  ASF/sam0/utils/stdio/read.c
  ASF/sam0/utils/stdio/write.c
  ASF/sam0/utils/syscalls/gcc/syscalls.c
  ASF/thirdparty/wireless/ble_sdk/ble_services/ble_mgr/ble_manager.c
  ASF/thirdparty/wireless/ble_sdk/ble_services/device_infomation/device_info.c
  ASF/thirdparty/wireless/ble_sdk/services/console/sam0/console_serial.c
  ASF/thirdparty/wireless/ble_sdk/services/serial/uart/sam0/serial_drv.c
  ASF/thirdparty/wireless/ble_sdk/services/serial_fifo/serial_fifo.c
  ASF/thirdparty/wireless/ble_sdk/services/timer/sam0/timer_hw.c
  ASF/thirdparty/wireless/ble_sdk/src/platform.c
)

set(V1_INC
  ASF/thirdparty/wireless/ble_sdk/ble_profiles/hid_device
  ASF/thirdparty/wireless/ble_sdk/ble_services/hid
  ASF/thirdparty/wireless/ble_sdk/ble_services/ble_mgr
  ASF/thirdparty/wireless/ble_sdk/ble_services/battery
  ASF/thirdparty/wireless/ble_sdk/inc
  ASF/thirdparty/wireless/ble_sdk/services/serial/uart
  ASF/sam0/drivers/sercom/usart
  ASF/sam0/utils
  ASF/sam0/utils/header_files
  ASF/sam0/utils/preprocessor
  ASF/thirdparty/CMSIS/Include
  ASF/thirdparty/CMSIS/Lib/GCC
  ASF/common/utils
  ASF/sam0/utils/cmsis/samd21/include
  ASF/sam0/utils/cmsis/samd21/source
  ASF/sam0/drivers/sercom
  ASF/sam0/drivers/system
  ASF/sam0/drivers/system/clock/clock_samd21_r21_da_ha1
  ASF/sam0/drivers/system/clock
  ASF/sam0/drivers/system/interrupt
  ASF/sam0/drivers/system/interrupt/system_interrupt_samd21
  ASF/sam0/drivers/system/pinmux
  ASF/sam0/drivers/system/power
  ASF/sam0/drivers/system/power/power_sam_d_r_h
  ASF/sam0/drivers/system/reset
  ASF/sam0/drivers/system/reset/reset_sam_d_r_h
  ASF/common2/services/delay
  ASF/common2/services/delay/sam0
  ASF/sam0/drivers/extint
  ASF/thirdparty/wireless/ble_sdk/services/serial_fifo
  ASF/thirdparty/wireless/ble_sdk/utils
  ASF/thirdparty/wireless/ble_sdk/services/console
  ASF/sam0/utils/stdio/stdio_serial
  ASF/common/services/serial
  ASF/thirdparty/wireless/ble_sdk/services/timer
  ASF/sam0/drivers/tc
  ASF/sam0/drivers/port
  ASF/thirdparty/wireless/ble_sdk/ble_services/device_infomation
  ASF/sam0/boards/samd21_xplained_pro
  ASF/sam0/boards
  ASF/common/boards
  ASF/thirdparty/wireless/ble_sdk/lib/cm0p/gcc
  ASF/thirdparty/CMSIS/Lib/GCC
  .
)

add_library(minibleV1 ${V1_SRC})
target_include_directories(minibleV1 PUBLIC ${V1_INC})
target_compile_options(minibleV1 PRIVATE
  -Wno-expansion-to-defined
)
target_compile_definitions(minibleV1 PUBLIC
  __SAMD21E18A__
  BOARD=USER_BOARD
  EXTINT_CALLBACK_MODE=true
  USART_CALLBACK_MODE=true
  TC_ASYNC=true
)
target_link_libraries(minibleV1 PRIVATE
  ASF/thirdparty/wireless/ble_sdk/lib/cm0p/gcc/libble_sdk.a
)
