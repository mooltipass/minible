#
# Makefile for c project of Atmel ATSAMD device
#

# Project specific properties
OUTPUT_FILE=main.elf
OUTPUT_FILE_NAME=$(basename $(OUTPUT_FILE))

## Microcontroller properties
DEVICE   = SAMD21E17A
CPU      = cortex-m0plus
LDSCRIPT = ASF/sam0/utils/linker_scripts/samd21/gcc/samd21e17a_flash.ld
CMSIS_PATH = ASF/thirdparty/CMSIS
ASF_PATH =  ASF/


### Compiler definitions
CC=arm-none-eabi-gcc
CXX=arm-none-eabi-g++
OBJCOPY=arm-none-eabi-objcopy
OBJDUMP=arm-none-eabi-objdump
SIZE=arm-none-eabi-size

## List of Defines
DEFINES  = -D __$(DEVICE)__
#DEFINES += -D UDD_NO_SLEEP_MGR
DEFINES += -D ACCESS_USB_ENABLED
DEFINES += -D TC_ASYNC=true
DEFINES += -D ARM_MATH_CM0PLUS=true
DEFINES += -D EXTINT_CALLBACK_MODE=true
DEFINES += -D UDD_ENABLE
DEFINES += -D USB_DEVIDCE_LPM_SUPPORT
DEFINES += -D BOARD=USER_BOARD
#DEFINES += -DARM_MATH_CM0PLUS=true
BLE_DEFINES += -D BLE_DEVICE_ROLE=BLE_ROLE_PERIPHERAL
BLE_DEFINES+= -D BLE_MODULE=BTLC1000_MR
BLE_DEFINES+= -D DEVICE_INFORMATION_SERVICE
BLE_DEFINES+= -D ENABLE_POWER_SAVE
BLE_DEFINES+= -D EXTINT_CALLBACK_MODE=true
BLE_DEFINES+= -D HID_DEVICE
BLE_DEFINES+= -D HID_GATT_SERVER
BLE_DEFINES+= -D HID_KEYBOARD_DEVICE
BLE_DEFINES+= -D HID_SERVICE
BLE_DEFINES+= -D HOST_SLEEP_ENABLE=true
BLE_DEFINES+= -D HOST_UART_BAUDRATE_CONFIG_VALUE=115200
BLE_DEFINES+= -D NENABLE_PTS
BLE_DEFINES+= -D NEW_EVT_HANDLER
BLE_DEFINES+= -D SLEEP_WALKING_ENABLED=false
BLE_DEFINES+= -D SYSTICK_MODE
BLE_DEFINES+= -D TC_ASYNC=true
BLE_DEFINES+= -D UART_FLOWCONTROL_4WIRE_MODE=false
BLE_DEFINES+= -D UART_FLOWCONTROL_6WIRE_MODE=true
BLE_DEFINES+= -D USART_CALLBACK_MODE=true
#BLE_DEFINES+= -D BTLC_REINIT_SUPPORT
BLE_DEFINES += -D _ASSERT_ENABLE_


# C Compiler FLAGS
CFLAGS = -x c -mthumb $(DEFINES) $(BLE_DEFINES)
CFLAGS += -O0 -fdata-sections -ffunction-sections -mlong-calls -g3 -Wall -mcpu=$(CPU) -c 
CFLAGS += -pipe -fno-strict-aliasing -Wall -Wstrict-prototypes -Wmissing-prototypes 
CFLAGS += -Werror-implicit-function-declaration -Wpointer-arith -std=gnu99 -ffunction-sections 
CFLAGS += -fdata-sections -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int 
CFLAGS += -Wmain -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs 
CFLAGS += -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow 
CFLAGS += -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return 
CFLAGS += -Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations 
CFLAGS += -Wpacked -Wredundant-decls -Wnested-externs -Wlong-long -Wunreachable-code -Wcast-align --param max-inline-insns-single=500

# Linker FLAGS
LIB_PATH+= ASF/thirdparty/CMSIS/Lib/GCC
LIB_PATH+= ASF/thirdparty/wireless/ble_sdk/lib/cm0p/gcc

LDLIBS+= -lm -larm_cortexM0l_math #-lble_sdk

LDFLAGS= -Wl,-Map=$(OUTPUT_FILE_NAME).map,--start-group $(addprefix -L , $(LIB_PATH)) -Wl,--end-group -Wl,--gc-sections -mthumb -mcpu=$(CPU) -T $(LDSCRIPT) 

## Include Dirs
INCLUDE_PATHS+= .
INCLUDE_PATHS+= config
INCLUDE_PATHS+= USBHID
INCLUDE_PATHS+= ASF/common/utils/interrupt
INCLUDE_PATHS+= ASF/thirdparty/CMSIS/Include
INCLUDE_PATHS+= ASF/sam0/utils/cmsis/samd21/include
INCLUDE_PATHS+= ASF/sam0/utils/cmsis/samd21/source

INCLUDE_PATHS+= ASF/common/utils
INCLUDE_PATHS+= ASF/sam0/utils/header_files
INCLUDE_PATHS+= ASF/sam0/utils/
INCLUDE_PATHS+= ASF/sam0/utils/preprocessor

# SLEEP MGR
INCLUDE_PATHS+=ASF/common/services/sleepmgr
INCLUDE_PATHS+=ASF/common/services/sleepmgr/samd

# USB KEYBOARD and HID GENERIC
INCLUDE_PATHS+=ASF/common/services/usb
INCLUDE_PATHS+=ASF/common/services/usb/udc
INCLUDE_PATHS+=ASF/common/services/usb/class/hid
INCLUDE_PATHS+=ASF/common/services/usb/class/hid/device
INCLUDE_PATHS+=ASF/common/services/usb/class/hid/device/kbd
INCLUDE_PATHS+=ASF/common/services/usb/class/hid/device/generic

# BLUETOOTH
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/apps
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/ble_profiles/hid_device
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/ble_services/ble_mgr
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/ble_services/device_infomation
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/ble_services/hid
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/services/console
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/services/console/sam0
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/services/serial_fifo
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/services/timer/sam0
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/services/timer
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/services/serial/uart
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/services/serial/uart/sam0
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/src
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/inc
#INCLUDE_PATHS+=ASF/thirdparty/wireless/ble_sdk/utils

#INCLUDE_PATHS+=ASF/sam0/utils/stdio/stdio_serial
#INCLUDE_PATHS+=ASF/common/services/serial
#INCLUDE_PATHS+=ASF/sam0/drivers/sercom/usart

# DRIVERS SYSTEM
_DRIVERS_SYSTEM_PATH = ASF/sam0/drivers/system
INCLUDE_PATHS+= $(_DRIVERS_SYSTEM_PATH)
INCLUDE_PATHS+= $(_DRIVERS_SYSTEM_PATH)/clock
INCLUDE_PATHS+= $(_DRIVERS_SYSTEM_PATH)/clock/clock_samd21_r21_da
INCLUDE_PATHS+= $(_DRIVERS_SYSTEM_PATH)/pinmux
INCLUDE_PATHS+= $(_DRIVERS_SYSTEM_PATH)/power
INCLUDE_PATHS+= $(_DRIVERS_SYSTEM_PATH)/power/power_sam_d_r
INCLUDE_PATHS+= $(_DRIVERS_SYSTEM_PATH)/reset
INCLUDE_PATHS+= $(_DRIVERS_SYSTEM_PATH)/reset/reset_sam_d_r
INCLUDE_PATHS+= $(_DRIVERS_SYSTEM_PATH)/interrupt
INCLUDE_PATHS+= $(_DRIVERS_SYSTEM_PATH)/interrupt/system_interrupt_samd21

# DRIVERS
INCLUDE_PATHS+= ASF/sam0/drivers/extint
INCLUDE_PATHS+= ASF/sam0/drivers/extint/extint_sam_d_r
INCLUDE_PATHS+= ASF/sam0/drivers/port
INCLUDE_PATHS+= ASF/sam0/drivers/usb
INCLUDE_PATHS+= ASF/sam0/drivers/usb/stack_interface
INCLUDE_PATHS+= ASF/sam0/drivers/usb/usb_sam_d_r
INCLUDE_PATHS+= ASF/sam0/drivers/dma
INCLUDE_PATHS+= ASF/sam0/drivers/tc
INCLUDE_PATHS+= ASF/sam0/drivers/sercom
INCLUDE_PATHS+= ASF/sam0/drivers/sercom/usart

INCLUDE_PATHS+= ASF/common/boards
INCLUDE_PATHS+= ASF/common2/boards/user_board
INCLUDE_PATHS+= ASF/common2/services/delay

# Append -I
IDIR = $(addprefix -I, $(INCLUDE_PATHS))

# C Files
SRCS += ASF/sam0/utils/cmsis/samd21/source/gcc/startup_samd21.c
SRCS += main.c 
SRCS += USBHID/usbhid.c

# DRIVERS
SRCS += $(_DRIVERS_SYSTEM_PATH)/system.c
SRCS += $(_DRIVERS_SYSTEM_PATH)/clock/clock_samd21_r21_da/clock.c
SRCS += $(_DRIVERS_SYSTEM_PATH)/clock/clock_samd21_r21_da/gclk.c
SRCS += $(ASF_PATH)/common/utils/interrupt/interrupt_sam_nvic.c
SRCS += $(_DRIVERS_SYSTEM_PATH)/pinmux/pinmux.c
#SRCS += ASF/sam0/drivers/tc/tc_sam_d_r/tc.c
#SRCS += ASF/sam0/drivers/tc/tc_interrupt.c
SRCS += ASF/common/services/sleepmgr/samd/sleepmgr.c
SRCS += ASF/sam0/drivers/port/port.c
#SRCS += ASF/sam0/drivers/dma/dma.c
SRCS += ASF/sam0/drivers/extint/extint_sam_d_r/extint.c
SRCS += ASF/sam0/drivers/usb/stack_interface/usb_dual.c
SRCS += ASF/sam0/drivers/usb/stack_interface/usb_device_udd.c
SRCS += ASF/sam0/drivers/usb/usb_sam_d_r/usb.c
SRCS += ASF/common/services/usb/class/hid/device/kbd/udi_hid_kbd.c
SRCS += ASF/common/services/usb/class/hid/device/generic/udi_hid_generic.c
SRCS += ASF/common/services/usb/class/hid/device/udi_hid.c
SRCS += ASF/common/services/usb/class/composite/device/udi_composite_desc.c
SRCS += ASF/common/services/usb/udc/udc.c

#BLE
#SRCS += ASF/thirdparty/wireless/ble_sdk/apps/hid_keyboard_app.c
#SRCS += ASF/thirdparty/wireless/ble_sdk/ble_profiles/hid_device/hid_device.c
#SRCS += ASF/thirdparty/wireless/ble_sdk/ble_services/ble_mgr/ble_manager.c
#SRCS += ASF/thirdparty/wireless/ble_sdk/ble_services/device_infomation/device_info.c
#SRCS += ASF/thirdparty/wireless/ble_sdk/ble_services/hid/hid.c
#SRCS += ASF/thirdparty/wireless/ble_sdk/services/console/sam0/console_serial.c
#SRCS += ASF/thirdparty/wireless/ble_sdk/services/serial/uart/sam0/serial_drv.c
#SRCS += ASF/thirdparty/wireless/ble_sdk/services/serial_fifo/serial_fifo.c
#SRCS += ASF/thirdparty/wireless/ble_sdk/services/timer/sam0/timer_hw.c
#SRCS += ASF/thirdparty/wireless/ble_sdk/src/platform.c

#SRCS += ASF/common2/services/delay/sam0/systick_counter.c
#SRCS += ASF/sam0/utils/stdio/read.c
#SRCS += ASF/sam0/utils/stdio/write.c
#SRCS += ASF/sam0/drivers/sercom/sercom.c
#SRCS += ASF/sam0/drivers/sercom/sercom_interrupt.c
#SRCS += ASF/sam0/drivers/sercom/usart/usart.c
#SRCS += ASF/sam0/drivers/sercom/usart/usart_interrupt.c
#SRCS += ASF/sam0/drivers/extint/extint_callback.c            
#SRCS += ASF/sam0/drivers/extint/extint_sam_d_r_h/extint.c
#SRCS += ASF/sam0/utils/syscalls/gcc/syscalls.c

# Convert .c to .o
OBJS = $(SRCS:.c=.o)

# All target
all: $(OUTPUT_FILE)

# Link target
$(OUTPUT_FILE): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $(OUTPUT_FILE)
	$(OBJCOPY) -O binary $(OUTPUT_FILE_NAME).elf $(OUTPUT_FILE_NAME).bin
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock -R .signature  $(OUTPUT_FILE_NAME).elf $(OUTPUT_FILE_NAME).hex
	$(OBJDUMP) -h -S $(OUTPUT_FILE_NAME).elf > $(OUTPUT_FILE_NAME).lss
	$(SIZE) $(OUTPUT_FILE_NAME).elf

# Compile target(s)
./%.o: ./%.cpp
	$(CXX) $(CXXFLAGS) $(IDIR) -o $@ $<

./%.o: ./%.c
	$(CC) $(CFLAGS) $(IDIR) -o $@ $<

startup/%.o: startup/%.c
	$(CC) $(CFLAGS) $(IDIR) -o $@ $<

clean:
	rm -f $(OBJS)
	rm -f $(OUTPUT_FILE_NAME).elf $(OUTPUT_FILE_NAME).bin $(OUTPUT_FILE_NAME).hex $(OUTPUT_FILE_NAME).lss $(OUTPUT_FILE_NAME).map

flash: $(OUTPUT_FILE_NAME).elf
	openocd -f atmelice.cfg -c "program $(OUTPUT_FILE_NAME).elf verify reset exit"

erase: $(OUTPUT_FILE_NAME).elf
	openocd -f atmelice.cfg -c "flash erase_sector 0 0 last"


debug:
	openocd -f atmelice.cfg

gdb: 
	arm-none-eabi-gdb -x gdb.cfg $(OUTPUT_FILE)

usbdebug:
	sudo modprobe usbmon
	sudo wireshark &
