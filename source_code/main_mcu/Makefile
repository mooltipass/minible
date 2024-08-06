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
-I"src/ASF/thirdparty/CMSIS/Include" \
-I"src/ASF/common/boards" \
-I"src/ASF/sam0/utils" \
-I"src/ASF/sam0/utils/header_files" \
-I"src/ASF/sam0/utils/preprocessor" \
-I"src/ASF/thirdparty/CMSIS/Include" \
-I"src/ASF/thirdparty/CMSIS/Lib/GCC" \
-I"src/ASF/common/utils" \
-I"src/ASF/sam0/utils/cmsis/samd21/include" \
-I"src/ASF/sam0/utils/cmsis/samd21/source" \
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
-I"src/ASF/common2/boards/user_board" \
-I"src" \
-I"src/config" \
-I"src/PLATFORM" \
-I"src/CLOCKS" \
-I"src/SERCOM" \
-I"src/FLASH" \
-I"src/FILESYSTEM" \
-I"src/DMA" \
-I"src/TIMER" \
-I"../packs/samd21a/include" \
-I"src/SMARTCARD" \
-I"src/OLED" \
-I"src/ACCELEROMETER" \
-I"src/INPUTS" \
-I"src/COMMS" \
-I"src/LOGIC" \
-I"src/SECURITY" \
-I"src/GUI" \
-I"src/NODEMGMT" \
-I"src/RNG" \
-I"src/BearSSL/src" \
-I"src/BearSSL/inc" \
-I"src/CRYPTO"

LIB_DIRS := \
-L"src/ASF/thirdparty/CMSIS/Lib/GCC" \
-L"Device_Startup"


ifeq ($(OS),Windows_NT)
INC_DIRS +=
else
INC_DIRS +=
endif

C_SRCS +=  \
src/ACCELEROMETER/lis2hh12.c \
src/BearSSL/src/symcipher/aes_ct.c \
src/BearSSL/src/symcipher/aes_ct_ctr.c \
src/BearSSL/src/symcipher/aes_ct_ctrcbc.c \
src/BearSSL/src/symcipher/aes_ct_enc.c \
src/BearSSL/src/hash/sha1.c \
src/BearSSL/src/hash/sha2small.c \
src/BearSSL/src/mac/hmac.c \
src/BearSSL/src/rand/hmac_drbg.c \
src/BearSSL/src/ec/ec_p256_m15.c \
src/BearSSL/src/ec/ecdsa_i15_sign_raw.c \
src/BearSSL/src/ec/ec_keygen.c \
src/BearSSL/src/ec/ec_pubkey.c \
src/BearSSL/src/ec/ec_secp256r1.c \
src/BearSSL/src/ec/ec_secp384r1.c \
src/BearSSL/src/ec/ec_secp521r1.c \
src/BearSSL/src/ec/ecdsa_i15_bits.c \
src/BearSSL/src/int/i15_ninv15.c \
src/BearSSL/src/int/i15_encode.c \
src/BearSSL/src/int/i15_decode.c \
src/BearSSL/src/int/i15_decmod.c \
src/BearSSL/src/int/i15_add.c \
src/BearSSL/src/int/i15_sub.c \
src/BearSSL/src/int/i15_modpow.c \
src/BearSSL/src/int/i15_muladd.c \
src/BearSSL/src/int/i15_montmul.c \
src/BearSSL/src/int/i15_fmont.c \
src/BearSSL/src/int/i15_iszero.c \
src/BearSSL/src/int/i15_rshift.c \
src/BearSSL/src/int/i15_bitlen.c \
src/BearSSL/src/int/i15_tmont.c \
src/BearSSL/src/codec/ccopy.c \
src/BearSSL/src/codec/dec32be.c \
src/BearSSL/src/codec/enc32be.c \
src/CLOCKS/driver_clocks.c \
src/COMMS/comms_aux_mcu.c \
src/COMMS/comms_hid_msgs.c \
src/COMMS/comms_hid_msgs_debug.c \
src/debug.c \
src/DMA/dma.c \
src/FILESYSTEM/custom_bitstream.c \
src/FILESYSTEM/custom_fs.c \
src/FILESYSTEM/custom_fs_emergency_font.c \
src/FLASH/dataflash.c \
src/FLASH/dbflash.c \
src/functional_testing.c \
src/GUI/gui_carousel.c \
src/GUI/gui_dispatcher.c \
src/GUI/gui_menu.c \
src/GUI/gui_prompts.c \
src/INPUTS/inputs.c \
src/LOGIC/logic_aux_mcu.c \
src/LOGIC/logic_bluetooth.c \
src/LOGIC/logic_database.c \
src/LOGIC/logic_device.c \
src/LOGIC/logic_encryption.c \
src/LOGIC/logic_gui.c \
src/LOGIC/logic_power.c \
src/LOGIC/logic_security.c \
src/LOGIC/logic_smartcard.c \
src/LOGIC/logic_user.c \
src/LOGIC/logic_accelerometer.c \
src/NODEMGMT/nodemgmt.c \
src/OLED/mooltipass_graphics_bundle.c \
src/OLED/sh1122.c \
src/PLATFORM/platform_io.c \
src/RNG/rng.c \
src/SECURITY/fuses.c \
src/SERCOM/driver_sercom.c \
src/SMARTCARD/smartcard_highlevel.c \
src/SMARTCARD/smartcard_lowlevel.c \
src/TIMER/driver_timer.c \
src/utils.c \
src/ASF/common2/boards/user_board/init.c \
src/ASF/common/utils/interrupt/interrupt_sam_nvic.c \
src/ASF/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/clock.c \
src/ASF/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/gclk.c \
src/ASF/sam0/drivers/system/interrupt/system_interrupt.c \
src/ASF/sam0/drivers/system/pinmux/pinmux.c \
src/ASF/sam0/drivers/system/system.c \
src/ASF/sam0/utils/cmsis/samd21/source/gcc/startup_samd21.c \
src/ASF/sam0/utils/cmsis/samd21/source/system_samd21.c \
src/ASF/sam0/utils/syscalls/gcc/syscalls.c \
src/main.c \
src/LOGIC/logic_fido2.c \
src/CRYPTO/monocypher.c \
src/CRYPTO/monocypher-ed25519.c

ifeq ($(PLATFORM),)
	PLATFORM = PLAT_V7_SETUP
endif

TEXT_SEC_START = 0x2000
ifeq ($(PLATFORM),PLAT_V7_SETUP)
	TEXT_SEC_START = 0x8000
endif

C_FLAGS := -mthumb

ifeq ($(DEBUG), 1)
    C_FLAGS += -DDEBUG -D$(PLATFORM) -g3 -O1
    OUTPUT_DIR := Debug
else
    C_FLAGS += -DDEBUG_LOG_DISABLED -DNDEBUG -D$(PLATFORM) -Os
    OUTPUT_DIR := Release
endif

C_FLAGS += -fdata-sections -ffunction-sections -mlong-calls -Wall -mcpu=cortex-m0plus -c -pipe -fno-strict-aliasing -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror-implicit-function-declaration -Wpointer-arith -std=gnu99 -ffunction-sections -fdata-sections -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Wmain -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return  -Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Wunreachable-code -Wcast-align --param max-inline-insns-single=500 -Wduplicated-cond -Wlogical-op -Wnull-dereference -Wjump-misses-init

C_DEFINES := -D__SAMD21G18A__ -DBOARD=USER_BOARD -DARM_MATH_CM0PLUS=true -D__CORTEX_SC=0 -DBR_ARMEL_CORTEXM_GCC=1 -DBR_amd64=0 -DBR_BE_UNALIGNED=0 -DBR_CT_MUL15=0 -DBR_ENABLE_INTRINSICS=0 -DBR_CT_MUL31=0 -DBR_i386=0 -DBR_LE_UNALIGNED=0 -DBR_NO_ARITH_SHIFT=0 -DBR_POWER_ASM_MACROS=0 -D_ARCH_PWR8=0 -D_MSC_VER=0 -D_M_IX86=0 -D_M_X64=0 -D__clang__=0 -DBR_POWER8=0

LINKER_SCRIPT_DEP +=  \
src/ASF/sam0/utils/linker_scripts/samd21/gcc/samd21g18a_flash.ld

OBJS := $(C_SRCS:.c=.o)
OBJS :=  $(addprefix $(OUTPUT_DIR)/,$(OBJS))
OBJ_DIRS := $(sort $(dir $(OBJS)))

C_DEPS := $(C_SRCS:.c=.d)
C_DEPS :=  $(addprefix $(OUTPUT_DIR)/,$(C_DEPS))

TARGET := $(OUTPUT_DIR)/mini_ble.elf
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

$(TARGET): $(OBJS) $(LIB_DEP) $(LINKER_SCRIPT_DEP)
	@echo Building target: $@
	@echo Invoking: ARM/GNU Linker
	$(CC) -o$(TARGET) $(OBJS) $(LIBS) -mthumb -Wl,-Map=$(TARGET_MAP) --specs=nano.specs -Wl,--start-group -larm_cortexM0l_math -lm  -Wl,--end-group $(LIB_DIRS) -Wl,--gc-sections -Wl,-section-start=.start_app_function_addr=0x200 -Wl,-section-start=.flash_start_addr=0x0 -Wl,-section-start=.text=$(TEXT_SEC_START) -mcpu=cortex-m0plus -Wl,--entry=Reset_Handler -Wl,--cref -T$(LINKER_SCRIPT_DEP) -Xlinker --undefined=jump_to_application_function_addr -Xlinker --undefined=jump_to_application_function
	@echo Finished building target: $@
	$(OBJCOPY) --verbose --gap-fill 0xFF -j .text -j .stack -j .bss -j .relocate -j .start_app_function_addr -j .flash_start_addr -j .ARM.attributes -O binary $(TARGET) $(TARGET_BIN)
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
wipe:
	$(RM) $(OUTPUT_DIR)

flash: $(TARGET)
	edbg -b -t samd21 -e -p -f $(TARGET_BIN)
