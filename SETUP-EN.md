# Setup

[日本語](SETUP-JP.md) | [Back to overview](README-EN.md)

## 1. Requirements

- Raspberry Pi Pico (RP2040) and a USB data cable
- Linux, Pico SDK 2.1.1, ARM GCC, CMake, and Python 3
- A USB connection for UF2 flashing and a serial terminal (such as `picocom`)

Install the SDK and toolchain first. The command below uses example installation paths.

## 2. Build

From the PicoNIX project root:

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
LC_ALL=C LANG=C \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DCMAKE_BUILD_TYPE=MinSizeRel
```

The script runs host checks, then configures and builds for `PICO_BOARD=pico` and `PICONIX_KERNEL_RAM=ON`. `MinSizeRel` (normally `-Os`) is the default; passing it explicitly avoids confusion with a previously configured `build/` directory.

Typical artifacts are `build/piconix.uf2` and `build/piconix.elf`. The build script does not automatically remove `build/`.

## 3. Flash the Pico

1. Connect the Pico while holding BOOTSEL.
2. Copy `build/piconix.uf2` to the `RPI-RP2` drive.
3. After reboot, open its USB CDC serial port (for example `/dev/ttyACM0` on Linux).

```sh
picocom -b 115200 /dev/ttyACM0
```

For USB CDC, `115200` is a terminal setting, not the physical UART bitrate. The device name depends on your system.

## 4. Smoke test

```text
piconix# help
piconix# ps
piconix# cat /sys/proc/uptime
piconix# blink &
piconix# ps
piconix# kill 2
```

PIDs are not fixed; use the PID shown by `ps`. Foreground `blink` can be stopped with `Ctrl+C`.

## 5. Troubleshooting

| Symptom | Check |
|---|---|
| SDK not found | `PICO_SDK_PATH` and SDK submodules |
| ARM toolchain not found | `PICO_TOOLCHAIN_PATH` and GCC installation |
| RAM overflow or link failure | Explicit `MinSizeRel` and fixed memory limits |
| No USB terminal output | Reopen the terminal after Pico re-enumerates |
| GPIO25 LED not blinking | Confirm you are using a standard Raspberry Pi Pico |

To choose a UART0 console, pass `-DPICONIX_CONSOLE_USB=OFF`. USB CDC is recommended by default. See [Internals](INTERNAL-EN.md) for the memory layout.
