cmake_minimum_required(VERSION 3.13)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake")
  include(pico_sdk_import.cmake)
elseif(DEFINED ENV{PICO_SDK_PATH})
  include("$ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake")
else()
  message(FATAL_ERROR "Set PICO_SDK_PATH to your pico-sdk checkout")
endif()
# Keep the RAM-resident kernel small by default. Explicit build types win.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build type" FORCE)
endif()
project(piconix C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()
find_package(Python3 COMPONENTS Interpreter REQUIRED)
# Reject invalid RAM application IDs before building firmware.
execute_process(
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/check_program_capacity.py"
  RESULT_VARIABLE PICONIX_CAPACITY_CHECK_RESULT
)
if(NOT PICONIX_CAPACITY_CHECK_RESULT EQUAL 0)
  message(FATAL_ERROR "RAM application ID validation failed")
endif()
if(NOT CMAKE_READELF)
  get_filename_component(TOOLCHAIN_BIN_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
  set(CMAKE_READELF "${TOOLCHAIN_BIN_DIR}/arm-none-eabi-readelf")
endif()
# Freestanding slot-linked images. All user program images are embedded in the firmware Flash.
set(PICONIX_EMBED_SOURCES "")
set(PICONIX_SYMBOL_ELFS "")
set(PICONIX_SYMBOL_BINS "")
foreach(APP IN ITEMS pins fbctl pwmctl kitesys shell blink omikuji python hello hexdump trace ps cat watch pwd ls rm echo kill tty stty mb reset lsof pong gpio fbed asm help edit mem tree find nc httpd tcp_echod udp_echod)
  # Compile each application once for the same canonical address.
  # Every image must also link identically at the next 32-KiB slot address.
  set(APP_ORIGIN 0x20010000)
  set(APP_DIR "${CMAKE_CURRENT_BINARY_DIR}/ram_${APP}")
  file(MAKE_DIRECTORY "${APP_DIR}")
  set(APP_LD "${APP_DIR}/slot.ld")
  configure_file("${CMAKE_CURRENT_LIST_DIR}/userland/slot.ld" "${APP_LD}" @ONLY)
  set(APP_OBJ "${APP_DIR}/${APP}.o")
  set(CRT_OBJ "${APP_DIR}/crt0.o")
  set(SYSCALLS_OBJ "${APP_DIR}/syscalls.o")
  set(LIBC_OBJ "${APP_DIR}/libc.o")
  set(APP_LD_ALT "${APP_DIR}/slot_alt.ld")
  set(APP_ORIGIN 0x20018000)
  configure_file("${CMAKE_CURRENT_LIST_DIR}/userland/slot.ld" "${APP_LD_ALT}" @ONLY)
  set(APP_ORIGIN 0x20010000)
  set(APP_ELF_ALT "${APP_DIR}/${APP}_alt.elf")
  set(APP_BIN_ALT "${APP_DIR}/${APP}_alt.bin")
  set(APP_ELF "${APP_DIR}/${APP}.elf")
  set(APP_BIN "${APP_DIR}/${APP}.bin")
  # RAM apps link libgcc for compiler-generated arithmetic helpers. Prevent GCC from emitting
  # Thumb-1 switch helpers such as __gnu_thumb1_case_uqi.
  set(APP_EXTRA_CFLAGS -fno-jump-tables -fno-tree-switch-conversion)
  add_custom_command(OUTPUT "${APP_BIN}"
    COMMAND "${CMAKE_C_COMPILER}" -mcpu=cortex-m0plus -mthumb -Os -fPIC
            -ffreestanding -fno-builtin -ffunction-sections -fdata-sections
            -fno-unwind-tables -fno-asynchronous-unwind-tables
            ${APP_EXTRA_CFLAGS}
            -I "${CMAKE_CURRENT_BINARY_DIR}"
            -c "${CMAKE_CURRENT_LIST_DIR}/userland/${APP}.c" -o "${APP_OBJ}"
    COMMAND "${CMAKE_C_COMPILER}" -mcpu=cortex-m0plus -mthumb -Os -fPIC
            -ffreestanding -fno-builtin -ffunction-sections -fdata-sections
            -fno-unwind-tables -fno-asynchronous-unwind-tables
            -c "${CMAKE_CURRENT_LIST_DIR}/userland/crt0.c" -o "${CRT_OBJ}"
    COMMAND "${CMAKE_C_COMPILER}" -mcpu=cortex-m0plus -mthumb -Os -fPIC
            -ffreestanding -fno-builtin -ffunction-sections -fdata-sections
            -fno-unwind-tables -fno-asynchronous-unwind-tables
            -I "${CMAKE_CURRENT_LIST_DIR}/userland"
            -c "${CMAKE_CURRENT_LIST_DIR}/userland/syscalls.c" -o "${SYSCALLS_OBJ}"
    COMMAND "${CMAKE_C_COMPILER}" -mcpu=cortex-m0plus -mthumb -Os -fPIC
            -ffreestanding -fno-builtin -ffunction-sections -fdata-sections
            -fno-unwind-tables -fno-asynchronous-unwind-tables
            -I "${CMAKE_CURRENT_LIST_DIR}/userland"
            -c "${CMAKE_CURRENT_LIST_DIR}/userland/libc.c" -o "${LIBC_OBJ}"
    COMMAND "${CMAKE_C_COMPILER}" -mcpu=cortex-m0plus -mthumb -nostdlib
            -Wl,--gc-sections "-Wl,-z,max-page-size=0x1000" "-Wl,-T,${APP_LD}"
            "${CRT_OBJ}" "${APP_OBJ}" "${SYSCALLS_OBJ}" "${LIBC_OBJ}" -lgcc -o "${APP_ELF}"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "${APP_ELF}" "${APP_BIN}"
    COMMAND "${CMAKE_C_COMPILER}" -mcpu=cortex-m0plus -mthumb -nostdlib
            -Wl,--gc-sections "-Wl,-z,max-page-size=0x1000" "-Wl,-T,${APP_LD_ALT}"
            "${CRT_OBJ}" "${APP_OBJ}" "${SYSCALLS_OBJ}" "${LIBC_OBJ}" -lgcc -o "${APP_ELF_ALT}"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "${APP_ELF_ALT}" "${APP_BIN_ALT}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/check_pic.py"
            "${APP_ELF}" "${APP_BIN}" "${APP_ELF_ALT}" "${APP_BIN_ALT}"
            --readelf "${CMAKE_READELF}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/check_slot_image.py"
            "${APP_ELF}" "${APP_BIN}" "${APP_ORIGIN}" --readelf "${CMAKE_READELF}"
    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/userland/${APP}.c"
            "${CMAKE_CURRENT_LIST_DIR}/kernel/pins.h"
            "${CMAKE_CURRENT_LIST_DIR}/userland/crt0.c"
            "${CMAKE_CURRENT_LIST_DIR}/userland/syscalls.c"
            "${CMAKE_CURRENT_LIST_DIR}/userland/unistd.h"
            "${CMAKE_CURRENT_LIST_DIR}/include/piconix_dirent.h"
            "${CMAKE_CURRENT_LIST_DIR}/userland/fcntl.h"
            "${CMAKE_CURRENT_LIST_DIR}/userland/libc.c"
            "${CMAKE_CURRENT_LIST_DIR}/userland/stdio.h"
            "${CMAKE_CURRENT_LIST_DIR}/userland/stdlib.h"
            "${CMAKE_CURRENT_LIST_DIR}/userland/string.h"
            "${CMAKE_CURRENT_LIST_DIR}/userland/trace.h"
            "${APP_LD}" "${APP_LD_ALT}"
            "${CMAKE_CURRENT_LIST_DIR}/tools/check_pic.py"
            "${CMAKE_CURRENT_LIST_DIR}/tools/check_slot_image.py"
    VERBATIM)
  set(EMBED_APP ${APP})
  set(EMBED_BIN "${APP_BIN}")
  configure_file("${CMAKE_CURRENT_LIST_DIR}/userland/embed_image.S.in"
                 "${APP_DIR}/embed_image.S" @ONLY)
  set_source_files_properties("${APP_DIR}/embed_image.S" PROPERTIES OBJECT_DEPENDS "${APP_BIN}")
  list(APPEND PICONIX_EMBED_SOURCES "${APP_DIR}/embed_image.S")
  list(APPEND PICONIX_SYMBOL_ELFS "${APP_ELF}")
  list(APPEND PICONIX_SYMBOL_BINS "${APP_BIN}")
endforeach()
# Generate a Flash-resident symbol catalog from linked RAM app ELF files.
set(PICONIX_SYMBOL_FILE "${CMAKE_CURRENT_BINARY_DIR}/piconix_symbols.txt")
add_custom_command(OUTPUT "${PICONIX_SYMBOL_FILE}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/make_symbols.py"
          --nm "${CMAKE_NM}" --output "${PICONIX_SYMBOL_FILE}"
          ${PICONIX_SYMBOL_ELFS}
  DEPENDS ${PICONIX_SYMBOL_BINS} "${CMAKE_CURRENT_LIST_DIR}/tools/make_symbols.py"
  VERBATIM)
configure_file("${CMAKE_CURRENT_LIST_DIR}/userland/embed_symbols.S.in"
               "${CMAKE_CURRENT_BINARY_DIR}/embed_symbols.S" @ONLY)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/embed_symbols.S"
  PROPERTIES OBJECT_DEPENDS "${PICONIX_SYMBOL_FILE}")
list(APPEND PICONIX_EMBED_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/embed_symbols.S")

# Fixed-size Flash reservation for kernel symbols. A post-link pass fills it
# without shifting kernel code/data addresses.
set(PICONIX_KERNEL_SYMBOL_FILE "${CMAKE_CURRENT_BINARY_DIR}/piconix_kernel_symbols.bin")
# Keep the reservation consistent when upgrading an existing build directory.
# A size change forces the first link to rebuild the embedded catalog.
set(PICONIX_KERNEL_SYMBOL_CAPACITY 65536)
if(EXISTS "${PICONIX_KERNEL_SYMBOL_FILE}")
  file(SIZE "${PICONIX_KERNEL_SYMBOL_FILE}" PICONIX_KERNEL_SYMBOL_CURRENT_SIZE)
endif()
if(NOT EXISTS "${PICONIX_KERNEL_SYMBOL_FILE}" OR
   NOT PICONIX_KERNEL_SYMBOL_CURRENT_SIZE EQUAL PICONIX_KERNEL_SYMBOL_CAPACITY)
  execute_process(COMMAND "${Python3_EXECUTABLE}" -c
    "import pathlib;pathlib.Path(r'${PICONIX_KERNEL_SYMBOL_FILE}').write_bytes(bytes(${PICONIX_KERNEL_SYMBOL_CAPACITY}))"
    COMMAND_ERROR_IS_FATAL ANY)
endif()
configure_file("${CMAKE_CURRENT_LIST_DIR}/userland/embed_kernel_symbols.S.in"
               "${CMAKE_CURRENT_BINARY_DIR}/embed_kernel_symbols.S" @ONLY)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/embed_kernel_symbols.S"
  PROPERTIES OBJECT_DEPENDS "${PICONIX_KERNEL_SYMBOL_FILE}")
list(APPEND PICONIX_EMBED_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/embed_kernel_symbols.S")

# Kernel .text/.rodata execute from XIP flash on both Pico and Pico W.
# Only .data/.bss/time-critical code/stacks use the first 64 KiB SRAM.
# The default SDK startup initializes writable sections and RAM vectors.
set(PICONIX_SDK_XIP_LD "${PICO_SDK_PATH}/src/rp2_common/pico_crt0/rp2040/memmap_default.ld")
if(NOT EXISTS "${PICONIX_SDK_XIP_LD}")
  set(PICONIX_SDK_XIP_LD "${PICO_SDK_PATH}/src/rp2_common/pico_standard_link/memmap_default.ld")
endif()
if(NOT EXISTS "${PICONIX_SDK_XIP_LD}")
  message(FATAL_ERROR "Missing Pico SDK RP2040 XIP linker script: ${PICONIX_SDK_XIP_LD}")
endif()
set(PICONIX_CUSTOM_LD "${CMAKE_CURRENT_BINARY_DIR}/piconix_kernel_xip.ld")
execute_process(
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/limit_sdk_xip.py"
          "${PICONIX_SDK_XIP_LD}" "${PICONIX_CUSTOM_LD}"
  RESULT_VARIABLE PICONIX_LD_RESULT
  ERROR_VARIABLE PICONIX_LD_ERROR)
if(NOT PICONIX_LD_RESULT EQUAL 0)
  message(FATAL_ERROR "Cannot derive safe SDK XIP linker script: ${PICONIX_LD_ERROR}")
endif()
message(STATUS "PicoNIX XIP linker: ${PICONIX_CUSTOM_LD}")
add_executable(piconix kernel/net.c kernel/net_service_telnet.c kernel/net_service_remote_devctl.c kernel/net_socket.c kernel/kernel.c kernel/gpio.c kernel/fb.c kernel/adc.c kernel/pwm.c kernel/context.S kernel/main.c
                   ${PICONIX_EMBED_SOURCES})
pico_set_linker_script(piconix "${PICONIX_CUSTOM_LD}")
# Wi-Fi follows the selected board. Both boards use the same five-slot layout.
# Reject stale cache options instead of silently retaining an old Wi-Fi setting.
if(DEFINED PICONIX_WIFI_LAYOUT)
  message(FATAL_ERROR "PICONIX_WIFI_LAYOUT has been removed. Delete the build directory or unset the cached option (-U PICONIX_WIFI_LAYOUT); select -DPICO_BOARD=pico_w instead.")
endif()
if(PICO_BOARD STREQUAL "pico_w")
  set(PICONIX_WIFI_ENABLED ON)
  target_compile_definitions(piconix PRIVATE PICONIX_WIFI=1)
  target_link_libraries(piconix pico_cyw43_arch_lwip_poll)
else()
  set(PICONIX_WIFI_ENABLED OFF)
  target_compile_definitions(piconix PRIVATE PICONIX_WIFI=0)
endif()
message(STATUS "PicoNIX Wi-Fi: ${PICONIX_WIFI_ENABLED} (PICO_BOARD=${PICO_BOARD})")
# Validate writable sections against fixed task and network regions on Pico W.
if(PICONIX_WIFI_ENABLED)
  add_custom_command(TARGET piconix POST_BUILD
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/audit_wifi_elf.py"
            "$<TARGET_FILE:piconix>" --readelf "${CMAKE_READELF}"
    VERBATIM)
endif()
target_include_directories(piconix PRIVATE include "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/kernel")
option(PICONIX_CONSOLE_USB "Use onboard USB CDC instead of UART0 console" ON)
target_compile_definitions(piconix PRIVATE PICONIX_CONSOLE_USB=$<BOOL:${PICONIX_CONSOLE_USB}>)
target_link_libraries(piconix pico_stdlib hardware_uart hardware_gpio hardware_adc hardware_pwm)
if(PICONIX_CONSOLE_USB)
  target_link_libraries(piconix pico_stdio_usb)
endif()
if(PICONIX_CONSOLE_USB)
  pico_enable_stdio_usb(piconix 1)
else()
  pico_enable_stdio_usb(piconix 0)
endif()
pico_enable_stdio_uart(piconix 0)
pico_add_extra_outputs(piconix)

# Post-link inspection of five fixed 32-KiB task slots.
# Enable explicitly while transitioning from SDK's default XIP linker script.
option(PICONIX_VERIFY_LAYOUT "Check task RAM placement in linked ELF" ON)
if(PICONIX_VERIFY_LAYOUT)
  add_custom_command(TARGET piconix POST_BUILD
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/verify_layout.py"
            "$<TARGET_FILE:piconix>" --slots "5" --nm "${CMAKE_NM}"
    VERBATIM)
endif()

# Stage 16: inspect the *real* Pico SDK ELF, rather than only the
# freestanding layout probe. SDK boot2 and vectors are produced by the SDK.
# This is deliberately strict: a default SDK memory layout may fail, which
# indicates that a dedicated firmware linker script is still required.
option(PICONIX_AUDIT_FIRMWARE "Audit SDK boot2/vectors and fixed RAM slots" ON)
if(PICONIX_AUDIT_FIRMWARE)
  add_custom_command(TARGET piconix POST_BUILD
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/audit_firmware.py"
            "$<TARGET_FILE:piconix>" --slots "5" --nm "${CMAKE_NM}"
            --objdump "${CMAKE_OBJDUMP}"
    VERBATIM)
endif()

# Stage 20: explicit host-only RAM image regression target.
# Does not depend on Pico SDK runtime or RP2040 hardware. Requires LLVM tools.
add_custom_target(piconix_ram_image_selftest
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/test_scripts/test_ram_pipeline.py"
  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
  COMMENT "Check all six PIC Cortex-M0+ images with LLVM"
  VERBATIM)
