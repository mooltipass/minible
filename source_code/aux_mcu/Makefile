ifeq ($(OS),Windows_NT)
SHELL := cmd.exe
#CROSS_COMPILE := C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-
CROSS_COMPILE := arm-none-eabi-
MKDIR := mkdir

define create_dir
	@if not exist "$(1)" $(MKDIR) "$(1)"
endef

SHELL := sh

else

CROSS_COMPILE := arm-none-eabi-
MKDIR := mkdir -p

define create_dir
	@$(MKDIR) $(1)
endef
endif

RM := rm -rf

CC    := "$(CROSS_COMPILE)gcc"
CPP   := "$(CROSS_COMPILE)g++"
ASM   := "$(CROSS_COMPILE)as"
LINK  := "$(CROSS_COMPILE)gcc"
OBJCOPY   := "$(CROSS_COMPILE)objcopy"
OBJDUMP   := "$(CROSS_COMPILE)objdump"
SIZE  := "$(CROSS_COMPILE)size"

INC_DIRS := \
-I"src" \
-I"src/ASF/thirdparty/wireless/ble_sdk/ble_profiles/hid_device" \
-I"src/ASF/thirdparty/wireless/ble_sdk/ble_services/hid" \
-I"src/ASF/thirdparty/wireless/ble_sdk/ble_services/ble_mgr" \
-I"src/ASF/thirdparty/wireless/ble_sdk/ble_services/battery" \
-I"src/ASF/thirdparty/wireless/ble_sdk/inc" \
-I"src/ASF/thirdparty/wireless/ble_sdk/services/serial/uart" \
-I"src/ASF/sam0/drivers/sercom/usart" \
-I"src/ASF/sam0/utils" \
-I"src/ASF/sam0/utils/header_files" \
-I"src/ASF/sam0/utils/preprocessor" \
-I"src/ASF/thirdparty/CMSIS/Include" \
-I"src/ASF/thirdparty/CMSIS/Lib/GCC" \
-I"src/ASF/common/utils" \
-I"src/ASF/sam0/utils/cmsis/samd21/include" \
-I"src/ASF/sam0/utils/cmsis/samd21/source" \
-I"src/ASF/sam0/drivers/sercom" \
-I"src/ASF/sam0/drivers/system" \
-I"src/ASF/sam0/drivers/system/clock/clock_samd21_r21_da_ha1" \
-I"src/ASF/sam0/drivers/system/clock" \
-I"src/ASF/sam0/drivers/system/interrupt" \
-I"src/ASF/sam0/drivers/system/interrupt/system_interrupt_samd21" \
-I"src/ASF/sam0/drivers/system/pinmux" \
-I"src/ASF/sam0/drivers/system/power" \
-I"src/ASF/sam0/drivers/system/power/power_sam_d_r_h" \
-I"src/ASF/sam0/drivers/system/reset" \
-I"src/ASF/sam0/drivers/system/reset/reset_sam_d_r_h" \
-I"src/ASF/common2/services/delay" \
-I"src/ASF/common2/services/delay/sam0" \
-I"src/ASF/sam0/drivers/extint" \
-I"src/ASF/thirdparty/wireless/ble_sdk/services/serial_fifo" \
-I"src/ASF/thirdparty/wireless/ble_sdk/utils" \
-I"src/ASF/thirdparty/wireless/ble_sdk/services/console" \
-I"src/ASF/sam0/utils/stdio/stdio_serial" \
-I"src/ASF/common/services/serial" \
-I"src/ASF/thirdparty/wireless/ble_sdk/services/timer" \
-I"src/ASF/sam0/drivers/tc" \
-I"src/ASF/sam0/drivers/port" \
-I"src/ASF/thirdparty/wireless/ble_sdk/ble_services/device_infomation" \
-I"src/ASF/sam0/boards/samd21_xplained_pro" \
-I"src/ASF/sam0/boards" \
-I"src/ASF/common/boards" \
-I"src/config" \
-I"src/CLOCKS" \
-I"src/COMMS" \
-I"src/DMA" \
-I"src/LOGIC" \
-I"src/PLATFORM" \
-I"src/SECURITY" \
-I"src/TIMER" \
-I"src/USB" \
-I"src/fido2" \
-I"src/tinycbor/src"

LIBCBOR := tinycbor

LIB_DIRS := \
-L"src/ASF/thirdparty/wireless/ble_sdk/lib/cm0p/gcc" \
-L"src/ASF/thirdparty/CMSIS/Lib/GCC" \
-L"src/tinycbor/lib"

LIB_DEP := \
$(LIBCBOR)

ifeq ($(OS),Windows_NT)
INC_DIRS +=
else
INC_DIRS +=
endif

C_SRCS +=  \
src/ASF/thirdparty/wireless/ble_sdk/ble_services/battery/battery.c \
src/CLOCKS/driver_clocks.c \
src/COMMS/comms_hid_msgs.c \
src/COMMS/comms_main_mcu.c \
src/COMMS/comms_raw_hid.c \
src/debug.c \
src/DMA/dma.c \
src/LOGIC/logic.c \
src/LOGIC/logic_battery.c \
src/LOGIC/logic_bluetooth.c \
src/LOGIC/logic_keyboard.c \
src/LOGIC/logic_sleep.c \
src/LOGIC/logic_rng.c \
src/main.c \
src/PLATFORM/platform_io.c \
src/SECURITY/fuses.c \
src/TIMER/driver_timer.c \
src/USB/udc.c \
src/USB/usb.c \
src/USB/usb_descriptors.c \
src/fido2/ctaphid.c \
src/fido2/ctap.c \
src/fido2/solo_compat_layer.c \
src/fido2/ctap_parse.c \
src/ASF/common2/services/delay/sam0/systick_counter.c \
src/ASF/common/utils/interrupt/interrupt_sam_nvic.c \
src/ASF/sam0/boards/samd21_xplained_pro/board_init.c \
src/ASF/sam0/drivers/extint/extint_callback.c \
src/ASF/sam0/drivers/extint/extint_sam_d_r_h/extint.c \
src/ASF/sam0/drivers/port/port.c \
src/ASF/sam0/drivers/sercom/sercom.c \
src/ASF/sam0/drivers/sercom/sercom_interrupt.c \
src/ASF/sam0/drivers/sercom/usart/usart.c \
src/ASF/sam0/drivers/sercom/usart/usart_interrupt.c \
src/ASF/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/clock.c \
src/ASF/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/gclk.c \
src/ASF/sam0/drivers/system/interrupt/system_interrupt.c \
src/ASF/sam0/drivers/system/pinmux/pinmux.c \
src/ASF/sam0/drivers/system/system.c \
src/ASF/sam0/drivers/tc/tc_interrupt.c \
src/ASF/sam0/drivers/tc/tc_sam_d_r_h/tc.c \
src/ASF/sam0/utils/cmsis/samd21/source/gcc/startup_samd21.c \
src/ASF/sam0/utils/cmsis/samd21/source/system_samd21.c \
src/ASF/sam0/utils/stdio/read.c \
src/ASF/sam0/utils/stdio/write.c \
src/ASF/sam0/utils/syscalls/gcc/syscalls.c \
src/ASF/thirdparty/wireless/ble_sdk/ble_services/ble_mgr/ble_manager.c \
src/ASF/thirdparty/wireless/ble_sdk/ble_services/device_infomation/device_info.c \
src/ASF/thirdparty/wireless/ble_sdk/services/console/sam0/console_serial.c \
src/ASF/thirdparty/wireless/ble_sdk/services/serial/uart/sam0/serial_drv.c \
src/ASF/thirdparty/wireless/ble_sdk/services/serial_fifo/serial_fifo.c \
src/ASF/thirdparty/wireless/ble_sdk/services/timer/sam0/timer_hw.c \
src/ASF/thirdparty/wireless/ble_sdk/src/platform.c

ifeq ($(PLATFORM),)
	PLATFORM := PLAT_V6_SETUP
endif

TEXT_SEC_START = 0x800

C_FLAGS := -mthumb

ifeq ($(DEBUG), 1)
    C_FLAGS += -DDEBUG -D$(PLATFORM) -g3 -O1 -fstack-usage
    OUTPUT_DIR := Debug
else
    C_FLAGS += -DDEBUG_LOG_DISABLED -DNDEBUG -D$(PLATFORM) -Os
    OUTPUT_DIR := Release
endif

C_FLAGS += -fdata-sections -ffunction-sections -mlong-calls -Wall -mcpu=cortex-m0plus -c -pipe -fno-strict-aliasing -Wall -Wmissing-prototypes -Werror-implicit-function-declaration -Wpointer-arith -std=gnu99 -ffunction-sections -fdata-sections -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Wmain -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return  -Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wnested-externs -Wlong-long -Wunreachable-code -Wcast-align --param max-inline-insns-single=500 -Wstrict-prototypes

C_DEFINES := -D__SAMD21E18A__ -DENABLE_POWER_SAVEXX -DSLEEP_WALKING_ENABLED=false -DHOST_SLEEP_ENABLE=true -DSYSTICK_MODE -DNENABLE_PTS -DTC_ASYNC=true -DHID_DEVICE -DUART_FLOWCONTROL_4WIRE_MODE=false -DNEW_EVT_HANDLER -DBOARD=USER_BOARD -DATT_DB_MEMORY -DHOST_UART_BAUDRATE_CONFIG_VALUE=921600 -DUART_FLOWCONTROL_6WIRE_MODE=true -DBLE_DEVICE_ROLE=BLE_ROLE_PERIPHERAL -DHID_KEYBOARD_DEVICE -DARM_MATH_CM0PLUS=true -DDEVICE_INFORMATION_SERVICE -DBLE_MODULE=BTLC1000_MR -DHID_SERVICE -D__SAMD21E18A__ -DEXTINT_CALLBACK_MODE=true -DUSART_CALLBACK_MODE=true -DHID_GATT_SERVER -DBATTERY_SERVICE

LINKER_SCRIPT_DEP +=  \
src/ASF/sam0/utils/linker_scripts/samd21/gcc/samd21j18a_flash.ld

OBJS := $(C_SRCS:.c=.o)
OBJS :=  $(addprefix $(OUTPUT_DIR)/,$(OBJS))
OBJ_DIRS := $(sort $(dir $(OBJS)))

C_DEPS := $(C_SRCS:.c=.d)
C_DEPS :=  $(addprefix $(OUTPUT_DIR)/,$(C_DEPS))

TARGET := $(OUTPUT_DIR)/aux_mcu.elf
TARGET_BIN := $(TARGET:.elf=.bin)
TARGET_HEX := $(TARGET:.elf=.hex)
TARGET_LSS := $(TARGET:.elf=.lss)
TARGET_MAP := $(TARGET:.elf=.map)
TARGET_SREC := $(TARGET:.elf=.srec)
TARGET_EEP := $(TARGET:.elf=.eep)

#OUTPUT_FILE_DEP:= ../makedep.mk

#$(OUTPUT_DIR):
#	$(MKDIR) $(OBJ_DIRS)

#.PHONY: $(OBJ_DIRS)

$(OUTPUT_DIR)/%.o: %.c
	@echo Building file: $@
	@echo Invoking: ARM/GNU C Compiler
	@$(call create_dir,$(dir $@))
	$(CC) $(C_FLAGS) $(C_DEFINES) $(INC_DIRS) -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<"
	@echo Finished building: $@

#ifneq ($(MAKECMDGOALS),clean)
#ifneq ($(strip $(C_DEPS)),)
#-include $(C_DEPS)
#endif
#endif

# All Target
all: $(TARGET)

$(LIBCBOR):
	#$(MAKE) -C src/tinycbor clean
	$(MAKE) -C src/tinycbor LDFLAGS='' CC="$(CROSS_COMPILE)gcc" CPPFLAGS="$(C_FLAGS)" lib/libtinycbor.a

$(LIBBEAR):

$(TARGET): $(OBJS) $(LIB_DEP) $(LINKER_SCRIPT_DEP)
	@echo Building target: $@
	@echo Invoking: ARM/GNU Linker
	$(CC) -o$(TARGET) $(OBJS) $(LIBS) -mthumb -Wl,-Map=$(TARGET_MAP) --specs=nano.specs -Wl,--start-group -larm_cortexM0l_math -lble_sdk -lm -l$(LIBCBOR) -Wl,--end-group $(LIB_DIRS) -Wl,--gc-sections -Wl,-section-start=.start_app_function_addr=0x200 -Wl,-section-start=.flash_start_addr=0x0 -Wl,-section-start=.text=$(TEXT_SEC_START) -Wl,-section-start=.data=0x20000004 -Wl,-section-start=.relocate=0x20000004 -Wl,-section-start=.bootloader_flag=0x20000000  -mcpu=cortex-m0plus -Wl,--entry=Reset_Handler -Wl,--cref -T$(LINKER_SCRIPT_DEP) -Xlinker --undefined=jump_to_application_function_addr -Xlinker --undefined=jump_to_application_function
	@echo Finished building target: $@
	$(OBJCOPY) -O binary $(TARGET) $(TARGET_BIN)
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock -R .signature $(TARGET) $(TARGET_HEX)
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0 --no-change-warnings -O binary $(TARGET) $(TARGET_EEP) || exit 0
	$(OBJDUMP) -h -S $(TARGET) > $(TARGET_LSS)
	$(OBJCOPY) -O srec -R .eeprom -R .fuse -R .lock -R .signature $(TARGET) $(TARGET_SREC)
	$(SIZE) $(TARGET)

# Other Targets
clean:
	$(RM) $(OBJS)
	$(RM) $(C_DEPS)
	rm -rf $(TARGET) $(TARGET_BIN) $(TARGET_HEX) $(TARGET_LSS) $(TARGET_EEP) $(TARGET_MAP) $(TARGET_SREC)
	$(MAKE) -C src/tinycbor clean
wipe:
	$(RM) $(OUTPUT_DIR)

flash: $(TARGET)
	edbg -b -t samd21 -p -f $(TARGET_BIN)

