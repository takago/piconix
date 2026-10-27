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
if(NOT CMAKE_READELF)
  get_filename_component(TOOLCHAIN_BIN_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
  set(CMAKE_READELF "${TOOLCHAIN_BIN_DIR}/arm-none-eabi-readelf")
endif()
# Freestanding slot-linked images. All four are embedded in the firmware Flash.
set(PICONIX_EMBED_SOURCES "")
foreach(APP IN ITEMS shell blink omikuji python)
  # Compile each application once for the same canonical address.
  # Every image must also link identically at the next 32-KiB slot address.
  set(APP_ORIGIN 0x20010000)
  set(APP_DIR "${CMAKE_CURRENT_BINARY_DIR}/ram_${APP}")
  file(MAKE_DIRECTORY "${APP_DIR}")
  set(APP_LD "${APP_DIR}/slot.ld")
  configure_file("${CMAKE_CURRENT_LIST_DIR}/ram_apps/slot.ld" "${APP_LD}" @ONLY)
  set(APP_OBJ "${APP_DIR}/${APP}.o")
  set(CRT_OBJ "${APP_DIR}/crt0.o")
  set(SYSCALLS_OBJ "${APP_DIR}/syscalls.o")
  set(APP_LD_ALT "${APP_DIR}/slot_alt.ld")
  set(APP_ORIGIN 0x20018000)
  configure_file("${CMAKE_CURRENT_LIST_DIR}/ram_apps/slot.ld" "${APP_LD_ALT}" @ONLY)
  set(APP_ORIGIN 0x20010000)
  set(APP_ELF_ALT "${APP_DIR}/${APP}_alt.elf")
  set(APP_BIN_ALT "${APP_DIR}/${APP}_alt.bin")
  set(APP_ELF "${APP_DIR}/${APP}.elf")
  set(APP_BIN "${APP_DIR}/${APP}.bin")
  # GCC may turn dense if/else comparisons into Thumb-1 switch tables.
  # Freestanding RAM images do not link libgcc (__gnu_thumb1_case_uqi).
  set(APP_EXTRA_CFLAGS "")
  if(APP STREQUAL "python")
    set(APP_EXTRA_CFLAGS -fno-jump-tables -fno-tree-switch-conversion)
  endif()
  add_custom_command(OUTPUT "${APP_BIN}"
    COMMAND "${CMAKE_C_COMPILER}" -mcpu=cortex-m0plus -mthumb -Os -fPIC
            -ffreestanding -fno-builtin -ffunction-sections -fdata-sections
            -fno-unwind-tables -fno-asynchronous-unwind-tables
            ${APP_EXTRA_CFLAGS}
            -c "${CMAKE_CURRENT_LIST_DIR}/ram_apps/${APP}.c" -o "${APP_OBJ}"
    COMMAND "${CMAKE_C_COMPILER}" -mcpu=cortex-m0plus -mthumb -Os -fPIC
            -ffreestanding -fno-builtin -ffunction-sections -fdata-sections
            -fno-unwind-tables -fno-asynchronous-unwind-tables
            -c "${CMAKE_CURRENT_LIST_DIR}/ram_apps/crt0.c" -o "${CRT_OBJ}"
    COMMAND "${CMAKE_C_COMPILER}" -mcpu=cortex-m0plus -mthumb -Os -fPIC
            -ffreestanding -fno-builtin -ffunction-sections -fdata-sections
            -fno-unwind-tables -fno-asynchronous-unwind-tables
            -I "${CMAKE_CURRENT_LIST_DIR}/ram_apps"
            -c "${CMAKE_CURRENT_LIST_DIR}/ram_apps/syscalls.c" -o "${SYSCALLS_OBJ}"
    COMMAND "${CMAKE_C_COMPILER}" -nostdlib
            -Wl,--gc-sections "-Wl,-z,max-page-size=0x1000" "-Wl,-T,${APP_LD}"
            "${CRT_OBJ}" "${APP_OBJ}" "${SYSCALLS_OBJ}" -o "${APP_ELF}"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "${APP_ELF}" "${APP_BIN}"
    COMMAND "${CMAKE_C_COMPILER}" -nostdlib
            -Wl,--gc-sections "-Wl,-z,max-page-size=0x1000" "-Wl,-T,${APP_LD_ALT}"
            "${CRT_OBJ}" "${APP_OBJ}" "${SYSCALLS_OBJ}" -o "${APP_ELF_ALT}"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "${APP_ELF_ALT}" "${APP_BIN_ALT}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/check_pic.py"
            "${APP_ELF}" "${APP_BIN}" "${APP_ELF_ALT}" "${APP_BIN_ALT}"
            --readelf "${CMAKE_READELF}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/check_slot_image.py"
            "${APP_ELF}" "${APP_BIN}" "${APP_ORIGIN}" --readelf "${CMAKE_READELF}"
    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/ram_apps/${APP}.c"
            "${CMAKE_CURRENT_LIST_DIR}/ram_apps/crt0.c"
            "${CMAKE_CURRENT_LIST_DIR}/ram_apps/syscalls.c"
            "${CMAKE_CURRENT_LIST_DIR}/ram_apps/unistd.h"
            "${CMAKE_CURRENT_LIST_DIR}/ram_apps/fcntl.h"
            "${APP_LD}" "${APP_LD_ALT}"
            "${CMAKE_CURRENT_LIST_DIR}/tools/check_pic.py"
            "${CMAKE_CURRENT_LIST_DIR}/tools/check_slot_image.py"
    VERBATIM)
  set(EMBED_APP ${APP})
  set(EMBED_BIN "${APP_BIN}")
  configure_file("${CMAKE_CURRENT_LIST_DIR}/ram_apps/embed_image.S.in"
                 "${APP_DIR}/embed_image.S" @ONLY)
  set_source_files_properties("${APP_DIR}/embed_image.S" PROPERTIES OBJECT_DEPENDS "${APP_BIN}")
  list(APPEND PICONIX_EMBED_SOURCES "${APP_DIR}/embed_image.S")
endforeach()
# Stage 17: experimental *real* SDK RAM execution path.
# Pico SDK's copy_to_ram boot path retains the SDK boot2 and startup support.
# We generate a copy of the SDK script with RAM restricted to bank 0 (64 KiB).
# The remaining three banks provide six fixed 32-KiB task slots.
option(PICONIX_KERNEL_RAM "Run SDK firmware from kernel SRAM bank 0" ON)
if(PICONIX_KERNEL_RAM)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  set(PICONIX_SDK_RAM_LD "${PICO_SDK_PATH}/src/rp2_common/pico_crt0/rp2040/memmap_copy_to_ram.ld")
  if(NOT EXISTS "${PICONIX_SDK_RAM_LD}")
    set(PICONIX_SDK_RAM_LD "${PICO_SDK_PATH}/src/rp2_common/pico_standard_link/memmap_copy_to_ram.ld")
  endif()
  if(NOT EXISTS "${PICONIX_SDK_RAM_LD}")
    message(FATAL_ERROR "Missing Pico SDK RAM linker script: ${PICONIX_SDK_RAM_LD}")
  endif()
  set(PICONIX_CUSTOM_LD "${CMAKE_CURRENT_BINARY_DIR}/piconix_kernel_ram.ld")
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/limit_sdk_ram.py"
            "${PICONIX_SDK_RAM_LD}" "${PICONIX_CUSTOM_LD}"
    RESULT_VARIABLE PICONIX_LD_RESULT
    OUTPUT_VARIABLE PICONIX_LD_OUTPUT
    ERROR_VARIABLE PICONIX_LD_ERROR)
  if(NOT PICONIX_LD_RESULT EQUAL 0)
    message(FATAL_ERROR "Cannot derive safe SDK linker script: ${PICONIX_LD_ERROR}")
  endif()
  message(STATUS "PicoNIX RAM linker: ${PICONIX_CUSTOM_LD}")
endif()
add_executable(piconix src/kernel.c src/context.S src/main.c
                   ${PICONIX_EMBED_SOURCES})
if(PICONIX_KERNEL_RAM)
  # SDK supplies boot2, initial vectors and flash-to-RAM relocation.
  pico_set_binary_type(piconix copy_to_ram)
  pico_set_linker_script(piconix "${PICONIX_CUSTOM_LD}")
endif()
target_include_directories(piconix PRIVATE include)
option(PICONIX_CONSOLE_USB "Use onboard USB CDC instead of UART0 console" ON)
target_compile_definitions(piconix PRIVATE PICONIX_CONSOLE_USB=$<BOOL:${PICONIX_CONSOLE_USB}>)
target_link_libraries(piconix pico_stdlib hardware_uart hardware_gpio)
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

# Post-link inspection of the three fixed 64-KiB task reservations.
# Enable explicitly while transitioning from SDK's default XIP linker script.
option(PICONIX_VERIFY_LAYOUT "Check task RAM placement in linked ELF" ON)
if(PICONIX_VERIFY_LAYOUT)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_custom_command(TARGET piconix POST_BUILD
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/verify_layout.py"
            "$<TARGET_FILE:piconix>" --nm "${CMAKE_NM}"
    VERBATIM)
endif()

# Stage 16: inspect the *real* Pico SDK ELF, rather than only the
# freestanding layout probe. SDK boot2 and vectors are produced by the SDK.
# This is deliberately strict: a default SDK memory layout may fail, which
# indicates that a dedicated firmware linker script is still required.
option(PICONIX_AUDIT_FIRMWARE "Audit SDK boot2/vectors and fixed RAM slots" ON)
if(PICONIX_AUDIT_FIRMWARE)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_custom_command(TARGET piconix POST_BUILD
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/audit_firmware.py"
            "$<TARGET_FILE:piconix>" --nm "${CMAKE_NM}"
            --objdump "${CMAKE_OBJDUMP}"
    VERBATIM)
endif()

# Stage 20: explicit host-only RAM image regression target.
# Does not depend on Pico SDK runtime or RP2040 hardware. Requires LLVM tools.
add_custom_target(piconix_ram_image_selftest
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/test_ram_pipeline.py"
  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
  COMMENT "Check Shell/Blink PIC Cortex-M0+ images with LLVM"
  VERBATIM)
