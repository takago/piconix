# PicoNIX Setup Guide

[日本語](SETUP-JP.md) | [English](SETUP-EN.md)

## 1. Requirements

- Raspberry Pi Pico (RP2040) and a USB data cable.
- Pico SDK 2.1.1, including its submodules.
- ARM GCC toolchain (for example, `~/.pico-sdk/toolchain/14_2_Rel1`).
- CMake, Make, Python 3, Git, and a USB serial terminal.

These instructions assume an Ubuntu-family Linux environment. Install the SDK and compiler before building.

## 2. Obtain the sources

Clone your PicoNIX repository or extract the source archive, then change into its root directory (the directory containing `CMakeLists.txt`).

## 3. Build

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
LC_ALL=C LANG=C \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DCMAKE_BUILD_TYPE=MinSizeRel
```

`build_sdk.sh` first runs host-side audit and RAM-image tests, then configures and builds the SDK firmware. `MinSizeRel` is the standard build type and optimizes for code size (`-Os`). Other CMake options can be supplied at the end of the command.

The host LLVM RAM-image test validates Shell and Blink. Omikuji and Python must additionally be validated with the ARM GCC SDK build; a successful host test alone does not establish that all four applications link.

## 4. Firmware output and flashing

Find the generated `piconix.uf2` in `build/`. Hold BOOTSEL when connecting the Pico, then copy the UF2 to its mounted drive. The board reboots automatically.

## 5. Serial console and smoke test

Open the USB CDC serial interface with your preferred terminal. After enumeration, the PicoNIX startup banner and `piconix#` prompt should appear.

```sh
help
ps
ls /dev
cat /sys/proc/tasks
blink
```

The default console is USB CDC. An alternative UART0 console build is selected by `-DPICONIX_CONSOLE_USB=OFF`.

## 6. Troubleshooting

If the banner does not appear, check that your serial terminal is connected to the correct USB CDC device and that enumeration has finished. Reconnect or reset the Pico if necessary. If configuration fails, verify `PICO_SDK_PATH`, the ARM compiler path, SDK submodules, and that the build type is `MinSizeRel`.

For the RAM layout and application startup process, read [Internal Architecture](INTERNAL-EN.md).
