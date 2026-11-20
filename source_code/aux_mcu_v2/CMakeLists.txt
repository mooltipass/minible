# Mooltipass V2 aux mcu
cmake_minimum_required(VERSION 3.20.0)
project(aux_mcu_v2)

set(CONFIG_FILE "prj.conf")
# adding this option to enable memcpy_s from picolibc
add_compile_options("-D__STDC_WANT_LIB_EXT1__=1")

set(BOARD "minible_v2")
list(APPEND BOARD_ROOT ${PROJECT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

include(cmake/tinycbor.cmake)

target_include_directories(app PUBLIC
  src
)
target_compile_options(app PUBLIC -Wno-main)
target_compile_definitions(app PUBLIC __ASSERT_ON=1)
target_sources(app PRIVATE
  src/main.c
)
target_link_libraries(app PUBLIC tinycbor)
add_subdirectory(src/ports)
