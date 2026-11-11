# Main MCU ATSAMD toolchain

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m0plus)
set(CMAKE_SYSTEM_ARCH armv6-m)
#force cmake to skip auto check

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_ASM_COMPILER arm-none-eabi-as)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_LINKER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_RANLIB arm-none-eabi-ranlib)
set(CMAKE_SIZE arm-none-eabi-size)
set(CMAKE_STRIP arm-none-eabi-strip)

add_compile_options(-mlittle-endian -mthumb -mcpu=cortex-m0plus -mfloat-abi=soft)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wshadow -Wdouble-promotion")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wundef -Wconversion -Warith-conversion")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -mthumb")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nano.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nolibc -lc -lm -Wno-common")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--Map=${PROJECT_NAME}.map,-cref") 
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--print-memory-usage")

