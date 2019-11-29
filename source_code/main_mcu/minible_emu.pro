QT       += core network gui widgets

TEMPLATE = app

TARGET = minible_emu

CONFIG += c++11

INCLUDEPATH += src/EMU \
    src \
    src/config \
    src/PLATFORM \
    src/CLOCKS \
    src/SERCOM \
    src/FLASH \
    src/FILESYSTEM \
    src/DMA \
    src/TIMER \
    src/SMARTCARD \
    src/OLED \
    src/ACCELEROMETER \
    src/INPUTS \
    src/COMMS \
    src/LOGIC \
    src/SECURITY \
    src/GUI \
    src/NODEMGMT \
    src/RNG \
    src/BearSSL/src \
    src/BearSSL/inc

SOURCES += src/EMU/lis2hh12.c \
    src/BearSSL/src/symcipher/aes_ct.c \
    src/BearSSL/src/symcipher/aes_ct_ctr.c \
    src/BearSSL/src/symcipher/aes_ct_ctrcbc.c \
    src/BearSSL/src/symcipher/aes_ct_enc.c \
    src/COMMS/comms_aux_mcu.c \
    src/COMMS/comms_hid_msgs.c \
    src/COMMS/comms_hid_msgs_debug.c \
    src/EMU/dma.c \
    src/FILESYSTEM/custom_bitstream.c \
    src/FILESYSTEM/custom_fs.c \
    src/FILESYSTEM/custom_fs_emergency_font.c \
    src/EMU/dataflash.c \
    src/EMU/dbflash.c \
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
    src/NODEMGMT/nodemgmt.c \
    src/OLED/mooltipass_graphics_bundle.c \
    src/OLED/sh1122.c \
    src/EMU/platform_io.c \
    src/RNG/rng.c \
    src/EMU/fuses.c \
    src/EMU/driver_sercom.c \
    src/SMARTCARD/smartcard_highlevel.c \
    src/EMU/smartcard_lowlevel.c \
    src/TIMER/driver_timer.c \
    src/utils.c \
    src/main.c \
    src/EMU/emu_aux_mcu.c \
    src/EMU/emulator.cpp \
    src/EMU/emu_oled.cpp \
    src/EMU/emu_smartcard.cpp \
    src/EMU/emu_storage.cpp \
    src/EMU/emulator_ui.cpp

QMAKE_CXXFLAGS += -fdata-sections \
    -ffunction-sections \
    -Wall \
    -pipe \
    -fno-strict-aliasing \
    -Werror-implicit-function-declaration \
    -Wpointer-arith \
    -ffunction-sections \
    -fdata-sections \
    -Wchar-subscripts \
    -Wcomment \
    -Wformat=2 \
    -Wmain \
    -Wparentheses \
    -Wsequence-point \
    -Wreturn-type \
    -Wswitch \
    -Wtrigraphs \
    -Wunused \
    -Wuninitialized \
    -Wunknown-pragmas \
    -Wundef \
    -Wshadow \
    -Wwrite-strings \
    -Wsign-compare \
    -Wmissing-declarations \
    -Wformat \
    -Wmissing-format-attribute \
    -Wno-deprecated-declarations \
    -Wpacked \
    -Wredundant-decls \
    -Wunreachable-code \
    -Wcast-align \
    -Wlogical-op \
    -fPIC
    
DEFINES += EMULATOR_BUILD

HEADERS  += src/MainWindow.h \ \
    src/BearSSL/inc/bearssl.h \
    src/COMMS/comms_aux_mcu.h \
    src/COMMS/comms_aux_mcu_defines.h \
    src/COMMS/comms_bootloader_msg.h \
    src/COMMS/comms_hid_msgs.h \
    src/COMMS/comms_hid_msgs_debug.h \
    src/EMU/asf.h \
    src/EMU/emu_aux_mcu.h \
    src/EMU/emu_oled.h \
    src/EMU/emu_smartcard.h \
    src/EMU/emu_storage.h \
    src/EMU/emulator.h \
    src/EMU/emulator_ui.h \
    src/EMU/qt_metacall_helper.h \
    src/FILESYSTEM/custom_bitstream.h \
    src/FILESYSTEM/custom_fs.h \
    src/FILESYSTEM/custom_fs_emergency_font.h \
    src/FILESYSTEM/text_ids.h \
    src/GUI/gui_carousel.h \
    src/GUI/gui_dispatcher.h \
    src/GUI/gui_menu.h \
    src/GUI/gui_prompts.h \
    src/INPUTS/inputs.h \
    src/LOGIC/logic_aux_mcu.h \
    src/LOGIC/logic_bluetooth.h \
    src/LOGIC/logic_database.h \
    src/LOGIC/logic_device.h \
    src/LOGIC/logic_encryption.h \
    src/LOGIC/logic_gui.h \
    src/LOGIC/logic_power.h \
    src/LOGIC/logic_security.h \
    src/LOGIC/logic_smartcard.h \
    src/LOGIC/logic_user.h \
    src/NODEMGMT/nodemgmt.h \
    src/OLED/mooltipass_graphics_bundle.h \
    src/OLED/sh1122.h \
    src/RNG/rng.h \
    src/SMARTCARD/smartcard_highlevel.h \
    src/TIMER/driver_timer.h \
    src/defines.h \
    src/main.h \
    src/utils.h