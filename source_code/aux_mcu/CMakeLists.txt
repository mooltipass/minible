# CMakelists.txt from tanche
# 
cmake_minimum_required(VERSION 4.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
project(aux_mcu LANGUAGES C ASM)
set(TARGET aux_mcu.elf)

include(cmake/tinycbor.cmake)
add_subdirectory(src/ports/minibleV1)

set(INC_DIR
  src
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
  # LIBS
)


set(SRC
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
)

add_executable(${TARGET} ${SRC})
set_target_properties(${TARGET} PROPERTIES C_STANDARD 99)
target_include_directories(${TARGET} PRIVATE ${INC_DIR})

target_link_libraries(${TARGET} PRIVATE
  minibleV1
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

