# CMakelists.txt from tanche
# 
cmake_minimum_required(VERSION 4.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
project(aux_mcu LANGUAGES C ASM)
set(TARGET aux_mcu.elf)


set(INC_DIR
  src
  src/ASF/thirdparty/wireless/ble_sdk/ble_profiles/hid_device
  src/ASF/thirdparty/wireless/ble_sdk/ble_services/hid
  src/ASF/thirdparty/wireless/ble_sdk/ble_services/ble_mgr
  src/ASF/thirdparty/wireless/ble_sdk/ble_services/battery
  src/ASF/thirdparty/wireless/ble_sdk/inc
  src/ASF/thirdparty/wireless/ble_sdk/services/serial/uart
  src/ASF/sam0/drivers/sercom/usart
  src/ASF/sam0/utils
  src/ASF/sam0/utils/header_files
  src/ASF/sam0/utils/preprocessor
  src/ASF/thirdparty/CMSIS/Include
  src/ASF/thirdparty/CMSIS/Lib/GCC
  src/ASF/common/utils
  src/ASF/sam0/utils/cmsis/samd21/include
  src/ASF/sam0/utils/cmsis/samd21/source
  src/ASF/sam0/drivers/sercom
  src/ASF/sam0/drivers/system
  src/ASF/sam0/drivers/system/clock/clock_samd21_r21_da_ha1
  src/ASF/sam0/drivers/system/clock
  src/ASF/sam0/drivers/system/interrupt
  src/ASF/sam0/drivers/system/interrupt/system_interrupt_samd21
  src/ASF/sam0/drivers/system/pinmux
  src/ASF/sam0/drivers/system/power
  src/ASF/sam0/drivers/system/power/power_sam_d_r_h
  src/ASF/sam0/drivers/system/reset
  src/ASF/sam0/drivers/system/reset/reset_sam_d_r_h
  src/ASF/common2/services/delay
  src/ASF/common2/services/delay/sam0
  src/ASF/sam0/drivers/extint
  src/ASF/thirdparty/wireless/ble_sdk/services/serial_fifo
  src/ASF/thirdparty/wireless/ble_sdk/utils
  src/ASF/thirdparty/wireless/ble_sdk/services/console
  src/ASF/sam0/utils/stdio/stdio_serial
  src/ASF/common/services/serial
  src/ASF/thirdparty/wireless/ble_sdk/services/timer
  src/ASF/sam0/drivers/tc
  src/ASF/sam0/drivers/port
  src/ASF/thirdparty/wireless/ble_sdk/ble_services/device_infomation
  src/ASF/sam0/boards/samd21_xplained_pro
  src/ASF/sam0/boards
  src/ASF/common/boards
  src/config
  src/CLOCKS
  src/COMMS
  src/DMA
  src/LOGIC
  src/PLATFORM
  src/SECURITY
  src/TIMER
  src/USB
  src/fido2
  src/tinycbor/src
  # LIBS
  src/ASF/thirdparty/wireless/ble_sdk/lib/cm0p/gcc
  src/ASF/thirdparty/CMSIS/Lib/GCC
  # src/tinycbor/lib
  # src/tinycbor/src
)


set(SRC
  src/ASF/thirdparty/wireless/ble_sdk/ble_services/battery/battery.c
  src/CLOCKS/driver_clocks.c
  src/COMMS/comms_hid_msgs.c
  src/COMMS/comms_main_mcu.c
  src/COMMS/comms_raw_hid.c
  src/debug.c
  src/DMA/dma.c
  src/LOGIC/logic.c
  src/LOGIC/logic_battery.c
  src/LOGIC/logic_bluetooth.c
  src/LOGIC/logic_keyboard.c
  src/LOGIC/logic_sleep.c
  src/LOGIC/logic_rng.c
  src/main.c
  src/PLATFORM/platform_io.c
  src/SECURITY/fuses.c
  src/TIMER/driver_timer.c
  src/USB/udc.c
  src/USB/usb.c
  src/USB/usb_descriptors.c
  src/fido2/ctaphid.c
  src/fido2/ctap.c
  src/fido2/solo_compat_layer.c
  src/fido2/ctap_parse.c
  src/ASF/common2/services/delay/sam0/systick_counter.c
  src/ASF/common/utils/interrupt/interrupt_sam_nvic.c
  src/ASF/sam0/boards/samd21_xplained_pro/board_init.c
  src/ASF/sam0/drivers/extint/extint_callback.c
  src/ASF/sam0/drivers/extint/extint_sam_d_r_h/extint.c
  src/ASF/sam0/drivers/port/port.c
  src/ASF/sam0/drivers/sercom/sercom.c
  src/ASF/sam0/drivers/sercom/sercom_interrupt.c
  src/ASF/sam0/drivers/sercom/usart/usart.c
  src/ASF/sam0/drivers/sercom/usart/usart_interrupt.c
  src/ASF/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/clock.c
  src/ASF/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/gclk.c
  src/ASF/sam0/drivers/system/interrupt/system_interrupt.c
  src/ASF/sam0/drivers/system/pinmux/pinmux.c
  src/ASF/sam0/drivers/system/system.c
  src/ASF/sam0/drivers/tc/tc_interrupt.c
  src/ASF/sam0/drivers/tc/tc_sam_d_r_h/tc.c
  src/ASF/sam0/utils/cmsis/samd21/source/gcc/startup_samd21.c
  src/ASF/sam0/utils/cmsis/samd21/source/system_samd21.c
  src/ASF/sam0/utils/stdio/read.c
  src/ASF/sam0/utils/stdio/write.c
  src/ASF/sam0/utils/syscalls/gcc/syscalls.c
  src/ASF/thirdparty/wireless/ble_sdk/ble_services/ble_mgr/ble_manager.c
  src/ASF/thirdparty/wireless/ble_sdk/ble_services/device_infomation/device_info.c
  src/ASF/thirdparty/wireless/ble_sdk/services/console/sam0/console_serial.c
  src/ASF/thirdparty/wireless/ble_sdk/services/serial/uart/sam0/serial_drv.c
  src/ASF/thirdparty/wireless/ble_sdk/services/serial_fifo/serial_fifo.c
  src/ASF/thirdparty/wireless/ble_sdk/services/timer/sam0/timer_hw.c
  src/ASF/thirdparty/wireless/ble_sdk/src/platform.c
)

set(WITH_FREESTANDING ON)
set(BUILD_SHARED_LIBS OFF)

add_executable(${TARGET} ${SRC})
set_target_properties(${TARGET} PROPERTIES C_STANDARD 99)
target_include_directories(${TARGET} PRIVATE ${INC_DIR})

set(CBOR_SRC
  src/tinycbor/src/cborerrorstrings.c
  src/tinycbor/src/cborencoder.c
  src/tinycbor/src/cborencoder_close_container_checked.c
  src/tinycbor/src/cborencoder_float.c
  src/tinycbor/src/cborparser.c
  src/tinycbor/src/cborparser_float.c
  src/tinycbor/src/cborpretty.c
)
add_library(tinycbor STATIC ${CBOR_SRC})
target_include_directories(tinycbor PUBLIC src/tinycbor/src)

target_link_libraries(${TARGET} PRIVATE
  ${CMAKE_SOURCE_DIR}/src/ASF/thirdparty/wireless/ble_sdk/lib/cm0p/gcc/libble_sdk.a
  tinycbor
)
target_link_options(${TARGET} PRIVATE
  -T ${CMAKE_SOURCE_DIR}/src/ASF/sam0/utils/linker_scripts/samd21/gcc/samd21j18a_flash.ld
  -Wl,-section-start=.start_app_function_addr=0x200
  -Wl,-section-start=.flash_start_addr=0x0
  -Wl,-section-start=.text=0x800
  -Wl,-section-start=.data=0x20000004
  -Wl,-section-start=.relocate=0x20000004
  -Wl,-section-start=.bootloader_flag=0x20000000
)
target_compile_definitions(${TARGET} PUBLIC
  PLAT_V7_SETUP
  $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG DEBUG_LOG_DISABLED>
  __SAMD21E18A__
  ENABLE_POWER_SAVEXX
  SLEEP_WALKING_ENABLED=false
  HOST_SLEEP_ENABLE=true
  SYSTICK_MODE
  NENABLE_PTS
  TC_ASYNC=true
  HID_DEVICE
  UART_FLOWCONTROL_4WIRE_MODE=false
  NEW_EVT_HANDLER
  BOARD=USER_BOARD
  ATT_DB_MEMORY
  HOST_UART_BAUDRATE_CONFIG_VALUE=921600
  UART_FLOWCONTROL_6WIRE_MODE=true
  BLE_DEVICE_ROLE=BLE_ROLE_PERIPHERAL
  HID_KEYBOARD_DEVICE
  ARM_MATH_CM0PLUS=true
  DEVICE_INFORMATION_SERVICE
  BLE_MODULE=BTLC1000_MR
  HID_SERVICE
  EXTINT_CALLBACK_MODE=true
  USART_CALLBACK_MODE=true
  HID_GATT_SERVER
  BATTERY_SERVICE
)

